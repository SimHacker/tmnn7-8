/**************************************************************************

NAME
   scrnctl.c - virtual terminal handler

SYNOPSIS
   See scrnctl.h

DESCRIPTION
   This module provides screen handling similar to the System V curses(3)
package for the USENET software. It requires the xerror() abort function,
the ttycl.c tty mode control package, and the vtermio.c functions

   The special getsize() entry point puts the size of the terminal or window
in LINES and COLUMNS; it is intended to be called from window signal handlers.
Presently it knows about the conventions used by 4.3BSD and the window system
on the UNIX PC (AT&T 7300/3B1).

WARNING
   If you happen to be running a compiler with 16-bit ints on a BSD system,
the (int) casts on the 2nd args of ioctl() calls put in to pacify lint might
cause lossage by truncation. This could maybe happen with something like
the Green Hills compilers, though I've never seen such a situation.

AUTHORS
   Written by Kenneth Almquist, AGS Computers  (HO 4C601, X7105).
   Modified by Stephen Hemminger, to use TERMCAP (without curses)
   Revised for the new era (TMN netnews) by Eric S. Raymond
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

*****************************************************************************/
/*LINTLIBRARY*/
#include "libport.h"

#ifdef UNIXPC
#undef CURSES		/* so as to use stuff in shared libraries */
#include <sys/font.h>	/* (sigh) have to keep lint happy */
#include <sys/window.h>	/* we need the struct uwdata */
#undef A_UNDERLINE	/* we *don't* want the tam(3) definition! */
#endif /* UNIXPC */

#include "libscrn.h"

#ifndef private
#define private static
#endif /* private */
#define public

#ifndef CURSES
#include "vtermio.h"
#include "ttyctl.h"
#include "scrnctl.h"

/*
 * These values for MAXPLEN and MAXLLEN are used to dimension arrays
 * that hold strings of relative cursor motions.  The actual arrays that
 * are used to hold screen images are malloc'd.
 */
#define MAXPLEN 90
#define MAXLLEN 160

#define BOTLINE (LINES - 1)
#define DIRTY 01

/* terminal escape sequences from termcap */
#define HO _tstr[0]		/* home */
#define CL _tstr[1]		/* clear screen */
#define CD _tstr[2]		/* clear to end of screen */
#define CE _tstr[3]		/* clear to end of line */
#define xUP _tstr[4]		/* up one line */
#define DO _tstr[5]		/* down one line */
#define US _tstr[6]		/* underline */
#define UE _tstr[7]		/* underline end */
#define BT _tstr[8]		/* backtab */
#define xBC _tstr[9]		/* backspace */
#define AL _tstr[10]		/* insert line */
#define DL _tstr[11]		/* delete line */
#define CM _tstr[12]		/* cursor move */
#define CH _tstr[13]		/* cursor horizontal move */
#define CV _tstr[14]		/* cursor vertical move */
#define CS _tstr[15]		/* scrolling region */
#define SF _tstr[16]		/* scroll forwards */
#define SR _tstr[17]		/* scroll backwards */
#define TI _tstr[18]		/* start cursor mode */
#define TE _tstr[19]		/* end cursor mode */
#define TA _tstr[20]		/* tab char (if not \t) */
#define CR _tstr[21]		/* carriage return (if not \r) */
#define xPC _tstr[22]		/* for reading pad character */
#define SO _tstr[23]		/* for standout start */
#define SE _tstr[24]		/* for standout end */

public char PC;			/* pad character */
public char *BC;		/* external variable for tgoto() */
public char *UP;		/* ditto */
public int HOlen;		/* length of HO string */

private char sname[] = "hoclcdceupdousuebtbcaldlcmchcvcssfsrtitetacrpcsose";
private char *_tstr[25];

/* terminal flags */
#define BS _tflg[0]		/* can backspace */
#define AM _tflg[1]		/* has auto margins */
#define XN _tflg[2]		/* no newline after wrap */
#define RET !_tflg[3]		/* has carriage return */
#define NS _tflg[4]		/* has SF (scroll forward) */
#define PT _tflg[5]		/* has tabs */
#define XT _tflg[6]		/* tabs are destructive */
public int	GT = 1;		/* tab stops on terminal are set */

private char bname[] = "bsamxnncnsptxt";
private char _tflg[7];

#ifndef lint
extern char *tgoto(), *tgetstr();
#else	/* can't always find llib-ltermcap.ln to lint against */
char *tgoto(st, a, b) char *st; int a, b; {return(st + b - a);}
/*ARGSUSED1*/
char *tgetstr(st, a) char *st, **a; {return(st);}
void tputs(st, m, f) char *st; int m; int (*f)(); {(*f)(st, m);}
int tgetnum(st) char *st; {return(st[0]);}
/*ARGSUSED1*/
int tgetflag(st) char *st; {return(st[0]);}
int tgetent(st, a) char *st, *a; {return(st[0]+a[0]);}
#endif /* lint */

/* Constants accessable by user */
public int     LINES;		/* number of lines on screen */
public int     COLS;		/* width of screen */

struct line {
    int     len;		/* length of the line */
    char    flags;		/* state, including whether out of date */
    char    *l;			/* characters on line */
    char    *h;			/* highlights on line */
};

private int	iswindon;	/* nz when window mode is enabled */
private int     hasscroll;	/* scrolling type, 0 == no scrolling */
private int     _junked = 1;
private struct line *_virt;	/* how we want the screen to look */
private struct line *_actual;	/* how it actually looks */
private int     _row, _col;
private int     _srow, _scol;
private int     _highlight = 0;
private int     _curjunked;
private int     _dir = 1;
private int	_topshift, _botshift;
private int	_shift;
private int	_scratched;

/*
 * Move routines
 * 	first compute direct cursor address string and cost
 *	then relative motion string and cost,
 *	then home then relative and cost
 *	choose smallest and do it.
 *
 *	The plod stuff is to build the strings (with padding) then decide
 */
private char *plodstr;		/* current location in relmove string */

private int plodput(c)
{
    *plodstr++ = c;
    return(OK);		/* so tput* can call it */
}

private int _vmove(orow, nrow)
{
    char direct[128];
    char *saveplod = plodstr;

    if (CV) {
	plodstr = direct;
	tputs(tgoto(CV, nrow, nrow), 0, plodput);
	*plodstr = '\0';
	plodstr = saveplod;
    }
    if (orow > nrow) {		/* cursor up */
	if (! UP)
	    return(ERR);
	while (orow > nrow) {
	    tputs(UP, 1, plodput);
	    orow--;
	}
    }
    while (orow < nrow) {		/* cursor down */
	if (DO)
	    tputs(DO, 1, plodput);
	else
	    *plodstr++ = '\n';
	orow++;
    }
    if (CV && plodstr - saveplod >= strlen(direct)) {
	register char *p;
	plodstr = saveplod;
	for (p = direct ; *plodstr = *p++ ; plodstr++)
		continue;
    }
    return(OK);
}

/* FIXME: speedup 1-char horiz moves:  print the char that's there. */
/* FIXME: avoid funniness if cm works. */
/* FIXME: Avoid setul(0) if cursor motion OK in standout mode (XM?) */

private void _amove(row, col)
/* force an actual move to given row and column */
int	row, col;
{
    char direct[20];
    char rel[MAXPLEN*10 + MAXLLEN*10];    /* longest move is full screen */
    char ho[MAXPLEN*10 + MAXLLEN*10];
    int cost, newcost;
    register char *movstr;

    if (row == _srow && col == _scol && _curjunked == 0)
	return;
 
    cost = 999;
    if (CM) {
	plodstr = direct;
	tputs(tgoto(CM, col, row), 0, plodput);
	cost = plodstr - direct;
	movstr = direct;
    }
    if (_curjunked == 0) {
	plodstr = rel;
	if (_vmove(_srow, row) >= 0
		&& _hmove(_scol, col, row) >= 0
		&& (newcost = plodstr - rel) < cost) {
	    cost = newcost;
	    movstr = rel;
	}
    }
    if (cost > HOlen) {	/* is it worth calculating */
	plodstr = ho;
	tputs(HO, 0, plodput);
	if (_vmove(0, row) >= 0
		&& _hmove(0, col, row) >= 0
		&& (newcost = plodstr - ho) < cost) {
	    cost = newcost;
	    movstr = ho;
	}
    }

    if (cost < 999)
	while (--cost >= 0) {
	    vputc(*movstr++);
    }
    _srow = row, _scol = col;
    _curjunked = 0;
}

/* non-move internals start here */

private void _sclear()
{
    register struct line   *lp;

    tputs(CL, 0, vputc);
    _srow = _scol = 0;
    for (lp = _actual; lp < &_actual[LINES]; lp++) {
	lp->len = 0;
    }
    for (lp = _virt; lp < &_virt[LINES]; lp++) {
	if (lp->len != 0)
	    lp->flags |= DIRTY;
    }
}

private void _aputc(c)
{
    if (AM && _scol == COLS - 1 && _srow == LINES - 1)
	    return;
    vputc(c);
    if (++_scol >= COLS) {
	if (! AM) {
	    _scol--;
	} else  if (XN) {
	    _curjunked++;
	} else {
	    _scol = 0;
	    ++_srow;
	}
    }
}

private void _sethl(row, col)
/* set the highlight indicated by current row and column */
int	row, col;
{
    register struct line   *vp;
    static int		   uline = 0;	/* is underlining currently on? */
    static int		   standon = 0;	/* is standout currently on? */

    vp = &_virt[row];
    if (vp->len >= col && (vp->h[col] & A_UNDERLINE)) {
	if (uline == 0 && US != (char *)NULL) {
	    tputs(US, 1, vputc);
	    uline = 1;
	}
    }
    else {
	if (uline && UE != (char *)NULL) {
	    tputs(UE, 1, vputc);
	    uline = 0;
	}
    }

    if (vp->len >= col && (vp->h[col] & A_STANDOUT)) {
	if (standon == 0 && SO != (char *)NULL) {
	    tputs(SO, 1, vputc);
	    standon = 1;
	}
    }
    else {
	if (standon != 0 && SE != (char *)NULL) {
	    tputs(SE, 1, vputc);
	    standon = 0;
	}
    }
}

private void _clrtoeol(row, col)
int	row, col;
{
    register struct line *lp = &_actual[row];
    register int i;

    if (CE && lp->len > col + 1) {
	_amove(row, col);
	tputs(CE, 1, vputc);
    } else {
	for (i = col ; i < lp->len ; i++) {
	    if (lp->l[i] != ' ') {
		_amove(row, i);
		_sethl(row, i);
		_aputc(' ');
	    }
	}
    }
    lp->len = col;
}

private void _fixlines()
{
    register struct line   *lp;
    register char  *p;
    register int    i;

    for (i = 0; i < LINES; i++) {
	lp = &_virt[i];
	if (lp->flags & DIRTY) {
	    for (p = &lp->l[lp->len]; --p >= lp->l && *p == ' ';)
		    continue;
	    lp->len = (int) (p - lp->l) + 1;
	    if (lp->len == _actual[i].len && strncmp(lp->l, _actual[i].l, lp->len) == 0)
		lp->flags &= ~DIRTY;
	}
    }
}


/*
 * Consider clearing the line before overwriting it.
 * We always clear a line if it has underlined characters in it
 * because these can cause problems.  Otherwise decide whether
 * that will decrease the number of characters to change.  This
 * routine could probably be simplified with no great loss.
 */

private void _ckclrlin(i)
{
    int     eval;
    int     len;
    int     first;
    register struct line   *vp, *ap;
    register int    j;

    if (!CE)
	return;
    ap = &_actual[i];
    vp = &_virt[i];
    len = ap->len;
    eval = -strlen(CE);
    if (len > vp->len)
    {
	len = vp->len;
	eval = 0;
    }
    for (j = 0; j < len && vp->l[j] == ap->l[j] && vp->h[j] == ap->h[j]; j++)
	continue;
    if (j == len)
	return;
    first = j;
    while (j < len)
    {
	if (vp->l[j] == ' ')
	{
	    if (ap->l[j] != ' ')
	    {
		while (++j < len && vp->l[j] == ' ' && ap->l[j] != ' ')
		    eval++;
		if (j == len)
		    eval++;
		continue;
	    }
	}
	else
	{
	    if (vp->l[j] == ap->l[j])
	    {
		while (++j < len && vp->l[j]==ap->l[j] && vp->h[j]==ap->h[j])
		    eval--;
		continue;
	    }
	}
	j++;
    }
    if (US)
    {
	for (j = 0 ; j < ap->len ; j++)
	{
	    if (ap->h[j] & A_UNDERLINE)
	    {
		eval = 999;
		if (first > j)
		    first = j;
		break;
	    }
	}
    }
    for (j = first; --j >= 0;)
	if (vp->l[j] != ' ')
	    break;
    if (j < 0)
	first = 0;
    if (eval > 0)
    {
	_amove(i, first);
	tputs(CE, 0, vputc);
	_actual[i].len = first;
    }
}

private int _hmove(ocol, ncol, row)
{
    char direct[128];
    char ret[MAXLLEN*10];
    char *saveplod = plodstr;
    char *movstr;
    int cost, newcost;

    cost = 999;
    if (CH) {
	plodstr = direct;
	tputs(tgoto(CH, ncol, ncol), 0, plodput);
	cost = plodstr - direct;
	movstr = direct;
	plodstr = saveplod;
    }
    if (RET && ocol > ncol) {	/* consider doing carriage return */
	plodstr = ret;
	if (CR)
	    tputs(CR, 1, plodput);
	else
	    *plodstr++ = '\r';
	if (_relhmove(0, ncol, row) == OK && (newcost = plodstr-ret) < cost) {
	    cost = newcost;
	    movstr = ret;
	}
	plodstr = saveplod;
    }
    if (_relhmove(ocol, ncol, row) == ERR) {
	if (cost == 999)
	    return(ERR);
	goto copy;
    }
    if (plodstr - saveplod > cost) {
copy:
	plodstr = saveplod;
	    while (--cost >= 0)
		*plodstr++ = *movstr++;
    }
    return(OK);
}

private int _relhmove(ocol, ncol, row)
{
    int tab;

    if (ocol < ncol && PT && GT) {	/* tab (nondestructive) */
	while ((tab = (ocol + 8) & ~07) <= ncol) {
	    if (TA)
		tputs(TA, 1, plodput);
	    else
		*plodstr++ = '\t';
	    ocol = tab;
	}
	if (tab < COLS && tab - ncol < ncol - ocol) {
	    if (TA)
		tputs(TA, 1, plodput);
	    else
		*plodstr++ = '\t';
	    ocol = tab;
	}
    } else if (BT && GT && ocol > ncol) {	/* backwards tab */
	while ((tab = (ocol - 1) &~ 07) >= ncol) {
	    if (BS && tab == ocol - 1) {
		if (BC)
		    tputs(BC, 1, plodput);
		else
		    *plodstr++ = '\b';
	    } else
		    tputs(BT, 1, plodput);
	    ocol = tab;
	}
	if (ncol - tab + 1 < ocol - ncol) {
	    tputs(BT, 1, plodput);
	    ocol = tab;
	}
    }
    if (ocol > ncol) {			/* cursor left */
	if (!BS)
	    return(ERR);
	while (ocol > ncol) {
	    if (BC != (char *)NULL)
		tputs(BC, 1, plodput);
	    else
		*plodstr++ = '\b';
	    ocol--;
	}
    }
    if (ocol < ncol) {			/* cursor right */
	register struct line *lp = &_actual[row];
	/*
	 * This code doesn't move over underlined characters properly,
	 * but in practice this doesn't seem to matter.
	 */
	while (ocol < ncol) {
	    if (ocol < lp->len)
		*plodstr++ = lp->l[ocol];
	    else
		*plodstr++ = ' ';
	    ocol++;
	}
    }
    return(OK);
}

private int _dshift(top, bot, count)
{
    register    int i;

    if (count >= bot - top || hasscroll < 4) {  /* must have CS or AL/DL */
	_scratched++;
	return 1;
    }
    for (i = bot - count; _actual[i].len == 0; i--)
	if (i == top)
	    return 1;
    for (i = top; i <= bot; i++)
	_virt[i].flags |= DIRTY;
    for (i = bot; i >= top + count; i--) {
	/* FIXME, this should be done by recirculating the pointers */
	register int j;
	j =     _actual[i].len   = _actual[i - count].len;
		_actual[i].flags = _actual[i - count].flags;
	(void) strncpy(_actual[i].l,      _actual[i - count].l, j);
    }
    for (; i >= top; i--)
	_actual[i].len = 0;

    /* can we define scrolling region, and scroll back */
    if (hasscroll != 5) {
	tputs(tgoto(CS, bot, top), 1, vputc);/* define scroll region */
	_curjunked = 1;
	_amove(top, 0);
	for (i = count; --i >= 0;)
	    tputs(SR, 1, vputc);/* scroll back */
	tputs(tgoto(CS, BOTLINE, 0), 1, vputc);
	_curjunked = 1;
    } else {
	_amove(bot - count + 1, 0);
	if (CD && bot == BOTLINE)
	    tputs(CD, 1, vputc);
	else {
	    for (i = count; --i >= 0;)
		tputs(DL, LINES - _srow, vputc);
	}
	_amove(top, 0);
	for (i = count; --i >= 0;)
	    tputs(AL, LINES - _srow, vputc);
    }
    return -1;
}

private int _ushift(top, bot, count)
{
    register int	i;

    if (count >= bot - top || hasscroll == 0) {
	_scratched++;
	return;
    }
    for (i = top + count; _actual[i].len == 0; i++)
	if (i == bot)
	    return;
    if (hasscroll == 1 || hasscroll == 3) {
	/* we cheat and shift the entire screen */
	/* be sure we are shifting more lines into than out of position */
	if ((bot - top + 1) - count <= LINES - (bot - top + 1))
	    return;
	top = 0, bot = BOTLINE;
    }
    for (i = top; i <= bot; i++)
	_virt[i].flags |= DIRTY;
    for (i = top; i <= bot - count; i++) {
	/* FIXME, this should be done by recirculating the pointers */
	register int j;
	j =     _actual[i].len   = _actual[i + count].len;
		_actual[i].flags = _actual[i + count].flags;
	(void) strncpy(_actual[i].l,      _actual[i + count].l, j);
    }
    for (; i <= bot; i++)
	_actual[i].len = 0;

    if (hasscroll != 5) {
	if (top != 0 || bot != BOTLINE) {
	    tputs(tgoto(CS, bot, top), 0, vputc);
	    _curjunked = 1;
	}
	_amove(bot, 0);	/* move to bottom */
	for (i = 0; i < count; i++) {
	    if (SF)		/* scroll forward */
		tputs(SF, 1, vputc);
	    else
		vputc('\n');
	}
	if (top != 0 || bot != BOTLINE) {
	    tputs(tgoto(CS, BOTLINE, 0), 0, vputc);
	    _curjunked = 1;
	}
    } else {
	_amove(top, 0);
	for (i = count; --i >= 0;)
	    tputs(DL, LINES - _srow, vputc);
	if (bot < BOTLINE) {
	    _amove(bot - count + 1, 0);
	    for (i = count; --i >= 0;)
		tputs(AL, LINES - _srow, vputc);
	}
    }
}

/* get strings from termcap */
private void _zap()
{
    static char tstrbuf[1024];
    static char *tp;
    register char  *namp, **sp, *bp;

    tp = tstrbuf;
    sp = _tstr;
    for (namp = sname; *namp; namp += 2) {
	*sp++ = tgetstr(namp, &tp);
    }
    bp = _tflg;
    for (namp = bname; *namp; namp += 2) {
	*bp++ = tgetflag(namp);
    }
}

/* entry point definitions begin here */

public bool isendwin()
{
    return(!iswindon);
}

/*ARGSUSED0*/
public int clearok(scr, flag)
int scr;	/* in a real curses this is a screen pointer */
int flag;
{
    _junked = flag;
    return(OK);
}

/*
 * Tell refresh() to shift lines in region upwards count lines.  Count
 * may be negative.  The virtual image is not shifted; this may change
 * later.  The variable _scratched is set to supress all attempts to
 * shift.
 */

public void ushift(top, bot, count)
{
    if (_scratched)
	return;
    if (_shift != 0 && (_topshift != top || _botshift != bot)) {
	_scratched++;
	return;
    }
    _topshift = top;
    _botshift = bot;
    _shift += count;
}

/*
 * generate a beep on the terminal
 */
public int beep()
{
    vputc('\7');
    return(OK);
}

public int attron(h)
int	h;
{
    _highlight |= h;
    return(OK);
}

public int attroff(h)
int	h;
{
    _highlight &=~ h;
    return(OK);
}

public int move(row, col)
/* set up to do next I/O operation from given location */
int	row, col;
{
    if (row < 0 || row >= LINES || col < 0 || col >= COLS)
	return(ERR);
    _row = row;
    _col = col;
    return(OK);
}

/*
 * Output string at specified location.
 */
public int mvaddstr(row, col, str)
int	row, col;
char	*str;
{
    (void) move(row, col);
    (void) addstr(str);
    return(OK);
}

public int addstr(s)
/* output string at current location */
char   *s;
{
    register char  *p, *q;
    register struct line   *lp;
    register int    col = _col;

    lp = &_virt[_row];

    /* if current line representation ends before column _col, pad it */
    if (lp->len < col) {
	p = &lp->l[lp->len];
	q = &lp->h[lp->len];
	while (lp->len < col) {
	    *p++ = ' ';
	    *q++ = 0;
	    lp->len++;
	}
    }

    /* add the string's characters to current line rep, mark it dirty */
    for (p = s; *p != '\0'; p++) {
	if (*p == '\n') {
	    lp->len = col;
	    lp->flags |= DIRTY;
	    col = 0;
	    if (++_row >= LINES)
		_row = 0;
	    lp = &_virt[_row];
	}
	else {
	    lp->l[col] = *p;
	    lp->h[col] = _highlight;
	    lp->flags |= DIRTY;
	    if (++col >= COLS) {
		lp->len = COLS;
		col = 0;
		if (++_row >= LINES)
		    _row = 0;
		lp = &_virt[_row];
	    }
	}
    }
    if (lp->len <= col)
	lp->len = col;
    _col = col;
    return(OK);
}

public int addch(c)
{
    register struct line   *lp;
    register char  *p, *q;

    lp = &_virt[_row];
    if (lp->len < _col) {
	p = &lp->l[lp->len];
	q = &lp->h[lp->len];
	while (lp->len < _col) {
	    *p++ = ' ';
	    *q++ = 0;
	    lp->len++;
	}
    }
    lp->l[_col] = c;
    lp->h[_col] = _highlight;
    if (lp->len == _col)
	lp->len++;
    if (++_col >= COLS) {
	_col = 0;
	if (++_row >= LINES)
	    _row = 0;
    }
    lp->flags |= DIRTY;
    return(OK);
}

/*ARGSUSED0*/
public int clrtoeol()
/* clear an entire line */
{
    register struct line   *lp;

    if ((lp = &_virt[_row])->len > 0)
    {
	lp->flags |= DIRTY;
	lp->len = _col;
    }
    return(OK);
}

public int erase()
{
    register int    i;

    for (i = 0; i < LINES; i++) {
	_virt[i].len = 0;
	_virt[i].flags |= DIRTY;
    }
    return(OK);
}

public int refresh()
{
    register int	i;
    int j, len;
    register char *p, *q, *r, *s;

    if (!iswindon)
    {
	iswindon = TRUE;
	(void) reset_prog_mode();
	if (TI != (char *)NULL)
	    tputs(TI, 0, vputc);
    }

    if (vcheckin())
	return(OK);
    i = 1;
    if (_junked) {
	_sclear();
	_junked = 0;
    } else if (! _scratched) {
	if (_shift > 0) {
	    _ushift(_topshift, _botshift, _shift);
	} else if (_shift < 0) {
	    i = _dshift(_topshift, _botshift, -_shift);
	} else {
	    i = _dir;
	}
    }
    _dir = i;
    _shift = 0;
    if (vcheckin())
	return(OK);
    _fixlines();
    for (i = _dir > 0 ? 0 : BOTLINE; i >= 0 && i < LINES; i += _dir) {
	if ((_virt[i].flags & DIRTY) == 0)
	    continue;
	_ckclrlin(i);	/* decide whether to do a clear line */
			/* probably should consider cd as well */
	len = _virt[i].len;
	if (_actual[i].len < len)
	    len = _actual[i].len;
	p = _virt[i].l;
	q = _actual[i].l;
	r = _virt[i].h;
	s = _actual[i].h;
	for (j = 0; j < len; j++) {
	    if (*p != *q || *r != *s) {
		_amove(i, j);
		_sethl(i, j);
		_aputc(*p);
		*q = *p;
		*s = *r;
	    }
	    p++, q++, r++, s++;
	}
	len = _virt[i].len;
	if (_actual[i].len > len) {
	    _clrtoeol(i, len);
	}
	else {
	    for (; j < len; j++) {
		if (*p != ' ') {
		    _amove(i, j);
		    _sethl(i, j);
		    _aputc(*p);
		}
		*q++ = *p++;
	    }
	    _actual[i].len = len;
	}
	if (vcheckin())
	    return(OK);
    }
    _dir = 1;
    _amove(_row, _col);
    vflush();			/* flush output buffer */
    _scratched = 0;
    return(OK);
}

/*
 * Initialize termcap strings for later use.
 */
public int initscr()
{
    static char tcbuf[1024];	/* termcap buffer */
    register char  *cp;

#ifdef USG
    struct termio tio;
#else /* !USG */
    struct sgttyb ttyb;
#endif /* !USG */

    if ((cp = getenv("TERM")) == (char *)NULL)
	(void) xerror("TERM not set in environment");

    switch (tgetent(tcbuf, cp)) {
	case 0:
		(void) xerror("Terminal not found in TERMCAP");
	case -1:
		(void) xerror("Can't open /etc/termcap");
	case 1:
		break;
    }

    /* if there's no underline highlight, standout will serve */
    if (US == (char *)NULL || *US == 0)
    {
	US = SO;
	UE = SE;
    }

    /* put current window or tty size in LINES, COLUMNS */
    if (getsize() == FAIL)
	(void) xerror("Can't get screen size");

    _zap();

    if (CL == (char *)NULL)
	(void) xerror("No clear screen defined");

    if (HO == (char *)NULL && CM == (char *)NULL)
	(void) xerror("No home or cursor addressing");
    if (HO)
	HOlen = strlen(HO);
    else
	HOlen = 999;

    PC = xPC ? xPC[0] : 0;
    BC = xBC;
    UP = xUP;

    if (tgetnum("ug") > 0)
	US = UE = (char *)NULL;

    if (XT)		/* Destructive tab code not included */
	PT = 0;		/* to keep things simple */

#ifdef USG
    if (ioctl(0, TCGETA, &tio) == 0)
	GT = tio.c_oflag & TAB3;
#else /* !USG */
    if (ioctl(0, (int)TIOCGETP, (char *)&ttyb) == 0)
	GT = ttyb.sg_flags&XTABS;
#endif /* !USG */

    if (LINES > MAXPLEN)
	LINES = MAXPLEN;
    if (COLS > MAXLLEN) {
	COLS = MAXLLEN;
	AM = XN = 1;
    }

    /* malloc the arrays for the screen display */
    {
	char *thelines, *thehighs;
	int i;
	char *malloc();

	thelines = malloc((unsigned)(2 * LINES * COLS));
	thehighs = malloc((unsigned)(2 * LINES * COLS));
	_virt = (struct line *)malloc(2 * LINES * sizeof (struct line));
	_actual = _virt + LINES;
	for (i = 0; i < LINES; i++) {
	    _virt[i].len = 0;
	    _virt[i].flags = 0;
	    _actual[i].len = 0;
	    _actual[i].flags = 0;
	    _virt[i].l = thelines;
	    thelines += COLS;
	    _virt[i].h = thehighs;
	    thehighs += COLS;
	    _actual[i].l = thelines;
	    thelines += COLS;
	    _actual[i].h = thehighs;
	    thehighs += COLS;
	}
    }

    /* Select article scrolling algorithm.  We prefer scrolling region
       over insert/delete line because it's faster on the HP */
    hasscroll = 0;
    if (!NS) {
	hasscroll = 1;
	if (SR)
	    hasscroll = 3;
	if (CS)
	    hasscroll++;
    }
    if (AL && DL && hasscroll != 4)
	hasscroll = 5;

#ifdef BSD
    wait_foreground();	/* not much sense in doing screen I/O in background */
#endif /* BSD */

    /* now go to ttyctl.c to set up the tty control modes */
    if (def_shell_mode() || def_prog_mode())
	xerror0("Can't get tty modes");
    return(OK);
}

public int endwin()
{
    if (iswindon)
    {
	iswindon = FALSE;
	_amove(BOTLINE, 0);
	vputc('\n');
	if (SE != (char *)NULL)
	    tputs(SE, 0, vputc);
	if (UE != (char *)NULL)
	    tputs(UE, 0, vputc);
	if (TE != (char *)NULL)
	    tputs(TE, 0, vputc);
	vflush();
	(void) reset_shell_mode();
    }
    return(OK);
}
#endif /* CURSES */

public int getsize()
/* update LINES and COLS, return TRUE if window changed size else FALSE  */
{
    /*
     * this avoids having to include curses.h,
     * which creates a definition conflict with sys/window.h on the UNIX
     */
    extern LINES, COLS;

    int	rows = -1, cols = -1;
#if defined(SIGWIND) || defined(SIGWINCH)
#ifdef TIOCWINSZ
    struct winsize ws;

    if(ioctl(1, (int)TIOCGWINSZ, (char *)&ws) == 0) {
	rows = ws.ws_row;
	cols = ws.ws_col;
    }
#endif /* TIOCWINSIZ */
#ifdef UNIXPC
    struct uwdata ws;

    if (ioctl(1, WIOCGETD, &ws) == 0)
    {
	rows = ws.uw_height / ws.uw_vs;
	cols = ws.uw_width / ws.uw_hs;
    }
#endif /* UNIXPC */

    /* if neither variety of ioctl() works, try conventional means */
    if (rows == -1 || cols == -1)
#endif /* SIGWIND, SIGWINCH */
	if ((rows = tgetnum("li")) == -1 || (cols = tgetnum("co")) == -1)
	    return(FAIL);

    if (rows != LINES || cols != COLS)
    {
	if (rows > 0)
	    LINES = rows;
	if (cols > 0)
	    COLS = cols;

#ifndef CURSES
	if (LINES > MAXPLEN)
	    LINES = MAXPLEN;
	if (COLS > MAXLLEN) {
	    COLS = MAXLLEN;
	    AM = XN = 1;
	}
#endif /* CURSES */
	return(TRUE);
    }
    else
	return(FALSE);    
}

/* scrnctl.c ends here */
