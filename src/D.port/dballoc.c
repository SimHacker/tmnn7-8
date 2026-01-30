/****************************************************************************

NAME
   dballoc.c -- generic allocation routines

SYNOPSIS
   char *dballoc(db)	    -- enter a new record, allocate space for it
   dbdef_t *db;

   void dbaenter(db, dp)    -- enter an item into the table
   dbdef_t *db; char *dp;

   int dbanext(db)	    -- advance the current-record pointer
   dbdef_t *db;

   char *dbafind(db, name, cmpfun)	-- find a named entry
   dbdef_t *db; char *name; bool (*cmpfun)();

   int dbarewind(db)	    -- reset the current-record pointer
   dbdef_t *db;

   int dbaread(db)	    -- read a database in from its file
   dbdef_t *db;

   int dbadump(db, fp)	    -- dump the external representation of a database
   dbdef_t *db;
   FILE *fp;

   void dbawrite(db)	    -- write the whole database out to disk
   dbdef_t *db;

   bool streq(s, t)         -- string equality test (for use in dbafind)
   char *s, *t;

DESCRIPTION
   These routines provide the rudiments of an in-core database facility for
an arbitrary number of records of some fixed length. Sequential and by-name
access are supported (name is assumed to be stored as a char * at the front
of the record, and simple linear search is used).
   Two additional entry points are defined by the dbatell() and dbaseek()
macros. The first resturns the count of entries in the database; the second
changes the location of the next-free slot to a given index, implicitly
discarding that record and all following ones.

AUTHOR
   Eric S. Raymond
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

*****************************************************************************/
/* LINTLIBRARY */
#include "libport.h"
#include "dballoc.h"

char *dballoc(db)
/* enter a new record, allocate space for it in core */
dbdef_t	*db;
{
    register int    i;

    if (db->records == (char *)NULL)
    {
	db->cp = db->records =
	    calloc((unsigned) db->arrsize, (unsigned) db->recsize);
	db->nextfree = 0;
    }
    else if (db->nextfree >= db->arrsize)
    {
	int	oldend = db->arrsize * db->recsize;

	i = ptoi(db, db->cp);
	db->arrsize += db->chunksize;
	db->records =realloc(db->records,(unsigned)(db->arrsize*db->recsize));
	(void) bzero(db->records + oldend, db->chunksize * db->recsize);
	db->cp = itop(db, i);
    }

    if (db->records == (char *)NULL)
	return((char *)NULL);

    return(itop(db, db->nextfree++));
}

/*ARGSUSED*/
void dbaenter(db, dp)
/* enter an item into the table -- only works for (char *)-sized items */
dbdef_t	*db;
char	*dp;
{
#ifndef lint
    *((char **) dballoc(db)) = dp;
#endif /* lint */
}

int dbanext(db)
/* advance the current-record pointer */
dbdef_t	*db;
{
    if (ptoi(db, db->cp) < db->nextfree - 1)
    {
	db->cp += db->recsize;
	return(SUCCEED);
    }
    else
	return(FAIL);
}

void dbarewind(db)
/* reset the current-record pointer so next dbanext() resets current record */
dbdef_t	*db;
{
    /* this is a little shaky... */
    db->cp = db->records - db->recsize;
}

int dbaread(db)
/* read a database in from its file */
dbdef_t	*db;
{
    extern FILE *xfopen();
    FILE	*fp = xfopen(db->file, "r");
    int		st;

    /* this loop allocates one extra record...we don't care */
    while ((st = (*db->rget)(dballoc(db), fp)) != FAIL)
	if (st != SUCCEED)
	{
	    /* someday we should recover the space used by these */
	    --db->nextfree;
	}

    (void) fclose(fp);
    return(--db->nextfree);
}

/*
 * Find in-core data corresponding to a given key.
 * This assumes that the first two bytes of the structure
 * are a pointer to the key.
 */
/*ARGSUSED0*/
char *dbafind(db, name, cmpfun)
dbdef_t	*db;
char	*name;
bool	(*cmpfun)();
{
#ifndef lint	/* no way to placate lint about those pointer coercions */
    register char *dp;

    /* first, search from the previous value up to the table end */
    for (dp = db->cp; ptoi(db, dp) < db->nextfree; dp += db->recsize)
	if ((*cmpfun)(name, *((char **) dp)))
	    return(db->cp = dp);

    /* next, try from the table base to the previous value */
    for (dp = db->records; dp < db->cp; dp += db->recsize)
	if (strncmp(name, *((char **) dp), strlen(name)) == 0)
	    return(db->cp = dp);
#endif /* lint */

    /* neither loop found a match */
    return((char *)NULL);
}

int dbadump(db, fp)
/* dump the external representation of a database to a file pointer */
dbdef_t	*db;
FILE	*fp;
{
    char    *dp;

    for (dp = db->records; ptoi(db, dp) < db->nextfree; plusplus(db, dp))
	if ((*db->rput)(dp, fp) == FAIL)
	    return(FAIL);
    return(SUCCEED);
}

void dbawrite(db)
/* write the whole database out to disk */
dbdef_t	    *db;
{
    FILE	*wrcfp;
    char	new_version[BUFSIZ];

    (void) strcpy(new_version, db->file);
    (void) strcat(new_version, ".new");
#ifdef ODEBUG
    (void) fprintf(stderr, "Writing to %s\n", new_version)
#endif /* DEBUG */

#ifdef VMS
    (void) vmsdelete(new_version);
#endif

    /* here goes the actual I/O */
    if (
	(wrcfp = xfopen(new_version, "w")) == (FILE *)NULL
	|| dbadump(db, wrcfp)
	|| fclose(wrcfp) < 0
	)
	xerror1("Error writing %s - no changes made\n", db->file);

#ifdef VMS
    (void) vmsdelete(db->file);
#endif
    if (rename(new_version, db->file) < 0)
	xerror2("Cannot rename %s file to %s", new_version, db->file);
}

bool streq(s, t)
/* string equality predicate */
char	*s, *t;
{
    return(strcmp(s, t) == 0);
}

/* dballoc.c ends here */
