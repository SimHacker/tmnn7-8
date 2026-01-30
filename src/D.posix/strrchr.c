/*
 * Return ptr in sp at which the character c last appears, or
 * NULL if not found
 *
 * Written by reading the System V Interface Definition, not the code.
 *
 * Totally public domain.
 *
 */
/*LINTLIBRARY*/

char *strrchr(sp, c)
register char *sp, c;
{
    register char *rp;

    rp = (char *)0;
    do {
	if (*sp == c)
	    rp = sp;
	} while
	    (*sp++);
    return(rp);
}
