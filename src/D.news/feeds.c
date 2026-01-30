/***************************************************************************

NAME
   feeds.c -- functions that access the feeds file

SYNOPSIS
   #include "feeds.h"

   extern s_maxhops;		-- maximum locality accepted by feed

   void s_read()		-- digest in contents of system file

   void s_rewind()		-- rewind current feed record pointer

   feed_t *s_next()		-- goto next record from the feeds file

   feed_t *s_find(system)	-- get a system record by name
   char *system;

   int s_tell(sp)		-- return index # of given system entry
   feed_t *sp;		

   char *s_option(sp, opt)	-- return args associated with a feed option
   feed_t *sp; char opt;

   int s_tell(sp)		-- returns record number of record
   feed_t *sp;

DESCRIPTION
   These functions provide access to the feeds file used to describe your
site's USENET links with other systems. All are pretty much self-explanatory
except s_option(). It returns whatever quoted argument information may follow
a given letter option in the target system's options field. If there is no
such option it returns NULL; if the option has no arguments it returns an
empty string. The returned info lives in a static buffer owned by s_option()
and will be overwritten by the next call.

   The s_maxhops variable contains the max of the hop counts implied by the
feed L options. It can be used to tell when we can bypass the broadcast logic
on systems that are (or are nearly) leaf nodes.

FILES
   ADM/feeds	-- file that controls your feeds to other sites

NOTE
   For this library module to operate properly, sitenames() must have been
called.

REVISED BY
   Eric S. Raymond
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

****************************************************************************/
/* LINTLIBRARY */
#include "news.h"
#include "dballoc.h"
#include "feeds.h"

int	s_maxhops;

#define SMASHWS

/*
 * Helper functions for feed record I/O */

int s_readrec(sp, fp)
/* read the next entry from a given file pointer */
register feed_t *sp;
FILE	*fp;
{
    char	sysbuf[2 * LBUFLEN];

    sysbuf[0] = '\0';

    /* read a logical line, possibly with \<CR> continuations, into sysbuf */
    while (fgets(bfr, LBUFLEN, fp) != (char *)NULL)
	if (bfr[0] == '#')	/* a comment, save it */
	{
	    if (sp->s_comments == (char *)NULL)	/* first line of comment */
	    {
		sp->s_comments = savestr(bfr);
		sp->s_clength = strlen(bfr) + 1;
	    }
	    else	/* there's more... */
	    {
		int	newlength = sp->s_clength + strlen(bfr) + 1;

		sp->s_comments = realloc(sp->s_comments, (iolen_t)newlength);
		(void) strcpy(sp->s_comments + sp->s_clength, bfr);
		sp->s_clength = newlength;
	    }    
	}
	else		/* not a comment */
	{
	    register char	*ep;
#ifdef SMASHWS
	    register char	*bp;

	    for (bp = bfr; isspace(*bp); bp++)
		continue;
	    ep = bp + strlen(bp);
#else
	    ep = bfr + strlen(bfr);
#endif /* SMASHWS */
	    if (ep[-1] != '\n')
		xerror0("feeds file line too long");
	    ep[-1] = '\0';	/* Zap the newline! */
	    /* Don't let blank lines mess us up */
#ifdef SMASHWS
	    if (ep == bp)
		    continue; 
#else
	    if (ep == bfr)
		    continue;
#endif	    
	    if (ep[-2] == '\\') {
#ifdef SMASHWS
		ep[-2] = '\0';
		(void) strcat(sysbuf, bp);
#else
		(void) strcat(sysbuf, bfr);
#endif /* SMASHWS */
	    }
	    else	/* we found an unescaped newline */
	    {
		ep[-1] = '\0';
#ifdef SMASHWS
		(void) strcat(sysbuf, bp);
#else
		(void) strcat(sysbuf, bfr);
#endif /* SMASHWS */
		break;
	    }
	}

    if (feof(fp))
	return(FAIL);

    /* parse out the line's subfields */
    sp->s_ngroups = sp->s_distribs = sp->s_flags = sp->s_xmit = (char *)NULL;
    sp->s_name = savestr(sysbuf);
    if (sp->s_ngroups = strchr(sp->s_name, S_FLDSEP))
    {
	*sp->s_ngroups++ = '\0';
	if (sp->s_distribs = strchr(sp->s_ngroups, S_FLDSEP))
	{
	    *sp->s_distribs++ = '\0';
	    if (sp->s_flags = strchr(sp->s_distribs, S_FLDSEP))
	    {
		*sp->s_flags++ = '\0';
		if (sp->s_xmit = strchr(sp->s_flags, S_FLDSEP))
		{
		    char	*cp;

		    *sp->s_xmit++ = '\0';
		    if (cp = strchr(sp->s_xmit, S_FLDSEP))
			*cp++ = '\0';
		}
	    }
	}
    }

    /* barf on missing fields */
    if (sp->s_name == (char *)NULL
		|| sp->s_ngroups == (char *)NULL
		|| sp->s_distribs == (char *)NULL
#ifndef B211COMPAT
		|| sp->s_xmit == (char *)NULL
#endif /* B211COMPAT */
		|| sp->s_flags == (char *)NULL)
	xerror1("bad feed specification, begins %s", bfr);

#ifdef B211COMPAT
    /* deal with 2.11-style 4-field format */
    if (sp->s_xmit == (char *)NULL)
    {
	sp->s_flags = sp->s_distribs;
	sp->s_xmit = sp->s_flags;
    }
#endif /* B211COMPAT */

    if (strcmp(sp->s_name, "ME") == 0)
	sp->s_name = savestr(site.pathname);

    /* fill in nosend field */
    if (sp->s_nosend = strchr(sp->s_name, S_FLEXSEP))
	*sp->s_nosend++ = '\0';
    else
	sp->s_nosend = "";

    /* fill in flexgroups field */
    if (sp->s_gflex = strchr(sp->s_ngroups, S_FLEXSEP))
	*sp->s_gflex++ = '\0';
    else
	sp->s_gflex = "";

    /* fill in flexdists field */
    if (sp->s_dflex = strchr(sp->s_distribs, S_FLEXSEP))
	*sp->s_dflex++ = '\0';
    else
	sp->s_dflex = "";

    return(SUCCEED);
}

/*
 * Entry points begin here
 */

dbdef_t feeds =
{
    (char *)NULL,	/* we must generate this at runtime */
    sizeof(feed_t),	/* size of an internal feed record */
    8,			/* initial size of feed_t array */
    4,			/* feed_t allocation chunk size */
    s_readrec,		/* input function */
    (int (*)())NULL,	/* no write function */
};

void s_read()
/* open the feeds file and digest it into core */
{
    feed_t	*sys, *base;

    /* read in the feeds data from the relevant file */
    Sprint1(feeds.file, "%s/feeds", site.admdir);
    (void) dbaread(&feeds);

    /* update the maximum-hops count */
#ifndef lint
    base = (feed_t *)feeds.records;
#else
    base = (feed_t *)NULL;
#endif /* lint */
    for (sys = base; sys < base + feeds.nextfree; sys++)
    {
	if (strcmp(sys->s_name, site.pathname))	/* don't include self! */
	{
	    char	*lopt;
	    int		lval;

	    if ((lopt = s_option(sys, 'L')) == (char *)NULL)
		lval = S_MANYHOPS;
	    else if ((lval = atoi(lopt)) == 0)
		lval = 1;

	    if (lval > s_maxhops)
		s_maxhops = lval;
	}
    }
    dbarewind(&feeds);
}

void s_rewind()
/* rewind the entry pointer */
{
    dbarewind(&feeds);
}

feed_t *s_next()
/* advance to next feed description */
{
    if (dbanext(&feeds) == FAIL)
	return((feed_t *)NULL);
    else
#ifdef lint
	return((feed_t *)NULL);
#else
	return((feed_t *)(feeds.cp));
#endif /* lint */
}

feed_t *s_find(sysname)
/* find the FEEDFILE record for a system */
char *sysname;
{
#ifdef lint
    return((feed_t *)NULL);
#else
    return((feed_t *)(dbafind(&feeds, sysname, streq)));
#endif /* lint */
}

char *s_option(sp, opt)
/*
 * Parse out a transmission option, returning the text following an
 * option if it is a sequence of digits or quoted string. In the latter
 * case, quotes are stripped. A double quote may be protected by \.
 */
feed_t	*sp;
char	opt;
{
    static char	    optbuf[SBUFLEN];
    char	    *cp, *tp = optbuf;
    int		    quoted;

    /* scan the flags field looking for the given options */
    for (quoted = 0, cp = sp->s_flags; *cp; cp++)
    {
	if (*cp == '"' && cp[-1] != '\'')
	    quoted++;
	else if ((quoted % 2) == 0 && *cp == opt)
	    break;
    }

    if (*cp != opt)	/* no such option present */
	return((char *)NULL);

    ++cp;			/* found option, look for arguments */

    while (islower(*cp))	/* gather lower-case suffixes */
	*tp++ = *cp++;

    if (isdigit(*cp))		/* option has a numeric argument */
    {
	while (isdigit(*cp))
	    *tp++ = *cp++;	/* grab chars till we run out of digits */
    }
    else if (*cp == '"')	/* option has a string argument */
    {
	while (*cp != '"')
	    *tp++ = *cp++;	/* grab chars till another doublequote */
    }

    *tp = '\0';
    return(optbuf);
}

/* feeds.c ends here */
