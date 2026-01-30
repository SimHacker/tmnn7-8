/*
 * Linear search algorithm, generalized from Knuth (6.1) Algorithm S.
 *
 * Written by reading the System V Interface Definition, not the code.
 *
 * Totally public domain.
 */
/*LINTLIBRARY*/

char *lsearch(key, base, nelp, width, compar)
register char *key;		/* key to be searched for */
register char *base;		/* base of table */
unsigned *nelp;			/* pointer to current table size */
unsigned width;			/* size of an element (chars) */
int (*compar)();		/* ordering function */
{
    register char *next;
    extern char *memcpy();

    for (next = base + *nelp * width ; base < next; base += width)
	if ((*compar)(key, base) == 0)
	    return (base);			/* we found it! */
    ++*nelp;					/* key not found, add it */
    return(memcpy(base, key, (int)width));	/* bump next ptr */
}

/* lsearch.c ends here */
