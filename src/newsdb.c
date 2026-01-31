/****************************************************************************

NAME
   newsdb -- news database debugging and testing tool

SYNOPSIS
   newsdb

DESCRIPTION
   A simple editor for files in the format of USENET's active, history, and
.newsrc files. This can be used to verify the functioning of the service
library routines. 
   In 'i' mode it can also exercise the get-next-message primitives used by
readers. Note that unlike a real reader, this mode ignores options and
group initial commands.
   DEL will abort a long output and return you to the debugger command prompt.
   Under most circumstances, you should never need to use the 'R' (Reread)
command in any of the sub-interpreters; it is performed automatically before
the first command executed in each.
   Type ? to the first * prompt for help, and/or read the code.

FILES
   /tmp/subsc?????	used by feed test code

AUTHOR
   Eric S. Raymond
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

**************************************************************************/
/*LINTLIBRARY*/
#include "news.h"
#include "vio.h"
#include "header.h"
#include "active.h"
#include "history.h"
#include "newsrc.h"
#include "nextmsg.h"
#include "dballoc.h"
#include "session.h"
#include "feeds.h"
#include "ngprep.h"
#include "gcmd.h"
#include "procopts.h"
#include "version.h"
#include "libfilt.h"
#ifdef FASCIST
#include "fascist.h"
#endif /* FASCIST */

char	*Progname = "newsdb";

char *usage = "Usage: newsdb [-vS]";

option_t options[] =
{
/*
optlet  filchar	flag        from      to       type     buf	*/
'v',	'\0',	&verbose,   DNC,      DNC,     NUMBER,  (char *)NULL,
'S',	'\0',	&session.thread,    DNC,      DNC,     OPTION,	(char *)NULL,
'\0',	'\0',	0,	    0,        0,       0,       (char *)NULL,
};

private int eargc;
private char **eargv;
private jmp_buf	sigjmpbuf;

typedef struct
{
    int val;
    char *name;
}
vtab_t;

typedef struct
{
    char key;
    int (*interp)();
    char *prompt;
    char *longname;
    int firsttime;
}
interp_t;

/* forward declare for methods table and current method pointer */
extern interp_t	methods[], *ip;

/* *** helper functions begin here *** */

#define I_NONE		0x00	/* no initial reads have been performed */
#define I_RDACTIVE	0x01	/* rdactive() has been performed */
#define I_HSTREAD	0x02	/* hstread() has been performed */
#define I_MSGINIT	0x03	/* msginit() has been performed (read side) */
#define I_RDNEWSRC	0x04	/* rdnewsrc() has been performed */
#define I_READOPTS	0x09	/* readopts() has been performed */
#define I_RDFLAGS	0x10	/* rdflags() has been performs */
static int current = I_NONE;	/* tell which initializations have been done */

private void debuginit(mask)
/* read appropriate database files to match the required initialization mask */
int	mask;
{
    if ((mask & I_READOPTS) == I_READOPTS)
    {
	if ((current & I_READOPTS) == I_READOPTS)
	    (void)  printf("Your .newsrc file and options are already read\n");
	else
	{
	    (void) printf("Reading your .newsrc file and options\n");
	    (void) fflush(stdout);				    
	    readopts(NOOPTS);
	    (void) printf("%ld articles waiting\n", (long)rccount(FALSE));
	    current |= I_READOPTS;
	}
    }
    else if ((mask & I_RDNEWSRC) == I_RDNEWSRC)
    {
	if ((current & I_RDNEWSRC) == I_RDNEWSRC)
	    (void) printf("Your .newsrc file is already read\n");
#if ng_defdist == rc_seen
	else if (current & I_RDFLAGS)
	    (void) printf("This would step on the ng_flags data!\n");
#endif 
	else
	{
	    (void) printf("Reading your .newsrc file\n");
	    rdnewsrc(username);
	    current |= (I_RDNEWSRC | I_RDACTIVE);
	}
    }
    else if ((mask & I_MSGINIT) == I_MSGINIT)
    {
	if ((current & I_MSGINIT) == I_MSGINIT)
	    (void)  printf("Default active and history files already read\n");
	else
	{
	    (void) printf("Reading default active and history files\n");
	    msginit(NULLPRED);
	    current |= I_MSGINIT;
	}
    }
    else
    {
	if (mask & I_RDACTIVE)
	{
	    if (current & I_RDACTIVE)
		(void) printf("Default active file already read\n");
	    else
	    {
		(void) printf("Reading default active file\n");
		rdactive(NULLPRED);
		current |= I_RDACTIVE;
	    }
	}

	if (mask & I_RDFLAGS)
	{
	    if (current & I_RDFLAGS)
		(void) printf("Admin file already read\n");
#if ng_defdist == rc_seen
	    else if (current & I_RDNEWSRC)
		(void) printf("This would step on the rc_seen data!\n");
#endif
	    else
	    {
		(void) printf("Reading admin file\n");
		(void) rdflags(0xffffL);	/* read in all flags */
		current |= I_RDFLAGS;
	    }
	}

	if (mask & I_HSTREAD)
	{
	    if (current & I_HSTREAD)
		(void) printf("Default history file already read\n");
	    else
	    {
		(void) printf("Reading default history file\n");
		hstread(TRUE);
		current |= I_HSTREAD;
	    }
	}
    }
}

private int sigcatch(sig)
int sig;
{
#if !defined(BSD4_2) && !defined(SYSV3)
    /*
     * Older UNIX systems reset caught signals to SIG_DFL.
     * This bad design requires that the trap be set again here.
     * Unfortunately, if the signal recurs before the trap is set,
     * the program will die.
     */
    (void) signal(sig, SIGCAST(sigcatch));
#endif /* !defined(BSD4_2) && !defined(SYSV3) */
    sigcaught = sig;
    (void) fflush(stdout);
    (void) fputs("Interrupted\n", stdout);
    if (ip == methods)
	xxit(0);
    else
	longjmp(sigjmpbuf, 1);
}

private void ngdisplay()
/* display status and flags of a group (active and admin info only) */
{
    ngshow(ngactive(), stdout);
    (void) printf(
		" (flags = %04x = %sm, %sl, %sc, %sv, %sz, %si, %sa, %se)\n",
		ngactive()->ng_flags & (NG_ACTFLAGS | NG_ADMFLAGS),
		ngflag(NG_MODERATED) ? "" : "!",
		ngflag(NG_LOCAL) ? "" : "!",
		ngflag(NG_CHANGED) ? "" : "!",
		ngflag(NG_VOLATILE) ? "" : "!",
		ngflag(NG_COMPRESSED) ? "" : "!",
		ngflag(NG_IGNEXPDATE) ? "" : "!",
		ngflag(NG_ARCHIVEIT) ? "" : "!",
		ngflag(NG_EXPIREIT) ? "" : "!"
		);

    if (current & I_RDFLAGS)
        if (ngactive()->ng_defdist != (char *)NULL)
	    (void) printf("Default distribution = \"%s\"\n",
			  ngactive()->ng_defdist);
	else
	    (void) printf("No default distribution.\n");
}

private void ngdump(ngp)
/* display full data on a group (including .newsrc data) */
group_t	*ngp;
{
    char    *cp;

    /* first, dump the active file data */
    ngshow(ngp, stdout);

    /* next, display the group's .newsrc status flags */
    (void) printf(" (flags=%04x=%sn,%sU,%su,%ss,%sh)",
		  ngp->rc_flags,
		  (ngp->rc_flags & RC_NOTED) ? "" : "!",
		  (ngp->rc_flags & RC_UNSEL) ? "" : "!",
		  (ngp->rc_flags & RC_UNSUB) ? "" : "!",
		  (ngp->rc_flags & RC_VISITED) ? "" : "!",
		  (ngp->rc_flags & RC_HASSUBS) ? "" : "!"
		  /* if you add more flags in newsrc.h, display them here */
		);

    /* next, display miscellaneous other .newsrc-derived fields */
    (void) printf(
#ifdef FEEDBITS
	" (feeds 0x%2x, index %ld, %d unread)%c",
	ngp->ng_feeds,
#else
	" (index %ld, %d unread, 0x%2x)%c",
#endif /* FEEDBITS */
	(long)ngp->rc_lindex, ngp->ng_unread,
	ngp->rc_seen ? ngp->rc_seen[0] : 0,
	(ngp->rc_flags & RC_UNSUB) ? UNSUBSCMK : SUBSCMK
	);

    /* finally, show which messages have been seen */
    wrbits(ngp, bfr);
    (void) fputs(bfr, stdout);
    (void) fputc('\n', stdout);
}

/* debugger interpreter methods start here */

private int testactive(cmdline)
/* exercise the active file functions */
char	*cmdline;
{
    char	strv[100];
    int		nv;
    time_t	now;

    if (cmdline[0] == 'R')
	debuginit(I_RDACTIVE);
    else if (cmdline[0] == 'r')
	debuginit(I_RDFLAGS);
#ifndef FLEXGROUPS
    else if (cmdline[0] == 'C')
    {
	(void) printf("Inverting submit flag...\n");
	if (ngflag(NG_MODERATED))
	    ngfclear(NG_MODERATED);
	else
	    ngfset(NG_MODERATED);
	ngshow(ngactive(), stdout);
	(void) fputc('\n', stdout);
    }
#endif /* FLEXGROUPS */
    else if (sscanf(cmdline, "c %s", strv) == 1)
    {
	(void) printf("Creating group %s\n", strv);
	ngcreate(strv, TRUE);
    }
    else if (cmdline[0] == 'd')
    {
	(void) printf("Deleting group %s\n", ngname());
	ngdelete();
    }
    else if (cmdline[0] == 'W')
    {
	(void) printf("Writing the default active file\n");
	wractive(FALSE);
    }
    else if (strcmp(cmdline, "w") == 0)
    {
	(void) printf("Writing .active data to stdout\n");
	wractfile(stdout, FALSE);
    }
    else if (sscanf(cmdline, "w %s", strv) == 1)
    {
	(void) printf("Writing specified active-format file\n");
	wractfile(xfopen(strv, "w"), FALSE);
    }
    else if (cmdline[0] == '\0')
    {
	(void) ngnext();
	ngdisplay();
    }
    else if (sscanf(cmdline, "g %s", strv) == 1)
    {
	group_t *ngp;

	if ((ngp = ngfind(strv)) == (group_t *)NULL)
	    (void) printf("%s: no such group\n", strv);
	else
	{
	    ngselect(ngp);
	    ngdisplay();
	}
    }
    else if (cmdline[0] == 'g')
    {
	ngdisplay();
    }
    else if (cmdline[0] == 'n')
    {
	if ((nv = ngnewart(ngactive())) == 0)
	    (void) printf("%s: couldn't add article\n", ngname());
	else
	{
	    (void) printf("Article number %d added to %s\n", nv,ngname());
	    (void) ngreread(active.article.m_group, NULLPRED);
	}
	ngshow(active.article.m_group, stdout);
	(void) fputc('\n', stdout);
    }
    else if (cmdline[0] == 'b')
    {
	ngrewind(TRUE);
	ngnext();
	ngshow(active.article.m_group, stdout);
	(void) fputc('\n', stdout);
    }
    else if (sscanf(cmdline, "a %ld", &now))
    {
	(void) fputs(arpadate(&now), stdout);
	(void) fputc('\n', stdout);
    }
    else if (cmdline[0] == '?')
    {
	(void) printf("R        -- read default active file\n");
	(void) printf("r        -- read admin file\n");
#ifndef FLEXGROUPS
	(void) printf("C        -- invert current group's submit flag\n");
#endif /* FLEXGROUPS */
	(void) printf("c name   -- create a new group by the given name\n");
	(void) printf("d        -- delete currently selected group\n");
	(void) printf("W        -- write default active file\n");
	(void) printf("w [file] -- write specified active-format file\n");
	(void) printf("<CR>     -- select and examine next newsgroup\n");
	(void) printf("g name   -- select and examine newsgroup <name>\n");
	(void) printf("g        -- show data on current newsgroup\n");
	(void) printf("n        -- add a new article to current group\n");
	(void) printf("b        -- rewind current group pointer\n");
	(void) printf("a tim    -- display a timestamp as an ARPA date\n");
	(void) printf("Omitted file arg to w defaults to stdout\n");
	(void) printf("The current group is initially '%s'\n\n", ngname());
    }
    else
	return(FAIL);
    return(SUCCEED);
}

private int testhist(cmdline)
/* excercise the history database functions */
char	*cmdline;
{
    char	strv[100], strv2[100];
    nart_t	nmsg;
    place_t	exloc;

    if (cmdline[0] == 'R')
	debuginit(I_HSTREAD);
    else if (cmdline[0] == '\0')
    {
	(void) hstnext();
	(void) fputs(hstline(), stdout);
    }
#ifdef DEBUG
    else if (cmdline[0] == 'W')
    {
	(void) printf("Writing your history file\n");
	hstwrite(TRUE);
    }
    else if (sscanf(cmdline, "w %s", strv) >= 1)
    {
	(void) printf("Writing specified history-format file\n");
	(void) hstwrfile(xfopen(strv, "w"), 0);
    }
    else if (cmdline[0] == 'w')
    {
	(void) printf("Writing history data to stdout\n");
	(void) hstwrfile(stdout, 0);
    }
#endif /* DEBUG */
    else if (cmdline[0] == 'b')
    {
	(void) hstrewind();
	(void) fputs("History file rewound.\n", stdout);
	(void) fputs(hstline(), stdout);
    }
    else if (sscanf(cmdline, "g %s", strv) == 1)
    {
	if (hstseek(strv, FALSE) == FAIL)
	    (void) printf("%s: no such ID\n", strv);
	else
	    (void) fputs(hstline(), stdout);
    }
    else if (cmdline[0] == 'g')
    {
	struct tm *tm;

	(void) fputs(hstline(), stdout);

	tm = localtime(&chstdate);
#ifdef USG
	(void) printf("Received: %2.2d/%2.2d/%d %2.2d:%2.2d\n",
#else /* !USG */
	(void) printf("Received: %02d/%02d/%d %02d:%02d\n",
#endif /* !USG */
		tm->tm_mon+1,tm->tm_mday,tm->tm_year,tm->tm_hour,tm->tm_min);

		tm = localtime(&chstexpd);
#ifdef USG
	(void) printf("Expired: %2.2d/%2.2d/%d %2.2d:%2.2d\n",
#else /* !USG */
	(void) printf("Expired: %02d/%02d/%d %02d:%02d\n",
#endif /* !USG */
		tm->tm_mon+1,tm->tm_mday,tm->tm_year,tm->tm_hour,tm->tm_min);
#ifdef DBHISTORY
	(void) printf("Content length = %d\n", strlen(hstline()));
#endif /* DBHISTORY */
    }
    else if (cmdline[0] == 'e')
    {
	(void) hstenter(cmdline + 2);
    }
    else if (sscanf(cmdline, "l %s %s %d", strv, strv2, &nmsg) == 3)
    {
	(void) hstadd(strv, time((time_t *)NULL), (time_t) 0, strv2, nmsg);
	(void) fputs(hstline(), stdout);
    }
    else if (cmdline[0] == 'n')
    {
	if (hstloc(&exloc) != SUCCEED)
	    (void) fputs("No locations\n", stdout);
	else
	    (void) printf("Looking at \"%s/%d\"\n",
			  exloc.m_group->ng_name, exloc.m_number);
    }
    else if (cmdline[0] == 'd')
    {
	(void) hstexpire();
	(void) fputs(hstline(), stdout);
    }
    else if (cmdline[0] == 'D')
    {
	hstdrop();
	(void) fputs("Current article dropped\n", stdout);
    }
    else if (sscanf(cmdline, "c %s", strv) == 1)
    {
	(void) hstcancel(strv);
	(void) printf("Article %s cancelled\n", strv);
    }
    else if (cmdline[0] == 'i')
    {
	(void) printf("The article ID is \"%s\"\n", hstid());
    }
    else if (cmdline[0] == 't')
    {
	(void) printf("The receipt date is \"%s\"\n",arpadate(&chstdate));
    }
#ifdef DBHISTORY
    else if (cmdline[0] == 'X')
	hstclean();
#endif /* DBHISTORY */
    else if (cmdline[0] == '?')
    {
	(void) printf("R           -- read in your history file\n");
#ifdef DEBUG
	(void) printf("W           -- write default history file\n");
	(void) printf("w [file]    -- write given history-format file\n");
#endif /* DEBUG */

	(void) printf("g name      -- select and examine id <name>\n");
	(void) printf("g           -- show data on current entry\n");
	(void) printf("<CR>        -- go to next entry, and show it\n");
	(void) printf("b           -- rewind to beginning of history\n");
#ifdef DBHISTORY
	(void) printf("X           -- clear the history database\n");
#endif /* DBHISTORY */

	(void) printf("e           -- enter a new record\n");
	(void) printf("l id gp loc -- enter a new record\n");
	(void) printf("n           -- get the next location\n");
	(void) printf("d           -- expire the current location\n");
	(void) printf("D           -- drop the current article\n");
	(void) printf("c id        -- cancel the given article\n");
	(void) printf("i           -- return ID of current record\n");
	(void) printf("t           -- show date of current record\n");
	(void) printf("Omitted file arg to w defaults to stdout\n\n");
    }
    else
	return(FAIL);
    return(SUCCEED);
}

private int testnewsrc(cmdline)
/* exercise the .newsrc-handling functions */
char	*cmdline;
{
    char	strv[100], options[100];
    int		nv;

    if (cmdline[0] == 'R')
    {
	debuginit(I_READOPTS);
	ngdump(active.article.m_group);
    }
    else if (sscanf(cmdline, "r %s", strv) >= 1)
    {
	(void) printf("Reading specified .newsrc file...");
	(void) fflush(stdout);				    
	rdnewsrc(strv);
	(void) printf("%ld articles waiting\n", (long)rccount(FALSE));
	ngdump(active.article.m_group);
    }
    else if (cmdline[0] == 'U')
    {
	(void) printf("Updating your .newsrc file and options...");
	(void) fflush(stdout);
	(void) printf("there were %d changed groups\n", rcupdate());
    }
    else if (cmdline[0] == 'W')
    {
	(void) printf("Writing your .newsrc file\n");
	wrnewsrc();
    }
    else if (strcmp(cmdline, "w") == 0)
    {
	(void) printf("Writing .newsrc data to stdout\n");
	(void) wrrcfile(stdout, options);
    }
    else if (sscanf(cmdline, "w %s", strv) >= 1)
    {
	(void) printf("Writing specified newsrc-format file\n");
	wrrcfile(xfopen(strv, "w"), options);
    }
    else if (cmdline[0] == '\0')
    {
	(void) ngnext();
	ngdump(active.article.m_group);
    }
    else if (sscanf(cmdline, "g %s", strv) == 1)
    {
	if (strv[0] == '<')
	    (void) printf("%s is %ssubscribed\n",
					strv, ckfollow(strv) ? "un" : "");
	else
	{
	    group_t *ngp = ngselect(ngfind(strv));

	    if (ngp == (group_t *)NULL)
	    {
		(void) printf("%s: no such group\n", strv);
	    }
	    else
		ngdump(ngp);
	}
    }
    else if (cmdline[0] == 'g')
    {
	ngdump(active.article.m_group);
    }
    else if (sscanf(cmdline, "s %d", &nv) == 1)
    {
	(void) printf("State of article %d is %d\n",nv,getbit(nv,active.article.m_group));
    }
    else if (sscanf(cmdline, "m %d", &nv) == 1)
    {
	if (setbit(nv, active.article.m_group) == FAIL)
	    (void) printf("Couldn't mark article %d\n", nv);
	else
	    ngdump(active.article.m_group);
    }
    else if (sscanf(cmdline, "u %d", &nv) == 1)
    {
	int rstat = clearbit(nv, ngactive());

	if (rstat == FAIL)
	    (void) printf("Couldn't mark article %d\n", nv);
	else
	    ngdump(active.article.m_group);
    }
    else if (sscanf(cmdline, "z %d", &nv) == 1)
    {
	if (nv)
	    (void) printf("Subscribing to this group\n");
	else
	    (void) printf("Unsubscribing from this group\n");
	subscribe(nv);
	ngdump(active.article.m_group);
    }
    else if (sscanf(cmdline, "d %s", strv) == 1)
	dontfollow(strv);
    else if (cmdline[0] == '?')
    {
	(void) printf("R        -- read in your .newsrc file\n");
	(void) printf("r file   -- read specified .newsrc-format file\n");
	(void) printf("W        -- write default .newsrc file\n");
	(void) printf("w [file] -- write specified .newsrc-format file\n");
	(void) printf("U        -- update your .newsrc file\n");

	(void) printf("<CR>     -- select and examine next newsgroup\n");
	(void) printf("g name   -- select and examine newsgroup <name>\n");
	(void) printf("g        -- show data on current group\n");
	(void) printf("s artn   -- show status of given article\n");
	(void) printf("m artn   -- mark a given article read\n");
	(void) printf("u artn   -- mark a given article unread\n");
	(void) printf("z num    -- subscribe/unsubscribe current group\n");
	(void) printf("d	-- unsubscribe from discussion\n");
	(void) printf("Omitted file arg to w defaults to stdout\n");
	(void) printf("The current group is initially 'general'\n\n");
    }
    else
	return(FAIL);
    return(SUCCEED);
}

private int testnextart(cmdline)
/* exercise the next-article primitives */
char	*cmdline;
{
    char	strv[100];
    int		nv;

    if (cmdline[0] == 'R')
    {
	debuginit(I_MSGINIT); debuginit(I_READOPTS);

	if (procopts(eargc, eargv, DNC, options) == FAIL)
	    xerror0(usage);

	if (rccount(FALSE))
	{
	    initmsg(session.reread, session.reverse);
	    (void) printf("session: at %s/%ld\n", ngname(), (long)msgnum());
	}
    }
    else if (cmdline[0] == 'S')
	(void) printf("session: thread is %s\n",
		      (session.thread = !session.thread) ? "on" : "off");
    else if (cmdline[0] == 'N')
    {
	if (nextmsg(session.reread, session.reverse) == FAIL)
	    (void) printf("session: no more articles.\n");
	else
	    (void) printf("session: at %s/%ld.\n",
			  active.article.m_group->ng_name,
			  (long)active.article.m_number);
    }
    else if (cmdline[0] == '\0')
    {
	session.action = M_NEXT;
	if (msgnext() == FAIL)
	    (void) printf("session: no more articles.\n");
	else
	    (void) printf("session: at %s/%ld.\n",
			  active.article.m_group->ng_name,
			  (long)active.article.m_number);
    }
    else if (cmdline[0] == '-')
    {
	if (msglast() == FAIL)
	    (void) printf("session: can't move back.\n");
	else
	    (void) printf("session: at %s/%ld.\n",
			  active.article.m_group->ng_name,
			  (long)active.article.m_number);
    }
    else if (cmdline[0] == 'n')
    {
	if (nextgroup(session.reread, session.reverse) == FAIL)
	    (void) printf("session: no more groups.\n");
	else
	    ngdump(active.article.m_group);
    }
    else if (sscanf(cmdline, "g %s", strv) >= 1)
    {
	if (gotogrp(strv, session.reverse))
	    (void) printf("session: no such group.\n");
	else
	    ngdump(active.article.m_group);
    }
    else if (sscanf(cmdline, "t %d", &nv) >= 1)
    {
	if (gotomsg(nv))
	    (void) printf("session: no such article.\n");
	else
	    (void) printf("session: status of article %d is %d\n",
			      nv, getbit(nv,active.article.m_group));
    }
    else if (cmdline[0] == '<')
    {
	if (hstseek(cmdline, FALSE) == FAIL)
	    (void) printf("%s: no such ID\n", strv);
	else
	    (void) printf("session: status of article %d is %d\n",
			      nv, getbit(nv,active.article.m_group));
    }
    else if (cmdline[0] == 'm' && cmdline[0] == 'l')
	(void) msgmark(TRUE, LOCAL);
#ifdef DELAYMARK
    else if (cmdline[0] == 'm' && cmdline[0] == 'd')
	(void) msgmark(TRUE, DELAYED);
#endif /* DELAYMARK */
    else if (cmdline[0] == 'm')
	(void) msgmark(TRUE, GLOBAL);
    else if (cmdline[0] == 'f' && cmdline[0] == 'l')
	(void) msgmark(FALSE, LOCAL);
#ifdef FEEDBACK
    else if (cmdline[0] == 'r')
	(void) msgrate(F_SEENTEXT);
    else if (cmdline[0] == 'p')
	(void) msgrate(F_PRAISE);
    else if (cmdline[0] == 'c')
	(void) msgrate(F_CONDEMN);
    else if (cmdline[0] == 's')
	(void) msgsweep();
#endif /* FEEDBACK */
#ifdef DELAYMARK
    else if (cmdline[0] == 'f' && cmdline[0] == 'd')
	(void) msgmark(FALSE, DELAYED);
#endif /* DELAYMARK */
    else if (cmdline[0] == 'f')
	(void) msgmark(FALSE, GLOBAL);
    else if (cmdline[0] == '?')
    {
	(void) printf("R        -- read in active, history, .newsrc data\n");
	(void) printf("S        -- toggle thread-following bit\n");
	(void) printf("<CR>     -- find next article with msgnext()\n");
	(void) printf("N        -- find next article with nextart()\n");
	(void) printf("-        -- move to previous article in queue\n");
	(void) printf("n        -- select and examine next group\n");
	(void) printf("g name   -- select and examine newsgroup <name>\n");
	(void) printf("t <num>  -- go to given article in current group\n");
	(void) printf("<msgid>  -- go to article with given Message-ID\n");
	(void) printf("m        -- global-mark current message\n");
	(void) printf("ml       -- local-mark current message\n");
#ifdef FEEDBACK
	(void) printf("r        -- register current article as read\n");
	(void) printf("p        -- praise current article\n");
	(void) printf("c        -- condemn present article\n");
	(void) printf("s        -- dump session feedback records\n");
#endif /* FEEDBACK */
#ifdef DELAYMARK
	(void) printf("md       -- delay-mark current message\n");
#endif /* DELAYMARK */
	(void) printf("f        -- global-forget current message\n");
	(void) printf("fl       -- local-forget current message\n");
#ifdef DELAYMARK
	(void) printf("fd       -- delay-forget current message\n");
#endif /* DELAYMARK */
    }
    else
	return(FAIL);
#ifdef DEBUG
    /* dump session trail, so we can tell if it gets corrupted */
    if (debug)
    {
	msgrec_t	*mp;

	for (mp = session.trail + 1; mp <= session.cmsg; mp++)
	    (void) printf("    %d: {{%s/%ld}, %s, &%d}\n",
			  mp - session.trail,
			  mp->loc.m_group->ng_name, (long)mp->loc.m_number,
			  mp->follow ? mp->follow : "(NULL)",
			  mp->parent ? (mp->parent - session.trail) : -1);
    }
#endif /* DEBUG */

    return(SUCCEED);
}

private void dumpsubsc(gsubsc)
/* dump the names of all groups matching gsubsc */
char *gsubsc;
{
    FILE	*tf;
    char	tempfile[NAMELEN];

    if (active.article.m_group == (group_t *)NULL)
	debuginit(I_RDACTIVE);

    (void) strcpy(tempfile, "/tmp/subscXXXXXX");
    (void) mktemp(tempfile);
    tf = fopen(tempfile, "w+");
    (void) unlink(tempfile);

    ngrewind(TRUE);
    while (ngnext())
	if (ngmatch(ngname(), gsubsc))
	    (void) fprintf(tf, "%s -- accepted\n", ngname());
	else
	    (void) fprintf(tf, "%s -- rejected\n", ngname());

    (void) fseek(tf, (off_t)0, SEEK_SET);
    more((char *)NULL, tf, 0);
    (void) pclose(tf);
}

private int testfeeds(cmdline)
/* exercise the feeds file functions */
char	*cmdline;
{
    extern void		s_dump();
    char		strv[100], strv2[100], *opt;
    static feed_t	*sys;

    static vtab_t acctypes[] =
    {    
	{A_JUNKIT, "junked and relayed"},
        {A_ACCEPT, "accepted"},
        {A_GRPSNG, "ignored (group not carried)"},
	{A_DISTNG, "ignored (distribution known not good)"},
	{0,        (char *)NULL}
    }, *ap;

    if (cmdline[0] == 'R')
    {
	(void) printf("Reading default feeds file\n");
	s_read();
	if ((sys = s_next()) == (feed_t *)NULL)
	    (void) fputs("No valid feed records.\n", stdout);
	else
	    (void) fprintf(stdout, "System '%s'\n", sys->s_name);
    }
    else if (cmdline[0] == 'b')
    {
	(void) fprintf(stdout, "Rewinding...\n");
	s_rewind();
	sys = s_next();
	(void) fprintf(stdout, "System '%s'\n", sys->s_name);
    }
    else if (cmdline[0] == '\0')
    {
	if ((sys = s_next()) == (feed_t *)NULL)
	    (void) fputs("No more feed records.\n", stdout);
	else
	    (void) fprintf(stdout, "System '%s'\n", sys->s_name);
    }
    else if (sscanf(cmdline, "g %s", strv) == 1)
    {
	if ((sys = s_find(strv)) == (feed_t *)NULL)
	    (void) fputs("No such feed record.\n", stdout);
	else
	    (void) fprintf(stdout, "System '%s'\n", sys->s_name);
    }
    else if (cmdline[0] == 'g')
    {
	(void) fprintf(stdout, "System: %s\n", sys->s_name);
	(void) fprintf(stdout, "Groups: %s/%s\n", sys->s_ngroups,sys->s_gflex);
	(void) fprintf(stdout, "Distribs: %s/%s\n",
				sys->s_distribs, sys->s_dflex);
	(void) fprintf(stdout, "Flags: %s\nCmnd: %s\n",
		       sys->s_flags, sys->s_xmit);
    }
    else if (sscanf(cmdline, "l %s", strv) == 1)
	dumpsubsc(strv);
    else if (cmdline[0] == 'l')
	dumpsubsc(sys->s_ngroups);
    else if (sscanf(cmdline, "m %s %s", strv, strv2) == 2)
    {
	if (ngmatch(strv, strv2))
	    (void) printf("'%s' and '%s' match\n", strv, strv2);
	else
	    (void) printf("'%s' and '%s' don't match\n", strv, strv2);
    }
    else if (sscanf(cmdline, "m %s", strv) == 1)
    {
	(void) fputs(
		 ngmatch(strv, sys->s_ngroups) ? "Accepted\n" : "Rejected\n",
		 stdout);
    }
    else if (sscanf(cmdline, "f %s %s", strv) == 2)
    {
	int uval = ngaccept(sys);

	for (ap = acctypes; ap->name; ap++)
	    if (ap->val == uval)
		break;

	if (ap->name)
	    (void) printf("Articles in this group would be %s\n");
	else
	    (void) printf("ngaccept() returned weird value %d\n", uval);
    }
    else if (sscanf(cmdline, "o %s", strv) == 1)
    {
	if ((opt = s_option(sys, strv)) == (char *)NULL)
	    (void) printf("No option %s on %s\n", opt, sys->s_name);
	else
	    (void) printf("Option %s on %s is %s\n", strv, sys->s_name, opt);
    }
#ifdef FEEDBITS
    else if (cmdline[0] == 'F')
    {
	if (active.article.m_group == (group_t *)NULL)
	    debuginit(I_RDACTIVE);
	feedbits();
    }
#endif /* FEEDBITS */
    else if (sscanf(cmdline, "a %s %s", strv, strv2) == 2)
	s_changesub(strv, strv2, TRUE);
    else if (sscanf(cmdline, "d %s %s", strv, strv2) == 2)
	s_changesub(strv, strv2, FALSE);
#ifdef DEBUG
    else if (cmdline[0] == 'w')
	s_dump(stdout);
#endif /* DEBUG */
    else if (cmdline[0] == '?')
    {
	(void) printf("R           -- read default feeds file\n");
	(void) printf("b           -- rewind feeds file pointer\n");
	(void) printf("<CR>        -- select next feed description\n");
	(void) printf("g system    -- select feed to <name>\n");
	(void) printf("g           -- display information on feed\n");
	(void) printf("l subsc     -- show all groups matching <subsc>\n");
	(void) printf("l           -- show groups accepted by this site\n");
	(void) printf("m grp subsc -- does grp match subsc?\n");
	(void) printf("m grp       -- is grp accepted for feed?\n");
	(void) printf("f grp dist  -- dist.grp accepted for feed?\n");
	(void) printf("o opt       -- value of option on current system\n");
#ifdef FEEDBITS
	(void) printf("F           -- load cached feed bits\n");
#endif /* FEEDBITS */
	(void) printf("a site grp  -- add subscription to given site\n");
	(void) printf("d site grp  -- delete subscription from given site\n");
#ifdef DEBUG
	(void) printf("w           -- dump altered feeds file to stdout\n");
#endif /* DEBUG */
    }
    else
	return(FAIL);
    return(SUCCEED);
}

private int testfilter(cmdline)
/* filter-testing code */
char	*cmdline;
{
    char    strv[BUFSIZ], strv2[BUFSIZ], filtername[BUFSIZ];
    int	    level;

    if (cmdline[0] == 'R')
    {
	debuginit(I_RDACTIVE);
	return(SUCCEED);
    }
    else if (cmdline[0] == '?')
    {
	(void) printf("o name lang -- open named filter\n");
	(void) printf("a id file   -- return score of article\n");
	(void) printf("g grp       -- return score of group\n");
	(void) printf("p command   -- enter filtering commmand\n");
#ifdef DEBUG
	(void) printf("D level     -- set debug level\n");
#endif /* DEBUG */
	(void) printf("c           -- close (terminate filter)\n");
    }
    else if (sscanf(cmdline, "o %s %s", filtername, strv) == 2)
    {
	if (fltinit(filtername, strv, VPROTO) != FAIL)
	    (void) fprintf(stderr, "Filter initialization succeeded.\n");
	else
	    (void) fprintf(stderr, "Filter initialization failed!\n");
	return(SUCCEED);
    }
    else if (sscanf(cmdline, "a %s %s", strv, strv2) == 2)
    {
	group_t	*ngp;
	char	filename[BUFSIZ];

	if (strv2[0] != '/')
	{
	    char	*cp;

	    (void) sprintf(filename, "%s/%s", site.textdir, strv2);
	    for (cp = filename; *cp; cp++)
		if (*cp == NGSEP)
		    *cp = '/';
	}
	(void) fprintf(stderr, "Score is %d\n", fltarticle(strv, filename));
	if (fltinfo[0])
	    (void) printf("Command returned: %s\n", fltinfo);
	return(SUCCEED);
    }
    else if (sscanf(cmdline, "g %s", strv) == 1)
    {
	(void) fprintf(stderr, "Score is %d\n", fltnewsgroup(strv));
	if (fltinfo[0])
	    (void) printf("Command returned: %s\n", fltinfo);
	return(SUCCEED);
    }
    else if (sscanf(cmdline, "p %s", strv) == 1)
    {
	if (fltprogram(strv) != FAIL)
	    (void) fprintf(stderr, "Filter command accepted\n");
	else
	    (void) fprintf(stderr, "Filter command rejected\n");
	if (fltinfo[0])
	    (void) printf("Command returned: %s\n", fltinfo);
	return(SUCCEED);
    }
#ifdef DEBUG
    else if (sscanf(cmdline, "D %d", &level) == 1)
    {
	debug = level;
	return(SUCCEED);
    }
#endif /* DEBUG */
    else if (cmdline[0] == 'c')
    {
	if (fltbye(filtername) == FAIL)
	    (void) fprintf(stderr, "Filter termination failed!\n");
	else
	    (void) fprintf(stderr, "Filter termination succeeded.\n");
	if (fltinfo[0])
	    (void) printf("Command returned: %s\n", fltinfo);
	return(SUCCEED);
    }
    else
	return(FAIL);
}

private int testmisc(cmdline)
/* exercise miscellaneous library functions */
char	*cmdline;
{
    char	strv[100], strv2[100], *opt;
    int		nv;

    if (cmdline[0] == 'R')
    {
	(void) printf("News version is: %s\n", NEWS_VERSION);
	return(SUCCEED);
    }
    else if (cmdline[0] == 's')
    {
	(void) printf("System admin mail address: %s\n", site.notify);
	(void) printf("System node name: %s\n", site.nodename);
	(void) printf("System path name: %s\n", site.pathname);
	(void) printf("System from name: %s\n", site.fromname);
	(void) printf("System true name: %s\n", site.truename);
	(void) printf("System autorouter: %s\n", site.smarthost);
	(void) printf("System library directory: %s\n", site.libdir);
	(void) printf("System administration directory: %s\n", site.admdir);
#ifdef NONLOCAL
	(void) printf("System articles directory: %s\n", site.textdir);
	(void) printf("System spool directory: %s\n", site.spooldir);
	(void) printf("System batch directory: %s\n", site.batchdir);
#endif /* NONLOCAL */
	return(SUCCEED);
    }
    else if (sscanf(cmdline, "p %s", strv) >= 1)
    {
	(void) printf("Attribute %s is: %s\n",
		      strv, newsattr(strv, "UNDEFINED"));
	return(SUCCEED);
    }
#ifdef DEBUG
    else if (sscanf(cmdline, "d %d", &nv) >= 1)
    {
	(void) printf("session: debug level set to %d\n", debug = nv);
	if (verbose < debug)
	    verbose = debug;
	return(SUCCEED);
    }
#endif /* DEBUG */
    else if (sscanf(cmdline, "f %s", strv) == 1)
    {
	(void) printf("physical blocks free: %ld\n", df(strv));
	return(SUCCEED);
    }
    else if (sscanf(cmdline, "u %s", strv) == 1)
    {
	(void) printf("size of UUCP queue for %s: %ld\n", strv, uuq(strv));
	return(SUCCEED);
    }
    else if (cmdline[0] == 'e')
    {
	fnexpand(cmdline + 1, bfr);
	(void) printf("\"%s\"\n", bfr);
	return(SUCCEED);
    }
    else if (cmdline[0] == 'm')
    {
	FILE *pfp;
	int olddebug = debug;

	if (debug < 1)
	    debug = 1;
	pfp = mailopen(cmdline + 1, "sysmail() test");
	(void)printf("Enter text lines, end with line containing leading .\n");
	while (fgets(bfr, BUFLEN, stdin) != (char *)NULL)
	    if (bfr[0] == '.')
		break;
	    else
		(void) fputs(bfr, pfp);
	(void) mailclose(pfp);
	debug = olddebug;
	return(SUCCEED);
    }
    else if (cmdline[0] == 'w')
    {
	FILE *pfp;
	int pestat;

	if ((pfp = peopen(cmdline + 1, "w")) == (FILE *)NULL)
	    (void) fprintf(stderr,
			   "newsdb: peopen(%s, \"w\") failed\n",cmdline+1);
	else
	{
	    (void) fputs("Type lines for command, end with .\\n\n", stdout);
	    while (*fgets(strv, sizeof(strv), stdin) != '.')
		(void) fputs(strv, pfp);
	    (void) fputs("*** End of input\n", stdout);
	}
	if (pestat = peclose(pfp))
	    (void) fprintf(stderr,
			   "peclose: child status was %d, errno %d\n",
			   errno, pestat);
	return(SUCCEED);
    }
    else if (cmdline[0] == 'r')
    {
	FILE *pfp;
	int pestat;

	if ((pfp = peopen(cmdline + 1, "r")) == (FILE *)NULL)
	    (void) fprintf(stderr,
			   "newsdb: peopen(%s, \"r\") failed\n", cmdline+1);
	else
	{
	    while (fgets(strv, sizeof(strv), pfp) != (char *)NULL)
		(void) fputs(strv, stdout);
	    (void) fputs("*** End of output\n", stdout);
	}
	if (pestat = peclose(pfp))
	    (void) fprintf(stderr,
			   "peclose: child status was %d, errno %d\n",
			   errno, pestat);
	return(SUCCEED);
    }
    else if (sscanf(cmdline, "n %s", strv) == 1)
    {
#ifdef FASCIST
	nasty_t	*perms = fascist(strv);
#endif /* FASCIST */

	(void) printf("User %s's full name is: %s\n", strv, fullname(strv));
#ifdef FASCIST
	(void) printf("Posting: \"%s\", reading \"%s\"\n",
		      perms->n_post, perms->n_read);
#endif /* FASCIST */
	return(SUCCEED);
    }
    else if (cmdline[0] == 't')
    {
#ifndef QUICKDATE
	time_t	timestamp = getdate(cmdline + 1, (struct timeb *)NULL);
#else
	extern time_t quickdate();

	time_t	timestamp = quickdate(cmdline + 1);
#endif /* QUICKDATE */

	if (timestamp == (time_t)FAIL)
	    (void) printf("Can't parse that date.");
	else
	    (void) fputs(arpadate(&timestamp), stdout);
	(void) fputc('\n', stdout);
	return(SUCCEED);
    } 
    else if (cmdline[0] == 'o')
    {
	(void) printf("Organization: %s\n", organization());
	return(SUCCEED);
    }
    else if (sscanf(cmdline, "h %s", strv) == 1)
    {
	(void) printf("%s hashes to %lx\n", strv, checkstring(strv));
	return(SUCCEED);
    }
    else if (cmdline[0] == '?')
    {
	(void) printf("R        -- show news version\n");
	(void) printf("s        -- show news configuration attributes\n");
	(void) printf("p <name> -- show a news attribute\n");
#ifdef DEBUG
	(void) printf("d <num>  -- set debug verbosity\n");
#endif /* DEBUG */
	(void) printf("f <dir>  -- show free blocks on a file system\n");
	(void) printf("u <sys>  -- show uucp queue size for system\n");
	(void) printf("e str    -- escape-expand given string\n");
	(void) printf("m <addr> -- spawn a copy of the configured mailer\n");
	(void) printf("r <cmd>  -- read-popen the given command\n");
	(void) printf("w <cmd>  -- write-popen the given command\n");
	(void) printf("n <user> -- show name, authorizations for user\n");
	(void) printf("t <time> -- try to parse date into standard form\n");
	(void) printf("o        -- generate Organization line\n");
	(void) printf("h <str>  -- show checkstring() hash of string\n");
	return(SUCCEED);
    }
    else
	return(FAIL);
}

private int toplevel(cmdline)
/* top level of the news debugger */
char	*cmdline;
{
    char	stringarg[BUFLEN];

    if (cmdline[0] == '?')
    {
	(void) printf("a        -- view/edit active file\n");
	(void) printf("h        -- view/edit history data\n");
	(void) printf("r        -- view/edit .newsrc file\n");
	(void) printf("s        -- view/check feeds file\n");
	(void) printf("i        -- exercise next article logic\n");
	(void) printf("f        -- exercise an article filter\n");
	(void) printf("m        -- exercise miscellaneous library tests\n");
	return(SUCCEED);
    }
    else
    {
	interp_t *ni;

	for (ni = methods; ni->key; ni++)
	    if (cmdline[0] == ni->key)
	    {
		if ((ip = ni) != methods && ip->firsttime++ == 0)
		    (*ip->interp)("R");
		return(SUCCEED);
	    }
    }
    return(FAIL);
}

/* debugger methods end here */

/* static */ interp_t methods[] =
{
	{' ', toplevel,   "newsdb> ",            "news debugger"},
	{'a', testactive, "active> ",      "active file editor"},
#ifndef DBHISTORY
	{'h', testhist,   "dbm history> ", "history file editor"},
#else
	{'h', testhist,   "history> ",     "history file editor"},
#endif /* DBHISTORY */
	{'r', testnewsrc, ".newsrc> ",     "newsrc file editor"},
	{'s', testfeeds,  "feeds> ",       "feeds file checker"},
	{'i', testnextart,"nextart> ",     "next-article primitive tester"},
	{'f', testfilter, "filt> ",	   "filter tester"},
	{'m', testmisc,	  "misc> ",	   "miscellany tester"},
	{'\0'}
},
*ip = methods;

main(argc, argv)
/* main sequence of the debugger */
int	argc;
char	**argv;
{
    char	cmdline[LBUFLEN], *dp;
    int		repeats;
    interp_t	*oldip = (interp_t *)NULL;

    (void) signal(SIGINT, SIGCAST(sigcatch));

    newsinit();

    eargc = argc;
    eargv = argv;

    for (;;)
    {
	(void) setjmp(sigjmpbuf);

	if (ip != oldip)
	    (void)printf("This is the %s, type ? for help\n",ip->longname);

	if (fputs(ip->prompt, stdout)==EOF || fgets(cmdline, sizeof(cmdline), stdin)==(char *)NULL)
	    break;

	/* first, check for common commands */
	if (cmdline[0] == 'x' || cmdline[0] == 'q')
	{
	    if (ip == methods)
		xxit(0);
	    else
	    {
		ip = methods;
		continue;
	    }
	}
	else if (cmdline[0] == '!')
	{
	    (void) system(cmdline + 1);
	    (void) fflush(stdout);	/* V7 system() doesn't flush stdout */
	    continue;
	}

	repeats = 0;
	for (dp = cmdline; *dp && isdigit(*dp); dp++)
	    repeats = repeats * 10 + ctoi(*dp);
	if (repeats == 0)
	    repeats = 1;
	oldip = ip;

	/* next, go to the current interpreter for dispatch */
	while (repeats--)
	    if ((*ip->interp)(dp) != SUCCEED)
	    {
		(void)fprintf(stdout,"Illegal command, type ? for help\n");
		break;
	    }

	/* finally, if user called for help, print common-command help */
	if (cmdline[0] == '?')
	{
	    (void) printf("\nx           -- exit\n");
	    (void) printf("! cmd       -- execute shell command\n");
	    (void) printf("?           -- print this help message\n");
	}
    }
}

/*ARGSUSED0*/
void vcommand(ca) cmdarg_t *ca; {/* do nothing */}

void vupdate() {/*no-op*/}
#ifdef lint
int vgetc() {return('\0');}
char *vgetline(prompt) char *prompt; {return(prompt);}
#endif /* lint */

catch_t xxit(status)
int status;
{
    (void) fputs("Exiting debugger, bye\n", stdout);
    exit(status);
}

/* newsdb.c ends here */
