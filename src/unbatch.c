/*****************************************************************************

NAME
   unbatch.c -- functions for cracking batches

SYNOPSIS
   int batchmode(mode)			-- set the batch mode to use
   char *mode;

   int batchproc(file, post, outfd)	-- filter a message file
   char *file; void (*post)() int outfd;

DESCRIPTION
   These functions handle batch filtering and cracking for rnews and friends.
   The batchmode() function sets decode, decompress and unbatch flags according
to the protocol name handed it.
   The batchproc() function takes a file (or stdin if the file name is NULL)
and processes it as a netnews batch, prefiltering it to undo compress/decoding
etc. For each article in the batch, the code calls the given post function with
the header global loaded with the article information. If outfd is nonzero, the
article's generated ID is written to outfd as each article is posted.

   The format accepted is the standard one, i.e.

	# {rnews|unbatch|cunbatch|c7unbatch}
	#! rnews xxxx
	<article containing xxxx characters>
	#! rnews yyy
	<article containing yyy characters>
	    :
	    :
	#! rnews zzzz
	<article containing zzzz characters>
	<EOF>

The unbatch code recovers properly if the very first line is missing; in that
case it will act as if that line had been "# unbatch\n". Note that the code's
knowledge about the batch delimiter line format is contained in the macros
CMDLINE(), BATCHCMD(), BATCHLINE() and BATCHSIZE().

This code also recognizes and converts both bnproc-style batches and notes
postings; these capabilities are enabled by definition of the macro symbols
BNCVT and ZAPNOTES.

FILES
   LIB/compress			-- program used to decompress mail
   TEXT/.tmp/newsbatch??????	-- temp file used to store incoming article(s)

AUTHOR
   Eric S. Raymond
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

*****************************************************************************/
/*LINTLIBRARY*/
#include "news.h"
#include "libpriv.h"
#include "header.h"

/*
 * Here is everything the program knows about batch line format.
 * Note that it makes no assumptions about what (if anything) follows the
 * batch size before the terminating newline.
 */
#define BATCHCHAR	'#'
#define CMDLINE(buf)	(strncmp(buf, "#! ", 3) == 0)
#define BATCHCMD(buf)	(buf + 3)
#define BATCHLINE(buf)	(strncmp(buf, "#! rnews ", 9) == 0)
#define BATCHSIZE(buf)	atol(buf + 9)

/* verbosity level minima for various messages */
#define V_SHOWSRC	1	/* indicate each source processed */
#define V_SHOWPHASE	2	/* show what processing phase we're in */
#define V_SHOWMSG	3	/* dump start/offsets for each article */
#define V_SHOWDECOMP	4	/* show decompression actions */
#define V_SHOWSIZE	5	/* report sizes expected */
#define V_SHOWHDR	6	/* report header reads */

/* unbatching mode controls */
private int	bflag, cflag;		/* news batch decode option values */
#ifdef DECODE
private bool	edecode = FALSE;	/* true if receiving 7 bits */
#endif /* DECODE */
private int	artcount = 0;		/* number of articles processed */

int batchmode(ptr)
/* set modes from a decompression name */
char	*ptr;
{
    char    *tp;

    if (tp = strrchr(ptr, '/'))
	ptr = tp + 1;

    if (strncmp(ptr, "unbatch", 7) == 0)
	bflag++;
    else if (strncmp(ptr, "cunbatch", 8) == 0)
	bflag++, cflag = TRUE;
    else if (strncmp(ptr, "uncompress", 10) == 0)
	cflag = TRUE;
    else if (strncmp(ptr, "c7unbatch", 9) == 0)
#ifdef DECODE
	bflag++, cflag = edecode = TRUE;
#else
	xerror0("Please recompile with DECODE defined to enable c7unbatch!");
    	/*NOTREACHED*/
#endif /* DECODE */
    else if (strncmp(ptr, "un7compress", 11) == 0)
#ifdef DECODE
	cflag = edecode = TRUE;
#else
	xerror0("Please recompile with DECODE defined to enable un7compress!");
    	/*NOTREACHED*/
#endif /* DECODE */
    else
	return(FAIL);
    return(SUCCEED);
}

private jmp_buf croaked;

private catch_t deadchild(signo)
/* catch broken pipes to decompress */
int	signo;
{
    longjmp(croaked, signo);
}

static FILE *batchfilter(file)
/* filter a message file from rname or msgin, put clear text in outfile */
char    *file;    /* input file, if the stuff is already on disk */
{
    int		first;
    char	srcfile[BUFLEN];	/* location of current source */
    char	pushback[BUFLEN];	/* 1 line of pushback to cleartext */
    char	inbuf[BUFLEN];		/* article file input buffer */
    char	doptbuf[BUFLEN];	/* hold name of a decompressor */
    FILE	*msgin;

    srcfile[0] = pushback[0] = '\0';	/* initially input isn't from a file */

    /* make sure input is on msgin, wherever it actually came from */
    if (file == (char *)NULL || file[0] == '\0')
	msgin = stdin;
    else if ((msgin = fopen(file, "r")) == (FILE *)NULL)
    {
	logerr2("couldn't open batch file %s, errno = %d", file, errno);
	return((FILE *)NULL);
    }

    /*
     * next step: undo any compression or encoding on the input and
     * determine whether or not it's a batch
     */
    bflag = cflag = FALSE;
#ifdef DECODE
    edecode = FALSE;
#endif /* DECODE */

    /* interpret leading line(s) beginning with # */
    if ((first = ungetc(fgetc(msgin), msgin)) == BATCHCHAR)
    {
	(void) fgets(inbuf, LBUFLEN, msgin);
	inbuf[strlen(inbuf) - 1] = '\0';
	if (BATCHLINE(inbuf))
	{
	    if (!bflag)
		(void) batchmode("unbatch");
	    if (file && file[0])
		(void) fseek(msgin, -((off_t)strlen(inbuf) + 1), SEEK_CUR);
	else
	    {
		(void) strlcpy(pushback, inbuf, sizeof(pushback));
		(void) strlcat(pushback, "\n", sizeof(pushback));
	    }
	}
	else if (batchmode(BATCHCMD(inbuf)) == SUCCEED)
	    log1("unbatching in %s mode", BATCHCMD(inbuf));
	else
	{
	    logerr1("invalid batch header line %s", inbuf);
	    return((FILE *)NULL);
	}
    }

    /*
     * if the input had batch header lines, the first line has now been eaten,
     * and the code knows whether or not it should decode and decompress;
     * otherwise, we've pushed back the first character and are sitting at
     * the file or stream start
     */

#ifdef DECODE
    /*
     * if we were called as c7unbatch, decode the incoming article to disk
     * and substitute a file pointer to the decoded form for msgin; arrange
     * for the file to autodestruct when we close msgin.
     */
    if (edecode)
    {
	FILE    *ofp;
	forward void    decode();

	(void) strlcpy(srcfile, "/tmp/decodeXXXXXX", sizeof(srcfile));
	(void) mktemp(srcfile);
	if ((ofp = fopen(srcfile, "w")) == (FILE *)NULL)
	    xerror1("couldn't write decoded text %s", srcfile);
	decode(msgin, ofp);
	(void) fclose(ofp);
	(void) freopen(srcfile, "r", msgin);
	(void) unlink(srcfile);
    }
#endif /* DECODE */

    /* check that we can actually read further in the file */
    if ((first = ungetc(fgetc(msgin), msgin)) == EOF)
    {
	logerr1("input read failed, errno = %d", errno);
	return((FILE *)NULL);
    }

    /* should we filter the (remainder of the) file through uncompress? */
    if (cflag || first == CMPMAGIC[0])
    {
	catch_t (*oldpbreak)();
	FILE	*ofp;
	int	status;

	/* assemble the decompression command we want */
	(void) snprintf(doptbuf, sizeof(doptbuf), "%s/%s", site.libdir, DECOMPRESS);
	if (first != CMPMAGIC[0])
	    (void) strlcat(doptbuf, " -C", sizeof(doptbuf));
	(void) strlcpy(srcfile, "/tmp/dcmpXXXXXX", sizeof(srcfile));
	(void) mktemp(srcfile);
	(void) strlcat(doptbuf, " >", sizeof(doptbuf));
	(void) strlcat(doptbuf, srcfile, sizeof(doptbuf));

	/* arrange to decompress the remaining input to the target file */
	oldpbreak = signal(SIGPIPE, SIGCAST(deadchild));
	if ((status = setjmp(croaked)) == 0)
	{
	    if ((ofp = peopen(doptbuf, "w")) == (FILE *)NULL)
	    {
		logerr3("decompression command '%s' failed on %s, errno %d",
			file ? file : "stdin", doptbuf, errno);
		return((FILE *)NULL);
	    }
#ifdef DEBUG
	    else if (verbose >= V_SHOWDECOMP)
		log1("decompressing with command '%s'", doptbuf);
#endif /* DEBUG */

	    /* now do it */
	    while (!feof(msgin) && !ferror(msgin))
		(void) putc(fgetc(msgin), ofp);

	    if ((status = peclose(ofp)) != SUCCEED)
		logerr2("decompress of %s returned wait status 0x%x",
			file ? file : "stdin", status);
	}
	else	/* longjmp got called, status holds the signal number */
	{
	    logerr3("\"%s\" gave signal %d while on batch %s",
		    doptbuf, status, file ? file : "stdin");
	    (void) signal(SIGPIPE, SIGCAST(oldpbreak));
	    (void) unlink(srcfile);
	    return((FILE *)NULL);
	}
	(void) signal(SIGPIPE, SIGCAST(oldpbreak));

	if (freopen(srcfile, "r", msgin) != (FILE *)NULL)
	    (void) unlink(srcfile);	/* arrange for it to vanish on close */
	else
	{
	    logerr1("couldn't reopen decompressed cleartext %s", srcfile);
	    (void) unlink(srcfile);	/* arrange for it to vanish on close */
	    return((FILE *)NULL);
	}
    }

#ifdef BNCVT
    /*
     * Turn bnproc-style batches into a format we know about. This code is
     * derived from Carl Gutenkunst's filter utility, I (esr) know nothing
     * about bncvt and only added it out of a possibly misguided urge to
     * support every format anyone seems to be using.
     */
    if ((first = ungetc(fgetc(msgin), msgin)) != BATCHCHAR && !isalpha(first))
    {
	int nbytes = 0, expected = 0;
	char	*cp;
	FILE	*ofp;

	log0("converting bnproc batch");
	(void) strlcpy(srcfile, "/tmp/bncvtXXXXXX", sizeof(srcfile));
	(void) mktemp(srcfile);
	if ((ofp = fopen(srcfile, "w")) == (FILE *)NULL)
	    xerror1("couldn't write converted text to %s", srcfile);

	while (fgets(bfr, LBUFLEN, msgin) != (char *)NULL)
	{
	    /*
	     * Check for an article eater. This is a DEL character, either
	     * 0x7F or 0xFF, in the first column preceeding a new article
	     * byte count. It usually throws off the byte count, so we have
	     * to add some padding to keep rnews from losing sync (and
	     * discarding the next article).
	     */
	    if ((bfr[0] & 0x7F) == 0x7F)
		if (expected > 0)
		    while (nbytes++ < expected)
			(void) putc('\0', ofp);

	    /*
	     * If we aren't expecting text, then we're expecting an article
	     * byte count. This is a left-justified integer, immediately
	     * followed by a newline. We ignore leading article-eater DEL
	     * characters.
	     */
	    if (nbytes >= expected)
	    {
		 nbytes = expected = 0;
		 cp = bfr;
		 while ((*cp & 0x7F) == 0x7F)
		     ++cp;
		 while (*cp >= '0' && *cp <= '9')
		     expected = expected * 10 + (*cp++ - '0');

		if (*cp == '\n' && expected > 0)
		     (void) printf("#! rnews %d\n", expected);
		else
		{  
		     (void) fprintf(stderr, "Sync->%s", bfr);
		     expected = 0;
		}
	    }
	    else  /* Another normal line of text: write it out */
	    {
		(void) fputs(bfr, ofp);
		nbytes += strlen(bfr);
	    }
	}
	(void) fclose(ofp);
	(void) freopen(srcfile, "r", msgin);
	(void) unlink(srcfile);
    }
#endif /* BNCVT */

    /* force a copy of the clear text to disk in a file we can discard */
    if (file == (char *)NULL && srcfile[0] == '\0')
    {
	(void) snprintf(srcfile, sizeof(srcfile), "%s/.tmp/newsbatchXXXXXX", site.textdir);
	collect(pushback[0] ? pushback : (char *)NULL, srcfile);
	(void) freopen(srcfile, "r", msgin);
	(void) unlink(srcfile);
    }

    return(msgin);
}

static int batchcrack(msgin)
/*
 * Accept an article from msgin, load public header structure with its info.
 * The article may be preceded by a batch header line. Note that msgin must
 * be seekable, i.e. not a tty.
 */
FILE	*msgin;	    /* the input source */
{
    int		lcount;		/* count of lines in current part */
    static long	expect = 0L;	/* size of next batch section */
    bool	foundhdr;	/* message ended by a batch header? */
    char	inbuf[BUFLEN];	/* article file input buffer */

    inbuf[0] = '\0';
    if (bflag)
    {
	/* grab lines until we find a start of batch section **/
	while (fgets(inbuf, sizeof(inbuf), msgin) && !BATCHLINE(inbuf))
	    continue;

	/* if out of input entirely, error out of the unbatching loop */
	if (feof(msgin))
	    return(FAIL);
	else if ((expect = BATCHSIZE(inbuf)) <= 0)
	    logerr1("bad size in batch delimiter line \"%s\"\n", inbuf);
#ifdef DEBUG
	else if (verbose >= V_SHOWSIZE)
	    log1("%ld characters expected", expect);
#endif /* DEBUG */
    }
    else if (artcount++ > 0)
    {
	artcount = 0;
	return(FAIL);
    }

    /* note our current seek position and read in the header */
    hfree(&header);
    if (hread(&header, expect, msgin) == 0)
    {
	if (feof(msgin))
	    logerr0("article source is zero-length or missing");
	else
	    logerr1("header at offset %ld garbled", (long)header.h_startoff);
	return(FALSE);
    }
#ifdef DEBUG
    else if (verbose >= V_SHOWHDR)
	(void) printf("just read header of %s\n", header.h_ident);
#endif /* DEBUG */

    /* now skip the text part */
    lcount = 0;
    foundhdr = FALSE;
    do {
#ifdef ZAPNOTES
	/* discard leading notesfile IDs (if any), mung header accordingly */
	if (inbuf[0] == '#' && inbuf[2] == ':'
		&& hlblank(header.h_nfid) && hlblank(header.h_nffrom))
	{
	    char *cp;

	    (void) nstrip(inbuf); hlcpy(header.h_nfid, inbuf);
	    (void) fgets(inbuf, BUFLEN, msgin);
	    (void) nstrip(header.h_nffrom); hlcpy(header.h_nffrom, inbuf);
	    (void) fgets(inbuf, BUFLEN, msgin);

	    /* Strip trailing " - (nf)" from subject */
	    if ((cp = strrchr(header.h_subject,'-'))&&!strcmp(--cp," - (nf)"))
		*cp = '\0';
	    (void) fprintf(stderr,
			   "Stripped notes headers on %s\n",header.h_ident);
	}
#endif /* ZAPNOTES */

	if (fgets(inbuf, sizeof(inbuf), msgin))
	    lcount++;
	else
	    break;		/* length checks will catch any problems */
    } while
	(!bflag || !(foundhdr = BATCHLINE(inbuf)));
    header.h_endoff = ftell(msgin);
    if (foundhdr)
    {
	header.h_endoff -= strlen(inbuf);
	--lcount;
    }

    /* complain if we got batched input that was too short */
    if (bflag && header.h_endoff - header.h_startoff < expect)
    {
	logerr3("%s was truncated: got (%ld - %ld)",
	    header.h_ident, (long)header.h_endoff, (long)header.h_startoff);
	return(FALSE);
    }

    /*
     * Check line count if there's already one attached to
     * the article.  We make this a fatal error -
     * throwing it away if it got chopped, in hopes that
     * another copy will come in later with a correct
     * line count.
     */
    if (header.h_intnumlines)
    {
	if (lcount != header.h_intnumlines)
	{
	    if (lcount == 0)
	    {
		logerr2("%s rejected; expected %d lines, got 0",
		    header.h_ident, header.h_intnumlines);
		return(FAIL);
	    }

	    if (lcount > header.h_intnumlines || lcount < header.h_intnumlines)
		log3("%s: expected %d lines, got %d",
		    header.h_ident, header.h_intnumlines, lcount);
	}
    }

    return(TRUE);
}

int batchproc(inf, post, outfd)
/* process and insert messages from a single batch file */
char	*inf;
void	(*post)();
int	outfd;
{
    FILE *batch;
    int bstat, bcount = 0;

#ifdef DEBUG
    if (verbose >= V_SHOWPHASE)
	log1("about to prefilter %s", inf);
#endif /* DEBUG */

    if ((batch = batchfilter(inf)) == (FILE *)NULL)
	return(FAIL);

#ifdef DEBUG
    if (verbose >= V_SHOWSRC)
	log1("processing filtered batch from %s", inf ? inf : "stdin");
#endif /* DEBUG */

    /* now run through the cleartext version posting messages */
    artcount = 0;
    while ((bstat = batchcrack(batch)) != FAIL)
	if (bstat == FALSE)	/* header was garbled, but keep going */
	    continue;
 	else			/* header OK, post article */
	{
#ifdef DEBUG
	    if (verbose >= V_SHOWMSG)
		log4("%s starts %ld, size %ld, date %s",
		     header.h_ident[0] ? header.h_ident : "<originating>",
		     (long)header.h_startoff,
		     (long)(header.h_endoff - header.h_startoff),
		     header.h_postdate);
#endif /* DEBUG */

	    /*
	     * batchcrack() loads the current-header global that post
	     * looks at, so this works OK
	     */
	    (*post)();
	    bcount++;

	    /* report the article's generated ID, if requested */
	    if (outfd)
		(void) write(outfd,
			     header.h_ident,
			     (iolen_t) (strlen(header.h_ident) + 1));
	}

    (void) fclose(batch);
    return(bcount);
}

/* unbatch.c ends here */
