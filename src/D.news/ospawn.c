/*****************************************************************************

NAME
   ospawn.c -- program interface for originating news

SYNOPSIS
   #include "news.h"

   char *ospawn(cmd)		-- call child with -o arg, read back a line
   char *cmd;

DESCRIPTION
   This code spawns a process in foreground to execute the given command. The
command must accept an option of the form -o <num> on which a line of status
data is written back. A pointer to this status data is returned. The calling
process is protected from SIGINT and SIGQUIT while the child is executing.
This code *will* do a PATH search to find the command.

AUTHOR
   Eric S. Raymond
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

*****************************************************************************/
/*LINTLIBRARY*/
#include "news.h"
#include "procopts.h"

char *ospawn(cmd)
char	*cmd;		/* article to transmit */
{
    int	    	child, opipe[2];
    char	*argv[MAXARGS];
    static char	returnbuf[BUFLEN];

    /*
     * We do things the slightly harder way, not using system(3),
     * in order to be exactly sure of what's going on
     */
    if ((pipe(opipe) == FAIL) || (child = vfork()) == FAIL)
	return((char *)NULL);
    else if (child)	/* the parent side */
    {
	catch_t (*savequit)(), (*saveint)(), (*savealrm)();

	savequit = signal(SIGQUIT, SIGCAST(SIG_IGN));
	saveint = signal(SIGINT, SIGCAST(SIG_IGN));
	savealrm = signal(SIGALRM, SIGCAST(SIG_IGN));

	(void) close(opipe[1]);		/* won't need to write to child */
	(void) fwait(child);		/* hang out till child croaks */

	/* grab the generated ID written by rnews -o off the pipe */
	(void) read(opipe[0], returnbuf, sizeof(returnbuf));

	(void) signal(SIGQUIT, SIGCAST(savequit));
	(void) signal(SIGINT, SIGCAST(saveint));
	(void) signal(SIGINT, SIGCAST(savealrm));

#ifdef DEBUG
	if (verbose)
	    (void) fprintf(stderr,
			   "ospawn: command \"%s\" returns %s\n",
			   cmd, returnbuf);
#endif /* DEBUG */
	return(returnbuf);
    }
    else		/* the child side */
    {
	(void) close(opipe[0]);

	/* time for the actual child invocation */
	(void) sprintf(bfr, "%s -o %d", cmd, opipe[1]);
	(void) vcrack(bfr, argv, MAXARGS);
	(void) execvp(argv[0], argv);
	(void) fprintf(stderr,
		       "ospawn: child execution failed, errno = %d\n",
		       errno);
	_exit(1);
	/*NOTREACHED*/
    }
}

/* ospawn.c ends here */
