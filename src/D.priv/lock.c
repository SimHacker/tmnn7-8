/*****************************************************************************

NAME
   lock.c -- calls for portable exclusive-use lock (mandatory)

SYNOPSIS
   void lock();	    -- assert a lock, wait till you get it

   void unlock();   -- remove a previous lock

   bool lockp();    -- returns true if we are currently asserting a lock
   
   int lcount();    -- count # of processes waiting (SV version only)

DESCRIPTION
   These routines may be used to lock critical resources for exclusive use.
The lock() and unlock() are approximately P and V operations on a single
integer-valued semaphore (approximate because the implementations' efforts
to guarantee atomicity are of varying degrees of bogosity).

BUGS
   The Version 7/System III implementation is ugly and flaky.

REVISED BY
   Eric S. Raymond
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.
   Thanks to paul@devon.UUCP for the XENIX semaphore code.

*****************************************************************************/
/* LINTLIBRARY */
#include "news.h"
#include "libpriv.h"

/*
 * News system locking.
 */
private int	lockcount = 0;		/* no. of times we've called lock */

bool lockp()
{
    return(lockcount > 0);
}

#ifdef SVIDSEMS	/* can do mandatory locking using the semaphore facilities */
#define SEMAPHORES

#include <sys/ipc.h>
#include <sys/sem.h>

#define NEWSKEY	    2317L   /* Hail Eris, All Hail Discordia! */

private int semid;

void lock()
{
    static struct sembuf lockop =
    {
	0,		/* there's only one semaphore */
	-1,		/* we want the simplest kind of decrement */
	SEM_UNDO,	/* yes, we want exit(0) to undo this */
    };

    if (lockcount++ == 0)
    {
	/* if the semaphore exists, get it */
	if ((semid = semget((key_t)NEWSKEY, 1, 00660)) == FAIL)
	{
	    /* semaphore did not exist, lets try to create it */
	    if ((semid = semget((key_t)NEWSKEY, 1, IPC_CREAT | 00660)) == FAIL)
		xerror1("could not get news lock semaphore, errno = %d",errno);

	    /*
	     * If we just created the semaphore, set its value to 1 (so that
	     * exactly one locked() process can be running at any given time)
	     */
	    (void) semctl(semid, 0, SETVAL, 1);
	}

	/* now try to assert the lock */
#ifdef lint	/* 3B1 llib-lc is deluded about the type of semop's arg 2 */
	if (lockop.sem_op)
#else
	if (semop(semid, &lockop, 1) == FAIL)	
#endif /* lint */
	    xerror1("lock operation aborted, errno = %d", errno);
    }
}

void unlock()
{
    static struct sembuf unlockop =
    {
	0,		/* there's only one semaphore */
	1,		/* we want the simplest kind of increment */
	SEM_UNDO,	/* yes, we want exit(0) to undo this */
    };
#ifdef NDEBUG
    assert(lockcount>0);
#endif
    
    if ((lockcount > 0) && (--lockcount == 0))
    {
#ifdef lint	/* SVr1 llib-lc is deluded about the type of semop's arg 2 */
	if (unlockop.sem_op)
#else
	if (semop(semid, &unlockop, 1) == FAIL)
#endif /* lint */
	    xerror1("unlock operation aborted, errno = %d", errno);

#ifndef lint	/* the #$@%#! sem.h on SVr1 doesn't define struct sembuf! */
	/* if we're the last process waiting on the semaphore, flush it */
	if (semctl(semid, 0, GETNCNT, 0) == 0)
	    (void) semctl(semid, 0, IPC_RMID, 0);
#endif /* lint */
    }
}

int lcount()
/* if the return of this is zero, we're the only locking process running */
{
    return(semctl(semid, 0, GETNCNT, 0));
}
#endif /* SVIDSEMS */

#if defined(XENIXSEMS) && !defined(SEMAPHORES)
#define SEMAPHORES

private char sem_name[] = "/tmp/.active_lock";
private int sem_num;

void lock()	/* get (or wait for) control of the resource */
{
    if (lockcount++ == 0) {
	/* try to create the semaphore */
	if ((sem_num = creatsem(sem_name, 0660)) == FAIL) {
	    if (errno == EEXIST) {
		/* semaphore exists, open it */
		if ((sem_num = opensem(sem_name)) == FAIL) {
		    xerror1("semaphore open failed; errno = %d", errno);
		}
	    } else {
		xerror1("semaphore create failed; errno = %d", errno);
	    }
	}
	/*
	 * Sem_name was either created or opened.  Now call waitsem()
	 * which will return as soon as this process may access the
	 * resource.  Hence, if this process created (as opposed to
	 * opened) the semaphore, waitsem will return immediately.
	 */
	(void) waitsem(sem_num);
    }
}

void unlock()	/* release control of the resource */
{
#ifdef NDEBUG
    assert(lockcount>0);
#endif
    
    if ((lockcount > 0) && (--lockcount == 0)) {
	(void) sigsem(sem_num);
    }
}

#endif	/* XENIXSEMS */

#ifndef SEMAPHORES
/*
 * This is the generic implementation in terms of filelock().
 * All V7, SIII and BSD systems up to 4.3 will use this.
 */
#ifdef	VMS
#define	NEWSLOCK	"/tmp/netnews.lck.1"
#else
#define NEWSLOCK	"/tmp/NEWSLOCK"
#endif /* VMS */

void lock()
{
    if (lockcount++ == 0)
	(void) filelock(NEWSLOCK);
}

void unlock()
{
#ifdef NDEBUG
    assert(lockcount > 0);
#endif
	
    if ((lockcount > 0) && (--lockcount == 0))
	(void) fileunlock(NEWSLOCK);
}

#endif /* !SEMAPHORES */

#ifdef MAIN
#include <stdio.h>

char	*Progname = "lock";

main(argc, argv)
/* test the locking primitives */
int	argc;
char	**argv;
{
    newsinit();
    (void) fprintf(stderr, "locktest: locking process %d\n", getpid());
    lock();
    (void) fprintf(stderr, "locktest: locked.\n");
    (void) sleep(atol(argv[1]));
    (void) fprintf(stderr, "locktest: unlocking process %d\n", getpid());
    unlock();
    (void) fprintf(stderr, "locktest: process %d exiting\n", getpid());
}
#endif /* MAIN */

/* lock.c ends here */
