/*****************************************************************************

NAME
   server.c -- execute a server process, returning pipes to its stdin & stdout

SYNOPSIS
   #include "server.h"

   server_t *srvopen(file, argv)   -- return data on an open server process
   char *file; char argv[];

   void srvclose(sp)	    -- close the channel associated with a server
   server_t *sp;

DESCRIPTION
   The srvopen() function hides the details of executing a server process of
some sort under control of the current process. It returns a pointer to a
struct that contains a pair of file descriptors, or NULL for error.

   In the former case the 'readsrv' member is opened for read and connected to
the server's stdout and stderr, and the 'writesrv' member is opened for write
connected to the server's stdin. In the latter case errno will contain the
relevant error number.

   The srvclose() function, given a channel descriptor returned by server(),
will close both fds and make the server array slot available. It does not wait
for the process to terminate (it is presumed that the caller, which has access
to the server's pid, will have killed the process if it cares).

   The srvopen() code uses execvp(), so it will check the user's PATH
variable and does not demand full filenames.

   Compiling this module with -DMAIN yields an interactive tester. To test
only the read from server, call 'server -r'; to test only the write to server,
call 'server -w'; to test both functions, type 'server -xw'. Type something to
each '*'.

BUGS
   This will not work on stock Version 7 systems, because they block until
512 bytes have piled up in a pipe regardless of how you do the reads and
writes.

AUTHOR
   Eric S. Raymond
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

*****************************************************************************/
/*LINTLIBRARY*/
#include "libport.h"
#include "server.h"

#ifndef private
#define private static
#endif

private server_t servers[MAXSERVERS];

server_t *srvopen(file, argv)
/* open a server process */
char	*file;
char	*argv[];
{
    int		    inpipe[2], outpipe[2];
    server_t	    *csrv;

    for (csrv = servers; csrv < &servers[MAXSERVERS]; csrv++)
	if (csrv->pid == 0)
	    break;

    if (csrv->pid || pipe(inpipe) < 0 || pipe(outpipe) < 0)
	return((server_t *)NULL);

    if ((csrv->pid = vfork()) == 0)
    {
	(void) close(0);
	(void) close(1);
	(void) close(2);
	(void) dup(inpipe[0]);
	(void) dup(outpipe[1]);
	(void) dup(outpipe[1]);
	(void) close(inpipe[0]);
	(void) close(inpipe[1]);
	(void) close(outpipe[0]);
	(void) close(outpipe[1]);

	(void) execvp(file, argv);
	_exit(1);
    }
    if (csrv->pid == -1)
	return((server_t *)NULL);

    (void) close(outpipe[1]);
    (void) close(inpipe[0]);

    csrv->readsrv = outpipe[0];
    csrv->writesrv = inpipe[1];
    return(csrv);
}

void
srvclose(csrv)
server_t *csrv;
{
    (void) close(csrv->readsrv);
    (void) close(csrv->writesrv);
    csrv->pid = 0;
}

#ifdef MAIN
#include "procopts.h"

char *Progname = "server";
char *usage = "Usage: server [-rwx] [-e server]";

#ifndef private
#define private static
#endif

private int rflag, wflag, xflag, count;
private char exebuf[BUFSIZ];

private option_t options[] =
{
/*
optlet  filchar	flag	    from      to       type     buf	*/
'e',	'\0',	0,	    DNC,      DNC,     STRING,	exebuf,
'r',	'\0',	&rflag,	    DNC,      DNC,     DNC,     (char *)NULL,
'w',	'\0',	&wflag,	    DNC,      DNC,     OPTION,  (char *)NULL,
'x',	'\0',	&xflag,	    DNC,      DNC,     OPTION,  (char *)NULL,
'\0',	'\0',	0,	    0,        0,       0,       (char *)NULL,
};

int main(argc, argv)
/* test server execution */
int	argc;
char	**argv;
{
    char	bfr[BUFSIZ];
    server_t	*serv;

    /* process command line to override .newsrc options */
    if (procopts(argc, argv, DNC, options) == FAIL)
	xerror0(usage);

    if (xflag)	    /* here's our 'child' tester */
    { 
	if (rflag || wflag)
	{
	    (void) fprintf(stderr, "child: starting up\n");

	    if (rflag)  /* read stuff off input forever and send somewhere */
	    {
		while (fgets(bfr, BUFSIZ, stdin))
		{
		    bfr[strlen(bfr) - 1] = '\0';
		    (void) fprintf(stderr,"child: I saw '%s' on stdin\n",bfr);
		    bfr[strlen(bfr) - 1] = '\n';
		    if (wflag)
		    {

			(void) fprintf(stdout,
			    "child: I send back '%s' on stdout\n", bfr);
			(void) fflush(stdout);
		    }
		}
		(void) fprintf(stderr, "child: no more input\n");
	    }
	    else	    /* Generate lines to output forever */
	    {
		for (;;)
		{
		    (void) fprintf(stdout,"child: line %d\n", count++);
		    (void) fflush(stdout);
		}
	    }
	    exit(0);
	}
    }
    else
    {
	/* pick one of server's avatars as a tester */
	if (exebuf[0] == '\0')
	{
	    if (wflag)
		argv[1] = "-xr";
	    else if (rflag)
		argv[1] = "-xw";
	    else
	    {
		(void) fprintf("server: no server specified\n");
		exit(1);
	    }
	}
	else /* assume -e was the only argument given */
	    argv += 2;	/* skip "server" and "-e" */

	/* try to fork and talk to the server */
	if ((serv = srvopen(argv[0], argv)) == (server_t *)NULL)
	{
	    (void) fprintf(stderr, "server: couldn't run '%s'\n", argv[0]);
	    exit(1);
	}
	else
	    (void) fprintf(stderr,"server: server '%s' spawned OK\n",argv[0]);

	/* here's the actual test loop */
	for (;;)
	{
	    if (fputs("* ",stdout), fgets(bfr, BUFSIZ, stdin) == (char *)NULL)
		break;
	    else
	    {
		if (wflag)
		{
		    if (srvputs(bfr, serv) == NULL)
			perror("writing to server");
		}

		if (rflag)
		{
		    if (srvgets(bfr, BUFSIZ, serv) == NULL)
			perror("reading from server");

		    bfr[strlen(bfr) - 1] = '\0';
		    (void) fprintf(stdout,
			"server: I saw '%s' on stdin\n", bfr);
		    (void) fflush(stdout);
		    bfr[strlen(bfr) - 1] = '\n';
		}
	    }
	}
    }
}
#endif /* MAIN */

/* server.c ends here */
