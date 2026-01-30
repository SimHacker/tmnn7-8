/*****************************************************************************

NAME
   df -- return count of free disk blocks in a file system

SYNOPSIS
   long df(dir)
   char *dir;

DESCRIPTION
   The df() function tries to find and return the number of free 1K blocks
available on the file system containing the given directory. It will
return -errno if its attempt is unsuccessful.

BUGS
   This code breaks on V7 because neither ustat(2) nor the enhancement of
df(1) that interprets arbitrary directory names correctly are supported. Only
arguments that are names of filesystem roots will work.

   On AT&T UNIX this function returns a count of physical blocks free rather
than (more useful) logical 1KB blocks. This makes it inaccurate on old-style
512KB file systems.

   The BSD code is dependent on vendor whim, because it parses the output
format of the ever-changing 'df' command. The assumptions are:

    1) df, when given a physical or logical file system name as argument,
will print out a data line relating to that file system only.

    2) header lines on the output, if any, never begin with a /.

AUTHOR
   Eric S. Raymond
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

*****************************************************************************/
/* LINTLIBRARY */
#include "libport.h"

/* insert new formats for hacked df(1) implementations here */
#ifdef BSD4_3
#define DFSCAN	"%*s %*ld %*ld %ld"
#else
#ifdef BSD4_2
#define DFSCAN	"%*s %*ld %*ld %s, %*ld %*s %*s"
#endif
#endif
#ifndef BSD4_2
#ifndef BSD4_3
#define DFSCAN	"%*s %*s %ld"
#endif
#endif

long df(dir)
/* return amount of free blocks on a given file system */
char	*dir;
{
#ifdef USTAT			/*  we have ustat(2) available  */
#include <ustat.h>
    struct stat	    st;
    struct ustat    ust;

    if (stat(dir, &st) || ustat(st.st_dev, &ust))
	return(-errno);
    else
	return(ust.f_tfree);
#else
#ifdef STATFS			/* he who has NFS has also STATFS */
#ifdef USG
#include <sys/statfs.h>
#else
#include <sys/vfs.h>
#endif /* !USG */
    struct statfs    buf;
    extern int statfs();

    if (statfs(dir, &buf))
	return(-errno);
    else
#ifdef USG
	return( buf.f_bsize * buf.f_bfree / 1024 );
#else
	return( buf.f_bsize * buf.f_bavail / 1024 );    
#endif /* !USG */
#else		/* oh, foo, we're on a 4.2 or 4.3BSD system with no NFS */
    FILE *fp;
    long freeblks = FAIL;
    char bfr[BUFSIZ];

    (void) sprintf(bfr, "df %s", dir);

    /* take our output from the df(1) command corresponding to the call */
    if ((fp = peopen(bfr, "r")) == (FILE *)NULL)
	return(FAIL);

    /* skip leading header lines */
    while (fgets(bfr, sizeof(bfr), fp) && bfr[0] != '/')
	continue;

    /* if we got here, we should be looking at the right line */
    if (sscanf(bfr, DFSCAN, &freeblks) != 1)
	freeblks = FAIL;

    (void) peclose(fp);
    return(freeblks);
#endif
#endif
}

/* df.c ends here */
