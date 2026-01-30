/*****************************************************************************

NAME
   log.c -- error-reporting and logging functions

SYNOPSIS
   #include "news.h"

   void loginit()			-- initialize for logging privileges

   FILE *xfopen(name, fmode)		-- open a file or xerror out
   register char *name, *fmode;

   xerror(message)			-- log an error and exit
   char *message

   int nlog(lmsg)			-- log a status message
   char *lmsg;

   int logerr(lmsg)			-- log an error message
   char *lmsg;

DESCRIPTION
   These functions provide error-logging services for the news software.
It is expected that xerror(), nlog() and logerr() will be called through the
corresponding libport.h macros. The error loggers write their message to
stderr as well as to the log file(s).

   If the debug flag is on, these functions write to stdout only. If the
verbose flag is on, all log messages are written to stderr and the appropriate
logfile(s). If debug and verbose are both off, ordinary log calls are written
to the log file only.

FILES
   ADM/log	-- log file for ordinary transactions
   ADM/errlog	-- log file for errors

REVISED BY
   Eric S. Raymond
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

*****************************************************************************/
/* LINTLIBRARY */
#include "news.h"
#include "libpriv.h"
#include "header.h"

#define CSIG(s) ((sig==SIGSEGV)||(sig==SIGBUS)||(sig==SIGFPE)||(sig==SIGILL))

char	xbf[BUFLEN];		/* buffer for assembling error messages	*/

private bool dump_core = FALSE;
private char *junkyard;

void doubletrouble()
{
    if (!chdir(junkyard))
    {
	(void) mkdir(Progname, 0777);	/* Put some structure into things! */
	(void) chdir(Progname);
    }

#ifdef BSD
    /* in BSD systems, a core dump won't happen if euid != ruid */
    (void) setruid(geteuid());
#endif /* BSD */
    (void) abort();
}

private catch_t sigabort(sig)
/* log the occurrence of a stray signal and die gracefully*/
int	sig;	/* the signal number */
{
    static int strike = 0;
	
    if (CSIG(sig))
	dump_core++;
    if (strike++ > 3)
    {
	logerr1("DOUBLE TROUBLE: recursive aborts on signal %d", sig);
	doubletrouble();
    }
    xerror2("TROUBLE: aborted with signal %d on or after message %s",
		sig, header.h_ident);
}

void loginit()
/* set up conditions for privileged programs */
{
    register int    i, newsuid, newsgid;
#ifdef RUNTIME
    struct passwd	*pw;
    struct group	*gp;

    if ((pw = getpwnam(NEWSUSR)) == (struct passwd *)NULL)
	xerror1("Cannot get %s password entry", NEWSUSR);
    newsuid = pw->pw_uid;

    if ((gp = getgrnam(NEWSGRP)) == (struct group *)NULL)
	xerror1("Cannot get %s groups entry", NEWSGRP);
    newsgid = gp->gr_gid;
#else
    newsuid = NEWSUID;
    newsgid = NEWSGID;
#endif /* RUNTIME */

    (void) umask(UMASK);

    /*
     * This is called "defensive programming". If rnews or expire or sendbatch
     * ever gets killed by one of these signals we damn well want to know about
     * it in order to post-mortem things. This isn't such an issue for readers,
     * because we'll *see* them fail and can look at the core image.
     */
    for (i = SIGILL; i <= SIGTERM; i++)
#ifdef DEBUG
	if (i != SIGQUIT)
#endif /* DEBUG */
	    (void) signal(i, sigabort);

    if (uid == 0 && duid == 0)
    {
	privileged = TRUE;
	/*
	 * Must go through with this kluge since
	 * some systems do not honor the setuid bit
	 * when root invokes a setuid program.
	 */
	(void) setgid(duid = newsuid);
	(void) setgid(dgid = newsgid);
    }
    else
	privileged = (duid == newsuid);

    Sprint1(junkyard, "%s/.bad", site.textdir);
}

FILE *xfopen(name, fmode)
/* open file or die */
register char *name, *fmode;
{
    register FILE *fp;
    char	*fname;

    if ((fp = fopen(name, fmode)) == (FILE *)NULL)
    {
#ifdef IHCC
	/*
	 * IHCC users only see the "filename" that was in trouble,
	 * not the whole path.  (for security!)
	 */
	fname = strrchr(name, '/') + 1;
#else
	fname = name;
#endif
	xerror3("Cannot open %s (%s): %s", fname, fmode, errmsg(errno));
    }

    /* kludge for setuid not being honored for root */
    if ((uid == 0) && (duid != 0) && ((*fmode == 'a') || (*fmode == 'w')))
	(void) chown(name, duid, dgid);
    return(fp);
}

/* VARARGS1 */
int xerror(message)
char *message;
{
    static int	strike = 0;
	
    (void) fflush(stdout);
    logerr(message);
    if (dump_core || (++strike > 3))
	doubletrouble();
    xxit(1);
}

int nlog(lmsg)
char *lmsg;
{
    logx(0, lmsg);
}

int logerr(lmsg)
char *lmsg;
{
    logx(1, lmsg);
}

#ifdef OPENDEBUG
/* avoid recursion-till-we-blow-up due to fopens() of log files */
#undef fopen
#undef fclose
#undef open
#undef close
#endif /* OPENDEBUG */

private int logx(level, lmsg)
/*
 * Log the given message if it can be written.  The time and an attempt at
 * figuring out the remote system name are also logged.
 */
int level;
char *lmsg;
{
    static char	    *lfsuffix[] = {"log", "errlog", (char *)NULL};
    extern char	    *Progname;
    FILE	    *logfile = (FILE *)NULL;
    register char   *p, *logtime;
    char	    logfname[BUFLEN];		/* the log file */
    char	    rmtsys[SBUFLEN];
    int		    i;
    time_t	    t;

#ifdef DEBUG
    if (debug)	/* switch on to see diagnostics without logging them */
    {
	(void) fprintf(stdout, "%s: %s\n", Progname, lmsg);
	return;
    }
#endif /* DEBUG */

    if (level || verbose)
	(void) fprintf(stderr, "%s: %s\n", Progname, lmsg);

    /* extract the last relay site from the Path header */
    (void) strncpy(rmtsys, header.h_path, sizeof(rmtsys));
    rmtsys[sizeof(rmtsys) - 1] = '\0';
    if ((p = strchr(rmtsys, PATHSEP)) == (char *)NULL)
	p = strchr(rmtsys, ':');
    if (p)
	*p = 0;
    else if (p = strrchr(rmtsys, '@'))
	(void) strncpy(rmtsys, p+1, sizeof(rmtsys));
    else
	(void) strcpy(rmtsys, "local");

    /* now compute the event time */
    (void) time(&t);
    logtime = ctime(&t);
    logtime[16] = '\0';
    logtime += 4;

    /* log the event to some logfile(s) */
    for (i = 0;  i <= level; i++)
    {
	(void) sprintf(logfname, "%s/%s", site.admdir, lfsuffix[i]);

	if (access(logfname, W_OK) == 0 && (logfile = fopen(logfname, "a")))
	{
	    /* force append mode on the file */
#ifdef FCNTL
	    int flags;
	    flags = fcntl(fileno(logfile), F_GETFL, 0);
	    (void) fcntl(fileno(logfile), F_SETFL, flags|O_APPEND);
#else /* !FCNTL */
	    (void) lseek(fileno(logfile), (off_t)0, SEEK_END);
#endif /* !FCNTL */

	    if (i)
		(void) fprintf(logfile, "%s %s\t%s: %s\n",
			logtime,
			hlnblank(header.h_ident)? header.h_ident : username,
			Progname,
			lmsg);
	    else
		(void) fprintf(logfile, "%s %s\t%s\n",
				logtime, rmtsys, lmsg);
	    (void) fclose(logfile);
	}
    }
}

#ifdef OPENDEBUG
/*
 * The following functions, together with the OPENDEBUG macros in priv.h,
 * front-end normal file open and close operations with debugging
 * instrumentation.
 */
typedef struct
{
    FILE	*p_fp;
    char	p_name[NAMELEN];
    bool	p_inuse;
}
fpkeep;
private fpkeep fpsave[_NFILE];

typedef struct
{
    int		d_fd;
    char	d_name[NAMELEN];
    bool	d_inuse;
}
fdkeep;
private fdkeep fdsave[_NFILE];

private fpkeep *fpsearch(fp, inuse)
/* find a given fp in the file table */
FILE *fp;
bool inuse;
{
    register fpkeep *pt;

    for (pt = fpsave; pt < fpsave + _NFILE; pt++)
	if (pt->p_inuse == inuse && (!inuse || fp == pt->p_fp))
	    return(pt);
    return((fpkeep *)NULL);
}

FILE *d_fopen(name, mode)
char *name;
char *mode;
{
    FILE *fp;
    fpkeep *fpfound;

    if ((fpfound = fpsearch((fpkeep *)NULL, FALSE)) == (fpkeep *)NULL)
	xerror1("opendebug: detects too many files open: %s", name);
    else if ((fp = fopen(name, mode)) != (FILE *)NULL)
    {
	log2("opendebug: opening %s (%s)", name, mode);
	(void) strcpy(fpfound->p_name, name);
	fpfound->p_fp = fp;
	fpfound->p_inuse = TRUE;
	return(fp);
    }
    else
    {
	logerr3("opendebug: can't open %s (%s): %s", name,mode, errmsg(errno));
	return((FILE *)fp);
    }
}

int d_fclose(fp)
FILE *fp;
{
    register fpkeep *pt;

    if ((pt = fpsearch(fp, TRUE)) != (fpkeep *)NULL)
    {
	pt->p_inuse = FALSE;
	log1("opendebug: fclosing %s", pt->p_name);
	return(fclose(fp));
    }
    else
    {
	log0("opendebug: table is missing a file pointer value!");
	return(FAIL);
    }
}

private fdkeep *fdsearch(fd, inuse)
/* find a given fd in the file table */
int fd;
bool inuse;
{
    register fdkeep *pt;

    for (pt = fdsave; pt < fdsave + _NFILE; pt++)
	if (pt->d_inuse == inuse && (!inuse || fd == pt->d_fd))
	    return(pt);
    return((fdkeep *)NULL);
}

int d_open(name, mode)
char *name;
int mode;
{
    int fd;
    fdkeep *fdfound;

    if ((fdfound = fdsearch(0, FALSE)) == (fdkeep *)NULL)
	xerror1("opendebug: detects too many files open: %s", name);
    else if ((fd = open(name, mode)) != FAIL)
    {
	log2("opendebug: opening %s (%s)", name, mode);
	(void) strcpy(fdfound->d_name, name);
	fdfound->d_fd = fd;
	fdfound->d_inuse = TRUE;
	return(fd);
    }
    else
    {
	logerr3("opendebug: can't open %s (%d): %s", name,mode, errmsg(errno));
	return(FAIL);
    }
}

int d_close(fd)
int fd;
{
    register fdkeep *pt;

    if ((pt = fdsearch(fd, TRUE)) != (fdkeep *)NULL)
    {
	pt->d_inuse = FALSE;
	log1("opendebug: closing %s", pt->d_name);
	return(close(fd));
    }
    else
    {
	log0("opendebug: table is missing a file descriptor value!");
	return(FAIL);
    }
}

#endif /* OPENDEBUG */

/* log.c ends here */
