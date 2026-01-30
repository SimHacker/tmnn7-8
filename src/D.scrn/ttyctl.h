/* ttyctl.h -- UNIX-version-independent tty mode control */

#define OK	0
#define ERR	-1
#define stdscr	0	/* this pseudo-curses only supports one screen */

extern void flushinp();
extern int def_prog_mode();
extern int def_shell_mode();
extern int reset_prog_mode();
extern int reset_shell_mode();
extern int cbreak();
extern int nonl();
extern int draino();
extern int baudrate();
#ifdef USG
extern int nodelay();		/* set/clear nodelay mode */
#endif /* USG */

extern char erasechar();
extern char killchar();
extern char intchar();		/* not supported by standard curses(3) */
#ifdef TIOCGLTC
extern char werasechar();	/* not supported by standard curses(3) */
#endif /* TIOCGLTC */

/* ttyctl.h ends here */
