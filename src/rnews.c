/*****************************************************************************

NAME
   rnews.c - insert, receive, and transmit news articles.

SYNOPSIS
   main(argc, argv)		-- main sequence of rnews
   int argc; char **argv;

   catch_t xxit(status)		-- exit the program, cleaning up all locks
   int status;

DESCRIPTION
   This is the main of the rnews program that posts messages and accepts them
off the net. The inews program uses it as a back end. This version accepts
batched news directly (no unbatch needed).

   This program can be invoked under names corresponding to some unbatching
programs in older versions in order to force various options on:

   unbatch	-- force unbatching on
   cunbatch	-- force unbatching and decompression on
   c7unbatch	-- force unbatching and decompression on, prefilter with decode

FILES
   ADM/seq		-- message-ID sequence number files
   ADM/log		-- event log file
   ADM/errlog		-- error event log file
   ADM/active		-- active-groups file (through active.c)
   ADM/moderators	-- list of group moderators
   ADM/history*		-- message-history(s) file (through history.c)
   SPOOL/*		-- take input files from here when -U is enabled

AUTHORS
   Eric S. Raymond
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

*****************************************************************************/
/*LINTLIBRARY*/
#include "news.h"
#include "libpriv.h"
#include "dballoc.h"
#include "header.h"
#include "procopts.h"
#include "active.h"
#include "history.h"
#include "post.h"

/*
 * If we have spoolnews defined, this implies that the rnews deamon
 * will be running.  In that case, we assume that we can lock out the
 * world whilst we run.  This produces an incredible speed increase,
 * since we don't need to continuously update the active file.
 */
#ifdef SPOOLNEWS
#undef ENTRYLOCK
#endif
	
#define UNIX	/* someday this won't be unconditional */

#define ROPTFORM    "usage: rnews [-uUdv] [-p file] [-x nosend] [-o fd]\n"

/* daemon-mode performance-tuning defines */
#define DWAIT	30	/* wait this long if expire is running */
#define EWAIT	30	/* wait this long after finding an empty spooldir */
#define NWAIT	5	/* wait this long for new news */
#define CHUNK	50	/* Check for EXPLOCK after this many batches */

extern void postinit();		/* pacify lint -- there's no insert.h */

char	*Progname = "rnews";	/* so xerror identifies failing program */

/* things only rnews.c needs to see */
private char	infile[BUFLEN];		/* hold name of -p option file */
private int	Uflag, nospool;		/* unspooling option values */
private int	outfd;			/* report file descriptor */
private int	quitsig = 0;		/* We received a singal to quit */
private int	wractonexit = 0; 	/* Write active file on exit */
private	char	pidfilename[BUFLEN];
#ifdef UNIX
private int	dflag = FALSE;			/* daemon mode */
#endif /* UNIX */
private	int	Pflag = FALSE;			/* Run in privileged mode */

private option_t postopts[] =
{ /*
opt  filchar flag     from to	buf     meaning   */
'p', '\0',   (int*)0, DNC, DNC,	STRING,	infile,		/* input file */
'o', '\0',   &outfd,  DNC, DNC,	NUMBER, (char *)NULL,	/* report fd */
'u', '\0',   &nospool,DNC, DNC,	OPTION, (char *)NULL,	/* nospool mode */
'v', '\0',   &verbose,DNC, DNC,	NUMBER, (char *)NULL,	/* verbosity level */
'x', '\0',   (int*)0, DNC, DNC,	STRING,	nosend,		/* suppress send to */
'U', '\0',   &Uflag,  DNC, DNC, OPTION, (char *)NULL,	/* unspool mode */
#ifdef UNIX
'd', '\0',   &dflag,  DNC, DNC, OPTION, (char *)NULL,   /* daemon mode */
#endif /* UNIX */
#ifdef DEBUG
'D', '\0',   &debug,  DNC, DNC, NUMBER, (char *)NULL,	/* debug mode */
#endif /* DEBUG */
'P', '\0',   &Pflag,  DNC, DNC, OPTION, (char *)NULL,	/* privileged mode */
'\0','\0',   (int*)0, 0,    0,	0,      (char *)NULL
};

/* verbosity level minima for various messages */
#define V_SHOWSRC	1	/* indicate each source processed */
#define V_SHOWPHASE	2	/* show what processing phase we're in */

#ifdef UNIX
/*
 * Daemon-mode support begins here
 */

private void rnews_daemon_exit()
{
    (void) unlink(pidfilename);
    exit(0);
}

static void detach_process()
/*
 * Set up standard environment by detaching from the parent.
 * Some of this may not be necessary for this use, all this is
 * out of a patch tytso@bloom-beacon hacked out of some old daemon code.
 */
{
    int	f;
	
    if (fork() != SUCCEED)
	exit(0);		/* Parent exits */
    for (f = 0; f < 5; f++)
	(void) close(f);
    (void) open("/dev/null", O_RDONLY);
    (void) open("/dev/null", O_WRONLY);
    (void) dup(1);
#ifdef BSD
    f = open("/dev/tty", O_RDWR);
    if (f > 0)
    {
	(void) ioctl(f, TIOCNOTTY, 0);
	(void) close(f);
    }
#else /* !BSD */
    (void) setpgrp();
    if (fork() != SUCCEED)
	    exit(0);		/* Parent exits */
#endif /* BSD */
}

static void rnews_daemon()
{
    static DIR			*directory; /* current directory structure */
    static struct dirent	*entry; /* points to an entry in it */
    wait_t			waitb;
    FILE			*pidfile;
    int				pid;
	
    Uflag = TRUE;	/* We're going to be unspooling stuff */

    (void) snprintf(pidfilename, sizeof(pidfilename), "%s/rnews.pid", site.admdir);	
    if (pidfile = fopen(pidfilename, "r"))
    {
	(void) fgets(bfr, sizeof(bfr), pidfile);
	/*
	 * Probe pid to see if other rnews daemon is still
	 * running by sending a 0 signal.  This may not work
	 * on all systems, in which case we'll need to ifdef
	 * it out.
	 */
	if ((pid = atoi(bfr)) && !kill(pid, 0)) {
		logerr1("Another rnews daemon pid %d already running", pid);
		xxit(1);   /* _can't_ call xxit, since wrong thing */
			   /* will happen */
	}
    }

#ifdef DEBUG
    if (debug)
	(void) printf("Debug mode, skipping detach_process().\n");
    else
#endif /* DEBUG */
	detach_process();

    (void) unlink(pidfilename);
    if (pidfile = fopen(pidfilename, "w"))
    {
	(void) fprintf(pidfile, "%d\n", getpid());
	(void) fclose(pidfile);
    }
    else
	logerr0("rnews: Couldn't write pid file for daemon mode");

    (void) signal(SIGTERM, rnews_daemon_exit);
    (void) signal(SIGINT, rnews_daemon_exit);
    (void) signal(SIGHUP, rnews_daemon_exit);
    
    for(;;)
    {
	if (privlockcheck())
	{
	    (void) sleep(DWAIT);
	    continue;
	}
	if ((directory = opendir(site.spooldir)) == (DIR *)NULL)
	    xerror0("can't access news spool directory");
	while (entry = readdir(directory)) 
	    if (entry->d_name[0] != '.')
		break;
	(void) closedir(directory);
	if (!entry)
	{
	    (void) sleep(EWAIT);
	    continue;
	}
	else
	    (void)sleep(NWAIT);	/* Give time for more news to come in. */
		
	switch(pid = fork())
	{
	case FAIL:
	    perror("fork: to rnews -U");
	    (void) sleep(30);
	    break;
	case SUCCEED:
	    loginit();		/* restore signal handlers */
	    return;		/* Go do the unspooling! */
	default:
	    log1("Rnews daemon %d --- beginning run", pid);
	    if (wait(&waitb) < 0)
	    {
		perror("wait: for rnews child process");
		(void) sleep(30);
	    }
	}
    }
}
#endif /* UNIX */

static catch_t catcher()
{
    quitsig++;
}

main(argc, argv)
int	argc;
char	**argv;
/*
 * Main sequence of the news poster. We may have to pre-filter the incoming
 * chars for compression/encoding, but in any case the text of a clear
 * version of the text ends up on stdin.
 */
{
#if defined(SHARED) || defined(NONLOCAL)
    (void) fputs(stderr, "Please post through inews or postnews!");
    xerror1("%s tried to run rnews direct.");
#else
    int	i;

#ifdef NICENESS
    (void) nice(NICENESS);
#endif /* NICENESS */

    /* uuxqt doesn't close all its files */
    for (i = 3; !close(i); i++)
	continue;

    /* tell the logfile about the options */
    if (verbose >= V_SHOWPHASE || !isatty(fileno(stdin)))
    {
	char	**cpp;
	int	ac;

	(void) snprintf(bfr, LBUFLEN, "running");
	for (ac = argc, cpp = argv; ac--; cpp++)
	{
	    (void) strlcat(bfr, " ", LBUFLEN);
	    (void) strlcat(bfr, *cpp, LBUFLEN);
	}
	(void) strlcat(bfr, "\n", LBUFLEN);
	log0(bfr);
    }

    /* process command-line options first */
    (void) batchmode(argv[0]);
#ifndef SPOOLNEWS
    nospool = TRUE;
#endif /* SPOOLNEWS */
    if (procopts(argc, argv, 0, postopts) == FAIL)
    {
	(void) fputs(ROPTFORM, stderr);
	xxit(1);
	/*NOTREACHED*/
    }

    newsinit();	/* set up defaults and initialize. */
#ifdef DEBUG
    if (!debug)
#endif /* DEBUG */
	loginit();

#ifdef DEBUG
    /*
     * assume that this is a debugging run and we want instrumentation.
     */
    if (!privileged)
	debug = 1;
    if (!privileged)
    {
	verbose = debug;
	(void) printf("Turning on debug mode....\n");
    }
#endif /* DEBUG */

#ifdef UNIX
    if (dflag)
	rnews_daemon();
#endif /* UNIX */

    if (!Pflag)
	privileged = 0;	/* Turn off privileged mode */

    /* we may simply want to spool news then xxit() */
    if (!Uflag)
    {
	if (privlockcheck() || !nospool)
	{
	    char spoolto[BUFLEN];

	    if (infile[0])
		(void) freopen(infile, "r", stdin);
	    (void) snprintf(infile, sizeof(infile), "%s/.tmp/spsafeXXXXXX", site.spooldir);
	    collect((char *)NULL, infile);

	    if (outfd)
	    {
		FILE	*fp;

		(void) idmake(infile);
		if ((fp = fopen(infile, "r")) == (FILE *)NULL ||
					hread(&header, 0L, fp) == 0L)
		    (void) write(outfd, (char *)NULL, (iolen_t)0);
		else
		    (void) write(outfd,
			     header.h_ident,
			     (iolen_t) strlen(header.h_ident));
		(void) fclose(fp);
		(void) close(outfd);
	    }

	    (void) snprintf(spoolto, sizeof(spoolto), "%s/spoolXXXXXX", site.spooldir);
	    (void) mktemp(spoolto);
	    (void) rename(infile, spoolto);

	    xxit(0);
	    /*NOTREACHED*/
	}
    }

#ifdef DEBUG
    if (!debug)
    {
	if (verbose >= V_SHOWPHASE)
	    log0("about to open and lock the history file");
#endif /* DEBUG */

    (void) signal(SIGHUP, catcher);
    (void) signal(SIGINT, catcher);
    (void) signal(SIGQUIT, catcher);
    (void) signal(SIGTERM, catcher);
#ifndef ENTRYLOCK
    /*
     * At this point, we need to read in the history file in order to
     * detect old messages. If the history data for the run is an on-disk
     * database, and we have region-locking to guarantee its integrity,
     * no need to do exclusions here. Otherwise we have to lock out other
     * instances of inews/rnews until history data is written out again
     * which in turn implies that we need signal protection to guarantee that
     * unlock() gets done.
     */
    lock();
#endif /* LOCKF */
    (void) hstread(TRUE);

#ifdef DEBUG
	if (verbose >= V_SHOWPHASE)
	    log0("history file lock done");
    }
#endif /* DEBUG */

    /* set up ACTIVE, read in active group data */
    (void) rdactive(NULLPRED);
    (void) rdflags(NG_COMPRESSED | NG_GETDIST);

    /*
     * We'd like to initialize the posting code before asserting a lock on the
     * database or going uninterruptible so as to spend as little time as
     * possible with the database files exclusive-use-locked -- but ACTIVE and
     * HISTORY have to be set up for postinit() to work OK, and that has to be
     * after the lock(). If lockf() is available, we are interruptible anyhow.
     */
    postinit();
    wractonexit++;		/* If we exit now, write out the */
				/* active file */

    /*
     * Now do the real work of interpreting batches
     */
    if (!Uflag)			/* normal mode (no unspool) */
 	(void) batchproc(infile[0] ? infile : (char *)NULL, post, outfd);
    else			/* unspool mode */
    {
#ifdef TDEBUG
	if (verbose >= V_SHOWPHASE)
	    log0("beginning unspooling mode");
#endif /* DEBUG */

	/* scan through spooled messages or batches, processing each */
	static DIR		*directory; /* current directory structure */
	static struct dirent    *entry;	    /* points to an entry in it */
	int			artcount, spoolcount = 0;

	/* first get access to the spool directory */
	if ((directory = opendir(site.spooldir)) == (DIR *)NULL)
	    xerror0("can't access news spool directory");

#ifdef FEEDBITS
	/*
	 * Spooled sets of incoming news tend to be quite large, so we presume
	 * caching subscription bits is a win. This may not be true when the
	 * average number of newsgroups listed in a batch set is less than the
	 * count of newsgroups in the active file. If CACHEBITS is on we'll
	 * just read the feedbits cache file most of the time, which is
	 * much cheaper.
	 */
	if (feedbits() == FAIL)   /* set up cached subscription bits */
	    logerr0("Feed bitmaps file was corrupted!");
#endif /* FEEDBITS */

	/* process each file in the directory */
	while (((entry = readdir(directory)) != (struct dirent *)NULL) &&
	       !quitsig)
        {
	    /* skip . and .. and files that are supposed to be invisible */
	    if (entry->d_name[0] == '.')
		continue;

#ifdef DEBUG
	    if (verbose >= V_SHOWSRC)
		(void) printf("rnews: gathering from spool file %s\n", entry->d_name);
#endif /* DEBUG */
	    spoolcount++;

	    (void) snprintf(infile, sizeof(infile), "%s/%s", site.spooldir, entry->d_name);

	    if (filesize(infile) <= 0)
	    {
		log1("spool file %s zero-length or missing", infile);
#ifdef DEBUG
		if (verbose >= V_SHOWSRC)
		    (void) printf("rnews: deleting 0-length %s\n", infile);
		if (!debug)
#endif /* DEBUG */
		    (void) unlink(infile);
	    }
	    else if ((artcount = batchproc(infile, post, outfd)) != FAIL)
	    {
		if (artcount > 1)
		    log2("%d articles processed from %s", artcount, infile);
#ifdef DEBUG
		if (!debug)
#endif /* DEBUG */
		    (void) unlink(infile);
	    }
	    else
	    {
                  /*
                   * Stash the offending batch away where it isn't
                   * going to trip up the next rnews run.
                   */
                  log1("Moving %s to .bad directory", infile);
                  (void) snprintf(bfr, LBUFLEN, "%s/.bad/%s", site.textdir,
                                 strrchr(infile, '/'));
                  (void) rename(infile, bfr);
                  (void) unlink(infile);
	      }

#ifdef UNIX
	    if (dflag && (spoolcount % CHUNK == 0) && privlockcheck())
		break;
#endif /* UNIX */
	}

	(void) closedir(directory);

	/* abort cleanly if there was no spooled news waiting */
	if ((spoolcount == 0) && !dflag)
	    xerror0("no spooled news waiting");

	if (dflag)
	    log1("rnews daemon: run finished, %d spool section(s) processed.",
		 spoolcount);
    }

#ifdef DEBUG
    if (debug)
	(void) hstwrite();	/* write out all history info */
#endif /* DEBUG */

    xxit(0);			/* all locks are removed here */
    /*NOTREACHED*/
#endif /* defined(SHARED) || defined(NONLOCAL) */
}

catch_t xxit(status)
/* exit and cleanup */
int status;
{
    /*
     * If this is an abort exit from unspool mode, stash the offending
     * batch away where it isn't going to trip up the next rnews run.
     */
    if (status != SUCCEED && Uflag)
    {
	(void) snprintf(bfr, LBUFLEN, "%s/.bad/%s", site.textdir, strrchr(infile, '/'));
	(void) rename(infile, bfr);
    }
    if (outfd)
	(void) close(outfd);	/* this is not strictly necessary */
#ifdef SPOOLNEWS
    if (wractonexit)
	    wractive(TRUE);
#endif
#ifndef ENTRYLOCK
#ifdef DEBUG
    if (!debug)
#endif /* DEBUG */
	unlock();
#endif /* ENTRYLOCK */
#ifdef PROFILE
    chdir(site.libdir);
    {
	    char	path[LBUFLEN];
	    getwd(path);
	    log1("Dumping profiling information in %s", path);
    }
    monitor(0);
#endif
#ifdef ZRETURN
    exit(0);
#else
    exit(status);
#endif /* ZRETURN */
}

/* rnews.c ends here */
