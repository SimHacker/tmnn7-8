/* fcopy.c -- perform fast (unbuffered) file-to-file copy */
/* LINTLIBRARY */
#include "libport.h"

extern int errno;

int fcopy(fn, newname)
char *fn, *newname;
{
    int f1, f2, r;
    char buf[BUFSIZ];

    if ((f1 = open(fn, 0)) < 0)
	return(FAIL);

    if ((f2 = open(newname, 1)) < 0)
    {
	if (errno == ENOENT)
	{
	    if ((f2 = creat(newname,0644)) < 0) 
	    {
		(void) close(f1);
		return(FAIL);
	    }
	}
	else
	{
	    (void) close(f1);
	    return(FAIL);
	}
    }

    while((r = read(f1, buf, (iolen_t) BUFSIZ)) > 0)
	(void) write(f2, buf, (iolen_t) r);

    (void) close(f1);
    (void) close(f2);
    return (r == 0) ? SUCCEED : FAIL;
}

/* fcopy.c ends here */
