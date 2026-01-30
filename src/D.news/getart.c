/**************************************************************************

NAME
   getart.c -- code to access article text.

SYNOPSIS
   int getart(pl, hd, fn)	    	-- get article text
   place_t *pl; hdr_t *hd; char *fn;

   int arterr(st, pl, hd, buf)	    	-- generate fetch status message
   int st; place_t *pl; hdr_t *hd; char *buf;

   int indexline(ngp, msg, buf)		-- get article index line
   group_t *ngp; nart_t msg; char *buf;

   void author(hp, buf)			-- the article sender's name
   hdr_t *hp; char *buf;

DESCRIPTION
   All fetching of article text is done through the function getart.c, which
is expected to take three arguments; the address of a place, the address of
a header block, and the address of a filename buffer. The function is expected
to load the header block with header information for the given article,
including the h_fp, h_startoff, h_textoff and h_endoff fields that give the
reader a real handle on the article text.

   The arterr() function formats a status message for a given status, header
and article location. It is provided in this form so that I/O managers may do
what they will with the generated message.

   The indexline() entry point returns a subject abstract of the current
article, prefixed by a character of status information. If RNESCAPES is enabled
the format of the subject abstract part is macroexpanded from the environment
variable SUBJLINE. Currently the only flag prefix character supported is '!'
indicating an article that references some posting by the invoking user.

   The author() function tries to extract a human name for an article author
out of the header's Reply-To and From lines.

BUGS
   The author() code compensates for broken gateways by replacing '.' chars
followed by letters with ' ', and deletes all " characters. This is a kluge.

SEE ALSO
   getfiles.c		-- access to news file hierarchy
   nntp.c		-- nntp server support (mimics history facilities)

AUTHOR
   Eric S. Raymond
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

**************************************************************************/
/*LINTLIBRARY*/
#include "news.h"
#include "vio.h"
#include "header.h"
#include "active.h"
#include "newsrc.h"
#include "history.h"

int getart(pl, hd, txt)
/* get us access to message text for given article */
place_t	*pl;
hdr_t	*hd;
char	*txt;
{
    int			lc;
    FILE		*nextfp;
#ifdef CRACKMAIL
    static group_t	*oldgroup = (group_t *)0;
#endif /* CHECKMAIL */

    /* generate the actual name of the message file to be looked at */
#ifdef CRACKMAIL
    if (pl->m_group->rc_flags & RC_MAILBOX)
    {
	if (pl->m_group == oldgroup)
	    lc = FALSE;
	else
	{
	    /* run crackmail() here if there's no newsgroup index in core */

	    (void) strcpy(txt, pl->m_group->ng_box->mb_file);
	    lc = TRUE;
	}
    }
    else
#endif /* CRACKMAIL */
	if ((lc = artname(pl, txt)) == FAIL || !exists(txt))
	    lc = FAIL;
	else
	    lc = TRUE;

    /* O.K., get a handle on the file */
    if (lc == FAIL)
	return(A_MISSING);
    else if (lc == TRUE)	/* filename has changed */
    {
	if ((nextfp = msgopen(txt)) != (FILE *)NULL)
	{
#ifdef CRACKMAIL
	    oldgroup = pl->m_group;
#endif /* CHECKMAIL */
	    if (hd->h_fp)
		(void) msgclose(hd->h_fp);
	}
	else
	{
	    nextfp = hd->h_fp;
	    return(A_NOOPEN);
	}
    }

#ifdef CRACKMAIL
    /* go to the article start */
    (void) fseek(nextfp, pl->m_group->rc_box->mb_start[pl->m_number],SEEK_SET);
#endif /* CRACKMAIL */

    /* whether or not we opened a new file, try to read an article header */
    hfree(hd);
    if (hread(hd, 0L, nextfp) == 0)
    {
	(void) msgclose(hd->h_fp);
	return(A_GARBLED);
    }

    /* ensure that the header's line count is OK */
    lc = linecount(hd->h_fp, hd->h_endoff);
    if (hd->h_intnumlines && (lc != hd->h_intnumlines))
	return(A_LINECOUNT);
    hd->h_intnumlines = lc;
    return(A_FETCHOK);
}

int arterr(status, pl, hd, buf)
/* format a default error message corresponding to a given article */
int	status;	/* error status */
place_t	*pl;
hdr_t	*hd;
char	*buf;
{
    switch(status)
    {
    case A_MISSING:
	(void) sprintf(buf, "Article %ld of group %s is missing.",
			pl->m_number, pl->m_group->ng_name);
	break;

   case A_NOOPEN:
	(void) sprintf(buf, "Can't open %s/%d, errno = %d.",
			pl->m_number, pl->m_group->ng_name,
			errno);
	break;

    case A_GARBLED:
	(void) sprintf(buf, "Article %ld of group %s is garbled",
			pl->m_number, pl->m_group->ng_name);
	break;

    case A_LINECOUNT:
	(void) sprintf(bfr,
			"Article %ld of group %s has a bad line count (%d)",
			pl->m_number, pl->m_group->ng_name,
			hd->h_intnumlines);
	break;
    }

    return(status);	/* so we can pass it right through */
}

void author(hp, buf)
/* return a best approximation of the article author's name */
hdr_t	*hp;
char	*buf;
{
    char	*cp, *sp;

    /* use Reply-To if we find one, otherwise the From line */
    if (hlnblank(hp->h_replyto)
		&& (cp = strchr(hp->h_replyto, '(')) != (char *)NULL
		&& cp[1] != ')')
	sp = hp->h_replyto;
    else
	sp = hp->h_from;

    /* if there's no non-empty comment on the line, use the whole thing */
    if ((cp = strchr(sp, '(')) == (char *)NULL || cp[1] == ')')
    {
	(void) strcpy(buf, sp);
	if ((cp = strchr(buf, '\n')) != (char *)NULL)
	    *cp = '\0';
    }
    else	/* found a comment start */
    {
	(void) strcpy(buf, cp + 1);
	if ((cp = strchr(buf, ')')) != (char *)NULL)
	    *cp = '\0';

	/* try to correct for braindamage by various gateways */
	for (cp = buf; *cp; cp++)
	    if (cp[0] == '"')
		continue;
	    else if (cp[0] == '.' && isalpha(cp[1]))
		*cp = ' ';
    }
}

int indexline(ngrp, num, buf)
/* generate a subject line for the given location */
group_t	*ngrp;
nart_t	num;
char	*buf;
{
    static hdr_t	subjhdr;
    place_t		loc;
    int			status;

    loc.m_group = ngrp;
    loc.m_number = num;
    if ((status = getart(&loc, &subjhdr, buf)) < 0)
    {
	buf[0] = I_NOART;
	(void) arterr(status, &loc, &subjhdr, buf + 1);
	return(FAIL);
    }
    else
    {
#ifdef RNESCAPES
	static char	*fmt = (char *)NULL;
#endif /* RNESCAPES */
	static char	*match = (char *)NULL;

	/* 1-char prefix tells if article is likely to be of interest */
	if (match == (char *)NULL)
	    Sprint2(match, "%s@%s", username, site.truename);
	buf[0] = strindex(subjhdr.h_references,match) > -1 ? I_LOCAL : I_BLANK;

#ifdef RNESCAPES
	if (fmt == (char *)NULL)
	    fmt = getenv("SUBJLINE");

	if (fmt != (char *)NULL)
	{
	    seeheader(&subjhdr);
	    strexpand(fmt, buf + 1);
	    seeheader(&header);
	}
	else
#endif /* RNESCAPES */
	{
	    char	frombfr[BUFLEN];

	    /* we do the equivalent of %a %e %-19,19[Subject] here */
	    author(&subjhdr, frombfr);
	    (void) sprintf(buf + 1, "%ld %-19.19s %s",
			   (long)num, frombfr, subjhdr.h_subject);
	}
    }

    return(SUCCEED);
}

/* getart.c ends here */
