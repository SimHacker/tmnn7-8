/***************************************************************************

NAME
   ngprep.c - header-interpretation functions used by posting code

SYNOPSIS
   #include "ngprep.h"

   void ngprepinit()	-- set up tables for compilation

   void ngprepare()	-- compile newsgroup/distribution info

   int ngaccept(sys)	-- tell whether to transmit current article to system
   feed_t *sys;

DESCRIPTION
   The ngprepare() function compiles the newsgroup and distribution lines
of the current header into an internal data structure more easily used by
other routines. Aliasing and bug-group rerouting are done here.

   This code may mung the current header information in two ways. The special
name 'control' may be munged into the header's newsgroups line if it is not
already present and the article addressed to that group in the older form.
Also, if the header's ID and distribution fields are empty (i.e. the article
originated at this site and no distribution was given for it), default
distributions for each group are written into it here.

   The ngaccept() function tells if the article attached to the current
header will be accepted on sys. Some optimizations of this expensive check
are enabled by FEEDBITS and CACHEBITS. See also feedbits.c

NOTE
   The LEAFNODE code is experimental and should be ignored for the moment.
The FEEDBITS code is broken.

FILES
    ADM/aliases       -- news group aliases file
    ADM/buggroups     -- bug groups mapping file
    ADM/distributions -- valid distributions list

AUTHOR
   Eric S. Raymond
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

*****************************************************************************/
/*LINTLIBRARY*/
#include "news.h"
#include "libpriv.h"
#include "header.h"
#include "dballoc.h"
#include "alist.h"
#include "feeds.h"
#include "active.h"
#include "ngprep.h"
#ifdef FEEDBITS
#include "bitmacros.h"
#endif /* FEEDBITS */
#ifdef LEAFNODE
#include "newsrc.h"
#endif /* LEAFNODE */

#undef FEEDBITS

ALIST(distlist, 20, 10)	    /* hold the distributions list */
ALIST(aliases, 20, 10)	    /* hold the newsgroup aliases list */
ALIST(buggroups, 20, 10)    /* hold the bug-group aliases list */
ALIST(pathlist, 2,  2)	    /* hold the moderator/backbone site list */

dest_t	destinations[CROSSPOSTS];

private char	distspace[BUFLEN], *distpt = distspace;

/*
 * The following hackery intends to minimize the number of those expensive
 * ngfind() operations that need to be performed while posting an article
 * The ngprepare() function below compiles the current header into an internal
 * form that points directly at the status info of destination groups.
 * The dest_t type and associated declarations define the internal form of a
 * Newsgroups line, as an array of dest_t objects. The array ends at the first
 * NULL grp slot, all other grp slots will contain valid group_t pointers. The
 * dist slots will be char pointers at the names of distribution prefixes;
 * if a group had no distribution prefix the slot will contain NULL.
 */

void ngprepinit()
/* set up lookup tables for newsgroup compilation */
{
    char	*p;

    /* ensure that mailpaths info is available */
    (void) sprintf(bfr, "%s/mailpaths", site.admdir);
    if (exists(bfr))
    {
    	pathlist.file = savestr(bfr);
    	(void) dbaread(&pathlist);
    }

    /* make sure the aliases list is in core */
    (void) sprintf(bfr, "%s/aliases", site.admdir);
    aliases.file = savestr(bfr);
    (void) dbaread(&aliases);

    /* make sure the bug groups list is in core */
    (void) sprintf(bfr, "%s/buggroups", site.admdir);
    buggroups.file = savestr(bfr);
    (void) dbaread(&buggroups);

    /* make sure the distributions list is in core */
    if (distlist.file == (char *)NULL)
    {
	(void) sprintf(bfr, "%s/%s", site.admdir, "distributions");
	distlist.file = savestr(bfr);
	(void) dbaread(&distlist);
    }

    /* find our path to the backbone (if we have one) */
    p = strchr(newsattr("backbone", BACKBONE), '%');
    if (p != (char*)NULL && p[1] != 's')
	xerror0("backbone address corrupted");
}

void ngprepare()
{
    register char *ptr;
    char	grplist[BUFLEN], defdist[BUFLEN];
    dest_t	*dest;
    bool	nodist;

    /*
     * Undo any allocations hanging out from a previous ngprepare().
     * Because D_NOMORE is 0, this will do nothing on the first call.
     */
    for (dest = destinations; dest->d_status != D_NOMORE; dest++)
    {
	if (dest->d_status == D_UNKNOWN && dest->d_name != (char*)NULL)
	{
	    free(dest->d_name);
	    dest->d_name = (char*)NULL;
	}
	dest->d_ptr = (group_t *) NULL;
	dest->d_status = D_NOMORE;
    }
    dest = destinations;

    /* replace the obsolete 'net' distribution with 'world' */
    if (hlblank(header.h_distribution) || !strcmp(header.h_distribution,"net"))
	hlcpy(header.h_distribution, "world");
    nodist = hlblank(header.h_distribution) && hlblank(header.h_ident);

    /*
     * Control messages are special. The second form is for upward
     * compatibility only and is not considered standard. We mung the
     * header here to bring it to RFC1036 conformance.
     */
    if (hlnblank(header.h_ctlmsg) || ngmatch(header.h_newsgroups, "all.ctl"))
	hlcpy(header.h_newsgroups, "control");

    /* now run through the munged list, checking for validity */
    (void) strcpy(grplist, header.h_newsgroups);
    defdist[0] = '\0';
    ptr = strtok(grplist, LISTSEP);
    do {
	char	*nextptr, *aliased, *buggrp;

	dest->d_ptr = (group_t *)NULL;
	dest->d_dist = (char *)NULL;

	/* here's where we check for normal groups */
	if (dest->d_ptr = ngfind(ptr))
	{
	    ngselect(dest->d_ptr);
	    dest->d_status = D_OK;
	}

	/* It's not in our active file, see if it's in our alias list */
	else if (aliased = afind(&aliases, ptr))
	{
#ifdef DEBUG
	    logerr2("Aliased newsgroup '%s' to '%s'", ptr, aliased);
#endif				/* DEBUG */
	    dest->d_ptr = ngfind(aliased);
	    dest->d_status = D_OK;
	}

	/* It's not a normal alias, see if it's in our bug groups list */
	else if (buggrp = amatch(&buggroups, ptr, ngmatch))
	{
#ifdef DEBUG
	    logerr2("Aliased bug group '%s' to '%s'", ptr, buggrp);
#endif				/* DEBUG */
	    dest->d_ptr = ngfind(buggrp);
	    dest->d_status = D_OK;
	}

	/* not a bug group either, does it have a known distribution prefix? */
	else if (amatch(&distlist, ptr, ngmatch) != (char *)NULL)
	{
	    /*
	     * Prefix known, stash it in distlist with dest->d_dist pointed at
	     * it. Note that we never get to this case if we're looking at a
	     * valid newsgroup name. Thus, to avoid having (say) ca.general
	     * mapped into your local general group, create a 'ca.general'
	     * entry in your active file.
	     */
	    nextptr = strchr(ptr, NGSEP);
	    *nextptr++ = '\0';
	    (void) strcpy(dest->d_dist = distpt, ptr);
	    distpt += strlen(ptr) + 1;

	    /*
	     * Someday, rewrite this so it recurses after finding a
	     * valid distribution prefixes, so that aliases can be used.
	     */
	    if ((dest->d_ptr = ngfind(ptr = nextptr)) != (group_t *)NULL)
		dest->d_status = D_OK;
	    else
	    {
		dest->d_status = D_UNKNOWN;
		dest->d_name = savestr(ptr);
	    }
	}

	else	/* group is invalid, save its name for error handling code */
	{
#ifndef B211COMPAT
	    char	*p;

	    /*
	     * This is where the multiple junk group feature is implemented.
	     * 
	     */
	    (void) strcpy(bfr, ptr);
	    dest->d_ptr = (group_t *)NULL;
	    while (p = strrchr(bfr, NGSEP))
	    {
		(void) strcpy(p, JUNKSUF);
		if ((dest->d_ptr = ngfind(bfr)) != (group_t *)NULL)
		    break;
		*p = '\0';
	    }

	    if (dest->d_ptr != (group_t *)NULL)
		dest->d_status = D_OK;
	    else
#endif /* B211COMPAT */
	    {
		dest->d_status = D_UNKNOWN;
		dest->d_name = savestr(ptr);
	    }
	}

	/* we may need to know if this has a moderator */
	if (pathlist.file[0])
	    dest->d_moderator = afind(&pathlist, ngname());

	/* add each group's default distribution if there isn't any given */
	if (nodist && dest->d_ptr->ng_defdist != (char *)NULL)
	    (void) setadd(defdist, dest->d_ptr->ng_defdist, ",");

	/* only go to next slot if we used this one */
	if (dest->d_ptr != (group_t *)NULL)
	    dest++;
    } while
	(ptr = strtok((char *)NULL, LISTSEP));

    if (nodist)
	hlcpy(header.h_distribution, defdist);
    dest->d_status = D_NOMORE;
}

int ngaccept(target)
/* return info on acceptability of current article to given system */
feed_t	*target;    /* against this feed description */
{
    dest_t  *dst;

    /* if there's a specified distribution and we don't match, fail */
    if (hlnblank(header.h_distribution)
		&& !ngmatch(header.h_distribution, target->s_distribs)
		&& !ngmatch(header.h_distribution, target->s_ngroups))
	return(A_DISTNG);
    else for (dst = destinations; dst->d_status != D_NOMORE; dst++)
    {
	bool	pref;

	/* ignore newsgroups prefixed with rejected distributions */
	if (dst->d_dist && !(pref = ngmatch(dst->d_dist,target->s_distribs)))
	    continue;

	/* unknown newsgroups may still be OK for retransmission */
	if (dst->d_status == D_UNKNOWN)
	{
	    if (ngmatch(dst->d_name, target->s_ngroups))
		return(A_ACCEPT);
	    continue;
	}

	/* assert(dst->d_status == D_OK); */

	/*
	 * now check to see if any of the [dist.]newsgroup destinations in
	 * the Newsgroups line is OK
	 */
	ngselect(dst->d_ptr);

#ifdef LEAFNODE
	/*
	 * If we're a leaf node, the right test is what people subscribe to.
	 * It would be better to change this site's subscription at each
	 * feed, but this is more flexible.
	 */
	if (rcflag(RC_HASSUBS))
	    return(A_ACCEPT);
#else
#ifdef FEEDBITS
	/* if this group's bitmap was ever compiled, use the bits */
	if (ngfeed(ngactive(), 0))
 	{
	    if (ngfeed(ngactive(), s_tell(target) + 1))
		return(A_ACCEPT);
	}
	else
#endif /* FEEDBITS */
	    if (ngmatch(ngname(), target->s_ngroups))
		return(A_ACCEPT);
#endif /* LEAFNODE */

	/* ensure that we pass things like 'to.foomach' and 'nj.forsale' */
	if (dst->d_dist && pref)
	    return(A_JUNKIT);
    }

    /* none of the groups matched, forget it */
    return(A_GRPSNG);
}

/* ngprep.c ends here */
