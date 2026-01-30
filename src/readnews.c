/****************************************************************************

NAME
   readnews.c -- command interpreter for the msgs and /bin/mail interfaces

SYNOPSIS
   #include "vio.h"

   main(argc, argv)	    -- main sequence of the news reader
   int argc; char *argv[];

   void vcommand()	    -- get and execute a single command

   void vupdate()	-- force a screen update

   catch_t xxit(stat)	-- exit, cleaning up and returning given status
   int stat;

DESCRIPTION
   This code implements the line-oriented interactive news reader interfaces.
The xxit() function is used by the interrupt handlers defined in readinit.c.
The interface manager functions (see gcmd.c) are implemented here and in
ttylike.c. See readnews.1 for details of the readnews interface:

ENVIRONMENT VARIABLES
   The FIRSTLINE variable controls the format of part of the prompt line.

FILES
   /tmp/subjlist$$	-- subject list file created by listart().

REVISED BY
   Eric S. Raymond
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

*****************************************************************************/
/*LINTLIBRARY*/
#include "news.h"
#include "vio.h"
#include "header.h"
#include "procopts.h"
#include "libread.h"
#include "active.h"
#include "newsrc.h"
#include "dballoc.h"	/* session.h needs the dbdef_t type */
#include "session.h"
#include "gcmd.h"
#ifdef RECMDS
#include "regexp.h"
#endif /* RECMDS */
#include "rfuncs.h"

#define FINDNEWNG	/* show new groups on reader entry */

/* read program modes (DNC is defined in procopts.h) */
#define NORMAL	    0x01    /* normal interactive mode */
#define BATCH	    0x02    /* batch (non-interactive) mode */
#define ANY	    0x03    /* this switch O.K. in either mode */
#define SCHECK	    0x04    /* only the -s option is good */
#define KFLAG	    0x05    /* only the -K option is good */

#ifndef SYSBUF
char	SYSBUF[BUFSIZ];	/* to buffer std out */
#endif

/* xerror() uses Progname to identify failing program */
char *Progname = "readnews";
char *usage =
    "Usage: readnews [-a date] [-n newsgroups] [-t titles] [-cleprxhfusSAK]";

#ifdef RNESCAPES
#define DEFLINE	"Article %a (%U unseen): %i"
private char *firstline;
#endif /* RNESCAPES */
private int pflag, cflag, slowtty, lflag, sflag, eflag, Lflag;

option_t options[] =
{
/*       (grouplist, titlebuf, and datebuf are in readinit.c)
optlet  filchar	flag	    from      to       type     buf	*/
'n', NGDELIM,	NONE,	    ANY,      DNC,     STRING,  grouplist,
'a',	 ' ',	NONE,	    ANY,      DNC,     STRING,  datebuf,
'c',	 ' ',	&cflag,	    ANY,      DNC,     OPTION,  (char *)NULL,
'e',	'\0',	&eflag,	    BATCH,    BATCH,   OPTION,  (char *)NULL,
'f',	'\0',	&nofoll,    ANY,      DNC,     OPTION,  (char *)NULL,
'h',	'\0',	&slowtty,   ANY,      DNC,     OPTION,  (char *)NULL,
'l',	 ' ',	&lflag,	    BATCH,    BATCH,   OPTION,  (char *)NULL,
'p',	'\0',	&pflag,	    BATCH,    BATCH,   OPTION,  (char *)NULL,
#ifdef FEEDBACK
'q',	'\0',	NONE,	    ANY,      DNC,     STRING,  session.quiet,
#endif /* FEEDBACK */
'r',	'\0',	&session.reverse,   ANY,      DNC,     OPTION,  (char *)NULL,
's', NGDELIM,	&sflag,	    SCHECK,   SCHECK,  STRING,  grouplist,
't',	'\0',	NONE,	    ANY,      DNC,     STRING,  titlebuf,
'u',	'\0',	&asave,	    NORMAL,   NORMAL,  OPTION,  (char *)NULL,
'x',	'\0',	&session.reread,    ANY,      DNC,     OPTION,  (char *)NULL,
'F',	 ' ',	NONE,	    ANY,      DNC,     STRING,  inclmark,
'K',	'\0',	&flushall,  KFLAG,    KFLAG,   OPTION,  (char *)NULL,
'L',	'\0',	&Lflag,	    ANY,      DNC,     OPTION,  (char *)NULL,
#ifdef BYTHREADS
'S',	'\0',	&session.thread,    ANY,      DNC,     OPTION,	(char *)NULL,
#endif /* BYTHREADS */
#ifdef DEBUG
'D',	'\0',	&debug,     ANY,      DNC,     NUMBER,  (char *)NULL,
#endif /* DEBUG */
'\0',	'\0',	0,	    0,        0,       0,       (char *)NULL,
};

/*ARGSUSED0*/
static void idler(arg)	int arg; {	/* no-op, for now */	}

main(argc, argv)
/* main sequence of the readnews reader */
int argc;
char *argv[];
{
    forward void listsubjs(), tprint(), vprompt();
    forward bool ngcheck();
    int	    unread;			/* count of unread messages */

    /* force stdout flush on NL even if stdout isn't a tty */
    (void) setbuf(stdout, SYSBUF);

    /* initialize the universe */
    if ((unread = readinit(argc, argv, procdlines)) <= 0)
    {
	(void) fprintf(stdout, "No news\n");
	exit(0);
	/*NOTREACHED*/
    }

    if (sflag)
    {
	(void) printf("Subscription list:  %s\n", grouplist);
	exit(0);	/* don't need xxit(0) here */
	/*NOTREACHED*/
    }

#ifdef CHECKMAIL
    getmailname();
#endif /* CHECKMAIL */

    /* we do this to suppress .newsrc writeout on termination */
    if (lflag)
	nochange++;

    /*
     * setup for miscellaneous other options
     */
    if (pflag && bitbucket(stdout))
	flushall = TRUE;
#ifdef RNESCAPES
    setexphook(vexpand);
#endif /* RNESCAPES */

#ifdef FINDNEWNG
    /*
     * Next, show new groups if any and we're in interactive mode
     */
    if (!cflag && !eflag && !eflag && !pflag && !flushall)
	(void) ngcheck(stdout);
#endif /* FINDNEWNG */

    /* give user some idea how much awaits */
    if (!flushall)
	msg1("There appear to be %d messages waiting", unread);

#ifdef RNESCAPES
    if ((firstline = getenv("FIRSTLINE")) == (char *)NULL)
	firstline = DEFLINE;
#endif /* RNESCAPES */


    /* set up the idler function */
    vsetidle(idler);

    /*
     * Here we go with the main read-display-execute loop.
     */
    do {
	sigcaught = NO_SIGNAL;

	/*
	 * read the next message, unless the last time around ran us
	 * out of messages (in which case the NOMORE bit is set).
	 */
	if (!pending(NOMORE))
	{
	    if (msgnext() == FAIL)
		request(NOMORE);
	    else if (msgtext() == FAIL)
	    {
		session.action = M_NEXT;
		(void) msgmark(READ, GLOBAL);
		--session.ngrp;
		continue;
	    }
	}
	/*
	 * autosave at intervals
	 */
	clockdaemon(wrnewsrc, asave * SAVESECS);

	/*
	 * if an interrupt came in while we were seeking to the next
	 * article, handle it now
	 */
	if (sigcaught)
	{
	    if (sigcaught == SIGHUP)
		return(SUCCEED);
	    if (pflag || eflag || lflag)
		xxit(0);
	    (void) printf("Abort (n)?  ");
	    (void) fflush(stdout);
	    (void) gets(bfr);
	    if (*bfr == 'y' || *bfr == 'Y')
		xxit(0);
	    sigcaught = NO_SIGNAL;
	}

#ifdef SUBJFILE
	if (session.ngrp == 1)
	{
	    /* if user specified -L and we're on new group, show list now */
	    if (Lflag)
	    {
		listsubjs();
		session.action = M_NEXT;
	    }
	}
#endif /* SUBJFILE */

	if (pending(NOMORE))
	{
	    /* avoid picking up final command unless we need it */
	    if (session.news == 0 || pflag || lflag || eflag || flushall)
		break;
	}
	else
	{
	    /*
	     * display the message header if indicated
	     */
	    if (flushall)
	    {
		(void) msgmark(READ, GLOBAL);
		continue;
	    }
	    else
		vupdate();

	    /*
	     * display the article selected if we're in -c mode or one of the
	     * non-interactive modes; any interrupt will drop us out.
	     */
	    if (pflag || lflag || eflag || cflag)
	    {
		sigcaught = NO_SIGNAL;

		if (pflag)
		    tprint(header.h_fp, stdout, FALSE);
		else if (cflag && !lflag && !eflag)
		{
		    (void) fflush(stdout);
		    (void) more(msgfile(), (FILE *)NULL, header.h_intnumlines);
#ifdef FEEDBACK
		    msgrate(F_SEENTEXT);
#endif /* FEEDBACK */
		}

		if (sigcaught)
		{
		    qfflush(stdout);
		    (void) putchar('\n');
		    return(SUCCEED);
		}

		if (!cflag)
		    continue;   /* go to next message */
	    }
	}

	do {
	    unset(CMDERR);
	    vprompt(bfr);
	    vcommand(gparse(vgetcmd(bfr)));
	    vclearin();
	} while
	    (pending(CMDERR));

	if (!cflag)
	    (void) putchar('\n');
    } while
	(!pending(EXIT));

    if (pending(NOMORE) && !flushall)
	msg0("No more articles");

    nochange = !flushall && !pending(NOMORE);
    cleanup();
    /*NOTREACHED*/
}

#ifdef FINDNEWNG
private bool ngcheck(ofp)
/* detect newsgroups that have been created since the last read session */
FILE	*ofp;
{
    int		newcount = 0, unscount = 0;
    group_t	*gsave = ngactive();

    ngrewind(TRUE);
    while (ngnext())
    {
	if (!rcflag(RC_NOTED))
	{
	    if (newcount++ == 0)
		(void) fprintf(ofp, "There are new newsgroups.\n");
	    if (rcsubsc(ngactive()))
		(void) fprintf(ofp,"You are subscribed to '%s'.\n",ngname());
	    else if (rcflag(RC_UNSUB))
		(void) fprintf(ofp,"You are unsubscribed from '%s'.\n",ngname());
	    else
	    {
		if (ngmatch(ngname(), ADMSUB))
		    continue;
		unscount++;
		(void) fprintf(ofp,"Do you want to see '%s?' [yn] ",ngname());
		(void) fflush(ofp);
		(void) fgets(bfr, BUFLEN, stdin);
		if (strchr(bfr, 'n'))
		    (void) unsubscribe();
	    }
	}
    }
    ngselect(gsave);

    /* return TRUE if we should pause for confirm before trashing screen */
    return(newcount && !unscount);
}
#endif /* FINDNEWNG */

void vcommand(cmdargs)
/* process a single enqueued command */
cmdarg_t	*cmdargs;
{
    forward void caesar_command();	/* decrypt a rotate-encoded article */
    forward void help();		/* get on-line help */
    forward void dash();

    int	ch, minus = 0;
    bool undigest = FALSE;	/* TRUE if we need to undigest */

    /* detect trailing garbage on commands that shouldn't have arguments */
    if ((strchr("ndypqx?", cmdargs->c_char) != (char *)NULL)
	&& cmdargs->c_char != '\0')	/* c_char may legally be NULL */
    {
	char nc = vgetc();

	if (isascii(nc) && isprint(nc))
	{
	    msg2("Bad character '%c' after '%c'.", nc, cmdargs->c_char);
	    demand(HOLD);
	    return;
	}
    }

    /* back up if the command suffix demands it */
    while ((ch = vgetc()) == '-')
	if (msglast() == SUCCEED)
	    minus--;
	else
	{
	    (void) msg2("can't back up further than %s/%ld\n",
			ngname(), (long)msgnum());
	    break;
	}
    vungetch(ch);

    switch (cmdargs->c_char)
    {
    case 'l':
#ifdef SUBJFILE
	listsubjs();
#else
	msg0("The subject page facility has been disabled to save space.");
#endif /* SUBJFILE */
	demand(HOLD);
	break;

    case 'd':		/* Undigestify the article. */
	undigest = TRUE;
	/* fall through */

    case '\0':		/* the default command */
	if (cmdargs->c_argform == C_COUNT)
	{
	    /* user specified an article to go to */
	    cmdargs->c_char = 'A';
	    gcmd(cmdargs);
	    break;
	}
	else if (pending(NOMORE))	/* 'q' if islast, otherwise 'y'. */
	{
	    demand((EXIT | NOMORE));
	    return;
	}
	/* fall through */

    case 'y':		/* yes: print this article, go on. */
	(void) msgmark(READ, GLOBAL);
	if (undigest)
	    (void) digest(header.h_fp, stdout, &header);
	else if (!lflag && !pflag && !eflag)
	{
	    (void) more((char *)NULL, header.h_fp, header.h_intnumlines);
#ifdef FEEDBACK
	    msgrate(F_SEENTEXT);
#endif /* FEEDBACK */
	}
	(void) msgmark(READ, GLOBAL);
	session.action = M_NEXT;
	break;

    case 'D':		/* decrypt joke */
	caesar_command(vgetline((char *)NULL));
	(void) msgmark(READ, GLOBAL);
	session.action = M_NEXT;
	break;

    case 'H':		/* display a more verbose header for this message */
    case 'h':
	if (!slowtty)
	    dash(8, stdout);
	(void) printf("Article %ld of %ld: %s\n",
	    (long)msgnum(), (long)ngmax(), header.h_ident);
	if (cmdargs->c_char == 'H')
	    hwrite(&header, stdout, FALSE);
	else
	    hprint(&header, !slowtty, NOHIGH);
	if (!slowtty)
	    dash(8, stdout);
	session.action = M_SEEK;   /* don't want to skip this message */
	break;

    default:		/* hand us off to the generic code */
	if (tolower(cmdargs->c_char) == 'r' && (vgetc() == 'd'))
	    cmdargs->c_count = 0L;
#ifdef B211COMPAT
	if (cmdargs->c_char == '+')	/* the old prefix-plus form */
	    (void) lgetnum(cmdargs->c_count);
#endif /* B211COMPAT */
	gcmd(cmdargs);
	if (pending(EXIT))
	    return;
	else if (!pending(CMDERR))
	    break;
	msg0("? for commands");
	/* FALL THROUGH */

    case '?':		/* display the help file */
	help(stdout);
	(void) fflush(stdout);
	cmdargs->c_char = '#';
	gcmd(cmdargs);
	break;
    }

    while (minus--)
    {
	session.action = M_NEXT;
	(void) msgnext();
    }
}

private void vprompt(pr)
/* prompt the user for a command */
char	*pr;
{
    pr[0] = '\0';
    if (dclock.justsaved)
	(void) strcat(pr, "[Saved .newsrc]");
#ifdef CHECKMAIL
    if (dclock.mailstate == M_NEWMAIL)
	(void) strcat(pr, "You have new mail.");
#endif /* CHECKMAIL */
#ifdef SHOWTIME
    (void) strcat(pr, "(");
    (void) sprintf(pr, dclock.time);
    (void) strcat(pr, ") ");
#endif /* SHOWTIME */    

    if (cflag)
	(void) sprintf(bfr, "? ");	/* the /bin/mail style prompt */
    else
    {
	if (pending(NOMORE))
	    (void) sprintf(pr + strlen(pr), "Last article.  [qfr] ");
	else
	{
	    if (header.h_intnumlines <= 0)
		(void) sprintf(pr + strlen(pr), "(0 lines) Next? [nqfr] ");
	    else
		(void) sprintf(pr + strlen(pr), "(%d lines) More? [ynq] ",
			       header.h_intnumlines);
	}
    }
}

/*
 * the following functions are used to print headers
 */

#ifndef RNESCAPES
private char *briefdate(dt)
/* print a very brief version of the date in question. */
time_t dt;
{
    time_t now;
    char *tmstr;
    char *wkday, *monthdate, *timeofday;
    static char rbuf[20];

    tmstr = ctime(&dt);

    wkday = tmstr; tmstr[3] = '\0';
    monthdate = tmstr+4; tmstr[10] = '\0';
    timeofday = tmstr+11; tmstr[16] = '\0';

    (void) time(&now);
    if (now - dt < 7 * DAYS)
	(void) strcpy(rbuf, wkday);
    else
	(void) strcpy(rbuf, monthdate);
    (void) strcat(rbuf, " ");
    (void) strcat(rbuf, timeofday);
    return(rbuf);
}
#endif /* !RNESCAPES */

void vupdate()
/* print out the appropriate header (also called by SIGCONT handler)*/
{
    forward void nghprint();

    if (lflag || eflag)
    {
	char buf1[80];

	(void) sprintf(buf1, "%s/%ld", ngname(), (long)msgnum());
	(void) sprintf(bfr, "%-20s %s", buf1, header.h_subject);
	(void) printf("%.76s\n", bfr);
    }
    else    /* usual interactive case */
    {
	/* print out a group header if this is 1st message in a new group */
	if (session.ngrp == 1)
	    nghprint(ngname());

	/* be a bit less verbose if the user specified slowtty */
	if (!slowtty)
#ifndef RNESCAPES
	    (void) printf("Article %ld of %ld, %s.\n",
		(long)msgnum(), (long)ngmax(), briefdate(header.h_posttime));
#else
	{
	    strexpand(firstline, bfr);
	    (void) puts(bfr);
	}
#endif /* RNESCAPES */
	hprint(&header, pflag, NOHIGH);
    }
}

private void nghprint(title)
char *title;
{
    forward void dash();
    int	    l;

    /* assemble the entire header in a buffer, because we need the length */
    (void) sprintf(bfr, "Newsgroup %s", title);
    if (!slowtty)
    {
	if (msgnum() == ngmax())
	    (void) sprintf(bfr + strlen(bfr), ": %ld", (long)ngmax());
	else
	    (void) sprintf(bfr + strlen(bfr), ": %ld-%ld",
				(long)msgnum(), (long)ngmax());
    }
    (void) sprintf(bfr + strlen(bfr), " (%d unread)", ngunread());

    (void) putchar('\n');
    if (!slowtty)
	dash(l = strlen(bfr), stdout);
    (void) printf("%s\n", bfr);
    if (!slowtty)
	dash(l, stdout);
    (void) putchar('\n');
}

private void dash(num, fp)
register int	num;
register FILE *fp;
{
    register int	i;

    for (i = 0; i < num; i++)
	(void) putc('-', fp);
    (void) putc('\n', fp);
}

private void tprint(ifp, ofp, checkfrom)
/* print ifp to ofp, watching for interrupts */
register FILE *ifp, *ofp;
bool checkfrom;
{
    while ((fgets(bfr, sizeof bfr, ifp)) != (char *)NULL && !sigcaught)
    {
	if (checkfrom && strncmp(bfr, "From ", 5) == 0)
	    (void) putc('>', ofp);
	(void) fputs(bfr, ofp);
    }
    if (sigcaught)
	qfflush(ofp);
    (void) fflush(ofp);
    (void) fprintf(ofp, (sigcaught ? "\n\n" : "\n"));
    sigcaught = FALSE;
}

#ifdef SUBJFILE
void listsubjs()
{
    int	i;

    for (i = ngmin(); i <= ngmax(); i++)
	if (indexline(ngactive(), i, bfr + PREFSPACE) != FAIL)
	{
	    vsubjline(i, bfr);
	    (void) fprintf(stdout, "%s\n", bfr);
	}
}
#endif /* SUBJFILE */

private void caesar_command(bptr)
char	*bptr;
{
    FILE	*pfp;
#ifdef PAGE
    char	*pager;

    if ((pager = getenv("PAGER")) == (char *)NULL)
	pager = PAGE;
#endif /* PAGE */

    (void) fprintf(stdout, "Caesar decoding:\n");
    (void) sprintf(bfr, "%s/%s", site.libdir, "caesar");
    if (*bptr)
    {
	(void) strcat(bfr, " ");
	(void) strcat(bfr, bptr);
    }
#ifdef PAGE
    if (*pager)
    {
	(void) strcat(bfr, " | ");
	(void) strcat(bfr, pager);
    }
#endif /* PAGE */
    pfp = popen(bfr, "w");
    tprint(header.h_fp, pfp, FALSE);
    (void) pclose(pfp);
}

private void help(fp)
register FILE *fp;
{
    register FILE *sfp;
    register int	c;

    if (!cflag)
    {
	(void) sprintf(bfr, "%s/%s", site.admdir, "readnews.help");
	if ((sfp = fopen(bfr, "r")) == (FILE *)NULL)
	    (void) fprintf(fp, "No help file.\n");
	else
	{
	    while ((c = getc(sfp)) != EOF && !sigcaught)
		(void) putc(c, fp);
	    (void) fclose(sfp);
	    return;
	}
    }

    (void) fprintf(fp,
	"(n)ext (w)rite (q)uit (r)eply (f)ollowup (N)ext (U)nsubscribe\n");
}

#ifdef RNESCAPES
static bool vexpand(cgetf, cungetf, buf)
/* expand the given reader-specific %-construct into the given buffer */
int (*cgetf)();		/* key character of escape */
int (*cungetf)();	/* how to push back characters */
char *buf;		/* buffer to expand esacapes into */
{
    /* readnews.c has no special escapes */
    return(rexpand(cgetf, cungetf, buf));
}
#endif /* RNESCAPES */

catch_t xxit(status)
int	status;
{
    exit(status);
}

#ifdef lint	/* this just shuts up lint for when we link with libread.a */
int vstat;
void vuline(to, from) char *to, *from; {(void)strcpy(to, from);}
#endif /* lint */

/* readnews.c ends here */
