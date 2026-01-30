/****************************************************************************

NAME
   ednews.c -- command interpreter for the ednews server

SYNOPSIS
   main(argc, argv)	    -- main sequence of the news reader
   int argc; char *argv[];

   void vcommand()	    -- get and execute a single command

   void vupdate()	-- force a screen update

   catch_t xxit(stat)	-- exit, cleaning up and returning given status
   int stat;

DESCRIPTION
   This code implements  a special stripped-down form of readnews designed to
be used as back end to EMACS or other programs that can do their own message
access and windowing. See ednews(1) for details.

NOTE
   It's O.K. to hack more responses into ednews if you really need more data
on what the server is doing, but please try to preserve the property that
responses can be distinguished by a leading ^G and a unique second letter --
and add code that recognizes those responses to the ednewsipc test driver!

BUGS
   The 'BYE' response from a child ednews can get lost if the child gives
up the ghost before read_ednews() does its fgets(). On AT&T UNIXes we can
trap SIGCLD and use it to post the 'BYE' condition; on V7/BSD versions
we have to make the child wait until the 'BYE' is acknowledged, or kill
it explicitly from the parent.
    It's not clear that the msgrate() calls for the rating system should
be in the main loop. The front end might accept an article and view no more
than the header. Perhaps the ednews protocol needs a special command message
for this?

FILES
   /tmp/newgroups??????	    -- hold a topic list
   /tmp/grpindex??????	    -- hold a group subject list

SEE ALSO
   readinit.c	-- the newsreader initialization and wrapup code
   rfuncs.c	-- common functions used by interactive news readers

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
#include "nextmsg.h"
#include "dballoc.h"	/* session.h needs the dbdef_t type */
#include "session.h"
#ifdef RECMDS
#include "regexp.h"
#endif /* RECMDS */
#include "rfuncs.h"
#include "gcmd.h"

/* xerror() uses Progname to identify failing program */
char *Progname = "ednews";
char *usage = "Usage: ednews [-n newsgroups] [-aftuxS]";

#ifndef SYSBUF
char	SYSBUF[BUFSIZ];	/* to buffer std out */
#endif

private char artfile[BUFLEN];
private bool filtered = FALSE;

option_t options[] =
{
/*       (grouplist, titlebuf, and datebuf are in readinit.c)
optlet  filchar	flag	    from      to       type     buf	*/
'n', NGDELIM,	NONE,	    DNC,      DNC,     STRING,  grouplist,
'a',	 ' ',	NONE,	    DNC,      DNC,     STRING,  datebuf,
'f',	'\0',	&nofoll,    DNC,      DNC,     OPTION,  (char *)NULL,
#ifdef FEEDBACK
'q',	'\0',	NONE,	    DNC,      DNC,     STRING,  session.quiet,
#endif /* FEEDBACK */
't',	'\0',	NONE,	    DNC,      DNC,     STRING,  titlebuf,
'u',	'\0',	&asave,	    DNC,      DNC,     OPTION,  (char *)NULL,
'x',	'\0',	&session.reread,    DNC,      DNC,     OPTION,  (char *)NULL,
#ifdef BYTHREADS
'S',	'\0',   &session.thread,    DNC,      DNC,     OPTION,	(char *)NULL,
#endif /* BYTHREADS */
#ifdef DEBUG
'D',	'\0',	&debug,     DNC,      DNC,     NUMBER,  (char *)NULL,
#endif /* DEBUG */
'\0',	'\0',	0,	    0,        0,       0,       (char *)NULL,
};

main(argc, argv)
/* main sequence of the readnews reader */
int argc;
char *argv[];
{
    forward void listsubjs();
    forward void show_header();

    int	    unread;			/* count of unread messages */
    FILE    *indfp;

    /* initialize the universe */
    if ((unread = readinit(argc, argv, procdlines)) <= 0)
	request(NOMORE);

#ifdef RNESCAPES
    setexphook(vexpand);
#endif /* RNESCAPES */

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
		 continue;
	     else if (filtered)
		 (void) unlink(artfile);
	     (void) strcpy(artfile, msgfile());
	     filtered = FALSE;
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
	    xxit(0);
	    /*NOTREACHED*/
	}

	/* now ship out the appropriate messages */
	if (pending(NOMORE))
	{
	    (void) fprintf(stdout, "\007NOMORE\n");
	    (void) fflush(stdout);
	}
	else
	{
	    if (session.news == 1)
	    {
		group_t	*oldgrp = ngactive();

		/* ship the front end a group data list */
		(void) strcpy(bfr, "/tmp/newgrpsXXXXXX");
		(void) mktemp(bfr);
		indfp = xfopen(bfr, "w");
		ngrewind(TRUE);
		while (ngnext())
		{
		    if (session.reread || ngunread())
			(void) fprintf(indfp,
				   "%-40s %-3d %s%s\n",
				   ngname(), ngunread(),
				   rcsubsc(ngactive()) ? "y" : "n",
				   rcflag(RC_NOTED) ? "!" : ""
				   );
		}
		ngselect(oldgrp);
		(void) fclose(indfp);
		(void) fprintf(stdout, "\007TOPICS %s %ld\n", bfr,
			       (long)unread);
		(void) fflush(stdout);
	    }

	    if (session.ngrp == 1)
	    {
		(void) sprintf(bfr, "\007GROUP %s %ld %ld %ld %c",
		    ngname(), (long)ngmin(), (long)ngmax(), (long)ngunread(),
		    rcsubsc(ngactive()) ? SUBSCMK : UNSUBSCMK);

		wrbits(ngactive(), bfr + strlen(bfr));
		(void) fputs(bfr, stdout);
		(void) fputc('\n', stdout);
		(void) fflush(stdout);
	    }

	    (void) fprintf(stdout, "\007ARTICLE %s %d %s\n",
		      artfile, header.h_intnumlines,
		      getbit(msgnum(), ngactive()) ? "seen" : "unseen");
	    (void) fflush(stdout);
	}

	do {
	    unset(CMDERR);
	    vcommand(gparse(vgetcmd((char *)NULL)));
	    vclearin();

	    /* assemble and send the command status message */
	    (void) sprintf(bfr, "\007DONE %02x\n", action);
	    if (session.action == M_SEEK)
		(void) strcat(bfr, " seek");
	    if (session.action == M_NEXT)
		(void) strcat(bfr, " next");
	    if (session.action == M_SKIP)
		(void) strcat(bfr, " skip");
	    if (pending(HOLD))
		(void) strcat(bfr, " hold");
	    if (pending(NOMORE))
		(void) strcat(bfr, " nomore");
	    if (pending(EXIT))
		(void) strcat(bfr, " exit");
	    if (pending(CMDERR))
		(void) strcat(bfr, " cmderr");
	    (void) strcat(bfr, "\n");
	    (void) fputs(bfr, stdout);
	    (void) fflush(stdout);

#ifdef FEEDBACK
	    msgrate(F_SEENTEXT);
#endif /* FEEDBACK */
	} while
	    (pending(CMDERR));

	/* it may be that we really want to reset to reread mode */
	if (!session.reread && pending(NOMORE))
	{
	    session.reread = TRUE;
	    if (unread = rccount(session.reread))
	    {
		session.action = M_NEXT;
		initmsg(session.reread, session.reverse);
	    }
	}
    } while
	(!pending(EXIT));

    nochange = !pending(NOMORE);
    cleanup();
    /*NOTREACHED*/
}

void vcommand(cmdargs)
/* process a single enqueued command */
cmdarg_t	*cmdargs;
{
    forward void listsubjs();	    /* display group index */
    forward void caesar_command();  /* to decrypt a rotate-encoded article */

    int	ch, minus = 0;

    /* detect trailing garbage on commands that shouldn't have arguments */
    if (strchr("ndypqx?", cmdargs->c_char) != (char *)NULL)
    {
	if (isprint(vgetc()))
	{
	    (void) fputs("\007CMDERR Command doesn't take arguments", stdout);
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
	    (void) fprintf(stdout,
			  "\007CMDERR Can't back up further than %s/%ld\n",
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

    case '\0':		/* the default command */
	if (cmdargs->c_argform != C_NONE)
	{
	    /* user specified an article to go to */
	    session.action = M_NEXT;
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

    case 'y':		/* mark this article, go on. */
	(void) msgmark(READ, GLOBAL);
	session.action = M_NEXT;
	break;

    case 'D':		/* decrypt joke */
	caesar_command(vgetline((char *)NULL));
	(void) msgmark(READ, GLOBAL);
	session.action = M_SEEK;
	break;

    default:		/* hand us off to the generic code */
	if (tolower(cmdargs->c_char) == 'r' && (vgetc() == 'd'))
	    cmdargs->c_count = 0L;
#ifdef B211COMPAT
	if (cmdargs->c_char == '+')	/* the old prefix-plus form */
	    (void) lgetnum(cmdargs->c_count);
#endif /* B211COMPAT */
	session.action = M_NEXT;
	gcmd(cmdargs);
	if (pending(CMDERR))
	{
	    (void) fputs("\007CMDERR Illegal Command\n", stdout);
	    (void) fflush(stdout);
	}
	break;
    }

    while (minus--)
    {
	session.action = M_NEXT;
	(void) msgnext();
    }
}

#ifdef SUBJFILE
void listsubjs()
{
    int		i;
    FILE	*pfp;
    char	subjtmp[BUFLEN];

    (void) strcpy(subjtmp, "/tmp/grpindexXXXXXX");
    (void) mktemp(subjtmp);
    pfp = xfopen(subjtmp, "w");

    for (i = ngmin(); i <= ngmax(); i++)
	if (indexline(ngactive(), i, bfr + PREFSPACE) != FAIL)
	{
	    vsubjline(i, bfr);
	    (void) fprintf(pfp, "%s\n", bfr);
	}
    (void) fclose(pfp);
    (void) fprintf(stdout, "\007SUBJECTS %s\n", subjtmp);
    (void) fflush(stdout);
}
#endif /* SUBJFILE */

private void caesar_command(bptr)
char	*bptr;
{
    (void) sprintf(bfr, "%s/caesar <%s >%s", site.libdir, msgfile(), artfile);
    (void) strcpy(artfile, "/tmp/caesarXXXXXX");
    (void) mktemp(artfile);
    if (*bptr)
    {
	(void) strcat(bfr, " ");
	(void) strcat(bfr, bptr);
    }
    (void) system(bfr);
    filtered = TRUE;
}

#ifdef RNESCAPES
static bool vexpand(cgetf, cungetf, buf)
/* expand the given reader-specific %-construct into the given buffer */
int (*cgetf)();		/* how to get key character of escape */
int (*cungetf)();	/* how to push back characters */
char *buf;		/* buffer to expand esacapes into */
{
    /* ednews.c has no special escapes */
    return(rexpand(cgetf, cungetf, buf));
}
#endif /* RNESCAPES */

catch_t xxit(status)
int	status;
{
    (void) fputs("\007BYE\n", stdout);
    (void) fflush(stdout);
#ifndef SIGCLD
    (void) gets(bfr);	/* ugggh...see ednewsipc.c for explanation */
#endif /* SIGCLD */

    exit(status);
}

void vupdate() {/*no-op*/}

#ifdef lint	/* this just shuts up lint for when we link with libread.a */
int vstat;
void vuline(to, from) char *to, *from; {/*no-op*/}
#endif /* lint */

/* ednews.c ends here */
