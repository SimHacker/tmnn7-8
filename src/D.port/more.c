/*****************************************************************************

NAME
   more.c -- display a file through the pager

SYNOPSIS
   int more(file, ifp, lines)	-- page the file
   char *file; FILE *ifp; int lines;

DESCRIPTION
   This function invokes the program named in the environment variable PAGER
(if it's set), or the system default pager (usually more(1) or pg(1)). If the
third argument is nonzero and less than the PAGEMIN constant (usually 23)
lines long, the file will simply be copied to stdout, avoiding the process
spawn overhead.

   In the latter case, if more() can detect that the pager being invoked
really is more(1), it will emit a trailing form feed to pause it. This is
a horrible hack that will probably fail off UNIX systems.

AUTHOR
   Eric S. Raymond
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.
*****************************************************************************/
/* LINTLIBRARY */
#include "libport.h"	/* for the PAGE #define */

#define PAGEMIN		23	/* allow for 24th line of more(1) prompt */

int more(file, ifp, lines)
char *file;
FILE *ifp;
int  lines;
{
    char bfr[BUFSIZ];
    static char *pager = (char *)NULL;

    if (pager == (char *)NULL)
	if ((pager = getenv("PAGER")) == (char *)NULL)
	    pager = PAGE;

    if (file != (char *)NULL)		/* caller gave a filename? */
    {
	if ((ifp = fopen(file, "r")) == (FILE *)NULL)
	    return(FAIL);
	(void) sprintf(bfr, "exec %s %s", pager, file);
	return(system(bfr)); 
    }
    else if (ifp != (FILE *)NULL)	/* caller gave a file pointer? */
    {
	FILE    *pfp = stdout;

	/* don't put us through the pager unless necessary */
	if (*pager && lines == 0 || lines > PAGEMIN)
	    if ((pfp = popen(pager, "w")) == (FILE *)NULL)
		pfp = stdout;

	while (fgets(bfr, sizeof(bfr), ifp))
	    (void) fputs(bfr, pfp);

	if (strncmp(pager, "more", 4) == 0)	/* gawd, what a kluge! */
	    (void) fputc('\f', pfp);

	if (pfp != stdout)
	    (void) pclose(pfp);	
	return(SUCCEED);
    }
    else
	return(FAIL);
}

/* more.c ends here */
