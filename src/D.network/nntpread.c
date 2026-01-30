/****************************************************************************

NAME
   nntpread.c -- functions for talking to the nntp server (reader side)

SYNOPSIS
   #include "news.h"
   #include "active.h"
   #include "history.h"

   void artinit()		-- establish connect to server

   int artname(place, buf)	-- generate an article name into given buffer
   place_t *place; char *buf;

   char *getactive();		-- fetch an active file copy

   void hstread(flg)		-- establish nntp connection
   bool flg;

   int hstseek(id, wlock)	-- select the history of a given id
   char	*id; bool wlock;

   int hstloc(mloc)		-- get next associated location
   place_t *mloc;

   void hstclose()		-- release history file

DESCRIPTION
   These functions mimic the entry points of getfiles.c and the history file
interface and provide a way to get current active file data, but they talk to
NNTP. This code thus does as much as possible to hide the differences between
sites that run with their own administrative files in ADM (the normal case)
and sites that talk to an NNTP server over Internet.

NOTE
   #define ALIASES should be turned on if you are running NNTP 1.5 patchlevel 4
or a more recent version.

BUGS
   Probably legion -- I don't have an nntp to test this on.

SEE ALSO
   rdhistory.c	-- read side of the history access code
   wrhistory.c	-- write side of the history access code
   artlist.c	-- functions for manipulating article reference lists.

AUTHOR
   Eric S. Raymond
Adapted from Stan Barber's NNTP support for 2.11B and UNTESTED. No warranties
express or implied, your mileage may vary, and like that.
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

**************************************************************************/
/*LINTLIBRARY*/
#include "news.h"
#include "active.h" 
#include "newsrc.h"
#include "history.h"
#include "nntp.h"
#include "response_codes.h"

private char nntpbuf[BUFLEN];	/* scratch space for comm. with nntp */
private lptr_t	hstlst;		/* location descriptor for the xref list */

/* here's the getfiles.c emulation */

void artinit()
/* establish an NNTP connection to the server */
{
    char	*host = newsattr("nntphost", NNTPSERVER);

    /* try to open a connection to the nntp server */
    if (nntpinit(host) == FAIL)
	xerror1("Couldn't reach %s news server, try again later.", host);
}

int artname(place, buf)
/* snarf the textfile corresponding to an article location */
place_t	*place;
char	*buf;
{
    static char	*oldpt = (char *)NULL;

    /*
     * KLUGE -- assuming we never step on our current-article-file-name global
     * buffers, this will delete the last current article copy
     */
    if (oldpt != (char *)NULL && oldpt[0])
	(void) unlink(oldpt);

    (void) sprintf(bfr, "GROUP %s", place->m_group->ng_name);
    if (nntpcommand(bfr, sizeof(bfr), 0) == FAIL)
	return(FAIL);

    if (buf == (char *)NULL)	/* user just wants to know if it exists */
    {
	(void) sprintf(bfr, "STAT %ld", (long)place->m_number);
	return(nntpcommand(bfr, sizeof(bfr), 0));
    }
    else			/* user wants the text back */
    {
	(void) sprintf(bfr, "ARTICLE %ld", (long)place->m_number);
	if (nntpsnarf(bfr, buf) == FAIL)
	    return(FAIL);
	else
	{
	    oldpt = buf;
	    return(SUCCEED);
	}
    }
}

/* here's the active-file snarfer */

char *getactive()
/* snarf a copy of the server's active file */
{
    char	actname[BUFLEN];

    if (nntpsnarf("LIST", actname) != SUCCEED)
	xerror1("Can't get active file from server: got \"%s\"", bfr);
    ACTIVE = savestr(actname);
}

/* here's the history file interface emulation */

/*ARGSUSED0*/
void hstread(flg)
bool	flg;
{
    /* do nothing -- connection already established */
}

/* ARGSUSED2 */
int hstseek(name, wlock)
/* grab cross-reference data on an article id */
char *name;
bool wlock;
{
    static char	hstline[LBUFLEN];
#ifdef ALIASES		/* Erik and Phil said they'd implement this */
    FILE	*lfp;
    char	*cp;

    (void) sprintf(hstline, "ALIASES %s");
    if (nntpsnarf(hstline, hstline) == FAIL || lfp = fopen("r", hstline))
	return(FAIL);
    else
    {
	for (cp = hstline; fgets(cp, sizeof(hstline), lfp); cp += strlen(cp))
	    (void) strcat(cp, " ");
	artlstset(&hstlst, hstline);
	return(SUCCEED);
    }
#else /* !ALIASES */
    char	*p;

    /* first, check that the server knows about the message */
    (void) sprintf(bfr, "STAT %s",name);
    if (nntpcommand(bfr, sizeof(bfr), 0) == FAIL)
	return(FAIL);

    /* then demand the xref header line */
    (void) sprintf(bfr, "XHDR xref %s",name);
    if (nntpcommand(bfr, sizeof(bfr), 1) == FAIL)
	return(FAIL);

    /* if there is an xref line, simply convert it */
    if ((p = strchr(bfr, ' ')) != (char *)NULL && *++p != '(')
    {
	/* copy to target buffer, convert from xref form to history form */
	(void) strcpy(hstline, p);
	for (p = hstline; p != (char *)NULL && *p; p = strchr(hstline, ':'))
	    *p = '/';
    }
    else	/* no xref line -- must do it the ugly way */
    {
	char grouplist[BUFLEN], *thisgroup, *bufp;

	(void) sprintf(grouplist, "XHDR newsgroups %s", name);
	if (nntpcommand(grouplist, sizeof(grouplist), 1) == FAIL)
	    return(FAIL);

	bufp = hstline;
	thisgroup = strtok(grouplist, " ");
	do {
	    group_t	*ngp;
	    nart_t	artnum;

	    if ((ngp = ngfind(thisgroup)) == (group_t *)NULL)
		return(FAIL);

	    (void)sprintf(bfr,"XHDR message-id %d-%d",ngp->ng_min,ngp->ng_max);
	    nntpput(bfr);
	    (void) nntpget(bfr, sizeof(bfr));
	    if (*bfr != CHAR_OK)
		return(FAIL);
	    while (nntpget(bfr, sizeof(bfr)) >= 0)
	    {
		if (*bfr == '.'  && strlen(bfr) == 1) 
		    return(FAIL);
		if (strindex(bfr, name) > -1)
		{
		    (void) sprintf(bufp, "%s/%s ", thisgroup, artnum);
		    bufp += strlen(bufp);
		    break;
		}
		else
		    artnum++;
	    }
	    nntpsync();
	} while
	    ((thisgroup = strtok(NULL, " ")) != (char *)NULL);
    }
    artlstset(&hstlst, hstline);
    return(SUCCEED);
#endif /* ALIASES */
}

int hstloc(exloc)
/* grab the next argument from the current location list */
place_t    *exloc;
{
    return(artlstloc(&hstlst, exloc));
}

void hstclose()
/* close the history file -- really the connection to the NNTP server */
{
    nntpclose();
    (void) unlink(ACTIVE);
}

/* nntpread.c ends here */
