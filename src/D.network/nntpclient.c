/****************************************************************************

NAME
   nntpclient.c -- NNTP client routines.

SYNOPSIS
   #include "news.h"

   int nntpinit(resource)	-- open a server connection
   char *resource;

   void nntpget(line, size)	-- accept a line of stuff from server
   char *line; int size;

   void nntpput(line)		-- send a line of stuff to the server
   char *line;

   void nntpclose()		-- release the server link

   void nntpsync()	-- discard server responses up to terminating dot

   int nntpcommand(cmd, siz, save)	-- send command, accept a response
   char *cmd; int siz; int save;

   int nntpsnarf(cmd, tempfile)		-- send command, snarf returned file
   char *command, *tempfile;

   int nntpspew(cmd, srcfile)		-- send command, follow with file
   char *command, *srcfile;

DESCRIPTION
   These are transaction routines for communication with an NNTP server. They
are the only ones that know about the client-to-server interface; everybody
above them sees it through them. The strindex function is assumed to be
the portability library's substring finder.

BUGS
   Probably legion -- I don't have an nntp to test this on.

AUTHOR
   Eric S. Raymond
Adapted from the clientlib.c 1.2 of 4/13/86 and UNTESTED. No warranties
express or implied, your mileage may vary, and like that.
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

**************************************************************************/
/*LINTLIBRARY*/
#include "news.h"
#include "server.h"
#include "nntp.h"

#ifdef BSD4_2
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#endif /* BSD4_2 */

private server_t server;

/*
 * nntpinit  Get a connection to the news server.
 *
 *	Parameters:	"resource" is the machine or fifo to connect to.
 *
 *	Returns:	-1 on error, 0 otherwise.
 *
 *	Side effects:	Connects to server.
 */

int nntpinit(resource)
char	*resource;
{
    char	line[BUFLEN];
#ifdef BSD4_2
    /* we have sockets? then get server fds from a socket to the resource */
    if ((server.readsrv = getsocket(resource)) == FAIL
		|| (server.writesrv = dup(server.readsrv)) == FAIL)
	return(FAIL);
#else
    /* no sockets? then just spawn a child server */
    server_t	*srv;
    char	**sargv;

    sargv[0] = resource;
    sargv[1] = (char *)NULL;
    if ((srv = srvopen(resource, sargv)) == (server_t *)NULL)
    {
	perror("nntpinit: couldn't open server");
	return(FAIL);
    }
#endif /* BSD4_2 */

    /* Now get the server's signon message */
    (void) nntpget(line, sizeof(line));
    if (line[0] != CHAR_OK)
    {
	(void) close(server.readsrv);
	(void) close(server.writesrv);
	return(FAIL);		/* And abort if it's not good */
    }
    return(SUCCEED);
}


#ifdef BSD4_2
/*
 * getsocket -- get us a socket connected to the news server.
 *
 *	Parameters:	"machine" is the machine the server is running on.
 *
 *	Returns:	Socket connected to the news server if
 *			all is ok, else -1 on error.
 *
 *	Side effects:	Connects to server.
 *
 *	Errors:		Printed via perror.
 */

int getsocket(machine)
char	*machine;
{
    int	s;
    struct	sockaddr_in sin;
    struct	servent *getservbyname(), *sp;
    struct	hostent *gethostbyname(), *hp;

    if ((sp = getservbyname("nntp", "tcp")) ==  NULL)
    {
	(void) fprintf(stderr, "nntp/tcp: Unknown service.\n");
	return(FAIL);
    }

    if ((hp = gethostbyname(machine)) == NULL)
    {
	(void) fprintf(stderr, "%s: Unknown host.\n", machine);
	return(FAIL);
    }

    (void) bzero((char *) &sin, sizeof(sin));
    (void) memcpy((char *)&sin.sin_addr, hp->h_addr, hp->h_length);
    sin.sin_family = hp->h_addrtype;
    sin.sin_port = sp->s_port;

    if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0)	/* Get the socket */
    {
	perror("socket");
	return(FAIL);
    }

    /* And then connect */
    if (connect(s, (struct sockaddr *) &sin, sizeof(sin)) < 0)
    {
	perror("connect");
	return(FAIL);
    }

    return(s);
}
#endif /* BSD4_2 */

/*
 * nntpput -- send a line of text to the server, terminating it
 * with CR and LF, as per ARPA standard.
 *
 *	Parameters:	"string" is the string to be sent to the
 *			server.
 *
 *	Returns:	Nothing.
 *
 *	Side effects:	Talks to the server.
 *
 *	Note:		This routine flushes the buffer each time
 *			it is called.  For large transmissions
 *			(i.e., posting news) don't use it.  Instead,
 *			do the (void) fprintf's yourself, and then a final
 *			fflush.
 */

void nntpput(string)
char *string;
{
#ifdef DEBUG
    (void) fprintf(stderr, ">>> %s\n", string);
#endif
    (void) strcpy(bfr, string); (void) strcat(string, "\r\n");
    (void) write(server.writesrv, string, strlen(string));
}


/*
 * nntpget -- get a line of text from the server.  Strips
 * CR's and LF's.
 *
 *	Parameters:	"string" has the buffer space for the
 *			line received.
 *			"size" is the size of the buffer.
 *
 *	Returns:	-1 on error, 0 otherwise.
 *
 *	Side effects:	Talks to server, changes contents of "string".
 */

int nntpget(string, size)
char	*string;
int	size;
{
    register char *cp;
    char	*index();

    if (read(server.readsrv, string, size) == FAIL)
	return(FAIL);

    if ((cp = index(string, '\r')) != NULL)
	*cp = '\0';
    else if ((cp = index(string, '\n')) != NULL)
	*cp = '\0';
#ifdef DEBUG
    (void) fprintf(stderr, "<<< %s\n", string);
#endif

    return(SUCCEED);
}


/*
 * nntpclose -- close the connection to the server, after sending
 *		the "quit" command.
 *
 *	Parameters:	None.
 *
 *	Returns:	Nothing.
 *
 *	Side effects:	Closes the connection with the server.
 *			You can't use "nntpput" or "nntpget"
 *			after this routine is called.
 */

void nntpclose()
{
    char	ser_line[BUFLEN];

    nntpput("QUIT");
    (void) nntpget(ser_line, sizeof(ser_line));

    (void) close(server.writesrv);
    (void) close(server.readsrv);
}

void nntpsync()
/* discard the remainder of the current nntp response */
{
    char nntpbuf[BUFLEN];

    while (nntpget(nntpbuf, sizeof(nntpbuf)) >= 0)
	if (nntpbuf[0] == '.' && strlen(nntpbuf) == 1)
	    break;
}

int nntpcommand(cmdbuf, bufsiz, save)
/* send an nntp command, accept a response, save the nth line */
char	*cmdbuf;
int	bufsiz;
int	save;
{
    int n = save;

    nntpput(cmdbuf);
    (void) nntpget(cmdbuf, bufsiz);
    if (*cmdbuf != CHAR_OK)
    {
	errno = atoi(cmdbuf);
	return(FAIL);
    }
    while (n--)
	(void) nntpget(cmdbuf, bufsiz);
    if (save > 0)
	nntpsync();
    return(SUCCEED);
}

int nntpsnarf(command, tempfile)
/* send down a command, snarf the returned file data */
char *command, *tempfile;
{
    FILE *fp;
    char nntpbuf[BUFLEN];

    nntpput(command);
    (void) nntpget(nntpbuf, sizeof(nntpbuf));
    if (*nntpbuf != CHAR_OK)
	return(FAIL);
    (void) strcpy(tempfile, "/tmp/nsartXXXXXX");
    (void) mktemp(tempfile);
    if ((fp = fopen(tempfile, "w")) == (FILE *)NULL)
    {
	nntpsync();
	return(FAIL);
    }
    while (nntpget(nntpbuf, sizeof(nntpbuf)) >= 0)
    {
	if (nntpbuf[0] == '.' && strlen(nntpbuf) == 1)
	    break;	/* get it and write it to */
	(void) fputs(nntpbuf, fp);	/* the temp file */
	(void) putc('\n', fp);
    }
    (void) fclose(fp);
    return(SUCCEED);
}

int nntpspew(cmd, file)
/* attempt to send a file to the server */
char	*cmd, *file;
{
    FILE *fp;

    /* send the command down */
    nntpput(cmd);

    /* does the response tell us to keep going? */
    (void) nntpget(bfr, sizeof(bfr));
    if (*bfr != CHAR_CONT)
    {
	errno = atoi(bfr);
	return(FAIL);
    }

    /* if so, stuff the file contents down it */
    if ((fp = fopen()) == (FILE *)NULL)
	return(FAIL);
    bfr[0] = '.';
    while (fgets(bfr + 1, sizeof(bfr), fp) != NULL)
    {
	(void) nstrip(bfr);
	(void) write(server.writesrv,
		bfr + (bfr[0] != '.'), strlen(bfr) - (bfr[0] != '.'));
    }

    /* terminate in the approved way */
    (void) write(server.writesrv, ".\r\n", 3);

    /* accept server's ACK or NAK */
    (void) nntpget(bfr, sizeof(bfr));
    errno = atoi(bfr);
    return(FAIL);
}

/* nntpclient.c ends here */
