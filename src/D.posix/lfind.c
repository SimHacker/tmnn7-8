/*
 * Linear search algorithm, generalized from Knuth (6.1) Algorithm S.
 *
 * Written by reading the System V Interface Definition, not the code.
 *
 * Totally public domain.
 */
/*LINTLIBRARY*/

extern char *memcpy();

char *lfind(key, base, nelp, width, compar)
register char * key;		/* key to be searched for */
register char * base;		/* base of table */
unsigned *nelp;			/* char * to current table size */
unsigned width;			/* width of an element (bytes) */
int (*compar)();		/* ordering function */
{
    register char *next;

    for (next = base + *nelp * width ; base < next; base += width)
	if ((*compar)(key, base) == 0)
	    return (base);	/* we found it! */
    return (char *)(-1);	/* it's not there */
}

/* lfind.c ends here */
