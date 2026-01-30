/****************************************************************************

NAME
   rdbits.c -- function for decoding bit-map representations

SYNOPSIS
   #include "active.h"
   #include "newsrc.h"

   int getbit(article, ngp)	-- get read/unread status of an article
   nart_t article; group_t *ngp;

   int setbit(article, ngp)	-- mark an article read
   nart_t article; group_t *ngp;

   int clearbit(article, ngp)	-- mark an article unread
   nart_t article; group_t *ngp;

   int rdbits(mode, cp, ngp)	-- set/clear the bits implied by line cp in ngp
   int mode; char *cp; group_t *ngp;

DESCRIPTION
   The rdbits() function following parses a list of numeric literals
alternating with dashes and/or commas into commands to change sections
of a group's bitmap. It also alters the group's unread-message count
and the count of total messages unread appropriately.
   We define this separately so that non-reader programs (in particular,
the eipclib.a library) can use it.

   Given a valid group data pointer the getbit(), setbit() and clearbit()
functions may be used to examine and change the ng_bits information -- but
see the MACRO INTERFACE section below.

THE MACRO INTERFACE
   Some macros are defined in newsrc.h that define pseudo-functional handles
on group information; they are all 'safe' (i.e. can be called with arguments
that have side-effects) and pretty much self-explanatory. The advantage of
using this layer is that it invisibly maintains the current-group pointer
active.article.m_group.

ngunread()	-- count of articles in the group currently unread
ngdump()	-- dump group data to stderr (only defined #ifdef DEBUG)
rccmds()	-- return commands field of the group

There is also:

rcsubsc(gp)	-- return TRUE if this group is subscribed to else FALSE

It is expected that most users will call these macros rather than the
underlying functions, so that active.article.m_group doesn't need to be
manipulated explicitly.

SEE ALSO
   rdnewsrc.c		-- read in seen .newsrc info
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
#include "bitmacros.h"

/* figure the minimum number of bits that will fit in one (char *) */
#ifdef SMALL_ADDRESS_SPACE
#define PTR_BITS	16
#else
#define PTR_BITS	32
#endif

#undef PTR_BITS		/* this code is weirdly buggy */

/*
 * Here are the functions for getting and setting the 'seen' bits
 */

int getbit(article, ngp)
/* return the status of an article */
nart_t	article;
group_t	*ngp;
{
    /* return FAIL if the article is out of range */
    if (article < ngp->ng_min || article > ngp->ng_max)
	return(FAIL);

    if (ngp->rc_seen == (uchar *)NULL)
	return(FALSE);
#ifdef PTR_BITS
    else if (ngp->ng_max - ngp->ng_min + 1 <= PTR_BITS)
	return(GET_BIT((char *)&ngp->rc_seen, article - ngp->ng_min) != 0);
#endif /* PTR_BITS */
    else
	return(GET_BIT(ngp->rc_seen, article - ngp->ng_min) != 0);
}

int setbit(article, ngp)
/* mark an article read */
nart_t	article;
group_t	*ngp;
{
    int grpsize = ngp->ng_max - ngp->ng_min + 1;
    bool changed;

#ifdef GTEST
    (void) fprintf(stderr,
	    "mark: marking article %d of %s\n",
	    article, ngp->ng_name);
#endif /* TEST */

    /* return FAIL if the article is out of range */
    if (article < ngp->ng_min || article > ngp->ng_max)
	return(FAIL);

    /* if there's no bitmap, allocate a right-sized one */
    if (ngp->rc_seen == (uchar *)NULL)
    {
#ifdef PTR_BITS
	if (grpsize > PTR_BITS)
#endif /* PTR_BITS */	
    if ((ngp->rc_seen = (uchar *) ALLOC_BITS(grpsize)) == (uchar*)NULL)
		xerror0("Bitmap allocation failed");
    }

    if (changed = (getbit(article, ngp) == FALSE))
	ngp->ng_unread--;

#ifdef PTR_BITS
    if (ngp->ng_max - ngp->ng_min + 1 <= PTR_BITS)
	SET_BIT((char *)&ngp->rc_seen, article - ngp->ng_min);
    else
#endif /* PTR_BITS */
	SET_BIT(ngp->rc_seen, article - ngp->ng_min);

    ngp->rc_flags |= RC_VISITED;
    return(changed);
}

int clearbit(article, ngp)
/* mark an article unread */
nart_t	article;
group_t	*ngp;
{
    bool changed;

    /* return FAIL if the article is out of range */
    if (article < ngp->ng_min || article > ngp->ng_max)
	return(FAIL);

    if (ngp->rc_seen)
    {
	if (changed = (getbit(article, ngp) == TRUE))
	    ngp->ng_unread++;

#ifdef PTR_BITS
	if (ngp->ng_max - ngp->ng_min + 1 <= PTR_BITS)
	    CLEAR_BIT((char *)&ngp->rc_seen, article - ngp->ng_min);
	else
#endif /* PTR_BITS */
	    CLEAR_BIT(ngp->rc_seen, article - ngp->ng_min);
    }
    else
	return(FAIL);	/* shaky...under some circumstances should be FALSE */

    return(changed);
}

int rdbits(mode, cp, ngp)
/* set/clear the bits implied by line cp to the group data in ngp */
int	mode;	/* whether to do sets, clears or both */
char	*cp;	/* the line to read this data from */
group_t	*ngp;	/* pointer to group data to update */
{
#define S_COMMA	    0	/* low end of a range has been gotten */
#define S_NUM	    1	/* comma after a number has been seen */
#define S_DASH	    2	/* dash has been seen */
#define S_EOL	    3	/* end-of-line seen */
#define S_SOL	    4	/* start-of-line state */

    static int	action[4][5]
	    = {{0,1,0,0,0}, {2,0,3,0,2}, {0,4,0,0,0}, {0,5,0,0,6}};

    int	    toktype, state;
    nart_t  lo = ngp->ng_min;
    nart_t  val = 0;
    nart_t  i;

    /* if we're on the 1st read, the initial value is wrong for CLEAR mode */
    if (mode == CLEAR && ngp->rc_seen == (uchar *)NULL)
	ngp->ng_unread = 0;

    /* parse an unpacked bitmap into a packed one */
    for (state = S_SOL; state != S_EOL; state = toktype)
    {
	/*
	 * Here's the lexical analyzer
	 */
	while (*cp && isspace(*cp))	/* skip leading whitespace */
	    cp++;

	/* look for a token */
	if (*cp == '\0' || *cp == SUBSCMK)
	{
	    toktype = S_EOL;
	}
	else if (*cp == ',')	    /* a comma (ends current range) */
	{
	    toktype = S_COMMA;
	    cp++;
	}
	else if (*cp == '-')	    /* a dash (flags a real range) */
	{
	    toktype = S_DASH;
	    cp++;
	}
	else if (isdigit(*cp))	    /* or a digit sequence */
	{
	    val = atoa(cp);	/* see news.h for atoa() */
	    toktype = S_NUM;
	    while (isdigit(*cp))
		cp++;
	}
	else			    /* anything else is an illegal char */
#ifndef GTEST
	    return(FAIL);
#else
	{
	    (void) fprintf(stderr,
		    "rdbits: choked on bad character '%c' = 0x%02x\n",
		    *cp, *cp);
	    return(FAIL);
	}

	(void) fprintf(stderr,
		"rdbits: token type %d, value %d\n",
		toktype, (toktype == S_NUM) ? val : -1
		);
#endif /* TEST */
	/*
	 * Here's the state machine that does the grammar processing
	 */
	switch(action[toktype][state])
	{
	case 1:		    /* number followed by comma */
	    /* we want to ignore articles too old to be active */
	    lo = (val > ngp->ng_min) ? val : ngp->ng_min;
	    break;

	case 2:		    /* comma followed by number */
#ifdef GTEST
	    (void) fprintf(stderr,
		"rdbits: forgetting %d through %d\n",lo+1,val-1);
#endif /* TEST */
	    if (mode & CLEAR)
		for (i = lo + 1; i < val; i++)
		    (void) clearbit(i, ngp);
	    break;

	case 3:		    /* dash followed by number */
#ifdef GTEST
	    (void) fprintf(stderr,
		"rdbits: marking %d through %d\n", lo, val);
#endif /* TEST */
	    if (mode & SET)
		for (i = lo; i <= val; i++)
		    (void) setbit(i, ngp);
	    break;

	case 4:		    /* number followed by dash */
	    lo = val;
	    break;

	case 5:		    /* digit followed by end-of-field */
	    (void) setbit(val, ngp);
	    break;

	case 6:		    /* start-of-field followed by end-of-field */
	    break;

	default:
	    return(FAIL);
	}
    }

    /* 
     * Now it's time to fix the unread count. If the mode was CLEAR,
     * everything is OK; we only added bits for the unseen ones. Otherwise,
     * if b is the number of unread articles and a the number of read ones;
     *	    (b + a) = (ngmax() - ngmin() + 1)
     * this was the initial value of the unread count.
     * If the mode is SET, the unread field will now have the value
     *	    (b + a) - a = b
     * If the mode is (SET | CLEAR), the unread field will now have the value
     *	    (b + a) + (b - a) = 2b
     * Note that this case will fail if the initial value wasn't (b+a), i.e.
     * usually if (SET | CLEAR) is used after the first read.
     */
    if (mode == (SET | CLEAR))
	ngp->ng_unread /= 2;

    return(SUCCEED);
}

/* rdbits.c ends here */
