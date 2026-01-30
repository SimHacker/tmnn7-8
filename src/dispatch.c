/***************************************************************************

NAME
   dispatch.c -- dispatch messages for transmission

SYNOPSIS
   bool dispatch(target, sp, hp, fname, maynotify)
   char *target; feed_t *sp; hdr_t *hp; char *fname; bool maynotify;

DESCRIPTION
   The dispatch() code is what rnews calls to do article sends. It sends to the
system list in the first argument, interpreting the fields of the link
description pointed to by its second argument to tell how to transmit the
article described by its third and fourth args. The maynotify flag enables the
N option.
   The dispatch() code interprets the F, L and N options. The A, B, C, D, E,
S, U, and X options are interpreted by transmit(). The B option is used by both
layers (transmit() needs it to generate the command to be used for remote
execution).

AUTHOR
   Eric S. Raymond
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

*****************************************************************************/
/*LINTLIBRARY*/
#include "news.h"
#include "libpriv.h"
#include "header.h"
#include "post.h"
#include "dballoc.h"
#include "feeds.h"
#include "history.h"

#ifdef OPENDEBUG
#undef open		/* avoid trouble with 3-arg open() call below */
#undef close
#endif /* OPENDEBUG */

bool dispatch(tp, sp, hp, fname, maynotify)
/* transmit to systems */
char		*tp;	/* target list to transmit to */
register feed_t	*sp;	/* data on the link to the target system */
hdr_t		*hp;	/* header of article */
char		*fname;	/* name of file to transmit */
bool		maynotify;  /* if FALSE, disables N option */
{
#ifndef u370
    static hdr_t hh;
#endif /* !u370 */
    bool    ust, copied;
    char    *transformed;

/* B:	transmit via sendbatch */
    bool batchit = (s_option(sp, 'B') != (char *)NULL);
/* F:	append name to file */
    bool fappend = (s_option(sp, 'F') != (char *)NULL);
/* L:	local: don't send the article unless it was generated locally */
    char *local = s_option(sp, 'L');
/* N:	notify: don't send the article, just tell him we have it */
    int notify = maynotify && (s_option(sp, 'N') != (char *)NULL);

    /* if local is enabled, don't retransmit messages from far away */
    if (local && hlnblank(hp->h_path))
    {
	int	maxhops = atoi(local);

	if (maxhops == 0)
	    maxhops = 1;
	for (local = hp->h_path; *local != '\0'; local++)
	    if (*local == PATHSEP)
		maxhops--;
	if (maxhops < 0)
	{
#ifdef DEBUG
	    if (verbose)
		(void) printf("%s won't be relayed to %s (nonlocal)\n",
				hp->h_ident, sp->s_name);
#endif /* DEBUG */
	    return(FALSE);
	}
    }

    /* if the link is a batched one, just write article's ID to batch file */
    if (batchit || fappend)
    {
	int	ofd;
	char	batchfile[BUFLEN];

      if (fappend && sp->s_xmit[0])
	  (void) strcpy(batchfile, sp->s_xmit);
      else
	  /* append to BATCH/<system> */
	  (void) sprintf(batchfile, "%s/%s", site.batchdir, sp->s_name);

#ifdef DEBUG
	if (!debug)
#endif /* DEBUG */
	    if ((ofd=open(batchfile, (O_CREAT|O_WRONLY|O_APPEND),0666))==FAIL)
		xerror2("can't open %s, errno = %d", sp->s_xmit, errno);

	/* construct the batch file line and append it to the batch */
	(void) strcpy(bfr, hp->h_ident);
#ifdef DEBUG
	if (debug)
	{
	    (void) printf("would append \"%s\" to %s\n", bfr, batchfile);
	    return(TRUE);
	}
	else
#endif /* DEBUG */
	{
	    (void) strcat(bfr, "\n");
	    if (write(ofd, bfr, (iolen_t)strlen(bfr)) == FAIL)
		logerr3("append of %s to %s failed, errno = %d",
				bfr, batchfile, errno);
	    (void) close(ofd);
	    return(TRUE);
	}
    }

    /* if the N option is active, just send a notification */
    if (notify)
    {
	char	titbuf[SBUFLEN], ngbuf[SBUFLEN];

	(void) sprintf(titbuf, "ihave %s %s", hp->h_ident, site.nodename);
	(void) sprintf(ngbuf, "to.%s.ctl", sp->s_name);
	return(xmitctrl(tp, sp, titbuf, ngbuf, (char *)NULL) != FAIL);
    }

    copied = FALSE;

#ifdef ARTFILTER
    /* do A translation if enabled */
    if (transformed = artfilter(sp, fname))
    {
	fname = transformed;
	copied = TRUE;
    }
#endif /* ARTFILTER */

    /* filter for compression and encoding options */
    if (transformed = filefilter(sp, fname))
    {
#ifdef ARTFILTER
	if (copied)
	    (void) unlink(fname);
#endif /* ARTFILTER */
	fname = transformed;
	copied = TRUE;
    }

    ust = transmit(tp, sp, hh.h_ident, fname, copied, FALSE);

    if (copied)
	(void) unlink(fname);

    return(ust);
}

/* dispatch.c ends here */
