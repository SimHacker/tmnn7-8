/* libscrn.h -- inclusions for screen libraries */

/* #undef CURSES	/* if your curses doesn't work, uncomment this */

/*
 * Sigh -- some curses implementations aren't good enough.
 * The UNIX-PC version uses a really nasty font for half-bright reverse video.
 * Microsoft XENIX apparently uses a really old BSD version that's broken.
 */
#if defined(CURSES) && (defined(UNIXPC) || defined(M_XENIX))
#undef CURSES
#endif

#ifdef CURSES		/* defined iff we're using native curses */

#undef CURSES		/* curses include file has stuff conditional on this */

#include "curses.h"	/* a mildly munged version of /usr/include/curses.h */

#define CURSES

/*
 * Some BSD things need to be undefined, or they'll collide with ttyctl.c
 * entry points and wreak havoc.
 */
#ifdef nonl
#undef nonl
#endif /* nonl */
#ifdef erasechar
#undef erasechar
#endif /* erasechar */
#ifdef killchar
#undef killchar
#endif /* killchar */
#ifdef baudrate
#undef baudrate
#endif /* baudrate */

extern char intchar();	/* not supported by standard curses(3) */

#ifdef lint
#undef mvaddstr
#define mvaddstr(y,x,s) puts(s)	/* to shut lint up about macro calls */
#endif /* lint */

#else			/* we're using our own curses emulation */

#include "ttyctl.h"
#include "scrnctl.h"

#endif /* !CURSES */

/* libscrn.h ends here */
