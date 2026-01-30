/****************************************************************************

NAME
   vrn.c - visual news interface command interpreter.

SYNOPSIS
   main()		-- vrn command interpreter main

   void vcommand()	    -- get and execute a single command

DESCRIPTION
   This module implements the command interpreter for vrn. It may be
linked with the standard screen-oriented I/O manager visual.c to produce
vrn or with the paged.c line-oriented interface manager to produce
a paged-output version.

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
#include "nextmsg.h"
#include "session.h"
#include "gcmd.h"	/* for gcmd() and action values */
#ifdef RECMDS
#include "regexp.h"
#endif /* RECMDS */

#include "insrc.h"	/* for the member declarations of insrc_t */
#include "browse.h"	/* for action and other paging things */
#include "rfuncs.h"	/* for procdlines() */

/* read program modes (DNC is defined in procopts.h) */
#define NORMAL	    0x01    /* the vrn program has no modes */
#define SCHECK	    0x02    /* only the -s option is good */

extern int LINES, COLS;	    /* must pick this up from the I/O manager */

char *Progname, *usage;	/* for xerror/procopts (set at start of main below) */

/* command line option flags */
private char    grestrict[LBUFLEN];  /* not used if RECMDS is disabled */
#ifdef RECMDS
private regexp  *oldre = (regexp *)NULL, *gre = (regexp *)NULL;
#endif /* RECMDS */

option_t options[] =
{
/*        (grouplist, titlebuf, and datebuf are in readinit.c)
optlet  filchar	flag        from      to       type     buf	*/
'n',	'\0',	0,	    DNC,      DNC,     STRING,  grouplist,
'o',	'\0',	0,	    DNC,      DNC,     STRING,  grestrict,
'C',	 ' ',	&asave,	    DNC,      DNC,     OPTION,  (char *)NULL,
#ifdef BYTHREADS
'S',	'\0',	&session.thread,    DNC,      DNC,     OPTION,	(char *)NULL,
#endif /* BYTHREADS */
'\0',	'\0',	0,	    0,        0,       0,       (char *)NULL,
};
/* 
 * -C is treated like vnews -u.
 * We don't attempt to support -cdDEFghHmMNrsv/ (but probably should).
 * Also note that -et are effectively always on and -ilLT are meaningless in
 * the visual.c environment.
 *
 * The -n option is treated as it is in vnews so that standard .newsrc files
 * won't cause vrn to barf with a 'Bad format' error.
 */

#ifdef RNESCAPES
private char *firstline =
    "%(%m=G?******** %u unread articles in %C\\):%C %a \\(%u unseen\\))";
#endif /* RNESCAPES */

private void grpcommand(args)
/* process one queued group-selection command */
cmdarg_t	*args;
/*
 * Emulate the rn top level (group selection level). All rn group level
 * commands are supported except L, m, &&, and ^K.
 */
{
    char	*mptr, *cp;
    int		i;

    switch(args->c_char)
    {
    case '.':		/* go to article select level with commands */
	if (mptr = vgetline("Initial commands? "))
	    for (cp = mptr; *cp; cp++)
		vungetch(*cp);
	/* FALL THROUGH */
    case 'y':
    case ' ':
    case '\0':
	(void) readfrom(ARTICLE);
	break;

    case '=':
	(void) readfrom(SUBJECTS);
	break;

    case 'n':		/* go to next group with unread news */
	(void) change_group((char *)NULL, 1, session.reread, session.reverse);
	break;

    case 'N':		/* go to next newsgroup */
	(void) change_group((char *)NULL, 1, TRUE, session.reverse);
	break;

    case 'p':		/* go to last group with unread news */
	(void) change_group((char *)NULL, 1, session.reread, !session.reverse);
	break;

    case 'P':		/* go to last newsgroup */
	(void) change_group((char *)NULL, 1, TRUE, !session.reverse);
	break;

    case '-':		/* go to last newsgroup seen */
	gcmd(args);
	break;

    case '^':	/* go to first newsgroup with unread news */
	initmsg(session.reread = FALSE, session.reverse);
	break;

    case '$':	/* go to last group */
	session.reverse = TRUE;
    case '1':	/* go to first group */
	initmsg(session.reread = TRUE, session.reverse);
	break;

    case 'g':	/* go to given group */
	if (mptr = vgetline("group? "))
	{
	    if (gotogrp(mptr) != FAIL)
	    {
		if (!rcsubsc(ngactive()))
		{
		    (void) sprintf(bfr,"Subscribe to %s? ", ngname());
		    mptr = vgetline(bfr);
		    if (mptr[0] == 'y')
			subscribe();
		}
	    }
	    else
		msg1("No such group as %s", mptr);
	}
	break;

#ifdef RECMDS
    case '?':	/* search backwards for group */
	session.reverse = TRUE;
    case '/':	/* search forward for group */
	(void) sprintf(bfr, "Pattern? %c", args->c_char);
	if (mptr = vgetline(bfr))
	{
	    char	*opts;
	    regexp	*pattern;
	    place_t	loc;

	    if (opts = strrchr(mptr, args->c_char))
	    {
		*opts = '\0';
		session.reread = (strchr(opts, 'r') != (char *)NULL);
	    }
	    if (mptr[0])
		pattern = oldre;		    
	    else if ((pattern = regcomp(mptr)) == (regexp *)NULL)
		break;
	    else
	    {
#ifndef lint
		if (oldre)
		    (void) free((char *) oldre);
#endif /* lint */
		oldre = pattern;
	    }
	    (void) tellmsg(&loc);
	    while (nextgroup(session.reread, session.reverse) != FAIL)
		if (regexec(pattern, ngname()))
		    goto breakout;
	    (void) seekmsg(&loc);	/* didn't find a match */
	breakout:;
	}
	break;

    case 'l':	/* list unsubscribed groups matching given pattern */
	if (mptr = vgetline("Pattern? "))
	{
	    regexp	*pattern = regcomp(mptr);

	    vttymode();
	    if (mptr[0])
		pattern = oldre;		    
	    else if ((pattern = regcomp(mptr)) == (regexp *)NULL)
		break;
	    else
	    {
#ifndef lint
		if (oldre)
		    (void) free((char *) oldre);
#endif /* lint */
		oldre = pattern;
	    }
	    ngrewind(TRUE);
	    while (ngnext())
		if (!rcsubsc(ngactive()) && regexec(pattern, ngname()))
		    (void) puts(ngname());
	    vcrtmode(TRUE);
	}
	break;
#endif /* RECMDS */

    case 'u':	/* unsubscribe from the current group */
	if (unsubscribe())
	    msg1("%s is now unsubscribed", ngname());
	else
	    msg1("Sorry, you can't unsubscribe any of %s", ADMSUB);
	break;

    case 'c':	/* mark all messages in this group read */
	for (i = ngmin(); i <= ngmax(); i++)
	    (void) setbit(i, ngactive());
	break;

#ifdef RECMDS
    case 'o':	/* change the current group restriction */
	grestrict[0] = '\0';
	/* FALL THROUGH */
    case 'a':
	if (mptr = vgetline("Restriction pattern? "))
	{
	    place_t	loc;

	    for (cp = mptr; *cp; cp++)
		if (*cp == ' ')
		    *cp = '|';
	    (void) strcat(grestrict, mptr);
	    gre = regcomp(grestrict);
	    (void) tellmsg(&loc);
	    ngrewind(TRUE);
	    while (ngnext())
		if (!rcflag(RC_NOTED))
		{
		    (void) sprintf(bfr, "Subscribe to %s? ", ngname());
		    if (*vgetline(bfr))
			subscribe();
		    else
			break;	/* user hit an interrupt */
		}
	    (void) seekmsg(&loc);
	}
	break;
#endif /* RECMDS */

    case '&':	/* view or set command-line switches */
    case '!':	/* execute a shell command */
    case 'x':	/* exit without writing .newsrc */
    case 'q':	/* exit, writing .newsrc */
    case 'v':	/* show version */
	gcmd(args);
	break;

    default:
	(void) sprintf(bfr, "%s/vrn.group.help", site.admdir);
	(void) readfrom(HELP, bfr);
	if (args->c_char != 'h')
	    if (isprint(args->c_char))
		msg1("Unknown command: %c", args->c_char);
	    else
		msg1("Unknown command: ASCII 0x%02x", args->c_char);
	break;
    }
}

private void artcommand(args)
/* process one queued article-selection command */
cmdarg_t	*args;
{
    switch(args->c_char)
    {
    case 'h':	/* get help */
	(void) sprintf(bfr, "%s/vrn.art.help", site.admdir);
	(void) readfrom(HELP, bfr);
	break;

    case 'n':	/* next article...just come back around browse loop */
    case ' ':
	session.reread = FALSE;
	break;

    case 'N':	/* next article, ignoring read status */
	session.reread = TRUE;
	break;

    case CTRL('N'):	/* ^N -- do subject-following, go to next article */
	session.reread = FALSE;
	session.thread = TRUE;
	break;

    case 'p':	/* go to previous unread article */
	session.reread = FALSE;
	session.reverse = TRUE;
	break;

    case 'P':	/* go to previous article, ignoring status */
	session.reverse = session.reread = TRUE;
	break;

    case '-':	/* go to previously-displayed article */
	(void) msglast();
	session.action = M_HOLD;
	break;

    case CTRL('P'):	/* ^P -- do subject-following, go to last article */
	session.reread = FALSE;
	session.thread = TRUE;
	break;

    case 'v':	/* restart article, displaying entire header */
	(void) readfrom(HEADER);
	break;

    case CTRL('R'):	/* ^R -- restart article */
	session.action = M_SEEK;
	break;

    case CTRL('L'):	/* ^L -- refresh screen */
	(void) readfrom(ARTICLE, (char *)NULL);
	break;

    case 'X':	/* X -- restart and ROT13 the message */
	gotoline(0);
	/* FALL THROUGH */

    case CTRL('X'):	/* ^X -- ROT13 the message */
	fliprot();
	break;

    case 'q':	/* quit -- pop back to group select level */
	demand(EXIT);
	break;

    default:
	if (strchr("j/rRfFsw|&!#", args->c_char))
	    gcmd(args);
	else if (isprint(args->c_char))
	    msg1("Unknown command: %c", args->c_char);
	else
	    msg1("Unknown command: ASCII 0x%02x", args->c_char);
    }
}

void pagecommand(args)
/* process one queued pager-level command */
cmdarg_t	*args;
{
    switch (args->c_char)
    {
    default:		/* hand us off to the generic code */
	gcmd(args);
	if (!pending(CMDERR))
	    break;
	msg0("? for commands");
	/* fall through on illegal command */

    case 'h':	/* get help */
	(void) sprintf(bfr, "%s/vrn.pager.help", site.admdir);
	(void) readfrom(HELP, bfr);
	if (args->c_char != 'h')
	    if (isprint(args->c_char))
		msg1("Unknown command: %c", args->c_char);
	    else
		msg1("Unknown command: ASCII 0x%02x", args->c_char);
	break;
    }
}

void vcommand(args)
/* process one queued command */
cmdarg_t	*args;
{
    session.reread = session.reverse = FALSE;

    if (tfreading(GROUPS))
	grpcommand(args);
    else if (session.action == M_NEXT)
	artcommand(args);
    else
	pagecommand(args);
}

private void enter()
/* called on entry to each new article */
{
    /* no-op in this reader */
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
		"%s %ld (%ld unseen)",
		ngname(), (long)msgnum(), (long)ngunread());
    else
	(void) sprintf(right,
		   "%s (total of %ld unseen)", ngname(),
		   (long)session.waiting);
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
    int unread;
    char *cp;
    extern void tick();

    /* usage message must vary depending on who called this */
    Sprint1(usage, "Usage: vrn [-CS] newsgroups", Progname = argv[0]);

    /* initialize and process options */
    if ((unread = readinit(argc, argv, procdlines)) <= 0)
    {
	(void) fprintf(stderr, "No news\n");
	exit(0);
    }
    (void) free(usage);

#ifdef RNESCAPES
    setexphook(vexpand);
#endif /* RNESCAPES */

    /* set up initial group restriction */
    if ((gre = regcomp(grestrict)) != (regexp *)NULL)
    {
	ngrewind(TRUE);
	while (ngnext())
	    if (!regexec(gre, ngname()))
		rcfset(RC_UNSUB);
    }

    if (sigcaught)
	xxit(1);

    /* if we get this far, we're interactive -- go to screen mode */
    vinit(textline, statline, tick);		/* this sets LINES and COLS */

    /* give user some idea how much awaits */
    if (!flushall)
	msg1("There appear to be %d articles waiting", unread);

#ifdef RNESCAPES
    if (p = getenv("FIRSTLINE"))
	firstline = p;
#endif /* RNESCAPES */

    /* process any initial newsgroup list */
    if (grestrict[0])
    {
	for (cp = grestrict; *cp; cp++)
	    if (*cp == ' ')
		*cp = '|';
	gre = regcomp(grestrict);
    }
    ihflag = TRUE;

    /* now simply do a browse using the vrn command interpreter */
    browseinit(GROUPS);
    browse(enter);

    /* clean up tty settings in case the group check is invoked */
    vttymode();

    nochange = !pending(NOMORE);

    if (sigcaught)
	(void) fprintf(stderr, "Aborted by signal %d\n", sigcaught);

    cleanup();	    /* write out .newsrc and all that jazz, then exit */
    /*NOTREACHED*/
}

private void tick()
/* function to call at tick intervals */
{
    clockdaemon(wrnewsrc, asave * SAVESECS);
}

static bool vexpand(cgetf, cungetf, buf)
/* expand the given reader-specific %-construct into the given buffer */
int (*cgetf)();		/* how to get key character of escape */
int (*cungetf)();	/* how to push back characters */
char *buf;		/* buffer to expand esacapes into */
{
#ifdef RNESCAPES
    int	c;

    if ((c = (*cgetf)()) == 'm')
    {
	if (tfreading(ARTICLE) || tfreading(HEADER))
	    buf[0] = 'p';
	else if (tfreading(SUBJECTS))
	    buf[0] = 'a';
	else if (tfreading(GROUPS))
	    buf[0] = 'n';
	else if (tfreading(HELP))
	    buf[0] = 'p';
	else
	    buf[0] = 'i';
	buf[1] = '\0';
	return(TRUE);
    }
    else
	(*cungetf)(c);
#endif /* RNESCAPES */
    return(rexpand(cgetf, cungetf, buf));
}

/* vrn.c ends here */
