/****************************************************************************

NAME
   rdactive.c -- functions for reading and manipulating the active file

SYNOPSIS
   #include "active.h"

   int rdactive(bproc)		-- load active info on newsgroups
   bool (*bproc)();

   void ngrewind(f);		-- reset the current-group pointer
   bool f;

   bool ngnext();		-- move to the next group

   bool ngback();		-- move to the last group

   group_t *ngfind(name)	-- return ptr to given group's data
   char name;

   bool ngreread(ngp)		-- update data for given group from disk
   group_t *ngp;

   void clsactive()		-- release the active file

   void ngrehash()		-- regenerate group hash lists

DESCRIPTION
   These functions provide a clean interface to the active-groups file used
by the USENET software. See wractive.c for companion functions usable for
modifying the active file.

   Use of these functions must begin with a call to rdactive(). This
reads data on active newsgroups into a malloc'd in-core array of group_t
structures. The rdactive() function may be called more than once.
Each time after the first updates the active file array to reflect new
postings without trashing the old data. It returns a count of the number
of groups that have changed status.

   If the argument of rdactive() is non-null it is assumed to be a hook
function to be called on each active group when updating it. The hook function
will be passed two group_t address arguments; they point to the group's new
data and its old data respectively. User hooks may assume that the .newsrc 
parts of the new structure are zero and copy or edit into it from the old
data. This feature allows user programs to do their own application-dependent
update of .newsrc data each time active-group data is updated.

   The rdactive() function leaves a file pointer to the active file in
the active.fp slot of the active global. This can be used for on-the-fly
reads and updates of the on-disk file; it will be opened for update if
permissions permit, read-only otherwise.

    Group data access functions return a pointer to the group_t structure that
describes a group. The user is warned that setting any of the members
of this structure is an invitation to disaster. This module supports access
that is sequential (stepping through the groups in active or .newsrc file
order) or random (by name). After any of these functions. the current-group
pointer is available as the value of ngactive().

   The functions ngnext() or ngback() are available to do sequential access.
Call ngrewind() first with the appropriate direction flag argument to set
things' up so the first ngnext() or ngback() will go to the first group.

   Access by name is through the ngfind() function. This function returns
a pointer to the data on the selected group. It does not set the current-group
pointer returned by ngactive() to its value; to accomplish that call the
ngselect() macro on the return value of ngfind(). Due to an optimization in
the search method ngfind() will blow up if called after ngrewind() but
before the first ngnext() or ngback(); a call to the macro ngreset() will
prevent this.

   The ngreread() function calls for an update of the data on a given group
from the on-disk information.  [If the group pointer argument is NULL, it
allocates a new group slot at the end of the array.  ---- No longer true!]

   All per-reader active-group information is contained in the structure
'active' declared in active.h. Someday active may become a pointer to data
contained in one of multiple execution threads.

THE MACRO INTERFACE
   Some macros are defined in active.h that define pseudo-functional handles
on group information; they are all 'safe' (i.e. can be called with arguments
that have side-effects) and pretty much self-explanatory. The advantage of
using this layer is that it invisibly maintains the current-group pointer
active.article.m_group.

ngname()	-- return pointer to current group's name
ngmin()		-- return lowest active article number
ngmax()		-- return highest active article number
ngexpire()	-- return the group's expire field
ngnew()		-- TRUE if group is newer than last read session else FALSE

It is expected that most users will call these macros rather than the
underlying functions, so that active.article.m_group doesn't need to be
manipulated explicitly.

   See news(5) for details of the active file format

NOTE
   If you have NONLOCAL defined (i.e. are running your readers as clients
of a network server) this code expects to be able to call a network service
library entry point getactive() that returns the name of a temp file
containing a copy of the master machine's active file.


FILES
   ADM/active	-- active group information
   ADM/feedbits	-- bit masks compiled from feed file data

NOTE
   The old code sometimes used longs for article numbers. This seemed a
little excessive, so I have recast everything in terms of an 'nart_t'
typedef defined in news.h. If BIGGROUPS is defined in defs.h longs will be
used throughout (pains are taken to ensure that *scanf() and *printf()
formats do the right thing). Otherwise ints will be used.

AUTHOR
   Eric S. Raymond
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

**************************************************************************/
/*LINTLIBRARY*/
#include "news.h"
#include "active.h"

/* tweak these constants to tune the allocation for in-core structures */
#define GRPCINIT    350		    /* initial size of group_t array */
#define GRPCGRAIN   50		    /* group_t allocation chunk size */
#define DSCINIT	    20		    /* initial size of discussions array */
#define DSCGRAIN    10		    /* discussions allocation chunk size */
#define COMCINIT    20		    /* initial size of comments array */
#define COMCGRAIN   10		    /* comments allocation chunk size */

/*
 * This module manages a calloc'd area of active and subscriber information
 * accessed as the array newsgroups[] of group_t structures (see active.h).
 * The rdactive(), wractive(), rdnewsrc() and wrnewsrc() functions import
 * and export this information to/from the file system.
 */
/* other modules need to see these */
char	*ACTIVE;	/* the active file */
active_t	active;	/* session active-groups array information */
int	rdactcount = 0;	/* nz if active file has been read */

private char	*grpnames;	/* pool space for group names */
private char	*np;		/* pool pointer for group names */

#ifdef HASHGROUPS
/*
 * Set up hash access for newsgroup names, see the code in rdactive() and
 * ngfind() below. This code has been dinked to a state of uncomfortable
 * trickiness because ngfind() is such a hot spot, beware of it and don't
 * mess with it unless you're sure you know what you're doing.
 *
 * The basic idea is to hash the newsgroup name to a bucket index; each
 * bucket contains the head of a short linked list which we can grovel
 * through quickly. We win if the hash function overhead plus the traverse
 * and strcmp() time for the average list is less than the linear search time
 * would be. With any reasonable number of buckets allocated this will nearly
 * always be true.
 *
 * We use the CRC hash from the port library because it's handy and
 * cheap (about 6 instructions per character). The edbm hash might give
 * better distribution, but is more expensive to compute; and collisions
 * don't cost as much here.
 *
 * Memory can be traded for speed by boosting the value of MAXBUCKETS.
 * Good values are prime numbers greater than your maximum expected number
 * of newsgroups; larger ones tends to make all chains short and search
 * time approach O(1). Allow some slop for the fact that the hash function
 * isn't perfect.
 *
 * The purpose of the for_hash() macro is to make it easy to change the hash
 * function and to hide the details of the hash list traversal.
 *
 * Note that the macro's first arg must be of type (group_t **), pointer to
 * pointer to group, and must be dereferenced once to get at actual group data.
 *
 * Warning: the for_hash() macro will fail hideously if the newsgroup record
 * allocator fails to zero out the link index field, or if the compiler fails
 * to initially zero out the buckets[] array.
 *
 * Profiling shows that a MAXBUCKETS value of 2007 (about an 8K bucket list)
 * is good for about a 350% speed increase over the linear-search version. At
 * c.330 groups there are only a handful of collisions, so it's probably not
 * worth going to higher primes yet.
 */
#define MAXBUCKETS	2007
static int buckets[MAXBUCKETS];
#define for_hash(b, s)	for (b = &buckets[checkstring(s,(ulong)0L) % MAXBUCKETS]; *b; b = &(active.newsgroups[*b].ng_nextg))
#endif /* HASHGROUPS */

private int ngread(buf, ngp)
/* read a single group record from the current file into a group_t */
char	    *buf;
group_t	    *ngp;
{
    int		rstat;
    char	flagfld[10];

    ngp->ng_age = NO_AGE;

    /*
     * WARNING: despite the size-independent appearance of this code,
     * the output format used in wractive.c:ngshow() means we are likely
     * to lose massively on a machine with sizeof(long) < 4
     */
    rstat = sscanf(buf,
#ifdef BIGGROUPS
		"%ld %ld %s %lx",
#else
		"%d %d %s %lx",
#endif /* BIGGROUPS */
		&(ngp->ng_max), &(ngp->ng_min),
		flagfld, &(ngp->ng_age));
    if (rstat != 3 && rstat != 4)
	xerror2("Active file is corrupt, status = %d, line = %s", rstat, bfr);

    /* set all status flags that we can deduce from the active file entry */
    ngp->ng_flags = (bits_t)0;
    if (strchr(flagfld, 'm'))
	ngp->ng_flags |= NG_MODERATED;
    if (!strchr(ngp->ng_name, NGSEP))
	ngp->ng_flags |= NG_LOCAL;

    ngp->ng_expire = (time_t)0;

    /*
     * The following lines are just here to serve as
     * insurance against realloc() functions that don't zero things
     */
#ifdef HASHGROUPS
    ngp->ng_nextg = 0;
#endif /* HASHGROUPS */
#ifdef FEEDBITS
    /* following information valid only after a feedbits() */
    ngp->ng_feeds = (bits_t)0;	/* subscription bits for news feeds */
#endif	/* FEEDBITS */
    ngp->rc_flags = (bits_t)0;
    ngp->rc_seen = (uchar *)NULL;

    /* this has to be done here, because rdnewsrc() won't see all groups */
    ngp->ng_unread = (ngp->ng_max - ngp->ng_min) + 1;

    return(SUCCEED);
}

#ifndef HASHGROUPS
/*ARGSUSED0*/
#endif /* HASHGROUPS */
group_t *ngalloc(name)
/* return pointer to allocated space for new group record */
register char	*name;
{
    static group_t	*ngp = (group_t *)NULL;	/* next-free-slot pointer */
    static int		gc = 0;			/* number of slots allocated */
#ifdef HASHGROUPS
    register int	*bucket;
#endif /* HASHGROUPS */

    /* allocate initial space for data on newsgroups */
#ifndef lint
    if (gc == 0)
	ngp = active.newsgroups =
	    (group_t *) calloc((unsigned)(gc = GRPCINIT),
			       (unsigned) sizeof(group_t));
#endif /* lint */

    /* and alloc new slots for groups if we've run out of space */
    if (ngp >= active.newsgroups + gc)
    {
	int i = ngp - active.newsgroups;

	gc += GRPCGRAIN;
#ifndef lint
	active.newsgroups =
	    (group_t *) realloc((char *) active.newsgroups,
				(unsigned) (gc * sizeof(group_t)));
#endif /* lint */
	ngp = active.newsgroups + i;
    }

#ifdef HASHGROUPS
    /*
     * Now cons the new record to the end of its hash chain.
     */
    for_hash(bucket, name)
	continue;
    bucket[0] = ngp - active.newsgroups + 1;
#endif /* HASHGROUPS */

    ngp++;
    active.ngc++;
    return(ngp - 1);
}

int rdactive(bproc)
/* this function reads or updates the active file into the newsgroups array */
bool	(*bproc)();	/* bitmap-processing hook */
{
    int		changed;
    group_t	hold, *newgrp;
    static long	loc = 0;
    register char *cp;
    char	*oldnames;
#ifndef NONUNIX
    struct stat	stbuf;
#endif /* NONUNIX */

#ifdef NONLOCAL
    if (active.fp != (FILE *)NULL)
	clsactive();
    ACTIVE = getactive();
#else
    if (ACTIVE == (char *)NULL)
	Sprint1(ACTIVE, "%s/active", site.admdir);
#endif /* NONLOCAL */

    /* open for read-write if we can, for read-only if we must */
    if ((active.fp = fopen(ACTIVE, "r+")) != (FILE *)NULL)
    {
#ifdef USG
	/*
	 * AT&T broke standard IO in System 5.2. This hack works
	 * OK on all "real" USG Unix systems, but will probably
	 * break on some obscure lookalike that doesn't use the
	 * real ATT stdio.h. The stdio library should have
	 * already done the following line for us, but it didn't.
	 */
	active.fp->_flag |= _IOWRT;
#endif /* USG */
	/*
	 * Without the following hackery data written to an r+ fp often
	 * gets lost even if you fseek() before the write as the manuals
	 * advise.
	 *
	 * This is known to fix the bug where it crops up in stdio(3)
	 * on the AT&T 6300+ and Microport 80286 UNIX implementations
	 * It should also do the right thing on 4.1BSD. USG systems in
	 * general will be covered by the ngnewart() change.
	 */
	(void) setbuf(active.fp, (char *)NULL);
    }
    else if ((active.fp = fopen(ACTIVE, "r")) == (FILE *)NULL)
	xerror1("what? -- can't get at active file at %s!", ACTIVE);

    /* allocate name pool space based on active file size */
    oldnames = grpnames;
#ifndef NONUNIX
    (void) fstat(fileno(active.fp), &stbuf);	/* avoid namei() overhead */
    np = grpnames = malloc((iolen_t)stbuf.st_size);
#else
    np = grpnames = malloc((iolen_t)filesize(ACTIVE));
#endif /* NONUNIX */

      /* now gather all that data */
    for (changed = 0; fgets(bfr, sizeof(bfr), active.fp) != (char *)NULL; )
    {
	register group_t	*mgp;

	/* store the group name */
	hold.ng_name = np;
        hold.rc_lindex = np - grpnames;	/* so we can realloc later */
	for (cp = bfr; *cp != ' ';)
	    *np++ = *cp++;
	*np++ = '\0';

	/* get seek cookie from before the read plus offset to minart field */
	hold.ng_fseek = loc + (cp - bfr + 1);
	loc = ftell(active.fp);	/* now update it for the next time around */

	/* decode the rest of the current line into the scratch area */
	if (ngread(cp + 1, &hold) == EOF)
  	    break;
  
	/* if there's already in-core data for the group, update it */
	if (rdactcount && (mgp = ngfind(hold.ng_name)))
	{
	    /* let's make sure we don't lose admin file info */
	    hold.ng_flags |= (mgp->ng_flags & NG_ADMFLAGS);

#ifdef DOXREFS
	    /* preserve next article number */
	    hold.ng_nextnum = mgp->ng_nextnum;
#endif
#ifdef HASHGROUPS
	    /* preserve hash link number */
	    hold.ng_nextg = mgp->ng_nextg;
#endif
	    /* it's up to the user to preserve other flag info if need be */
	    if (bproc != NULLPRED)
		if ((*bproc)(&hold, mgp))	/* tweak bitmaps if needed */
		{
		    hold.ng_flags |= NG_CHANGED;
		    changed++;
		}
	
	    /* copy the updated information to the group slot */
	    (void) memcpy((char *)mgp, (char *)&hold, sizeof(group_t));
	}
	else	/* this is the first time we've seen this group */
	{
	    /* so mark it changed */
	    hold.ng_flags |= NG_CHANGED;
	    changed++;
	    
	    /* allocate space for it in the array */
	    newgrp = ngalloc(hold.ng_name);

	    /* copy in new information and update the last-record pointer */
	    (void) memcpy((char *)newgrp, (char *)&hold, sizeof(group_t));
	}
    }

    /* O.K., now reclaim the unused portion of the name pool */
    grpnames = realloc(grpnames, (iolen_t)(np - grpnames));
    for (newgrp = active.newsgroups; newgrp < active.newsgroups + active.ngc; newgrp++)
	newgrp->ng_name = grpnames + newgrp->rc_lindex;
    if (rdactcount)
	free(oldnames);

    active.article.m_group = active.newsgroups;
    rdactcount++;
    return(changed);	
}

void ngrewind(f)
/* set active.article.m_group so the next ngnext() will start it on the right group */
bool	f;	/* TRUE to set up for steps forward, FALSE for back */
{
    active.article.m_group = (f)
	? (active.newsgroups - 1)
	    : (active.newsgroups + active.ngc);
}

bool ngnext()
/* move to the next group, wrapping around if we're at the last */
{
    if (++active.article.m_group < active.newsgroups + active.ngc)
	return(TRUE);
    else
    {
	active.article.m_group = active.newsgroups;
	return(FALSE);
    }
}

bool ngback()
/* move to the last group, wrapping around if we're at the first */
{
    if (--active.article.m_group >= active.newsgroups)
	return(TRUE);
    else
    {
	active.article.m_group = active.newsgroups + active.ngc - 1;
	return(FALSE);
    }
}

group_t *ngfind(name)
/* find in-core data on a newsgroup */
register char *name;
{
#ifndef HASHGROUPS
    register group_t *ngp, *bound;
    /*
     * A stupid but robust linear-search implementation. It's inherently
     * quadratic - we spend lots of CPU time here because we may search through
     * the whole table for each line. The contents of active.article.m_group 
     * remembers where the last match was found; we start the search there and
     * loop around to the beginning, in the hopes that the calls will be
     * roughly in order. NULL means the code didn't find a group matching the
     * given name.
     */
    /* first, search from the previous value up to the table end */
    bound = active.newsgroups + active.ngc;
    for (ngp = active.article.m_group; ngp < bound; ngp++)
	if (name[0] == ngp->ng_name[0] && !strcmp(name, ngp->ng_name))
	    return(ngp);

    /* next, try from the table base to the previous value */
    bound = active.article.m_group;
    for (ngp = active.newsgroups; ngp < bound; ngp++)
	if (name[0] == ngp->ng_name[0] && !strcmp(name, ngp->ng_name))
	    return(ngp);
#else
    /*
     * A less stupid version using a hash to linked-list chains. See the 
     * comment attached to the for_hash() macro.
     */
    register int *bucket;

    /* search through the group's hash list for it */
    for_hash(bucket, name)
    {
	register group_t *ngp = active.newsgroups + *bucket - 1;
	register char *gname = ngp->ng_name;

	if (name[0] == gname[0] && !strcmp(name, gname))
	    return(ngp);
    }
#endif /* HASHGROUPS */

    /* neither loop found a match */
    return((group_t *)NULL);
}

void clsactive()
/* release the active-groups file */
{
    (void) fclose(active.fp);
#ifdef NONLOCAL
    (void) unlink(ACTIVE);
#endif /* NONLOCAL */
}

bool ngreread(ngp, bproc)
/* update the in-core bitmap of a given group to match reality */
group_t	*ngp;		/* group to update */
bool	(*bproc)();	/* hook for preserving old group info */
{
    static group_t	new;

    if (fseek(active.fp, (off_t)ngp->ng_fseek, SEEK_SET) == FAIL)
	xerror0("active file seek failed");
	/*NOTREACHED*/
    else
    {
	if (fgets(bfr, sizeof(bfr), active.fp) == (char *)NULL)
	    return(FAIL);

  	/* decode the current line into a scratch area */
	(void) ngread(bfr, &new);

	/* let's make sure we don't lose admin file info or search offset */
	new.ng_flags |= (ngp->ng_flags & ~NG_ACTFLAGS);
	new.ng_fseek = ngp->ng_fseek;
	new.ng_name = ngp->ng_name;

	if (bproc == NULLPRED)
	{
	    (void) memcpy(ngp, &new, sizeof(group_t));
	    return(TRUE);
	}
	else
	    return((*bproc)(ngp, &new));
    }
    /* NOTREACHED */
}

#if defined(HASHGROUPS) && defined(SORTACTIVE)
void ngrehash()
/* regenerate the hash lists to reflect the rc_oindex field */
{
    register group_t *ngp, *lastgrp = active.newsgroups + active.ngc;
    register int	*bucket;

    (void) bzero((char *)buckets, sizeof(buckets));
    for (ngp = active.newsgroups; ngp < lastgrp; ngp++)
	ngp->ng_nextg = 0;
    for (ngp = active.newsgroups; ngp < lastgrp; ngp++)
    {
	for_hash(bucket, ngp->ng_name)
	    continue;
	bucket[0] = ngp - active.newsgroups + 1;
    }
}
#endif /* defined(HASHGROUPS) && defined(SORTACTIVE) */

#ifdef PROFILE
char *Progname = "profactive";

main(argc, argv)
/*
 * A torture tester for ngfind(). Compile with -p to test search optimizations.
 * Takes an iteration count and a seed for srand() as arguments -- rand()'s
 * "poor spectral properties" are no problem here, we're just using it to
 * generate a non-sorted sequence of names. Note that this code does *not*
 * test the invalid-group case.
 */
int	argc;
char	*argv[];
{
    long iterations = atol(argv[1]);
    register group_t *rgrp, *gp;
    int i = 0;

    newsinit();
    (void) rdactive(NULLPRED);
    srand(atol(argv[2]));
    while (iterations--)
    {
	/* pick a random group */
	rgrp = active.newsgroups + rand() % active.ngc;

	if ((gp = ngfind(rgrp->ng_name)) == (group_t *)NULL)
	    (void) printf("%04d: ERROR, %s NOT FOUND\n", ++i, rgrp->ng_name);
	else
	    (void) printf("%04d: Group %s found, name %s\n",
				++i, rgrp->ng_name, gp->ng_name);
    }
    clsactive();
}
#endif /* PROFILE */

/* rdactive.c ends here */
