/*****************************************************************************

NAME
   feedbits.c -- feed bit reading, checking and caching

SYNOPSIS
   int feedbits()	-- set up feed bitmasks in active structure

DESCRIPTION
   These functions are used to implement checking of subscription bits for
article sends. Most of the implementation of the FEEDBITS and CACHEBITS
options lives here.

   If the FEEDBITS switch is on, the entry point feedbits() will
initialize an extra field containing a bit for each of the systems this site
feeds (including itself). The bit will tell whether the group is accepted
by that system. The 0 bit of this field is set if the field was set by the
code that sets these up. Bits 1 to n correspond to the first through nth
entries in the feeds file. This data is used by the ngaccept() function.

   If the CACHEBITS switch is on, the FEEDBITS initialization will look for a
bit file at ADM/feedbits to read subscription bits from. If this file is
older than the feeds file or non-existent it will be regenerated. The theory
here is that the feeds file seldom changes, and that reading the bitfile is
faster than regenerating the bitmap information with many ngmatch() calls.

NOTE
   The feed bits field of a group_t will hold subscription bits for at most
sizeof(bits_t) - 1 target systems. The bits_t type is defined in news.h,
initially as unsigned int. It can be changed to unsigned long with no problem;
this may be good thing to do on 16-bit machines (or, conceivably, on 32-bit
machines with 64-bit longs and *lots* of downstream sites...).

AUTHOR
   Eric S. Raymond
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

*****************************************************************************/
/* LINTLIBRARY */
#include "news.h"
#include "libpriv.h"
#include "active.h"
#include "dballoc.h"
#include "feeds.h"
#include "bitmacros.h"

#if defined(CACHEBITS) && !defined(FEEDBITS)
#undef CACHEBITS
#endif /* defined(CACHEBITS) && !defined(FEEDBITS) */

#ifdef FEEDBITS
int feedbits()
/* set up the feedbits fields in the active file */
{
    feed_t	*sys;

#ifdef CACHEBITS
    char	*bitsfile;
    FILE	*ffp;

    Sprint1(bitsfile, "%s/feedbits", site.admdir);

    /* if the feeds file has been munged, recompile the bitmap file */
    if (!exists(bitsfile) || modtime(bitsfile) < modtime(feeds.file))
    {
	/* prepare to write the recompiled version in a readable form */
	if ((ffp = fopen(bitsfile, "w")) == (FILE *)NULL)
	{
	    logerr0("Can't regenerate the feedbits file. Continuing...");
	    return(SUCCEED);
	}

	/* recompile the bitmaps in the ng_feeds fields of the active file */
#ifdef DEBUG
	(void) logerr0("Regenerating feed bits cache file");
#endif /* DEBUG */
#endif /* CACHEBITS */

	s_rewind();
	while ((sys = s_next()) != (feed_t *)NULL)
	{
	    ngrewind(TRUE);
	    while (ngnext())
		if (ngmatch(ngname(), sys->s_ngroups))
		    ngmkfeed(ngactive(), s_tell(sys));
	}

#ifdef CACHEBITS
	ngrewind(TRUE);
	while (ngnext())
	    (void) fprintf(ffp, "%s:%x\n", ngname(), ngactive()->ng_feeds);
	(void) fclose(ffp);
    }
    else    /* don't need to recompile, just read in the bitmaps */
    {
	ffp = xfopen(bitsfile, "r");
	while (fgets(bfr, BUFLEN, ffp) != (char *)NULL)
	{
	    char	grpname[BUFLEN];
	    bits_t	rbits;

	    if (sscanf(bfr, "%[^:]:%x\n", grpname, &rbits) == 2)
	    {
	        group_t *ngp = ngfind(grpname);

		if (ngp != (group_t *)NULL)
		{
		    ngp->ng_feeds = rbits;
		    ngmkfeed(ngp, 0);
		}
	    }
	}
	(void) fclose(ffp);
    }

    (void) free(bitsfile);
#endif /* CACHEBITS */

    return(SUCCEED);
}
#endif /* FEEDBITS */

/* feedbits.c ends here */
