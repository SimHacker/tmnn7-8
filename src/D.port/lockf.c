/**********************************************************************

NAME
   lockf.c -- Xenix implementation of lockf()

SYNOPSIS
   int lockf(fd, func, length)

DESCRIPTION
   This routine is compatible with the /usr/group system call lockf().
It implements the lockf() semantics using the Xenix locking() call.

NOTE
   This routine is also useful for the AT&T UNIX PC, which for some bizarre
reason implements locking() but not lockf().

AUTHOR
   Chip Salzenberg (chip@ateng).
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

************************************************************************/
/*LINTLIBRARY*/
#include "libport.h"

#ifdef M_XENIX
#include <sys/locking.h>
#else
/* goddess help us if they ever change these on the 3B1... */
#define LK_UNLCK	0	/* release lock */
#define LK_LOCK		1	/* mandatory lock */
#define LK_NBLCK	2	/* advisory lock */
#endif /* M_XENIX */

int lockf(fd, func, length)
int fd, func;
long length;
{
    switch (func)
    {
    case F_ULOCK: return(locking(fd, LK_UNLCK, length));
    case F_LOCK:  return(locking(fd, LK_LOCK, length));
    case F_TLOCK: return(locking(fd, LK_NBLCK, length));
    default: errno = EINVAL; return(FAIL);
    }
}

/* lockf.c ends here */
