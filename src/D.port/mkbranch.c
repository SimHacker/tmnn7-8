/****************************************************************************

NAME
   mkbranch.c -- directory-making functions

SYNOPSIS
   #include "libport.h"

   int mkbranch(path, perm) -- recursively make a branch with mkdir()
   char *path; int perm;

DESCRIPTION
   Mkdir() can only make one directory at a time. Mkbranch() will take an
arbitrary path rooted in an accessible directory and make all directories
necessary to create the bottommost one.

BUGS
   It scribbles on its argument.

NOTE
   If you compile this with -DMAIN it will generate an interactive tester
for mkbranch(), see the code below.

REVISED BY
   Eric S. Raymond
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

****************************************************************************/
/* LINTLIBRARY */

#include "libport.h"

int mkbranch(path, perm)
/*
 * make a chain of directories ending in the given full path, and set all
 * such directories to be owned by caller's effective uid and gid.
 */
char *path;
int perm;
{
    register char   *this, *slash, c;
    int		    segcount = 0;

    /* make the directories */
    for (this = path; *this != '\0'; this = slash)
    {
	while (*this == '/')
		this++;
	for (slash = this; (c = *slash) != '\0' && c != '/'; slash++)
		;
	segcount++;
	*slash = '\0';

#ifdef MAIN
	(void) fprintf(stderr, "Checking branch %s\n", path);
#endif /* MAIN */

	if (mkdir(path, perm) == FAIL && errno != EEXIST)
	{
	    *slash = c;
	    return(segcount);
	}
	else
	    *slash = c;
    }

    return(SUCCEED);
}

#ifdef MAIN
main(argc, argv)
int	argc;
char	*argv[];
{
    int dstat, perms;

    /* give user a chance to set the umask before attempting a mkbranch */
    if (strncmp(argv[1], "-u", 2) == SUCCEED)
    {
	(void) sscanf(argv[1] + 2, "%o", &perms);
	(void) fprintf(stderr, "main: setting umask to %o\n", perms);
	(void) umask(perms);
	++argv; --argc;
    }

    if (argc > 2)
	(void) sscanf(argv[2], "%o", &perms);
    else
	perms = 0777;

    if (dstat = mkbranch(argv[1], perms))
    {
	(void) fprintf(stderr,
		    "mkbranch: cannot mkbranch %s: errno = %d on seg %d\n",
		    argv[1], errno, dstat);
	exit(dstat);
    }
}

int logerr(m) char *m; { (void) fprintf(stderr, "%s\n"); }
#endif /* MAIN */

/* mkbranch.c ends here */
