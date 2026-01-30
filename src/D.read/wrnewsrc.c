/****************************************************************************

NAME
   wrnewsrc.c -- functions for modifying and writing .newsrc files

SYNOPSIS
   #include "active.h"
   #include "newsrc.h"

   int readopts(optf)		-- load the current user's .newsrc options
   char *newsrc, int (*optf)();

   int rcupdate(ngp)		-- check to see if more news awaits
   group_t *ngp;

   nart_t rccount(reread)	-- count waiting messages
   bool reread;

   void wrnewsrc()		-- write caller's .newsrc data back out

   void setsubc();		-- set a subscription list

   void dontfollow(id)		-- add id to list of unsubscribed discussions
   char *id;

   bool ckfollow(id)		-- check if followups to id are unsubscribed
   char *id;

   void subscribe()		-- subscribe to current group

   bool unsubscribe()		-- unsubscribe from current group

DESCRIPTION
   These functions work with rdnewsrc.c to provide a clean interface to the
.newsrc files described in newsrc(5). They are intended for the use of news
reader programs. They assume that rdactive() has been called to initialize
the in-core data structure that holds group data.

   The readopts() function is a wrapper for rdnewsrc() that gets
data on the current user, permits the environment variable NEWSRC to override
the default .newsrc name, and enables storage of comments, option lines and
unsubscribed-discussion IDs. The function argument should be a pointer to a
function usable for interpreting option lines; if it is NULL, option lines are
ignored (but are always written out with the file).

   If readopts() can't find a .newsrc file, it assumes it's dealing with a
new user. A new .newsrc is written with default options written out at its
head.

   Additionally, sortactive() will be invoked to reorder the in-core data by
its .newsrc index. This function is only defined if SORTACTIVE is on.

   When the session is done, a call to wrnewsrc() must be made to ensure
that the .newsrc file reflects the changes in the in-core data. This functions
write the canonical text form that rdnewsrc() and readopts() expects to see.
If DEBUG is enabled and debug is on, the old .newsrc is kept around as
.newsrc-old.

  The rcupdate() function checks to see if new news has come in during the
session. A NULL argument updates all groups; a non-NULL argument is assumed
to be a group data pointer and updates only the given group. The code leaves
the group array information (including seen bits) updated, and returns the
number of groups that had new news. Otherwise it returns zero.

  The dontfollow() and ckfollow() functions maintain a list of article IDs
for which all followups should be discarded.

SEE ALSO
   rdnewsrc.c		-- read in seen .newsrc info
   rdbits.c		-- functions to examine and set seen bits

FILES
   ~/.newsrc		-- per-user info on which articles have been read
   ~/.newsrc-new	-- temp file for updated version
   ~/.newsrc-old	-- old version, useful for debugging

AUTHOR
   Eric S. Raymond
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

**************************************************************************/
/*LINTLIBRARY*/
#include "news.h"
#include "active.h"
#include "newsrc.h"
#include "dballoc.h"
#include "alist.h"
#include "slist.h"
#include "grow.h"

#define NOTFOUND	99999L	/* marks a group not in the .newsrc */

SLIST(comments, 20, 10)		/* hold comments from the .newsrc file */
SLIST(discussions, 20, 10)	/* hold list of unsubscribed discussions */

/* things that wractive.c and other modules must see */
private int	ngrc;		/* count of groups in last .newsrc */
private char	newsrc[BUFLEN];	    /* where to keep current .newsrc */

#ifdef SORTACTIVE

private int lcompar(p1, p2)
char	*p1, *p2;
{
    register long	diff;

#ifdef lint
    diff = (int)(p1 - p2);  /* we don't know if ptrdiff_t == int */
#else
    diff = ((group_t *) p1)->rc_lindex - ((group_t *) p2)->rc_lindex;
#endif /* lint */
    if (diff > 0)
	return(1);
    if (diff < 0)
	return(-1);
#ifdef lint
    return(0);
#else
    return(strcmp(((group_t *) p1)->ng_name, ((group_t *) p2)->ng_name));
#endif /* lint */
}

private void sortactive()
{
    register group_t *ngp, *mgp, *lastgrp = active.newsgroups + active.ngc;

    /*
     * try to index groups not mentioned next to parent groups of their
     * subhierarchies, so that a new group 'comp.foo.baz' will get
     * sorted after 'comp.foo.bar' and before 'comp.foo.qux'.
     */
    for (ngp = active.newsgroups; ngp < lastgrp; ngp++)
    {
	if (!(ngp->rc_flags & RC_NOTED))
	{
	    int			preflen = 0;
	    register group_t	*match = (group_t *)NULL;

	    /* search for longest prefix match (left in mgp if there is one */
	    for (mgp = active.newsgroups; mgp < lastgrp; mgp++)
	    {
		int	grlen = strlen(mgp->ng_name);

		if (grlen > preflen
		    && !strncmp(mgp->ng_name, ngp->ng_name, grlen)
		    && grlen < strlen(ngp->ng_name))
		{
		    match = mgp;
		    preflen = grlen;
		}
	    }

	    if (match == (group_t *)NULL)
	        ngp->rc_lindex = NOTFOUND;
	    else
	    {
		register group_t *gp;

		/*
		 * If we found a longest prefix match, try advancing until
		 * something doesn't share that long a prefix with or else
		 * sorts greater-than the new group, then give
		 * the new newsgroup its sort index and trust the sort
		 * function to order them properly.
		 */
		for (gp = mgp; gp < lastgrp; gp++)
		    if (strncmp(gp->ng_name, mgp->ng_name, preflen))
			break;
		    else if (strcmp(gp->ng_name, mgp->ng_name) > 0)
			break;
		ngp->rc_lindex = gp->rc_lindex;
	    }
	}
    }

    /* here goes the actual sort */
    (void) qsort((char *)active.newsgroups,
		 (iolen_t)active.ngc, sizeof(group_t), lcompar);

#ifdef HASHGROUPS
    ngrehash();		/* regenerate hashlists according to ng_oindex */
#endif /* HASHGROUPS */
}
#endif /* SORTACTIVE */

int readopts(optfunc)
/* read reader options data out of a given .newsrc file */
int	(*optfunc)();	/* option-processing hook */
{
    char	    *cp;
#ifdef GROW
    struct grow	    *parent = (struct grow *)NULL;
    extern char	    *growfp();
    extern struct grow	    *growinit();
    char	    *rcline;
#else
    char	    rcline[BUFLEN];
#endif /* GROW */    
    FILE	    *fp;

    /* user may want to change .newsrc locations */
    if ((cp = getenv("NEWSRC")) == (char *)NULL)
	cp = ".newsrc";

    /* here's where we generate and write the actual name/.newsrc pair */
#ifdef RNESCAPES
    if (getenv("DOTDIR") != (char *)NULL)
	(void) sprintf(newsrc, "%s/%s", getenv("DOTDIR"), cp);
    else
#endif /* RNESCAPES */
	(void) sprintf(newsrc, "%s/%s", userhome, cp);

    /*
     * If there's no .newsrc file waiting, create one.
     * If SORTACTIVE is defined, cause DFLTSUB groups to show first.
     */
    if (!exists(newsrc))
    {
	fp = xfopen(newsrc, "w");
	(void) strcpy(bfr, DFLTSUB);
	cp = strtok(bfr, LISTSEP);
	do {
	    (void) fprintf(fp, "%s:\n", cp);
	} while
	    (cp = strtok((char *)NULL, LISTSEP));
	ngrewind(TRUE);
	while (ngnext())
	    if (!ngmatch(ngname(), DFLTSUB))
		(void) fprintf(fp, "%s:\n", ngname());
	(void) fclose(fp);
    }

    /*
     * if there's no such accessible file after what we just did,
     * there's something too badly wrong for us to muck with!
     */
    fp = xfopen(newsrc, "r");

    /* read in all the seen bits */
    if (rdnewsrc(newsrc))
	return(FAIL);

    /* loop through the .newsrc file lines, interpreting them */
#ifdef GROW
    for (parent = growinit(parent);
	 rcline = growfp(parent, fp); parent = growinit(parent))
#else
	    while (fgets(rcline, sizeof(rcline), fp))
#endif /* GROW */
		    
    {
	group_t	*ngp;

	(void) nstrip(rcline);

	/* skip blank lines */
	if (rcline[0] == '\0' || isspace(rcline[0]))
	    continue;

	/*
	 * We must check for directives *before* checking for group lines
	 * so the directives can contain ! without getting clobbered. If
	 * the evaluation fails, the directive will be treated as a comment.
	 * This is a feature, not a bug! -- new directive types will doubtless
	 * be added in the future, we want this code to ignore them.
	 */
	if (optfunc != NOOPTS)
	    if (strncmp(rcline, "options", 7) == SUCCEED)
	    {
		if ((*optfunc)(N_OPTIONS, rcline + 7) != FAIL)
		{
		    senter(&comments, rcline);
		    continue;
		}
	    }
#ifdef MACROS
	    else if (strncmp(rcline, "macro", 5) == SUCCEED)
	    {
		if ((*optfunc)(N_MACRO, rcline + 7) != FAIL)
		{
		    senter(&comments, rcline);
		    continue;
		}
	    }
#endif /* MACROS */
	/* add more syntax extensions here */

	/* check that the format is O.K for a group line */
	if ((cp = strchr(rcline, SUBSCMK)) || (cp = strchr(rcline, UNSUBSCMK)))
	{
	    *cp++ = 0;

#ifdef CRACKMAIL
	    /* map mail.* groups so the mailbox will get cracked */
	    if (strncmp(rcline, "mail.", 5) == SUCCEED)
	    {
		bool	changed = (modtime(cp) > modtime(ACTIVE));

		if ((ngp->rc_box = crackmail(cp, changed)) == (mbox_t *)NULL)
		    continue;
		ngp = ngalloc();
		ngp->rc_flags = RC_MAILBOX;
		ngp->ng_min = 1;
		ngp->rc_unread = ngp->ng_max = ngp->rc_box->mb_artcount;
		for (i = 1; i <= ngp->ng_max; i++)
		    if (ngp->rc_box->mb_seen[i])
			(void) setbit(ngp, i);
	    }
	    else
#endif /* CRACKMAIL */

	    /* ignore groups not in the in-core group list */
	    if ((ngp = ngfind(rcline)) == (group_t *)NULL)
		continue;
	    else
	    {
		ngselect(ngp);
	    }

#ifdef SORTACTIVE
	    /* set up an index to sort by */
	    ngactive()->rc_lindex = ngrc++;
#endif /* SORTACTIVE */

	    continue;
	}

#ifndef B211COMPAT
	/* detect and enter lines that are actually ignore IDs */
	if (prefix(rcline, "ignore"))
	{
	    dontfollow(rcline + 7);	/* 7 = strlen("ignore") + 1; */
	    continue;
	}
#endif /* B211COMPAT */

	/* only comments get this far */
	senter(&comments, savestr(rcline));
    }

#ifdef SORTACTIVE
    sortactive();
#endif /* SORTACTIVE */

    return(SUCCEED);
}

private bool rcmap(new, old)
/* fix the bitmaps in a group record to reflect its new state */
group_t	    *new, *old;
{
    register int    i;

    /* preserve information about unsubscriptions in this session */
    new->rc_flags = old->rc_flags;

    /* have to fix up new bitmaps to reflect the changed state of things */
    if (new->ng_min != old->ng_min || new->ng_max != old->ng_max)
    {
	new->ng_unread = new->ng_max - new->ng_min + 1;
	for (i = new->ng_min; i <= new->ng_max; i++)
	    if (getbit(i, old) == TRUE)
		(void) setbit(i, new);

	if (old->rc_seen)
	    (void) free((char *)old->rc_seen);
	old->rc_seen = (uchar *)NULL;

	return(TRUE);
    }
    else	/* no change in group status, just copy old data */
    {
	new->ng_unread = old->ng_unread;
	new->rc_seen = old->rc_seen;
	return(FALSE);
    }
}

#ifndef NGUPDATE
/*ARGSUSED0*/
#endif /* !NGUPDATE */
int rcupdate(ngp)
/* re-check active and .newsrc files for new pending messages */
group_t	*ngp;
{
#ifndef NGUPDATE
    if (ngp)
	return(ngreread(artloc.m_group, rcmap));
    else
#endif /* NGUPDATE */
	return(rdactive(rcmap));
}

nart_t rccount(reread)
/* count waiting messages */
bool	reread;
{
    nart_t	ngtotwait = 0;

    ngrewind(TRUE);
    while (ngnext())
    {
	if (rcsubsc(ngactive()))
	{
	    if (reread)
		ngtotwait += ngmax() - ngmin() + 1;	/* this is inexact */
	    else
		ngtotwait += ngunread();
	}
    }
    return(ngtotwait);
}

void setsubsc(list)
/* set the list of groups we're subscribed to */
char	*list;	/* comma-separated list of groups to subscribe to */
{
    register group_t	*ngp;

    if (list == (char *)NULL || *list == '\0')
	return;

    for (ngp = active.newsgroups; ngp < active.newsgroups + active.ngc; ngp++)
	if (!ngmatch(ngp->ng_name, list))
	    ngp->rc_flags |= RC_UNSEL;
}

void subscribe()
/* subscribe to a new group */
{
    rcfclear(RC_UNSUB);
    if (!rcflag(RC_NOTED))
	rcfset(RC_NOTED);
}

bool unsubscribe()
/* unsubscribe from the current group (if ADMSUB permits) */
{
    if (ngmatch(ngname(), ADMSUB))
	return(FALSE);
    else
    {
	rcfset(RC_DROPSUBSC);
	return(TRUE);
    }
}

void dontfollow(id)
/* unsubscribe from the identified discussion */
char	*id;
{
    senter(&discussions, id);
}

bool ckfollow(id)
/* see if we're unsubscribed from the identified discussion */
char	*id;
{
    return(sfind(&discussions, id));
}

int wrrcfile(fp)
/* write articles-read data to a file */
FILE	*fp;	/* pointer to write to */
{
    register group_t	*ngp;

    (void) dbadump(&comments, fp);	    /* write out saved comments */

    for (ngp = active.newsgroups; ngp < active.newsgroups + active.ngc; ngp++)
    {
	(void) fprintf(fp, "%s%c", ngp->ng_name,
	    (ngp->rc_flags & (RC_UNSUB|RC_DROPSUBSC)) ? UNSUBSCMK : SUBSCMK);
	(void) wrbits(ngp, bfr);
	(void) fputs(bfr, fp);
	(void) fputc('\n', fp);
    }

    /* write out unsubscribed discussions */
    (void) dbadump(&discussions, fp);

    return(SUCCEED);
}

void wrnewsrc()
/*
 * Write out the current user's .newsrc file (the path name is in newsrc[]).
 * Take care that data is all written, and flushed before trashing old copy.
 */
{
    FILE	    *wrcfp;
    char	    new_newsrc[BUFLEN];

    (void) strcpy(new_newsrc, newsrc);
    (void) strcat(new_newsrc, "-new");
#ifdef OTEST
    (void) fprintf(stderr, "wrnewsrc: writing to %s\n", bfr);
#endif /* TEST */

#ifdef VMS
    (void) vmsdelete(new_newsrc);
#endif

    /* here goes the actual I/O */
    wrcfp = xfopen(new_newsrc, "w");
    if (wrrcfile(wrcfp) != SUCCEED || fclose(wrcfp) < 0)
	xerror1("Error writing new .newsrc %s, no changes made\n", newsrc);

#ifdef DEBUG
    if (debug)
    {
	/* keep the old .newsrc around for debugging purposes */
	(void) strcpy(bfr, newsrc);
	(void) strcat(bfr, "-old");
	(void) rename(newsrc, bfr);
    }
#endif /* DEBUG */

#ifdef VMS
    (void) vmsdelete(newsrc);
#endif

    if (rename(new_newsrc, newsrc) < 0)
	xerror1("Cannot rename new .newsrc file to %s", newsrc);
}

#ifdef BIGGROUPS
#define F1	"%ld"
#define F2	",%ld"
#define F3	"-%ld"
#else
#define F1	"%d"
#define F2	",%d"
#define F3	"-%d"
#endif /* BIGGROUPS */

void wrbits(ngp, tp)
group_t	*ngp;	/* group data to represent bits from */
char	*tp;	/* buffer to write the representation to */
{
    register int    tbit, lbit = 0, intv = 0;
    register nart_t i, lo;

    *tp = '\0';

    /* must do this because the loop below always runs at least once */
    if (ngp->ng_max >= ngp->ng_min)
    {
	/* we're deliberately running the index 1 past the map end */
	for (i = ngp->ng_min; i <= ngp->ng_max + 1; i++)
	{
	    /* this will be FAIL on ngp->ng_max + 1 */
	    tbit = getbit(i, ngp);
#ifdef GTEST
	    (void) fprintf(stderr,
		"wrbits: bits= %d, index= %d, tbit= %d, lbit= %d\n",
		ngp->ng_bits[0] & 0xff, i, tbit, lbit);
#endif /* TEST */

	    if (tbit != lbit)
	    {
		if (tbit == TRUE && intv++ == 0)
		    (void) sprintf(tp, F1, (lo = i));
		else if (tbit == TRUE)
		    (void) sprintf(tp, F2, (lo = i));
		else if (i > lo + 1 && (tbit == FALSE || lbit))
		    (void) sprintf(tp, F3, i - 1);
		tp += strlen(tp);
	    }
	    lbit = tbit;
	}
    }
}
#undef F1
#undef F2
#undef F3

/* wrnewsrc.c ends here */
