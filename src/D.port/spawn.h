/* spawn.h -- code for subprocess control */

/* shcmd() argument 2 flag masks */
#define FORGRND	0000	/* run process in foreground */
#define CWAIT	0001	/* type "continue?" and wait for return */
#define BAKGRND	0002	/* run process in the background */
#define NOPRIVS	0004	/* ensure child doesn't run suid or sgid anything */

extern int shcmd();
extern int spawn();

#ifndef SHELL
char *SHELL;
#endif

/* spawn.h ends here */
