/*
 * ssignal -- user-settable software signals facility
 *
 * Written by reading the System V Interface Definition, not the code.
 *
 * Totally public domain.
 */
/*LINTLIBRARY*/
#include <signal.h>

#define SIGMAX 16

static int (*sigs[SIGMAX])();

int (*ssignal(sig, fn))()
register int sig, (*fn)();
{
    register int (*oldfn)();

    if(sig >= 1 && sig <= SIGMAX)
    {
	oldfn = sigs[sig - 1];
	sigs[sig - 1] = fn;
    }
    else
	oldfn = SIG_DFL;
    return(oldfn);
}

int gsignal(sig)
register int sig;
{
    register int (*sigfn)();

    if (sig < 1 || sig > SIGMAX || (sigfn = sigs[sig - 1]) == SIG_DFL)
	return(0);
    else if (sigfn == SIG_IGN)
	return(1);
    else
    {
	sigs[sig - 1] = SIG_DFL;
	return((*sigfn)(sig));
    }
}

/* ssignal.c ends here */
