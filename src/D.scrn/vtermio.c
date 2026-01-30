/****************************************************************************

NAME
   vtermio.c -- I/O, interrupt handling and timer functions for vnews.c

SYNOPSIS
   #include "vtermio.h"

   void vterminit(onchar, onsig, idler)	-- set up the package
   void (*onchar)(), (*onsig)(), (*idler)();

   void vflush()		-- flush pending output to screen

   int vgetch()			-- get a character

   int vputc(c)			-- put a character
   char c;

   int vcheckin()		-- check for pending input

   int vclearin()		-- clear any pending input

   void venqueue(c)		-- enqueue a character onto the input stream
   char c;

   void vungetch(c)		-- push a character back onto the input stream
   char c;

   void vsetbuf(pr)	-- point input functions at alternate input buffer
   char *pr;

   void wait_foreground()	-- (BSD only) spin while in background

DESCRIPTION
   These functions provide graceful queued I/O and interrupt-handling for
visual.c (and possibly visual mail readers in the future). They are
intended to work with scrnctl.c, which uses termcap to provide a subset
of Berkeley curses(3).

   The package is initialized by calling vterminit() with three function
pointer arguments. The first names a function to be called on each actual
character input by vgetc() (unqueueing of pushed-back characters doesn't
count). The second names a function to be called just after every alarm
received. The third names a function to be called on exit or abort.

WARNING
   If you happen to be running a compiler with 16-bit ints on a BSD system,
the (int) casts on the 2nd args of ioctl() calls put in to pacify lint might
cause lossage by truncation. This could maybe happen with something like
the Green Hills compilers, though I've never seen such a situation.

AUTHOR
   Eric S. Raymond
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

*****************************************************************************/
/*LINTLIBRARY*/
#include "libport.h"
#include "libscrn.h"	/* system curses.h with some libport clashes removed */

#ifndef private
#define private static
#endif /* private */
#define public

#define ALARMINT	1	/* alarm tick interval in seconds */

#ifdef SIGTSTP
private int reading;		/* to keep stupid BSD from restarting reads */
jmp_buf intjmp, alrmjmp;
#endif /* SIGTSTP */

#define INBUFSIZ 8
private char inbuf[INBUFSIZ];	/* input buffer */
private int innleft = 0;	/* # of chars in input buffer */
private char *innext = inbuf;	/* next input character */
private char outbuf[BUFSIZ];	/* output buffer */
private int outnleft = BUFSIZ;	/* room left in output buffer */
private char *outnext = outbuf;	/* next space in output buffer */
private int tickflag = 0;	/* set if unprocessed alarm signal */
private int intflag;		/* set if interrupt received */
private void (*update)();	/* called on each real character in */
private catch_t (*onsig)();	/* called on signal events */
private void (*idle)();		/* called during wait on input */
private char cintr;		/* current interrupt character */

/*** initialization ***/

void vterminit(upd, sig, idler)
void (*upd)();
catch_t (*sig)();
void (*idler)(); 
{
    int i, timer();
    catch_t onint(), onalarm();
#ifdef SIGCONT
    forward catch_t oncont();
#endif /* SIGCONT */
#ifdef SIGTSTP
    forward catch_t onstop();
#endif /* SIGTSTP */

    update = upd;
    onsig = sig;
    idle = idler;

    /* redirect all signals to user's signal handler */
    for (i = SIGILL; i <= SIGTERM; i++)
	(void) signal(i, sig);

    /* now put specific hooks on some of them */
    (void) signal(SIGINT, onint);
    (void) signal(SIGALRM, onalarm);
#ifdef SIGTSTP
    (void) signal(SIGSTOP, onstop);
#endif /* SIGTSTP */
#ifdef SIGCONT
    (void) signal(SIGCONT, oncont);
#endif /* SIGCONT */
    (void) alarm(ALARMINT);

    cintr = intchar();
    timer();

#if defined(USG) && !defined(FIORDCHK)
    (void) nodelay(stdscr, TRUE);
#endif /* defined(USG) && !defined(FIORDCHK) */
}

/*** signal handlers ***/

static catch_t onint()
{
#ifdef SIGTSTP
    int dojump = reading;

    reading = FALSE;
#endif /* SIGTSTP */
    (void) signal(SIGINT, SIGCAST(onint));
    (*onsig)(SIGINT);
    (void) flushinp();			/* flush input queue */
    innleft = 0;
#ifdef SIGTSTP
    if (dojump)
	longjmp(intjmp, 1);
#endif /* SIGTSTP */
    intflag++;
}

#ifdef SIGTSTP
static catch_t onstop(signo)
int signo;
{
    /* restore old terminal state */
    (*onsig)(signo);
    (void) signal(signo, SIGCAST(SIG_DFL));
#ifdef BSD4_2
    (void) sigblock(sigmask(SIGALRM)|sigmask(SIGINT));
    (void) sigsetmask(sigblock(0) & ~sigmask(signo));
#else				/* BSD4_1 */
    (void) alarm((unsigned) 0);
#endif				/* BSD4_1 */
    (void) kill(0, signo);	/* stop here until continued */

    (void) signal(signo, SIGCAST(onstop));
}
#endif /* SIGTSTP */

#ifdef SIGCONT
static catch_t oncont(signo)
int signo;
{
    (*onsig)(signo);
#ifdef BSD4_2
    (void) sigsetmask(sigblock(0) & ~(sigmask(SIGALRM)|sigmask(SIGINT)));
#else				/* BSD4_1 */
    timer();
#endif				/* BSD4_1 */
}
#endif /* SIGCONT */

/*
 * Called on alarm signal.
 * Simply sets flag, signal processed later.
 */
static catch_t onalarm()
{
#ifdef SIGTSTP
    int dojump = reading;
#endif /* SIGTSTP */

#if defined(USG)
    (void) signal(SIGALRM, SIGCAST(onalarm));
#endif /* defined(USG) */

#ifdef SIGTSTP
    reading = FALSE;
    if (dojump)
	longjmp(alrmjmp, 1);
#endif /* SIGTSTP */

    tickflag++;
    (void) alarm(ALARMINT);
}

static int timer()
/* process already-received alarm signal (or start clock) */
{
    tickflag = 0;
    (*onsig)(SIGALRM);		/* call the user's timer-tick exit */
}

/* I/O primitives start here */

void vflush()
/* flush the output buffer */
{
    register char *p;
    register int i;
#ifdef BSD4_2
    int mask;
#else
    unsigned oalarm;
#endif

#ifdef BSD4_2
    mask = sigblock(1 << (SIGALRM-1));
#else
    oalarm = alarm((unsigned) 0);
#endif
    for (p = outbuf ; p < outnext ; p += i) {
	if ((i = write(1, p, (iolen_t)(outnext - p))) < 0) {
	    if (errno != EINTR)
		(void) abort();	/* "Can't happen" */
	    i = 0;
	}
    }
    outnleft = BUFSIZ;
    outnext = outbuf;
#ifdef BSD4_2
    (void) sigsetmask(mask);
#else
    (void) alarm(oalarm);
#endif
}

int vgetch()
/* input a character, return it */
{
    /* we use setjmp(), so don't declare any of these register;
     * some systems don't restore register variables properly on longjmp()
     */
    int	c, icount;

lookagain:
    if (--innleft >= 0)	/* if queue is nonempty */
	c = *innext++;	/*	just snarf next char off it */
    else		/* else must get a real one */
    {
	if (tickflag)
	    timer();
	(*update)();		/* update the display */
	icount = 0;
	for (;;)
	{
	    /* new input or timer tick demands that we try the queue again */
	    if (innleft > 0 || tickflag)
		goto lookagain;
	    intflag = 0;
#ifdef SIGTSTP
	    if (setjmp(alrmjmp))
		continue;
	    if (setjmp(intjmp))
		return(cintr);
	    reading = TRUE;
#endif /* SIGTSTP */

#if defined(USG) || defined(FIORDCHK) || defined(FIONREAD)
	    /*
	     * USG or Berkeley system -- repeatedly poll the tty, executing the
	     * idler function between tries.
	     */
	    for (;;)
	    {
#ifdef FIONREAD
		int count = 0;			/* in case FIONREAD fails */

		(void) ioctl(0, (int)FIONREAD, (char *)&count);
		if (count)
#endif /* FIONREAD */
#ifdef FIORDCHK
	 	if (ioctl(0, (int)FIORDCHK, (char *)NULL) > 0)
#endif /* FIORDCHK */
		    /* on a USG system, next call won't block til input */
#endif /* defined(USG) || defined(FIORDCHK) || defined(FIONREAD) */
		    /* but on a V6 or V7 system, it will */
		    if ((innleft = read(0, inbuf, (iolen_t)INBUFSIZ)) > 0)
		    {
			innext = inbuf;
			break;
		    }
#if defined(USG) || defined(FIORDCHK) || defined(FIONREAD)
		(*idle)(++icount);	/* execute the idler once */
		(void) sleep(0);	/* give up our process slot */
	    }
#endif /* defined(USG) || defined(FIORDCHK) || defined(FIONREAD) */
#ifdef SIGTSTP
	    reading = FALSE;
#endif /* SIGTSTP */
	    if (innleft > 0)
		break;
	    if (innleft == 0)
		return(EOF);
	    if (errno != EINTR)
		(void) abort();	/* "Can't happen" */
	    if (intflag)
	    {
		intflag--;
		return(cintr);
	    }
	}
	innleft--;
	c = inbuf[0];
	innext = inbuf;
    }
#ifndef USG
#ifndef CBREAK
    c &= 0177;
    if (c == '\034')		/* FS character */
	(*onsig)(SIGTERM);	/* do graceful exit */
#endif
#endif
    if (c == '\r')
	c = '\n';
    return(c);
}

bool vcheckin()
/* check for terminal input */
{
    /* first, check our buffer */
    if (innleft > 0 || tickflag)
	return(TRUE);

#if defined(USG) || defined(FIORDCHK) || defined(FIONREAD)
    /* we want to know if we can assume that all pending I/O is done */
    if (baudrate() >= 9600)	/* this is fast enough to count on it */
	return(FALSE);

    /* otherwise flush pending output; wait for it if we're slow */
    vflush();
    if (baudrate() <= 300)
	(void) draino(0);

    /* now check the buffer in the tty device */
#if defined(USG) && !defined(FIORDCHK)
    (void) nodelay(stdscr, TRUE);
    if ((innleft = read(0, inbuf, (iolen_t) INBUFSIZ)) > 0)
    {
	innext = inbuf;
	return(TRUE);
    }
#endif /* defined(USG) && !defined(FIORDCHK) */
#if defined(FIONREAD)
    {
	int count = 0;			/* in case FIONREAD fails */

	(void) ioctl(0, (int)FIONREAD, (char *)&count);
	if (count)
	    return(TRUE);
    }
#endif /* defined(FIONREAD) */
#ifdef FIORDCHK
    if (ioctl(0, (int)FIORDCHK, NULL) > 0)
	return(TRUE);
#endif /* FIORDCHK */
#endif /* defined(USG) || defined(FIORDCHK) || defined(FIONREAD) */

    /* if we have no way to check (V6 or V7), just assume we can go */
    return(FALSE);
}

void vclearin()
/* discard any input buffered or presently pending */
{
    innleft = 0;
    innext = inbuf;
    (void) flushinp();
}

void venqueue(c)
/* queue a character onto the input stream */
char	c;
{
    /* to be processed *after* currently waiting input */
    inbuf[innleft++] = c;
}

void vungetch(c)
/* push a character back on the input */
char	c;
{
    if (innext > inbuf)
	*innext-- = c;
    else
    {
	register char *cp;

	for (cp = inbuf + innleft; cp >= inbuf; cp--)
	    cp[1] = cp[0];
	inbuf[0] = c;
    }
    innleft++;
}

/*ARGSUSED0*/
void vsetbuf(pr)
/* point input functions at alternate buffer */
char	*pr;
{
    /*
     * Stub routine, intended to prevent linker from bombing on command
     * interpreters written for cooked I/O interfaces. We'll actually implement
     * this someday as a bunch of pushbacks with code that makes it a no-op
     * when the alternate buffer is empty -- that check will make it safe
     * for raw-mode command interpreters to call.
     */
}

vputc(c)
/* put a character to output */
{
    if (--outnleft < 0) {
	vflush();
	outnleft--;
    }
    *outnext++ = c;
}

#ifdef BSD
public void wait_foreground()
{
#ifndef BSD4_2
    catch_t	oldttin, oldttou, oldstop;
#endif /* BSD4_2 */

#ifdef SIGTSTP
    int getpgrp();
#if defined(BSD4_2) || defined(BSD4_1C)
    int tpgrp;
#else /* BSD4_1 */
    short tpgrp;
#endif /* BSD4_1 */

retry:
#ifdef BSD4_2
    (void) sigblock(sigmask(SIGTSTP)|sigmask(SIGTTIN)|sigmask(SIGTTOU));
#else				/* !BSD4_2 */
    oldstop = signal(SIGTSTP, SIGCAST(SIG_HOLD));
    oldttin = signal(SIGTTIN, SIGCAST(SIG_HOLD));
    oldttou = signal(SIGTTOU, SIGCAST(SIG_HOLD));
#endif				/* !BSD4_2 */
    if (ioctl(2, (int)TIOCGPGRP, (char *)&tpgrp) == SUCCEED)
	if (tpgrp != getpgrp(0)) {	/* not in foreground */
	    (void) signal(SIGTTOU, SIGCAST(SIG_DFL));
#ifdef BSD4_2
	    (void) sigsetmask(sigblock(0) & ~sigmask(SIGTTOU));
#endif				/* BSD4_2 */
	    (void) kill(0, SIGTTOU);
	    /* job stops here waiting for SIGCONT */
	    goto retry;
        }
#ifdef BSD4_2
    (void) sigsetmask(sigblock(0) & ~(sigmask(SIGTSTP)|sigmask(SIGTTIN)|sigmask(SIGTTOU)));
#else
    (void) signal(SIGTTIN, SIGCAST(oldstop));
    (void) signal(SIGTTOU, SIGCAST(oldttin));
    (void) signal(SIGTSTP, SIGCAST(oldttou));
#endif /* BSD4_2 */
#endif /* SIGTSTP */
}
#endif /* BSD */

/* vtermio.c ends here */
