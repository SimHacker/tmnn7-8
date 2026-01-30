/*****************************************************************************

NAME
   paged.c -- pseudo-screen output for dumb terminals and ttys

SYNOPSIS
   #include "vio.h"

   void vinit(textl, statl, tick)	-- initialize hooks for screen support
   int (*textl)(); void (*statl)(), (*tick)();

   void vupdate()			-- force a screen update

   void vuline(to, from)		-- insert-highlight dummy
   char *to, *from;

   int vjunk(level)			-- declare level of refresh to do
   int level;

   bool vkexpand(cgetf, cungetf, buf)	-- dummy hook for k[0-9] %-escapes
   int (*cgetf)(), (*cungetf)(); char *buf;

   xxit(stat)	    		-- exit, cleaning up and returning given status
   int stat;

DESCRIPTION
   This module adds calls to the ttyin/ttyout interface manager that make it
possible to run a tool written to use visual.c entirely in cooked I/O mode.

   The protypical use is to implement a news reader with the vnews command
set that does page-at-a-time output without termcap.

SEE ALSO
   ttyin.c --	the rest of the line-oriented interface manager

AUTHOR
   Eric S. Raymond
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

*****************************************************************************/
/*LINTLIBRARY*/
#include "news.h"
#include "vio.h"

#ifndef private
#define private static
#endif /* private */
#define public

/* these are just defaults */
int LINES = 24;	/* number of rows on the terminal */
int COLS = 80;	/* number of columns on the terminal */
int vstat;	/* update status */

private char prompt[VBUFLEN] = "next?";	/* prompter */
private int (*textfmt)();	/* the text line formatter */
private void (*statfmt)();	/* the status line formatter */
private void (*ontick)();	/* call this on each alarm signal */

void vinit(textl, statl, tick)
/* initalize hooks for screen support */
int	(*textl)();	/* text line formatter */
void	(*statl)();	/* status line formatter */
void	(*tick)();	/* tick function */
{
    extern void	vsetidle();	/* TEMPORARY! */

    /* set up the line-formatting functions */
    textfmt = textl;
    statfmt = statl;
    ontick = tick;
    (void) bsetsize((LINES - 2), COLS);		/* initialize insrc routines */
    vsetidle(tick);
}

void vupdate()
/* display the current page-sized piece of article */
{
    int		i;
    char	vbfr[VBUFLEN];

    (*ontick)(0);

    /* first, update the text area */
    if (vstat != V_STATLINE)
    {
	for (i = 0; i < LINES - 2 && (*textfmt)(i, vbfr) != FAIL; i++)
	{
	    (void) fputs(vbfr, stdout);
	    (void) putchar('\n');
	}

	vstat = V_STATLINE;
    }

    /* prepend any notifications to the status line */
    if (dclock.justsaved)
	(void) printf(" [Saved .newsrc] ");
#ifdef CHECKMAIL
    if (dclock.mailstate == M_NEWMAIL)
	(void) printf("[You have new mail.] ");
#endif /* CHECKMAIL */
#ifdef SHOWTIME
    (void) printf("[%s] ", dclock.time);
#endif /* SHOWTIME */    

    /* the status line itself -- also serves as our prompt */
    (*statfmt)(prompt, vbfr);

    if (vbfr[0])
	(void) printf("%s: %s", vbfr, prompt);
    else
	(void) fputs(prompt, stdout);
}    

void vuline(to, from)
/* put underline format markers around a given buffer */
char	*to, *from;
{
    (void) strcpy(to, from);
}

/*ARGSUSED0*/
bool vkexpand(cgetf, cungetf, buf)
/* in a visual interface, expand k[0-9] %-construct into the given buffer */
int (*cgetf)();		/* key character of escape */
int (*cungetf)();	/* how to push back characters */
char *buf;		/* buffer to expand escapes into */
{
    return(FALSE);
}

catch_t xxit(status)
int	status;
{
    exit(status);
}

/* paged.c ends here */
