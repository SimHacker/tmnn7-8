/***************************************************************************

NAME
   collect.c -- snarf stdin into a tempfile

SYNOPSIS
   #include "news.h"

   void collect(hdrline, srcfile)
   char *hdrline, *srcfile;

DESCRIPTION
   Collect characters off stdin into a temporary file with a specified header. 
The srcfile argument should be a mktemp(3) template. For maximum portability
it should *not* be a string constant, these are not guaranteed to be writeable.
A NULL hdrline argument will be ignored.

AUTHOR
   Eric S. Raymond
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

****************************************************************************/
/*LINTLIBRARY*/
#include "news.h"
#include "libpriv.h"

void collect(hdrline, srcfile)
/* collect input from stdin to a generated file name */
char	*hdrline;	/* header line to write to file */
char	*srcfile;	/* buffer to mktemp() generated filename in */
{
    register int	c;
    FILE		*bfp;

    (void) mktemp(srcfile);
    bfp = xfopen(srcfile, "w");
    if (hdrline != (char *)NULL)
	(void) fputs(hdrline, bfp);
    while ((c = getc(stdin)) != EOF)
	    (void) putc(c, bfp);
    (void) fclose(bfp);
}

/* collect.c ends here */
