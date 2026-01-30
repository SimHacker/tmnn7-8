/*****************************************************************************

NAME
   tofilter.c -- comm primitives for reader/newsfilter transactions

SYNOPSIS
   #include "libfilt.h"

   char	*fltinfo;

   int fltinit(filter, flang, fversion)	-- initialize filter, check version
   char *filter, *flang, *fversion;

   int fltarticle(id, file)	-- return `interest score' of article
   char *id, *file;

   int fltnewsgroup(name)		-- return `interest score' of group
   char *name;

   int fltprogram(command)		-- program the filter
   char *command;

   int fltbye(filter)		-- drop the named filter
   char *filter;

DESCRIPTION
   These functions may be used by a reader to communicate with a news
filter program.

   The fltinit() function invokes the filter process. The arguments are a
filter process name and an expected version level. The function returns
FAIL if the process cannot be invoked, TRUE if the newsfilter returns a
language name different than the elang one given, or if the version string
lexically greater than the expect string, FALSE otherwise.

   The fltarticle() function conducts all necessary dialogue with the
newsfilter to determine the `interest score' of an article, which it returns.
If fltinit() failed or the filter process has died it returns a constant value
of 0. The filename argument is optional; if NULL, the protocol will exchange
portions of the article over pipes or some other form of IPC to the filter.

   The fltnewsgroup() function performs a similar service for newsgroups. A
score of zero designates a newsgroup for which fltarticle() should be called
individually on each article.

   The fltprogram() function downloads filter-command code to the newsfilter.
Interpretation of this code is entirely up to the newsfilter, which may conduct
a query dialogue with the reader (as with the fltarticle() function) to obtain
portions of the article needed to interpret the command. The filename argument
is optional; if NULL, the protocol will exchange portions of the article over
pipes or some other form of IPC to the filter.

   The fltbye() command disconnects from a named filter. If the argument is
NULL, all filters are dropped.

NOTE
   This code can be exercised from newsdb(1).
   At present, only one filter process per reader is supported. Someday
this library will support filter stacks; for this reasion, fltbye() takes
a filter name argument which is currently not used.

AUTHOR
   Eric S. Raymond, from a protocol spec developed with Brad Templeton.
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

*****************************************************************************/
/* LINTLIBRARY */
#include "news.h"
#include "vio.h"
#include "header.h"
#include "active.h"
#include "procopts.h"
#include "server.h"
#include "transact.h"
#include "libfilt.h"

#define STARTTIME	20	/* allow this many seconds of startup time */

char	*fltinfo;

static bool filteron = FALSE;
static fresp_t	resp;

static int ftransact(type, code, arg1, arg2, arg3, arg4, arg5)
char	type;
char	code;
char	*arg1, *arg2, *arg3, *arg4, *arg5;
{
    static int		seqno = 0;

    if (ftransmit(type, code, ++seqno, arg1, arg2, arg3, arg4, arg5)==FAIL)
	return(FAIL);
    else if (freceive(&resp) == FAIL)
	return(FAIL);
    else
	return(seqno);
}

static catch_t deadkid()
{
    msg0("Newsfilter process aborted!");
    filteron = FALSE;
}

int fltinit(filt, flang, fversion)
char	*filt, *flang, *fversion;
{
    server_t	*srv;
    int		ansok;
    catch_t	(*oalarm)();
    char	*largv[MAXARGS], *defilt, *deflang, *p;
    char	pathbuf[BUFLEN];

    /* figure out who we ought to be invoking */
    if ((defilt = getenv("NEWSFILTER")) == (char *)NULL)
	defilt = "rnkill";
    if ((deflang = strchr(defilt, ':')) != (char *)NULL)
	*deflang++ = '\0';
    else
	deflang = defilt;
    if (filt == (char *)NULL)
	filt = defilt;
    if (flang == (char *)NULL)
	flang = deflang;

    /* in case child dies, avoid this signal */
    (void) signal(SIGPIPE, deadkid);

    /* OK, let's go open the relevant process now */
#ifdef DEBUG
    if (debug)
	(void) sprintf(bfr, "%s -D -p", filt);
    else
#endif /* DEBUG */
	(void) sprintf(bfr, "%s -p", filt);
    (void) vcrack(bfr, largv, MAXARGS);

    /* wedge the user's dot-file directory into his/her execution path */
    if ((p = getenv("DOTDIR")) == (char *)NULL)
	p = userhome;
    (void) strcpy(pathbuf, p);
    if (p = getenv("PATH"))
    {	
	if (*p != ':')
	    (void) strcat(pathbuf, ":");
	(void) strcat(pathbuf, p);
    }
    (void) setenv("PATH", pathbuf);

    if ((srv = srvopen(filt, largv)) == (server_t *)NULL)
	return(FAIL);

    /* tell the packet level about it */
    fconnect(srv->readsrv, srv->writesrv);

    /* expect the initial 'version' pseudo-response from the filter */
    oalarm = signal(SIGALRM, SIG_IGN);
    (void) alarm(STARTTIME);		/* give process time to start */
    ansok = freceive(&resp);		/* look for a 'response' */
    (void) signal(SIGALRM, oalarm);	/* we got one, or were alarmed */
    (void) alarm(0);

    fltinfo = "";
    if (ansok == FAIL || resp.code != 'O')
    {
	msg1("Filter \"%s\" could not be spawned.", filt);
	(void) kill(srv->pid, SIGKILL);
	return(FAIL);
    }
    else if (ftransact('C','V',VPROTO, NOARG,NOARG,NOARG,NOARG) == FAIL)
    {
	msg1("Filter \"%s\" is not responding!", filt);
	(void) kill(srv->pid, SIGKILL);
	return(FAIL);
    }
    else if (resp.code != 'V')
    {
	msg1("Filter \"%s\" sent back an incorrect Version response!", filt);
	(void) kill(srv->pid, SIGKILL);
	return(FAIL);
    }
    else if (strcmp(flang, resp.arg4))
    {
	msg1("Filter \"%s\" did not return the expected language!", filt);
	(void) kill(srv->pid, SIGKILL);
	return(FAIL);
    }
    else if (atoi(resp.arg1 + 1) < atoi(fversion + 1))
    {
	msg3("%s protocol version (%s) doesn't match reader's (%s)!",
	     filt, resp.arg1, fversion);
	(void) kill(srv->pid, SIGKILL);
	return(FAIL);
    }
    else
    {
	filteron = TRUE;
	return(SUCCEED);
    }
}

int fltarticle(id, fn)
/* get an accept-reject status for a given article */
char	*id, *fn;
{
    if (!filteron)
    {
	fltinfo = "";
	return(0);
    }

    (void) ftransact('C','A', id, "F", fn, NOARG, NOARG);
    if (resp.code == 'A' || resp.code == 'R')
    {
	fltinfo = resp.arg2;
	return(atoi(resp.arg1));
    }
    else
    {
	fltinfo = resp.arg1;
	return(0);
    }
}

int fltnewsgroup(name)
/* get an accept-reject status for a newsgroup */
char	*name;
{
    if (!filteron)
    {
	fltinfo = "";
	return(0);
    }

    (void) ftransact('C', 'N', name, NOARG, NOARG, NOARG, NOARG);
    if (resp.code == 'O')
    {
	fltinfo = resp.arg1;
	return(0);
    }
    else
    {
	fltinfo = resp.arg2;
	return(atoi(resp.arg1));
    }
}

int fltprogram(command)
/* program the filter */
char	*command;
{
    if (!filteron)
    {
	fltinfo = "";
	return(FAIL);
    }

    (void) ftransact('C', 'P', command, NOARG, NOARG, NOARG, NOARG);
    fltinfo = resp.arg1;
    if (resp.code == 'O')		/* everything is OK */
	return(SUCCEED);
    else
	return(FAIL);		/* we returned an error */
}

/*ARGSUSED1*/
int fltbye(filtername)
/* drop the filter */
char	*filtername;
{
    if (!filteron)
    {
	fltinfo = "";
	return(0);
    }

    (void) ftransact('C', 'Q', NOARG, NOARG, NOARG, NOARG, NOARG);
    fltinfo = resp.arg1;
    if (resp.code == 'O')
	return(SUCCEED);
    else
	return(FAIL);
}

/* tofilter.c ends here */
