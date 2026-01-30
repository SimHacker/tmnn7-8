/* feeds.h -- definitions for use of the news feeds file */

#define S_FLDSEP	':'   	/* feed file field separators */
#define S_FLEXSEP	'/'	/* auxilliary field separator */

#define	SNLN		8	/* max significant chars in system names */
#define S_MANYHOPS	9999	/* maximum hops on articles to pass through */

typedef struct		/* the parsed form of a line from FEEDFILE */
{
    char    *s_name;		/* system name		*/
    char    *s_nosend;		/* system's other feeds	*/
    char    *s_ngroups;		/* system subscriptions */
    char    *s_gflex;		/* system flex groups	*/
    char    *s_distribs;	/* system distributions */
    char    *s_dflex;		/* system flex distribs	*/
    char    *s_flags;		/* system flags		*/
    char    *s_xmit;		/* system xmit routine	*/
    char    *s_comments;	/* comments for record	*/
    int     s_clength;		/* length of comment	*/
}
feed_t;

/* read-side entry points */
extern int	s_maxhops;	/* maximum path length implied by L options */
extern void	s_read();	/* get read access to feeds file */
extern void	s_rewind();	/* rewind the feeds file pointer */
extern feed_t	*s_next();	/* read next record */
extern feed_t	*s_find();	/* find system by name */
extern char	*s_option();	/* parse options of given system */
#ifdef FEEDBITS
extern int	s_tell();	/* number of the current record */
#endif /* FEEDBITS */

/* modify-side entry points */
extern void	s_changesub();	/* add or delete a subscription */
extern int	s_writerec();	/* write out a single feeds records */
extern void	s_write();	/* write out the modified feeds file */

extern dbdef_t	feeds;		/* feed list control structure */

#define s_tell(sp)   (((char *)sp - feeds.records)/sizeof(feed_t))

/* feeds.h ends here */
