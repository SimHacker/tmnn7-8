/****************************************************************************

NAME
   checkinit.c -- common initialization code of the news readers

SYNOPSIS
   int checkinit(argc, argv, hook)	-- generic news browser initialization
   int argc, char *argv[]; int (*int)();

   int lprocargs(opts)	    -- process read options from text string
   char *opts

   bool aselect(hp)	    -- article-selection predicate
   hdr_t *hp;

   void qfflush(fp)	    -- quiet flush
   FILE *fp;

DESCRIPTION
   This module provides common initialization code for the news reader
interfaces. The checkinit() function expects to find a public variable
'options' containing an options table for the reader, and another, 'usage',
containing a usage message. It returns a count of waiting messages.

   The aselect() function may be called on a message header any time during
the reader execution to check that it matches the selection defined by the
grouplist, datebuf and titbuf buffers (which are set by readnews -n, -a, and
-t options or their equivalents). It will also return FALSE if the message is
a followup and 'nofoll' has been set (by an -f option or equivalent).

NOTE
   The lprocargs() function assumes that argv[0] of the current runfile is
available in the extern 'Progname'.

SEE ALSO
   readnews.c	-- the original readnews command interpreter
   vnews.c	-- visual-interface interpreter
   mailnews.c	-- mailer interface code
   rfuncs.c	-- common functions used by interactive news readers
   procopts.c	-- argument processing code

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
#ifdef RECMDS
#include "regexp.h"
#endif /* RECMDS */
#include "libread.h"
#include "active.h"	/* the place_t structure needs the group_t type */
#include "newsrc.h"
#include "history.h"
#include "nextmsg.h"
#include "dballoc.h"	/* session.h needs the dbdef_t type */
#include "session.h"

time_t	    starttime;		/* use this to track news-reading time */

/* these are set by various options */
char	datebuf[BUFLEN];	/* where dates are kept */
char	titlebuf[BUFLEN];	/* where title to be matched lives */
char	grouplist[BUFLEN];	/* where -n option values are kept */
int	nofoll = FALSE;		/* set TRUE to suppress followups */

private time_t	atime;

private int titmat(h, titlist)
/* check to see if the article title contains the given string */
register hdr_t *h;
register char	*titlist;
{
    register char  *p;

    while (*titlist != '\0')
    {
	if (strcmp(titlist, h->h_ident) == 0)
	    return(TRUE);
	for (p = h->h_subject; *p != '\0'; p++)
	    if (prefix(p, titlist))
		return(TRUE);
	while (*titlist++ != '\0')
	    continue;
    }
    return(FALSE);
}

bool aselect(hp)
/* Return TRUE if user's option selections have not ruled out this article */
register hdr_t *hp;
{
    /* note that we don't check subscriptions here */
    if (titlebuf[0] && !titmat(hp, titlebuf))
	return(FALSE);
    if (datebuf[0] && hp->h_rectime < atime)
	return(FALSE);
    if (nofoll && (hlnblank(hp->h_references)||!strncmp(hp->h_subject,"Re:",3)))
	return(FALSE);
    return(TRUE);
}

int checkinit(argc, argv, hook)
/* newsrc file set up for checknews and other statistics-gatherers */
int	argc;		/* count of command-line arguments */
char	*argv[];	/* the arguments themselves */
int	(*hook)();	/* directive-processing hook */
{
    extern option_t options[];
    
    newsinit();			/* set up defaults and initialize */
    starttime = time((time_t *)NULL);

    /* go arrange access to history and active data */
    if (datebuf[0] || titlebuf[0] || nofoll)
	msginit(aselect);
    else
	(void) msginit(NULLPRED);

#ifdef COMMUNIST
    /* this is to keep readers who are setuid root within security bounds */
    if (duid == 0 && uid != 0)
	(void) setuid(getuid());
#endif /* COMMUNIST */

    /* now get user's .newsrc data */
    if (!session.reread)
	if (readopts(hook) == FAIL)
	    xerror0("Your .newsrc file appears to be corrupted!");

    /* process command line to override .newsrc options */
    if (procopts(argc, argv, DNC, options) == FAIL)
	xerror0(usage);

#ifdef DEBUG
    if (debug > verbose)
	verbose = debug;
#endif /* DEBUG */

    /*
     * if the user has set a threshold date with -a, set it
     */
    if (*datebuf)
    {
	if ((atime = getdate(datebuf, (struct timeb *)NULL)) == -1)
	    xerror0("Cannot parse date string");
    }
    else
	atime = 0;

    /*
     * unset the appropriate bits for our subscription in the groups table
     * if an -n option was given.
     */
    if (grouplist[0])
      setsubsc(grouplist);

    /* verify that there is news, else some interface managers may barf */
    if (session.waiting = rccount(session.reread))
	initmsg(session.reread, session.reverse);
    else if (checkngs(header.h_newsgroups))
	return(FAIL);

    /* pass back the count of waiting messages */
    return(session.waiting);
}

int lprocargs(opts)
/* process options specified in a text string */
char	*opts;		/* option string */
{
    extern char	    *Progname;
    extern option_t options[];

    int	    largc = 1, selected = 0;
    char    *largv[MAXARGS];
    char    text[LBUFLEN];

    /* we need our own copy of the option text to alter */
    if (opts == (char *)NULL || *opts == '\0')
	return(SUCCEED);
    else
	(void) strncpy(text, opts, sizeof(text) - 1);

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
    return(procopts(largc - selected, largv + selected, DNC, options));
}

private int checkngs(nbuf)
/* check that the groups in the user's subscription list are valid */
char *nbuf;
{
    register char   schar, *cp, *sp;
    int	     badcount = 0;

    ngreset(TRUE);
    for (cp = nbuf; *cp; cp = sp)
    {
	while (*cp && (isspace(*cp) || *cp == NGDELIM))
	    cp++;

	if (*cp == '\0')
	    return(badcount);

	sp = cp;
	while (*sp && (isalnum(*sp) || *sp == NGSEP || *sp == NEGCHAR))
	    sp++;

	if (strncmp(cp, "all", 3) == 0 && !isalnum(cp[3]))
	    continue;

	if (*cp != UNSUBSCMK)
	{
	    schar = *sp;
	    *sp = 0;
	    if (ngfind(cp) == (group_t *)NULL)
	    {
		(void) fprintf(stderr, "%s: no such group\n", cp);
		badcount++;
	    }
	    *sp = schar;
	}
    }

    return(badcount);
}

/*ARGSUSED*/
void qfflush(fp)
/* quiet 'flush' -- empty (without fflush()) the buffer for stream fp */
FILE *fp;
{
#ifdef fileno
    int	fno, err;

    fno = fileno(fp);
    err = ferror(fp);
    fileno(fp) = -1;
    (void) fflush(fp);
    fileno(fp) = fno;
    if (!err)
	(void) clearerr(fp);
#endif /* fileno */
}

/* checkinit.c ends here */
