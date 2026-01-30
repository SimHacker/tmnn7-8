/*****************************************************************************

NAME
   newpost.c -- program interface for originating news

SYNOPSIS
   #include "libpost.h"

   char *newpost(file, options)		-- call rnews to post an article
   char *file, char *options;

DESCRIPTION
   The newpost() function accepts the name of the entire composed message and
hands it off to rnews with the appropriate arguments. If NONLOCAL is on, these
gymnastics are actually done by a network interface library, see the code
in D.network.

   If DEBUG is on, the debug variable modifies this behavior if nonzero.
At 1 it dumps the posting command and the processed article to stdout rather
than passing it to rnews. At 2 it appends a -D debug option to the rnews
invocation. Note that neither of these switches work of NONLOCAL is on.

BUGS
   The return value of newpost() (the ID of the new article) lives in a common
static buffer and could get stomped if you don't copy it before using other
library functions.

AUTHORS
   Eric S. Raymond
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

*****************************************************************************/
/*LINTLIBRARY*/
#include "news.h"
#include "libpost.h"

/* if you change these, don't forget the header comment */
#define D_SHOWCOMMAND	1	/* show rnews command that would have been */
#define D_PASSDEBUG	2	/* pass debug option to rnews */

#if !defined(NONLOCAL) && !defined(SHARED)
/*ARGSUSED1*/
#endif /* !defined(NONLOCAL) && !defined(SHARED) */
char *newpost(file, options)
/* call rnews to post an article */
char	*file;		/* article to transmit */
char	*options;	/* list of systems not to transmit to */
{
#if defined(NONLOCAL) || defined(SHARED)
    char	*id;

    id = idmake(file);
    if (net_post(file) == SUCCEED)	/* see your network service library */
	return(id);
    else
	return((char *)NULL);

#else	/* normal case, post to local hierarchy */

    char	cmdbuf[BUFLEN];

#ifndef SPOOLPOST
    /* override SPOOLNEWS if we've configured for it */
    (void) sprintf(cmdbuf, "%s/rnews -u -p %s", site.libdir, file);
#else
    /* process postings originated here just like incoming news */
    (void) sprintf(cmdbuf, "%s/rnews -p %s", site.libdir, file);
#endif /* SPOOLPOST */

    /* user may have passed down some debugging or -x options */
    if (options && options[0])
    {
	(void) strcat(cmdbuf, " ");
	(void) strcat(cmdbuf, options);
    }

#ifdef DEBUG
    if (debug == D_PASSDEBUG)
	(void) strcat(cmdbuf, " -D");
    else if (debug == D_SHOWCOMMAND)
    {
	(void) printf("Posting command would be: %s\n", cmdbuf);
	return((char *)NULL);
    }
#endif /* DEBUG */

    /* go execute the command, return the generated ID */
    return(ospawn(cmdbuf));
#endif /* defined(NONLOCAL) || defined(SHARED) */
}


/* newpost.c ends here */
