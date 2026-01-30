/* lcase.c -- convert a string to lowercase */
/* LINTLIBRARY */
#include "libport.h"

void lcase(s)
register char *s;
{
    for (; *s; s++)
	if (isascii(*s) && isupper(*s))
	    *s = tolower(*s);
}

/* lcase.c ends here */
