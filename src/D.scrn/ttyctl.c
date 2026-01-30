/****************************************************************************

NAME
   ttyctl.c -- I/O, interrupt handling and timer functions for vnews.c

SYNOPSIS
   #include "ttyctl.h"

   char erasechar()		-- return current erase character

   char killchar()		-- return current kill character

   char intchar()		-- return current interrupt character

   char werasechar()		-- return current cwerase character

   void flushinp()		-- flush tty input queue

   int cbreak()			-- enter raw or cbreak mode

   int nonl()			-- disable CR/NL processing

   int reset_prog_mode()	-- enter program mode

   int reset_shell_mode()	-- restore normal line discipline

   int def_prog_mode()		-- save current modes as program mode

   int def_shell_mode()		-- save current modes as shell mode

   int nodelay(scr, bf);	-- turn nodelay mode on/off
   int scr, bf;

   int baudrate();		-- return baud rate in bits-per-second

   int draino(n)		-- allow output queue to drain (arg ignored)
   int n;

DESCRIPTION
   A package for control of tty modes modelled on the facilities of System V
curses(3). The intchar() function, which returns the current SIGINT character,
is nonstandard. So is the werase() function, which returns the current word
erase character (BSD only).
   Note that the with-CURSES and without-CURSES versions aren't identical. The
baud rate and special-character-query functions give valid results for cooked
mode only in the without-CURSES version.

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
#include "libscrn.h"	/* system curses.h without the libport.h conflicts */

#ifndef private
#define private static
#endif /* private */
#define public

#if !defined(CURSES) || defined(crmode)
private int trspeed(scode)
/* portable baud-code to bits-per-second translation */
int scode;
{
    switch(scode)
    {
    case B0:	return(0);
    case B50:	return(50);
    case B75:	return(75);
    case B110:	return(110);
    case B134:	return(134);
    case B150:	return(150);
    case B200:	return(200);
    case B300:	return(300);
    case B600:	return(600);
    case B1200:	return(1200);
    case B1800:	return(1800);
    case B2400:	return(2400);
    case B4800:	return(4800);
    case B9600:	return(9600);
#ifdef B19200			/* non-SVID systems may be missing this */
    case B19200:	return(19200);
#endif /* B19200 */
#ifdef B38400			/* non-SVID systems may be missing this */
    case B38400:	return(38400);
#endif /* B38400 */
    default:	return(0);	/* unclear what to do here... */
    }
    /*NOTREACHED*/
}
#endif /* !defined(CURSES) || defined(crmode) */

#ifdef USG	/* tty handling for System III, System V */

#ifdef CURSES	/* a curses library will be linked */

public char intchar()
{
    struct termio ttparms;

    if (ioctl(1, TCGETA, &ttparms) < 0)
	return('\0');
    return(ttparms.c_cc[VINTR]);
}

#ifdef __FOO__		/* we may need this later... */
public int nonl()
{
    struct termio ttparms;

    if (ioctl(1, TCGETA, &ttparms) < 0)
	return(ERR);
    ttparms.c_iflag &=~ (INLCR|ICRNL);
    ttparms.c_oflag &=~ (OPOST);	/* shouldn't really do this here */
    while (ioctl(1, TCSETAF, &ttparms) < 0 && errno == EINTR)
	continue;
    return(OK);
}
#endif /* __FOO__ */

#ifdef	TIOCGLTC
/* It's not impossible for a so-called USG system to support BSD ioctls */
public char werasechar()
{
    struct ltchars oldltchars;

    if (ioctl(1, (int)TIOCGLTC, (char *)&oldltchars) < 0)
	return('\0');
    return(oldltchars.t_werasc);
}
#endif /* TIOGLTC */

#else 	/* no curses library will be linked, we must roll our own */
private struct termio ttcooked, ttraw;

/* trivial functions for environment queries */
public int baudrate() {return(trspeed((int)(ttcooked.c_cflag & CBAUD)));}
public char erasechar() {return(ttcooked.c_cc[VERASE]);}
public char killchar() {return(ttcooked.c_cc[VKILL]);}
public char intchar() {return(ttcooked.c_cc[VINTR]);}

public void flushinp()
/* flush terminal input queue */
{
    (void) ioctl(0, TCFLSH, (char *)0);
}

/*ARSUSED0*/
public int nodelay(scr, bf)
/* turn nodelay mode on/off */
int scr;	/* in a real curses this would be a screen pointer */
int bf;
{
    static int oflags = 0;

    if (bf && !(oflags & O_NDELAY))
	return(fcntl(0, F_SETFL, oflags |= O_NDELAY));
    else if (!bf && (oflags & O_NDELAY))
	return(fcntl(0, F_SETFL, oflags &=~ O_NDELAY));
    else
	return(OK);
}

public int def_shell_mode()
{
     return(ioctl(1, TCGETA, &ttcooked) < 0 ? ERR : OK);
}

public int def_prog_mode()
{
    return(ioctl(1, TCGETA, &ttraw) < 0) ? ERR : OK;
}

public int cbreak()
{
    ttraw.c_iflag &=~ (IGNCR);
    ttraw.c_lflag &=~ (ICANON|ECHO|ECHOE|ECHOK|ECHONL);
    ttraw.c_lflag |=  (NOFLSH);
    ttraw.c_cc[VMIN] = 1;
    ttraw.c_cc[VTIME] = 0;
    (void) reset_prog_mode();
}

public int nonl()
{
    ttraw.c_iflag &=~ (INLCR|ICRNL);
    ttraw.c_oflag &=~ (OPOST);	/* shouldn't really do this here */
    (void) reset_prog_mode();
}

public int reset_prog_mode()
/* enable saved modes for screen use */
{
    while (ioctl(1, TCSETAF, &ttraw) < 0 && errno == EINTR)
	continue;
    return(OK);
}

/*ARGSUSED0*/
public int draino(n)
/* wait for output queue to drain -- argument not implemented */
int	n;
{
   errno = 0;
   while (ioctl(1, TCSETAW, &ttraw) < 0 && errno == EINTR)
	continue;
   return(errno ? ERR : OK);
}

public int reset_shell_mode()
/* restore normal (cooked) modes */
{
    while (ioctl(1, TCSETAF, &ttcooked) < 0 && errno == EINTR)
	continue;
    (void) nodelay(stdscr, FALSE);
    return(OK);
}

#endif /* !CURSES */
#else	/* V7/BSD tty handling starts here */

public void flushinp()
/* flush terminal input queue */
{
#ifdef TIOCFLUSH
    (void) ioctl(0, (int)TIOCFLUSH, (char *)0);
#else
    struct sgttyb tty;
    (void) ioctl(0, (int)TIOCGETP, (char *)&tty);
    (void) ioctl(0, (int)TIOCSETP, (char *)&tty);
#endif /* TIOCFLUSH */
}

/*ARGSUSED0*/
public int draino(n)
/* wait for output queue to drain -- argument not implented */
int	n;
{
#ifdef TIOCDRAIN	/* This ioctl is a local mod on linus */
    return ioctl(1, (int)TIOCDRAIN, (char *)0) ? ERR : OK;
#else
    return(ERR);
#endif
}

#ifdef CURSES
private struct sgttyb ttcooked;

#ifndef crmode
public int cbreak() {return(crmode());}
#endif /* crmode */

public char beep()
/* ring terminal bell */
{
    /* we should really crack the bell or visibell string out of termcap */
    return((char) putchar(0x7f));
}

public int baudrate()
{
    struct sgttyb ttparms;

    if(ioctl(1, (int)TIOCGETP, (char *)&ttparms) < 0)
	return(ERR);
    return(trspeed(ttcooked.sg_ospeed));
}

public int nonl()
{
    struct sgttyb ttyparms;

    /* Do this too.... I'm not sure if it's needed or not. */
    _tty.sg_flags &= ~CRMOD, _pfast = TRUE, stty(_tty_ch, &_tty);

    if (ioctl(1, (int) TIOCGETP, (char *)&ttyparms) < 0)
	return(ERR);
    ttyparms.sg_flags &= ~CRMOD;
    if (ioctl(1, (int) TIOCSETP, (char *)&ttyparms) < 0)
	return(ERR);
   return(OK);
}

public char erasechar()
{ 
    struct sgttyb ttparms;

   if(ioctl(1, (int)TIOCGETP, (char *)&ttparms) < 0)
	return('\0');
    return(ttcooked.sg_erase);
}

public char killchar()
{
    struct sgttyb ttparms;

    if (ioctl(1, (int)TIOCGETP, (char *)&ttparms) < 0)
	return('\0');
    return(ttcooked.sg_kill);
}

public char intchar()
{
    struct tchars otchars;

    if (ioctl(1, (int)TIOCGETC, (char *)&otchars) < 0)
	return('\0');
    return(otchars.t_intrc);
}

#ifdef	TIOCGLTC
public char werasechar()
{
    struct ltchars oldltchars;

    if (ioctl(1, (int)TIOCGLTC, (char *)&oldltchars) < 0)
	return('\0');
    return(oldltchars.t_werasc);
}
#endif /* TIOGLTC */

#else	/* no curses library will be linked, we must roll our own */

private struct sgttyb ttcooked, ttraw;
private struct tchars otchars, ntchars;	/* special characters */
#ifdef TIOCGLTC
private struct ltchars oldltchars, newltchars;
#endif

/* trivial functions for environment queries */
public int baudrate() {return(trspeed(ttcooked.sg_ospeed);}
public char erasechar() {return(ttcooked.sg_erase);}
public char killchar() {return(ttcooked.sg_kill);}
public char intchar() {return(otchars.t_intrc);}
#ifdef	TIOCGLTC
public char werasechar() {return(oldltchars.t_werasc);}
#endif /* TIOGLTC */

public int cbreak()
{
    ttraw.sg_flags &=~ (ECHO|XTABS);
#ifdef CBREAK
    ttraw.sg_flags |= CBREAK;
#else				/* !CBREAK */
    ttraw.sg_flags |= RAW;
#endif				/* !CBREAK */
#ifdef	TIOCGLTC
    newltchars = oldltchars;
    newltchars.t_dsuspc = -1;
#endif
    (void) reset_prog_mode();
}

public int int def_shell_mode()
{
    return (ioctl(1, (int)TIOCGETP, (char *)&ttcooked) < 0
		&& ioctl(1, (int)TIOCGETC, (char *)&otchars) < 0
#ifdef TIOCGLTC
		&& ioctl(1, (int)TIOCGLTC, (char *)&oldltchars) < 0
#endif /* TIOCGLTC */
		) ? ERR : OK;
}

public int def_prog_mode()
{
    if ((ioctl(1, (int)TIOCGETP, (char *)&ttraw) < 0)
#ifdef CBREAK
		&& (ioctl(1, (int)TIOCGETC, (char *)&ntchars) < 0)
#endif /* CBREAK */
#ifdef TIOCGLTC
		&& (ioctl(1, (int)TIOCGLTC, (char *)&newltchars) < 0)
#endif /* TIOCGLTC */
		)
	return(ERR);
    else
	return(OK);
}

public int nonl()
{
    ttraw.sg_flags &=~ (CRMOD);
    (void) reset_prog_mode();
}

public int reset_prog_mode()
/* enable modes for screen use */
{
    while (ioctl(1, (int)TIOCSETN, (char *)&ttraw) < 0 && errno == EINTR)
	    continue;
#ifdef TIOCGLTC
#ifndef lint		/* 4.2BSD lint doesn't like this comparison */
    if (newltchars.t_dsuspc == (char)-1)
#endif /* lint */
	while (ioctl(1, (int)TIOCSLTC, (char *)&newltchars) < 0 && errno == EINTR)
	    continue;
#endif
    return(OK);
}

public int reset_shell_mode()
/* restore tty modes to normal cooked mode */
{
    while (ioctl(1, (int)TIOCSETN, (char *)&ttcooked) < 0 && errno == EINTR)
	continue;
#ifdef TIOCGLTC
#ifndef lint		/* 4.2BSD lint doesn't like this comparison */
    if (newltchars.t_dsuspc == (char)-1)
#endif /* !lint */
	while (ioctl(1, (int)TIOCSLTC, (char*)&oldltchars) < 0 && errno==EINTR)
	    continue;
#endif /* lint */
    return(OK);
}

#endif /* CURSES */
#endif /* USG */

/* ttyctl.c ends here */
