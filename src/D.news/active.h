/* active.h -- types and equates for in-core newsgroup data */

#ifdef CRACKMAIL
typedef struct s_mailbox mbox_t;	/* this will be declared in newsrc.h */
#endif /* CRACKMAIL */

#ifdef MULTISOURCE
typedef struct s_rdconnect rdconn_t;	/* ditto */
#endif /* MULTISOURCE */
 
typedef struct	/* an element of the in-core group data table */
{
    /* these fields are valid after a rdactive() */
    char	*ng_name;	/* name of the group */
#ifdef HASHGROUPS
    int		ng_nextg;	/* next group in current hash bucket */
#endif /* HASHGROUPS */
    nart_t	ng_max;		/* newsgroup size */
    nart_t	ng_min;		/* minimum article number */
    bits_t	ng_flags;	/* flag qualifiers for the group */
    time_t	ng_expire;	/* expiration date for the group */
    off_t	ng_fseek;	/* pointer to data in the file */
    time_t	ng_age;		/* time since last post to group */
    nart_t	ng_unread;	/* count of unread articles in the group */

#ifndef MISER
    char	*ng_defdist;	/* Default distributions */
#endif

#ifdef FEEDBITS
    /* following information valid only after a feedbits() */
    bits_t	ng_feeds;	/* subscription bits for news feeds */
#endif	/* FEEDBITS */

#ifdef MULTISOURCE
    /* this tells us how to get at an article's text */
    rconn_t	*rc_srctype;	/* method table for article source */
    char	*rc_srcname;	/* name of source */
#endif /* MULTISOURCE */

    /* following information only valid after a rdnewsrc() */
    bits_t	rc_flags;	/* read status flags for the group */
    short	rc_lindex;	/* index of group in the last .newsrc */
    uchar	*rc_seen;	/* ptr to malloc'd bitmap */
#ifdef CRACKMAIL
    mbox_t	*rc_box;	/* non-NULL if this is a mail group */
#endif /* CRACKMAIL */
}
group_t;

#ifdef MISER
/*
 * Note that when rdflags() is called with NG_GETDIST, the per-group default
 * destination info gets stuffed in the ng_seen field. I did this to avoid
 * declaring another (char *) field and increasing the memory size of the
 * active-groups array, but it has the disadvantage that you can't use default
 * distributions and user seen bits in the same program.
 */
#define ng_defdist	rc_seen
#endif

/*
 * For similar reasons, we use ng_lindex as the field rdflags() stuffs the
 * line index of a group's last administration flags line into.
 */
#define ng_findex	rc_lindex

#ifdef DOXREFS
/*
 * Rnews needs a place to keep new-article numbers when DOXREFS is active and
 * they have to be generated early.
 */
#define ng_nextnum	rc_flags
#endif /* DOXREFS */
/*
 * Finally, use rc_lindex to store subject span index offsets in some readers.
 */
#define rc_subjstart	rc_lindex

/* active file status flags (bits 0x00ff are reserved for this) */
#define NG_MODERATED	0x0001	/* can post to group */
#define NG_LOCAL	0x0002	/* the group is local */
#define NG_CHANGED	0x0004	/* articles posted since last rdactive() */
#define NG_REMOVED	0x0008	/* group has been rmgrouped */
#define NG_HASHED	0x0080	/* this group has been added to a hash chain */
#define NG_ACTFLAGS	0x00ff	/* use for clearing active file flags */

/* admin file status flags (bits 0xff00 are reserved for this) */
#define NG_VOLATILE	0x0100	/* it's volatile */
#define NG_COMPRESSED	0x0200	/* store articles compressed */
#define NG_IGNEXPDATE	0x0400	/* ignore explicit expire dates */
#define NG_ARCHIVEIT	0x0800	/* archive this group */
#define NG_EXPIREIT	0x1000	/* apply expiration-by-ageing to this group */
#define NG_ADMFLAGS	0xff00	/* use for clearing admin file flags */

/* miscellaneous other flag values */
#define NG_EXPIRE	0x1000	/* there is expiry info for this group */
#define NG_GETDIST	0x2000	/* only used as a rdflags() argument */

#define NO_AGE		(time_t)0

typedef struct			/* used for storing an article location */
{
    group_t *m_group;		/* the group data pointer */
    nart_t  m_number;		/* a message number within the group */
}
place_t;

typedef struct
{
    group_t	*newsgroups;	/* the group information array */
    place_t	article;	/* the current article location */
    int		ngc;		/* the highest group index */
    FILE	*fp;		/* a handle on the active file */
}
active_t;
extern active_t	active;		/* must be visible for the macros to work */

#define ngunread()	active.article.m_group->ng_unread
#define art	active.article

#define AT(x)	((x).m_group == art.m_group && (x).m_number == art.m_number)
#define TO(x)	((art.m_group = (x).m_group) && (art.m_number = (x).m_number))
#define AOK(g,n)((g)!=(group_t*)NULL && (n)>=(g)->ng_min && (n)<=(g)->ng_max)

/* group data I/O functions */
extern int rdactive();		/* read the current active file */
extern char **rdflags();	/* read in administration flags */
extern void ngshow();		/* write active data for single group */
extern int wractfile();		/* write active file index to stdout */
extern void wractive();		/* write active data */
extern void clsactive();	/* release active data */
extern bool ngreread();		/* reread data for given group */

/* methods to change the current record */
#define ngreset(flg)	(art.m_group = (flg) ? active.newsgroups : (active.newsgroups + active.ngc - 1))
extern void ngrewind();		/* reset the current group pointer */
extern bool ngnext();		/* go to next group (with wraparound) */
extern bool ngback();		/* go to previous group (with wraparound) */
extern group_t *ngfind();	/* select a group by name */
#define ngselect(p)	(art.m_group = (p))

/* ways to read the current-group data */
#define ngactive()	art.m_group
#define ngfirst()	(art.m_group == active.newsgroups)
#define nglast()	(art.m_group == active.newsgroups + active.ngc - 1)
#define ngname()	art.m_group->ng_name
#define ngmin()		art.m_group->ng_min
#define ngmax()		art.m_group->ng_max
#define ngflag(n)	((art.m_group->ng_flags & (n)) != 0)
#define ngexpire()	art.m_group->ng_expire
#define msgnum()	art.m_number

#ifdef FEEDBITS
/* get or set feed subscription bits */
#define ngfeed(gp, n)	(((gp)->ng_feeds >> (n)) & 1)
#define ngmkfeed(gp, n)	((gp)->ng_feeds |= (1 << (n)))
#endif /* FEEDBITS */

/* methods for modifying current-group data */
#define mkngmin(n)	(art.m_group->ng_min = (n))
#define mkngmax(n)	(art.m_group->ng_max = (n))
#define ngfset(n)	(art.m_group->ng_flags |= (n))
#define ngfclear(n)	(art.m_group->ng_flags &= ~(n))

/* functions for testing and modifying the active file in place */
extern void ngcreate();		/* append a new newsgroup */
extern nart_t ngnewart();	/* bump a maximum article number */
extern void ngdelete();		/* mark a group for deletion */

#if defined(HASHGROUPS) && defined(SORTACTIVE)
extern void ngrehash();		/* regenerate group hash lists */
#endif /* defined(HASHGROUPS) && defined(SORTACTIVE) */

/* file names for existence and permission checks */
extern char *ACTIVE;		/* the active file */

/* active.h ends here */
