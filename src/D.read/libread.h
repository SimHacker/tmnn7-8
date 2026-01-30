/* libread.h - declarations for miscellaneous readlib.a objects */

/* functions declared in checkinit.c */
extern int checkinit();
extern void qfflush();
extern int lprocargs();

/* public storage owned by checkinit.c */
extern char	*usage;
extern int	nofoll, asave;
extern char	grouplist[], datebuf[], titlebuf[];
extern time_t	starttime;

/* article header display control */
#define NOHIGH	(void(*)())NULL
#define P_SLOWTTY	0	/* print minimum header lines */
#define P_PARTHDRS	1	/* print selected headers */
#define P_FULLHDRS	2	/* print all non-noise RFC-1036 headers */
extern void	hprint(), cleanup();

/* other functions and storage declared by readinit.c */
extern void	cleanup();
extern int	readinit();
extern int	nochange, flushall;
extern char	inclmark[];

/* vinfoline.c things */
#define PREFSPACE	2	/* prefix space presumed available for marks */
extern void vsubjline();
extern void vgroupline();

#ifdef DEBUG
extern char	usetty[];	/* used only for debugging */
#endif /* DEBUG */

/* libread.h ends here */
