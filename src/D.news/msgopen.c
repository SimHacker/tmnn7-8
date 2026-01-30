/****************************************************************************

NAME
   msgopen.c -- handle RFC-822 messages, possibly compressed

SYNOPSIS
   #include "news.h"
   #include "header.h"

   FILE *msgopen(mfile)		-- open an RFC-822 message
   char *mfile;

   void msgclose(fp)		-- close a message handle (macro)
   FILE *fp;

DESCRIPTION
   These functions give access to I/O streams derived from messages in the
article tree. They hide the difference between articles stored in normal and
compressed form. Which kind a message is is determined by the presence or
absence of the compression magic number CMPMAGIC on the front of the message.

   Msgclose() is now a macro expanding to fclose(), but may become a function
in the future if more attributes are added.

   In the future, this function should interpret the RFC1049 Content-Type
field.

FILES
   /tmp/tmpart??????	-- temporary file for article clear text

AUTHOR
   Eric S. Raymond
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

****************************************************************************/
/* LINTLIBRARY */
#include "news.h"
#include "header.h"

FILE *msgopen(mfile)
/* open currently selected message */
char		*mfile;	/* the article tree location to use */
{
    FILE    *fp;
    char   *magic = CMPMAGIC;	/* Gould UTX doesn't like CMPMAGIC[0,1] */
    char	t1,t2;

    if ((fp = fopen(mfile, "r")) == (FILE *)NULL)
	return((FILE *)NULL);

    t1 = getc(fp); 
    t2 = getc(fp);

    if ((t1 != magic[0]) || (t2 != magic[1]))
	(void) fseek(fp, (off_t)0, SEEK_SET);
    else
    {
	char    *tempfile = "/tmp/tmpartXXXXXX";
	FILE	*nfp;
	int	c;

	(void) fclose(fp);
	(void) sprintf(bfr, "%s/%s <%s", site.libdir, DECOMPRESS, mfile);
	nfp = popen(bfr, "r");

	/* can't return a popen() pointer because we'll need the file size */
	(void) mktemp(tempfile);
	if ((fp = fopen(tempfile, "w")) == (FILE *)NULL)
	    return((FILE *)NULL);
	while ((c = getc(nfp)) != EOF)
	    (void) putc(c, fp);
	(void) fclose(fp);
	(void) pclose(nfp);

	fp = fopen(tempfile, "r");
	(void) unlink(tempfile);	    /* vanishes when fp is closed */
    }

    return(fp);
}

/* msgopen.c ends here */
