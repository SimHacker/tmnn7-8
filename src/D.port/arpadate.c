/* arpadate.c -- convert timestamp to ARPANET date format */
/* LINTLIBRARY */
#include "system.h"

char *arpadate(longtime)
/*
 * arpadate() is like ctime(3) except that the time is returned in
 * an acceptable ARPANET time format instead of ctime format.
 */
time_t *longtime;
{
    register char  *p, *q, *ud;
    register int    i;
    static char b[40];

    /*  Get current time. This will be used resolve the timezone. */
    ud = asctime(gmtime (longtime));

    /*  Crack the UNIX date line in a singularly unoriginal way. */
    q = b;

#ifdef notdef
/* until every site installs the fix to getdate.y, the day
   of the week can cause time warps */
    p = &ud[0];			/* Mon */
    *q++ = *p++;
    *q++ = *p++;
    *q++ = *p++;
    *q++ = ',';
    *q++ = ' ';
#endif

    p = &ud[8];			/* 16 */
    if (*p == ' ')
	p++;
    else
	*q++ = *p++;
    *q++ = *p++;
    *q++ = ' ';

    p = &ud[4];			/* Sep */
    *q++ = *p++;
    *q++ = *p++;
    *q++ = *p++;
    *q++ = ' ';

    p = &ud[22];		/* 1979 */
    *q++ = *p++;
    *q++ = *p++;
    *q++ = ' ';

    p = &ud[11];		/* 01:03:52 */
    for (i = 8; i > 0; i--)
	*q++ = *p++;

    *q++ = ' ';
    *q++ = 'G';			/* GMT */
    *q++ = 'M';
    *q++ = 'T';
    *q = '\0';

    return(b);
}

/* arpadate.c ends here */
