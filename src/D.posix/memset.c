/*
 * Fill an array of n chars starting at sp with the character c.
 *
 * Written by reading the System V Interface Definition, not the code.
 *
 * Totally public domain.
 *
 */
/*LINTLIBRARY*/

char *memset(sp, c, n)
register char *sp, c;
register int n;
{
    register char *base = sp;

    while (--n >= 0)
	*sp++ = c;
    return(base);
}
