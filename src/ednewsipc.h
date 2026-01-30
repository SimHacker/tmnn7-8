/* ednewsipc.h -- types and functions for controlling an ednews server */

#define EBUFLEN	   BUFSIZ   /* should be large enough to hold a screen line */

/* ednews session block */
typedef struct
{
    int type;	    /* tells us which variant to expect */
#define ED_ERROR    -1	/* message error indication */
#define ED_MESSAGE  0	/* a message line */
#define	ED_GROUP    1	/* group data */
#define ED_ARTICLE  2	/* an article-data message */
#define ED_TOPICS   3	/* file name of group data list */
#define ED_SUBJECTS 4	/* file name of group subject line listing */
#define ED_DONE	    5	/* status update messages */
#define ED_NOMORE   6	/* notification that we're about to run out of msgs */
#define ED_BYE	    7	/* server is terminating */
#define ED_CMDERR   8	/* command passed down was no good */

    /* this is set by the TOPICS message */
    char topics[EBUFLEN];	/* name of the topic file */
    int ngtotwait;		/* total count of messages waiting */

    /* these are set by the GROUP message */
    group_t	gp;		/* group data for current group */
    char groupname[EBUFLEN];	/* where the group name lives */

    /* this is set by the ARTICLE response */
    char artfile[EBUFLEN];	/* the name of the article file */
    int lines;			/* the number of lines in it */
    bool seen;			/* tells whether article has been seen */

    char text[EBUFLEN];		/* text buffer for messages */
    int action;			/* action set by last DONE message */
}
ednews_t;

extern int open_ednews();
extern void close_ednews();
extern int read_ednews();
extern int write_ednews();

/* ednewsipc.h ends here */
