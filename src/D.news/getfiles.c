/****************************************************************************

NAME
   getfiles.c -- get to database files and articles

SYNOPSIS
   #include "news.h"

   void artinit()		-- initialize paths to text and database

   char *artdir(ng)		-- generate the directory name of a group
   char *ng;

   int artname(place, buf)	-- generate an article name into given buffer
   place_t *place; char *buf;

DESCRIPTION
   These functions seal off the mapping from article and subject list
locations to physical files. In the reader, they may be replaced by
equivalent routines that talk to a network server.

SEE ALSO
   D.network/nntpread.c	-- nntp server access routines

AUTHOR
   Eric S. Raymond
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

**************************************************************************/
/*LINTLIBRARY*/
#include "news.h"
#include "active.h"

#ifndef NONLOCAL

void artinit()
{
    site.textdir = newsattr("textdir", TEXTDIR);
    site.spooldir = newsattr("spooldir", SPOOLDIR);
    site.batchdir = newsattr("batchdir", BATCHDIR);
    site.archdir = newsattr("archdir", ARCHDIR);
}

char *artdir(ng)
/* generate the directory name of a group */
char *ng;
{
    static char rbuf[BUFLEN];
    register char *p;

    (void) sprintf(rbuf, "%s/%s", site.textdir, ng);
    for (p = rbuf + strlen(site.textdir); *p; p++)
	if (*p == NGSEP)
	    *p = '/';
    return(rbuf);
}

int artname(place, buf)
/* generate an article name into the given buffer */
place_t	*place;
char *buf;
{
#ifdef BIGGROUPS
    (void) sprintf(buf, "%s/%ld",
#else
    (void) sprintf(buf, "%s/%d",
#endif /* BIGGROUPS */
		   artdir(place->m_group->ng_name),
		   (long)place->m_number);
    return(SUCCEED);
}

#endif /* NONLOCAL */

/* getfiles.c ends here */
