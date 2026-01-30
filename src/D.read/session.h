/* session.h -- interface file for the message trail functions */

/* move types for msgnext() */
#define M_HOLD	-1	/* we've backed up into the trail */
#define M_SEEK	0	/* we're sitting on the article wanted */
#define M_NEXT	1	/* go to next article */
#define M_SKIP	2	/* go to next article at this followup level */

/* constants for msgmark() arguments */
#define READ	TRUE
#define UNREAD	FALSE

/* constants for msgrate() arguments (used only if FEEDBACK on) */
#define F_SEENTEXT	'*'	/* register that the article is read */
#define F_PRAISE	'+'	/* praise the current article */
#define F_CONDEMN	'-'	/* condemn the current article */

#define RUNCOUNT	/* try to economize on trail storage */

/*
 * The trail datum structure.
 *
 * Warning: trail items should *never* be directly munged by code other than
 * the session.c and reader.c entry points! 
 */
typedef struct msgr
{
    place_t	loc;		/* location of end of current run */
    int		marktype;	/* type of the mark, and whether it's on */
#define NOMARK	0x00			/* no marking action */
#define LOCAL	0x01			/* mark it only in the current group */
#define GLOBAL	0x02			/* mark this article in all groups */
#define DELAYED	0x04			/* do local mark at group exit */
#define MKTYPE	0x07			/* mask out the type field with this */
#define MARKON	0x08			/* this bit on if mark is on */
#define SYSMARK	0x0f			/* reserved for internal use */
#ifdef BYTHREADS
    char	*follow;	/* pointer to next followup to read */
    struct msgr *parent;	/* ptr to parent if we're followuping */
#endif /* BYTHREADS */
#ifdef RUNCOUNT
    nart_t	runcount;	/* count of locations in current run */
#endif /* RUNCOUNT */
#ifdef FEEDBACK
    char	*id;		/* non-NULL if text of this article was seen */
    int		pro;		/* praise count */
    int		con;		/* condemnation count */
#endif /* FEEDBACK */
}
msgrec_t;

/*
 * The session structure holds all public state information on the reader
 * session to be used by readers and reader-like tools.
 *
 * Note: readers can safely set the action, reread, thread, and quiet members.
 * Everything else should be considered read-only!
 */
typedef struct
{
    /* trail and backtracking information */
    dbdef_t	tralloc;	/* private alloc block for message trail */
    msgrec_t	*trail;		/* pointer to base of article trail */
    msgrec_t	*cmsg;		/* pointer to current article trail slot */
    nart_t	backtrack;	/* are we backed into the trail? */
#ifdef RUNCOUNT
    nart_t	backin;		/* how far are we back into current run? */
#endif /* RUNCOUNT */

    /* global session status information */
    char	text[BUFLEN];	/* file corresponding to current message */
    int		action;		/* action to take on next message get */
    int		reread;		/* TRUE if we should see old messages */
    int		reverse;	/* TRUE if we want to read in reverse order */
    nart_t	news;		/* count of msgs seen in all groups */
    nart_t	waiting;	/* total number of messages waiting */
    nart_t	ngrp;		/* count of msgs seen in this group */
#ifdef NEWSFILTER
    int		interest;	/* interest score of current article */
#endif /* NEWSFILTER */
#ifdef BYTHREADS
    int		thread;		/* enable thread-following */
    int		depth;		/* current conversation-tree depth */
#endif /* BYTHREADS */
#ifdef DELAYMARK
    nart_t	ndmarks;	/* count delay marks in current group */
#endif /* DELAYMARK */
#ifdef FEEDBACK
    char	quiet[BUFLEN];	/* groups to report feedback on */
#endif /* FEEDBACK */
}
session_t;

extern session_t session;	/* the session control block */

/* session.c functions */
extern void msginit();	/* set up message access for reader */
extern int msgread();	/* get next article */
extern int msgtext();	/* rewind the article file pointer to start of text */
extern int msggoto();	/* select article by Internet ID and go to it */
extern void msgend();	/* end the browse session */

/* reader.c functions */
extern int msgnext();	/* get next article, interactive version */
extern int msglast();	/* back up into previous message */
extern int msgparent();	/* go to parent of current message */
extern int msgmark();	/* mark a message read or unread, possibly globally */
extern int msgtell();	/* return a placemark */
extern void msgseek();	/* seek to a placemark */
extern void msgadd();	/* update groups holding a followup article */
extern void msgrate();	/* register feedback on an article */
extern void msgsweep();	/* dump session feedback to the sweep database */

#define msgfile()	session.text

/* session.h ends here */
