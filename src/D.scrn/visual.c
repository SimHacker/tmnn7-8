/****************************************************************************

NAME
   visual.c - high-level screen handling for visual tools

SYNOPSIS
   void vinit(textl, statl, tick)	initialize hooks for screen support
   int (*textl)(); void (*statl)(), (*tick)();

   int msg(legend)	-- printf() with up to 4 args to status line
   char *legend;

   int vprint(lin)	-- queue a line to go to the screen
   char *lin;

   void vuline(to, from)	-- insert highlight marks around a buffer
   char *to, *from;

   char vgetc()		-- get character with echo/expansion

   void vungetch()	-- push a char back, with echo

   char *vgetline(pr)	-- emit a prompt and return a string of input
   char *prompt;

   void vdelay()	-- delay so a message can be read

   void vttymode()	-- go to normal tty mode

   void vcrtmode(ref)	-- go to screen mode
   bool ref;

   vupdate()		-- force a screen update

   int vjunk(level)	-- declare level of refresh to do
   int level;

   char *vgetcmd(pr)	-- get a line of text for a command
   char *pr;

   bool vkexpand(cgetf, cungetf, buf)	-- hook for k[0-9] %-escapes
   int (*cgetf)(), (*cungetf)(); char *buf;

   catch_t xxit(stat)	-- exit, cleaning up and returning given status
   int stat;

DESCRIPTION
   This code supports vnews-style screen handling for interactive browser
interfaces.

   The xxit() function is used by the interrupt handlers defined in
readinit.c.

   If RNESCAPES is defined, the implementation of vgetline() in this module
has the special property that typing %% will force escape expansion of the
buffer.

SEE ALSO
   readinit.c	-- the newsreader initialization code
   vnews.c	-- switch handling and command interpretation for vnews.
   rfuncs.c	-- common functions used by interactive news readers
   vtermio.c	-- queued raw-mode I/O handling.
   ttyctl.c	-- tty mode control
   scrnctl.c	-- screen control

BUGS
   The %%-expansion should really be done via an escape to a state machine.
Then this module wouldn't have to know about news.h.

AUTHOR
   Eric S. Raymond
working from an original designed by Ken Almquist.
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

*****************************************************************************/
/*LINTLIBRARY*/
#include "news.h"
#include "libscrn.h"
#include "vio.h"
#include "vtermio.h"
#include "insrc.h"
#include "browse.h"

#ifndef private
#ifdef DEBUG
#define private
#else
#define private static
#endif /* DEBUG */
#endif /* private */
#define public

#ifdef SIGWIND			/* if we're on a UNIX PC */
#define SIGWINCH SIGWIND	/* treat SIGWIND like a 4.3BSD SIGWINCH */
#endif				/* (see scrnctl.c for details on this trick) */

#if defined(SIGTSTP) || defined(SIGCONT) || defined(SIGWINCH)
#define SCREEN_INTERRUPTS
#endif /* SIGTSTP, SIGCONT, SIGWINCH */

#define STATTOP		/* status line at top of screen, rather than bottom */

/* these defines specify the screen layout */
#ifdef STATTOP
#define PRLINE	0	/* prompter line */
#define SPLINE	(LINES-1)/* secondary prompt line */
#define ARTWIN	1	/* first line of article window */
#else
#define PRLINE	(LINES-1)/* prompter line */
#define SPLINE	(LINES-2)/* secondary prompt line */
#define ARTWIN	0	/* first line of article window */
#endif

#define PRIPRLEN    30		/* length of primary prompter */
#define SECPRLEN    145		/* length of secondary prompter */

#define META	    0x80	/* meta character bit (as in emacs) */
#define RANGEMARK   ','		/* allow this in numeric prefixes */

/* values of curflag, used to force cursor to a given location */
#define CURWAIT	    1		/* cursor after prompt */
#define CURREAD	    2		/* cursor after secondary prompt */
#define CURHOME	    3		/* cursor at home position */

#ifdef A_UNDERLINE
#define UL_ON	0x0e		/* ASCII SO character */
#define UL_OFF	0x0f		/* ASCII SI character */
#endif /* A_UNDERLINE */

int vstat;	/* update status */

private char vbfr[VBUFLEN];			/* common buffer */
private char prompt[PRIPRLEN + 1] = "next?";	/* prompter */
private char secpr[SECPRLEN + 1];		/* secondary prompt */

private short curflag;		/* where to locate cursor */
private int (*textfmt)();	/* the text line formatter */
private void (*statfmt)();	/* the status line formatter */
private void (*ontick)();	/* call this on each alarm signal */

/* data for environment queries */
private char cerase;		/* erase character */
private char ckill;		/* kill character */
private char cintr;		/* interrupt character */
#ifdef TIOCGLTC
private char cwerase;		/* word erase character */
#endif /* TIOCGLTC */

void vclear()
/* force low-level refresh */
{
    (void) clearok(stdscr, 1);
}

void vinit(textl, statl, tick)
/* initalize hooks for screen support */
int	(*textl)();	/* text line formatter */
void	(*statl)();	/* status line formatter */
void	(*tick)();	/* tick function */
{
    forward catch_t onsig();

    /* set up the line-formatting functions */
    textfmt = textl;
    statfmt = statl;
    ontick = tick;

    /* this has to be done *before* vterminit(), else nodelay() will barf */
    (void) initscr();			/* set up screen structures */
    (void) cbreak();			/* define screen-mode tty params */
    (void) nonl();
#ifdef BSD
    /*
     * this is needed by many BSDs to avoid doubling of all input -- initscr()
     * by itself should do it, but a common bug in nonl() undoes noecho.
     */
    noecho();
#endif /* BSD */

#ifdef KEYPAD
    (void) keypad(stdscr, TRUE);	/* enable function keys */
#endif /* KEYPAD */

    /* now set up for full-screen fanciness, including nodelay input */
    vterminit(vupdate, onsig, tick);
#ifdef SIGWINCH
    (void) signal(SIGWINCH, onsig);	/* allow changing window size */
#endif /* SIGWINCH */

    cerase = erasechar();
    ckill = killchar();
    cintr = intchar();
#ifdef TIOCGLTC
    cwerase = werasechar();
#endif /* TIOCGLTC */
    (void) getsize();			/* get current screen size */
    (void) bsetsize((LINES - 2), COLS);	/* initialize insrc routines */
    (void) refresh();
}

void vhome()
/* sit the cursor up in the corner while next article is being gotten */
{
    curflag = CURHOME;
    (void) move(0, 0);
    if (!vcheckin())
	(void) refresh();
}

/*ARGSUSED0*/
char *vgetcmd(pr)
/* get the first key letter of a command */
char *pr;
{
    register int ch;
    char	*argpref;

restart:    
    curflag = CURWAIT;

#ifdef MACROS
    /*
     * Accept chars via vgetch() till we get a recognized macro or
     * something that can't be a prefix of any macro, and enqueue
     * the results for vgetc().
     */
    vmacro();
#endif /* MACROS */

    /* first, process possible digit prefix */
    for (argpref = vbfr;;)
    {
	if ((ch = vgetc()) == EOF)
	    return((char *)NULL);
	else if (ch == erasechar())
	{
	    if (argpref > vbfr)
		vputc(erasechar());
	    else
		goto restart;
	}
	else if (ch == killchar()
#ifdef TIOCGLTC
	    	|| ch == werasechar()
#endif /* TIOCGLTC */
	    )
	{
	    while (argpref-- > vbfr)
		(void) vputc(erasechar());
	    goto restart;
	}
	else if (isdigit(ch) || ch == RANGEMARK)
	    *argpref++ = ch;
	else
	    break;
    }

    /* now get the actual command key */
    if (ch == ESC && (ch = vgetc()) != cintr)
	ch |= META;
    
    *argpref++ = ch;
    *argpref = '\0';

    /* make sure screen shows current states of arguments */
    secpr[0] = '\0';
    (void) refresh();
    return(vbfr);
}

int vgetc()
/* get a character, doing appropriate exchoing and expansion */
{
    int   c = vgetch();

    /* someday macro expansion should be done here */
    if (curflag != CURWAIT)
	curflag = CURWAIT;
    if (isprint(c))
	(void) vputc(c);
#ifdef ASCII
    else if (c == ESC)
    {
	vputc('E');
	vputc('S');
	vputc('C');
	vputc('-');
    }
    else if (iscntrl(c) && c != erasechar() && c != killchar())
    {
	(void) vputc('^');
	(void) vputc('@' + c);
    }
#endif /* ASCII */
    return(c);
}

void vdelay()
/* wait long enough for a message to be seen */
{
    (void) sleep(1);
}

void vcrtmode(reflag)
/* return to normal screen mode */
bool reflag;	/* force-refresh flag */
{
    (void) refresh();
    if (reflag)
	(void) clearok(stdscr, 1);
}

void vttymode()
/* move to normal tty mode */
{
    (void) endwin();
}

char *vgetline(prompter)
/* get a line of input in the primary command line */
char	*prompter;
{
    static char	buf[VBUFLEN];
    char	*p, *q, *r;
    int		c, lastc;

    curflag = CURREAD;
    r = buf;
    lastc = '\0';
    for (;;)
    {
	*r = '\0';
	p = secpr;
	for (q = prompter ; *q ; q++)
	    *p++ = *q;
	for (q = buf ; *q ; q++)
	{
	    if (p < &secpr[SECPRLEN-1] && *q && isprint(*q))
		*p++ = *q;
	}
	*p = '\0';
	c = vgetch();

#ifdef RNESCAPES
	if (c == RNESCCHR && lastc == RNESCCHR)
	{
	    *r++ = RNESCCHR;
	    *r++ = RNESCCHR;
	    *r = '\0';
	    strexpand(buf, vbfr);
	    (void) strcpy(buf, vbfr); 
	    continue;
	}
#endif /* RNESCAPES */

	if (c == EOF)
	    c = cintr;
	if (c == '\n' || c == cintr)
	    break;
	if (c == cerase)
	{
	    if (lastc == '\\')
		r[-1] = c;
	    else if (r > buf)
		r--;
	}
	else if (c == ckill)
	{
	    if (lastc == '\\')
		r[-1] = c;
	    else
		r = buf;
	}
#ifdef TIOCGLTC
	else if (c == cwerase)
	{
	    if (lastc == '\\')
		r[-1] = c;
	    else
	    {
		while (r > buf && (r[-1] == ' ' || r[-1] == '\t'))
		    r--;
		while (r > buf && r[-1] != ' ' && r[-1] != '\t')
		    r--;
	    }
	}
#endif /* TIOCGLTC */
	else
	    *r++ = c;

	lastc = c;
    }
    curflag = CURHOME;
    secpr[0] = '\0';
    return(c == cintr ? (char *)NULL : buf);
}

/* VARARGS1 */
int msg(s)
/* emit a message to the message area -- always called through macros */
char *s;
{
    (void) strcpy(secpr, s);
}

int vprint(lin)
/* queue a line to go to the screen */
char *lin;
{
    return(tfappend(lin));
}

void vuline(to, from)
/* put underline format markers around a given buffer */
char	*to, *from;
{
#ifdef A_UNDERLINE
    (void) sprintf(to, "%c%s%c", UL_ON, from, UL_OFF);
#else
    (void) strcpy(to, from);
#endif /* A_UNDERLINE */
}

/* now set up signal handlers and other arcana */

#ifdef SCREEN_INTERRUPTS
#ifdef SIGWINCH
static int  UPDATING = 0, WINCH = 0;
#endif /* SIGWINCH */
#endif /* SCREEN_INTERRUPTS */

private catch_t onsig(signo)
/* handle random incoming signals */
{
    switch (signo)
    {
    case SIGALRM:		/* gets called every second */
	(*ontick)(0);
	break;
#ifdef SIGTSTP
    case SIGTSTP:		/* we're stopped -- free up the screen */
	(void) endwin();
	break;
#endif /* SIGTSTP */
#ifdef SIGCONT
    case SIGCONT:		/* refresh screen before job continuation */
	(*ontick)(0);
	(void) refresh();
	break;
#endif /* SIGCONT */
#ifdef SIGWINCH
    case SIGWINCH:		/* our window changed -- get the new size */
	if (getsize() != FAIL && !bsetsize((LINES - 2),COLS))
	    (void) refresh();
	else if (UPDATING)		/* concurrency.  wow! */
	    WINCH++;
	else if (WINCH == 0)
	{
	    vstat = V_JUNKALL;
	    (void) clearok(stdscr, 1);
	    vupdate();
	}
	break;
#endif /* SIGWINCH */
    case SIGQUIT:
    case SIGTERM:
	xxit(0);
	break;
    default:
	vttymode();
	(void) fprintf(stderr, "Aborted with signal %d\n", signo);
	exit(0);
	break;
    }
}

void vupdate()
/*
 * The display is entirely controlled by this routine,
 * which means that this routine may get pretty snarled.
 */
{
    int	    	i;
    char	vlinbuf[VBUFLEN];
#ifndef CURSES
    static int	oldtopline = -1;	/* topline() on last vupdate() */
#endif /* CURSES */

#ifdef SIGWINCH
    UPDATING++;
#endif /* SIGWINCH */

    if (vcheckin())
	return;

    /* first, update the text area */
    if (vstat != V_STATLINE)
    {
#ifndef CURSES
	/*
	 * if we haven't moved to a new article, we may get some benefit
	 * from trying to scroll up pieces of the old screen. Note that
	 * this is strictly an optimization.
	 */
	if (vstat == V_PAGEMOVE)
	    ushift(ARTWIN, ARTWIN + (LINES - 2) - 1, topline() - oldtopline);
#endif /* CURSES */

	/* now clear the text area */
	for (i = ARTWIN ; i < ARTWIN + (LINES - 2) ; i++)
	{
	    (void) move(i, 0);
	    (void) clrtoeol();
	}

	for (i = 0 ; i < (LINES - 2); i++)
	{
#ifdef A_UNDERLINE
	    char *cp;
	    bool uline;
#endif /* A_UNDERLINE */

	    /* generate the appropriate text line */
	    if ((*textfmt)(i, vlinbuf) == FAIL)
		continue;

#ifdef A_UNDERLINE	/* on V7/BSD curses you lose...sorry about that! */
	    /*
	     * do as few mvaddstr() calls as possible to emit the
	     * highlights indicated by ULINE flagging
	     */
	    uline = FALSE;
	    for (cp = vlinbuf; *cp; cp++)
		uline = uline || *cp == UL_ON || *cp == UL_OFF;

	    if (uline)
	    {
		(void) move(ARTWIN + i, 0);
		for (cp = vlinbuf; *cp; cp++)
		{
		    if (*cp == UL_ON)
			(void) attron(A_UNDERLINE);
		    else if (*cp == UL_OFF)
			(void) attroff(A_UNDERLINE);
		    else
			(void) addch(*cp);
		}
	    }
	    else
#endif /* A_UNDERLINE */
		(void) mvaddstr(ARTWIN + i, 0, vlinbuf);
	}
	vstat = V_STATLINE;
#ifndef CURSES
	oldtopline = topline();
#endif /* CURSES */
    }

    /* now update the status area */

    /* the status line itself */
    (*statfmt)(prompt, vlinbuf);
    (void) standout();
    (void) mvaddstr(PRLINE, 0, prompt);
    (void) clrtoeol();
    if (vlinbuf[0])
	(void) mvaddstr(PRLINE, COLS - strlen(vlinbuf) - 1, vlinbuf);

    /* center section is reserved for mail and time display */
#ifdef SHOWTIME
    if (!dclock.clockoff)
	(void) mvaddstr(PRLINE, (COLS - strlen(dclock.time))/2, dclock.time);
#endif /* SHOWTIME */
#ifdef CHECKMAIL
    if (!dclock.clockoff && dclock.mailstate)
    {
	(void) mvaddstr(PRLINE,
		(COLS - 4)/3, dclock.mailstate == M_NEWMAIL ? "MAIL" : "mail");
	if (dclock.mailstate == M_NEWMAIL)
	    (void) beep();
    }
#endif /* CHECKMAIL */

    (void) standend();

    /* and the messages line */
    if (dclock.justsaved)
	(void) strcat(secpr, " (newsrc just saved)");
    secpr[COLS - 1] = '\0';	/* insurance... */
    (void) mvaddstr(SPLINE, 0, secpr);
    (void) clrtoeol();

    /* reposition the cursor to indicate what we're doing */
    if (curflag == CURWAIT)
	(void) move(PRLINE, strlen(prompt));
    else if (curflag == CURHOME)
	(void) move(0, 0);
    if (!vcheckin())
	(void) refresh();

#ifdef SIGWINCH
    UPDATING=0;
    if (WINCH) { /* window changed while updating screen */
	WINCH = 0;
	if (getsize() != FAIL)
	    (void) bsetsize((LINES - 2), COLS);
    }
#endif /* SIGWINCH */
}

bool vkexpand(cgetf, cungetf, buf)
/* expand k[0-9] %-construct into the given buffer */
int (*cgetf)();		/* key character of escape */
int (*cungetf)();	/* how to push back characters */
char *buf;		/* buffer to expand escapes into */
{
#ifdef RNESCAPES
    char kstring[3];

    if ((kstring[0] = (*cgetf)()) != 'k')
	(*cungetf)(kstring[0]);
    else if (!isdigit(kstring[1] = (*cgetf)()))
    {
	(*cungetf)(kstring[1]);
	(*cungetf)(kstring[0]);
    }
    else
    {
	char *bufptr = buf;	/* some machines can't address the stack */
#undef KEYPAD			/* won't work while I/O is through vtermio.c */
#ifndef KEYPAD
	extern char *tgetstr();

	kstring[3] = '\0';
	(void) tgetstr(kstring, &bufptr);
#else
	bufptr[1] = '\0';
	bufptr[0] = KEY_F(ctoi(kstring[1]));
#endif /* KEYPAD */
	return(TRUE);
    }
#endif /* RNESCAPES */
    
    return(FALSE);
}

catch_t xxit(status)
int    status;
{
    vttymode();
    exit(status);
}

/* visual.c ends here */
