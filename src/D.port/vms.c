/* LINTLIBRARY */
#include "libport.h"

#ifdef VMS
/*
 * vmslink allows simulation of file linking under VMS.
 */
int vmslink(infile,outfile)
char *infile, *outfile;
{
    FILE *fp;

    if (access(outfile, F_OK) == 0)
    {
	errno = EEXIST;
	return(FAIL);
    }

    fp = fopen(outfile, "w");
    if (fp == (FILE *)NULL)
    {
	errno = EACCES;
	return(FAIL);
    }

    (void) fprintf(fp, "%s", infile);
    (void) fclose(fp);

    return(SUCCEED);
}

/*
 * vmsdelete deletes all revisions of a file.  It attempts to
 * appear as unlink(2) under conventional Unix in other respects.
 */
int vmsdelete(file)
char *file;
{
    int i;

    if ((i = unlink(file)) != 0)
	return(i);

    i = errno;
    while (unlink(file) == 0)
	continue;
    errno = i;

    return(SUCCEED);
}

/*
 * Convert a Unix file to a VMS fixed record format file by
 * executing the 'unixtovms' command.
 */
int unixtovms(file)
char *file;
{
    char buf[BUFLEN];

    (void) sprintf(buf, "exec /etc/unixtovms %s", file);
    (void) fflush(stdout);	/* V7 system() doesn't flush stdout */
    return(system(buf));
}

/*
 * Convert a VMS fixed record format file to a Unix file by
 * executing the 'vmstounix' command.
 */
int vmstounix(file)
char *file;
{
    char buf[BUFLEN];
    (void) sprintf(buf,"exec /etc/vmstounix %s", file);
    (void) fflush(stdout);	/* V7 system() doesn't flush stdout */
    return(system(buf));
}
#endif /* VMS */

