/*
 * memchr.c
 *
 * Return address in 1st n chars of sp at which the character c appears;
 * NULL if not found; don't terminate at \0.
 *
 * Written by reading the System V Interface Definition, not the code.
 *
 * Totally public domain.
 *
 */
/*LINTLIBRARY*/

char *memchr(sp, c, n)
register char *sp, c;
register int n;
{
    while (--n >= 0)
	if (*sp++ == c)
	    return(--sp);
    return(0);
}
