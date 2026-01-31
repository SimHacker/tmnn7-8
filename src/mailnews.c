/*****************************************************************************

NAME
   mailnews -- use a mailer as the news reader interface

SYNOPSIS
   main(argc, argv)	    -- main sequence of the news reader
   int argc; char *argv[];

   xxit(stat)	    -- exit, cleaning up files
   int stat;

DESCRIPTION
   This module handles news reading through UNIX mailers, depending on the
consistency of the -f option to help. If the symbol TMAIL is defined, the
default mailnews mailer is the Berkeley Mail program with the -T option;
otherwise the local default mailer is used. In either case the -c option
can be used to specify a different mailer. This interface uses the
line-oriented virtual I/O calls.

   This code is also interesting as a reasonably simple model for
writing other interfaces.  We write out all relevant articles to
a temp file, then invoke the specified mailer (with an option to
have it tell us which articles it read if that's available).

   Because there is no command interpreter, newsgroup initial commands are
ignored.

NOTE
   If you use this interface with TMAIL undefined, your execution path must
get you standard UNIX sed(1), sort(1) and comm(1) utilities, else weird things
will happen.

FILES
   /tmp/M[123]XXXXXX	-- mail handling temp files

SEE ALSO
   readinit.c	-- the newsreader initialization and wrapup code

AUTHOR
   Eric S. Raymond
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

*****************************************************************************/
/*LINTLIBRARY*/
#include "news.h"
#include "mailbox.h"
#include "vio.h"
#include "header.h"
#include "libread.h"
#include "procopts.h"
#include "active.h"
#include "newsrc.h"
#include "nextmsg.h"
#include "dballoc.h"	/* session.h needs the dbdef_t type */
#include "session.h"
#ifdef lint
#include "gcmd.h"
#endif /* lint */

/* verbosity level for various messages */
#define V_CHATTY	2	/*  make main sequence chatty */

char *Progname = "mailnews";	/* used by xerror */
char *usage =
"Usage: mailnews [-a date] [-n groups] [-t titles] [-rxfvS] [-c mailer]";

#ifndef TMAIL
private char	mymailer[BUFLEN] = MAILFRONT;
#else
private char	mymailer[BUFLEN] = TMAIL;
#endif /* TMAIL */

private int  dflag;

option_t options[] =
{
/*      (grouplist, titlebuf, and datebuf are in readinit.c)
optlet  filchar	flag	oldstate      newstate type     buf	*/
'n', NGDELIM,	NONE,	    DNC,      DNC,     STRING,  grouplist,
'a',	 ' ',	NONE,	    DNC,      DNC,     STRING,  datebuf,
'c',	 ' ',	NONE,	    DNC,      DNC,     STRING,  mymailer,
'd',	'\0',	&dflag,     DNC,      DNC,     OPTION,  (char *)NULL,
'f',	'\0',	&nofoll,    DNC,      DNC,     OPTION,  (char *)NULL,
#ifdef FEEDBACK
'q',	'\0',	NONE,	    DNC,      DNC,     STRING,  session.quiet,
#endif /* FEEDBACK */
'r',	'\0',	&session.reverse,   DNC,      DNC,     OPTION,  (char *)NULL,
't',	'\0',	NONE,	    DNC,      DNC,     STRING,  titlebuf,
'x',	'\0',	&session.reread,    DNC,      DNC,     OPTION,  (char *)NULL,
'S',	'\0',	&session.thread,    DNC,      DNC,     OPTION,	(char *)NULL,
#ifdef DEBUG
'D',	'\0',	&debug,     DNC,      DNC,     NUMBER,  (char *)NULL,
#endif /* DEBUG */
'\0',	'\0',	0,	    0,        0,       0,       (char *)NULL
};

private char	readbox[BUFLEN] = "/tmp/M1XXXXXX";
private char	msglist[BUFLEN] = "/tmp/M2XXXXXX";
#ifndef TMAIL
private char	newlist[BUFLEN] = "/tmp/M3XXXXXX";
#endif /* TMAIL */

private int catchterm()
{
    /* don't want to update .newsrc file here, else we'd use cleanup() */
    xxit(0);
}

private int takeopts(ltype, line)
/* process directive lines for checknews */
int	ltype;	/* directive type */
char	*line;	/* text of directive */
{
    if (ltype == N_OPTIONS)
	return(lprocargs(line));
    else
	return(SUCCEED);
}

main(argc, argv)
/* main sequence of the mailnews reader */
int argc;
char *argv[];
{
#ifndef TMAIL
    register FILE   *nfp;
#else
    char	    *ptr;
#endif /* TMAIL */
    FILE	    *ofp;
    forward int	    catchterm();

    /* initialize the universe */
    if (readinit(argc, argv, takeopts) <= 0)
    {
	(void) fprintf(stderr, "No news\n");
	exit(0);
    }

#ifdef DEBUG
    if (verbose >= V_CHATTY)
	(void) msg0("Initialization complete.");
#endif /* DEBUG */

    (void) mktemp(readbox);
#ifndef TMAIL
    if (!dflag)
	nfp = xfopen(mktemp(newlist), "w");
#endif /* TMAIL */

    /* loop through the active groups printing selected messages */
    for (; msgread() != FAIL; session.action = M_NEXT)
    {
	/* read header, arrange access to text */
	if (msgtext() == FAIL)
	    continue;
	if (saveto(readbox, S_MAILFMT | S_SAVEHDR | S_NOSIGNAL) == FAIL)
	    xerror2("Append of %s to %s failed", msgfile(), readbox);

#ifndef TMAIL
	if (!dflag)
	    (void) fprintf(nfp, "%s\n", header.h_ident);
#endif /* TMAIL */
    }

#ifndef TMAIL
    if (!dflag)
	(void) fclose(nfp);
#endif /* TMAIL */

#ifdef DEBUG
    if (verbose >= V_CHATTY)
	(void) msg0("News scanning complete.");
#endif /* DEBUG */

    if (session.news)
    {
	(void) signal(SIGHUP, SIGCAST(catchterm));
	(void) signal(SIGTERM, SIGCAST(catchterm));

	/* call the mailer and wait on its completion */
#ifdef TMAIL
	(void) snprintf(bfr, LBUFLEN, "%s -f %s -T %s",mymailer,readbox,mktemp(msglist));
#else
	(void) snprintf(bfr, LBUFLEN, "%s -f %s", mymailer, readbox);
#endif /* TMAIL */
	(void) system(bfr);
	(void) fflush(stdout);	/* V7 system() doesn't flush stdout */

#ifdef DEBUG
    if (verbose >= V_CHATTY)
	(void) msg0("Returned from mailer.");
#endif /* DEBUG */

#ifndef TMAIL
	/*
	 * This is where it gets tricky. The dflag case is easy, we process
	 * the altered mailfile to extract the message IDs. For the normal
	 * case, we have to set-difference them from the new list to get a
	 * list of messages seen. As a first step, we sort the list of
	 * new-message IDs in place. The pipeline stages work as follows:
	 * 1. Extract from the readbox all Message-ID header lines,
	 *    discarding the Message-ID tag.
	 * 2. Sort the lines.
	 * We now have a sorted list of IDs of articles *not* deleted.
	 * 3. Comm this against the previously sorted list of IDs of all
	 *    new messages, making msglist a list of IDs of messages deleted.
	 */
	if (dflag)
	    (void) snprintf(bfr, LBUFLEN,
		"sed -n <%s -e '/^Message-ID: */s///p' >%s",
		readbox, mktemp(msglist));
	else
	    (void) snprintf(bfr, LBUFLEN,
 "sort %s -o %s; sed -n <%s '/^Message-ID: */s///p' |sort| comm -13 - %s >%s",
		newlist, newlist, readbox, newlist, mktemp(msglist));
	(void) system(bfr);
	(void) fflush(stdout);	/* V7 system() doesn't flush stdout */
#endif /* TMAIL */

	/* the msglist file contains a list of messages to be marked read */
	ofp = xfopen(msglist, "r");
	while (fgets(bfr, BUFLEN, ofp) != (char *)NULL)
	{
	    (void) nstrip(bfr);
#ifdef TMAIL
	    /* the -T option generates them in the form <group>/<number> */
	    if (ptr = strchr(bfr, '/'))
		*ptr = 0;
	    if (ngselect(ngfind(bfr)) == (group_t*)NULL
			|| gotomsg(atoa(++ptr)) == FAIL)
#else
	    /* otherwise we're looking at a list of Message-IDs */
	    if (msggoto(bfr) == FAIL)
#endif /* TMAIL */
		continue;
	    (void) msgmark(READ, GLOBAL);
	}
	(void) unlink(msglist);
    }

    /*
     * If the mail file was changed, write out changed .newsrc data.
     * This means if you leave /bin/mail with an x your .newsrc will
     * not be altered, but a q (normal exit) will update it.
     */
    nochange = modtime(readbox) < starttime;
    cleanup();
    /*NOTREACHED*/
}

catch_t xxit(status)
int status;
{
    (void) unlink(readbox);
    (void) unlink(msglist);
#ifndef TMAIL
    (void) unlink(newlist);
#endif /* TMAIL */
    exit(status);
}

/*ARGSUSED0*/
void vupdate() {/*no-op*/}

#ifdef lint	/* these just shut up lint for when we link with libread.a */
int vstat;
int vgetc() {return('\0');}
char *vgetline(prompt) char *prompt; {return(prompt);}
/*ARGSUSED0*/
void vcommand(args) cmdarg_t *args; {/*no-op*/}
void vuline(to, from) char *to, *from; {(void)strcpy(to, from);}
#endif /* lint */

/* mailnews.c ends here */
