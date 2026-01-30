/****************************************************************************

NAME
   rdhistory.c -- functions for read access to the history file

SYNOPSIS
   #include "news.h"
   #include "history.h"

   void hstread(flg)		-- load history info
   bool flg;

   void hstrewind(flg)		-- reset for sequential access
   bool flg;

   int hstnext(wlock)		-- go to next article
   bool wlock;

   char *hstfile(id)		-- return location of copy of given article
   char *id;

   int hstseek(id, wlock)	-- select the history of a given id
   char	*id; bool wlock;

   int hstloc(mloc)		-- get next associated location
   place_t *mloc;

   void hstclose()		-- release history file

   int hstenter(line)		-- decode line, enter corresponding record
   char *line;

DESCRIPTION
   These functions provide the read side of a clean interface to the article
history files used by the USENET software. They require the ngfind() function
from active.c. For the write side, see wrhistory.c. The hstenter() entry point
is for internal use (by wrhistory.c) only.

   Normally the history file contains information on all messages received,
posted, cancelled and expired during the last HISTEXP seconds (normally 4
weeks). This information will be kept in the database defined by history.dir,
history.dat, and history.pag. The dbhistory.dat file will remain readable.

   Use of these functions must begin with a call to hstread(). The flag
controls whether previous history data is read in; you can leave it FALSE if
you only need to append to the history file.

   Data from a given history entry may be queried or set following a call to
the hstseek() function.

   The hstnext() function may be used to step through history entries in
sequence (a hstrewind() must have been done previously for this to work).
If file segment locking is available, the wlock argument is TRUE, and the
caller has appropriate permissions, it automatically locks each record that
it accesses.

   The hstfile() function takes a Message-ID and returns a file name which
contains one of its copies.

THE MACRO INTERFACE
   Some macros are defined in history.h that define pseudo-functional handles
on history information; they are all 'safe' (i.e. can be called with arguments
that have side-effects) and pretty much self-explanatory.

hstid()		-- return the ID of the current article
hstdate()	-- return the receipt timestamp of the current article
hstcancel()	-- cancel the current article

FILE FORMATS
   History files consist of zero or more lines each having two required
fields and a third optional one, all tab-separated. The first field is
an angle-bracket-enclosed Internet-style Message-ID; the second is a date
stamp (time of receipt) in getdate() format, and the third (if present)
is either a space-separated list of article names (each in the form
<newsgroup>/<number>), or the word 'cancelled'.

NOTES
   If TMNCONVERT or DEBUG is on this module will accept the old history
file format as well as the new one. If TMNCONVERT is on, the old format
will be generated. This, unfortunately, is less useful than it sounds like;
the edbm database format may not be compatible with older dbm files. Note that
articles with old-format history lines will have any explicit expire date
ignored.

   The global variable wrhistdb controls where history writes will occur.
This normally the same as rdhistdb, so that everything will work normally.
However, this allows expire to redirect history writes to the nhistory file.

SEE ALSO
   wrhistory.c	-- write side of the history access code
   artlist.c	-- functions for manipulating article reference lists.
   nntp.c	-- "fake" history functions for nntp support

FILES
   ADM/history.dat  -- history database data file
   ADM/history.pag  -- history database page file
   ADM/history.dir  -- history database key directory file

AUTHOR
   Eric S. Raymond
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

**************************************************************************/
/*LINTLIBRARY*/
#include "news.h"
#include "active.h"
#include "newsrc.h"
#include "history.h"

#ifndef NONLOCAL	/* let a network service library take over, maybe */

#define V_ALLOC		2	/* verbose level enables allocation messages */

/* other modules need to see these */
char	*HISTORY;		/* the name of the history file */
char	chstname[BUFLEN];	/* ID of the current article */
time_t	chstdate;		/* receipt date of the article */
time_t	chstexpd;		/* expire date of the article */
int	chstatus;		/* status of the last hstcrack() */
lptr_t	hstlst;			/* location descriptor for the xref list */

/* wrhistory.c needs to see these */
int	histrdok = 0;		/* >0 if history records read in */

/* scratch data areas for file I/O etc. */
database    *rdhistdb;	/* database of pointers to history lines */
database    *wrhistdb;	/* database to use when writing stuff out  */
char	    *chline;	/* allocated copy of current line */
private char	line[LBUFLEN];	/* scratch space for everybody */

static int hstcrack(text)
/* crack a history record into its component fields */
char	*text;
{
    register char   *tab1, *tab2, *tab3;

    /* delimit the ID field */
    if (text[0] != '<' || ((tab1 = strchr(text, TAB)) == (char *)NULL))
	return(chstatus = GARBLED);
    else
    {
	*tab1 = '\0';
	(void) strcpy(chstname, text);
	*tab1++ = TAB;
    }

#if defined(TMNCONVERT) || defined(DEBUG)
    if (strchr(tab1, ':'))
    {
	/* parse the receipt date field */
	if ((tab3 = strchr(tab1, TAB)) == (char *)NULL)
	    return(chstatus = GARBLED);
	else
	{
	     *tab3 = '\0';
#ifndef lint
	    chstdate = getdate(tab1, (struct timeb *)NULL);
#else
	    chstdate = 0L;	/* avoid 'struct timeb undefined' from lint */
#endif	/* lint */
	    *tab3++ = TAB;
	}
    }
    else
#endif /* defined(TMNCONVERT) || defined(DEBUG) */
    {
	/* parse the receipt date field */
	if ((tab2 = strchr(tab1, SPACE)) == (char *)NULL)
	    return(chstatus = GARBLED);
	else
	{
	    *tab2 = '\0';
	    chstdate = atol(tab1);
	    *tab2++ = SPACE;
	    /* chstdate = (time_t)0L; (redundant) */
	}

	/* parse the expire date field */
	if ((tab3 = strchr(tab2, TAB)) == (char *)NULL
	    	&& (tab3 = strchr(tab2, '~')) == (char *)NULL)
	    return(chstatus = GARBLED);
	else
	{
	    *tab3 = '\0';
	    chstexpd = atol(tab2);
	    *tab3++ = TAB;
	}
    }

    while (isspace(*tab3))   /* ignore leading whitespace on location list */
	tab3++;

    /* interpret the article list field */
    if (*tab3 == '\0')
	return(chstatus = EXPIRED);
    else if (strncmp(tab3, CANCEL_TOKEN, CANCEL_SIZE) == 0)
	return(chstatus = CANCELLED);
    else
    {
	artlstset(&hstlst, tab3);
	return(chstatus = (*tab3 == '<') ? REFERENCE : VALID);
    }
}

int hstnext(wlock)
/* go to the next article */
bool	wlock;
{
    if (dbmnext(rdhistdb, wlock) == FAIL)
	return(FAIL);
    else
    {
	unsigned int clen;
	int status;

	if (chline != (char *)NULL)
	    (void) free(chline);
	chline = dbmget(&clen, rdhistdb);
	chline[clen] = '\0';
	chline = savestr(chline);
	if ((status = hstcrack(chline)) == GARBLED)
	    return(GARBLED);
	else
	    return(status);
    }
}

int hstenter(hlin)
/* enter a new history record */
char	*hlin;
{
    char    namebuf[BUFLEN];
    char    *cp = hlin;
    char    *tp = namebuf;
#ifdef HDEBUG
    FILE    *hfp = xfopen(HISTORY, "a");

    /* Unisoft 5.1 doesn't seek to EOF on 'a' */
    (void) fseek(hfp, (off_t)0, SEEK_END);

    (void) fputs(hlin, hfp);
    (void) fclose(hfp);
#endif /* DEBUG */

    while (*cp && !isspace(*cp))
	*tp++ = *cp++;
    *tp = '\0';
    lcase(namebuf);
    (void) dbmput(namebuf, (unsigned) strlen(namebuf),
		hlin, (unsigned) strlen(hlin), wrhistdb);
    if (chline != hlin) {
	    if (chline != (char *)NULL)
		    (void) free(chline);
	    chline = savestr(hlin);
    }
}

void hstread(readstuff)
/* this function may read the default hist file */
bool	readstuff;
{
    if (!histrdok)
	Sprint1(HISTORY, "%s/history", site.admdir);

    if (readstuff && !histrdok)
    {
	if ((rdhistdb = dbmopen(HISTORY)) == (database *)NULL)
	    xerror2("Can't open %s database, errno is %d", HISTORY, errno);
	else
	    histrdok++;
	wrhistdb = rdhistdb;
    }
}

int hstseek(name, wlock)
/* find history data on a article id */
char *name;
bool wlock;
{
    register int    i;

    /* strip out the ID to use as a search key */
    for (i = 0; name[i] && !isspace(name[i]); i++)
	line[i] = name[i];
    line[i++] = '\0';
    lcase(line);
    /* find the named record */
    if (dbmseek(line, (unsigned) strlen(line), rdhistdb, wlock) == FAIL)
	return(FAIL);
    else
    {
	unsigned int clen;

	if (chline != (char *)NULL)
	    (void) free(chline);
	chline = dbmget(&clen, rdhistdb);
	if (chline == NULL)
	    return(FAIL);
	chline[clen] = '\0';
	chline = savestr(chline);
	return(hstcrack(chline));
    }
}

char *hstfile(artid)
/* return a file name associated with the given ID */
char *artid;
{
    static char	hstfilen[BUFLEN];
    char	*cloc = (char *)NULL;
    place_t	myloc;

    if (hstseek(artid, FALSE) != SUCCEED)
	return((char *)NULL);

    while (hstloc(&myloc) == SUCCEED)
    {
	if (artname(&myloc, hstfilen) == FAIL || access(hstfilen,F_OK) == FAIL)
	    continue;

	/* we'd prefer an uncompressed version */
	if (myloc.m_group->ng_flags & NG_COMPRESSED)
	{
	    cloc = savestr(hstfilen);
	    continue;
	}

	return(hstfilen);
    }

    if (cloc)	/* we only found a compressed version */
    {
	(void) strcpy(hstfilen, cloc);
	(void) free(cloc);
	return(hstfilen);
    }
    else
	return((char *)NULL);
}

int hstloc(exloc)
/* grab the next argument from the current location list */
place_t    *exloc;
{
    if (chstatus != SUCCEED)
	return(chstatus);

    return(artlstloc(&hstlst, exloc));
}

void hstclose()
/* close the history file -- really a dummy for the non-NONLOCAL case */
{
    /* do nothing */
}

#endif /* NONLOCAL */

/* rdhistory.c ends here */
