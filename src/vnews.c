/****************************************************************************

NAME
   vnews.c - visual news interface command interpreter.

SYNOPSIS
   main()		-- vnews command interpreter main

   void vcommand()	-- get and execute a single command

DESCRIPTION
   This module implements the command interpreter for vnews. It may be
linked with the standard screen-oriented I/O manager visual.c to produce
vnews or with the paged.c line-oriented interface manager to produce
a paged-output version.

   This module defines commands a, l, g, m, \n, y, <SP>, ^V, ^F, ^H, META-V,
^B, ^D, ^U, ^N, ^P, ^E, ^Y, ^L, K, h, D, H, @, ! and ?.
   If KEYMAP is enabled, the following keycaps are mapped into commands:

KEY_LEFT, KEY_PREVIOUS	n (next article)
KEY_RIGHT, KEY_NEXT	- (previous article)
KEY_SF			^N (line down)
KEY_SR			^P (line up)
KEY_NPAGE		^F (page forward)
KEY_PPAGE		^B (page backward)
KEY_REFRESH		^L (refresh screen)

ENVIRONMENT VARIABLES
   The FIRSTLINE variable controls the format of part of the prompt line.

SEE ALSO
   readinit.c	-- the newsreader initialization code
   browse.c	-- code for browsing articles by page
   gcmd.c	-- generic command interpreter
   rfuncs.c	-- common functions used by interactive news readers

BUGS
   The CTRL() & ALT() macros may do weird things in a non-ASCII (i.e. EBCDIC)
environment. But if you're using EBCDIC you're probably stuck with 3270-style
block I/O and hence thoroughly f***ed anyhow.

AUTHOR
   Eric S. Raymond
starting from an original design and implementation by Kenneth Almquist
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

*****************************************************************************/
/*LINTLIBRARY*/
#include "news.h"
#include "vio.h"
#include "header.h"
#include "procopts.h"
#include "libread.h"
#include "active.h"	/* for place_t, needed by nextmsg.h */
#include "newsrc.h"
#include "dballoc.h"	/* session.h needs the dbdef_t type */
#include "session.h"	/* for session.thread */
#include "gcmd.h"	/* for gcmd() and action values */
#ifdef RECMDS
#include "regexp.h"
#endif /* RECMDS */

#include "insrc.h"	/* for the member declarations of insrc_t */
#include "browse.h"	/* for action and other paging things */
#include "rfuncs.h"	/* for procdlines() */

#undef KEYPAD	/* can't implement this until vtermio.c uses getch() */
#ifdef KEYPAD
#include <curses.h>
#endif /* KEYPAD */

/* read program modes (DNC is defined in procopts.h) */
#define NORMAL	    0x01    /* the vnews program has no modes */
#define SCHECK	    0x02    /* only the -s option is good */

extern int LINES, COLS;	    /* must pick this up from the I/O manager */

char *Progname, *usage;	/* for xerror/procopts (set at start of main below) */

/* command line option flags */
private char	hdropts[SBUFLEN];

option_t options[] =
{
/*        (grouplist, titlebuf, and datebuf are in readinit.c)
optlet  filchar	flag        from      to       type     buf	*/
'n', NGDELIM,	NONE,	    NORMAL,   DNC,     STRING,  grouplist,
'a',	 ' ',	NONE,	    NORMAL,   DNC,     STRING,  datebuf,
'c',	 ' ',	&ihflag,    NORMAL,   DNC,     OPTION,  (char *)NULL,
'p',	 ' ',	NONE,	    NORMAL,   DNC,     STRING,  hdropts,
#ifdef FEEDBACK
'q',	'\0',	NONE,	    NORMAL,   DNC,     STRING,  session.quiet,
#endif /* FEEDBACK */
'r',	'\0',	&session.reverse,   NORMAL,   DNC,     OPTION,  (char *)NULL,
't',	'\0',	NONE,	    NORMAL,   DNC,     STRING,  titlebuf,
'u',	 ' ',	&asave,	    NORMAL,   DNC,     OPTION,  (char *)NULL,
'w',	 ' ',	&dclock.clockoff,   NORMAL,   DNC,     OPTION,  (char *)NULL,
'x',	'\0',	&session.reread,    NORMAL,   DNC,     OPTION,  (char *)NULL,
'f',	'\0',	&nofoll,    NORMAL,   DNC,     OPTION,  (char *)NULL,
'F',	 ' ',	NONE,	    NORMAL,   DNC,     STRING,  inclmark,
#ifdef BYTHREADS
'S',	'\0',	&session.thread,    NORMAL,   DNC,     OPTION,	(char *)NULL,
#endif /* BYTHREADS */
#ifdef DEBUG
'D',	'\0',	&debug,     NORMAL,   DNC,     NUMBER,  (char *)NULL,
'T',	'\0',	NONE,       NORMAL,   DNC,     STRING,	usetty,
#endif /* DEBUG */
'\0',	'\0',	0,	    0,        0,       0,       (char *)NULL,
};

private insrc_t	*exit_to = ARTICLE;	/* mode to enter on article exit */
#ifdef RNESCAPES
private char *firstline = "%(%m=G?%C \\(total of %# waiting\\):%C %a \\(%U unseen\\))";
#endif /* RNESCAPES */

void vcommand(args)
/* process one queued command */
cmdarg_t	*args;
{
    static insrc_t	*oldmode = (insrc_t *)NULL;
    char		*sp;
#ifdef DIGPAGE
    int			nextpart;
#endif /* DIGPAGE */

#ifdef KEYPAD
    /*
     * All mappings of KEY_* keycaps to normal command keys are done
     * here. Note that they assume standard gcmd.c bindings for these
     * keystrokes.
     */
    switch(CHR(args->c_char))
    {
    case KEY_LEFT:
    case KEY_PREVIOUS:
        args->c_char = '-';
        break;
    case KEY_RIGHT:
    case KEY_NEXT:
        args->c_char = 'n';
        break;
    case KEY_SF:	args->c_char = CTRL('N'); break;
    case KEY_SR:	args->c_char = CTRL('P'); break;
    case KEY_NPAGE:	args->c_char = CTRL('v'); break;
    case KEY_PPAGE:	args->c_char = ALT('v'); break;
    case KEY_REFRESH:	args->c_char = CTRL('L'); break;
    case KEY_DOWN:	args->c_char = 'N'; break;
    case KEY_UP:	args->c_char = 'P'; break;
    case KEY_CANCEL:	args->c_char = 'c'; break;
    case KEY_COPY:	args->c_char = 'w'; break;
    case KEY_EXIT:	args->c_char = 'q'; break;
    case KEY_FIND:	args->c_char = 'f'; break;
    case KEY_HELP:	args->c_char = '?'; break;
    case KEY_MARK:	args->c_char = 'j'; break;
    case KEY_MESSAGE:	args->c_char = 'r'; break;
    case KEY_MOVE:	args->c_char = 'A'; break;
    case KEY_OPTIONS:	args->c_char = '&'; break;
    case KEY_REDO:	args->c_char = 'e'; break;
    case KEY_REFERENCE:	args->c_char = 'p'; break;
    case KEY_RESTART:	args->c_char = 'W'; break;
    case KEY_SAVE:	args->c_char = 's'; break;
    case KEY_SUSPEND:	args->c_char = '!'; break;
    case KEY_SEXIT:	args->c_char = 'x'; break;
    case KEY_SFIND:	args->c_char = '\\'; break;
    case KEY_SMESSAGE:	args->c_char = 'R'; break;
    case KEY_SREDO:	args->c_char = 'E'; break;
    }
#endif /* KEYPAD */

    switch (CHR(args->c_char))
    {
    case 'a':
	if (!tfreading(ARTICLE))
	    oldmode = readfrom(exit_to = ARTICLE);
	else if (oldmode)
	    (void) readfrom(exit_to = oldmode);
	break;

    case 'l':
#ifdef SUBJFILE
	if (!tfreading(SUBJECTS))
	    oldmode = readfrom(exit_to = SUBJECTS);
	else if (oldmode)
	    (void) readfrom(exit_to = oldmode);
#else
	msg0("The subject list feature has been disabled");
#endif /* SUBJFILE */
	break;

    case 'g':
	if (!tfreading(GROUPS))
	    oldmode = readfrom(exit_to = GROUPS);
	else if (oldmode)
	    (void) readfrom(exit_to = oldmode);
	break;

#ifdef DIGPAGE
    case 'm':		/* skip to next digest section */
	/* an 'm' in subject-list or group mode should fall through */
	if (tfreading(ARTICLE))
	{
	    src->i_dtype = DT_DIGEST;
	    if (nextpart = tfskip())
	    {
		gotoline(nextpart);
		break;
	    }
	    else
		session.action = M_NEXT;
	}
	/* if we didn't find a digest end, fall through to next article */
#endif /* DIGPAGE */

    case '\n':		/* show more of current article, or advance to next */
    case ' ':
    case 'y':		/* for readnews compatibility */
    case '\0':
    case CTRL('V'):	/* for emacs compatibility */
    case CTRL('F'):	/* for vi compatibility */
	if (pending(NOMORE))
	    demand(NOMORE | EXIT);
	else if (tfreading(GROUPS))
#ifdef SUBJFILE
	{
	    (void) readfrom(exit_to = SUBJECTS);
	    break;
	}
	else if (tfreading(SUBJECTS))
#endif /* SUBJFILE */
	{
	    exit_to = readfrom(ARTICLE);
	    break;
	}
	else if (!tfreading(ARTICLE))	/* catch HEADER and HELP cases */
	    (void) readfrom(ARTICLE);
	else if (session.action == M_NEXT)
	{
	    args->c_char = 'n'; args->c_count = 1L;
	    gcmd(args);
	}
	else
	    nextpage(1);
	break;

    case '\b':    /* Back up count pages */
	if (topline() == 0)
	{
	    args->c_count = (long)(msgnum() - 1);
	    args->c_char = 'A';
	    gcmd(args);
	    break;
	}
	/* FALL THROUGH -- if not at article start, treat as page back */

    case ALT('v'):	/* EMACS page-back (ESC-v) */
    case CTRL('B'):	/* vi page-back */
#ifndef lint	/* prevent long-int assignment flames on some machines */
	backpage((int)args->c_count);
#endif /* lint */
	break;

    case CTRL('D'):    /* forward a half page - as in vi */
	changeline((src->i_height - 2) / 2);
	break;

    case CTRL('U'):    /* back half a page -- as in vi */
	changeline(-(src->i_height - 2) / 2);
	break;

    case CTRL('N'): case CTRL('E'):	/* forward <count> lines */
#ifndef lint	/* prevent long-int assignment flames on some machines */
	changeline((int)args->c_count);
#endif /* lint */
	break;

    case CTRL('P'): case CTRL('Y'):	/* backwards <count> lines */
#ifndef lint	/* prevent long-int assignment flames on some machines */
	changeline((int)-args->c_count);
#endif /* lint */
	break;

    case CTRL('L'):			/* force screen refresh */
	demand(HOLD);
	vclear();
	break;

    case 'K':
	if (tfreading(SUBJECTS))	/* kill the current page of articles */
	{
	    group_t	*oldgrp = ngactive();

	    do {
		(void) msgmark(READ, LOCAL);
	    } while
		(msgnum() <= most
		 && nextmsg(session.reread, session.reverse) != FAIL
		 && ngactive() == oldgrp);
	    session.action = M_SEEK;
	}
	else			/* do the usual K thing on the whole group */
	    gcmd(args);
	break;

    case 'h':    /* display header */
	gotohdr();
	break;

    case 'D':    /* Decrypt joke.  Always does rot 13 */
	fliprot();
	break;

    case 'H':    /* Print the full header */
	(void) readfrom(HEADER);
	break;

    case '@':	/* change browse chunk size -- intended for paged.c */
	demand(HOLD);
	if ((sp = vgetline("new screen size is? ")) != (char *)NULL)
        {
	    (void) sscanf(sp, "%d,%d", &LINES, &COLS);
	    (void) bsetsize(LINES, COLS);
	}
	break;

    case '!':		/* count 0 will force a confirmation prompt */
	args->c_count = 0L;
    default:		/* hand us off to the generic code */
	gcmd(args);
	if (!pending(CMDERR))
	    break;
	msg0("? for commands");
	/* fall through on illegal command */

    case '?':    /* error */
	(void) sprintf(bfr, "%s/vnews.help", site.libdir);
	(void) readfrom(HELP, bfr);
	if (args->c_char != '?')
	    if (isprint(args->c_char))
		msg1("Unknown command: %c", args->c_char);
	    else
		msg1("Unknown command: ASCII 0x%02x", args->c_char);
	break;
    }
}

private void enter()
/* called on entry to each new article */
{
    if (session.ngrp == 1)
	(void) readfrom(exit_to = GROUPS);
    else if (!tfreading(exit_to))
	(void) readfrom(exit_to);
}

private void statline(left, right)
/* generate an appropriate status line for the current screen */
char	*left, *right;
{
    if (pending(HDRONLY))
	(void) strcpy(left, "more? ");
    else if (session.action == M_NEXT || tfartlen() == 0)
	(void) strcpy(left, "next? ");
    else if (pending(NOMORE | HOLD))
    {
	(void) strcpy(left, "done? ");
	right[0] = '\0';
	return;
    }
    else
	(void) sprintf(left, "more(%d%%)? ", tfpercent(src->i_botline));

#ifdef RNESCAPES
    strexpand(firstline, right);
#else
    if (tfreading(ARTICLE) || tfreading(SUBJECTS))
	(void) sprintf(right,
		"%s %ld (%ld more)",
		ngname(), (long)msgnum(), (long)ngunread());
    else
	(void) sprintf(right,
		   "%s (total of %ld unseen)",
		   ngname(), (long)session.waiting);
#endif /* RNESCAPES */

    if (tfreading(ARTICLE)) /* prompt reader if the article is rot13 */
    {
	char *cp = strrchr(header.h_subject, '(');

	if (cp && !strncmp(cp, "(rot 13)", 8))
	    msg0("Type D to unrotate this joke.");
    }
}

main(argc, argv)
/* main sequence of a generic screen-oriented news reader */
int argc;
char *argv[];
{
#ifdef RNESCAPES
    char *p;
#endif /* RNESCAPES */
    int unread, newgroups = 0;
    extern void tick();

    /* usage message must vary depending on who called this */
    Sprint1(usage,
	"Usage: %s [-a date] [-n newsgroups] [-t titles] [-cprxwhfusSLK]",
	Progname = argv[0]);

    /* initialize and process options */
    if ((unread = readinit(argc, argv, procdlines)) <= 0)
    {
	(void) fprintf(stderr, "No news\n");
	exit(0);
    }
    (void) free(usage);

    /* set article header display verbosity */
    if (!isdigit(hdropts[0]))
	pflag = 1;
    else
	pflag = atoi(hdropts);
    ihflag = (strchr(hdropts, 'h') == (char *)NULL);

    if (sigcaught)
	xxit(1);

#ifdef RNESCAPES
    setexphook(vexpand);
#endif /* RNESCAPES */

    /* if we get this far, we're interactive -- go to screen mode */
    vinit(textline, statline, tick);	/* this sets LINES and COLS */

    /* give user some idea how much awaits */
    if (!flushall)
	msg1("There appear to be %d articles waiting", unread);

#ifdef RNESCAPES
    if (p = getenv("FIRSTLINE"))
	firstline = p;
#endif /* RNESCAPES */

    browseinit(GROUPS);

    /* check for new groups that the user's never seen */
    if (!flushall)
    {
	group_t	*gsave = ngactive();

	ngrewind(TRUE);
	while (ngnext())
	    if (!rcflag(RC_NOTED))
	    {
		subscribe();
		newgroups++;
	    }
	ngselect(gsave);
    }
    if (newgroups)
	msg1("There are %d new newsgroups.", newgroups);

    /* now simply do a browse using the vnews command interpreter */
    browse(enter);

#ifdef DEBUG
    browsewrap();	/* release files used for browsing */
#endif /* DEBUG */

    /* clean up tty settings in case the group check is invoked */
    vttymode();

    nochange = !pending(NOMORE);

    if (sigcaught)
	(void) fprintf(stderr, "Aborted by signal %d\n", sigcaught);

    cleanup();		/* write out .newsrc and all that jazz, then exit */
    /*NOTREACHED*/
}

private void tick(icount)
/* function to call at tick intervals */
int	icount;	/* NZ if on keyboard wait, 0 on timer tick interrupt */
{
    if (icount == 0)
	clockdaemon(wrnewsrc, asave * SAVESECS);
    else
	subjadd();	/* compile more index information */
}

static bool vexpand(cgetf, cungetf, buf)
/* expand the given reader-specific %-construct into the given buffer */
int (*cgetf)();		/* key character of escape */
int (*cungetf)();	/* how to push back characters */
char *buf;		/* buffer to expand escapes into */
{
#ifdef RNESCAPES
    int	c;

    if ((c = (*cgetf)()) == 'm')
    {
	if (tfreading(ARTICLE) || tfreading(HEADER))
	    buf[0] = 'A';
	else if (tfreading(SUBJECTS))
	    buf[0] = 'S';
	else if (tfreading(GROUPS))
	    buf[0] = 'G';
	else if (tfreading(HELP))
	    buf[0] = 'H';
	else
	    buf[0] = 'I';
	buf[1] = '\0';
	return(TRUE);
    }
    else
	(*cungetf)(c);
#endif /* RNESCAPES */
    
    return(vkexpand(cgetf, cungetf, buf) || rexpand(cgetf, cungetf, buf));
}

/* vnews.c ends here */
