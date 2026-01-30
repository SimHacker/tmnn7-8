/****************************************************************************

NAME
   mung.c -- code to mung article headers while preserving links

SYNOPSIS
   #include "news.h"
   #include "libpriv.h"
   #include "header.h"

   int mungread(file, hp)	-- grab an article file preparatory to munging
   char *file; hdr_t *hp;

   int mungwrite(doit)		-- rewrite the munged article.
   bool doit;

DESCRIPTION
   This code allows a caller to mung the header of an article in the
news tree, preserving that article's text and links. To set this up, call
mungread(), giving a file name and a pointer to header storage. When it
returns successfully, the header of the article will have been read to *hp
and the text safely squirrelled away in a tempfile. After you've munged
the header, call mungwrite() to write the new version. An argument of
FALSE to mungwrite() cleans up the tempfile without doing the change.

BUGS
   There is no protection against concurrent munging. This might happen if
(for example) two rnews instances running concurrently both try to patch in
a Back-Reference entry at once and their timing happens to be just exactly
wrong. Someday this should be fixed using region-locking.

   Also, a reader looking at an article just as it gets munged might see some
mildly odd behavior. Most likely the reader's seek pointer would appear to have
jumped by the amount of text added or subtracted by the mung operation.

FILES
   /tmp/mung??????	-- temp file for munging article headers

AUTHOR
   Eric S. Raymond
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

**************************************************************************/
/*LINTLIBRARY*/
#include "news.h"
#include "libpriv.h"
#include "header.h"

private char tmpname[BUFLEN];	/* name of unmodified copy of article text */
private char whereis[BUFLEN];	/* where we found it */
private hdr_t *shp;		/* header to rewrite from */

int mungread(file, hp)
char *file; hdr_t *hp;
{
    FILE	*tfp, *ofp;
    char	buf[BUFSIZ];

    /* open the article for read */
    (void) strcpy(whereis, file);
    if ((ofp = fopen(whereis, "r")) == (FILE *)NULL)
	return(FAIL);

    hfree(hp);
    if (hread(hp, 0L, ofp) == 0L)
    {
	(void) fclose(ofp);
	return(FAIL);
    }

    /* open a temp file to copy text to */
    shp = hp;
    (void) strcpy(tmpname, "/tmp/mungXXXXXX");
    (void) mktemp(tmpname);
    if ((tfp = fopen(tmpname, "w+")) == (FILE *)NULL)
    {
	(void) fclose(ofp);
	return(FAIL);
    }

    /* all files are properly open, copy the text */
    while (fgets(buf, BUFSIZ, ofp) != (char *)NULL)
	(void) fputs(buf, tfp);

    (void) fclose(tfp);
    (void) fclose(ofp);

    return(SUCCEED);
}

int mungwrite(doit)
/* flush a header/file pair read in for munging, or just release it */
bool doit;	/* if FALSE, just clean up; if TRUE, do the munging */
{
    FILE	*tfp, *ofp;
    char	buf[BUFSIZ];

    if (doit)
    {
	if ((tfp = fopen(tmpname, "r")) == (FILE *)NULL)
	{
	    (void) unlink(tmpname);
	    return(FAIL);
	}
	else if ((ofp = fopen(whereis, "w")) == (FILE *)NULL)
	{
	    (void) fclose(tfp);
	    return(FAIL);
	}

	/* all files are properly open, write header and recopy the text */
	(void) hwrite(shp, ofp, TRUE);
	while (fgets(buf, BUFSIZ, tfp) != (char *)NULL)
	    (void) fputs(buf, ofp);

	(void) fclose(tfp);
	(void) fclose(ofp);
    }
    (void) unlink(tmpname);
    return(SUCCEED);
}

/* mung.c ends here */
