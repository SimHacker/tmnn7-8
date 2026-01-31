/*****************************************************************************

NAME
   slist.c -- use dballoc routines to manage string lists

SYNOPSIS
   #include "dballoc.h"
   #include "slist.h"

   int sgets(as, fp)		-- read a string list file line
   char	**as; FILE *fp;

   int sputs(as, fp)		-- write a string list file line
   char	**as; FILE *fp;

   bool sfind(db, key)		-- check for presence of a key in db
   dbdef_t *db; char *key;

DESCRIPTION
   This code uses the dballoc.c routines to handle string lists.
A string list is represented by a file of text lines, with lines
beginning with # ignored.

NOTE
   Compiling this module with -DMAIN will produce an interactive tester for
these routines.

AUTHOR
   Eric S. Raymond
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

*****************************************************************************/
/* LINTLIBRARY */
#include "libport.h"

#ifndef BUFLEN
#define BUFLEN	512	/* so as not to need news.h */
#endif

#include "dballoc.h"
#include "slist.h"

int sgets(as, fp)
/* get a line, turn it into an allocated record */
char	**as;
FILE	*fp;
{
    char    bfr[BUFSIZ];

    if (fgets(bfr, sizeof(bfr), fp) == (char *)NULL)
	return(FAIL);
    if (bfr[1] == '#')
	return(COMMENT);

    (void) nstrip(bfr);
    *as = savestr(bfr);
    return(SUCCEED);
}

int sputs(as, fp)
/* dump an allocated string to a file pointer */
char	**as;
FILE	*fp;
{
    (void) fputs(*as, fp);
    (void) fputc('\n', fp);
    return(SUCCEED);
}

bool sfind(db, dkey)
/* check for presence of the key in the given db */
dbdef_t *db;
char *dkey;
{
    return(dbafind(db, dkey, streq) != (char *)NULL);
}

#ifdef MAIN

SLIST(slist, 20, 10)

/*
 * An exerciser for these functions.
 */
char	*Progname = "slist";

main()
{
    char    cmdline[100], strv[100];
    int	    nv;

    while (fputs("* ", stdout), gets(cmdline) != (char *)NULL)
    {
	(void) nstrip(cmdline);

	if (sscanf(cmdline, "a %s", strv) == 1)
	{
	    (void) printf("Reading specified slist-format file\n");
	    (void) strcpy(slist.file, strv);
	    (void) printf("There were %d records\n", dbaread(&slist));
	}
	else if (cmdline[0] == 'A')
	{
	    (void) printf("Reading default file\n");
	    (void) strcpy(slist.file, "/usr/lib/news/aliases");
	    (void) printf("There were %d records\n", dbaread(&slist));
	}
	else if (sscanf(cmdline, "g %s", strv) == 1)
	{
	    char *ngp = dbafind(&slist, strv);

	    if (ngp == (char *)NULL)
		(void) printf("%s: no such string\n", strv);
	    else
		(void) printf("%s: present\n", strv);
	}
	else if (cmdline[0] == 'w')
	    (void) dbadump(&slist, stdout);
	else if (cmdline[0] == 'x')
	    exit(0);
	else if (cmdline[0] == '!')
	{
	    system(cmdline + 1);
	    (void) fflush(stdout);	/* V7 system() doesn't flush stdout */
	}
	else if (cmdline[0] == '?')
	{
	    (void)printf("a file   -- read specified slist-format file\n");
	    (void)printf("A file   -- read default slist-format file\n");
	    (void)printf("g name   -- see value of name <name>\n");
	    (void)printf("w        -- dump all records to stdout\n");

	    (void)printf("x        -- exit\n\n");

	    (void)printf("! cmd    -- execute shell command\n");
	    (void)printf("?        -- print this help message\n");
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

/* slist.c ends here */
