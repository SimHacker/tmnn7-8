/*
 * Local copy of strchr (a.k.a. index) for portability.
 * Totally public domain.
 *
 * Written by reading the System V Interface Definition, not the code.
 *
 * Totally public domain.
 */

#include <stdio.h>

char *
strchr(s, c)
char *s, c;
{
    char *x = s;

    while (*x != c)
	if (*x == '\0')
	    return(NULL);
	else
	    ++x;
    return(x);
}

