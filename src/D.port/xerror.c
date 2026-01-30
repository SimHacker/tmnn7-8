/*****************************************************************************

NAME
   xerror.c -- error-reporting functions

SYNOPSIS
   #include "libport.h"

   int xerror(message)			-- log an error and exit
   char *message;

   FILE *xfopen(name, fmode)		-- open a file or xerror out
   register char *name, *fmode;

DESCRIPTION
   This module defines standard error and exit functions. It is expected that
xerror() will be called through the libport.h macros.

AUTHOR
   Eric S. Raymond
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

*****************************************************************************/
/*LINTLIBRARY*/
#include "libport.h"

char	xbf[BUFSIZ];		/* buffer for assembling error messages	*/

int xerror(message)
char *message;
{
    extern char *Progname;

    (void) fprintf(stderr, "%s: %s.\n", Progname, message);
    exit(1);
}

FILE *
xfopen(name, fmode)
register char *name, *fmode;
{
    register FILE *fp;

    if ((fp = fopen(name, fmode)) == (FILE *)NULL)
	xerror3("Cannot open %s (%s): %s", name, fmode, errmsg(errno));
    return(fp);
}

/* xerror3.c ends here */
