/* LINTLIBRARY */
/*
 *  This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
 * of protecting free redistribution; see the LICENSE file for details.
 */
#include "libport.h"

bool prefix(full, pref)
/* return TRUE if pref is a prefix of full, FALSE otherwise */
register char *full, *pref;
{
    register char fc, pc;

    while ((pc = *pref++) != '\0')
    {
	fc = *full++;
	if (isupper(fc))
	    fc = tolower(fc);
	if (isupper(pc))
	    pc = tolower(pc);
	if (fc != pc)
	    return(FALSE);
    }
    return(TRUE);
}

/* prefix.c ends here */
