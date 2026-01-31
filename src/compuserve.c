/*
 * compuserve.c -- transaction robot for CompuServe
 *
 * This version was written for the 4D(62) release of the Forum software.
 *
 * It expects you to be in Command mode and using EDIT for both Forums and
 * EasyPlex (so that there are no line prompts while entering text). It
 * expects EasyPlex to be in unpaged mode.
 *
 * If you're reading this as a model for building a new robot, also consider
 * the debug levels D_* defined at the head of the bbsauto.c file.
 */
/*LINTLIBRARY*/
#include "news.h"
#include "libpriv.h"
#include "header.h"
#include "bbsauto.h"

forward void clogon(), cread(), cpost(), clogoff(),
			cistonews(), cistomail(), cresynch();

bbstype compuserve =
{
    "CompuServe",	/* name of service */
    clogon,		/* logon code */
    cread,		/* read-side interpreter */
    cpost,		/* per-article post code */
    clogoff,		/* logoff code */
    cistonews,		/* Forum to news translation */
    cistomail,		/* EasyPlex to UNIX-mail format */
    cresynch,		/* resynchronization function */
};

/* constants for general CIS interaction */
#define C_NPWD	"?? LOGINE"	/* login error, password no good */
#define C_BANG	" !"		/* Standard command prompt */
#define C_CONT	"Press <CR>!"	/* Standard continuation prompt */

/* EasyPlex info and error-message strings */
#define E_MAIL	"You have Electronic Mail waiting.\r\n"
#define E_DOWN	"% EasyPlex is temporarily unavailable\r\n"
#define E_NMSG	"% No messages pending\r\n"	/* Not currently used */
#define E_GCMD	"\r\nEasyPlex! "		/* Easyplex top-level prompt */
#define E_ACTP	"\r\n  Action! "		/* Easyplex action prompt */
#define E_UPLD	"Enter message. (/EXIT when done)\r\n"
#define E_SUBJ	"\r\nSubject: "			/* Easyplex subject prompt */
#define E_NAME	"Send to (Name or User ID): "	/* Name/user ID prompt */
#define E_BANG	"! "				/* Easyplex prompt */
#define E_SUBH	"Subj: "			/* Subject header */
#define E_DATE	"Date: "			/* Date header */

/* Forum software constants */
#define FUNIQUE	6		/* # significant chars in forum names */
#define MAXSUBJ	24		/* max chars of Sb: header */
#define MAXMSG	2000		/* max chars per message chunk */
#define F_PRIV "(P)"		/* CompuServe private-mail marker */
#define F_FLSH	"News Flash:\r\n"	/* Forum News Flash tag */
#define F_GCMD	"\r\nForum !"	/* Forum top-level prompt */
#define F_MSGS	"Messages !"	/* Forum message prompt */
#define F_UPLD	"(/EXIT when done)\r\n"
#define F_UNRE	"is unrecognized !"

/* this stuff helps implement TAPCIS continuation conventions */
#define MORE	"[More]"
#define CONT	"[Continued]"

/* internal constants used by the robot -- can be changed freely */
#define FORUM	"FORUM: "	/* Forum marker inserted by read dialogue */
#define MAXSPLIT	20	/* Maximum split parts for long messages */

static bool havemail = FALSE;	/* do we have EasyPlex mail waiting? */

static void clogon()
/* go through normal login sequence */
{
    static char *logresp[] = {C_BANG, E_MAIL, C_NPWD};

    send("\r\n");		/* give host a chance to autobaud */
    send("\r\n");
    expect("Host Name: "); send("%s\r\n", remote.hostname);
    expect("User ID: "); send("%s\r\n", remote.logon);
    expect("Password: ");
    echo(OFF); send(remote.password); echo(ON);
    send("\r\n");

    switch(await(sizeof(logresp)/sizeof(char *), logresp, 10, 1))
    {
    case 0:			/* found command prompt */
	havemail = FALSE;
	break;
    case 1:
	havemail = TRUE;	/* found mail-waiting indication */
	expect(C_BANG);
	break;
    default:			/* password failed */
	xerror1("password for %s no good!", remote.servname);
	break;
    }
}

static int gotoforum(forum)
/* go to given forum, get a command prompt there */
char	*forum;
{
    static char	cforum[IDSIZE];

    if (strcmp(cforum, forum))
    {
	send("GO %s\r\n", forum);
	expect("!");
	if (saw(F_UNRE) != FAIL)
	    return(FAIL);
	else if (saw(C_CONT) != FAIL)
	{
	    send("\r\n");
	    expect("!");
	}

	/*
	 * We must do this to compensate for EasyPlex's bogus nonstandard
	 * prompts. Let's hope this changes someday...
	 */
	if (strcmp("EASYPLEX", forum) == 0)
	    expect(" ");

	(void) strlcpy(cforum, forum, sizeof(cforum));
	return(TRUE);
    }
#ifdef DEBUG
    else if (debug)
	log1("already at forum %s", forum);
#endif /* DEBUG */
    return(FALSE);
}

static void cread(newsfp, mailfp)
/* transaction interpreter for CompuServe */
FILE	*newsfp;	/* where to put captured news */
FILE	*mailfp;	/* where to put captured mail */
{
    int		i;

    /* first, go read mail if there's any waiting */
    if (havemail)
    {
	static char	*eplpr[] = {E_DOWN, E_GCMD};

	(void) gotoforum("EASYPLEX");
	switch (await(sizeof(eplpr)/sizeof(char *), eplpr, 10, 1))
	{
	case 0:			/* found E_DOWN, try to recover */
	    expect(C_CONT);
	    send("\r\n");
	    errmail("EasyPlex was down");
	    break;

	default:		/* saw E_GCMD, EasyPlex is up */
	    send("READ ALL\r\n");
	    for (;;)
	    {
		capture(E_BANG, mailfp);
		if (saw(E_ACTP) != FAIL)
		{
		    remote.nmail++;
		    send("\r\n");
		    (void) fputs("\r\n", mailfp);
		}
		else if (saw(E_GCMD) != FAIL)
		    break;
	    }
	    send("DEL ALL\r\n");
	    expect(E_BANG);
	    send("T\r\n");
	    expect(C_BANG);
	    break;
	}
    }

    /* now read `news' (forum postings) */
    for (i = 0; i < remote.ntopics; i++)
    {
	if (gotoforum(remote.topics[i]) == FAIL)
	{
	    errmail("%s is not a recognized forum", remote.topics[i]);
	    continue;
	}
	send("OPTION;PAUSE NEVER;S\r\n");
	(void) fprintf(newsfp, "%s %s\n\n", FORUM, remote.topics[i]);
	expect(F_GCMD); send("MESSAGES;READ NEW\r\n");
	(void) capture(F_MSGS, newsfp); 
    }    

    /* force us to top level so read messages get registered */
    send("T\r\n");
    expect(C_BANG);
}

/* postpart() modes */
#define M_POST	0	/* post to a forum */
#define M_REPLY	1	/* reply to a forum message */

static char *postpart(mode, target, start, end, partno, parts)
/* post a given section of the current article, return the generated ID */
bool	mode;		/* see defined M_* constants */
char	*target;	/* target of message (section, parent id, or user) */
off_t	start, end;	/* start/end offsets of section */
int	partno, parts;	/* part # of this part, total part count */
{
    static char idbuf[IDSIZE];
    static const char *responses[] =
    {
#define P_POSTOK	0
	"\r\nMessage # ",
#define P_NOAUTH	1
	"% You are not authorized for section",
    };

    /* start upload sequence */
    if (mode == M_REPLY)
	send("REPLY %s\r\n", target);
    else
	send("COMPOSE\r\n");

    /* do ASCII upload of message text */
    expect(F_UPLD);
    if (parts > 1 && partno > 1)
	send(".%s                 (%d of %d)\r\n", CONT, partno, parts);
    upload(header.h_fp, start, end, M_LFTOCRLF);
    if (parts > 1 && partno < parts)
	send(".%s                 (%d of %d)\r\n", MORE, partno, parts);

    /* generate and send the submission command */
    switch (mode)
    {
    case M_POST:
	send("/POST TO:\"%s\" SUB:\"%s\" SEC:%s\r\n",
	     hlnblank(header.h_to) ? header.h_to : "ALL",
	     header.h_subject, target);
	break;

    case M_REPLY:
	send("/POST\r\n");
	break;
    }

    /* collect response to tell us if post was OK */
    switch(await(sizeof(responses) / sizeof(char *), responses, 10, 1))
    {
    case P_POSTOK:
	expect("\r\n");
	(void) sscanf(remote.lastline, "%s posted", idbuf);
#ifdef DEBUG
	if (debug)
	    log1("message posted ok with ID %s", idbuf);
#endif /* DEBUG */
	expect(C_BANG);
	return(idbuf);

    case P_NOAUTH:
	logerr1("CIS authorization failure in %s", header.h_newsgroups);
	expect("Post action !");
	send("CAN\r\n");
	expect("(Y or N)? ");
	send("Y\r\n");
	expect(C_BANG);
	errmail("authorization failure for target forum");
	return((char *)NULL);
    }
}

static void cpost()
/* post the article or EasyPlex mail defined by a header */
{
    if (hlblank(header.h_newsgroups))
    {
	int ugroup, unum;
	char	*cp, *tp;

	gotoforum("EASYPLEX");
	send("COMPOSE\r\n");
	expect(E_UPLD);

	upload(header.h_fp, header.h_textoff, header.h_endoff, M_LFTOCRLF);

	send("/EXIT\r\n");
	expect(E_NAME);
	cp = header.h_to;
	tp = bfr;
	while (*cp)
	{
	    if (*cp == '@')
	    {
		while (*cp && !isspace(*cp))
		    cp++;
		*tp++ = ';';
	    }
	    else if (*cp == '(')
	    {
		while (*cp && *cp != ')')
		    cp++;
		cp++;
	    }
	    else if (*cp == '.')
	    {
		*tp++ = ',';
		cp++;
	    }
	    else
		*tp++ = *cp++;
	}
	*tp = '\0';
	if (cp = strrchr(bfr, ';'))
	    *cp = '\0';
	(void) strlcat(bfr, "\r\n", LBUFLEN);
	send(bfr);
	expect(E_SUBJ);
	send("%s\r\n", header.h_subject);
	expect("(Y or N)! ");
	send("Y\r\n");
    }
    else
    {
	static char ngsep[] = {NGDELIM, ' ', '\0'};
	char forum[IDSIZE],section[IDSIZE],scratch[IDSIZE],parent[IDSIZE],*grp;
	int fgood;

	header.h_subject[MAXSUBJ] = '\0';	/* CIS is soooo brain-dead! */

	/* there are no 'cross-posts' in the CIS world */
	grp = strtok(header.h_newsgroups, ngsep);
	do {
	    bool	isreply;
	    int		i, nparts, parts[MAXSPLIT + 1];
	    char	*target;

	    /*
	     * First, make sure current group is a valid forum section
	     * This method may cause problems someday if we want to implement
	     * nicer aliases for CIS groups.
	     */
	    if (sscanf(grp,
		       "cis.%[abcdefghijklmnopqrstuvwxyz0123456789].s%s",
		       forum, section) != 2)
	    {
		errmail("couldn't parse a forum name out of", grp);
		continue;
	    }

	    /* to post to a forum, we must go there */
	    if ((fgood = gotoforum(forum)) == FAIL)
	    {
		errmail("%s is not a recognized forum name", forum);
		continue;
	    }
	    else if (fgood)
	    {
		send("MESSAGES\r\n");
		expect(F_MSGS);
	    }

	    isreply = (sscanf(header.h_references,
		       "References: <%[a-z]%[0-9]@compuserve>",
		       scratch, parent) == 2);
	    nparts = msplit(MAXMSG, MAXSPLIT, parts);
	    if (isreply)
		target = postpart(M_REPLY,parent, parts[0],parts[1], 1,nparts);
	    else
		target = postpart(M_POST,section, parts[0],parts[1], 1,nparts);
	    for (i = 1; i < nparts; i++)
		target = postpart(M_REPLY, target,
				  parts[i], parts[i + 1], i + 1, nparts);
	} while
	    (grp = strtok((char *)NULL, ngsep));
    }
}

static void clogoff()
/* log us off CompuServe */
{
    send("OFF\r\n");
    expect("\004");	/* look for trailing EOT */
}

static void cresynch()
/* try to resynchronize following error */
{
    char	*uexpect = "^U\r\n";

    echo(OFF);
    send("\025\r\n");	/* ^U to clear line */
    echo(ON);
    (void) await(1, &uexpect, 10, 1);
}

/*
 * Here is a description of the header-munging rules used by cisnews.
 *
 * We assume the following CompuServe header syntax:
 *
 * #: <msgnum> [(P)] S<sectnum>/<sectname>
 *     <local-date>
 * Sb: [#<threadnum>-]<subject>
 * Fm: <from-name> <grp-num>,<user-num>
 * To: <to-name> <grp-num>,<user-num> [(X)]
 *
 * Here's what we do with it:
 *
 * The ID line is used to generate an article-id for the article. The site
 * part is always compuserve; the local-ID part concatenates the significant
 * part of the forum name with the message number.
 *
 * The ID line is also used to qualify the pseudo-group the article is posted
 * to. Each CompuServe forum is treated as a second-level hierarchy under cis,
 * so that (for example) Section 3 of UNIXFORUM becomes cis.unixforum.s3. The
 * section name fills the article's Keywords field; it can't be used as a
 * newsgroup name directly because section names are long and may contain
 * whitespace and punctuation. It is recommended that the sysop add entries
 * to the LIB/aliases file to map forum sections to sensible names.
 *
 * The privacy-marker causes the article to be redirected to the user's
 * mailbox rather than posted as news. The Posted-To header referred to below
 * becomes a To: header filled with the *local* UNIX username of the person
 * owning the CompuServe account.
 *
 * The Date line (which Compuserve corrects for local node timezone) becomes
 * the Posted-Date.
 *
 * The Subject line is used to fill in the netnews Subject header. If a thread
 * number of a parent article is present, it is used to fake an ID in the
 * way described above, and the ID fills a References header.
 *
 * The Fr: line is used to generate a From line. The sender name is stuck in
 * comment parens following a faked Internet name for the sender. As with the
 * site ID, the site part is always compuserve; the user part consists of the
 * user's CompuServe account number with the comma replaced by a period (RFC-
 * 822 does not allow commas in user-name parts). 
 *
 * The To: line is copied into an information-only header, not recognized by
 * other netnews software, called "Posted-To" -- unless the article is a
 * 'private' posting (see above under the ID line description).
 */

static void mkid(buf, bufsize, grp, num)
/* fake up an Internet-style message-ID corresponding to a forum/number pair */
char	*buf;
size_t	bufsize;
char	*grp;
int	num;
{
    (void) strlcpy(buf, "<", bufsize);
    (void) strlcat(buf, grp, bufsize);  /* limited by bufsize, not FUNIQUE */
    (void) snprintf(buf + strlen(buf), bufsize - strlen(buf), "%d@compuserve>", num);
}

static void cistonews(ifp, nfp, mfp)
/* tranform a batch of CompuServe news with FORUM lines into netnews form */
FILE *ifp;	/* CompuServe news batch */
FILE *nfp;	/* news batch file */
FILE *mfp;	/* mail batch file */
{
    char	line[BUFSIZ], cgrp[BUFSIZ], str[BUFSIZ];
    int		msgnum, sectnum;
    FILE	*ofp = (FILE *)NULL;
    bool	blankflag = TRUE;

    while (fgets(line, BUFSIZ, ifp) != (char *)NULL)
    {
	/* a forum message prompt means we're at end of batch */
	if (strcmp(line, F_MSGS) == 0)
	    break;

	/* interpret forum-change lines */
	if (blankflag && (sscanf(line, "FORUM: %s\n", cgrp)== 1
			|| sscanf(line, "There is %d Reply.\n",&msgnum)==1
			|| sscanf(line, "There are %d Replies.\n",&msgnum)==1))
	{
	    blankflag = TRUE;
	    continue;
	}

	/* continuation line means we want to blow right by next header */
	else if (!strncmp(line, MORE, sizeof(MORE)))
	{
	    while (fgets(line, BUFSIZ, ifp) != (char *)NULL)
		if (!strncmp(line, CONT, sizeof(CONT)))
		    break;
	    continue;
	}

	/* interpret CompuServe article headers */
	else if (sscanf(line, "#: %d S%d/%[^\n]", &msgnum, &sectnum, str) == 3
		|| sscanf(line,"#: %d (P) S%d/%[^\n]",&msgnum,&sectnum,str)==3)
	{
	    /* if this is a 'private' posting, treat like mail */
	    ofp = (strindex(line, F_PRIV) == FAIL) ? nfp : mfp;
	    if (ofp == nfp)
		remote.nnews++;
	    else
		remote.nmail++;

	    /* required headers that we fake for compuserve 'news' */
	    hfree(&header);

	    /* some headers we only want if this will really become news */
	    if (ofp == nfp)
	    {
		hlcpy(header.h_path, "compuserve");
		(void) snprintf(bfr, LBUFLEN, "cis.%s.s%d", cgrp, sectnum);
		hlcpy(header.h_newsgroups, bfr);

		/* generate an article-ID from the message number */
		mkid(bfr, LBUFLEN, cgrp, msgnum);
		hlcpy(header.h_ident, bfr);

		/* wedge section name into Keywords for now */
		hlcpy(header.h_keywords, str);
	    }

	    /* date line should follow header */
	    if (fgets(line, BUFSIZ, ifp) == (char *)NULL)
		header.h_posttime = FAIL;
	    else
		header.h_posttime = getdate(line, (struct timeb *)NULL);

	    /* subject line should follow date */
	    if (fgets(line, BUFSIZ, ifp) != (char *)NULL)
	    {
		int	threadnum;

		if (sscanf(line, "Sb: #%d-%[^\n]", &threadnum, str) == 2)
		{
		    (void) snprintf(line, sizeof(line), "Re: %s", str);
		    hlcpy(header.h_subject, line);
		    mkid(bfr, LBUFLEN, cgrp, threadnum);
		    hlcpy(header.h_references, bfr);
		}
	        else	/* parent message of this thread expired */
	 	{
		    char	*cp;

		    (void) nstrip(line);
		    for (cp = line + strlen("Sb: "); *cp; cp++)
			if (*cp != '#' && *cp != '-')
			    break;
		    hlcpy(header.h_subject, cp);
		    threadnum = 0;
		}
	    }

	    /* from line should follow subject */
	    if (fgets(line, BUFSIZ, ifp) != (char *)NULL)
	    {
		int	ugrp, unum;

		if (sscanf(line, "Fm: %[^0123456789]%d,%d",str,&ugrp,&unum)==3)
		{
		    (void) nstrip(str);
		    (void) snprintf(bfr, LBUFLEN, "%d.%d@compuserve (%s)",ugrp,unum,str);
		    hlcpy(header.h_from, bfr);
		}
	    }

	    /* to line should follow from */
	    if (fgets(line, BUFSIZ, ifp) != (char *)NULL)
	    {
		int	ugrp, unum;

		/* it may be directed to a specific person */
		if (sscanf(line, "To: %[^0123456789]%d,%d",str,&ugrp,&unum)==3)
		{
		    (void) nstrip(str);
		    if (ofp == nfp)
		    {
			(void) snprintf(bfr, LBUFLEN,
				   "Posted-To: %d.%d@compuserve (%s)",
				   ugrp, unum, str);
			happend(&header, bfr);
		    }
		    else
			hlcpy(header.h_to, remote.owner);
		}

		/* otherwise it's to ALL, treat normally */
	    }

	    /* write the generated header */
	    (void) hwrite(&header, ofp, TRUE);

	    blankflag = TRUE;
	}

	/* ignore leading cruft and adjacent blank lines */
	else if ((blankflag && line[0] == '\n') || ofp == (FILE *)NULL)
	    continue;

	/* it's failed all special tests, just pass it through */
	else
	{
	    (void) fputs(line, ofp);
	    blankflag = (line[0] == '\n');
	}
    }
}

/*
 * Here is a description of the header-munging rules used by cismail.
 *
 * We assume the following CompuServe header syntax:
 *
 * Date: <date>
 * Subj: <subject>
 * From: <from-name> [<grp-num>,<user-num>]
 *
 * We transform this to the following RFC-822-compatible form
 *
 * From: <grp-num>.<user-num>@compuserve (<from-name>)
 * Date: <rfc-822-date>
 * Subject: <subject>
 */

static void cistomail(ifp, ofp)
/* mung a batch of EasyPlex mail into RFC822 mailbox form with delimiters */
FILE *ifp, *ofp;
{
    char	str[BUFSIZ], from[BUFLEN];
    bool	blankflag = TRUE;
    time_t	posttime;

    while (fgets(bfr, BUFSIZ, ifp) != (char *)NULL)
    {
	int	ugrp, unum;

	if (prefix(bfr, E_GCMD + 2))
	    break;
	else if (prefix(bfr, E_ACTP + 2))
	{
	    blankflag = TRUE;
	    (void) fputs(MDELIM, ofp);
	}
	else if (prefix(bfr, E_DATE))
	    posttime = getdate(bfr + sizeof(E_DATE) - 1, (struct timeb *)NULL);
	else if (sscanf(bfr, "From: %[^[] [%d,%d]", str, &ugrp, &unum) == 3)
	{
	    char	*cp = str + strlen(str) - 1;

	    while (isspace(*cp))
		*cp-- = '\0';
	    (void) snprintf(from, sizeof(from), "%d.%d@compuserve (%s)", ugrp, unum, str);
	}
	else if (prefix(bfr, E_SUBH))
	{
	    /* write header out in RFC-822 form */
	    (void) fprintf(ofp, "From: %s\n", from);
	    (void) fprintf(ofp, "Date: %s\n", arpadate(&posttime));    
	    (void) fprintf(ofp, "Subject: %s\n", bfr + sizeof(E_SUBH) - 1);
	}
	else if (!blankflag || bfr[0] != '\n')
	{
	    (void) fputs(bfr, ofp);
	    blankflag = (bfr[0] == '\n');
	}
    }
}

/* compuserve.c ends here */
