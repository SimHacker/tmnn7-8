/******************************************************************************

NAME
   bbsauto -- automaton for communication with BBS forum software

SYNOPSIS
   bbsauto [-ug] [-n service] [-p newsfile]

DESCRIPTION
   This program tries to create the illusion that a group of BBS fora are
groups in a pseudo-hierarchy on USENET. See the bbsauto(1) man page for
details.
   Send-expect transactions are logged on stdout, with all CR (ASCII 13)
characters suppressed.

BUGS
   If the UNIX-to-BBS posting code punts in mid-batch, all the failure mail
will get generated while the robot is still on line to the BBS. This could
cost money, but the alternative is to risk losing the error notification
due to a handshake failure in hangup().
   Various buffers will overrun if lines are longer than BUFSIZ chars.

FILES
   LIBDIR/bbsauto	-- control information for bbsauto
   /tmp/takenews??????	-- capture file for incoming news (BBS format)
   /tmp/takemail??????	-- capture file for incoming mail (BBS format)
   /tmp/bbsarts??????	-- incoming news with netnews headers
   /tmp/bbsnews??????	-- temp file for incoming news
   /tmp/bbsmail??????	-- temp file for incoming mail
   /tmp/rebatch??????	-- temp file for news rebatching

AUTHOR
   Eric S. Raymond
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

******************************************************************************/
/*LINTLIBRARY*/
#include "news.h"
#include "spawn.h"
#include "libpriv.h"
#include "header.h"
#include "procopts.h"
#include "server.h"
#include "bbsauto.h"

/* debug levels */
#define D_SHOWAWAIT	1	/* show expect strings */
#define D_SHOWPHASE	2	/* show current phase of dialogue */
#define D_KEEPFILES	3	/* don't delete tempfiles at end of run */
#define D_SHOWSEND	4	/* show send strings */
#define D_SHOWCU	5	/* run cu with debug option */

#ifdef USG
#define RMAIL	"LOGNAME=%s rmail %s"
#else
#define RMAIL	"USER=%s rmail %s"
#endif /* USG */

/*
 * Here's where we enable the code to talk to various BBS programs.
 */
extern bbstype compuserve, dbbs;
bbstype *bbstypes[] =
{
    &compuserve,	/* CompuServe Information Service */
    &dbbs,		/* Random DBBS boards */
};
#define MAXTYPES	(sizeof(bbstypes) / sizeof(bbstype *))

bbs remote;		/* for now, just support one bbs per call */
bool doecho = TRUE;	/* tells whether we expect echo of send strings */

/*
 * This module exists to provide a wrapper for (and dispatch to) BBS
 * transaction interpreters implemented in other modules. See bbsauto.h for
 * the interface to the services these interpreters can count on.
 */

#define WSPC	"\t \n"		/* whitespace for command line parsing */
#define MAXALTS	5		/* max alternatives for accept */
#define RETRIES	3		/* # retries on error */
#define DEFRESP	30		/* default response timeout */
#define CAPTIME (10*60)		/* default capture timeout */
#define DEFSERV	"compuserve"	/* default service */
#define FIRST	"From: "	/* first header printed by hwrite() */

char	*Progname = "bbsauto";
char	*usage = "Usage: bbsauto [-ug] [-n service] [-p newsfile] [-m mailfile]";

/* option-processing and miscellaneous global variables */
private char service[IDSIZE] = DEFSERV;	/* name of service */
private char infile[BUFSIZ];		/* news input file */
private char dkill[IDSIZE];		/* dialer kill command */
private jmp_buf	abortbuf;		/* to jump to if expects fail */
private int getnews;			/* should we fetch news? */
private int nospool;			/* force rnews -u */
#ifdef DEBUG
private int snarf;			/* snarf stuff off BBS, no xlate */
private int filter;			/* filter as incoming news or mail */
private int fmail;			/* set this to filter mail */
#endif /* DEBUG */
bool deletein = FALSE;			/* delete input file? */

option_t options[] =
{
/*
optlet  filchar	flag        from      to       type     buf	*/
'p',	'\0',	(int *)0,   DNC,      DNC,     STRING,	infile,
'g',	'\0',	&getnews,   DNC,      DNC,     OPTION,	(char *)NULL,
'n',	'\0',	(int *)0,   DNC,      DNC,     STRING,	service,
'u',	'\0',	&nospool,   DNC,      DNC,     OPTION,	(char *)NULL,
#ifdef DEBUG
'D',	'\0',	&debug,     DNC,      DNC,     NUMBER,	(char *)NULL,
'f',	'\0',	&filter,    DNC,      DNC,     OPTION,	(char *)NULL,
'm',	'\0',	&fmail,     DNC,      DNC,     OPTION,	(char *)NULL,
's',	'\0',	&snarf,     DNC,      DNC,     OPTION,	(char *)NULL,
#endif /* DEBUG */
'\0',	'\0',	0,	    0,        0,       0,       (char *)NULL,
};

/*
 * Primitives for BBS transaction programming begin here.
 * They are as follows:
 *	send()		-- send a string to the bbs
 *	expect()	-- expect a given prompt
 *	saw()		-- check if string was in last line received
 *	capture()	-- catch everything up to a given prompt
 *	msplit()	-- split current message into chunks
 *	upload()	-- upload a file section to the BBS
 *	errmail()	-- send error mail back to message originator
 *	punt()		-- abort bbs session, error-return all articles
 */

/*
 * if we ever support more than 1 BBS per call, these
 * have to move into the BBS structure
*/
private char *fora[MAXTOPICS];	/* list of fora to read */
private int nfora;		/* number of fora in list */
private FILE *logfp = stdout;	/* if non-NULL, log transactions to here */
private server_t *comm;		/* server pair for I/O to BBS */
private bool overtime;		/* set TRUE by expect() timeout */

#ifdef SIGCLD
static void oncld(signo)
int	signo;
{
    log3("%s died (signal %d), dialogue with %s aborted",
	 remote.dial, signo, remote.servname);
    xxit(1);
}
#endif /* SIGCLD */

static catch_t onalarm()
/* catcher for alarm signals */
{
#if !defined(BSD4_2) && !defined(SYSV3)
    (void) signal(SIGALRM, onalarm);	/* for portability */
#endif
    overtime = TRUE;
}

static bool waitcheck(fd, wtime)
/* wait wtime secs, tell us if any I/O piled up during that time */
int fd;
int wtime;
{
#ifdef USG
    /*
     * This code assumes that alarm signals can interrupt a read(2) call.
     * Note that when waitcheck() returns FALSE, a character has been
     * discarded.
     */
    char cc;
    int whilewaiting, oldalrm = alarm(wtime);

    if ((whilewaiting = read(fd, cc, 1)) == 1)
    {
	(void) alarm(oldalrm);
	return(FALSE);
    }
    else if (whilewaiting == FAIL && errno == EINTR)
    {
	(void) alarm(oldalrm);
	return(TRUE);
    }
    else
	return(FAIL);	/* should never happen */
#endif /* USG */
#ifdef BSD
   /*
     * Being able to check for input makes like easier
     */
    int count = 0;	/* if FIONREAD fails, assume no input */

    (void) sleep(wtime);
    (void) ioctl(fd, (int)FIONREAD, (char *)&count);

    return(count == 0);
#endif /* BSD */
}

int await(nalts, alts, timeout, retries)
/* expect a response string from the remote */
int	nalts;		/* count of alternatives */
char	*alts[];	/* wait for this string */
int	timeout;	/* timeout on wait */
int	retries;	/* maximum error retries */
{
    int mcount[MAXALTS];	/* completion counts -- only use of MAXALT */
    char *cp, **ap;
    bool newline = TRUE;
    int i;

    for (i = 0; i < MAXALTS; i++)
	mcount[i] = 0;
    overtime = FALSE;
    (void) signal(SIGALRM, onalarm);
    (void) alarm(timeout);

    /* have up to RETRIES tries at the expect string, with resynchronization */
    while (retries-- > 0)
    {
	char	cc;
	bool	waiting;

	/* read characters until we either get the expect string or time out */
	do {
	    /* sometime, use fdopen and make this buffered */
	    int	is = srvread(comm, &cc, 1);

	    /* read no good? */
	    if (is == FAIL)
		if (errno == EINTR)
		    break;		/* we timed out */
	        else
		{
		    logerr1("bad read from remote, errno = %d", errno);
		    longjmp(abortbuf, 1);
		}

	    if (newline)
	    {
		cp = remote.lastline;
		newline = FALSE;
	    }

	    /* arrange for last line to be available and logged */
	    if (cc != '\r')
	    {
		*cp++ = cc;
		*cp = '\0';
	    }
	    if (cc == '\n')	/* this works on MS-DOS or XENIX */
		newline = TRUE;

	    /* log all non-carriage-return characters */
	    if (logfp != (FILE *)NULL)
		if (cc != '\r')
		{
		    if (iscntrl(cc) && cc != '\n' && cc != ' ')
			(void) fprintf(logfp, "^%c", cc + '@');
		    else
			(void) fputc(cc, logfp);
		}

	    /* have we found anything on the expect token list? */
	    waiting = FALSE;
	    for (ap = alts; ap < alts + nalts; ap++)
	 	if (*ap != (char *)NULL)
		{
		    char *pp = *ap + mcount[ap - alts];

		    /* check match with next char or escape in string */
		    if (pp[0] == '\\' && pp[1] == 'T')
		    {
			/*
			 * Note: the USG implementation of waitcheck() tosses
			 * a character when it resturns FALSE. Thus, if you
			 * have \T in an expect string and it fails, following
			 * expect strings that don't also have a \T at that
			 * spot will see not the current character
			 * but the *next* one. We could solve this by
			 * implementing a queue, but the situation seems very
			 * unlikely to arise in a real script.
			 */
			if (waiting || (waiting = waitcheck(comm->readsrv, 1)))
			    mcount[ap - alts] += 2;
		    }
		    else if (cc == pp[0])
			mcount[ap - alts]++;
		    else
			mcount[ap - alts] = 0;

		    /* 
		     * if, after checking, we have a complete match, we're
		     * done -- catches *shortest* match
		     */
		    if (mcount[ap - alts] == strlen(*ap))
			return(ap - alts);
		}
	} while
	    (!overtime);

	/* if we're going to retry, try to resynchronize */
	if (retries)
	    (*(remote.type->resynch))();
    }

    return(FAIL);
}

static char *visible(buf)
/* visibilize a given string */
char	*buf;
{
    static char visbuf[BUFSIZ];
    char *tp = visbuf;

    while (*buf)
    {
	if (isprint(*buf) || *buf == ' ')
	    *tp++ = *buf++;
	else if (*buf == '\n')
	{
	    *tp++ = '\\'; *tp++ = 'n';
	    buf++;
	}
	else if (*buf == '\r')
	{
	    *tp++ = '\\'; *tp++ = 'r';
	    buf++;
	}
	else if (*buf == '\b')
	{
	    *tp++ = '\\'; *tp++ = 'b';
	    buf++;
	}
#ifdef ASCII
	else if (*buf < ' ')
	{
	    *tp++ = '\\'; *tp++ = '^'; *tp++ = '@' + *buf;
	    buf++;
	}
#endif /* ASCII */
	else
	{
	    (void) sprintf(tp, "\\0x%02x", *buf++);
	    tp += strlen(tp);
	}
    }
    *tp++ = '\0';
    return(visbuf);
}

static void swrite(buf, len)
/* write with echo check */
char	*buf;	/* buffer of chars to send */
int	len;	/* count of characters to send */
{
    char waitfor[BUFLEN], *largv[2];

    if (srvwrite(comm, buf, len) == FAIL)
	logerr1("write to remote failed, errno = %d", errno);

    (void) memcpy(waitfor, buf, len);
    waitfor[len] = '\0';
    largv[0] = waitfor;
    largv[1] = (char *)NULL;
    if (doecho && await(1, largv, DEFRESP, 1) == FAIL)
    {
	logerr1("timeout while awaiting echo of %s", visible(buf));
	longjmp(abortbuf, 1);
    }
}

/*VARARGS1*/
void send(sendstr, a1, a2, a3, a4, a5)
/* send a command to remote */
char	*sendstr, *a1, *a2, *a3, *a4, *a5;
{
    (void) sprintf(bfr, sendstr, a1, a2, a3, a4);
#ifdef DEBUG
    if (debug >= D_SHOWSEND)
    {
	(void) fprintf(stderr, "<<send: %s>>", visible(bfr));
	(void) fflush(stderr);
    }
#endif /* DEBUG */
    swrite(bfr, strlen(bfr));
}

int strindex(sx, tx)
/* strindex returns location of tx in sx */
char *sx, *tx;
{
    int i, n;

    n = strlen(tx);
    for (i = 0; sx[i] != '\0'; i++)
	if (strncmp(sx + i, tx, n) == 0)
	    return(i);
    return(FAIL);
}

int saw(substring)
/* check to see if a given substring was in the last prompt */
char *substring;
{
    /*
     * this does the right thing for prompts that we designate
     * as starting at the beginnings of lines
     */
    while (strchr("\r\n", *substring) != (char *)NULL)
	substring++;
    return(strindex(remote.lastline, substring));
}

#ifdef DEBUG
void waitshow(wtype, nalts, alts)
/* display an expect vector */
char	*wtype;		/* wait type */
int	nalts;		/* count of alternatives */
char	*alts[];	/* wait for this string */
{
    if (nalts == 1)
	(void) fprintf(stderr, "<<%s: %s>>", wtype, visible(alts[0]));
    else if (nalts == 2)
	(void) fprintf(stderr, "<<%s: %s or %s>>",wtype,alts[0],alts[1]);
    else if (nalts == 3)
	(void) fprintf(stderr,
		       "<<%s: %s or %s or %s>>",
		       wtype, alts[0], alts[1], alts[2]);
    else
	(void) fprintf(stderr,
		       "<<%s: %s or %s or %s or others>>",
		       wtype, alts[0], alts[1], alts[2]);
}
#endif /* DEBUG */

void expect(prompt)
/* skip characters until we get to a given prompt or time out */
char *prompt;
{
    char *largv[2];

    largv[0] = prompt;
    largv[1] = (char *)NULL;

#ifdef DEBUG
    if (debug >= D_SHOWAWAIT)
	waitshow("expect", 1, largv);
#endif /* DEBUG */
    if (await(1, largv, DEFRESP, RETRIES) == FAIL)
    {
	logerr2("timeout after %d retries while awaiting %s", RETRIES,
		visible(prompt));
	longjmp(abortbuf, 1);
    }
}

int capture(until, textfp)
/* skip characters until we get to a given prompt or time out */
char *until;
FILE *textfp;
{
    char *largv[2];
    int ret;
    FILE *oldlogfp = logfp;

    largv[0] = until;
    largv[1] = (char *)NULL;
    logfp = textfp;
#ifdef DEBUG
    if (debug >= D_SHOWAWAIT)
	waitshow("capture", 1, largv);
#endif /* DEBUG */
    ret = await(1, largv, CAPTIME, 1);
    logfp = oldlogfp;
    if (ret == FAIL)
    {
	logerr0("capture timed out");
	longjmp(abortbuf, 1);
    }
    else
	return(ret);
}

int msplit(chunksize, maxsplit, tsplit)
/* split message text in current header into appropriate-sized pieces */
off_t	chunksize;
int	maxsplit;
off_t	tsplit[];
{
    register off_t	here, *tp = &tsplit[0];

    (void) fseek(header.h_fp, tsplit[0] = header.h_textoff, SEEK_SET);
    for (here = header.h_textoff;
	   here < header.h_endoff && tp < tsplit + maxsplit;
	   here++)
    {
	if (fgetc(header.h_fp) == '\n')
	    tp[1] = here + 1;

	if (here - tp[0] >= chunksize)
	    tp++;
    }
    tp[1] = header.h_endoff;
    return((here < header.h_endoff) ? FAIL : tp - tsplit);
}

void upload(fp, start, end, mode)
/* upload a file to the target system in a given mode */
FILE	*fp;		/* file to upload from */
off_t	start, end;	/* start and end offsets of upload section */
int	mode;		/* upload mode */
{
    off_t	ccnt = end - start;
    int		c;
    char	xmit[LBUFLEN], *cp;

#ifdef DEBUG
    if (debug >= D_SHOWPHASE)
	log1("about to upload %d characters", ccnt);
#endif /* DEBUG */

    if (fseek(fp, start, SEEK_SET) == FAIL)
	log1("seek to upload section failed, errno %d", errno);
    switch (mode)
    {
    default:
    case M_NOXLATE:
    case M_LFTOCRLF:
	cp = xmit;
	while (ccnt-- > 0)
	{
	    *cp++ = c = fgetc(fp);
	    if (c == '\n')
	    {
		if (mode == M_LFTOCRLF)
		{
		    cp[-1] = '\r';
		    *cp++ = '\n';
		}
		swrite(xmit, cp - xmit);
		cp = xmit;
	    }
	}
	break;

    case M_XMODEM:
	/* not yet implemented */
	break;
    }

#ifdef DEBUG
    if (debug >= D_SHOWPHASE)
	log0("upload complete");
#endif /* DEBUG */
}

void errmail(emsg, e1, e2, e3)
/* mail contents of current message back to originator with error indication */
char	*emsg, *e1, *e2, *e3;
{
    FILE	*mfp;
    int		i;

    if (fseek(header.h_fp, header.h_startoff, SEEK_SET) == FAIL)
	xerror1("seek to failed message failed, errno %d", errno);

    mfp = mailopen(mailreply(&header), "Returned BBS mail or posting");

    (void) fprintf(mfp,"\nYour post didn't make it onto %s\n",remote.servname);
    (void) sprintf(bfr, e1, e2, e3);
    (void) fprintf(mfp, "The problem appears to be: %s\n", bfr);
    (void) fprintf(mfp, "\n   ----- Unsent message follows -----\n");

    for (i = header.h_startoff; i < header.h_endoff; i++)
	(void) fputc(fgetc(header.h_fp), header.h_fp);

    (void) mailclose(mfp);

    /*
     * We don't have to re-seek header.h_fp because it's now sitting at the end
     * of the failed message, which is where we want it.
     */
}

void punt()
/* give up */
{
    longjmp(abortbuf, 1);
}

static void hangup()
/* hang up the BBS connection */
{
    if (dkill[0])
	srvwrite(comm, dkill, strlen(dkill));
    else if (kill(comm->pid, SIGTERM) == FAIL)
	log2("kill of `%s' failed, errno = %d", remote.dial, errno);
    puts("Bye");
}

/*
 * wrapper code starts here
 */

static void ctrlmap(buf, str, var)
char *buf, *str, **var;
{
    char	vbuf[BUFSIZ], *ret;

    if (!strncmp(buf, str, strlen(str)))
    {
	(void) strcpy(vbuf, buf + strlen(str));
	(void) nstrip(vbuf);
#ifdef DEBUG
	if (debug >= D_SHOWPHASE && verbose)
	    log2("%s%s", str, vbuf);
#endif /* DEBUG */
	*var = savestr(vbuf);
    }
}

static int readparams(service, params)
/* read parameters for the given service into remote */
char	*service;
char	*params;
{
    char	*servtype;
    FILE	*fp;
    bool	foundit;
    int		i;

    if ((fp = fopen(params, "r")) == (FILE *)NULL)
	return(FAIL);

    remote.servname = service;

    /* first, find the service we're calling */
    foundit = FALSE;
    while (fgets(bfr, BUFSIZ, fp) != (char *)NULL)
    {
	(void) nstrip(bfr);
	if (!strncmp(bfr, "service: ", 9) && !strcmp(bfr + 9, service))
	{
	    foundit = TRUE;
	    break;
	}
    }
    if (!foundit)
	return(FAIL);

    /* load the account info for that service */
    servtype = (char *)NULL;
    remote.owner = site.notify;
    while (fgets(bfr, BUFSIZ, fp) != (char *)NULL && strcmp(bfr, "%%\n"))
    {
	ctrlmap(bfr, "type: ", &servtype);
	ctrlmap(bfr, "dial: ", &(remote.dial));
	ctrlmap(bfr, "hostname: ", &(remote.hostname));
	ctrlmap(bfr, "logon: ", &(remote.logon));
	ctrlmap(bfr, "password: ", &(remote.password));
	ctrlmap(bfr, "owner: ", &(remote.owner));
	ctrlmap(bfr, "notify: ", &(remote.notify));
	ctrlmap(bfr, "topics: ", &(remote.alltopics));
    }
    (void) fclose(fp);

    /* now set the service type */
    if (servtype == (char *)NULL)
	xerror1("no service type given for %s", service);
    for (i = 0; i < MAXTYPES; i++)
	if (!strcmp(servtype, bbstypes[i]->typename))
	{
	    remote.type = bbstypes[i];
	    break;
	}
    if (remote.type == (bbstype *)NULL)
	xerror2("couldn't find service type %s for %s", servtype, service);

    /* O.K., we have a valid BBS/type combination now */
    if (remote.hostname == (char *)NULL)
	remote.hostname = remote.type->typename;
    remote.ntopics = vcrack(remote.alltopics, remote.topics, MAXTOPICS);

    return(SUCCEED);
}

static void dopost()
/* wrapper function for BBS-specific posting code */
{
    static bool punted = FALSE;

    if (punted)
	errmail("bbsauto punted on previous message, try resending this one");
    else
    {
	if (setjmp(abortbuf) == 0)
	    (*remote.type->bbspost)();
	else
	{
	    errmail("gateway punted during post attempt");
	    punted = TRUE;
	}
    }
}

void bbsfetch()
/* do transactions with the current BBS host */
{
    char	dialcmd[BUFSIZ], *dcvec[MAXARGS];
    FILE	*mailfp, *newsfp;
    int		i;
    time_t	starttime;
    jmp_buf abortsave;

    /* prepare the dial-out command */
    if (remote.dial == (char *)NULL)
	(void) snprintf(dialcmd, sizeof(dialcmd), "cu %s", remote.servname);  /* FIXED: OpenBFD */
    else
	(void) strcpy(dialcmd, remote.dial);

    /* if it's cu(1) or tip(1), we know how to make it exit gracefully */
    if (strindex(remote.dial,"cu")!=FAIL || strindex(remote.dial,"tip")!=FAIL)
	(void) strcpy(dkill, "~.\n");

#ifdef DEBUG
    if (debug >= D_SHOWCU)
	(void) strcat(dialcmd, " -d");
    if (debug >= D_SHOWPHASE)
	log1("dial command is: %s", remote.dial);
#endif /* DEBUG */
    (void) vcrack(remote.dial, dcvec, MAXARGS);

#ifdef SIGCLD
    (void) signal(SIGCLD, oncld);
#endif /* SIGCLD */
    (void) signal(SIGPIPE, oncld);

    /*
     * try to be sure that exit from this thing won't leave the dialer
     * hanging on the line, because that could get *real* expensive...
     */
    (void) signal(SIGINT, hangup);
    (void) signal(SIGQUIT, hangup);
    (void) signal(SIGTERM, hangup);
#ifdef DEBUG
    if (!debug)
#endif /* DEBUG */
    {
	(void) signal(SIGSEGV, hangup);
	(void) signal(SIGBUS, hangup);
    }

    /* open the dialer to the remote as a communications server */
    if ((comm = srvopen(dcvec[0], dcvec)) == (server_t *)NULL)
	xerror1("call failed, errno %d", errno);

    /* if the dialer is cu-compatible, we know what to expect */
    if (strindex(dcvec[0], "cu") == FAIL || strindex(dcvec[0], "tip") == FAIL)
	expect("Connected\007\r\n");
    
    starttime = time((time_t *)NULL);

    /*
     * The section starting here may be time-critical if we're on a commercial
     * service like CompuServe. So we bring in batches and post what's waiting
     * as quickly as possible, then postprocess the retrieved stuff.
     */

    if (setjmp(abortbuf) == 0)
    {
#ifdef DEBUG
	if (debug >= D_SHOWPHASE)
	    logerr0("logon sequence begins...");
#endif /* DEBUG */
	(*(remote.type->bbslogon))();	/* log us on */

	/*
	 * Here's the read dialogue. This should pick up both news and
	 * mail equivalents off the BBS.
	 */
	if (infile[0] == '\0' || getnews)
	{
#ifdef DEBUG
	    if (debug >= D_SHOWPHASE)
		logerr0("read dialogue begins...");
#endif /* DEBUG */
	    if ((newsfp = fopen(remote.newsfile, "w")) == (FILE *)NULL)
		xerror1("can't open news capture file %s!", remote.newsfile);
	    if ((mailfp = fopen(remote.mailfile, "w")) == (FILE *)NULL)
		xerror1("can't open mail capture file %s!", remote.mailfile);
	    (*(remote.type->bbsread))(newsfp, mailfp);
	    (void) fclose(newsfp);
	    (void) fclose(mailfp);
	}

	/*
	 * The posting code. Infile is guaranteed to have been preprocessed
	 * into news batch form (with length headers) so batchproc() will
	 * work.
	 */
	if (infile[0] != '\0')
	{
#ifdef DEBUG
	    if (debug >= D_SHOWPHASE)
		logerr0("post dialogue begins...");
#endif /* DEBUG */

	    /*
	     * This is tricky. We want expect() failures and punt()s inside
	     * dopost() to be handled at the dopost() level. So, while the
	     * batch processing is going on, we make abortbuf available
	     * for dopost() to setjmp on.
	     */
	    (void) memcpy(abortsave, abortbuf, sizeof(jmp_buf));
	    (void) batchproc(strcmp(infile,"-")?infile:(char*)NULL, dopost, 0);
	    (void) memcpy(abortbuf, abortsave, sizeof(jmp_buf));
	}

#ifdef DEBUG
	if (debug >= D_SHOWPHASE)
	    logerr0("logoff sequence begins...");
#endif /* DEBUG */
	(*(remote.type->bbslogoff))();	/* log out */
    }
    /*
     * Critical section ends here.
     */

    log2("bbsauto spent %ld seconds on %s",
	     (long)(time((time_t *)NULL) - starttime), remote.servname);

    /* tell the child to go away gracefully */
#ifdef SIGCLD
    (void) signal(SIGCLD, SIG_DFL);
#endif /* SIGCLD */
    (void) signal(SIGPIPE, SIG_DFL);
    (void) signal(SIGINT, SIG_DFL);
    (void) signal(SIGQUIT, SIG_DFL);
    (void) signal(SIGTERM, SIG_DFL);
#ifdef DEBUG
    if (!debug)
#endif /* DEBUG */
    {
	(void) signal(SIGSEGV, SIG_DFL);
	(void) signal(SIGBUS, SIG_DFL);
    }

    hangup();
}

static int rebatch(ifp, ofp)
/* decorate a sequence of messages with batch headers */
FILE	*ifp, *ofp;
{
    long	lcount;

    /* this code assumes no spurious FIRSTs in the articles */
    while (!feof(ifp))
    {
	/* if there are no headers left, we're done */
	hfree(&header);
	if (hread(&header, 0, ifp) <= 0)
	    return(FAIL);

	/* we're at start of text, count chars and lines */
	lcount = 0;
	do {
	    (void) fgets(bfr, BUFLEN, ifp);
	    lcount++;
	} while
	    (!feof(ifp) && strncmp(FIRST, bfr, sizeof(FIRST) - 1));
	header.h_endoff = ftell(ifp);
	if (!feof(ifp))
	{
	    header.h_intnumlines = --lcount;
	    header.h_endoff -= strlen(bfr);
	}
	(void) fseek(ifp, header.h_textoff, SEEK_SET);

	/* we're at start of text again, emit new header */
	(void) fprintf(ofp,"#! rnews %ld\n",
		       (long)(header.h_endoff-header.h_startoff));
	(void) hwrite(&header, ofp, TRUE);

	/* and the new text */
	while (lcount-- > 0)
	{
	    (void) fgets(bfr, LBUFLEN, ifp);
	    (void) fputs(bfr, ofp);
	}
    }
    return(SUCCEED);
}

static void postproc()
/* translate captured news and mail to UNIX form and submit it */
{
    char articles[BUFSIZ], newsbatch[BUFSIZ], mailbatch[BUFSIZ];
    char *cargv[MAXARGS];
    FILE *ifp, *mfp, *nfp;

#ifdef DEBUG
    if (debug >= D_KEEPFILES && !filter)
	(void) log2("news and mail capture files are %s and %s",
		    remote.newsfile, remote.mailfile);
#endif /* DEBUG */

    /*
     * First postprocess captured mail into UNIX mailbox form.
     */
    (void) strcpy(mailbatch, "/tmp/bbsmailXXXXXX");
    (void) mktemp(mailbatch);
    if ((mfp = fopen(mailbatch, "w")) == (FILE *)NULL)
	logerr0("couldn't reopen mailbatch file for mail transcription");
    else if ((ifp = fopen(remote.mailfile, "r")) == (FILE *)NULL)
	logerr0("couldn't reopen mail capture file");
    else
    {
	(*(remote.type->tomail))(ifp, mfp);

	/* cleanup time */
	(void) fclose(ifp);
#ifdef DEBUG
	if (debug < D_KEEPFILES && !filter)
#endif /* DEBUG */
	    (void) unlink(remote.mailfile);
    }

    /*
     * Grind news into local form and post it.
     * Some `news' may actually be mail -- leave the mail batch file open
     * during news transcription so we can deal with this
     */
    (void) strcpy(articles, "/tmp/bbsartsXXXXXX");
    (void) mktemp(articles);
    if ((ifp = fopen(remote.newsfile, "r")) == (FILE *)NULL)
	logerr0("couldn't reopen news capture file");
    else if ((nfp = fopen(articles, "w")) == (FILE *)NULL)
	logerr0("couldn't reopen temp file for news transcription");
    else
    {
	(*(remote.type->tonews))(ifp, nfp, mfp);

	(void) fclose(ifp);
#ifdef DEBUG
	if (debug < D_KEEPFILES && !filter)
#endif /* DEBUG */
	    (void) unlink(remote.newsfile);
	(void) fclose(nfp);

	/* decorate the batch with batch header lines etc. */
	(void) strcpy(newsbatch, "/tmp/bbsnewsXXXXXX");
	(void) mktemp(newsbatch);
	if ((ifp = fopen(articles, "r")) == (FILE *)NULL)
	    logerr0("couldn't reopen article list file");
	else if ((nfp = fopen(newsbatch, "w")) == (FILE *)NULL)
	    logerr0("couldn't reopen temp file for news rebatching");
	else
	{
	    (void) rebatch(ifp, nfp);

	    (void) fclose(ifp);
#ifdef DEBUG
	    if (debug < D_KEEPFILES && !filter)
#endif /* DEBUG */
		(void) unlink(articles);
	}
    }

    /* we can presume we're not going to write more mail or news after this */
    (void) fclose(mfp);
    (void) fclose(nfp);

    /*
     * Finally, feed each piece of the gathered batch mail to UNIX rmail
     * with appropriate arguments. This isn't a perfect forgery (the >From
     * line in the generated message will reveal who actually posted it) but
     * it's close enough -- V7-format-compatible mailers will let you reply
     * to the forged address.
     *
     * Sad to relate, reply capability may not be very useful unless you're
     * running a mail back end at least as flexible as sendmail so you can
     * tell it to direct mail intended for fubar@bbs to bbsauto -m -n bbs.
     * What you *really* want is to direct bbs mail to ~/bbs.mail and then
     * periodically call bbsauto -m bbs.mail.
     */
    if (remote.nmail)
    {
	char	mailcmd[BUFLEN], from[BUFLEN];
	FILE	*fp = (FILE *)NULL;

	mfp = fopen(mailbatch, "r");
	while (mfp && fgets(bfr, BUFLEN, mfp))
	{
	    if (prefix(bfr, MDELIM))		/* message end? */
	    {
		if (fp != (FILE *)NULL)
		    (void) pclose(fp);
	    }
	    else if (prefix(bfr, FIRST))	/* message start? */
	    {
		char	*cp;

		/*
		 * This handles only the RFC-1036 restricted From form,
		 * i.e with fullname if any in parens after the id.
		 * Your BBS-to-UNIX-mail translation hook must generate
		 * this form.
		 */
		(void) strcpy(from, bfr + strlen(FIRST));
		if ((cp = strchr(from, ' ')) != (char *)NULL)
		    *cp = '\0';
		(void) sprintf(mailcmd, RMAIL, from, remote.owner);

		/*
		 * Yes, it's OK to use popen(3) here. Forging mail is trivial
		 * anyhow...
		 */
		fp = popen(mailcmd, "w");
		(void) fputs(bfr, fp);
	    }
	    else if (fp != (FILE *)NULL)	/* an ordinary line */
		(void) fputs(bfr, fp);
	}
	(void) fclose(mfp);
    }
#ifdef DEBUG
    else if (debug >= D_SHOWPHASE)
	log0("no mail gathered");
#endif /* DEBUG */

#ifdef DEBUG
    if (debug < D_KEEPFILES && !filter)
#endif /* DEBUG */
	(void) unlink(mailbatch);

    /* 
     * Similarly, feed the massaged bbs postings to rnews.
     */
    if (remote.nnews)
    {
	char cmdbuf[BUFSIZ];

	(void) sprintf(cmdbuf, "%s/rnews <%s", site.libdir, newsbatch);
	if (nospool)
	    (void) strcat(cmdbuf, " -u");
#ifdef DEBUG
	if (debug)
	    (void) sprintf(cmdbuf + strlen(cmdbuf), " -D %d", debug);
	if (debug >= D_SHOWPHASE)
	    log1("rnews command is \"%s\"", cmdbuf);
#endif /* DEBUG */
	(void) system(cmdbuf);

	/*
	 * If this stuff is supposed to be posted immediately,
	 * mail notification to people on the notify list so they'll
	 * know to read it immediately.
	 */
	if (nospool && remote.notify)
	{
	    (void) sprintf(mailbatch, "News from %s", remote.servname);
	    nfp = mailopen(remote.notify, mailbatch);
	    (void) fprintf(nfp,
			   "\nNews arrived from %s\n", remote.servname);
	    (void) mailclose(nfp);
	}
    }
#ifdef DEBUG
    else if (debug >= D_SHOWPHASE)
	log0("no news gathered");
#endif /* DEBUG */

#ifdef DEBUG
    if (debug < D_KEEPFILES && !filter)
#endif /* DEBUG */
	(void) unlink(newsbatch);
}

main(argc, argv)
int argc;
char *argv[];
{
    char newsh[BUFLEN], myh[BUFLEN];

    (void) setbuf(logfp, (char *)NULL);

    newsinit();

    if (procopts(argc, argv, DNC, options) == FAIL)
	xerror0(usage);

    (void) sprintf(myh, "%s/.bbsauto", userhome);
    (void) sprintf(newsh, "%s/bbsauto", site.libdir);
    if (readparams(service, myh) == FAIL && readparams(service, newsh) == FAIL)
	xerror1("can't load parameters for service %s", service);

    /* initialize capture files */
    remote.nnews = 0;
#ifdef DEBUG
    if (snarf)
	(void) strcpy(remote.newsfile, "news.capture");
    else
#endif /* DEBUG */
    {
	(void) strcpy(remote.newsfile, "/tmp/takenewsXXXXXX");
	(void) mktemp(remote.newsfile);
    }
    remote.nmail = 0;
#ifdef DEBUG
    if (snarf)
	(void) strcpy(remote.mailfile, "mail.capture");
    else
#endif /* DEBUG */
    {
	(void) strcpy(remote.mailfile, "/tmp/takemailXXXXXX");
	(void) mktemp(remote.mailfile);
    }

    /*
     * Make sure input is in batch form with headers
     */
#ifdef DEBUG
    if (infile[0] && !filter)
#else
    if (infile[0])
#endif /* DEBUG */
    {
	FILE	*ofp;
	int	c;

	/* interpret - file name as command to read from stdin */
	if (strcmp(infile, "-"))
	    (void) freopen(infile, "r", stdin);

	/* assume it's already in batch form if first char is OK */
	if ((c = fgetc(stdin)) != '#')
	{
	    char batchtmp[BUFLEN];

#ifdef DEBUG
	    if (debug >= D_SHOWPHASE)
		log0("rebatching input...");
#endif /* DEBUG */
	    (void) ungetc(c, stdin);
	    (void) strcpy(batchtmp, "/tmp/rebatchXXXXXX");
	    (void) mktemp(batchtmp);
	    ofp = xfopen(batchtmp, "w");
	    if (rebatch(stdin, ofp) == FAIL)
		logerr0("garbage header ended input batch");
	    (void) fclose(ofp);
	    (void) strcpy(infile, batchtmp);
	    deletein = TRUE;
	}
    }

#ifdef DEBUG
    /* debugging switches to enable testing of news and mail-cracking code */
    if (filter && infile[0] != '\0')
    {
	if (fmail)
	{
	    (void) strcpy(remote.mailfile, infile);
	    remote.nmail = 1;
	}
	else
	{
	    (void) strcpy(remote.newsfile, infile);
	    remote.nnews = 1;
	}
    }
    else
#endif /* DEBUG */
	bbsfetch();		/* all the real work gets done here */

#ifdef DEBUG
    if (!snarf)
#endif /* DEBUG */
	postproc();	/* translate news & mail to local form and submit */

    xxit(0);
}

catch_t xxit(n)
/* make clean exit, releasing the phone line */
int	n;
{
#ifdef DEBUG
    if (!filter)
#endif /* DEBUG */
	hangup();
    if (deletein)
	(void) unlink(infile);
    exit(n);
}

/* bbsauto.c ends here */
