/*****************************************************************************

NAME
   ttyout.c -- interface manager input functions for line-oriented display

SYNOPSIS
   void msg(...)    -- equivalent to fprintf(stderr,...) for up to 4 args

   void vprint(s)   -- print a line
   char *s;

   void vdelay()    -- dummy function

DESCRIPTION
   This module provides a common interface manager for tools that
do line-oriented output.

AUTHOR
   Eric S. Raymond
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

*****************************************************************************/
/*LINTLIBRARY*/
#include "news.h"		/* for stdio.h, ctype.h, nart_t and time_t */

int msg(s)
/* emit a message to the user -- always called through macros */
char *s;
{
    (void) fputs(s, stderr);
    (void) fputc('\n', stderr);
}

int vprint(line)
/* write a line of output (called through vprint[01]() macros)*/
char	*line;
{
    (void) puts(line);
    (void) fflush(stdout);
    return(SUCCEED);
}

/* ttyout.c ends here */
