/*****************************************************************************

NAME
   alist.c -- use dballoc routines to manage association lists

SYNOPSIS
   #include "dballoc.h"
   #include "alist.h"

   int asplit(as, fp)	-- split the given line into an association record
   assoc *as; FILE *fp;
 
   int adump(as, fp)	-- write the text form of a record to a file pointer
   assoc *as; FILE *fp;

   char *amatch(db, name, fn)	-- return the value associated with a given key
   dbdef_t *db; char *name; bool (*fn)();

   char *afind(db,name)	-- return the value associated with a given key
   dbdef_t *db; char *name;

   void aenter(db, key, val)	-- enter a value for a new key
   dbdef_t *db; char *key, *val;

   char *areplace(db, newval)	-- change the value of the selected key
   dbdef_t *db; char *newval;

   char *adelete(db)		-- delete the given pair
   dbdef_t *db;

DESCRIPTION
   This code handles association lists. An association list is represented by
a file of lines each containing two fields separated by writespace (lines
beginning with # are consdidered comments and ignored). Entry points are given
to read in a file of associations and to do lookup on the first field and
return the value of the second. It uses the dballoc.c routines.

BUGS
   adelete() doesn't reclaim the table slot it theoretically frees up. Also,
dbanext() will still find a deleted key with value NULL, and a following
aenter() would leave two instances of the key in the table.
 
NOTE
   Compiling this module with -DMAIN will produce an interactive tester for
these routines.

AUTHOR
   Eric S. Raymond
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

*****************************************************************************/
/* LINTLIBRARY */

#include <stdio.h>
#include <ctype.h>
#include "libport.h"
#include "dballoc.h"
#include "alist.h"

#ifndef BUFLEN
#define BUFLEN	512	/* so as not to need news.h */
#endif

int asplit(as, fp)
/* split the given line into an association record */
assoc	*as;
FILE	*fp;
{
    char    line[BUFLEN], *p;

    if (fgets(line, sizeof(line), fp) == (char *)NULL)
	return(FAIL);
    if (line[0] == '#')
	return(COMMENT);

    (void) nstrip(line);
    for (p = line; *p && (isalnum(*p) || ispunct(*p)); p++)
	    continue;
    *p++ = 0;
    for (; *p && isspace(*p); p++)
	    continue;
    as->key = savestr(line);
    as->value = savestr(p);
    return(SUCCEED);
}

int adump(as, fp)
/* write the external form of a record to a file pointer */
assoc	*as;
FILE	*fp;
{
    if (as->value)
	(void) fprintf(fp, "%s\t%s\n", as->key, as->value);
    return(SUCCEED);
}

#ifdef lint
/*ARGSUSED0*/
#endif /* lint */
char *amatch(db, name, cmpfun)
/* return the value associated with a given key, NULL if none */
dbdef_t	*db;
char	*name;
bool	(*cmpfun)();
{
#ifndef lint
    assoc   *as = (assoc *) dbafind(db, name, cmpfun);
#else
    assoc   *as = (assoc *)NULL;
#endif /* lint */

    if (as == (assoc *) 0)
	return((char *) 0);
    else
	return(as->value);
}

char *afind(db, name)
/* return the value associated with a given key, NULL if none */
dbdef_t	*db;
char	*name;
{
    return(amatch(db, name, streq));
}

/*ARGSUSED*/
void aenter(db, key, val)
/* enter a value for a new key */
dbdef_t *db; char *key, *val;
{
#ifndef lint
    assoc   *ap = (assoc *) dballoc(db);
#else
    assoc   *ap = (assoc *)NULL;
#endif /* lint */
    ap->key = savestr(key);
    ap->value = savestr(val);
}

#ifdef lint
/*ARGSUSED*/
#endif
void areplace(db, str)
/* replace 2nd part of currently selected association wth a copy of str */
dbdef_t	*db;	/* alist to modify */
char	*str;	/* text data */
{
#ifndef lint
    assoc   *as = (assoc *) db->cp;
#else
    assoc   *as = (assoc *)NULL;
    db = db;
#endif /* lint */
    (void) free(as->value);
    if (str)
	as->value = savestr(str);
    else
	as->value = (char *)NULL;
}

#ifdef MAIN

#ifdef __FOO__
dbdef_t alist =
{
    (char *)NULL,	/* no file associated */
    sizeof(assoc), 	/* records are character pointers */
    20,			/* initially, allocate space for 20 records */
    10,			/* allocate 10 records at a time when we run out */
    asplit,		/* use this function to convert records */
    adump,		/* use this function to write out records */
};
#else
ALIST(alist, 20, 10)
#endif

/*
 * An exerciser for these functions.
 */
char	*Progname = "alist";

main()
{
    char    cmdline[100], strv[100], strv2[100];
    int	    nv;

    (void) printf("This is the a-list code tester, type ? for help\n");
    while (fputs("* ", stdout), fgets(cmdline, sizeof(cmdline), stdin) != (char *)NULL)
    {
	(void) nstrip(cmdline);

	if (sscanf(cmdline, "a %s", strv) == 1)
	{
	    (void) printf("Reading specified alist-format file\n");
	    alist.file = savestr(strv);
	    (void) printf("There were %d records\n", dbaread(&alist));
	}
	else if (cmdline[0] == 'A')
	{
	    (void) printf("Reading default alist-format file\n");
	    alist.file = savestr("/usr/lib/news/aliases");
	    (void) printf("There were %d records\n", dbaread(&alist));
	}
	else if (sscanf(cmdline, "g %s", strv) == 1)
	{
	    char *ngp = afind(&alist, strv);

	    if (ngp == (char *)NULL)
		(void) printf("%s: no such key\n", strv);
	    else
		(void) printf("%s: value is %s\n", strv, ngp);
	}
	else if (sscanf(cmdline, "e %s %s", strv, strv2) == 2)
	    (void) areplace(&alist, strv, strv2);
	else if (sscanf(cmdline, "r %s", strv) == 1)
	    (void) areplace(&alist, strv);
	else if (cmdline[0] == 'd')
	    (void) adelete(&alist);
	else if (cmdline[0] == 'w')
	    (void) dbadump(&alist, stdout);
	else if (cmdline[0] == 'x')
	    exit(0);
	else if (cmdline[0] == '!')
	    system(cmdline + 1);
	else if (cmdline[0] == '?')
	{
	    (void)printf("a file     -- read specified alist-format file\n");
	    (void)printf("A file     -- read default alist-format file\n");
	    (void)printf("g name     -- see value of name <name>\n");
	    (void)printf("e key val  -- enter value of selected pair\n");
	    (void)printf("r newval   -- replace value of selected pair\n");
	    (void)printf("d          -- delete selected pair\n");
	    (void)printf("w          -- dump all records to stdout\n");

	    (void)printf("x          -- exit\n\n");

	    (void)printf("! cmd      -- execute shell command\n");
	    (void)printf("?          -- print this help message\n");
	}
	else
	    (void)printf("Illegal command -- type ? for help\n");
    }
}

catch_t xxit(n)
int	n;
{
    exit(n);
}
#endif /* MAIN */

/* alist.c ends here */
