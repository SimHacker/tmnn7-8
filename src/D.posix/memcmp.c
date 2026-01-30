/*
 * memcmp.c
 *
 * Compare n bytes using normal lexicographic ordering, so that if
 * sp1 is less than sp2 the result comes out negative, if sp1 == sp2 it
 * is zero, and if sp1 is greater than sp2 it is positive.
 *
 * Written by reading the System V Interface Definition, not the code.
 *
 * Totally public domain.
 *
 */
/*LINTLIBRARY*/

int memcmp(sp1, sp2, n)
register char *sp1, *sp2;
register int n;
{
    int delta;

    if (sp1 != sp2)
	while (--n >= 0)
	    if (delta = *sp1++ - *sp2++)
		return(delta);
    return(0);
}
