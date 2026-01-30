/* newsrc.h -- defines and types for using .newsrc files */

/*
 * Note: this header file defines the interface to *three* object modules --
 *    rdbits.o (for reading .newsrc-style bit vector representations)
 *    rdnewsrc.o (for reading .newsrc file articles-seen data only)
 *    wrnewsrc.o (full .newsrc format read & write facilities for readers)
 */

/* more masks for the rc_flags field of group_t, see active.h */
#define RC_NOTED	0x0001	/* group is in user's .newsrc */
#define RC_UNSEL	0x0002	/* it was excluded by subscription */
#define RC_UNSUB	0x0004	/* it's been explicitly unsuscribed */
#define RC_DROPSUBSC	0x0008	/* it will be unsubscribed at session end */
#define RC_VISITED	0x0010	/* it's been visited at least once */
#define RC_MAILBOX	0x0020	/* the group is actually a mailbox */
#define RC_HASSUBS	0x0040	/* group has at least one subscriber */

#define SUBSCMK	    ':'		    /* after a group name in a .newsrc */
#define UNSUBSCMK   '!'		    /* these say whether it's subscribed to */

#define SET	    0x01	/* tell rdbits() to do bit sets */
#define CLEAR	    0x02	/* tell rdbits() to do bit clears */

/* use this as arg of readopts() to suppress options processing */
#define NOOPTS	    (int (*)())NULL

#define NO_INDEX	-1	/* out-of-band for subject index line # */

/* directive types */
#define N_OPTIONS	1	/* marks an options line */
#define N_MACRO		2	/* marks a macro definition line */

#ifdef MULTISOURCE
/*
 * this structure specifies everything we need to know about a reader resource
 */
typedef struct rdconnect
{
    char	*r_name;	/* the resource name */
    void	(*r_open)();	/* open a connection */
    char	(*r_active)();	/* get an active file copy */
    int		(*r_artname)();	/* get a copy of a given article */
    int		(*r_hstseek)();	/* select an article ID for perusal */
    int		(*r_nextloc)();	/* get the next location of the article */
    void	(*r_close)();	/* close the connection */
}
rconn_t;
#endif /* MULTISOURCE */

extern int rdnewsrc();		/* read a user's .newsrc data */
extern int readopts();		/* read current user's .newsrc + options */
extern int rcupdate();		/* check for new articles waiting */
extern nart_t rccount();	/* count new articles waiting */
extern void setsubsc();		/* set a subscription list */
extern void wrnewsrc();		/* write a user's .newsrc data */
extern void touchrc();		/* touch the caller's .newsrc */

#define rcsubsc(gp)	(!((gp)->rc_flags & (RC_UNSUB | RC_UNSEL)))

#define rcflag(n)	(active.article.m_group->rc_flags & (n))
#define rcfset(n)	(active.article.m_group->rc_flags |= (n))
#define rcfclear(n)	(active.article.m_group->rc_flags &= ~(n))

#define rcindex()	active.article.m_group->rc_lindex

/* handling of subscribes and unsubscribes */
extern void subscribe();    /* subscribe to a group */
extern bool unsubscribe();  /* unsubscribe from a group (if ADMSUB permits) */
extern bool ckfollow();	    /* check if an ID is in no-followups list */
extern void dontfollow();   /* add an ID to the no-followups list */

#undef setbit		/* necessary to avoid fooup on 4.3BSD */

extern int getbit();	/* get the 'seen' bit for an article */
extern int setbit();	/* set an article 'seen' */
extern int clearbit();	/* clear an article's 'seen' bit */
extern int rdbits();	/* read buffer in .newsrc form to bitmap */
extern void wrbits();	/* write bitmap to buffer in .newsrc form */

/* newsrc.h ends here */
