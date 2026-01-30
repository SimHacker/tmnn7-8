/* fwait.c -- secure wait on process termination */
/* LINTLIBRARY */
#include "system.h"

int fwait(pid)
/* wait on a child process, shielding it from SIGINT/SIGHUP/SIGALRM */
register int pid;
{
    register int w;
    wait_t status;
    catch_t (*onalrm)(), (*onint)(), (*onhup)();

    onalrm = signal(SIGALRM, SIGCAST(SIG_IGN));
    onint = signal(SIGINT, SIGCAST(SIG_IGN));
    onhup = signal(SIGHUP, SIGCAST(SIG_IGN));

    while ((w = wait(&status)) != pid && w != -1)
	continue;
    if (w == -1)
	status.w_status = -1;

    (void) signal(SIGHUP, SIGCAST(onhup));
    (void) signal(SIGINT, SIGCAST(onint));
    (void) signal(SIGALRM, SIGCAST(onalrm));
    return(status.w_status);
}

/* fwait.c ends here */
