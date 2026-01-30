/*****************************************************************************

NAME
   filter.c -- bottom-end comm primitives for reader/newsfilter transactions

SYNOPSIS
   See below   

DESCRIPTION
   This module provides a generic main for news filter code. It expects to
be supplied with the following:

   1. a filter language name string.

   2. filter language version string

   3. A startup function. This is called in pipe mode when the filter
receives an initialization 'V' request from the news reader; in background mode
it is called immediately after startup. It receives as string
arguments the protocol version, the language name, and the version sent down.
If it returns FAIL, an error response will be sent up to the reader; otherwise
the normal acknowledgement will be sent, incorporating the first two args.

   4. A group scorer function. This is called on each 'N' command in pipe
mode only, and receives a string argument which is the newsgroup name. The
integer it returns will be sent up to the reader as a score.

   5. A article scorer function. This is called on each 'A' call in pipe mode;
in background mode it is called on every article. It expects a pointer to
an article header block.

   6. A program command processor, called on each 'P'. If it returns
SUCCEED, OK will be sent up to the reader, otherwise ERROR will be sent.

   7. A wrapup procedure. This will be called after the quit
acknowledge is sent up to the reader.

   The newsgroup and article scorers, the command processor and the wrapup
function all get an additional argument; the address of a response buffer.
Informational messages for the reader may be generated into this buffer and
will be passed back up in pipe mode (or logged to stderr on background mode).

AUTHOR
   Eric S. Raymond, from a protocol spec developed with Brad Templeton.
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

*****************************************************************************/
/* LINTLIBRARY */
#include "news.h"
#include "header.h"
#include "history.h"
#include "active.h"
#include "transact.h"
#include "libfilt.h"
#include "procopts.h"

/* filter run modes */
#define R_PIPE	0x01	/* run over pipes talking to a reader */
#define R_BKGD	0x02	/* run in background, caching selection info */

char	*Progname;	/* filter() will set this */
#ifdef lint
char	*usage = "Usage: filter -p";
#endif /* lint */

option_t options[] =
{
/*
optlet  filchar	flag        from      to       type     buf	*/
'p',	'\0',	&verbose,   R_PIPE,   R_PIPE,  OPTION,  (char *)NULL,
#ifdef DEBUG
'D',	'\0',   &debug,	    DNC,      DNC,     NUMBER,	(char *)NULL,
#endif /* DEBUG */
'\0',	'\0',	0,	    0,        0,       0,       (char *)NULL,
};

static char	response[MAXERRSIZE];

static int bkgdfilt(lang, vers, init, artscore, wrap)
/* cache selection info for a later pipe run */
char	*lang, *vers;	/* filter ID info */
int	(*init)();	/* initialization code */
int	(*artscore)();	/* article score */
void	(*wrap)();	/* wrapup code */
{
    FILE	*fp;

    if ((*init)(lang, vers, VPROTO) == FAIL)
	xerror0("startup failure");

    (void) hstread(TRUE);

    /*
     * Loop through all articles on-line, compiling information on scores.
     */
    hfree(&header);
    hstrewind();
    while (hstnext(TRUE) != FAIL)
	if (hstat() == VALID
		&& (fp = fopen(hstfile(hstid()), "r")) != (FILE *)NULL
		&& (hread(&header, 0L, fp) != 0))
	{
	    putscore(header.h_ident, (*artscore)(&header, response));
	    if (response[0])
		(void) fprintf(stderr, "%s\n", response);
	    hfree(&header);
	}

    (*wrap)(response);
    if (response[0])
	(void) fprintf(stderr, "%s\n", response);

    hstclose();
    return(SUCCEED);
}

static int pipefilt(lang, vers, init, nscore, artscore, prog, wrap)
/* state machine harness for news filter programs */
char	*lang, *vers;	/* filter ID info */
int	(*init)();	/* initialization code */
int	(*nscore)();	/* newsgroup scorer */
int	(*artscore)();	/* article score */
int	(*prog)();	/* command processor */
void	(*wrap)();	/* wrapup code */
{
    int		seqno = 0, status, score;
    fresp_t	r;
    char	numbuf[10];

    /* we talk to the reader via stdin/stdout */
    fconnect(0, 1);

    /* first thing, send it the OK response it's looking for */
    (void) ftransmit('R', 'O', 0, NOARG, NOARG, NOARG, NOARG, NOARG);

    /* crunch incoming requests until xmit error or we're told to quit */
    while ((seqno = freceive(&r)) != FAIL)
    {
	response[0] = '\0';
	switch(r.code)
	{
	case 'V':	/* version */
	    if ((*init)(r.arg1) == FAIL)
	    {
	        (void) ftransmit('R', 'E', seqno,
					NOARG, NOARG, NOARG, NOARG, NOARG);
		return(FAIL);
	    }
	    else
		status = ftransmit('R', 'V', seqno,
			  VPROTO, "AHNBPQV","ABEHORV", lang, vers);
	    break;

	case 'N':	/* check newsgroup */
	    if ((getscore(r.arg1, &score)) == FAIL)
		score = (*nscore)(r.arg1, response);
	    if (score == 0)
		status = ftransmit('R','O', seqno,
				   response[0] ? response : NOARG,
				   NOARG, NOARG, NOARG, NOARG);
	    else 
	    {
		(void) sprintf(numbuf, "%d", score);
		if (score < 0)
		    status = ftransmit('R','R', seqno,
				       numbuf, response[0] ? response : NOARG,
				       NOARG,NOARG,NOARG);
		else
		    status = ftransmit('R','A', seqno,
				       numbuf, response[0] ? response : NOARG,
				       NOARG,NOARG,NOARG);
	    }
	    break;

	case 'A':	/* check article */
	    if ((getscore(r.arg1, &score)) == FAIL)
	    {
		FILE	*fp = (FILE *)NULL;

		if (strcmp(r.arg2, "F") == 0)
		    fp = fopen(r.arg3, "r");
#ifdef FOO	/* aargh! not *another* potential NNTP connect! */
		else if (hstseek(r.arg1, FALSE) == VALID)
		    fp = fopen(hstfile(), "r");
		else
		{
		    score = P_IGNORE;
		    (void) strcpy(response, "Couldn't find id in history!");
		}
#endif

		if (fp == (FILE *)NULL)
		{
		    score = P_IGNORE;
		    (void) strcpy(response, "Article file is missing!");
		}
		else
		{
		    hfree(&header);
		    if (hread(&header, 0L, fp) == 0)
		    {
			score = P_IGNORE;
			(void) strcpy(response, "Header is unreadable!");
		    }
		    else
			score = (*artscore)(&header, response);
		    (void) fclose(fp);
		}
	    }
	    if (score == 0)
		status = ftransmit('R','O', seqno,
				   response[0] ? response : NOARG,
				   NOARG, NOARG, NOARG, NOARG);
	    else 
	    {
		(void) sprintf(numbuf, "%d", score);
		if (score < 0)
		    status = ftransmit('R','R', seqno,
				       numbuf, response[0] ? response : NOARG,
				       NOARG,NOARG,NOARG);
		else
		    status = ftransmit('R','A', seqno, 
				       numbuf, response[0] ? response : NOARG,
				       NOARG,NOARG,NOARG);
	    }
	    break;

	case 'P':	/* accept a command for the newsfilter */
	    if ((*prog)(r.arg1, response) == SUCCEED)
		status = ftransmit('R','O', seqno,
				   response[0] ? response : NOARG,
				   NOARG, NOARG, NOARG, NOARG);
	    else
		status = ftransmit('R','E', seqno,
				   response[0] ? response : NOARG,
				   NOARG, NOARG, NOARG, NOARG);
	    break;

	case 'Q':	/* we've been told to quit */
	    status = ftransmit('R','O', seqno,
			  response[0] ? response : NOARG,
			  NOARG, NOARG, NOARG, NOARG);
	    (*wrap)(response);
	    return(SUCCEED);

	default:	/* invalid call */
	    if (ftransmit('R','E', seqno,
			  "Invalid filter request",
			  NOARG, NOARG, NOARG, NOARG) == FAIL)
		return(FAIL);
	    break;
	}

	if (status == FAIL)
	    return(FAIL);
    }

    return(SUCCEED);
}

int filter(lang, vers, init, nscore, artscore, prog, wrap, argc, argv)
/* drive the filter defined by selector entry points */
char	*lang, *vers;	/* filter ID info */
int	(*init)();	/* initialization code */
int	(*nscore)();	/* newsgroup scorer */
int	(*artscore)();	/* article score */
int	(*prog)();	/* command processor */
void	(*wrap)();	/* wrapup code */
int argc;
char *argv[];
{
    int		mode;

    Progname = lang;

    newsinit();

    if ((mode = procopts(argc, argv, R_BKGD, options)) == FAIL)
    {
	(void) sprintf(bfr, "Usage: %s [-p]", argv[1]);
	xerror0(bfr);
    }

    if (rdactive(NULLPRED) == FAIL)
	xerror0("couldn't read active file!");

    switch (mode)
    {
    case R_PIPE: return(pipefilt(lang,vers,init,nscore,artscore,prog,wrap));
    case R_BKGD: return(bkgdfilt(lang, vers, init, artscore, wrap));
    default:
	(void) fprintf(stderr, "%s: unknown run mode %d\n", lang, mode);
	return(FAIL);
    }
}

#ifdef MAIN
char	*Progname;
int	report;

#define REPORT(s)	(void) write(report, s, sizeof(s) - 1)

static int ftstartup(name, version)
char	*name, *version;
{
    return(SUCCEED);
}

static int ftgroupscore(name, response)
char	*name;
char	*response;
{
    if (response != (char *)NULL)
	(void) sprintf(response, "filter: tngchk(%s)", name);
    return(SUCCEED);
}

static int ftartscore(hp, response)
hdr_t	*hp;
char *response;
{
    if (response != (char *)NULL)
	(void) sprintf(response, "filter: tartchk(%s)", hp->h_ident);
    return(SUCCEED);
}

static int ftprogram(cmd, response)
char	*cmd;
char	*response;
{
    if (response != (char *)NULL)
	(void) sprintf(response, "filter: tentercmd(%s)", cmd);
    return(SUCCEED);
}

static void ftwrapup(response)
char	*response;
{
    if (response != (char *)NULL)
	(void) sprintf(response, "filter: tendit()");
}

main(argc, argv)
int argc;
char *argv[];
{
    return(filter("filter", VPROTO, 
		  ftstartup, ftgroupscore, ftartscore, ftprogram, ftwrapup,
		  argc, argv));
}
#endif /* MAIN */

/* filter.c ends here */
