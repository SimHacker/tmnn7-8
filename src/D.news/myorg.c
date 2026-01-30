/*****************************************************************************

NAME
   myorg.c -- get the user's organizational affilation

SYNOPSIS
   #include "news.h"

   char *organization()	-- retrieve the user's organization

DESCRIPTION
   This function properly interprets the ORGANIZATION environment variable,
falling back on the MYORG define from confignews.h. If it contains a filename,
the organization is fetched out of the first line of the file (but if the file
has > 1 lines, a line is picked at random). A leading ~ is expanded to the
calling user's home directory.

BUGS
   This should be called organization.c, but bad things happen when lint
makes an *.ln with that name on an AT&T UNIX with the 14-character file
segment name limit -- if organization.l is sitting around make can get
very confused and try to lex it!

NOTE
   If I really wanted to encourage signature bloat, I'd make randline()
public and use it as part of a hook for inserting random witty sayings into
my signature. But I wouldn't do anything *that* chaotic, would I? Naah...

AUTHOR
   Eric S. Raymond
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

*****************************************************************************/
/*LINTLIBRARY*/
#include "news.h"

#define NOORG	"Frobozz"

private void randline(lfile, buf, bufsiz)
/* put a random line from lfile into buf */
char	*lfile;
char	*buf;
int	bufsiz;
{
    FILE	*mfp;

    /* grab a line out of the file */
    if (mfp = fopen(lfile, "r"))
    {
	(void) fgets(buf, bufsiz, mfp);

	/* if there's more than one line, pick one at random */
	if (!feof(mfp))
	{
	    int lines = 1;

	    /* count the number of alternatives */
	    while (fgets(buf, bufsiz, mfp) != NULL)
		lines++;

	    /* seed the random-number generator and pick one */
	    srand((seed_t) (time((time_t *) 0) + getpid()));
	    lines = rand() % lines;

	    /* rewind to file start and fetch the indicated line */
	    rewind(mfp);
	    while (lines-- >= 0)
		(void) fgets(buf, bufsiz, mfp);
	}

	(void) fclose(mfp);	/* we're done, clean up */
    }
}
     
char *organization()
/* retrieve the user's organization */
{
    static char	*myorg = (char *)NULL;
    char	*ptr;

    if (myorg != (char *)NULL)
	return(myorg);

    /*
     * Note that the organization can be turned off by
     * setting it to the null string, either in MYORG or
     * $ORGANIZATION in the environment.
     */
    if ((myorg = getenv("ORGANIZATION")) == (char *)NULL)
    {
	myorg = newsattr("organization", ORGANIZATION);
	if (!strncmp(myorg, NOORG, 7))
	    return((char *)NULL);
    }

    /* if the first character is a slash or tilde, pick out of a file */
    if (myorg[0] && strchr("/~", myorg[0]))
    {
	/* expand leading tilde if present */
	if (myorg[0] == '~')
	{
	    (void) strcpy(bfr, userhome);
	    (void) strcat(bfr, myorg + 1);
	    myorg = bfr;
	}

	/* pick a random line out of the file */
	randline(myorg, bfr, sizeof(bfr));

	if (ptr = strchr(bfr, '\n'))
	    *ptr = 0;
	myorg = savestr(bfr);
    }
    return(myorg);
}

/* myorg.c ends here */

