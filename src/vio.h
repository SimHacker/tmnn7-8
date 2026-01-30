/* vio.h -- interactive I/O hooks to interface managers */

#define VBUFLEN		128	/* size of common I/O buffer */

#define ctoi(c)	((c) - '0')	/* OK for ASCII and EBCDIC */

#ifdef CTRL				/* BSD defines this in ttychars.h */
#undef CTRL
#endif
#define CTRL(c)	((c) & 0x1f)
#define ALT(c)	((c) | 0x80)
#define CHR(c)	((c) & 0xff)	/* for defeating sign extension in switch */
#define ESC	0x1b		/* escape, used as meta prefix char on input */

/* these will be defined in any interface manager */
extern void vsetidle();		/* set the interface's idler function */
extern int vgetch();		/* get a char from the command queue */
extern int vgetc();		/* vgetch() with echo/macroexpansion */
extern void vungetch();		/* push a char back to the command queue */
extern void vsetbuf();		/* change the start-of-buffer pointer */
extern char *vgetcmd();		/* grab a command line from input */
extern char *vgetline();	/* get a line from the command queue */
extern void venqueue();		/* append a character to the command queue */
extern void vclearin();		/* clear the input queue */
extern void vttymode();		/* go to tty mode */
extern void vcrtmode();		/* go to screen mode */
extern void vdelay();		/* pause (if in screen mode) */

/* the following entry points are required for cooked (tty-like interfaces */

/* the following entry points are required for screen-like interfaces */
extern void vinit();		/* init text and status-line formatters */
extern void vhome();		/* home cursor (if in screen mode) */
extern void vclear();		/* force low-level refresh */
extern void vupdate();		/* force update of screen */
extern void vuline();		/* wrap buffer with underline marks */

#define vjunk(n) vstat = (n)	/* tell I/O mgr what screen parts are new */ 
#define V_STATLINE	0		/* status line only */
#define V_PAGEMOVE	1		/* new text, try scroll optimization */
#define V_JUNKALL	2		/* new text, don't try scrolling */
extern int vstat;

/* these must be defined by the reader */
extern void vcommand();		/* process a command */
extern void vexec();		/* execute commands from a string */
#ifdef RNESCAPES
extern bool vexpand();		/* escape-expansion hook */
#endif /* RNESCAPES */
#ifdef MACROS
extern void vmacro();		/* expand a macro in the input stream */
extern void vmacdef();		/* define a macro */
#endif /* MACROS */

/* an interface that does input macro expansion may define a macro hook */
extern bool vkexpand();		/* the interface's hook function */

/* 
 * clock daemon data (this should be declared volatile when possible)
 */
#define SAVESECS	300	/* standard autosave interval in seconds */

typedef struct
{
    bool justsaved;		/* has the .newsrc just been saved? */
#ifdef SHOWTIME
#define TIMELEN		20	/* length of time string */
    char time[TIMELEN];		/* time appears here */
#endif /* SHOWTIME */
#ifdef CHECKMAIL
    int	mailstate;		/* user's mail status */
#define M_NOMAIL	0		/* no mail waiting */
#define M_NEWMAIL	1		/* new mail just came in */
#define M_WAITING	2		/* mail came in recently */
#endif /* CHECKMAIL */
    int clockoff;		/* if nz, suppress mail and time checks */
}
cldat_t;
extern cldat_t	dclock;		/* accessible to everybody */

extern void getmailname();	/* compute user mailbox name for mail check */
extern void clockdaemon();	/* update the notification variables */

/* the v[012] macros are defined in portlib.h */

extern int vprint();
#define vprint0(f)		v0(vprint, f)
#define vprint1(f,x1)		v1(vprint, f, x1)
#define vprint2(f,x1,x2)	v2(vprint, f, x1, x2)

extern int msg();
#define msg0(f)			v0(msg, f)
#define msg1(f, x1)		v1(msg, f, x1)
#define msg2(f, x1, x2)		v2(msg, f, x1, x2)
#define msg3(f, x1, x2, x3)	v3(msg, f, x1, x2, x3)
#define msg4(f, x1, x2, x3, x4)	v4(msg, f, x1, x2, x3, x4)

/* vio.h ends here */
