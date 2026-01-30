/*****************************************************************************

NAME
   transact.c -- comm primitives for reader/newsfilter transactions

SYNOPSIS
   #include "transact.h"

   void fconnect(fd1, fd2)		-- connect to partner
   int fd1, fd2;

   int ftransmit(type, code, arg1,arg2,arg3,arg4,arg5)	-- transmit a request
   char type, code; int seqno; char *arg1, *arg2, *arg3, *arg4, *arg5;

   int freceive(response)		-- accept a response
   fresp_t *response;

DESCRIPTION
   This module provides lowest-level primitives for communication from
a newsreader to a newsfilter child process.

AUTHOR
   Eric S. Raymond, from a protocol spec developed with Brad Templeton.
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

*****************************************************************************/
/* LINTLIBRARY */
#include "news.h"
#include "transact.h"

/* length of fixed-size article header (counting terminating NUL) */
#define PREFIXLEN	14

static int	ifd, ofd;	/* comm file descriptors */

void fconnect(fd1, fd2)
int	fd1, fd2;
{
    ifd = fd1;
    ofd = fd2;
}

int ftransmit(type, code, seqno, arg1, arg2, arg3, arg4, arg5)
char	type;
char	code;
int	seqno;
char	*arg1, *arg2, *arg3, *arg4, *arg5;
{
    unsigned int	totlen;
    int	len1 = (arg1 == NOARG) ? 0 : strlen(arg1) + 1;
    int len2 = (arg2 == NOARG) ? 0 : strlen(arg2) + 1;
    int len3 = (arg3 == NOARG) ? 0 : strlen(arg3) + 1;
    int len4 = (arg4 == NOARG) ? 0 : strlen(arg4) + 1;
    int len5 = (arg5 == NOARG) ? 0 : strlen(arg5) + 1;
    totlen = len1 + len2 + len3 + len4 + len5;

    (void) sprintf(bfr, "%c%c %06d %03d\000", type, code, seqno, totlen);
#ifdef PDEBUG
    (void) fprintf(stderr, "%s: about to transmit \"%s\", (%d bytes)\n",
		   Progname, bfr, strlen(bfr));
#endif /* PDEBUG */
    if (write(ofd, bfr, PREFIXLEN) == FAIL)
	return(FAIL);

    if (arg1 != NOARG)
    {
	(void) strcpy(bfr, arg1);
	if (arg2 != NOARG)
	{
	    (void) strcpy(bfr + len1, arg2);
	    if (arg3 != NOARG)
	    {
		(void) strcpy(bfr + len1 + len2, arg3);
		if (arg4 != NOARG)
		{
		    (void) strcpy(bfr + len1 + len2 + len3, arg4);
		    if (arg4 != NOARG)
			(void) strcpy(bfr + len1 + len2 + len3 + len4, arg5);
		}
	    }
	}

#ifdef PDEBUG
	(void) fprintf(stderr, "%s: about to transmit \"%s\", (%d bytes)\n",
		   Progname, bfr, totlen);
#endif /*PDEBUG */
	if (write(ofd, bfr, totlen) == FAIL)
	    return(FAIL);
    }

    return(SUCCEED);
}

int freceive(rp)
fresp_t	*rp;
{
    int	seq = FAIL;
    unsigned int len = 0;

    rp->buf[0] = '\0';
    errno = 0;
    if (read(ifd, rp->buf, PREFIXLEN) != PREFIXLEN)
	return(FAIL);
#ifdef PDEBUG
    (void) fprintf(stderr, "%s: received \"%s\" (%d bytes), errno = %d\n",
		   Progname, rp->buf, strlen(rp->buf), errno);
#endif /*PDEBUG */
    (void) sscanf(rp->buf, "%c%c %d %u", &rp->type, &rp->code, &seq, &len);

    rp->arg1 = rp->arg2 = rp->arg3 = rp->arg4 = rp->arg5 = "";
    if (len != 0)
    {
	(void) bzero(rp->buf, MAXERRSIZE);
	if (read(ifd, rp->buf, len) == FAIL)
	    return(FAIL);
#ifdef PDEBUG
	(void) fprintf(stderr, "%s: received \"%s\", (%d bytes)\n",
		       Progname, bfr, len);
#endif /*PDEBUG */
	rp->arg1 = rp->buf;
	rp->arg2 = rp->arg1 + strlen(rp->arg1) + 1;
	rp->arg3 = rp->arg2 + strlen(rp->arg2) + 1;
	rp->arg4 = rp->arg3 + strlen(rp->arg3) + 1;
	rp->arg5 = rp->arg4 + strlen(rp->arg4) + 1;
    }
    
    return(seq);
}

/* transact.c ends here */
