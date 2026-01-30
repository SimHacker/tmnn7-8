/****************************************************************************

NAME
   mkdir.c -- emulation of BSD/SVr3-style mkdir(2) system call

SYNOPSIS
   int mkdir(path, perm)    -- make a directory with given permissions
   char *path; int perm;

DESCRIPTION
   The mkdir() function is used for making directories on systems that
don't have a mkdir(2) call (that is, V7 and USG systems before V.3). It tries
to act as much like mkdir(2) as possible. Due to various bogosities (why, oh
why wasn't mknod(2) for a directory made a non-privileged call?) it cannot
completely succeed. It returns 0 on success and -1 on failure but
only detect the following ERRNO conditions; ENOENT, EEXIST, EACCESS, ENOTDIR.

   Note: this function emulates the SVr3 behavior (group ID of the directory
is the effective group ID of the calling process) rather than the BSD
behavior (group ID of the directory is the group ID of the parent directory).

   On USG it will succeed if the real or apparent uid of the calling process
has write privileges in the current directory (the latter case is implemented
by a kluge that tries to set the parent directory's permissions to 0777, 'ware
security holes!). The new directory will be owned by the effective uid.

   On V7 (because it restricts the chown(2) call and mkdir(1) makes
directories owned by the real ID of its caller) the call will only succeed
if the real ID matches, and the new directory will be owned by the real
uid.

REVISED BY
   Eric S. Raymond

****************************************************************************/
/* LINTLIBRARY */
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <signal.h>

extern int errno;

static int fwait(pid)
/* wait on a child process, shielding it from SIGINT and SIGHUP */
register int pid;
{
    register int w;
    int status;

    while ((w = wait(&status)) != pid && w != -1)
	continue;
    if (w == -1)
	status = -1;

    return(status);
}

int mkdir(path, perm)
char *path;
int perm;
{
    int		    status, pid;
    int		    uid = getuid(), gid = getgid();
#ifndef lint
    int		    euid = geteuid();
    int		    egid = getegid();
#endif /* lint */
    struct stat	    pstat, sbuf;
    register char   *p;
    char	    parent[200];
    extern char	    *strrchr();

#ifdef MAIN
    (void) fprintf(stderr, "Attempting to mkdir %s\n", path);
#endif /* MAIN */

    errno = 0;

    /*
     * check that the directory doesn't already exist, so that the
     * do loop below must run at least once
     */
    if (stat(path, &sbuf) == 0)
    {
	errno = EEXIST;
#ifdef MAIN
	(void) fprintf(stderr, "Directory %s exists\n", path);
#endif /* MAIN */
	return(-1);
    }

    /* construct the parent's name */
    if (p = strrchr(path, '/'))
    {
	*p = '\0';
	(void) strcpy(parent, path);
	*p = '/';
    }
    else
	(void) strcpy(parent, ".");

    if (stat(parent, &pstat) == -1)    /* check that the parent exists */
    {
	errno = ENOENT;
	return(-1);
    }
    else if (!(pstat.st_mode & S_IFDIR))    /* and that it's a directory */
    {
	errno = ENOTDIR;
	return(-1);
    }

#ifdef USG
    /*
     * If the parent directory is 755 (rwxr-xr-x) the mkdir(1) below
     * will probably fail because it will get suid'd to our real uid, which
     * is random (and thus probably won't match the parent owner's).
     * So we have to temporarily chmod the parent to 777 (rwxrwxrwx).
     */
    if (sbuf.st_uid != uid)
    {
	if (chmod(parent, 0777) == -1)
	{
	    errno = EACCES;
	    return(-1);
	}
    }
#endif /* USG */

    /* now we can make the new directory */
    if (pid = fork())	/* parent side */
    {
	if (pid == -1)
	    return(-1);

	status = fwait(pid);	/* wait till mkdir child is done */

#ifdef USG
	/*
	 * Spawn another child to set ownership correctly -- we do this so
	 * that it gets set to effective ID even if we're running su. This
	 * only works where chown(2) can be called to give files away by a
	 * non-superuser.
	 */
	if (pid = fork())
	    (void) fwait(pid);
	else {
#ifndef lint
	    int		    oldumask = umask(0777);
#endif /* lint */

	    (void) stat(".", &pstat);
	    (void) setuid(uid);
	    (void) setgid(gid);
#ifndef lint	    /* USG lints disagree about 2nd argument types */
	    (void) chmod(path, (unsigned)(perm & ~oldumask));
	    (void) chown(path, euid, egid);
	    (void) umask(oldumask);
#endif /* lint */
	    _exit(0);
	}
#endif /* USG */
    } else {		/* child side */
	(void) close(1);	/* stdout */
	(void) close(2);	/* stderr */
	(void) execlp("mkdir", "mkdir", path, (char *)0);
	perror(path);
	_exit(1);
    }

#ifndef lint	/* USG lints disagree about the type of arg 2 */
#ifdef USG
    if (sbuf.st_uid != uid)
	(void) chmod(parent, (unsigned) pstat.st_mode);	/* put it back */
#endif /* USG */
#endif /* lint */

    return(status);
}

/* mkdir.c ends here */
