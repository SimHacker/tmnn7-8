/*****************************************************************************

NAME
   filelock.c -- calls for portable mandatory exclusive locking on a file

SYNOPSIS
   int filelock(file);		-- assert a lock, wait till you get it
   char *file;

   bool filecheck(file)		-- check the lockedness of a file
   char *file;

   int fileunlock(file);	-- remove a previous lock on a file
   char *file;

DESCRIPTION
   These routines may be used to lock a file for exclusive use. For back-
compatibility with SIII and V7 systems, the last segment of a locked filename
is only significant to 9 characters -- that is, locks made for files that
differ only in the 10th or later characters may step on each other.

   A failure return from either routine indicates serious lossage, probably
due to stray filelocks left over from a crash, exhaustion of file descriptors,
or file system trouble. Accordingly, failures are logged through logerr().

BUGS
   The non-LOCKF/LOCK_EX implementations are ugly and flaky.

WRITTEN BY
   Eric S. Raymond and Theodore Ts'o.
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.
*****************************************************************************/
/*LINTLIBRARY*/
#include "libport.h"
#include "libpriv.h"

#define NAMELIMIT	9	/* 14, - 5 suffix chars */ 

#undef HAVELOCKS

#ifdef LOCKF
#define HAVELOCKS
/*
 * First and best, the SVID/POSIX implementation. This doesn't depend
 * on the assumption that everybody is polite.
 */
int filelock(file)
char *file;
{
    int	fd;

    if ((fd = open(file, O_RDWR | O_CREAT, 0600)) < 0)
    {
	logerr1("filelock: open: errno = %d", errno);
	return(FAIL);
    }
    (void) lockf(fd, F_LOCK, filesize(file));
    return(SUCCEED);
}

bool filecheck(file)
char *file;
{
    int	fd;
    bool locked;

    if ((fd = open(file, O_RDWR, 0)) < 0)
	return(FALSE);
    locked = (lockf(fd, F_TEST, filesize(file)) == FAIL);
    (void) close(fd);
    return(!locked);
}

int fileunlock(file)
char	*file;
{
    int	fd;

    if ((fd = open(file, O_RDONLY, 0)) < 0)
    {
	logerr1("fileunlock: open: errno = %d", errno);
	return(FAIL);
    }
    (void) lockf(fd, F_ULOCK, filesize(file));
    return(SUCCEED);
}

#endif /* LOCKF */

#ifdef BSD4_2
#define HAVELOCKS
/*
 * Next the BSD version, this should work fine also.
 */
int filelock(file)
char *file;
{
    int fd;

    if ((fd = open(file, 0)) == FAIL)
    {
	logerr1("filelock: open: errno = %d", errno);
	return(FAIL);
    }

    /* This will sleep until the other program releases the lock */
    /* We may need to alarm out of this, but I don't think so */
    return(flock(fd, LOCK_EX));
}

bool filecheck(file)
char *file;
{
    int fd;

    /* I hope this is portable back to pre-4.1C BSD versions... */
    if ((fd = open(file, O_NDELAY)) == FAIL)
	return(TRUE);
    else
    {
	(void) fclose(fd);
	return(FALSE);
    }
}

int fileunlock(file)
char *file;
{
    int fd;

    if ((fd = open(file, 0)) == FAIL)
    {
	logerr1("fileunlock: open: errno = %d", errno);
	return(FAIL);
    }

    (void) flock(fd, LOCK_UN);
    (void) close(fd);
    return(SUCCEED);
}
#endif /* BSD4_2 */

#ifdef VMS
#define HAVELOCKS
/*
 * OK, the VMS version assumes that everybody is polite, that is that all
 * functions attempting to access a lockable file call this function first.
 * Even so we have to time out the lock -- stray lock files could have got
 * left behind in a crash.
 *
 * These routines are different from the V7/SIII versions below because we can
 * not effectively simulate links, and VMS supports multiple version numbers
 * of files.
 */
int filelock(file)
char *file;
{
    register int i, fd;

    (void) strlcpy(bfr, file, LBUFLEN);
    (void) strlcat(bfr, "-LCK.1", LBUFLEN);

    i = DEADTIME;
    while ((fd = creat(bfr, 0444)) < 0)
    {
	if (--i < 0)
	{
	    (void) unlink(bfr);
	    logerr1("timeout during lock attempt on %s", file);
	    return(FAIL);
	}
	if (i < -3)
	{
	    logerr1("lock attempt on %s failed", file);
	    return(FAIL);
	}
	(void) sleep((unsigned)1);
    }
    (void) close(fd);
    return(SUCCEED);
}

bool filecheck(file)
file *file;
{
    (void) strlcpy(bfr, file, LBUFLEN);
    (void) strlcat(bfr, "-LCK.1", LBUFLEN);
    return(!exists(bfr));
}

void fileunlock(file)
char *file;
{
    (void) strlcpy(bfr, file, LBUFLEN);
    (void) strlcat(bfr, "-LCK.1", LBUFLEN);  /* was strcpy - bug in original */
    (void) unlink(bfr);
    return(SUCCEED);
}
#endif /* VMS */

#ifndef HAVELOCKS
/*
 * USG UNIX versions up to Vr2 don't have mandatory file locking or semaphore
 * operations. Thus, this assumes that everybody is polite: that is that all
 * functions attempting to access a lockable file call this function first.
 * Even so we have to time out the lock -- stray lock files could have got
 * left behind in a crash.
 */
int filelock(file)
char *file;
{
    register int i;
    int	fd;
    char tmplock[BUFLEN], procfile[BUFLEN];

    (void) strncpy(procfile, file, NAMELIMIT);
    (void) sprintf(procfile + strlen(file), "..%d", file, getpid());
    if ((fd = creat(procfile, 0666)) == FAIL)
    {
	logerr1("filelock: creat: errno = %d", errno);
	return(FAIL);
    }
    else
	(void) close(fd);

    (void) snprintf(tmplock, sizeof(tmplock), "/tmp/%s..LCK", file);

    /*
     * And because the OS doesn't know our lock file is wedded to
     * the caller's PID, something like a kill -9 can leave the lock
     * in place forever. Thus we have to time out after some period
     * of failing. Gawd, this is ugly.
     */
    i = DEADTIME;
    while (link(procfile, tmplock) == FAIL)
    {
	if (errno != EEXIST)
	    break;
	if (--i < 0)
	{
	    logerr1("lock attempt on %s timed out", file);
	    return(FAIL);
	}
	(void) sleep((unsigned)1);
    }
}

bool filecheck(file)
char	*file; 
{
    (void) snprintf(bfr, LBUFLEN, "/tmp/%s..LCK", file);
    return(!exists(bfr));
}

int fileunlock(file)
char *file;
{
    char tmplock[BUFLEN], procfile[BUFLEN];
    struct stat stbuf;
    int fd;

    (void) strncpy(procfile, file, NAMELIMIT);
    (void) snprintf(procfile + strlen(file), sizeof(procfile) - strlen(file), "..%d", getpid());

    if (stat(procfile, &stbuf) == FAIL)
	return(FAIL);
    else if (stbuf.st_nlink == 2)
    {
	(void) snprintf(tmplock, sizeof(tmplock), "/tmp/%s..LCK", file);
	(void) unlink(tmplock);
    }

    (void) unlink(procfile);
    return(SUCCEED);
}
#endif /* !HAVELOCKS */

/* filelock.c ends here */
