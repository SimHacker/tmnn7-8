/****************************************************************************

NAME
   vinfoline.c -- standard auxilliary functions for browsing readers

SYNOPSIS
   void vgroupline(ngrp, line)	-- generate readable line of info about a group
   group_t *ngrp; char *line;

   void vsubjline(lnum, line)	-- prefix-mark a subject list line
   nart_t lnum; char *line;

DESCRIPTION
   Miscellaneous I/O-manager-independent, command-set-independent functions
implementing the standard interface style for news readers live here, so that
they can be substituted without touching the browse code.

AUTHOR
   Eric S. Raymond
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

*****************************************************************************/
/*LINTLIBRARY*/
#include "news.h"
#include "vio.h"
#include "active.h"
#include "libread.h"
#include "newsrc.h"
#include "insrc.h"
#include "browse.h"

/* column 1 and 2 marks for subject list mode */
#define MK_PROMPT   '>'	/* column 1: points at current article */
#define MK_NOPROMPT ' '	/* column 1: points at non-current article */
#define MK_SEEN	    '_'	/* article has been seen */
#define MK_UNSEEN   ' '	/* article has not been seen */
#define MK_INVAL    '*'	/* article not available on this go-round */
#define MK_DELAYED  '^'	/* article is delay-marked (not yet used) */

void vgroupline(ngrp, line)
/* generate a human-readable line of information about a given group */
group_t	*ngrp;
char	*line;
{
    if (ngrp == ngactive())
	line[0] = MK_PROMPT;
    else
	line[0] = MK_NOPROMPT;

    if (!(ngrp->rc_flags & RC_NOTED))			/* new group? */
	line[1] = MK_INVAL;
    else if (!rcsubsc(ngrp))				/* unsubscribed? */
	line[1] = MK_SEEN;
    else						/* ordinary */
	line[1] = MK_UNSEEN;

    (void) sprintf(line + PREFSPACE,
		   "%-25.25s (%5.5ld to %5.5ld) %4ld waiting",
		   ngrp->ng_name, ngrp->ng_min, ngrp->ng_max, ngrp->ng_unread);
}

void vsubjline(lnum, line)
/* decorate returned subject info on current article */
nart_t	lnum;
char	*line;
{
    /* show either the current-article pointer or the article flag char */ 
    line[0] = (lnum == msgnum()) ? MK_PROMPT : MK_NOPROMPT;

    /* now indicate the article status character */
    switch (getbit(lnum, ngactive()))
    {
    case TRUE: line[1] = MK_SEEN; break;
    case FALSE: line[1] = MK_UNSEEN; break;
    default: line[1] = MK_INVAL; break;
    }
}

/* vinfoline.c ends here */
