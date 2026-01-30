/*
 * memccpy.c
 *
 * Copy no more than n bytes of t to s, stopping if char c is copied. Return
 * a pointer to the byte following character c, or NULL if c is not found
 * in the first n bytes.
 *
 * Written by reading the System V Interface Definition, not the code.
 *
 * Totally public domain.
 *
 */
/*LINTLIBRARY*/

char *memccpy(t, s, c, n)
register char *t, *s;
register int c, n;
{
    while (--n >= 0)
	if ((*t++ = *s++) == c)
	    return(t);
    return(0);
}
