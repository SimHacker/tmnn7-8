/****************************************************************************

NAME
   wractive.c -- functions for modifying and writing the active file

SYNOPSIS
   #include "active.h"

   nart_t ngnewart(ngp)		-- add a new article number, in place
   group_t *ngp;

   void ngcreate(name, mod)	-- add a new group to the active file in place
   char *name; bool mod;

   void ngdelete()		-- mark current group for deletion

   void ngshow(ngp, fp)		-- display active data of a group
   group_t *ngp; FILE *fp;

   int wractfile(fp, dodels)	-- write active group data to given fp
   FILE	*fp; bool dodels;

   void wractive(dodels)	-- write active data back out to its file
   bool dodels;

   char **rdflags(mode)		-- read in given administration flags
   int mode;

DESCRIPTION
   These functions work with the code in rdactive.c to provide a clean
interface to the active-groups file used by the USENET software. For
general information on the active file formats and incore data structure
see the header comment of rdactive.c

   The wractive() function updates the entire active file from the in-core
image.

   The functions ngcreate() and ngnewart() do not use the allocation
machinery the other functions are built around; they alter the active file
in place (ngnewart() uses lock() and unlock() to ensure that multiple users
can do this consistently; ngcreate() relies on the atomicity of small writes.
The intent is to permit multiple posting programs to run concurrently. Note
that posting programs still can and should use the result of a rdactive()
to check for the validity of group names.

However, if SPOOLNEWS is defined, ngcreate() and ngnewart() do not
alter the active file directly.  This is done because it much faster
to update the in-core version that it is to rewrite the entire
active file.

   The rdflags() function reads newsgroup control information out of the
group administration file ADM/admin. See news(5) for details of the admin
file format. It returns a NULL-terminated dope vector holding malloced copies
of the expiration control lines (statistics routines can index this with the
contents of the ng_findex field of each group, which rdflags() sets to the line
number of the last line that the group matches).

NOTE
   When the code is compiled with DEBUG and debug is on, actions that would
modify the on-disk version of the database are suppressed. Diagnostics are
printed instead.

WARNING
   When deallocating an active-groups array after a rdflags(NG_GETDIST), bear
in mind that more than one ng_defdist fields may point at the same malloced
distribution-name storage.

FILES
   ADM/active	-- active group information
   ADM/admin	-- group flags and expiration period information

AUTHOR
   Eric S. Raymond
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

**************************************************************************/
/*LINTLIBRARY*/
#include "news.h"
#include "libpriv.h"
#include "active.h"

extern int	rdactcount;	/* defined in rdactive.c */

/*
 * What we really want to say here is `if sizeof(nart_t) >= 4', but that's
 * not portable. This loses if (a) you have 4-byte ints and haven't defined
 * BIGGROUPS, or (b) you have BIGGROUPS on and 2-byte longs. The maintainers
 * are not interested in the latter case. The price for being able to support
 * really big article numbers is your active-groups array gets bigger...
 */
#ifdef BIGGROUPS
#define MAXART	999999999	/* largest guaranteed to fit in a long */
#else
#define MAXART	99999		/* largest guaranteed to fit in an int */
#endif /* BIGGROUPS */

#define MAXCTLS		256	/* max expiration control lines */

#define	SEPARATORS	" \t:"

nart_t ngnewart(ngp)
/* reserve space for a new article */
group_t	*ngp;
{
#ifndef SPOOLNEWS
#ifdef DEBUG
    if (!debug)
#endif /* DEBUG */
	lock();
    (void) ngreread(ngp, NULLPRED);
#endif
    if (ngp->ng_min < 0 || ngp->ng_max >= MAXART)
    {
	logerr2("bad maxart %ld ng %s, setting to 1",
		(long)ngp->ng_max, ngp->ng_name);
	ngp->ng_max = 1;
    }

    ngp->ng_max++;
    ngp->ng_age = time((time_t *)NULL);
#ifndef SPOOLNEWS
    (void) fseek(active.fp,
		 (off_t)(ngp->ng_fseek - strlen(ngp->ng_name) - 1), SEEK_SET);
#ifdef DEBUG
    if (debug)
	log1("ngnewart()'s update of %s suppressed", ngp->ng_name);
    else
#endif /* DEBUG */
	ngshow(ngp, active.fp);
#ifdef DEBUG
    if (!debug)
#endif /* DEBUG */
	unlock();
#endif /* SPOOLNEWS */
    return(ngp->ng_max);
}

static void ngmake(newname, flags)
/* create a group, no check to see if parents exist */
char	*newname;
bits_t	flags;
{
    group_t		*new = NULL;
    extern group_t	*ngalloc();	/* not in the public interface! */
    
    if (ngfind(newname) == (group_t *)NULL)
    {
	/* we fill in everything except the expire and fseek fields here */
	new = ngalloc(newname);
	new->ng_name = savestr(newname);
	new->ng_min = (nart_t)1;
	new->ng_max = new->ng_unread = (nart_t)0;
	new->ng_flags = flags;
	new->ng_age = time((time_t *)NULL);

#ifndef SPOOLNEWS	
#ifdef DEBUG
	if (debug)
	    log1("ngcreate()'s add of %s to active file suppressed", newname);
	else
#endif				/* DEBUG */
	{
	    (void) fseek(active.fp, (off_t)0, SEEK_END);
	    new->ng_fseek = ftell(active.fp);
	    ngshow(new, active.fp);
	    (void) fputc('\n', active.fp);
	}
#endif
    }
}

void ngcreate(ng, moderated)
/* append a new group to the active file */
char	*ng;		/* name of new newsgroup */
bool	moderated;	/* is it moderated */
{
#ifdef MAKEPARENTS
    char	*ep, *np, newname[BUFLEN];
    int		len;

    (void) strcpy(newname, ng);
    newname[len = strlen(newname)] = NGSEP;
    newname[len+1] = '\0';
    for (ep = newname; (np = strchr(ep, NGSEP)) != (char *)NULL; ep = np + 1)
    {
	*np = '\0';
	ngmake(newname, (moderated && (ep-newname == len)) ? NG_MODERATED : 0);
	*np = NGSEP;
    }
#else
    /* just make the group itself */    
    ngmake(ng, moderated ? NG_MODERATED : (unsigned) 0);	
#endif /* MAKEPARENTS */
}

void ngdelete()
/* delete the current group */
{
    register group_t	*cgp = active.article.m_group;

#ifdef DEBUG
    if (!debug)
#endif /* DEBUG */
	lock();
    (void) ngreread(cgp, NULLPRED);
    cgp->ng_flags |= NG_REMOVED;
    (void) fseek(active.fp,
		 (off_t)(cgp->ng_fseek - strlen(cgp->ng_name) - 1), SEEK_SET);
#ifdef DEBUG
    if (debug)
	log1("ngdelete()'s update of %s suppressed", cgp->ng_name);
    else
#endif /* DEBUG */
	ngshow(cgp, active.fp);
#ifdef DEBUG
    if (!debug)
#endif /* DEBUG */
	unlock();
}

void ngshow(ngp, fp)
/* display the state of a group */
group_t	*ngp;
FILE	*fp;
{
    char	flag;

    if (ngp->ng_flags & NG_REMOVED)
	flag = 'x';
    else if (ngp->ng_flags & NG_MODERATED)
	flag = 'm';
    else
	flag = 'y';

    /*
     * Constrained to be same size as any old record by %09d format.
     * This will overflow when we get 999,999,999 articles in 1 group (!).
     */
    (void) fprintf(fp,
		"%s %09ld %09ld %c %08lx",
		ngp->ng_name, (long)ngp->ng_max, (long)ngp->ng_min,
    		flag,
		ngp->ng_age);

    /* we don't emit LF here so we can append more info per line as needed */
}

int wractfile(fp, dodels)
/* write active data in readable format to a given destination */
FILE	*fp;	/* file pointer to write to */
bool	dodels;	/* if TRUE, don't write groups marked for deletion */
{
    register group_t	*ngp;

    if (!rdactcount) {
	logerr0("Warning: Botch!  wractfile() called before rdactive()....");
	return(FAIL);
    }
    for (ngp = active.newsgroups; ngp < active.newsgroups + active.ngc; ngp++)
    {
	if (dodels && (ngp->ng_flags & NG_REMOVED))
	    continue;

	ngshow(ngp, fp);
	(void) fputc('\n', fp);
    }
    return(SUCCEED);
}

void wractive(dodels)
/* write out the in-core active file data */
bool	dodels;	/* if TRUE, don't write groups marked for deletion */
{
    FILE	    *wrcfp;
    char	    new_active[BUFLEN], old_active[BUFLEN];
    int		    omask = umask(0113);

    if (!rdactcount) {
	logerr0("Warning: Botch!  wractive() called before rdactive()....");
	return;
    }
    if (!ACTIVE)
	xerror0("Programming botch!  ACTIVE not initialized!");
    (void) strcpy(new_active, ACTIVE);
    (void) strcat(new_active, ".new");
    (void) strcpy(old_active, ACTIVE);
    (void) strcpy(old_active, ".old");
    
#ifdef OTEST
    (void) fprintf(stderr, "wractive: writing to %s\n", new_active);
#endif /* TEST */

#ifdef DEBUG
    if (debug)
	log0("wractive()'s write of active file suppressed");
    else
    {
#endif /* DEBUG */

#ifdef VMS
    (void) vmsdelete(new_active);
#endif

    /* here goes the actual I/O */
    if (
	(wrcfp = xfopen(new_active, "w")) == (FILE *)NULL
	|| wractfile(wrcfp, dodels) != SUCCEED
	|| fclose(wrcfp) < 0
	)
	xerror1("Error writing new active file %s - no changes made\n", new_active);

    (void) fclose(active.fp);
#ifdef VMS
    (void) vmsdelete(ACTIVE);
#endif
    (void) unlink(old_active);
    (void) link(ACTIVE, old_active);
    if (rename(new_active, ACTIVE) < 0)
	xerror1("Cannot rename new active file to %s", ACTIVE);

    (void) umask(omask);
#ifdef DEBUG
    }
#endif /* DEBUG */
}

typedef struct
{
    char	flag;
    int		value;
}
flagdef;

char **rdflags(mode)
/* read in a given collection of administration flags */
int	mode;
{
    FILE    *fp;
    static flagdef flaglist[] =
    {
	/* to add more flags, just add entries to this table */
	{'v',	NG_VOLATILE},
	{'e',	NG_EXPIREIT},
	{'a',	NG_ARCHIVEIT},
	{'i',	NG_IGNEXPDATE},
	{'c',	NG_COMPRESSED},
	{'x',	NG_REMOVED},
	{'\0',	0}
    };
    flagdef *fpt;
    static char *ctrllines[MAXCTLS];
    char **cpp;
    int ctline = 0;

    (void) sprintf(bfr, "%s/admin", site.admdir);
    if ((fp = fopen(bfr, "r")) == (FILE *)NULL)
	return((char **)NULL);

    for (cpp = ctrllines; cpp < ctrllines + MAXCTLS; cpp++)
    {
	char	*cp, *name;
	bool	innum = FALSE, notflg = FALSE;
	int	turnon = 0x00, turnoff = 0x00;
	long	expire = 0L;

	if (fgets(bfr, BUFLEN, fp) == (char *)NULL)
	    break;

	(void) nstrip(bfr);
	*cpp = savestr(bfr);
	ctline++;
	
	if ((cp = strchr(bfr, '#')) != (char *)NULL)
	    *cp = '\0';

	if (bfr[0] == '\0')
	    continue;

	/* field 1 is the subscription */
	name = strtok(bfr, SEPARATORS);

	/* field 2 is the flags */
	if ((cp = strtok((char *)NULL, SEPARATORS)) == (char *)NULL)
	    continue;

	/* analyze the options in field 2 */
	for (; *cp; cp++)
	{
	    /* handle strings of digits in the flags field */
	    if (isdigit(*cp))
	    {
		if (!innum)
		{
		    expire = atol(cp) * DAYS;
		    innum = TRUE;
		    continue;
		}
		continue;
	    }
	    innum = FALSE;

	    /* now handle ! and letters in the flags field */
	    if (*cp == '!')
		notflg = TRUE;
	    else
	    {
		for (fpt = flaglist; fpt->flag; fpt++)
		    if (fpt->flag == *cp)
		    {
			if (notflg)
			    turnoff |= fpt->value;
			else
			    turnon |= fpt->value;
			break;
		    }
		if (fpt->flag == '\0')
		    notflg = FALSE;
	    }
	}

	/* field 3, if present, is a distribution */
	if (cp = strtok((char *)NULL, SEPARATORS))
	    cp = *cpp + (cp - bfr);

	/* next step: apply options to each group that matches the spec */
	if ((turnon | turnoff) & mode)
	{
	    ngrewind(TRUE);
	    while (ngnext())
	    {
		if (ngmatch(ngname(), name))
		{
		    group_t *ngrp = active.article.m_group;

		    ngrp->ng_findex = ctline;
		    ngrp->ng_flags |= (mode & turnon);
		    ngrp->ng_flags &=~ (mode & turnoff);
		    if (expire)
		    {
			ngrp->ng_expire = expire;
			ngrp->ng_flags |= NG_EXPIRE;
		    }
		    if ((mode & NG_GETDIST) && cp != (char *)NULL)
			ngrp->ng_defdist = cp;
		}
	    }
	}
    }
    (void) fclose(fp);
    *cpp++ = (char *)NULL;
    return((char **) ctrllines);
}

/* wractive.c ends here */
