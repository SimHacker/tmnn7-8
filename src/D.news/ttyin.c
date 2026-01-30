/*****************************************************************************

NAME
   ttyin.c -- line-oriented command-getter functions

SYNOPSIS
   #include "news.h"
   #include "vio.h"

   void vsetidle(func)	-- set idler function
   void (*func)();

   char vgetc()		-- get next character from command line

   void vungetch(c)	-- push a character back to the command queue
   char c;

   void vsetbuf(ptr)	-- set alternate buffer
   char *ptr;

   void venqueue(c)	-- queue up a character for the command parser
   int c;

   void vclearin()	-- clear the input queue

   void vdelay()	-- delay so a message can be read

   void vttymode()	-- go to normal tty mode

   void vcrtmode(ref)	-- go to screen mode
   bool ref;

   char *vgetline(prompt)	-- get a line of data
   char *prompt;

   char *vgetcmd(pr)	-- fetch a line of commands into the input buffer
   char *prompt;

   void vhome()		-- home cursor (no-op)

   vclear()		-- clear screen (no-op)

DESCRIPTION
   This module provides a common interface manager for interfaces that
do line-oriented output. It does one interpretation; the sequence ESC-<x>
is returned as ALT-x.

   The only function with unobvious semantics is vsetbuf(). This is used
to set the interface to grab characters from a buffer other than the standard
private buffer. The buffer pointer gets reset to default by a vgetcmd() or
vclearin(). The main use anticipated for it is for moving the command buffer
pointer past text that has already been parsed.

AUTHOR
   Eric S. Raymond
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

*****************************************************************************/
/*LINTLIBRARY*/
#include "libport.h"		/* for stdio.h, ctype.h and time_t */
#include "vio.h"

#ifndef private
#define private static
#endif /* private */
#define public

#define NO_IDLER	(void(*)())NULL

private char	vcmdbuf[VBUFLEN];	/* common input buffer */
private char	*vbufptr;		/* pointer to current input buffer */
private char	*cmd = vcmdbuf;		/* pointer to current command char  */
private bool	restart;		/* TRUE if setjmp on sigjmp valid */
private jmp_buf	sigjmpbuf;		/* for processing signal interrupts */
private int	vsigcaught;		/* have we caught a signal */
private void	(*idler)() = NO_IDLER;	/* the idler function pointer */

forward static char *vgetclin();

void vsetidle(hook)
/* set an idler function to execute during keyboard waits */
void	(*hook)();
{
    idler = hook;
}

int vgetch()
/* get another character from the buffer */
{
    return(*cmd ? *cmd++ : EOF);
}

int vgetc()
/* get character */
{
    /*
     * this exists as a dummy only because screen-oriented I/O managers
     * recognize a difference between echoed and non-echoed input
     */
    return(vgetch());
}

void vungetch(c)
/* push a char back to the buffer */
char	c;
{
    if (cmd > vbufptr)
	*--cmd = c;
    else
    {
	register char *cp;

	for (cp = vbufptr + strlen(vbufptr); cp >= vbufptr; cp--)
	    cp[1] = cp[0];
	vbufptr[0] = c;
    }
}

void vsetbuf(ptr)
/* set alternate buffer pointer */
char *ptr;
{
    cmd = vbufptr = ptr;
}

void venqueue(c)
/* get another character from the buffer */
char	c;
{
    *cmd++ = c;
}

void vclearin()
/* clear the input buffer */
{
    *(cmd = vcmdbuf) = '\0';
}

/* ARGSUSED0 */
char *vgetline(prompt)
/* get a line of input for the generic command interpreter */
char	*prompt;
{
    static char	lbuf[VBUFLEN];
    char	*tp;
    int		cc;

    lbuf[0] = '\0';
    for (tp = lbuf; (cc = vgetch()) != EOF && cc != '\n'; tp++) {
	if (cc != ' ' && cc != '\t')
		*tp = cc;
	else --tp;
    }
    *tp = '\0';
    return(lbuf);
}

char *vgetcmd(prompt)
/* get and parse a command and return the digested form to the user */
char *prompt;
{
    register char *sp, *tp;

    if (vgetclin(prompt) == (char *)NULL)
	return((char *)NULL);
    cmd = vbufptr = vcmdbuf;

#ifndef lint	/* this saves us from *incredible* hassles with lint */
#ifdef MACROS
    /*
     * Accept chars via vgetch() till we get a recognized macro or
     * something that can't be a prefix of any macro, and enqueue
     * the results for vgetc().
     */
    vmacro();
#endif /* MACROS */
#endif /* lint */

    /* perform escape interpretation */
    for (sp = tp = vbufptr; *sp; sp++)
	if (*sp == ESC)
	    *tp++ = ALT(*++sp);
	else
	    *tp++ = *sp;
    *tp = '\0';

    return(vbufptr);
}

/* ARGSUSED0 */
void vttymode()
/* restore tty to blocking mode, disable idler */
{
#ifdef USG
    int	oflags = fcntl(0, F_GETFL, 0);

    (void) fcntl(0, F_SETFL, oflags &= ~O_NDELAY);
#endif /* USG */
}

/* ARGSUSED0 */
void vcrtmode(f)
bool f;
/* put the tty in non-blocking mode, so the idler will work */
{
#ifdef USG
    int	oflags = fcntl(0, F_GETFL, 0);

    (void) fcntl(0, F_SETFL, oflags |= O_NDELAY);
#endif /* USG */
}

/* ARGSUSED0 */
void vdelay()		{    /* no-op in cooked mode */	    }
void vhome()		{    /* no-op in cooked mode */	    }
void vclear()		{    /* no-op in non-screen modes */}

/* from here down is all stuff for interactive line input */

/*
 * Routine to catch signals while getting a user command line.
 */
private catch_t catchsig(sig)
int sig;
{
#if !defined(BSD4_2) && !defined(SYSV3)
    /*
     * Older UNIX systems reset caught signals to SIG_DFL.
     * This bad design requires that the trap be set again here.
     * Unfortunately, if the signal recurs before the trap is set,
     * the program will die.
     */
    (void) signal(sig, SIGCAST(catchsig));
#endif /* !defined(BSD4_2) && !defined(SYSV3) */
    vsigcaught = sig;
    (void) fflush(stdout);
#ifdef SIGCONT
    if (sig == SIGCONT)
	vupdate();
    else
#endif /* SIGCONT */
	(void) putchar('\n');
    if (sig != SIGINT && restart)
	longjmp(sigjmpbuf, 1);
}

private char *vgetclin(prompt)
char *prompt;
/*
 * this is the code that gets a command line from the user;
 * its private interrupt-catcher follows.
 */
{
    /* we use setjmp(), so don't declare any of these register;
     * some systems don't restore register variables properly on longjmp()
     */
    int	ccount, icount;
    catch_t (*ointr)();
#ifdef	SIGCONT
    catch_t (*ocont)();
#endif

    for (;;)
    {
	(void) setjmp(sigjmpbuf);

	restart = TRUE;
	vsigcaught = 0;

	if (prompt)
	    (void) fputs(prompt, stdout);
	(void) fflush(stdout);

	/*
	 * now grab the actual command; we play some games to set up a
	 * temporary interrupt catcher during the get
	 */
	ointr = signal(SIGINT, SIGCAST(catchsig));
#ifdef SIGCONT
	ocont = signal(SIGCONT, SIGCAST(catchsig));
#endif
#if defined(USG) || defined(FIONREAD) || defined(FIORDCHK)
	/*
	 * USG or Berkeley system -- repeatedly poll the tty, executing the
	 * idler function between tries.
	 */
	for (icount = 0;;)
	{
#ifdef FIONREAD
	    int count = 0;			/* in case FIONREAD fails */

	    (void) ioctl(0, (int)FIONREAD, (char *)&count);
	    if (count)
#endif /* FIONREAD */
#ifdef FIORDCHK
	    if (ioctl(0, (int)FIORDCHK, (char *)NULL))
#endif /* FIORDCHK */
		/* on a USG system, next call won't block til input */
#endif /* defined(USG) || defined(FIONREAD) || defined(FIORDCHK) */
		/* but on a V6 or V7 system, it will */
		if ((ccount = read(0, vcmdbuf, (iolen_t)VBUFLEN)) > 0)
		    break;
#if defined(USG) || defined(FIONREAD) || defined(FIORDCHK)
	    if (idler != NO_IDLER)
		(*idler)(++icount);	/* execute the idler once */
	    (void) sleep(0);	/* cheap way of giving up our process slot */
	}
#endif /* defined(USG) || defined(FIONREAD) || defined(FIORDCHK) */
	if (vsigcaught == SIGINT)
	    return((char *)NULL);
	restart = FALSE;
	(void) signal(SIGINT, SIGCAST(ointr));
#ifdef SIGCONT
	(void) signal(SIGCONT, SIGCAST(ocont));
#endif
	if (ccount != FAIL)
	    break;
	if (!vsigcaught)
	    return(vcmdbuf);
#ifdef SIGCONT
	else if (vsigcaught != SIGCONT)
#endif
	    (void) printf("\n");
    }

    vcmdbuf[ccount - 1] = '\0';	/* strip trailing '\n' */
    return(vcmdbuf);
}

#ifdef SIGTSTP
int onstop(signo)
/* handle incoming stop signal during sub-shell execution */
int signo;
{
	(void) signal(signo, SIGCAST(SIG_DFL));
#ifdef BSD4_2
	(void) sigblock(sigmask(SIGALRM)|sigmask(SIGINT));
	(void) sigsetmask(sigblock(0) & ~sigmask(signo));
#else /* BSD4_1 */
	(void) alarm((unsigned) 0);
#endif /* BSD4_1 */
	(void) kill(0, signo);	/* stop here until continued */

	(void) signal(signo, SIGCAST(onstop));
#ifdef BSD4_2
	(void) sigsetmask(sigblock(0) & ~(sigmask(SIGALRM)|sigmask(SIGINT)));
#endif /* BSD4_2 */
}
#endif /* SIGSTP */

/* ttyin.c ends here */
