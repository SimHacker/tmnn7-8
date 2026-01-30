/****************************************************************************

NAME
   rdnewsrc.c -- functions for reading .newsrc files

SYNOPSIS
   #include "active.h"
   #include "newsrc.h"

   int rdnewsrc(newsrc)		-- load .newsrc info from given file
   char *newsrc;

DESCRIPTION
   These functions provide the read side of a clean interface to the .newsrc
files described in newsrc(5) (the write side lives in wrnewsrc.c). They assume
that rdactive() has been called to initialize the in-core data structure that
holds group data.

   Typically, the caller will perform one or more calls to the rdnewsrc()
function. This function merges information from a given .newsrc file
into the ng_bits parts of the newsgroups array.

FILES
   ~/.newsrc		-- per-user info on which articles have been read
   ADM/authorized	-- user may be locked out of subscribing to some groups

FILE FORMAT
   See news(5). The rdnewsrc() function ignores options, comments, group
command lines, and unsubscribed-discussion IDs.

SEE ALSO
   rdbits.c		-- functions to examine and set seen bits
   wrnewsrc.c	-- full .newsrc interpretation for use with readers.

AUTHOR
   Eric S. Raymond
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

**************************************************************************/
/*LINTLIBRARY*/
#include "news.h"
#include "active.h"
#include "newsrc.h"
#ifdef COMMUNIST
#include "fascist.h"
#endif /* COMMUNIST */

private int	rcreadok = 0;	/* count of .newsrc files read in */

int rdnewsrc(newsrc)
/*
 * This function reads a .newsrc-format file, and logical-ands
 * its already-read information into the calloc'd area created
 * by a rdactive(). For each unread article a zero is anded into
 * the corresponding place in the bitmap. If no rdactive() or
 * rdactfile() has been done a rdactive() is forced. If FAIL is
 * returned, errno will be set to EINVAL (for bad format) if the
 * problem was a format error, otherwise the errno value will be
 * whatever condition caused fopen(3) to fail.
 */
char	*newsrc;
{
    register group_t *ngp;
    char	    subsc, *cp;
    char	    rcline[BUFLEN];
    FILE	    *fp;
#ifdef COMMUNIST
    nasty_t	    *restrict = fascist(username);
#endif /* COMMUNIST */

    if (active.newsgroups == (group_t *)NULL)
	(void) rdactive(NULLPRED);

    /* barf if the .newsrc file is ill-formed */
    if ((fp = fopen(newsrc, "r")) == (FILE *)NULL)
	return(FAIL);	/* errno set by fopen() */

    /* loop through the .newsrc file lines, interpreting them */
    while (fgets(rcline, sizeof(rcline), fp))
    {
	(void) nstrip(rcline);

	/* reject lines we can tell aren't newsgroup lines */
	if (!isalpha(rcline[0]) || !strncmp(rcline, "options", 7))
	    continue;

#ifdef MACROS
	/* and reject macro definition lines */
	if (!strncmp(rcline, "macro", 5))
	    continue;
#endif /* MACROS */

	/* check that the format is O.K for a group line */
	if ((cp = strchr(rcline, SUBSCMK)) || (cp = strchr(rcline, UNSUBSCMK)))
	{
	    subsc = *cp;
	    *cp++ = 0;
	}
	else
	{
	    errno = EINVAL;
	    return(FAIL);
	}

	/*
	 * the special group name 'all' exists so we can hang global
	 * initial commands off it
	 */
	if (strcmp(rcline, "all") == 0)
	    continue;

#ifdef CRACKMAIL
	/*
	 * mail.* groups declare mailboxes to be cracked.
	 */
	if (strncmp(rcline, "mail.", 5) == 0)
	    continue;
#endif /* CRACKMAIL */

	/* ignore groups not in the in-core group list */
	if (ngp = ngfind(rcline))
	{
	    ngp->rc_flags |= RC_NOTED;
	    if (subsc == UNSUBSCMK)
		ngp->rc_flags |= RC_UNSUB;
	    else
		ngp->rc_flags |= RC_HASSUBS;
#ifdef COMMUNIST
	    if (!ngmatch(ngp->ng_name, restrict->n_read))
		ngp->rc_flags |= RC_UNSUB;
#endif /* COMMUNIST */
	}
	else
	    continue;

	while (isspace(*cp))
	    cp++;
    
	/*
	 * If there's nothing in the articles-read part of the input line,
	 * skip the bit map construction that follows. This means that
	 * ng_bits fields get left NULL until we see real info for them.
	 * Also skip the construction for any group not carrying a
	 * CHANGED mark; all groups have this after the first rdactive(),
	 * some groups may have it after subsequent rdactive() calls.
	 *
	 * The fun and games with rcreadok mean that on the second through
	 * nth .newsrc reads, unsubscribed groups won't affect the map at
	 * all and (in subscribed groups) only bit clears will get done.
	 */
	if (*cp &&
		(ngp->ng_flags & NG_CHANGED) &&
		!(rcreadok && (ngp->rc_flags & RC_UNSUB)) &&
		rdbits(rcreadok ? CLEAR : SET, cp, ngp))
	{
	    errno = EINVAL;
	    return(FAIL);
	}

	ngp->ng_flags &=~ NG_CHANGED;	/* read bits are now correct */
    }

    active.article.m_group = active.newsgroups;
    rcreadok++;
    return(SUCCEED);
}

/* rdnewsrc.c ends here */
