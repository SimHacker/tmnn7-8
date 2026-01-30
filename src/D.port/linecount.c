/* linecount.c
 *
 * Count the number of remaining lines in file fp,
 * out to a maximum given by maxoffs.
 * Do not move the file pointer.
 */
/*LINTLIBRARY*/
#include "libport.h"

int linecount(fp, maxoffs)
FILE *fp;
off_t maxoffs;
{
    off_t	curpos, chc;
    register int	nlines = 0;
    register int	c;

    if (fp == (FILE *)NULL)
	return(0);
    curpos = ftell(fp);
    chc = maxoffs - curpos;
    while ((c = getc(fp)) != EOF && (maxoffs == 0 || chc-- > 0))
	if (c == '\n')
	    nlines++;
    (void) fseek(fp, (off_t)curpos, SEEK_SET);
    return(nlines);
}

/* linecount.c ends here */
