/* post.h -- interface to article-posting routines */

/* multicast limits (used by post.c and sendbatch.c) */
#define	MAXMCAST    20	    /* max sites that can be attached to a master */
#define	MAXMCS	    10	    /* max mult-cast masters per feed file */

/* functions and variables exported by post.c */
extern void	post();
extern int	verbose;
extern char	nosend[];

/* miscellaneous posting functions */
extern bool dispatch();		/* from dispatch.c */
extern bool controlmain();	/* from control.c */
extern int insert();		/* from insert.c */
extern int batchmode();		/* from unbatch.c */
extern int batchproc();		/* from unbatch.c */

/* post.h ends here */
