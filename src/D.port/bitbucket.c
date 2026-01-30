/* bitbucket.c -- tell us if a file pointer is directed to /dev/null */
/*LINTLIBRARY*/
#include "libport.h"

bool bitbucket(fp)
/* return TRUE iff given fp has been redirected to /dev/null. */
FILE	*fp;
{
    struct stat ss, ns;

    if (fstat(fileno(fp), &ss) < 0)
	return FALSE;
    if (stat("/dev/null", &ns) < 0)
	return FALSE;
    if (ss.st_dev == ns.st_dev && ss.st_rdev == ns.st_rdev)
	return(TRUE);
    return(FALSE);
}

/* bitbucket.c ends here */
