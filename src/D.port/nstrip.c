/* nstrip.c -- strip trailing whitespace from a string  */
/* LINTLIBRARY */
#include "libport.h"

nstrip(s)
/*
 * Strip trailing newlines, blanks, and tabs from 's'.
 * Return TRUE if newline was found, else FALSE.
 */
register char *s;
{
	register char *p;
	register bool rc;

	rc = FALSE;
	p = s;
	while (*p)
		if (*p++ == '\n')
			rc = TRUE;
	while (--p >= s && (*p == '\n' || *p == ' ' || *p == '\t'));
	*++p = '\0';
	return rc;
}

/* nstrip.c ends here */
