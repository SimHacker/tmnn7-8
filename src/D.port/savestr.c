/* savestr.c -- return a pointer to an allocated copy of a given string */
/*
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.
*/
/* LINTLIBRARY */
#include "libport.h"

char *savestr(cp)
register char *cp;
{
    register char *mp;

    if ((mp = malloc((unsigned) strlen(cp) + 1)) == (char *)0)
	xerror1("malloc failed on %s", cp);

    (void) strcpy(mp, cp);
    return(mp);
}

/* savestr.c ends here */
