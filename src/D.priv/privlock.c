/*****************************************************************************

NAME
   privlock.c -- calls for portable exclusive-use lock (mandatory),
		requesting exclusion before waiting for the lock.

SYNOPSIS
   void privlock();  -- request for exclusion, assert a lock,
   			wait till you get it

   void privunlock() -- remove exclusion request, release the lock

   bool privlockcheck() -- check to see if anyone as requested exclusion

DESCRIPTION
   These routines may be used to lock critical resources for exclusive use.
Privlock() creates a flag which signals to rnews that it would be a
Good Idea if it were to release its resources.   Privunlock() release
both the lock and the flag to rnews.   Privlockcheck() returns true if 
the flag is set.
 
WRITTEN BY
   Theodore Ts'o

*****************************************************************************/
/* LINTLIBRARY */
#include "news.h"
#include "libpriv.h"

private	char	*explock = NULL;
private	char	*expsetup = "%s/EXPLOCK";

void privlock()
{
	FILE	*f;
	
	if (!explock) {
		(void) sprintf(bfr, expsetup, site.admdir);
		explock = savestr(bfr);
	}
	if (creat(explock, 0644) == FAIL)
		xerror0("Couldn't create the expire lock file");
	if ((f = fopen(explock, "w")) == NULL)
		xerror0("Couldn't fopen the expire lock file");
	(void) fprintf(f, "%d\n", getpid());
	(void) fclose(f);

	lock();
}

void privunlock()
{
	if (!explock) {
		(void) sprintf(bfr, expsetup, site.admdir);
		explock = savestr(bfr);
	}
	(void) unlink(explock);
	unlock();
}

bool privlockcheck()
{
	if (!explock) {
		(void) sprintf(bfr, expsetup, site.admdir);
		explock = savestr(bfr);
	}
	return(exists(explock));
}


