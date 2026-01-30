/* dballoc.h -- generic allocation routines definitions */

/*
 * Control structure for allocated table areas. The dballoc code uses the
 * recsize field everywhere it has to do allocation or pointer bumping
 */
typedef struct
{
    char    *file;		/* associated filename */

    int	    recsize;		/* the size of an in-core record */
    int	    arrsize;		/* current allocation size */
    int	    chunksize;		/* increment size for reallocation */

    /* for these, first arg is record ptr, second arg is file pointer */
    int	    (*rget)();		/* how to read a record into core */
    int	    (*rput)();		/* how to write a record to disk */

    /* the user never initializes these */
    char    *records;		/* allocated area for the stuff */
    int     nextfree;		/* count of records */
    char    *cp;		/* pointer to the current record */
}
dbdef_t;

#define ptoi(db, p)	(((p) - (db)->records) / (db)->recsize)
#define itop(db, n)	((db)->records + ((n) * (db)->recsize))
#define plusplus(db, p)	(p += (db)->recsize)

#define dbathis(db)	(db->cp)
#define dbatell(db)	((db)->nextfree)
#define dbaseek(db, n)  ((db)->cp = itop((db), (db)->nextfree = (n)))

extern char *dballoc();
extern void dbaenter();
extern int dbanext();
extern void dbarewind();
extern int dbaread();
extern char *dbafind();
extern int dbadump();
extern void dbawrite();
extern bool streq();

#ifndef SUCCEED
#define SUCCEED	0
#define FAIL	-1
#endif
#define COMMENT	1	/* must not be SUCCEED or FAIL */

/* dballoc.h ends here */
