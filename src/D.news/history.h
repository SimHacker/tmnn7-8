/* history.h -- types and equates for in-core history data */

#ifndef SUCCEED
#define SUCCEED	    0
#define FAIL	    -1
#endif /* SUCCEED */

/* additional status codes for history file entries */
#define VALID	    SUCCEED	/* so we can pass out hstcrack() returns */
#define CANCELLED   1		/* article corresp. to current key expired */
#define EXPIRED	    2		/* article corresp. to current key cancelled */
#define REFERENCE   3		/* article is an unreceived parent */
#define GARBLED	    4		/* content of current history key is garbled */

/* use these for delimiter searches */
#define SPACE	' '
#define TAB	'\t'

#define CANCEL_TOKEN	"cancelled"
#define CANCEL_SIZE	9

/* the history file name (for existence and permission checks */
extern char *HISTORY;

/* group data I/O functions */
extern void hstread();		/* read the current hist file */
extern void hstwrite();		/* write hist data */
extern void hstclose();		/* release the history file or NNTP connect */

/* methods to move the current record pointer */
extern void hstrewind();	/* initialize the world */
extern int hstnext();		/* go to the next article */
extern int hstseek();		/* select a history entry by name */

/* ways to read the current entry data */
#define hstid()		chstname
#define hstdate()	chstdate
#define hstexp()	chstexpd
#define hstat()		chstatus
#define hstline()	chline
#define hstattr()	hstlst.l_this
extern int hstloc();	    /* get the next article location */

/* ways to modify the current entry data */
extern int hstexpire();	    /* drop the current article location */
extern int hstadd();	    /* add a location to the list */
extern char *hstfile();	    /* find an article file by ID */
extern void hstparent();    /* mark the parents of a given header */
#define hstcancel(id)	  (void)hstadd(id,(time_t)0,(time_t)0,CANCEL_TOKEN,(nart_t)FAIL)
#define hstrefer(id, ref) (void)hstadd(id,(time_t)0, (time_t)0, ref, (nart_t)0)
#define hstrewind()	  dbmrewind(rdhistdb)
#define hstdrop()	  (void) dbmdelete(rdhistdb);
#define hstclean()	  dbmtrunc(rdhistdb)

/* declarations that must be visible for the macros to work */
extern char chstname[];		/* ID of the current record */
extern time_t chstdate;		/* receipt date of the current record */
extern time_t chstexpd;		/* expire date of the current record */
extern int chstatus;		/* status of the last hstparse() */

/*
 * Everything below this line is for internal use only
 * (communication between rdhistory.c and wrhistory.c)
 */

extern int hstenter(), *tfind();

#include "edbm.h"

extern database		*rdhistdb;
extern database		*wrhistdb;
extern char		*chline;
extern bool		selfalloc;

#define hstkey()        rdhistdb->current.dptr
#define hstkeylen()     rdhistdb->current.dsize
#define hstunlock()	dbmunlock(rdhistdb)

extern int	histrdok;
extern int	hstwrfile();
extern lptr_t	hstlst;

/* history.h ends here */
