/****************************************************************************

NAME
   wrhistory.c -- functions for write access to the history file

SYNOPSIS
   #include "news.h"
   #include "history.h"

   int hstline()		-- return current history line

   int hstexpire()		-- expire the current location

   void hstdrop()		-- drop the current history record

   int hstadd(id, rdate, edate, gp, msg)   -- add a new location
   char *id; time_t rdate, edate;
   char *gp; nart_t msg;

   void hstparent(hp)		-- tell parent of hp where its followup is
   hdr_t *hp;

   int hstwrfile(fp, start)	-- dump the text form of the data file
   FILE *fp; int start;

   void hstwrite(mode)		-- write hist data back out to its file
   bool mode;

DESCRIPTION
   For full descriptions of the data structures used by wrhistory.c, see
the header comment on rdhistory.c.

   The hstadd() function permits you to add a posting location to the
history record of a given ID. It takes as arguments the ID, parent ID, receipt
timestamp, group and article number of a posting. If there is no history
record corresponding to the given ID one is created using the given ID and
receipt date (otherwise the receipt date argument is ignored). If the
group pointer given is NULL, the article is cancelled instead.

   The hstparent() function parses the References line of a given header (if
there is one) and adds its ID to the Back-References header of the last parent
listed (establishing a link that can be used for followup-chasing). If the
parent article isn't on site, hstparent() makes a reference entry in the
history file.

SEE ALSO
   rdhistory.c	-- read side of the history access code
   artlist.c	-- functions for manipulating article reference lists.
   nntp.c	-- "fake" history functions for nntp support

AUTHOR
   Eric S. Raymond
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

**************************************************************************/
/*LINTLIBRARY*/
#include "news.h"
#include "libpriv.h"
#include "active.h"
#include "newsrc.h"
#include "header.h"	/* only hstparent() needs this */
#include "history.h"

#ifdef BIGGROUPS
#define GFORM	"%s/%ld"
#else
#define GFORM	"%s/%d"
#endif /* BIGGROUPS */

#ifdef DEBUG
int hstwrfile(fp)
/* write hist data in readable format to a given destination */
FILE	*fp;	/* file pointer to write to */
{
    char		*chp;
    unsigned int	clen;

    if (access(HISTORY, F_OK) == 0)
    {
	dbmrewind(rdhistdb);
	while (dbmnext(rdhistdb, FALSE) == SUCCEED)
	{
	    chp = dbmget(&clen, rdhistdb);
	    chp[clen] = '\0';
	    (void) fputs(chp, fp);
	}
    }

    return(SUCCEED);
}

void hstwrite()
/* write out all the in-core hist file data */
{
    int		    omask = umask(0113);
    FILE	    *wrcfp;

    /* write a human-readable form of the database */
    if ((wrcfp = fopen(HISTORY, "w")) == (FILE *)NULL || hstwrfile(wrcfp))
	logerr0("can't update human-readable form of history database");

    (void) umask(omask);
}
#endif /* DEBUG */

private void hstmake(id, rdate, edate, buf)
/* generate new id and date fields for an ID */
char	*id;	/* ID to enter the info under */
time_t	rdate;	/* receipt timestamp of the article */
time_t	edate;	/* explicit expire date of the article */
char	*buf;	/* where to write new entry */
{
#ifdef TMNCONVERT
    struct tm *tm;
#endif /* TMNCONVERT */

    /* first, generate the ID and date fields if required */
    if (rdate == (time_t)0)
	rdate = time((time_t *)NULL);
#ifndef TMNCONVERT
    (void) sprintf(buf, "%s\t%ld %ld\t", id, (long)rdate, (long)edate);
#else /* TMNCONVERT */
    tm = localtime(&rdate);
#ifdef USG
    (void) sprintf(buf,"%s\t%2.2d/%2.2d/%d %2.2d:%2.2d\t",
#else /* !USG */
    (void) sprintf(buf,"%s\t%02d/%02d/%d %02d:%02d\t",
#endif /* !USG */
	    id, tm->tm_mon+1,tm->tm_mday,tm->tm_year,tm->tm_hour,tm->tm_min);
#endif /* TMNCONVERT */

}

int hstadd(id, rdate, edate, gp, artn)
/* add the given location to the history entry of a given ID */
char	*id;	/* ID to enter the info under */
time_t	rdate;	/* receipt timestamp of the article */
time_t	edate;	/* explicit expire date of the article */
char	*gp;	/* group of the article, or special token value */
nart_t	artn;	/* article number of the article */
{
    char    *ep;
    database	*rdhistsave;
    int		retval;

    /*
     * This horrible kludge is needed because we need hstseek() to use
     * wrhistdb instead of rdhistsdb (in the case where the two
     * pointers differ).
     */
    rdhistsave = rdhistdb;
    rdhistdb = wrhistdb;
    retval = hstseek(id, TRUE);
    rdhistdb = rdhistsave;
    
    /* 
     * generate ID and date fields if either:
     *	a) this is a new entry, or
     *  b) the existing entry is a forward reference that we're superseding.
     */
    if (retval == FAIL || (hstat() == REFERENCE && artn > 0))
	hstmake(id, rdate, edate, bfr);
    else	/* we're adding to a previously entered record */
    {
	/* make sure we don't insert same article in same group twice */
	for (ep = hstline(); *ep; ep++)
	    if (prefix(ep, gp) && strchr("/ ", ep[strlen(gp)]))
		return(hstat() != FAIL);

	(void) strcpy(bfr, hstline());
	bfr[strlen(bfr) - 1] = SPACE;	    /* replace trailing newline */
    }

    if (artn > 0)	/* user specified a non-zero article number */
    {
	/* add the given location to the list */
	(void) sprintf(bfr + strlen(bfr), GFORM, gp ? gp : "", artn);
    }
    else	/* tag the article with the given special attribute */
    {
	ep = bfr + strlen(bfr) - 1;

	/* we may want to trash the list that's already there */
	if (artn == FAIL)
	    while (*ep && *ep != TAB)
		ep--;

	(void) strcpy(ep + 1, gp);	/* append the attribute to the list */
    }
    (void) strcat(bfr, "\n");
    hstenter(bfr);

    return(hstat() != FAIL);
}

int hstexpire()
/* drop the current article location */
{
    register char	line[LBUFLEN];
    char	id[LBUFLEN];
    register int	retval;
    
    (void) strcpy(id, hstid());
    lcase(id);
    (void) artlstdel(&hstlst);
    (void) sprintf(line, "%s\t%ld %ld\t%s", chstname, chstdate, chstexpd,
	    artlstret(&hstlst));
    retval = dbmput(id, (unsigned) strlen(id),
		      line, (unsigned) strlen(line), wrhistdb);
    return(retval);
}

void hstparent(hp)
/* set up Back-Reference links implied by a References line */
hdr_t	*hp;	/* header of current article */
{
	register char	*this, *last, *hf;
	char		refbuf[LBUFLEN];
	
	if (hlblank(hp->h_references))	/* no references? then we're done */
	    return;

	(void) strcpy(refbuf, hp->h_references);
	(void) strcat(refbuf, " ");
	(void) strcat(refbuf, hp->h_ident);

	this = strtok(refbuf, ", ");
	while (last = this, this = strtok((char *)NULL, ", "))
	{
	    static hdr_t	hh;
	    /*
	     * if the parent isn't onsite, do a reference entry to the
	     * history file that will cause its Back-References line to be
	     * properly updated when the child comes in.
	     *
	     * If the parent article is on site, try to add the backreference
	     * to it. See mung.c for cautions on this.
	     */
	    if (!idvalid(last))
		logerr2("Bad references entry in %s: %s", hp->h_ident, last);
	    else if ((hf = hstfile(last)) == (char *)NULL)
		hstrefer(last, this);
	    else
	    {
		FILE	*f;

		hfree(&hh);
		if ((f = fopen(hf, "r")) && (hread(&hh, 0, f) > 0))
		{
		    /* the article is onsite, mung it */
		    (void) strcpy(bfr, hh.h_backrefs);
		    if (setadd(bfr, this, " ") && (mungread(hf, &hh)==SUCCEED))
		    {
			hlcpy(hh.h_backrefs, bfr);
			(void) mungwrite(TRUE);
		    }
		    (void) fclose(f);
		}
	    }
	}
}
   

/* wrhistory.c ends here */


