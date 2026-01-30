/* errmsg.c -- return error message string corresponding to an errno value */
/* LINTLIBRARY */
#include "libport.h"

char *errmsg(code)
int code;
{
    extern int sys_nerr;
    extern char *sys_errlist[];
    static char ebuf[6+5+1];

    if (code > sys_nerr)
    {
	(void) sprintf(ebuf, "Error %d", code);
	return(ebuf);
    }
    else
	return(sys_errlist[code]);
}

/* errmsg.c ends here */
