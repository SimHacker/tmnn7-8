/****************************************************************************

NAME
   readinit.c -- code common to all readers

SYNOPSIS
   #include "news.h"

   int readinit(argc, argv, hook)	-- the full news reader initialization
   int argc; char *argv[]; int (*hook)();

   void hprint(hp, vlevel, hilite)	-- print the header of an article
   hdr_t *hp; int vlevel; void (*hilite)();

   void cleanup()			-- interrupt handling code

DESCRIPTION
   Functions used by all readers live here.
   The readinit() sequence is the full reader initialization call with
signal trapping and environment variable checks. It calls the main entry
point of checkinit.c, which see.
   The hprint() code expects to be able to call the vprint[01]() macros to
do output through the interface manager.
   The cleanup() function provides a common exit-with-cleanup for all
readers; it writes out a count of messages read.

REVISED BY
   Eric S. Raymond
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

****************************************************************************/
/*LINTLIBRARY*/
#include "news.h"
#include "vio.h"
#include "header.h"
#include "active.h"
#include "newsrc.h"	/* to declare wrnewsrc() and make lint happy */
#include "libread.h"
#include "dballoc.h"	/* session.h needs the dbdef_t type */
#include "session.h"	/* for msgsweep() */

/* control variables */
int	flushall = FALSE;	/* if set TRUE, throw away all news */
int	nochange = FALSE;	/* if set TRUE, .newsrc writeout suppressed */
int	asave = FALSE;		/* if set TRUE, .newsrc autosave on */
char	inclmark[SBUFLEN] = ">";	/* included-text mark */
#ifdef DEBUG
char	usetty[SBUFLEN];	/* which tty to attach to */
#endif /* DEBUG */

private catch_t ronsig(n)
/* trap interrupts for everybody */
int	n;
{
#if !defined(BSD4_2) && !defined(SYSV3)
    /*
     * Older UNIX systems reset caught signals to SIG_DFL.
     * This bad design requires that the trap be set again here.
     * Unfortunately, if the signal recurs before the trap is set,
     * the program will die.
     */
    (void) signal(n, ronsig);
#endif /* !defined(BSD4_2) && !defined(SYSV3) */
    sigcaught = n;
}

int readinit(argc, argv, hook)
/* full initialization sequence for a reader */
int	argc;		/* count of command-line arguments */
char	*argv[];	/* the arguments themselves */
int	(*hook)();	/* .newsrc directive-processing hook */
{
    int	waiting;

    privileged = uid && strcmp(site.notify, username);

    /* process options specified in the environment */
    (void) lprocargs(getenv("NEWSOPTS"));

#ifndef DEBUG
    (void) signal(SIGQUIT, SIGCAST(SIG_IGN));
#endif /* DEBUG */
#ifdef SIGWIND
    (void) signal(SIGWIND, SIGCAST(SIG_IGN));	/* for the AT&T 7300 */
#endif /* SIGWIND */
#ifdef SIGPHONE
    (void) signal(SIGPHONE, SIGCAST(SIG_IGN));	/* for the AT&T 7300 */
#endif /* SIGPHONE */
    (void) signal(SIGHUP, ronsig);
    (void) signal(SIGINT, ronsig);
    (void) signal(SIGPIPE, ronsig);

    /* do lower-level initialization with .newsrc file option scan */
    waiting = checkinit(argc, argv, hook);

#ifdef DEBUG
    /* user may wish to redirect terminal I/O for debugging purposes */
    if (usetty[0])
    {
	/* compute the required full path name for the I/O device */
	if (usetty[0] == '/')
	    (void) strcpy(bfr, usetty);
	else
	    if (isdigit(usetty[0]))
		(void) sprintf(bfr, "/dev/tty%s", usetty);
	    else
		(void) sprintf(bfr, "/dev/%s", usetty);

	/* now attach us to it */
	(void) printf("Attaching to %s\n", bfr);
	(void) close(0);
	if (open(bfr, O_RDONLY) == FAIL)
	{
	    (void) printf("Redirect of stdin failed, errno = %d\n", errno);
	    exit(0);
	}
	(void) close(1);
	if (open(bfr, O_WRONLY) == FAIL)
	{
	    (void) printf("Redirect of stdout failed, errno = %d\n", errno);
	    exit(0);
	}
	(void) printf("Attached to %s\n", bfr);
    }
#endif /* DEBUG */

   return(waiting);
}

void hprint(hp, vlevel, hilite)
/* display a header in a somewhat condensed format */
register hdr_t	*hp;		/* header of the message being investigated */
int		vlevel;		/* verbosity level */
void		(*hilite)();	/* highlight indicator, FALSE if none */
{
    register char   *p1, *p2;
    char	    fname[BUFLEN];

    /* display the Subject line, possibly highlighted */
    if (hilite == NOHIGH || vlevel == P_SLOWTTY)
	(void) sprintf(bfr, "Subject: %s", hp->h_subject);
    else
    {
	(*hilite)(fname, hp->h_subject);
	(void) sprintf(bfr, "Subject: %s", fname);
    }

    if (vlevel >= P_FULLHDRS)
    {
	/* print most of the RFC-1036 non-noise headers */
	vprint0(bfr);			/* the Subject header */
	if (site.smarthost[0])
	    vprint1("Path: %s", hp->h_path);
	else
	    vprint1("From: %s", hp->h_from);
	if (hlnblank(hp->h_organization))
	    vprint1("Organization: %s", hp->h_organization);
	vprint1("Date: %s", hp->h_postdate);
	if (hlnblank(hp->h_sender))
	    vprint1("Sender: %s", hp->h_sender);
	if (hlnblank(hp->h_replyto))
	    vprint1("Reply-To: %s", hp->h_replyto);
	if (hlnblank(hp->h_followto))
	    vprint1("Followup-To: %s", hp->h_followto);
    }
    else if (site.smarthost[0])
    {
	(void) strncpy(fname, hp->h_from, sizeof(fname) - 1);
	fname[sizeof(fname) - 1] = '\0';

	/* if a fullname is present, append the organization (if any) */
	if ((p1 = strchr(fname, '(')) && (p2 = strchr(p1, ')')) &&
	    hlnblank(hp->h_organization) && vlevel > P_SLOWTTY)
	{
	    *p2 = '\0';
	    (void) strcat(fname, " @ ");
	    (void) strcat(fname, hp->h_organization);
	    (void) strcat(fname, ")");
	}

	vprint1("From: %s", fname);
	vprint0(bfr);			/* the Subject header */
    }
    else
    {
	/* Extract sender's full name into a buffer for later use */
	fname[0] = '\0';	/* initialize the name holder */

	/* Find the sender's full name. */
	p1 = strchr(hp->h_from, '(');
	if (p1 == (char *)NULL && hlnblank(hp->h_path))
	    p1 = strchr(hp->h_path, '(');
	if (p1 != (char *)NULL)
	{
	    (void) strncpy(fname, p1+1, sizeof(fname) - 1);
	    fname[sizeof(fname) - 1] = '\0';
	    p2 = strchr(fname, ')');
	    if (p2 != (char *)NULL)
		*p2 = '\0';
	}

	/* if we print the From: anyhow, arrange we don't see the fullname */
	if (p1 != (char *)NULL)
	    *--p1 = '\0';

	vprint1("Path: %s", tailpath(hp));

	/* plug the gap that we created in the From: header */
	if (p1 != (char *)NULL)
	    *p1 = ' ';

	/* tack author's full name onto subject header */
	if (fname[0] || hlnblank(hp->h_organization))
	{
	    (void) strcat(bfr, " (");
	    if (fname[0] == '\0')
	    {
		(void) strncpy(fname, hp->h_from, sizeof(fname) - 1);
		fname[sizeof(fname) - 1] = '\0';
		p2 = strchr(fname,'@');
		if (p2)
		    *p2 = '\0';
	    }

	    (void) strcat(bfr, fname);
	    if (hlnblank(hp->h_organization) && vlevel > P_SLOWTTY)
	    {
		(void) strcat(bfr, " @ ");
		(void) strcat(bfr, hp->h_organization);
	    }

	    (void) strcat(bfr, ")");
	    vprint0(bfr);			/* the Subject header */
	}
    }

    /* Control line is also a must if it exists */
    if (hlnblank(hp->h_ctlmsg))
	vprint1("Control: %s", hp->h_ctlmsg);

    /* Summary and Keywords come next unless we're on a slow tty */
    if (vlevel > P_SLOWTTY && hlnblank(hp->h_summary))
	vprint1("Summary: %s", hp->h_summary);
    if (vlevel > P_SLOWTTY && hlnblank(hp->h_keywords))
	vprint1("Keywords: %s", hp->h_keywords);

    if (strcmp(hp->h_newsgroups, ngname()) != 0)
	vprint1("Newsgroups: %s", hp->h_newsgroups);
}

void cleanup()
/* write out updated .newsrc info (unless suppressed) and quit */
{
    time_t	elapsed;

    (void) signal(SIGHUP, SIGCAST(SIG_IGN));
    (void) fflush(stdout);
    if (!nochange && !session.reread)
    {
	wrnewsrc();
#ifdef FEEDBACK
	msgsweep();
#endif /* FEEDBACK */
	msgend();
    }

    if (!flushall)
    {
	elapsed = time((time_t *)NULL) - starttime;
	(void) fputs("You have been reading news for ", stdout);
	if (elapsed > 3600)
	    (void) printf("%d hour(s), %d minute(s) and %d second(s)\n",
		    (int) elapsed / 3600,
		    (int) (elapsed % 3600) / 60,
		    (int) elapsed % 60);
	else if (elapsed > 60)
	    (void) printf("%d minute(s) and %d second(s)\n",
		    (int) (elapsed % 3600) / 60, (int) elapsed % 60);
	else
	    (void) printf("%d seconds\n", (int) elapsed);
    }

    xxit(0);
}

/* readinit.c ends here */
