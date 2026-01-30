/*****************************************************************************

NAME
   savescore.c -- save article/group priorities for later use

SYNOPSIS
   #include "libfilt.h"

   void initscore()			-- initialize article score cacheing

   void putscore(id)			-- store a score
   char *id;

   int getscore(id, pscore)		-- retrieve a score
   char *grp; int *pscore;

   void wrapscore()			-- write out altered article file

DESCRIPTION
   These functions are used to put article and group scores from a background-
mode filter run into a cache file, then retrieve them later when a pipe-mode
run wants them.

   Note: this is trickier than it looks. In particular, we have to arrange
to shed scores stored for expired articles, otherwise the cache file can
grow without bound.

AUTHOR
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

*****************************************************************************/
/* LINTLIBRARY */
#include "news.h"
#include "active.h"
#include "libfilt.h"

void initscore()
/* initialize score cacheing */
{
    /* do nothing, we're not ready to do real cacheing yet */
}

/*ARGSUSED0*/
void putscore(id, score)
/* cache priority score for given article */
char	*id;
int	score;
{
    /* do nothing, we're not ready to do real cacheing yet */
}

/*ARGSUSED0*/
int getscore(id, pscore)
/* retrieve priority score for given article */
char	*id;
int	*pscore;
{
    /* doesn't work yet, force caller to compute score */
    return(FAIL);
}

void wrapscore()
/* deinitialize score cacheing */
{
    /* do nothing, we're not ready to do real cacheing yet */
}

/* savescore.c ends here */

