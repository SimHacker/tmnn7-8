/*****************************************************************************

NAME
   peopen.c -- secure popen replacement

SYNOPSIS
   FILE *peopen(cmd, mode)	-- open a pipe to a command
   char *cmd;

   int peclose(mfp)		-- close a pipe opened by peopen
   FILE *mfp;

DESCRIPTION
   These functions provide a reasonably secure interface for doing popen(3)-
like things from trusted programs. The main entry point, peopen(), forks a
bare process instead of a shell. Path search and a limited I/O redirection
capability is supported. Commands opened for "r" may include an argument of
the form "<file", commands opened for "w" may have an argument like ">file";
these do the obvious redirections and are dropped out of the argument list
before the execvp().

BUGS
   We'd like to close all file descriptors except stdin or stdout before
spawning the child, but attempting to do so seems to cause broken pipe errors
in processes that do multiple peopens().

REVISED BY
   Eric S. Raymond
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

****************************************************************************/
/*LINTLIBRARY*/
#include "libport.h"

#define MAXARGS	64	/* max # of arguments accepted for secure command */

#ifndef _NFILE		/* this will be defined correctly on USG systems */
#define _NFILE	64
#endif

#define	RDR	0
#define	WTR	1
static	int	peopen_pid[_NFILE];

#define NOSHELL	/* without NOSHELL this just forks a Bourne shell */

FILE *peopen(cmd, mode)
/*
 * This is a modified version of popen, made more secure.  Rather than
 * forking off a shell, you get a bare process.
 */
char *cmd, *mode;
{
    int	pipes[2];
    register int fd, myside, yourside, pid, wmode = (strcmp(mode, "w") == 0);

#ifdef NOSHELL
    char    *cp, line[BUFSIZ], *largv[MAXARGS], *redirect = (char *)NULL;
    int	    largc = 0;

    /* crack the argument list into a dope vector */
    (void) strcpy(line, cmd);
    for (cp = line; *cp; cp++)
    {
	if (isspace(*cp))
	    *cp = '\0';
	else if (cp == line || cp[-1] == 0)
	{
 	    /* I/O redirection tokens aren't arguments */
	    if (*cp == '<' || *cp == '>')
	    {
		if ((wmode && *cp == '>') || (!wmode && *cp == '<'))
		    redirect = cp + 1;
		else
		    return((FILE *)NULL);
	    }
	    else if (largc >= MAXARGS - 1)
		return((FILE *)NULL);
	    else
		largv[largc++] = cp;
	}
    }
    largv[largc] = (char *) NULL;
#endif /* NOSHELL */

    if (pipe(pipes) < 0)
	return((FILE *)NULL);
    myside = (wmode ? pipes[WTR] : pipes[RDR]);
    yourside = (wmode ? pipes[RDR] : pipes[WTR]);
    if ((pid = fork()) == 0)	/* myside/yourside reverse roles in child */
    {
#ifdef CLOSEALL	/* this seems to break processes with multiple peopens() */
	/* close all pipes from other peopen()s */
	for (fd = 2; fd < _NFILE; fd)
	    if (peopen_pid[fd])
		(void) close(fd);
#endif /* CLOSEALL */

	(void) close(myside);
	if (yourside != (wmode ? 0 : 1))
	{
	    (void) close(wmode ? 0 : 1);
	    (void) dup(yourside);
	    (void) close(yourside);
	}

#ifdef NOSHELL
	/* do redirections */
	if (redirect != (char *)NULL)
	{
	    FILE *fp;

	    fp = fopen(redirect, mode);
	    if (fp == (FILE *)NULL || (fd = fileno(fp)) == FAIL)
		_exit(2);

	    if (wmode != fd)
	    {
		(void) close(wmode);
		(void) dup(fd);
		(void) close(fd);
	    }
	}

	(void) execvp(largv[0], largv);
#else
	(void) execl("/bin/sh", "sh", "-c", cmd, 0);
#endif /* NOSHELL */
	perror("exec in peopen() failed");
	_exit(1);
    }
    if (pid == -1)
	return((FILE *)NULL);
    peopen_pid[myside] = pid;
    (void) close(yourside);
    return(fdopen(myside, mode));
}

int peclose(ptr)
/* we're done writing to the command, close up the pipes and let 'er rip */
FILE *ptr;
{
    register int	f, r;
    catch_t		(*hangupf)(), (*ignoref)(), (*quitf)();
    wait_t		status;

    f = fileno(ptr);
    (void) fclose(ptr);
    ignoref = signal(SIGINT, SIGCAST(SIG_IGN));
    quitf = signal(SIGQUIT, SIGCAST(SIG_IGN));
    hangupf = signal(SIGHUP, SIGCAST(SIG_IGN));
    while((r = wait(&status)) != peopen_pid[f] && (r != -1 || errno == EINTR))
	continue;
    if (r == -1)
	status.w_status = -1;
    (void) signal(SIGINT, SIGCAST(ignoref));
    (void) signal(SIGQUIT, SIGCAST(quitf));
    (void) signal(SIGHUP, SIGCAST(hangupf));
#ifdef CLOSEALL	/* this seems to break processes with multiple peopens() */
    peopen_pid[f] = 0;
#endif /* CLOSEALL */
    return(status.w_status);
}

/* peopen.c ends here */
