/****************************************************************************

NAME
   nntppost.c -- functions for talking to the nntp server

SYNOPSIS
   #include "news.h"

   int net_post(file)		-- call rnews to post an article
   char *file;

   char *getgroups()		-- get a group descriptions list

   char *getdistribs()		-- get a current distributions list

DESCRIPTION
   The primitives to post news, implemented to go through the NNTP network
server.

NOTE
   This code assumes you are running NNTP 1.5 patchlevel 4 or a more recent
version, so that `LIST newsgroups' and `LIST distributions' are supported.

BUGS
   Probably legion -- I don't have an nntp to test this on.

AUTHOR
   Eric S. Raymond
Adapted from Phil Lapsley's NNTP inews.c and UNTESTED. No warranties
express or implied, your mileage may vary, and like that.
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

**************************************************************************/
/*LINTLIBRARY*/
#include "news.h"

/*ARGSUSED1*/
int net_post(file, options)
/* post the given article via an NNTP network connection */
char	*file;
char	*options;
{
    char	*stid, *endid;

    /*
     * We're talking to NNTP via a server
     */
    if (nntpinit(newsattr("nntphost", NNTPSERVER)) < 0)
    {
	logerr0("NNTP connect failed");
	return(FAIL);
    }

    /* try the post */
    errno = SUCCEED;
    if (nntpspew("POST", file) == FAIL)
    {
	switch(errno)
	{
	ERR_NOPOST:
	    logerr0("Sorry, you can't post from this machine.");
	    break;
	ERR_POSTFAIL:
	    logerr0("Article not accepted by server; not posted.");
	    break;
	default:
	    logerr1("Remote error: %s", bfr);
	    break;
	}
    }

    /*
     * Nntpclose sends the server a
     * "quit" command for us, which is why we don't send it.
     */
    nntpclose();

    return(errno);
}

char *getgroups()
{
    if (nntpsnarf("LIST newsgroups", bfr) == SUCCEED)
	return(savestr(bfr));
    else
	return((char *)NULL);
}

char *getdistribs()
{
    if (nntpsnarf("LIST distributions", bfr) == SUCCEED)
	return(savestr(bfr));
    else
	return((char *)NULL);
}

/* nntppost.c ends here */
