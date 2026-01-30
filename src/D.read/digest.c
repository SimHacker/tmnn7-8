/*
 * digest - process ARPANET digests
 *
 * digest(ifile, ofile, header)
 * FILE *ifile, *ofile;
 * hdr_t *header;
 * void (*pout)();
 *
 * returns:	TRUE	EOF reached, exit from digest.
 *		FALSE	normal exit, continue reading digest.
 *
 * For a description of supported commands, see the body of the '?' case.
 *
 * All other functions in this file are helpers and have been declared private.
 *
 * This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
 * of protecting free redistribution; see the LICENSE file for details.
 */
/*LINTLIBRARY*/

#include "news.h"
#include "header.h"
#ifdef RECMDS
#include "regexp.h"
#endif /* RECMDS */
#include "rfuncs.h"

struct art
{
    long	a_hdr;	/* seek offset of article header */
    long	a_bod;	/* seek offset of article body */
    int		a_hlen;	/* header length in lines */
    int		a_blen;	/* body length in lines */
};

#define	loop		for(;;)
#define	getnum(p, n)	for (n=0; *p>='0' && *p<='9'; p++) n = n*10 + *p-'0'
#define	errchk(p)	if (*p) goto badopt

#define PAGELEN		24
#define	MAXART		128

private struct art	*arts;
private int		lastart;
private char		*pager;

private int isheader(s)
register char *s;
{
    if (isupper (*s) || islower (*s))
    {
	while (*s && *s != ':' && !isspace (*s))
	    s++;
	if (*s == ':' && *++s == ' ')
	    return TRUE;
    }
    return FALSE;
}

private int dscan(ifp)
register FILE *ifp;
{
    char    scanbuf[BUFLEN];
    register int    n, len;
    register char  *s;
    register long   pos;
    bool was_blank, ishead;
    
    n = len = 0;
    was_blank = FALSE;
    s = scanbuf;
    arts[0].a_bod = arts[1].a_hdr = ftell(ifp);
    arts[0].a_hdr = 0;
    arts[1].a_bod = -1L;

    loop
    {
	if (sigcaught)
	    return(TRUE);
	pos = ftell(ifp);
	if (fgets(s, BUFLEN, ifp) == (char *)NULL)
	    *s = '\0';
	if (was_blank && isheader(s))
	{
	    long    lastpos;
	    short   is_blank;
	    short   nhlines;
	    arts[n++].a_blen = len;
	    len = 0;
	    nhlines = 0;
	    arts[n].a_hdr = pos;
	    is_blank = FALSE;
	    ishead = TRUE;
	    do
	    {
		lastpos = pos;
		was_blank = is_blank;
		nhlines++;
		pos = ftell(ifp);
		if (fgets(s, BUFLEN, ifp) == (char *)NULL)
		    *s = '\0';
		else
		    len++;
		is_blank = (*s == '\n') ? TRUE : FALSE;
		if (is_blank && nhlines == 1)
		    /* one liner--not a header */
		    break;
		if (!ishead || (s[0] != ' ' && s[0] != '\t'))
		    ishead = isheader(s);
	    } while ((is_blank && !was_blank) || ishead);
	    if ((!is_blank && !was_blank) || nhlines < 2)
	    {
		/* oops! not a header... back off */
		arts[n].a_hdr = arts[n - 1].a_bod;
		len += arts[--n].a_blen;
	    }
	    else
	    {
		if (was_blank)
		    pos = lastpos;
		arts[n].a_hlen = len;
		arts[n].a_bod = arts[n + 1].a_hdr = pos;
		arts[n + 1].a_bod = -1L;
		arts[n + 1].a_hlen = 3;/* average header len */
		len = 0;
	    }
	}
	if (*s == '\0')
	    break;
	was_blank = (*s == '\n') ? TRUE : FALSE;
	len++;
    }
    arts[n].a_blen = len;
    arts[n + 1].a_hdr = pos;
    lastart = n;
    return FALSE;
}

private int dhprint(art, ifp, ofp)
register int art;
register FILE *ifp, *ofp;
{
    register char   c;
    register long   pos = arts[art].a_hdr;
    register long   epos = arts[art].a_bod;
    register int    nlines = 1;

    (void) putc('\n', ofp);
    (void) fseek(ifp, (off_t)pos, SEEK_SET);
    while (pos++ < epos && !sigcaught)
    {
	if ((c = getc(ifp)) == '\n')
	    nlines++;
	(void) putc(c, ofp);
    }
    (void) fflush(ofp);
    sigcaught = FALSE;
    return nlines;
}

private int dprint(art, ifp, ofp)
int art;
FILE *ifp, *ofp;
{
    static forward int adprint();
#ifdef PAGE
    register int    cnt;
    FILE *pfp;

    if (art && arts[art].a_blen > (PAGELEN-1) - arts[art + 1].a_hlen && *pager)
    {
	if ((pfp = popen(pager, "w")) == (FILE *)NULL)
	    (void) adprint(art, ifp, ofp);
	else
	{
	    cnt = adprint(art, ifp, pfp) % (PAGELEN - 1);
	    if (cnt > (PAGELEN - 1) - arts[art + 1].a_hlen)
		while (cnt++ < PAGELEN)
		    (void) putc('\n', pfp);
	    (void) pclose(pfp);
	}
    }
    else
#endif /* PAGE */
	(void) adprint(art, ifp, ofp);
}

private int adprint(art, ifp, ofp)
int art;
register FILE *ifp, *ofp;
{
    register char   c;
    register long   pos = arts[art].a_bod;
    register long   epos = arts[art + 1].a_hdr;
    register int    nlines = 0;

    (void) fseek(ifp, (off_t)pos, SEEK_SET);
    while (pos++ < epos && !sigcaught)
    {
	if ((c = getc(ifp)) == '\n')
	    nlines++;
	(void) putc(c, ofp);
    }
    (void) fflush(ofp);
    sigcaught = FALSE;
    return nlines;
}

private int dsaveart(art, ifp, ofp, name)
int art;
register FILE *ifp, *ofp;
register char *name;
{
    register    FILE *nfp;
    char    fname[BUFLEN];
    register char  *nb;
    forward static int dhprint(), adprint();
    
    while (*name == ' ' || *name == '\t')
	name++;

    if (*name == '|')
    {
	(void) fprintf(ofp, "don't know how to pipe yet.\n");
	(void) fflush(ofp);
	return;
    }
    else if (*name == '/')
	(void) strcpy(fname, name);
    else
    {
	if (nb = getenv("NEWSBOX"))
	    (void) strcpy(fname, nb);
	else
	    (void) strcpy(fname, userhome);
	(void) strcat(fname, "/");
	(void) strcat(fname, name);
    }

    (void) fprintf(ofp, "Save digest article %d in \"%s\"", art, fname);
    (void) fflush(ofp);
    if ((nfp = fopen(fname, "a")) != (FILE *)NULL)
    {
	int     ln;

	/* Unisoft 5.1 won't seek to EOF on 'a' */
	(void) fseek(nfp, (off_t)0, SEEK_END);

	ln = dhprint(art, ifp, nfp);
	ln += adprint(art, ifp, nfp);
	(void) fprintf(ofp, " [Appended] %d lines\n", ln);
	(void) fclose(nfp);
    }
    else
	(void) fprintf(ofp, " cannot append to.\n");
}

int digest(ifp, ofp, h)
FILE *ifp, *ofp;
hdr_t *h;
{
    register int    n, curart;
    struct art  artbuf[MAXART];
    bool printh, eod, nomore;
    char    cbuf[BUFLEN], *cmd;
    extern  void hprint();
    
    arts = artbuf;
    printh = TRUE;
    nomore = eod = FALSE;
    curart = 1;

    if (dscan(ifp))
	return FALSE;

#ifdef PAGE
    if ((pager = getenv("PAGER")) == (char *)NULL)
	pager = PAGE;
#endif /* PAGE */

    dprint(0, ifp, ofp);

    loop
    {
	if (nomore)
	    break;
	if (curart < 1)
	{
	    curart = 1;
	    eod = nomore = FALSE;
	}
	if (curart > lastart)
	    curart = lastart;
	if (eod)
	    nomore = TRUE;
	if (printh && !nomore)
	    (void) dhprint(curart, ifp, ofp);
getcmd: 
	loop
	{
	    sigcaught = FALSE;
	    (void) fprintf(ofp,
		    "Digest article %d of %d ", curart, lastart);
	    if (curart == lastart && nomore)
		(void) fprintf(ofp, "Last digest article ");
	    (void) fprintf(ofp, "(%d lines) More? [%s] ",
		    arts[curart].a_blen, nomore ? "snq" : "ynq");
	    (void) fflush(ofp);
	    cmd = cbuf;
	    if (fgets(cmd, BUFLEN, stdin))
		break;
	    if (!sigcaught)
		return(TRUE);
	    (void) putc('\n', ofp);
	}
	(void) nstrip(cmd);
	while (*cmd == ' ' || *cmd == '\t')
	    cmd++;
	printh = TRUE;

	switch (*cmd++)
	{
	    case '#': 
		(void) fprintf(ofp, "%d articles in digest\n", lastart);
		(void) fflush(ofp);
		printh = FALSE;
		break;

	    case '$': 
		curart = lastart;
		break;

	    case '!': 
		(void) shell_command(FALSE, cmd);
		(void) fprintf(ofp, "!\n");
		printh = FALSE;
		break;

	    case '\0': 
		if (nomore)
		{
		    (void) putc('\n', ofp);
		    return(FALSE);
		}
		cmd--;
	    case 'y': 
	    case 'p': 
		errchk(cmd);
		dprint(curart++, ifp, ofp);
		if (curart > lastart)
		    eod = TRUE;
		break;

	    case 'n': 
		errchk(cmd);
		if (++curart > lastart)
		{
		    (void) putc('\n', ofp);
		    return(FALSE);
		}
		break;

	    case '+': 
		getnum(cmd, n);
		errchk(cmd);
		if (nomore)
		{
		    (void) putc('\n', ofp);
		    return(FALSE);
		}
		if (n)
		    curart += n;
		else
		{
		    curart += 1;
		    if (curart > lastart)
			eod = TRUE;
		}
		break;

	    case '-': 
		getnum(cmd, n);
		errchk(cmd);
		eod = nomore = FALSE;
		curart -= (n) ? n : 1;
		break;

	    case '0': 
	    case '1': 
	    case '2': 
	    case '3': 
	    case '4': 
	    case '5': 
	    case '6': 
	    case '7': 
	    case '8': 
	    case '9': 
		cmd--;
		getnum(cmd, n);
		errchk(cmd);
		curart = n;
		eod = nomore = FALSE;
		break;

	    case 'q': 
	    case 'x': 
		(void) putc('\n', ofp);
		return(FALSE);

	    case '?': 
		(void) fprintf(ofp, "\nDigester options:\n\n");
		(void) fprintf(ofp, "y\tyes, print article.\n");
		(void) fprintf(ofp, "n\tno, go to next article.\n");
		(void) fprintf(ofp, "q\texit from digester.\n");
		(void) fprintf(ofp, "h\tprint article header.\n");
		(void) fprintf(ofp, "s file\tsave article in file.\n");
		(void) fprintf(ofp, "w file\tsave article text in file.\n");
		(void) fprintf(ofp, "t\ttable of contents.\n");
		(void) fprintf(ofp, "T\ttable of contents with header.\n");
		(void) fprintf(ofp, "+[n]\tforward n articles (1).\n");
		(void) fprintf(ofp, "-[n]\tback n articles (1).\n");
		(void) fprintf(ofp, "\nh and s may be followed by '-'\n");
		(void) fflush(ofp);
		break;

	    case 'h': 
		n = curart;
		if (*cmd == '-')
		{
		    cmd++;
		    if (n > 1)
			n--;
		}
		errchk(cmd);
		(void) dhprint(n, ifp, ofp);
		nomore = printh = FALSE;
		if (n != curart)
		    (void) putc('\n', ofp);
		break;

	    case 's': 
	    case 'w': 
		n = curart;
		if (*cmd == '-')
		{
		    cmd++;
		    if (n > 1)
			n--;
		}
		while (*cmd == ' ' || *cmd == '\t')
		    cmd++;
		dsaveart(n, ifp, ofp, cmd);
		nomore = printh = FALSE;
		if (n != curart)
		    (void) putc('\n', ofp);
		break;

	    case 'H': 
		errchk(cmd);
		hwrite(h, ofp, TRUE);
		eod = nomore = FALSE;
		break;

	    case 'T': 
	    case 't': 
		errchk(cmd);
		if (cmd[-1] == 'T')
		    hprint(h, 0, 0);
		dprint(0, ifp, ofp);
		eod = nomore = FALSE;
		break;

	    default: 
	badopt: 
		if (!nomore)
		    (void) fprintf(ofp, "y (yes), n (no), ");
		(void) fprintf(ofp,
			"q (quit), s file (save), h (header), t (table of contents)\n");
		(void) fprintf(ofp, "? for help\n");
		goto getcmd;
	}
    }
    (void) putc('\n', ofp);
    return(FALSE);
}

/* digest.c ends here */
