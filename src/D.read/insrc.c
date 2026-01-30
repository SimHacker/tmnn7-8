/****************************************************************************

NAME
   insrc.c -- routines for handling article temporary file

SYNOPSIS
    insrc_t *src	-- current input source

    void tfopen(addr, sfp)	-- address to switch to
    insrc_t *addr; FILE	*sfp;

    void tfclose(addr)		-- deinitialize a source block (DEBUG only)
    insrc_t *addr;

    int tfgrab(sz)	-- get the next chunk of input source
    int sz;

    int tfcount(sz)	-- tell how many lines to display for current page
    int sz;

    int tfskip()	-- return start line of next digest section

    void tfget(lin, i, len)	-- get a line from the current temp file
    char *lin; int i, len;

    void tfput(lin, i)	-- put a line to the temp file
    char *lin; int i;

    int tfappend(lin)	-- append a line to the tempfile, folding long ones
    char *lin;

DESCRIPTION
   These routines handle buffering and line folding of a file
source for presentation through a screen-oriented interface. A vprint()
function suitable for use by the generic code is supplied. Depending
on the values of the input block's i_hdrend and i_endsuba fields, the source
may be treated as an article, an unstructured file, or as an ARPANET digest.

BUGS
   This code is grotty and complex and I don't entirely understand it myself.

FILES
   /tmp/insrc??????	-- temp buffer file for screen handling

SEE ALSO
   vnews.c  -- interface manager for the standard visual reader
   edvnews.c  -- ditto, using cooked output only

AUTHOR
   Original design and implementation by Kenneth Almquist

REVISED BY
   Eric S. Raymond
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

*****************************************************************************/
/*LINTLIBRARY*/
#include "news.h"
#include "insrc.h"

#ifndef private
#define private static
#endif /* private */

#define TABWIDTH	8
#define even(cols)	((cols & 1) ? cols + 1 : cols)

insrc_t *src;		/* current input source */

private char linebuf[BUFSIZ];	/* temporary workspace */
private char *maxcol;

void tfopen(addr, sfp)
insrc_t	*addr;	/* address to switch to */
FILE	*sfp;	/* file to get input from */
{
    if (addr->i_tfp == (FILE *)NULL)
    {
	char tfname[20];	/* name of temp file */
	/*
	 * Initialize the temp file associated with the given source block.
	 */
	(void) strcpy(tfname, "/tmp/insrcXXXXXX");
	(void) mktemp(tfname);
	(void) close(creat(tfname, 0666));
	if ((addr->i_tfp = fopen(tfname, "w+")) == (FILE *)NULL)
	    (void) xerror1("Can't create temp file, errno = %d", errno);

	/*
	 * Unlink it after opening so that it is guaranteed to self-destruct
	 * when the current process exits.
	 */
#ifdef DEBUG
	if (debug)
	    /* we need to keep the temp file around for debugging purposes */
	    addr->i_tfname = savestr(tfname);
	else
#endif /* DEBUG */
	    (void) unlink(tfname);
    }

    src = addr;
    src->i_fp = sfp;
    src->i_nomore = FALSE;			/* haven't seen whole text */
    src->i_lastlin = src->i_topline = src->i_botline = 0;	/* haven't seen any in fact */
#ifdef DIGPAGE
    src->i_dtype = src->i_endsuba = 0;		/*   and not a digest */
#endif /* DIGPAGE */
    src->i_lastwrf = TRUE;
}

#ifdef DEBUG
void tfclose(addr)
insrc_t	*addr;	/* address to switch to */
{
    /*
     * the disadvantage of this over unlinking the tempfile at source
     * initialization time is that it won't go away if the program
     * aborts -- we accept this for debugging mode
     */
    if (debug)
	(void) unlink(addr->i_tfname);
}
#endif /* DEBUG */

private void outline()
{
    *maxcol = '\0';
    if (strncmp(linebuf, ">From ", 6) == 0)
    {
	register char *p = linebuf;
	while (p[0] = p[1])
	    p++;
    }
    (void) tfappend(linebuf);
    if (maxcol > linebuf)
	src->i_artlines = src->i_lastlin;
    maxcol = linebuf;
}

/*
 * Append file to temp file, handling control characters, folding lines, etc.
 * We don't grow the temp file to more than lastline so that a user won't have
 * to wait for 20 seconds to read in a monster file from net.sources.
 * What we really want is coroutines--any year now. This new version
 * returns FALSE until all lines in the article have been read, then TRUE.
 */
private int appfile(iop, lastline)
register FILE	*iop;		/* file to get text from */
int		lastline;	/* snarf text up to this line */
{
    register int c;
    register char *icol;	/* &linebuf[0] <= icol <= maxcol */

    if (src->i_nomore || iop == (FILE *)NULL)
	return(TRUE);
    else if (lastline && (src->i_artlines >= lastline))
	return(TRUE);
    maxcol = linebuf;
    icol = linebuf;
    while ((c = getc(iop)) != EOF)
    {
	switch (c)
	{
	case ' ':
	    if (icol == maxcol && icol < linebuf + BUFSIZ - 1)
	    {
		*icol++ = ' ';
		maxcol = icol;
	    }
	    else
		icol++;
	    break;

	case '\t':
	    icol = (icol - linebuf &~ 07) + 8 + linebuf;
	    while (maxcol < icol && maxcol < linebuf + BUFSIZ - 1)
		*maxcol++ = ' ';
	    break;

	case '\b':
	    if (icol > linebuf) --icol;
	    break;

	case '\n':
	    outline();
	    if (lastline && src->i_artlines >= lastline)
		return(FALSE);
	    icol = linebuf;
	    break;

	case '\r':
	    icol = linebuf;
	    break;

	case '\f':
	    outline(); outline(); outline();
	    if (lastline && src->i_artlines >= lastline)
		return(FALSE);
	    icol = linebuf;
	    break;

	default:
	    if (icol >= linebuf + BUFSIZ - 1)
		icol++;
	    else if (icol == maxcol)
	    {
		*icol++ = c;
		maxcol = icol;
	    }
	    else
		*icol++ = c;
	    break;
	}
    }

    /*
     * if we got this far, we ran out of file before getting the
     * required number of lines
     */
    if (maxcol != linebuf)	/* file not terminated with newline */
	outline();
    return(src->i_nomore = TRUE);
}

bool tfgrab(csize)
/* get the next chunk of input source */
int	csize;	    /* number of lines to get */
{
    (void) appfile(src->i_fp, src->i_topline + csize + 1);
#ifdef DIGPAGE
    src->i_endsuba = findend(src->i_topline, src->i_topline + csize);
#endif /* DIGPAGE */

    if (!src->i_folded || src->i_artlines <= src->i_topline + csize
#ifdef DIGPAGE
		&& !(src->i_endsuba > 0 && src->i_endsuba < src->i_artlines)
#endif /* DIGPAGE */
		)
	return(FALSE);		    /* no text left in this source */

    /* otherwise compute last line we've seen */
#ifdef DIGPAGE
    if (src->i_endsuba > 0)
	src->i_botline = src->i_endsuba;
    else
#endif /* DIGPAGE */
	src->i_botline = src->i_topline + csize;
    return(TRUE);
}

#ifdef DIGPAGE
private bool endline(buf)
/*
 * Tell whether buf contains a section-ender line for the current format.
 * This is where the mail-format-specific hacks should go. We implement this
 * as a state machine, assuming it will be called on sequential lines, in order
 * to package all the logic in this one function.
 */
char *buf;
{
#define RFC822CHARS	"qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDFGHJKLZXCVBNM-"
#define S_NORMAL	0	/* last line was nonblank */
#define S_WANTBLANK	1	/* blank line next for true mail delimiter */
#define S_HAVEBLANK	2	/* last line was blank */
    static int state = S_NORMAL;
    int	n;

    if (buf[0] == '\0')		/* blank line, set or handle special states */
    {
	if (state == S_WANTBLANK)
 	{
	    state = S_NORMAL;
	    return(TRUE);
	}
	else
	{
	    state = S_HAVEBLANK;
	    return(FALSE);
	}
    }
    else			/* line is nonblank, check for delimiters */
    {
	state = S_NORMAL;
	switch (src->i_dtype)
	{
	case DT_NOBREAKS:	/* ordinary article */
	    return(FALSE);

	case DT_DIGEST:	/* ARPANET-style digest */
	    if ((n = strspn(buf, "-")) && ((n>23 && n<33) || (n>65 && n<79)))
		state = S_WANTBLANK;
	    return(FALSE);

	case DT_MAILBREAKS:	/* mail or news batch */
	    /* recognize as many different mail formats as we can */
	    if (!strncmp(buf,"From ",5)||!strncmp(buf,">From ",6))
		return(TRUE);					/* /bin/mail */
	    else if (!strcmp(buf, "\001\001\001\001\n"))
		return(TRUE);					/* MMDF */
	    else if (!strncmp(buf, "#! rnews ", 8))
		return(TRUE);					/* newsbatch */
#ifdef RFC822
	    else if ((n = strspn(buf, RFC822CHARS)) && buf[n] == ':'))
		return(state == S_HAVEBLANK);			/* RFC822 */
#endif /* RFC822 */
	    else
		return(FALSE);					/* none */
	}
    }
    /* NOTREACHED */
}

private int findend(from, to)
/*
 * find end of current subarticle in digest, 0 if no digest end found;
 * note that this only looks forward as far as the text that the tempfile
 * has already picked up.
 */
int	from, to;
{
    register int i;

    for (i = from; i < to && i < src->i_lastlin ; i++)
    {
	tfget(linebuf, i, (int)src->i_width);
	if (endline(linebuf))
	    return(i);
    }
    return(0);
}

int tfskip()
/* return start line of next digest section */
{
    if (src->i_endsuba)			/* if we know where part begins */ 
	return(src->i_endsuba + 1);	/* go to it */
    else
    {
	(void) appfile(src->i_fp, 0);
	return(findend(src->i_topline, src->i_lastlin));
    }
}
#endif /* DIGPAGE */

int tfcount(count)
/* tell how many lines we want to display on current page */
int	count;	/* maximum lines of input to display */
{
#ifdef DIGPAGE
    if (src->i_endsuba > 0 && count > src->i_endsuba - src->i_topline)
	count = src->i_endsuba - src->i_topline;
#endif
    if (count > src->i_lastlin - src->i_topline)
	count = src->i_lastlin - src->i_topline;

    return(count);
}

private void tfseek(linno, wrflag)
/* seek to a given line in the current temp file */
{
    if (linno != src->i_tfnext || wrflag != src->i_lastwrf)
    {
	(void) fseek(src->i_tfp,
		     (off_t)(linno * even(src->i_width)), SEEK_SET);
	src->i_tfnext = linno;
	src->i_lastwrf = wrflag;
    }
}

void tfput(tline, linno)
/*
 * Put at most src->i_width chars of a line to the temp file and bump the temp
 * file line number variable. Tabs expand into sequences of TABWIDTH spaces.
 */
register char *tline;
int linno;
{
    register FILE *rtfp;	/* try to make it a little faster */
    register int i;
    int	max = even(src->i_width);

    tfseek(linno, 1);
    rtfp = src->i_tfp;

    for (i = 1; i <= max; i++)
    {
	if (*tline == '\0')
	    (void) putc('\0', rtfp);
	else if (*tline == '\t')
	{
	    (void) putc(' ', rtfp);
	    while ((i % TABWIDTH) && (i <= max))
	    {
		(void) putc(' ', rtfp);
		i++;
	    }
	    tline++;
	}
	else
	    (void) putc(*tline++, rtfp);
    }
    src->i_tfnext++;
}

int tfappend(tline)
/* append a line to the temp file; long lines are folded */
register char *tline;
{
    register char    *nxtlin;

    do {
	nxtlin = strchr(tline, '\n');
	if (nxtlin)
	    *nxtlin++ = '\0';

	while (strlen(tline) > src->i_width)
	{
	    tfput(tline, src->i_lastlin++);
	    tline += src->i_width;
	    src->i_folded++;	/* we folded a line, increment total count */
	}
	tfput(tline, src->i_lastlin++);
    } while
	((tline = nxtlin) != (char *)NULL);

    return(src->i_lastlin);
}

void tfget(tline, linno, len)
/* get a given line (folded, with at most src->i_width chars) */
char *tline;
int linno, len;
{
    tfseek(linno, 0);
    (void) fread(tline, even((int)src->i_width), 1, src->i_tfp);
    tline[len] = '\0';
    src->i_tfnext++;
}

/* insrc.c ends here */
