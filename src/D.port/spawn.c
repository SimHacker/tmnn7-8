/****************************************************************************

NAME
   spawn.c -- functions for running commands under program control

SYNOPSIS
   #include "spawn.h"

   int shcmd(cmd, flags, aval)		-- execute shell command
   char *cmd, int flags; char *aval;

   int spawn(args, flags, aval)		-- front end to exec(2)
   char **args; int flags, char *aval;

DESCRIPTION
   Implementations of common commands for programs that involve spawning
child processes are isolated here. Child processes are spawned with default
signal masks, but signal masks in the parent are restored to the values they
had before the fwait() call before exit.

REVISED BY
   Eric S. Raymond
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

****************************************************************************/
/*LINTLIBRARY*/
#include "libport.h"
#include "spawn.h"

#ifndef SHELL
char *SHELL = "/bin/sh";
#endif

int spawn(args, flags, aval)
/* run a child command. $A in the child shell's environment is set to aval */
char **args;	/* arguments for child command */
int flags;	/* foreground/background/confirmation flags */
char *aval;	/* set $A to this if non-NULL */
{
    int pid, i, retval;
    wait_t pstatus;
    char *env[100], a[BUFSIZ + 2], **envp;
    extern char **environ;
#ifdef SIGTSTP
    catch_t (*oldstop)(), (*oldttin)(), (*oldttout)();

    oldstop = signal(SIGTSTP, SIGCAST(SIG_DFL));
    oldttin = signal(SIGTTIN, SIGCAST(SIG_DFL));
    oldttout = signal(SIGTTOU, SIGCAST(SIG_DFL));
#endif /* SIGTSTP */
    while ((pid = fork()) == -1)
	(void) sleep(1);	/* must not clear alarm */
    if (pid == 0)		/* child side */
    {
	for (i = 3 ; i < 20 ; i++)
	    (void) close(i);
	if (flags & BAKGRND)
	{
	    (void) signal(SIGINT, SIGCAST(SIG_IGN));
	    (void) signal(SIGQUIT, SIGCAST(SIG_IGN));
	    (void) close(0);
	    (void) close(1);
	    (void) open("/dev/null", 2);
	    (void) dup(0);
	}

	/* set $A */
	if (aval != (char *)NULL)
	{
	    (void) sprintf(a, "A=%s", aval);
	    env[0] = a;
	    for (envp = env+1; *environ!=(char*)NULL && envp<env+98; environ++)
		if ((*environ)[0] != 'A' || (*environ)[1] != '=')
		    *envp++ = *environ;
	    *envp = (char *)NULL;
	}

#ifndef lint		/* various lints disagree on setuid/setgid arg type */
	if (flags & NOPRIVS)
	{
	    (void) setuid(getuid());
	    (void) setgid(getgid());
	}
#endif /* lint */

	(void) execve(args[0], args, env);
	(void) fprintf(stderr, "%s: not found\n", args[0]);
	exit(20);
    }
    if (!(flags & BAKGRND))
    {
	catch_t (*savequit)(), (*saveint)();

	savequit = signal(SIGQUIT, SIGCAST(SIG_IGN));
	saveint = signal(SIGINT, SIGCAST(SIG_IGN));
	while ((i = wait(&pstatus)) != pid && (i != -1 || errno == EINTR))
	    continue;
	if (i == -1)
	    retval = 1;
	else
	    retval = pstatus.w_status;
	if (flags & CWAIT)
	{
	    (void) fprintf(stderr, "continue? ");
	    while ((errno = 0, i = getchar()) != '\n'
		&& (i != EOF || errno == EINTR))
		    continue;
	}
	(void) signal(SIGQUIT, SIGCAST(savequit));
	(void) signal(SIGINT, SIGCAST(saveint));
#ifdef SIGTSTP
	(void) signal(SIGTSTP, SIGCAST(oldstop));
	(void) signal(SIGTTIN, SIGCAST(oldttin));
	(void) signal(SIGTTOU, SIGCAST(oldttout));
#endif /* SIGTSTP */
	return(retval);
    }
    else
	return(SUCCEED);
}

int shcmd(cmd, flags, aval)
/* execute a shell command */
char *cmd;	/* command to execute */
int flags;	/* flags tell whether to background and/or wait for prompt */
char *aval;	/* environment string to be entered, if any */
{
    char *arg[4];

#ifndef SHELL
    if ((SHELL = getenv("SHELL")) == (char *)NULL)
	SHELL = "/bin/sh";
#endif /* SHELL */

    arg[0] = SHELL, arg[1] = "-c"; arg[2] = cmd; arg[3] = (char *)NULL;
    return(spawn(arg, flags, aval));
}

/* spawn.c ends here */
