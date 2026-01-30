/* edbm.h -- interface to generic database management routines */

typedef struct
{
    char	*dptr;
    unsigned	dsize;
    long	daddress, dlength;
}
datum;

typedef struct
{
    int	    dirf;	/* file descriptor of the directory file */
    int     pagf;	/* file descriptor of the page file */
    int     datf;	/* file descriptor of the data file */
    char    *dbnm;	/* the database name */
    char    *dirnm;	/* the directory file name */
    char    *datnm;	/* the data file name */
    char    *pagnm;	/* the page file name */
    int	    dbrdonly;	/* TRUE if the database is to be read-only */

    datum   current;	/* the currently-selected datum */
    char    *freeptr;	/* content of the datum (so we can free it later) */

    long    bitno;
    long    maxbno;
    long    blkno;
    long    hmask;

    long    oldpagb;
    long    olddirb;
#define	PBLKSIZ	4096		/* page block size */
    char    pagbuf[PBLKSIZ];
#define	DBLKSIZ	4096		/* directory block size */
    char    dirbuf[DBLKSIZ];
}
database;

extern void dbmtrunc();
extern database *dbmopen();
extern int dbmseek();
extern int dbmdelete();
extern void dbmrewind();
extern int dbmnext();
extern int dbmput();
extern char *dbmget();
extern void dbmunlock();
extern void dbmclose();

#define dbmkey(db)	((db)->current.dptr)

/* edbm.h ends here */
