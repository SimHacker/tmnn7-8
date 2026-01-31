/***************************************************************************

NAME
   sendbatch.c -- transmit news batches for USENET

SYNOPSIS
   sendbatch [-DFfk] [-sx] sysname...

DESCRIPTION
   A call to sendbatch with no arguments ships pending batches to all systems.
If a list of system names is given on the command, only those systems are sent
to. The -x option overrides the tests for minimum uucp spool space available
and uucp queue length, forcing transmission.
   The A, B, C, D, E, N, Q, S, U, V and X transmission options are interpreted
when sending batches from here. Special arrangements are made to do N and Q;
the others are handed off to the lower level routines in transmit.c.
   The format sendbatch emits is the standard one, i.e.

        #! aaaa
        #! rnews xxxx
        <article containing xxxx characters>
        #! rnews yyy
        <article containing yyy characters>
            :
            :
        #! rnews zzzz
        <article containing zzzz characters>
        <EOF>

where aaaa is one of the names unbatch, cunbatch or c7unbatch to indicate the
compression type. The V2.10.2 option will suppress this header.

Normally, any text after the Message-ID on a batch file line will be simply
concatenated to the corresponding batch header. This may be useful in passing
transmission directives to the remote rnews or other agent on a per-article
basis.

NOTE
   The call hierarchy of this program is as follows:

main
  newsbatch	    -- send all batched news to a given system
    uuq		    -- check for UUCP queue overflow
    df		    -- check for sufficient spool space
    uuxbatch	    -- transmit batches implied by a batch file
      catart      -- add a given message to a growing batch
      uuxfile       -- transmit a file to given system(s)

Everything ultimately resolves into calls to the transmit() routine (or, if
B211COMPAT is defined, perhaps a call to a <system>.cmd script in the batch
directory).

FILES
   /tmp/bout??????      -- used to build batch (before compression)
   /tmp/batch??????     -- the final batch
   /tmp/mcast??????	-- temp files for multicast assembly
   BATCH/<system>.work  -- work copy of the batch file for system

AUTHOR
   Eric S. Raymond
The 4.3BSD Q option and SPOOLMIN code are descended from Chris Lewis's batcher
program. The Q option code for non-4.3 systems is from Bob Esposito's nbatcher
program. The redirect() code derives from version 2.1 of Shane McCarron's
uucast package.
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

*****************************************************************************/
/*LINTLIBRARY*/
#include "news.h"
#include "libpriv.h"
#include "dballoc.h"
#include "alist.h"
#include "active.h"
#include "header.h"
#include "procopts.h"
#include "post.h"
#include "feeds.h"
#include "history.h"
#include "libuucp.h"

/* verbosity minima for various messages */
#define V_SHOWSYS	1	/* show batching transmission targets */
#define V_SHOWPARTS	2	/* show names of files added to batches */

#define REDIRECT    "To: "	/* leading string of a redirect line */
#define REDIRLEN    4		/* size of redirect string */

#define UUCPDIR	    "/usr/spool/uucp"	/* should probably be configurable */

/*
 * We define these here so we can do sizeofs on them below. The compiler
 * should reduce those expressions to constants in the object code.
 */
#define UNBATCH		"#! unbatch\n"
#define CUNBATCH	"#! cunbatch\n"
#define C7UNBATCH	"#! c7unbatch\n"

extern char *sys_errlist[];

char    *Progname = "sendbatch";    /* so xerror identifies failing program */

private char    sendto[LBUFLEN] = "all";
private char    outfile[BUFLEN] = "/tmp/boutXXXXXX";
private char    batch[BUFLEN] = "/tmp/batchXXXXXX";
private char    workfile[BUFLEN];
private char    batchname[BUFLEN];
private int     nochk = FALSE, fileg = FALSE, filef = FALSE, killf = FALSE;

/* program modes */
#define XMIT	0x01	/* actually ship batches */
#define FILT	0x02	/* filter input ID lists to filenames */

private option_t options[] =
{ /*
opt  filchar flag    from  to	buf     meaning   */
's', '\0',   NONE,   XMIT,XMIT,	STRING,	sendto,		/* send to these */
'x', '\0',   &nochk, XMIT,XMIT,	OPTION,	(char *)NULL,	/* suppress checks */
'f', '\0',   &fileg, XMIT,XMIT,	OPTION, (char *)NULL,	/* dump filenames */
'k', '\0',   &killf, XMIT,XMIT,	OPTION, (char *)NULL,	/* kill batch files */
'v', '\0', &verbose, DNC, DNC,	NUMBER, (char *)NULL,	/* verbosity level */
'F', '\0',   &filef, FILT,FILT,	OPTION, (char *)NULL,	/* filter to files */
#ifdef DEBUG
'D', '\0',  &debug,  DNC, DNC,	NUMBER, (char *)NULL,	/* debug level */
#endif /* DEBUG */
'\0','\0',   0,	     0,	   0,	0,      (char *)NULL
};

main(argc, argv)
int     argc;
char    *argv[];
{
    feed_t      	*target;
    forward void        newsbatch();
    static DIR		*directory;
    static struct dirent    *entry;

    if (procopts(argc, argv, DNC, options) == FAIL)
	xerror0("sendbatch: usage is sendbatch [-DFfkv] [-sx] sysname...");
    if (fileg)
	nochk = TRUE;

#ifdef DEBUG
    /*
     * if invoking user doesn't have permissions to modify privileged things,
     * assume that this is a debugging run and we want instrumentation.
     */
    if (!privileged)
	debug = 1;
    if (debug > verbose)
	verbose = debug;
#endif /* DEBUG */

    newsinit();	/* set up defaults and initialize. */
#ifdef DEBUG
    if (!debug)
#endif /* DEBUG */
	loginit();

    if (chdir(site.batchdir))
        xerror1("Couldn't get to batch directory %s", site.batchdir);
    (void) rdactive(NULLPRED);	/* history code needs the active-group data */
    hstread(TRUE);

    if (filef)		/* filter an ID list on input to yield filenames */
    {
	while (fgets(sendto, BUFLEN, stdin) != (char *)NULL)
	{
	    char	*copy;

	    (void) nstrip(sendto);
	    if ((copy = hstfile(sendto)) == (char *)NULL)
		log1("couldn't get a copy of %s", sendto);
	    else
		(void) printf("%s\n", copy);
	}
    }
    else		/* do transmissions */
    {
	s_read();

	/* we'll need these files to accumulate the batches in */
	(void) mktemp(outfile);
	(void) mktemp(batch);

	/* first get access to the batch directory */
	if ((directory = opendir(site.batchdir)) == (DIR *)NULL)
	{
	    log0("can't access news batch directory");
	    xxit(1);
	    /*NOTREACHED*/
	}

	/* process each file in the directory */
	while ((entry = readdir(directory)) != (struct dirent *)NULL)
	{
	    /* skip . and .. and files that are supposed to be invisible */
	    if (entry->d_name[0] == '.')
		continue;

	    /* do batch sends for each file that matches the send argument */
	    if (ngmatch(entry->d_name, sendto))
	    {
		if ((target = s_find(entry->d_name)) == (feed_t *)NULL)
		    xerror1("couldn't find feed data for %s", entry->d_name);
		else
		    newsbatch(entry->d_name, target);
		if (killf)
		    (void) unlink(entry->d_name);
	    }
	}
    }

    return(SUCCEED);
}

private long catart(fname, extra, ofp)
/* add the given article to the batch on ofp */
char    *fname;	/* the message instance */
char	*extra;	/* extra per-article info to be passed through */
FILE    *ofp;   /* file pointer to the batch */
{
    register int    c;
    register bool   delete = FALSE;
    register long   n, laststart = ftell(ofp);
    register FILE   *nfp;
    struct stat     sbuf;
#ifdef ARTFILTER
    char	    *fn;
#endif /* ARTFILTER */

#ifdef ARTFILTER
    /* apply any per-article filtering */
    if (fn = artfilter(sp, fname))
    {
        if (delete)
            (void) unlink(fname);
        fname = fn;
        delete = TRUE;
    }
#endif /* ARTFILTER */

    /* create a new batch section containing the contents of fname */
retry:
    nfp = msgopen(fname);
    if (nfp == (FILE *)NULL || fstat(fileno(nfp), &sbuf))
        xerror0("a batch section has vanished, probably trashed by expire");
    else
        (void) fprintf(ofp, "#! rnews %ld %s\n", (long)sbuf.st_size, extra);
    n = 0;
    while ((c = getc(nfp)) != EOF)
    {
        (void) fputc(c, ofp);
        n++;
    }
    (void) fclose(nfp);

    /* this is strictly from paranoia, but humor me */
    if (n != sbuf.st_size)
    {
        /* back over the message */
        (void) fseek(ofp, (off_t)laststart, SEEK_SET);
	/* and rewind the source */
        (void) fseek(nfp, (off_t)0, SEEK_SET);
        logerr3("%s, expected %ld bytes, got %ld",
		fname, (long)n, (long)sbuf.st_size);
        goto retry;
    }

    /* if we've made a copy at some point, delete it */
    if (delete)
        (void) unlink(fname);

    return(sbuf.st_size);
}

void newsbatch(target, sp)
/* ship all pending messages to a given system */
char	*target;	/* system list to send to */
feed_t  *sp;            /* data on the link */
{
    register char *cp, *tp;
    forward void redirect(), uuxbatch();
    off_t maxbytes, qlen, spoolmin;

    /* Find the batch list at BATCH/<system(s)>... */
    (void) snprintf(batchname, sizeof(batchname), "%s/%s", site.batchdir, target);

    /*
     * ...so we can exclusive-lock it. This may involve waiting on
     * another sendbatch to let go of it.
     */
    (void) filelock(batchname);

    /* check that all UUCP queues for target systems are below threshold */
    if (!nochk && (cp = s_option(sp, 'Q')))
    {
	tp = strtok(target, ",");
	do {
	    if ((qlen = uuq(target)) == FAIL || qlen < atol(cp))
	    {
		logerr3("Can't batch to %s, %s uucp queue is %ld",
			target, tp, qlen);
		return;
	    }
	} while
	    (tp = strtok((char *)NULL, ","));
    }

    if (!nochk && (maxbytes = df(UUCPDIR)) >= 0
	&& maxbytes < (spoolmin = atoi(newsattr("spoolmin", SPOOLMIN))))
    {
        logerr1("Can't batch to %s, too low on uucp spool space", target);
        return;
    }

    if (verbose >= V_SHOWSYS)
	(void) fprintf(stderr, "Batching to system(s) %s\n", target);

    /* check that batching is OK here */
    if ((cp = s_option(sp, 'B')) == (char *)NULL)
    {
        logerr1("Batching not enabled for system(s) %s", target);
        return;
    }

    /* skip the 'o', if present */
    while (*cp && !isdigit(*cp))
	cp++;
    if ((maxbytes = atol(cp)) == 0)
        maxbytes = spoolmin;

    /* now transmit the contents of that batch to the target */
    uuxbatch(target, sp, batchname, maxbytes);

    /* we're done, release the batch file */
    (void) fileunlock(batchname);
}

/*
 * The following function dispatches the articles listed in a batch file
 * to their destinations. It breaks the transmission up into chunks of
 * at most maxbytes bytes.
 */

private void uuxbatch(target, sp, batchfile, bytesleft)
/* execute the commands implied by a batch file */
char	*target;	/* target system(s) to send to */
feed_t	*sp;		/* for xmit command and options */
char	*batchfile;	/* the file */
long	bytesleft;	/* maximum size of batch sections */
{
    char    		linbuf[BUFLEN], *batchline, *fname, *extra = linbuf;
    register FILE	*fp, *nfp, *ofp;
    forward void	uuxfile();
    int			goodsends;

    /* if we're just sending an ihave list, do that and exit */
    if (!fileg && s_option(sp, 'N') != (char *)NULL)
    {
	char	titbuf[SBUFLEN], ngbuf[SBUFLEN];

	(void) snprintf(titbuf, sizeof(titbuf), "ihave %s", site.nodename);
	(void) snprintf(ngbuf, sizeof(ngbuf), "to.%s.ctl", sp->s_name);
	(void) xmitctrl(target, sp, titbuf, ngbuf, (char *)NULL);
	return;
    }

    do {
#ifdef DEBUG
	if (debug)	    /* so we can test without news permissions */
	{
	    if ((fp = fopen(batchfile, "r")) == (FILE *)NULL)
		xerror2("fopen(%s,r) %s", batchfile, sys_errlist[errno]);
	}
	else
#endif /* DEBUG */
	/*
	 * Rename real file to a work name to avoid race conditions.
	 * If workfile already exists, skip the rename in order
	 * to recover from a crash without losing anything.
	 */
	{
	    (void) strlcpy(workfile, batchfile, sizeof(workfile));
	    if (!fileg)
	    {
		(void) strlcat(workfile, ".work", sizeof(workfile));
		if (access(workfile, F_OK) < 0)
		{
		    if (access(batchfile, F_OK) < 0 && errno == ENOENT)
			return;	/* no news */
		    if (rename(batchfile, workfile) < 0)
		    {
			logerr3("rename(%s,%s) %s",
				sp->s_name, workfile, sys_errlist[errno]);
			return;
		    }
		}
	    }
	    if ((fp = fopen(workfile, "r")) == (FILE *)NULL)
	    {
		logerr2("fopen(%s,r) %s", workfile, sys_errlist[errno]);
		return;
	    }
	}

	/* Set up the output file */
	if (!fileg)
	    if ((ofp = fopen(outfile, "w")) == (FILE *)NULL)
		continue;

	/* assemble the next transmission section from the ID list */
	goodsends = 0;
	while ((batchline=fgets(linbuf,sizeof(linbuf),fp)) && bytesleft>0)
	{
	    (void) nstrip(linbuf);

	    /* we only want to key on the first token */
	    while (*extra && !isspace(*extra))
		extra++;
	    if (*extra)
		*extra++ = '\0';    /* keep around the rest of the line */

	    /* try to fetch a copy of the article sought */
	    if ((fname = hstfile(linbuf)) == (char *)NULL)
	    {
		logerr1("no copy of %s available", linbuf);
		continue;
	    }
	    else
	    {
		goodsends++;
		if (verbose >= V_SHOWPARTS)
		    (void)fprintf(stderr,
				  "sendbatch: adding %s to batch\n", linbuf);
	    }

	    /* time to do the actual concatenation */
	    if (fileg)
		(void) fprintf(stdout, "%s\n", linbuf);
	    else
		bytesleft -= catart(fname, extra, ofp);
	}

	/*
	 * If we've exceeded the maximum byte size and there are still
	 * ID lines in the file, batchline points to the ID we failed on,
	 * and we must make sure the remainder of the file will get
	 * picked up on the next cycle.
	 */
	if (batchline)
	{
	    char ltmpfile[SBUFLEN];

	    (void) umask(2);
	    (void) strlcpy(ltmpfile, batchfile, sizeof(ltmpfile));
	    (void) strlcat(ltmpfile, ".tmp", sizeof(ltmpfile));
	    if ((nfp = fopen(ltmpfile, "w")) == (FILE *)NULL)
	    {
		logerr2("fopen(%s,w) %s", ltmpfile, sys_errlist[errno]);
		return;
	    }
	    do {
		(void) fputs(linbuf, nfp);
	    } while
		(fgets(linbuf, sizeof(linbuf), fp) != (char *)NULL);
	    (void) fclose(nfp);
		(void) fclose(fp);

	    /* put the file where we'll pick it up next time thru */
	    if (rename(ltmpfile, workfile) < 0)
		logerr3("rename(%s,%s) %s",
			ltmpfile, workfile, sys_errlist[errno]);
	}

	/* OK, we're ready to transmit */
	if (!fileg)
	{
	    (void) fclose(ofp);
	    if (goodsends > 0)
		uuxfile(target, sp, outfile);
	    (void) unlink(outfile);
	}
    } while
	/* if there were leftover IDs, go back around */
	(batchline != (char *)NULL);

    /* if we got through the entire workfile, trash it */
#ifdef DEBUG
    if (!debug)
#endif /* DEBUG */
	if (!fileg)
	    (void) unlink(workfile);
}
 
void uuxfile(target, sp, fname)
/* transmit a batch section to a given remote system */
char	*target;	/* target system(s) to send to */
feed_t  *sp;            /* data on the link */
char    *fname;         /* name of the batch to send */
{
    char    *fn;
    int	     ifd, ofd, len;

#ifdef B211COMPAT
    /*
     * if the remote system has a .cmd file in the batch directory, use
     * it as a transmission method; if not, do a normal transmit
     */
    (void) snprintf(bfr, LBUFLEN, "%s/%s.cmd", BATCH, sp->s_name);
    if (access(bfr, F_OK) == SUCCEED)
    {
	(void) snprintf(bfr, LBUFLEN,
	    "%s/%s.cmd <%s %s", BATCH, sp->s_name, fname, syslist);
#ifdef DEBUG
	if (debug)
	    (void) printf("batchxmit: would execute %s\n", bfr);
	else
#endif /* DEBUG */
	    (void) system(bfr);
	(void) unlink(fname);
	return;
    }
#endif /* B211COMPAT */

    /* filter the entire file if indicated */
    if (fn = filefilter(sp, fname))
	fname = fn;

    if ((ofd = creat(batch, 0600)) < 0)
    {
	(void) fprintf(stderr, "%s uuxfile: %s", Progname, batch);
	xxit(1);
    }

      /* generate the right kind of header */
      if (strchr(s_option(sp, 'B'), 'o') == (char *)NULL)
  	if (s_option(sp, 'C') == (char *)NULL)
	    (void) write(ofd, UNBATCH, sizeof(UNBATCH) - 1);
	else if (s_option(sp, 'E'))
	    (void) write(ofd, CUNBATCH, sizeof(CUNBATCH) - 1);
	else
	    (void) write(ofd, C7UNBATCH, sizeof(C7UNBATCH) - 1);
    
    /* now copy the rest of the batch to the transmission file */
    if ((ifd = open(fname, O_RDONLY)) < 0)
    {
	(void) fprintf(stderr, "%s uuxfile: %s", Progname, fname);
	xxit(1);
    }
  
    while (len = read(ifd, bfr, (unsigned)sizeof(bfr)))
	(void) write(ofd, bfr, (unsigned)len);

    (void) close(ifd);
    (void) unlink(fname);
    (void) close(ofd);
  
    (void) transmit(target, sp, (char*)NULL, batch, FALSE, TRUE);
    (void) unlink(batch);
}

catch_t xxit(status)
/* exit and cleanup */
int status;
{
    /* we don't remove the workfile because we can pick it up next time */
    (void) unlink(batch);
    (void) unlink(outfile);
    unlock();
    exit(status);
}

/* sendbatch.c ends here */
