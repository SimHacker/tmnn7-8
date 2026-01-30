/*****************************************************************************

NAME
   environ.c -- general environment manipulation

SYNOPSIS
    bool setenv(var, val)	-- set a variable in the environment
    char *var, val;

    bool delenv(var)		-- delete a variable
    char *var,

DESCRIPTION
    These functions modify the environment seen by programs executing at and
under the current shell. They both return TRUE if the variable being
manipulated was defined before the modification, FALSE otherwise.

NOTE
   These functions use malloc(3). The space used by old variable definitions
is not reclaimed when a new one is written or a delete is performed.

SEE ALSO
   getenv(3)		-- get the value of an environment variable

REVISED BY
   Eric S. Raymond
from original code posted by Ron Natalie.
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

*****************************************************************************/
/* LINTLIBRARY */
#include "libport.h"

extern char **environ;		    /*  Pointer to environment, set by crt0
				     *  used by execv, execl
				     */
static bool _set_env = FALSE;	    /*  When FALSE, indicates that the environ
				     *  points to the original stack resident
				     *  environment.  We set it to TRUE so that
				     *  we can free memory we malloc'd on
				     *  subsequent calls to setenv.
				     */

static char **findenv(var)
/* search the environment for the definition of a given variable */
char	*var;
{
    register char	**e;
    int			len = strlen(var);

    for (e = environ; *e; e++)
    {
	if (strncmp(var, *e, len) == 0)
	    return(e);
    }
    return((char **)0);
}

/*
 *  setenv - add an element to the environment
 *
 *  var is added to the environment.  It should be a string of
 *  the form "NAME=value".  Note, that the actual string is not
 *  copied, but a pointer is used, so be careful not to overwrite
 *  it before the exec.  Also, this does not check to make sure
 *  one doesn't already exist, so delenv first if you're not sure.
 */
bool setenv(var, val)
char	*var, *val;
{
    char	**e, **n;
    int		count;
    char	**new;

    if ((e = findenv(var)) != (char **)0)
    {
	/*
	 * The variable is already defined. No need to copy over the
	 * environment, we'll just overwrite the pointer to the definition.
	 */
	*e = malloc((unsigned) (strlen(var) + strlen(val) + 2*sizeof(char)));
	(void) sprintf(*e, "%s=%s", var, val);
	return(TRUE);
    }
    else
    {
	/*
	 *  We must add a new variable definition to the environment.
         *  Count the number of items in the environment and malloc
	 *  room for all those plus the one we are adding.
	 */
	for (e = environ, count = 0; *e; e++)
	    count++;
#ifndef lint
	new = (char **)malloc((unsigned) ((count+2)*sizeof (char *)));
#else
	new = (char **)NULL;
#endif /* lint */

	/* copy over the pointers from the old to the new */
	for (e = environ, n = new; count; count--)
	    *n++ = *e++;

	/* now add the new one */
	*n = malloc(strlen(var) + strlen(val) + 2*sizeof(char));
	(void) sprintf(*n++, "%s=%s", var, val);
	*n = (char *)0;

	/*
	 *  If we had allocated this environ from a previous call,
	 *  free it now.
	 */
	if (_set_env)
	    (void) free((char *)environ);
	else
	    _set_env = TRUE;
	environ = new;
	return(FALSE);
    }
}

/*
 *  delenv - delete an environment variable.
 *
 *  Var should be the name of an environment variable such as "PATH"
 *  Note, that the space is not reclaimed, but this doesn't happen all
 *  that much.
 */
bool delenv(var)
char	*var;
{
    char	**e;
    /*
     *  Check that the given variable is actually defined; if so,
     *  move everything after it, including the null terminator
     *  down one, obliterating the deleted pointer.
     */
    if ((e = findenv(var)) == (char **)0)
	return(FALSE);
    else
    {
	while(*e)
	{
	    *e = e[1];
	    e++;
	}
	return(TRUE);
    }
}

/* environ.c ends here */
