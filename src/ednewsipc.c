/*****************************************************************************

NAME
   ednewsipc.c -- control 'ednews' as a server

SYNOPSIS
   #include "ednewsipc.h"

   int open_ednews(argv)	-- open an ednews server
   char argv[];

   int write_ednews(cmd)	-- write a command to the server
   char *cmd;

   int read_ednews(session)	-- read a response from the server
   ednews_t *session;

   void close_ednews()		-- close an ednews server

DESCRIPTION
   This module uses server.c to control an ednews server. Use it to do IPC
to ednews when building a windowing news front-end. Ednews's responses are
parsed into a structure defined in ednews.h.
   This is *not* the source for the ednews executable, which is actually
compiled from an ednews.c derived from readnews.c.

NOTE
   Compiling this module with -DMAIN yields an interactive tester executable
named ednewsipc. Command line arguments given ednewsipc will be passed to
the ednews subprocess.

BUGS
   The 'BYE' response from a child ednews can get lost if the child gives
up the ghost before read_ednews() does its fgets(). On AT&T UNIXes we can
trap SIGCLD and use it to post the 'BYE' condition; on V7/BSD versions
we have to make the child wait until the 'BYE' is acknowledged, or kill
it explicitly from the parent.

AUTHOR
   Eric S. Raymond
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

*****************************************************************************/
/*LINTLIBRARY*/
#include "news.h"
#include "server.h"
#include "active.h"
#include "newsrc.h"
#include "ednewsipc.h"

#define CMDCHAR	'\007'	/* ednews's command-prefix char */

private server_t    *server;		/* server data for ednews process */
private char	    buf[BUFLEN];	/* scratch buffer for everybody */

#ifdef SIGCLD
private bool	    terminated;		/* TRUE if sigcld has been seen */

private void sigcld()
{
    terminated = TRUE;
}
#endif /* SIGCLD */

int open_ednews(argv)
/* open ednews as a server */
char	*argv[];
{
#ifndef MAIN
    (void) sprintf(buf, "%s/ednews", site.libdir);
#else
    (void) strcpy(buf, "ednews");
#endif
    if (server = srvopen(buf, argv))
    {
#ifdef SIGCLD
	(void) signal(SIGCLD, SIGCAST(sigcld));
#endif /* SIGCLD */
	return(SUCCEED);
    }
    else
	return(FAIL);
}

int read_ednews(rsp)
/* parse a response from the server into a session block */
ednews_t    *rsp;
{
#ifdef SIGCLD
    if (terminated)
    {
	rsp->type = ED_BYE;
	return(SUCCEED);
    }
#endif /* SIGCLD */

    if (read(server->readsrv, rsp->text, LBUFLEN) == FAIL)
	return(ED_ERROR);
    else if (rsp->text[0] != CMDCHAR)
    {
	rsp->type = ED_MESSAGE;
    }
    else switch(rsp->text[1])
    {
    case 'G':		/* GROUP */
	if (sscanf(rsp->text,
#ifdef BIGGROUPS
		"%*s %s %ld %ld %ld %*s\n",
#else
		"%*s %s %d %d %d %*s\n",
#endif /* BIGGROUPS */
		rsp->groupname,
		&(rsp->gp.ng_min), &(rsp->gp.ng_max), &(rsp->gp.ng_unread)
	    ))
	{
	    char *cp = strchr(rsp->text, SUBSCMK);

	    rsp->gp.ng_name = rsp->groupname;
	    if (cp)
		rdbits(SET | CLEAR, cp + 1, &rsp->gp);
	    rsp->type = ED_GROUP;
	}
	else
	    rsp->type = ED_ERROR;
	break;

    case 'A':		/* ARTICLE */
	if (sscanf(rsp->text, "%*s %s %d %s\n",
			rsp->artfile, &rsp->lines, rsp->text) == 0)
	    rsp->type = ED_ERROR;
	else
	{
	    rsp->type = ED_ARTICLE;
	    rsp->seen = (rsp->text[0] == 's');
	}
	break;

    case 'T':		/* TOPICS */
	rsp->type =
	    (sscanf(rsp->text, "%*s %s %d\n", rsp->topics, &(rsp->ngtotwait)))
		? ED_TOPICS : ED_ERROR;
	break;

    case 'S':		/* SUBJECTS */
	rsp->type =
	    (sscanf(rsp->text, "%*s %s\n", rsp->text))
		? ED_SUBJECTS : ED_ERROR;
	break;

    case 'D':		/* DONE */
	rsp->type =
	    (sscanf(rsp->text, "%*s %x %d\n", &(rsp->action)))
		? ED_DONE : ED_ERROR;
	break;

    case 'N':		/* NOMORE */
	rsp->type = ED_NOMORE;
	break;

    case 'B':		/* BYE */
	rsp->type = ED_BYE;
#ifndef SIGCLD
	(void) fputs("BYE", server->writesrv);
#endif /* !SIGCLD */
	break;

    case 'C':		/* CMDERR */
	rsp->type = ED_CMDERR;
	break;

    default:
	rsp->type = ED_ERROR;
	break;
    }
    return(SUCCEED);
}

int write_ednews(str)
/* write to the server */
char	*str;
{
#ifndef lint	/* BSD lint gets upset when you try to use an fputs return */
    if (write(server->writesrv, str, strlen(str)) == FAIL)
	return(FAIL);
    else
#endif /* lint */
	return(SUCCEED);
}

void close_ednews()
/* close ednews */
{
    srvclose(server);
}

#ifdef MAIN
/* interactive tester is under construction here */

char *Progname = "ednewsipc";

int main(argc, argv)
/* test ednews server execution */
int	argc;
char	**argv;
{
    ednews_t	session;
    int		status;

    /* start up an ednews child */
    if (open_ednews(argv) == FAIL)
    {
	(void) fprintf(stdout, "Server startup failure!\n");
	exit(1);
    }

    /* interpret responses forever */
    while (read_ednews(&session) != FAIL)
    {
	char	cmdbuf[EBUFLEN], *cp;

	if (cp = strchr(session.text, '\n'))
	    *cp = '\0';

	switch(session.type)
	{
	case ED_ERROR:
	    (void) printf("ED_ERROR: response '%s' garbled\n", session.text);
	    break;

	case ED_MESSAGE:
	    (void) printf("ED_MESSAGE: %s\n", session.text);
	    break;

	case ED_GROUP:
	    (void) printf("ED_GROUP: %s, min %ld, max %ld unread %ld\n",
			session.gp.ng_name,
			(long) session.gp.ng_min, (long)session.gp.ng_max,
			(long) session.gp.ng_unread);
	    break;

	case ED_ARTICLE:
	    (void) printf("ED_ARTICLE: %d lines, file is %s\n* ",
				session.lines, session.artfile,
				session.seen ? "seen" : "useen");
	    (void) fgets(cmdbuf, BUFSIZ, stdin);
	    if ((status = write_ednews(cmdbuf)) == FAIL)
		(void) printf("Write to server failed\n");
	    break;

	case ED_TOPICS:
	    (void) printf("ED_TOPICS: file is %s\n", session.topics);
	    break;

	case ED_SUBJECTS:
	    (void) printf("ED_SUBJECTS: file is %s\n", session.text);
	    break;

	case ED_DONE:
	    (void) printf("ED_DONE: message is %s\n", session.text);
	    break;

	case ED_NOMORE:
	    (void) printf("ED_NOMORE: no more articles\n* ");
	    (void) fgets(cmdbuf, BUFSIZ, stdin);
	    if ((status = write_ednews(cmdbuf)) == FAIL)
		(void) printf("Write to server failed\n");
	    break;

	case ED_CMDERR:
	    (void) printf("ED_CMDERR: no more articles\n");
	    (void) fgets(cmdbuf, BUFSIZ, stdin);
	    if ((status = write_ednews(cmdbuf)) == FAIL)
		(void) printf("Write to server failed\n");
	    break;

	case ED_BYE:
	    (void) printf("ED_BYE: server is exiting, goodbye\n");
	    exit(0);
	}
    }
}
#endif /* MAIN */

/* ednewsipc.c ends here */
