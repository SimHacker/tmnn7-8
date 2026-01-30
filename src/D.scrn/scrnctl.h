/* scrnctl.h -- interface to the virtual-terminal handler code */

extern int COLS;	/* number of columns on the terminal */
extern int LINES;	/* number of rows on the terminal */

/* these entry points act like their curses(3) counterparts */
extern int addch();	/* output character at current location */
extern int addstr();	/* output string at current location */
extern int attroff();	/* turn off given attribute mode */
extern int attron();	/* turn on given attribute mode */
extern int beep();	/* generate a beep to the terminal */
extern int clearok();	/* force screen refresh */
extern int clrtoeol();	/* clear the current line to right of the cursor */
extern int endwin();	/* exit screen mode */
extern int erase();	/* clear the screen */
extern int initscr();	/* set up for screen mode */
extern bool isendwin();	/* return TRUE if out of window mode */
extern int move();	/* move to given row and column */
extern int mvaddstr();	/* output string at specified location */
extern int refresh();	/* re-enter screen mode and refresh */

/* highlight masks */
#define A_UNDERLINE	0x01    /* invokes underline mode */
#define A_STANDOUT	0x02	/* invokes standout mode (us. reverse video) */

#define standout()	attron(A_STANDOUT)
#define standend()	attroff(A_STANDOUT)

/* these don't have exact curses(3) equivalents */
extern void ushift();	/* scroll region upwards (optimization only) */
#ifdef SIGWIND
extern int getsize();	/* restore window status after interrupt */
#endif /* SIGWIND */

/* scrnctl.h ends here */
