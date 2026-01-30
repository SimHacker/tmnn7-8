/*****************************************************************************

NAME
   postnews -- post a news message to USENET

SYNOPSIS
   postnews [-s] [-t title] [-n groups] [-d dist] [-k keywords] [-X]
   postnews [original] [-s] [-o] [-f original] [-F [includemark]]
   postnews -i

DESCRIPTION
   This is the program normally used to submit articles to USENET. It will
invoke your favorite editor and post the results of the edit session.

   For details on the command-line options, see the postnews(1) man page.

   If NONLOCAL is enabled, the active-newsgroup-list command probably won't
work, because the relevant newsgroup list file isn't guaranteed to exist.
Also, we can't generally check distributions, because current network servers
doesn't know about them. Whatever network service library you link postnews 
with is expected to define two entry points getgroups() & getdistribs()
that fill a given buffer with the names of a temp files holding association
lists of (respectively) active-group names or distribution names with plain-
English descriptions.

FILES
   /tmp/incltemp??????	-- include-mode copy of article text

SEE ALSO
   editart.c	-- user-interface-indepent editing functions
   newpost.c	-- user-interface-independent posting functions
   inews.c	-- posting front end to rnews

REVISED BY
   Eric S. Raymond
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

*****************************************************************************/
/*LINTLIBRARY*/
#include "news.h"
#include "vio.h"
#include "header.h"
#include "dballoc.h"
#include "alist.h"
#include "active.h"
#include "history.h"
#include "feeds.h"
#include "libpost.h"
#include "procopts.h"
#ifdef FASCIST
#include "fascist.h"
#endif /* FASCIST */

/*
 * We assume that anything over this size will go over 64K once all the
 * transport-layer headers are tacked on, and consequently cause a) extreme
 * UUCP sluggishness and b) some older conforming news implementations to
 * barf (we only talk about the second problem in the warning message).
 */
#define MAXSIZE	65000

/*
 * Any completed article under this size is assumed to be empty or
 * bogus (normal headers would make it larger).
 */
#define MINSIZE	5

#ifdef VMS
#define unlink vmsdelete
#endif /* VMS */

#define INCLTEMP	"/tmp/incltempXXXXXX"

private char original[BUFLEN];	/* reference to original, used in followup */
private char	ccname[BUFLEN];	/* file name for article copy */
private cite_t	*cite;		/* article id and poster of original */
private char	*article;		/* name of the article tempfile */
private char inclmark[SBUFLEN] = "> ";	/* included-text mark */

private char ngsep[] = { NGDELIM, '\0' };	/* == "," */

char *Progname = "postnews";		/* for xerror */

private int cflag, sflag, outfd, xpert;
private char	*newid;

/* read program modes */
#define DNC	    0x00    /* state not set, we don't care */
#define POST	    0x01    /* we're posting an original article */
#define FOLLOW	    0x02    /* we're following up to another article */
#define INCL	    0x04    /* include direct (don't prompt for headers) */

private option_t postopts[] =
{ /*
opt  filchar flag   old    new    type    buf	        meaning   */
'f', '\0',   NONE,  FOLLOW,FOLLOW,STRING, original,     /* followup to this */
'd', '\0',   NONE,  POST,  POST,  STRING, header.h_distribution,
'i', '\0',   NONE,  INCL,  INCL,  STRING, original,	/* include direct */
'k', '\0',   NONE,  POST,  POST,  STRING, header.h_keywords,
'n', NGDELIM,NONE,  POST,  POST,  STRING, header.h_newsgroups,
'o', '\0',   &outfd,DNC,   DNC,   NUMBER, (char *)NULL, /* write back ID */
's', '\0',   &sflag,DNC,   DNC,   OPTION, (char *)NULL, /* no signature */
't', ' ',    NONE,  POST,  POST,  STRING, header.h_subject,
'F', '\0',   &cflag,DNC,   FOLLOW,STRING, inclmark,	/* include copy? */
'X', '\0',   &xpert,DNC,   DNC,   OPTION, (char *)NULL,	/* expert mode */
#ifdef DEBUG
'D', '\0',   &debug,DNC,   DNC,   NUMBER, (char *)NULL, /* debug mode */
#endif /* DEBUG */
'\0','\0',   0,	    0,	   0,    0,       (char *)NULL
};

ALIST(distr, 20, 10)     /* declare the distribution control structure */

private void get_newsgroups()
/* ask for a list of topic groups; put them in header.h_newsgroups */
{
    int		n;
    static int	first = TRUE;
    char	*actlst, *cp;
    forward bool	valid_ng(), isflex();

    if (hlnblank(header.h_newsgroups))   /* user specified -n option */
	return;

    msg0("Newsgroups (enter one at a time, end with a blank line):");
    if (!xpert && first)
    {
	msg0("\nThe most relevant newsgroup should be the first, please add");
	msg0("others only if your article really MUST be read by people who");
	msg0("choose not to see the appropriate group for your article; but");
	msg0("DO use multiple newsgroups rather than posting many times.");
	first = FALSE;
    }
#ifndef NONLOCAL
    msg0("For a list of newsgroups, type ?");
    Sprint1(actlst, "%s/newsgroups", site.admdir);
#endif /* NONLOCAL */
    n = 0;
    hlfree(header.h_newsgroups);

    for(;;)
    {
	if (*(cp = vgetcmd("> ")) == '\0')
	{
#ifdef NONLOCAL
	    if (actlst[0])
		(void) unlink(actlst);
#endif /* NONLOCAL */
	    (void) free(actlst);
	    break;
	}
	else if (cp[0] == '?')
	{
#ifdef NONLOCAL
	    if ((actlst = getgroups()) == FAIL)
		msg0("No active groups list available");
	    else
#endif /* NONLOCAL */
		(void) more(actlst, (FILE *)NULL, 0);
	}
	else if (valid_ng(cp))
	{
	    if (n++ != 0)
		(void) strcat(header.h_newsgroups, ngsep);
	    (void) strcat(header.h_newsgroups, cp);
	}
	else if (isflex(cp))
	{
	    msg1("The group %s is not active at this site. Posting this article", cp);
	    msg0("will create a new newsgroup, here and possibly at other sites.");
	    if (!askyes("Are you sure you want to do this", "n"))
		xxit(0);

	    if (n++ != 0)
		(void) strcat(header.h_newsgroups, ngsep);
	    (void) strcat(header.h_newsgroups, cp);
	}
	else
	    msg1("%s is not a valid newsgroup.", cp);
    }
}

private void get_distribution()
/* find out how widely the author wants the message distributed */
{
    char *ep;

    while ((ep = vgetcmd("Distribution (? for help):")) != (char *)NULL)
    {
	/* did the user ask for help? */
	if (*ep == '?')
	{
	    msg0("How widely should your article be distributed?");
	    (void) dbadump(&distr, stdout);
	    msg0("\nEnter the name of the distribution that you require.");
	}
	else if (distr.file[0] == '\0')
	{
	    msg0("I can't check your distribution.");
	    break;
        }
	else if (afind(&distr, ep) == (char *)NULL)
	    msg1("Sorry, '%s' is an unknown distribution.", ep);
	else	    /* Found a match. Do any special rewriting. */
	{
	    register int n;
	    char *r;
	    
	    if (strcmp(ep, "net") == 0)
		ep = "world";
	    /* 
	     * A distribution of foo is useless
	     * if all the newsgroups are in foo.all
	     */
	    n = strlen(ep);
	    r = header.h_newsgroups;
	    do {
		if (strncmp(r, ep, n) == 0)
		{
		    ep = (char *)NULL;
		    break;
		}
	    } while
		(r = strchr(r, NGDELIM));
	    break;
        }
    }

    if (ep == (char *)NULL)
	(void) printf("Your article will get default distribution.\n");
    else
	hlcpy(header.h_distribution, ep);
}

private bool post_checks(mode, artfile)
/* do sanity checks after the author has typed in the message */
int	mode;
char	*artfile;
{
    FILE	*fp;
    bool	rotate = FALSE, iswanted();
    char	*p;
    int		included;

    if (!exists(artfile))
    {
	msg0("File deleted - no message posted.");
	(void) unlink(artfile);
	exit(1);
    }

    /* is this the right number?  Most of the headers are yet to be added */
    if (filesize(artfile) > MAXSIZE)
    {
	msg0("\nYour message will probably be truncated when it");
	msg0("passes through a notesfile site, since it is");
	msg0("greater than 65000 characters.");
	if (!askyes("Do you still want to post it", "n"))
	{
	    (void) sprintf(ccname, "%s/dead.article", userhome);
	    savemsg(artfile, ccname);
	    (void) unlink(artfile);
	    exit(1);
	}
    }

    /* the article is really there, read in the (possibly modified) header */
    fp = xfopen(artfile, "r");
    hfree(&header);
    (void) hread(&header, 0L, fp);

    if (hlblank(header.h_newsgroups))
    {
	msg0("Not sending to any newsgroup - no message posted");
 	(void) sprintf(ccname, "%s/dead.article", userhome);
 	savemsg(artfile, ccname);
	(void) unlink(artfile);
	exit(1);
    }

    if ((included = checkincl(&header, artfile)) != 0)
    {
	(void) fprintf(stderr,
		       "Too much quoted text (%d out of %d) in article\n",
		       included, header.h_intnumlines);
	exit(1);
    }

    /*
     * Generate some other headers from the environment
     * (and toss a few that the reader isn't permitted to specify)
     */
    if (originate(&header, FALSE) == FAIL)
    {
	msg0("There was no subject on that article!");
	exit(1);
    }

    /* ask for a Followup-To: line on all crossposts */
    if (strchr(header.h_newsgroups, ',') && hlblank(header.h_followto))
    {
	if (!xpert)
	{
	    if (mode == FOLLOW)
		msg1("The article you're following up was cross-posted to %s.",
		     header.h_newsgroups);
	    else
		msg1("You cross-posted to %s.", header.h_newsgroups);
	    if (hlblank(header.h_followto))
		msg0("Please list groups for followups to *your* article.");
	    else
	    {
		msg0("Please give a comma-separated list of followup groups,");
		msg1("or press enter to followup to %s.", header.h_followto);

	    }
	}

	if ((p = vgetcmd("Followup-To: ")) != (char *)NULL && *p != '\0')
	    hlcpy(header.h_followto, p);
    }

    /* give user chance to change other things if different Follow-To given */
    if (!xpert 
		&& hlnblank(header.h_followto)
		&& strcmp(header.h_followto, header.h_newsgroups)
	        && askyes("Edit your article to mention followup groups?","y"))
	{
	    if (artcopy(&header, fp, artfile, rotate) != FAIL)
		return(FALSE);
	    else
		msg0("Sorry, I can't seem to re-read the article.");
	}

    /* allow modification of Subject line, prompt for a Summary line */
    if (mode == FOLLOW)
    {
	int	i;

	/* give user a chance to change the Subject line */
	msg1("The present Subject line is \"%s\".", header.h_subject);
	if (!xpert)
	    msg0("Please give a new one (or press enter to leave it alone).");
	if ((p = vgetcmd("Subject: ")) != (char *)NULL && *p != '\0')
	    hlcpy(header.h_subject, p);

	/* get a Summary line */
	msg0("Please enter a summary of your contribution to the discussion");
	msg0("Just one or two lines ...   (end with a blank line)");
	bfr[0] = '\0';
	for (i = 0; i < 3; i++)	/* 3 * 80 < 256, should be safe .. */
	{
	    if ((p = vgetcmd("> ")) != (char *)NULL && *p == '\0')
		break;
	    if (i > 0)
		(void) strcat(bfr, "\n\t");
	    (void) strcat(bfr, p);
	}
	hlcpy(header.h_summary, bfr);
    }

    /* now check to see if we should ROT13 this */
    if (ngmatch(header.h_newsgroups, "rec.humor,!rec.humor.any"))
    {
	if (askyes("Could this be offensive to anyone", "n"))
	{
	    char    *targets;

	    targets = vgetcmd("Whom might it offend? ");
	    (void) sprintf(bfr," - offensive to %s (rot 13)", targets);
	    (void) strcat(header.h_subject, bfr);
	    rotate = TRUE;
	}
    }

    if (ngmatch(header.h_newsgroups, "comp.sources.all,!comp.sources.wanted"))
    {
	if (hlblank(header.h_subject))
	{
	    msg0("There seems to be no subject for this article.");
	    (void) strcpy(header.h_subject, vgetcmd("Subject: "));
	}
	if (ngmatch(header.h_newsgroups, "all.wanted")
					    || iswanted(header.h_subject))
	{
	    msg0("Requests for sources should not be posted to any of the");
	    msg0("sources newsgroups, please post such requests to");
	    msg0("a 'wanted' group only. Please reenter the newsgroups.");
	    get_newsgroups();
	}

	if (ngmatch(header.h_newsgroups, "all.sources,!all.sources.{d,bugs}")
	    && filesize(artfile) < (4*1024))
	{
	    msg0("Your article seems rather small for a program source.");
	    if (!askyes("Are you certain it is really source", "n"))
		get_newsgroups();
	    if (strchr(header.h_newsgroups, NGDELIM))
	    {
		msg0("Sources should be posted to one newsgroup only.");
		msg0("Please pick one appropriate group for yours.");
		get_newsgroups();
	    }
	}
    }

    /*
     * Merge the header (which was almost certainly modified, at least if
     * we originated) with the rest of the article.
     */
    if (artcopy(&header, fp, artfile, rotate) == FAIL)
    {
	savemsg(artfile, ccname);
	exit(1);
    }

    (void) fclose(fp);
    return(TRUE);
}

private bool iswanted(str)
/* try to detect help-wanted messages */
register char *str;
{
    while (*str == ' ')
	str++;

    if (prefix(str, "Re:"))
	return(FALSE);

    if (isin(str, " wanted ") || isin(str, " can any") || isin(str, " sell ") 
	|| isin(str, " need ") || isin(str, " please ") || isin(str, " help ")
	|| isin(str, " looking ") || isin(str, " recommend ") 
	|| isin(str, " sale ") || strchr(str, '?'))
	    return(TRUE);

    return(FALSE);
}

private int isin(str, words)
register char *str, *words;
{
    register char *p;
    register int sc, wc;

    p = words;
    while (sc = *str++) {
	if ((wc = *p++) == '\0')
	    return (TRUE);
	if (wc == ' ') {
	    if (strchr(".,!?-; \t\n", sc))
		continue;
	} else {
	    if (isupper(wc))
		wc = tolower(wc);
	    if (isupper(sc))
		sc = tolower(sc);
	    if (wc == sc)
		continue;
	}
	str -= p - words - 1;
	p = words;
    }
    if (*p == '\0')
	return(TRUE);
    return(FALSE);
}

private int askyes(ymsg, def)
/* get a yes or no answer to a question (a default may be specified) */
char *ymsg, *def;
{
    for(;;)
    {
	(void) sprintf(bfr, "%s [%s]? ", ymsg, def);
	switch(*vgetcmd(bfr))
	{
	case 'y':
	case 'Y':
	    return(TRUE);
	case 'n':
	case 'N':
	    return(FALSE);
	case '\0':
	    switch(*def)
	    {
	    case 'y':
	    case 'Y':
		return(TRUE);
	    case 'n':
	    case 'N':
		return(FALSE);
	    }
	default:
	    msg0("Please answer yes or no.");
	}
    }
}

private bool valid_ng(ng)
/* verify that the newsgroup exists, and is accessible; if so, select it */
char	*ng;
{
    group_t	*ngp;

    if ((ngp = ngfind(ng)) == (group_t *)NULL && strncmp("to.", ng, 3))
	return(FALSE);
    else
	ngselect(ngp);

#ifdef FASCIST
    /* check that the originating user can post to the group */
    if (!privileged && !ngmatch(ng, fascist(username)->n_post))
	xerror1("You aren't authorized to post to %s\n", ng);
#endif /* FASCIST */
    return(TRUE);
}

private bool isflex(ng)
/* is a given unknown newsgroup in a flexgroups hierarchy? */
char	*ng;
{
    static feed_t	*sys = (feed_t *)NULL;

    if (sys == (feed_t *)NULL)
    {
	s_read();
	if ((sys = s_find(site.pathname)) == (feed_t *)NULL)
	    xerror0("couldn't find this site's entry in the feeds file!");
    }
    return(ngmatch(ng, sys->s_gflex));
}

private void dofollowup()
/* generate full headers for a followup article */
{
    cite = mkfollowup(&header);

    /* check that the newsgroup is OK */
    if (ngmatch(header.h_newsgroups, "misc.jobs.{resumes,offered}"))
    {
    msg0("misc.jobs.{resumes,offered} are for posting of job announcements");
    msg0("and requests, not discussion. Your followup has been directed to");
    msg0("misc.jobs.misc.");
    (void) hlcpy(header.h_newsgroups, "misc.jobs.misc");
    }
}

main(argc, argv)
char *argv[];
{
    register int c;
    int mode;
    register char *cprompt, *p, *opts;
    FILE    *fp;
    static int	cmdcount = 0;
    static bool fbypost = FALSE;
    forward catch_t xxit();

    newsinit();

    (void) signal(SIGHUP, xxit);
    (void) signal(SIGINT, xxit);
    (void) signal(SIGQUIT, xxit);
    (void) signal(SIGTERM, xxit);

#ifndef NONLOCAL
    (void) sprintf(bfr, "%s/%s", site.admdir, "distributions");
    distr.file = savestr(bfr);
#else
    distr.file = getdistribs();
#endif /* NONLOCAL */
    (void) dbaread(&distr);

    if ((p = getenv("NEWSARCHIVE")) != (char *)NULL)
    {
	if (*p == '\0')
	    (void) sprintf(ccname, "%s/author_copy", userhome);
	else
	    (void) strcpy(ccname, p);
    }

    (void) rdactive(NULLPRED);

#ifdef ALLOCHDRS
    /* headers that procopts() might write into have to be real storage */
    header.h_subject = malloc(BUFLEN);
    header.h_newsgroups = malloc(BUFLEN);
    header.h_keywords = malloc(BUFLEN);
    header.h_summary = malloc(BUFLEN);
#endif /* ALLOCHDRS */

    /* process options specified in the environment */
    if ((opts = getenv("POSTOPTS")) != (char *)NULL)
    {
	int	largc = 1, selected = 0;
	char    *largv[MAXARGS];
	char    text[LBUFLEN];

	/* we need our own copy of the option text to alter */
	if (opts == (char *)NULL || *opts == '\0')
	    return(SUCCEED);
	else
	    (void) strcpy(text, opts);

	/* put pointers to null-separated tokens in the dope vector largv[] */
	if ((largc = vcrack(text, largv + 1, MAXARGS)) == FAIL)
	    xerror0("Too many arguments");
	else
	    largc++;

	/* check that the options are applicable */
	if (largv[1][0] && largv[1][0] != '-')
	    if (strcmp(largv[1], Progname))
		return(SUCCEED);
	    else
		selected = 1;

	/* hand the argument vector we just created off to be processed */
	(void) procopts(largc - selected, largv + selected, DNC, postopts);
    }

    /* now process stuff from command line */
    mode = procopts(argc, argv, DNC, postopts);

    if (mode == INCL)
    {
	(void) strcpy(bfr, INCLTEMP);
	(void) mktemp(bfr);
	(void) fcopy(original, bfr);
	(void) strcpy(original, bfr);
	article = original;
    }
    else if (mode == FOLLOW)
    {
	if (original[0] == '<')	/* a Message-ID */
	{
	    int	st;

	    if (hstseek(original, FALSE) != VALID)
		return(FAIL);
	    else
		while ((st = hstloc(&art)) != FAIL)
		    if (st == SUCCEED && artname(&art, bfr) == SUCCEED)
			return(SUCCEED);
	    return(FAIL);
	}
	else			/* a group/number pair */
	    (void) refname(original, &art, bfr);
	(void) strcpy(original, bfr);
	fp = xfopen(original, "r");
	hfree(&header);
	(void) hread(&header, 0L, fp);
	fbypost = hlnblank(header.h_followto)
	    && (strcmp(header.h_followto, "poster") == 0);
	(void) fclose(fp);

	dofollowup();
    }
    else if (mode != POST
		&& askyes("Is this in response to some other article", "n"))
    {
	char *ng, *num;

	if (hlnblank(header.h_newsgroups))
	    msg1("Following up to %s", ng = header.h_newsgroups);
	else
	    ng = vgetcmd("In what newsgroup was the article posted? ");

	if (!valid_ng(ng))
#ifndef FLEXGROUPS
	    xerror0("There is no such newsgroup.");
#else
	    xerror0("No such group is currently active at this site");
#endif /* FLEXGROUPS */

	active.article.m_number = ngmax();
	for (;;)
	{
	    int	i;

	    (void) sprintf(original,
		"What was the article number [default %ld of %ld-%ld]? ",
		(long)msgnum(), (long)ngmin(), (long)ngmax());

	    if ((num = vgetcmd(original)) == (char*)NULL)
		break;

	    switch(num[0])
	    {
	    case '+': active.article.m_number++; break;
	    case '-': active.article.m_number--; break;
	    case '\0': break;
#ifdef SUBJFILE
	    case '?':
		for (i = ngmin(); i <= ngmax(); i++)
		    if (indexline(ngactive(), i, bfr) != FAIL)
			(void) fprintf(stdout, "%s\n", bfr);
		break;
#endif /* SUBJFILE */
	    default: active.article.m_number = atoi(num); break;
	    }
	    if (artname(&art, original) == FAIL)
		msg2("I can't find a copy of %s/%ld.",
		     ngname(), (long)msgnum());
	    else if ((fp = fopen(original, "r")) == (FILE *)NULL)
		msg2("The text of %s/%ld is missing.",
		     ngname(), (long)msgnum());
	    else
	    {
		hfree(&header);
		(void) hread(&header, 0L, fp);
		(void) fclose(fp);

		msg2("\nArticle %s/%ld", ngname(), (long)msgnum());
		if (hlnblank(header.h_from))
		    msg1("From: %s", header.h_from);
		if (hlnblank(header.h_subject))
		    msg1("Subject: %s", header.h_subject);
		if (askyes("Is this the one you want", "y"))
		    break;
	    }
	}

	mode = FOLLOW;
	dofollowup();
	if (!cflag &&
		askyes("Do you want to include a copy of the article", "n"))
	    cflag = TRUE;
	if (cflag)
	    msg0("Please edit out unnecessary verbiage, signatures, etc.");
	else
	    original[0] = '\0';
    }
    else	/* not a followup */
    {
	while (hlblank(header.h_subject))
	{
	    char *subj = vgetcmd("Subject: ");

	    (void) strcpy(header.h_subject, subj);
	    if (subj[0] == '?')
	    {
		msg0("People read the subject line to learn what your article is about.");
		msg0("You want it to do the same job as a newspaper headline.");
		msg0("So type in something both brief and descriptive.");
		hlfree(header.h_subject);
	    }
	}
	if (hlblank(header.h_keywords))
	    (void) strcpy(header.h_keywords, vgetcmd("Keywords: "));

	get_newsgroups();
	if (!xpert
	    	&& hlblank(header.h_distribution)
	        && !ngmatch(header.h_newsgroups, "to.all"))
	    get_distribution();
    }

    if (p = recording(header.h_newsgroups))
    {
	int	n;

	(void) more(p, (FILE *)NULL, 0);
    	msg0("Do you understand this?  Hit <return> to proceed, <BREAK> to abort:");
	n = read(0, bfr, (iolen_t)BUFLEN);
	if (n <= 0 || (bfr[0] && !strchr("yY\n", bfr[0])))
	    exit(1);
    }

    if (mode != INCL)
	article = prepmsg(&header, original,
		      (mode == FOLLOW) ? cite->citeline : (char *)NULL,
		      inclmark);

#ifdef NONLOCAL
    /* toss the tempfile(s) that the network server handed back */
    if (original[0])
	(void) unlink(original);
    if (distr.file[0])
	(void) unlink(distr.file);
    /* we could close the connection to the network server here */
#endif /* NONLOCAL */

    if (mode == FOLLOW)
    {
	if (strcmp(header.h_followto, "poster") == 0)
	    cprompt = "\nWhat now? [mail, edit, list, quit, write, help] ";
	else
	    cprompt = "\nWhat now? [mail, post, edit, list, quit, write, help] ";
    }
    else
	cprompt = "\nWhat now? [post, edit, list, quit, write, help] ";

    /* get a posting command */
    for (c = 'e'; ; c = *vgetcmd(cprompt))
    {
	if (isupper(c))
	    c = tolower(c);

	switch (c)
	{
	case 'q':
	    goto done;

	case 'e':
	    do {
		if (cmdcount == 0)
		{
		    if (xpert != sflag)
			signmsg(article);
		}
		else if (!xpert)
		{
		    (void) printf(
			"Note: you should delete the article's From header.\n");
		    (void) sleep(2);
		}
		if (editmsg(article) == FAIL)
		    xerror0("You don't seem to have an editor!");
		if (cmdcount++ == 0)
		{
		    if (modtime(article) == header.h_posttime
			|| filesize(article) < MINSIZE)
		    {
			msg0("File not modified - no message posted.");
			(void) unlink(article);
			exit(1);
		    }
		}
	    } while
		 /* reads modified header into 'header' */
		(!post_checks(mode, article));
	    break;

	case 'l':
	    (void) more(article, (FILE *)NULL, header.h_intnumlines);
	    break;

	case 'm':	/* reply to article by email */
	    if (mode != FOLLOW)
		msg0("There's no originator to mail to. Try 'p'");
	    else
	    {
		FILE *ifp = fopen(article, "r");
		FILE *ofp = mailopen(cite->address, header.h_subject);
		register int ch;

		if (ofp	== (FILE*)NULL)
		{
		    (void) fclose(ifp);
		    msg0("Couldn't open the mailer.");
		    break;
		}
		while ((ch = fgetc(ifp)) != EOF)
		    (void) fputc(ch, ofp);
		(void) fclose(ifp);
		(void) mailclose(ofp);

		msg0("Mail is being sent");
		goto done;
	    }
	    break;

	case 'p':    /* post the article */
	    if (mode == FOLLOW && fbypost)
		msg0("Poster specified reply by mail only. Try 'm'");
	    else
	    {
		if ((newid = newpost(article, (char *)NULL)) == (char *)NULL)
		{
#ifdef DEBUG
		    if (!debug)
#endif /* DEBUG */
			msg0("Article not posted -- exec of rnews failed");
		}
		else
		    msg1("Article %s posted successfully.", newid);

		/* save a copy for the reader's archives */
#ifdef DEBUG
		if (!debug)
#endif /* DEBUG */
		    if (ccname[0])
		    {
			/* mung in the article ID passed back, if any */
			if (newid && newid[0])
			{
			    fp = fopen(article, "r");
			    while (fgets(bfr, LBUFLEN, fp) && isalpha(bfr[0]))
				continue;
			    hlfree(header.h_ident);
			    hlcpy(header.h_ident, newid);
			    hlfree(header.h_from);
			    hlfree(header.h_path);
			    hlfree(header.h_organization);

			    (void) artcopy(&header, fp, article, FALSE);
			    (void) fclose(fp);
			}

			savemsg(article, ccname);
			msg1("A copy has been saved in %s", ccname);
		    }
		goto done;
	    }
	    break;

	case 'w':	/* write the article to a named file */
	    (void) strcpy(bfr, vgetcmd("Filename? "));
	    if (saveto(bfr, 0) == FAIL)
		msg1("Save to %s failed", bfr);
	    break;

	default:
	    if (mode == FOLLOW)
		msg0("m -- mail reply to the poster of the original message");
	    msg0("e -- edit the reply, using your default editor");
	    msg0("p -- post the reply to USENET");
	    msg0("l -- list the reply through your pager");
	    msg0("q -- quit, throwing away the reply text");
	    msg0("w -- write the reply to a file (you'll be prompted for a name)");
	    msg0("h,? -- this message.");
	    break;
	}
    }

done:
    (void) unlink(article);

    /* report the article's generated ID, if requested */
    if (outfd)
    {
	(void) write(outfd, newid, (iolen_t)(strlen(newid) + 1));
	(void) close(outfd);	/* not strictly necessary */
    }
    return(SUCCEED);
}

catch_t xxit(i)
{
    hstclose();	/* disconnect from network server, if any */
    exit(i);
}

#ifdef SIGCONT
void vupdate() {/* no-op */}
#endif /* SIGCONT */

/* postnews.c ends here */
