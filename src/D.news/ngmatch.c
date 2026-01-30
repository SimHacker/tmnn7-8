/*****************************************************************************

NAME
   ngmatch -- news group matching

SYNOPSIS
   bool ngmatch(nglist, sublist)	-- match groups against subscriptions
   register char *nglist, *sublist;

DESCRIPTION
   If nglist is a list of newsgroups, and sublist is a list of subscriptions,
then ngmatch(nglist, sublist) will return TRUE if some one of the groups in
nglist matches a subscription in sublist. The sublist argument may have
wildcards in it or be prefixed by a ! (see below). Braces { } may be used
to enclose a list of alternative suffixes. All list elements are NGDELIM
separated.

   Group names consist of segments separated by NGSEP characters. In comparing
them, segments are matched left to right until something is not equal or
ngmatch() runs out of string on one or the other. Thus, if subscription X
matches group G, it also matches G.anything groups (but empty strings don't
match anything). The following wildcards are supported:

	any  -- matches any segment
	all  -- matches any sequence of segments

Thus 'all.ctl' matches any group ending in 'ctl' but 'any.ctl' matches only
groups of the form 'foo.ctl'.

   A subscription part prefixed by ! is an exclusion. If a newsgroup matches
a !-subscription the return of ngmatch() will be FALSE unless the group
matches some later subscription. Thus for example

    comp.unix.sources,!comp.unix.sources.all

will subscribe you to comp.unix.sources but exclude comp.unix.sources.d and
comp.unix.sources.wanted.

   A component list enclosed by { } is a list of alternative suffixes. For
example

    comp.sources.{unix,mac} == comp.sources.unix,comp.sources.mac

The negation char distributes over alternation. Thus

    !talk.{religion,politics} == !talk.politics,!talk.religion

The {} construct is implemented by recursion, so it will even do reasonable
things with nested braces and ! inside braces. Be warned that bad things will
happen if your braces are not balanced!

    Embedded whitespace (including <NL>) and backslashes are both ignored.
Thus, you can format a subscription string with tabs and \<NL> digraphs
and it will still be interpreted properly.

NOTE
   The 'all' prefix used to match segments only. The change was deliberate,
for 2 reasons: 1) if you used to subscribe to all.foo you probably want
all.foo in the new, more deeply branched newsgroup hierarchy, and 2) the
connotations of 'any' and 'all' in English fit this scheme better. Due to
the rule that 'X matches G <=> X matches G.anything', foo.any and foo.all
are equivalent.

REVISED BY
   Eric S. Raymond
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

*****************************************************************************/
/* LINTLIBRARY */

#include "news.h"

#define OPEN	    '{'	    /* start bracket for news subscriptions */
#define CLOSE	    '}'	    /* end bracket for news subscriptions */

#define END_GROUP(c)	(c == '\0' || c == NGDELIM)
#define END_SEGMENT(c)	(c == '\0' || c == NGDELIM || c == NGSEP)
#define SKIP_SEGMENT(n)	while (!END_SEGMENT(*n)) n++

bool ngmatch(nglist, sublist)
/* compare a list of newsgroups against a list of subscriptions */
register char *nglist, *sublist;
{
    register char  *n, *s, ch;
    register bool   rc;
    forward static bool ngcmp();

#ifdef PARANOID
    if (!nglist || !sublist)
	return(FALSE);
#endif /* PARANOID */

    rc = FALSE;
    for (n = nglist; *n != '\0' && rc == FALSE;)
    {
	if (isspace(*n) || *n == '\\')
	{
	    n++;
	    continue;
	}

	for (s = sublist; *s != '\0' && *s != CLOSE;)
	{
	    register int    bdepth = 0;

	    if (isspace(*s) || *s == '\\')
	    {
		s++;
		continue;
	    }

	    /* match current group against current subscription part */
	    if (*s != NEGCHAR)
		rc = rc || ngcmp(n, s);
	    else
		rc = rc && !ngcmp(n, s + 1);

	    /* skip to next subscription component */
	    while (((ch = *s++) != NGDELIM || bdepth > 0) && *s != '\0')
		if (ch == OPEN)
		    bdepth++;
		else if (ch == CLOSE)
		    bdepth--;
	}

	/* skip to next NGDELIM-separated token in nglist */
	while (*n++ != NGDELIM && *n != '\0')
	    continue;
    }
    return rc;
}

private bool ngcmp(group, subsc)
/*
 * Compare two newsgroups for equality. The second one may be a "meta"
 * newsgroup, i.e. a subscription including 'all' and 'any' wildcards.
 * The code is optimized for speed.
 */
register char *group, *subsc;
{
    while (!END_GROUP(*subsc) && *subsc != CLOSE)
    {
	if (subsc[0] == 'a' )
	{
	    if (subsc[1] == 'l' && subsc[2] == 'l' && END_SEGMENT(subsc[3]))
	    {
		/*
		 * skip current subscription segment, then see if the subsc is
		 * matched by any tail of the remainder of the group name
		 */
		subsc += 3;	/* strlen("all") */
		while (!END_GROUP(*group))
		    if (ngcmp(group++, subsc))
			return(TRUE);
		return(ngcmp(group, subsc));
	    }
	    else if (subsc[1]=='n' && subsc[2]=='y' && END_SEGMENT(subsc[3]))
	    {
		/* skip current subscription segment, and one group segment */
		subsc += 3;	/* strlen("any") */
		SKIP_SEGMENT(group);
		continue;
	    }
	}

	if (isspace(*subsc) || *subsc == '\\')
	    subsc++;
	else if (*subsc == OPEN)
	    return(ngmatch(group, ++subsc));	/* the ++ is the key! */
	else if (*subsc++ != *group++)
	    return(FALSE);
    }

    /*
     * If I'm ever free to nuke the `foo matches foo.bar' misfeature, this
     * will become END_GROUP(*group)
     */
    return(END_SEGMENT(*group));
}

/* ngmatch.c ends here */
