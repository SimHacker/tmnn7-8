/* ngprep.h -- interface to the ngprep.c group-line compilation code */

#define CROSSPOSTS  32	    /* max # of cross-posting destinations accepted */

typedef struct
{
    int		d_status;	/* status of insertion to this group */
#define D_NOMORE	0		/* no more groups */
#define D_OK		1		/* group recognized OK */
#define D_UNKNOWN	2		/* group not recognized */
#define D_MAILED	3		/* already mailed */
#define D_CANTMAIL	4		/* attempt to mail this failed */
    group_t	*d_ptr;		/* a group to be posted to */
    char	*d_name;	/* name of group, if not recognized */
    char	*d_dist;	/* distribution prefixes if any */
    char	*d_moderator;	/* moderator address */
}
dest_t;
extern dest_t	destinations[];	/* the list of compiled destinations */

extern	void	ngprepinit();
extern	void	ngprepare();
extern	int	ngaccept();

/* return types for ngaccept() */
#define A_JUNKIT	1	/* accept article, send to junk */
#define A_ACCEPT	0	/* accept article, post to newsgroups */
#define A_GRPSNG	-1	/* no accepted groups on article */
#define A_DISTNG	-2	/* distribution of article mot accepted */

#ifdef FEEDBITS
/* functions and variables exported by feedbits.c */
extern	int	feedbits();
#endif /* FEEDBITS */

/* ngprep.h ends here */
