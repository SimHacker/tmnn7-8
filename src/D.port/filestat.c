/* filestat.c -- file  status querying functions */
/*LINTLIBRARY*/
#include "libport.h"

static struct stat fildat;

bool exists(fn)
/* does the given file exist? */
char *fn;
{
     return(stat(fn, &fildat) == SUCCEED);
}

bool isdir(fn)
/* is the given file a directory? */
char *fn;
{
    if (stat(fn, &fildat) == FAIL)
	return(FALSE);
    else
	return((fildat.st_mode & S_IFMT) == S_IFDIR);
}

off_t filesize(fn)
/* return the size of the file */
char *fn;
{
    if (stat(fn, &fildat) == FAIL)
	return((off_t)FAIL);
    else
	return(fildat.st_size);
}

time_t modtime(fn)
/* return the last-modified-time of the file */
char *fn;
{
    if (stat(fn, &fildat) == FAIL)
	return((time_t)FAIL);
    else
	return(fildat.st_mtime);
}

void setmodtime(fn, adate)
/* set the last-modified-time of the file */
char *fn;
time_t adate;
{
    struct {time_t actime; time_t modtime;} utimes;

    utimes.actime = utimes.modtime = adate;
    (void) utime(fn, &utimes);
}

/* filestat.c ends here */

