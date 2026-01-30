/****************************************************************************

NAME
   browse.c - high-level screen handling for visual news interfaces.

SYNOPSIS
   int ihflag, pflag;

   bool bsetsize(rows, cols)		-- set output chunk size
   int rows, cols;

   insrc_t *readfrom(nsrc, newin)	-- switch to another input stream
   insrc_t *nsrc; char *newin;

   void browseinit(startsrc)		-- initialize source stream
   insrc_t *startsrc;

   void browse(enter)			-- execute interpreter commands
   int (*enter)();

   void browsewrap()			-- deinitialize source streams

   bool textline(i, line)		-- format a text line
   int i; char *line;

   void fliprot()		-- flip-flop us in/out of ROT13 mode

   void nextpage(count)  	-- page forward in current source 
   int count;

   void backpage(count)		-- page us backard in current source
   int count;

   void changeline(count)	-- move count lines in current source
   int count;

DESCRIPTION
   This code supports vnews-style screen handling for interactive news reader
interfaces.
   The major entry point is browse(). It loops forever, displaying the
next screenful of stuff and accepting a command on each iteration, until a
command returns EXIT or QUIT status or an interrupting signal is received.
   You must supply it with two arguments. The first should be an escape
function to be executed on each first entry to an article.

   Somewhere in any runtime that uses this module, there must be a function
called vcommand() that fetches and interprets user commands in the style of
gcmd() (see gcmd.c).
   A function named vgroupline() is also required, a status line formatter
taking as its second argument the name of a buffer to be filled with
information on the current article and/or group. The first argument should be
one of the U_* defines in browse.h.

   Some other entry points are supplied to make writing the command
interpreter easier and seal off the underlying implementation. These include
the readfrom(), nextpage(), backpage(), fliprot() and changeline() functions
and the topline(), gotoline(), and gotohdr() macros (see insrc.h).

   Note that this code will set up the mail checker if CHECKMAIL is defined,
but it does nothing with the clock daemon variables (that's left to the I/O
managers).

SEE ALSO
   readinit.c	-- the newsreader initialization code
   session.c	-- reader trail maintenance
   rfuncs.c	-- common functions used by interactive news readers

BUGS
   The hardwiring of PREFSPACE is grotty. Ideally this module shouldn't know
anything about output formats at all.
   The code will probably die with SIGSEGV if the screen depth or width
increases over their starting value during the run. A try at a more general
solution foundered on malloc() flakiness.

AUTHOR
   Eric S. Raymond
working from an original designed by Ken Almquist.
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

*****************************************************************************/
/*LINTLIBRARY*/
#include "news.h"
#include "vio.h"
#include "header.h"
#include "insrc.h"
#include "active.h"
#include "newsrc.h"
#include "nextmsg.h"
#include "dballoc.h"	/* session.h needs the dbdef_t type */
#include "session.h"
#include "libread.h"
#include "gcmd.h"
#ifdef RECMDS
#include "regexp.h"
#endif /* RECMDS */
#include "rfuncs.h"
#include "browse.h"

/* internal flags in action variable (masks 0xF000 reserved for this) */
#define UPDATE	0x1000	/* force screen update */

#define M_PAGE	-2	/* page move marker */

typedef struct
{
    char	*text;
    nart_t	num;
}
subjline;

typedef struct
{
    char	*text;
    group_t	*grp;
}
grpline;

int ihflag, pflag;    	/* option flags */

/* chunked-input objects for the display modes */
insrc_t article;	/* source object for article text */
#ifdef SUBJFILE
insrc_t subjects;	/* source object for subject list */
#endif /* SUBJFILE */
insrc_t groups;		/* source object for group list */
insrc_t help;		/* source object for on-line help */
insrc_t msghdr;		/* source object for raw current message header */

int least, most;	/* on-page item bounds for list modes */

private char *helpfile;	/* current help file */
#ifdef SUBJFILE
private subjline *subjlst;	/* page buffer for subject mode */
#endif /* SUBJFILE */
private grpline *grplst;	/* page buffer for group list mode */

bool bsetsize(rows, cols)
/* set window sizes for the source objects */
int	rows, cols;
{
    static int	oldrows = 0, oldcols = 0;

    tfsetsize(&article, rows, cols);
#ifdef SUBJFILE
    tfsetsize(&subjects, rows, cols);
#endif /* SUBJFILE */
    tfsetsize(&groups, rows, cols);
    tfsetsize(&help, rows, cols);
    tfsetsize(&msghdr, rows, cols);
    return(rows != oldrows || cols != oldcols);
}

#ifdef SUBJFILE
private void mksubjline(ngrp, num)
/* make a subject line for a given article */
group_t	*ngrp;
nart_t	num;
{
    char	indbuf[BUFLEN];

    /*
     * If there are no subject lines for this group yet, or the group has had
     * new messages added since the last scan, reserve as many blank
     * lines in the tempfile as we might need for subject lines for the whole
     * group. Point the group's subjstart field at the beginning of the new
     * section.
     */
    if (ngrp->rc_subjstart == NO_INDEX || (ngrp->ng_flags & NG_CHANGED))
    {
	nart_t	i;

	ngrp->rc_subjstart = (short) subjects.i_lastlin;
	for (i = ngmin(); i <= ngmax(); i++)
	    (void) tfappend("");
    }

    /* fill in the subject line for the particular article */
    indbuf[0] = '\0';
    tfget(indbuf, ngrp->rc_subjstart+num-ngrp->ng_min, (int)subjects.i_width);
    if (indbuf[0] == '\0')
    {
#ifdef IDEBUG
	if (indexline(ngrp, num, indbuf) == FAIL)
	    msg1("Index failure: %s", indbuf + 1);
#else
	(void)indexline(ngrp, num, indbuf);
#endif /* IDEBUG */
	indbuf[subjects.i_width] = '\0';
	tfput(indbuf, ngrp->rc_subjstart + num - ngrp->ng_min);
#ifdef DEBUG
	(void) fflush(subjects.i_tfp);
#endif /* DEBUG */
    }
}
#endif /* SUBJFILE */

/*VARARGS1*/
private int takefrom(nsrc, newin)
insrc_t	*nsrc;	    /* new input mode */
char	*newin;	    /* new input stream to work from */
{
    FILE	    *sfp;

    /* go to the new mode */
    if (nsrc == ARTICLE)
    {
	/* arrange access to the article text */
	(void) msgtext();

	/* re-initialize the article temp file */
	tfopen(&article, header.h_fp);

	hprint(&header, pflag, vuline);
	if (header.h_intnumlines
	    && article.i_artlines + header.h_intnumlines >= article.i_height)
	{
	    (void) sprintf(bfr, "(%d lines)", header.h_intnumlines);
	    (void) tfappend(bfr);
	}

	tfsetlen(header.h_intnumlines);
	article.i_hdrend = article.i_artlines = tfappend("");
	unset(ROT13);

	if (!ihflag)
	    request(HDRONLY);
    }
#ifdef SUBJFILE
    else if (nsrc == SUBJECTS)
    {
	static nart_t	baseart = 0;
	static	group_t	*oldgrp = (group_t *)NULL;
	int	screenline = 0;

	if (oldgrp != ngactive() || msgnum() < baseart)
	{
	    least = baseart = msgnum();
	    oldgrp = ngactive();
	}
	else
	    least = msgnum() - ((msgnum() - baseart) % subjects.i_height);

	/* generate a screenfull of subject lines */
	for (most=least; screenline<subjects.i_height && most<=ngmax(); most++)
	{
	    /*
	     * First force the line we want to exist in the subject list file;
	     * then snarf the line, putting the prefix flag char where it can
	     * be overwritten by the decorating function
	     */
	    mksubjline(ngactive(), most);
	    tfget(subjlst[screenline].text,
		  ngactive()->rc_subjstart + (most - ngmin()),
		  (int) subjects.i_width);

	    /*
	     * If there really is such a line, we've just created and written
	     * a valid screen line.
	     */
	    if (subjlst[screenline].text[0] != I_NOART)
		subjlst[screenline++].num = most;
	}
	most--;

	/* this is so we can tell which lines are valid */
	while (screenline < subjects.i_height)
	    subjlst[screenline++].num = FAIL;
    }
    else if (nsrc == GROUPS)
    {
	grpline	*gl = grplst;	/* current screen line */

	least = ngactive() - active.newsgroups;

	/* generate a screenfull of group lines */
	for (most=least; gl<grplst+groups.i_height && most<active.ngc; most++)
	{
	    gl->grp = active.newsgroups + most;

	    if ((session.reread || gl->grp->ng_unread)
		&& (rcsubsc(gl->grp) || !(gl->grp->rc_flags & RC_NOTED)))
	    {
		vgroupline(gl->grp, gl->text);
		gl++;
	    }
	}
	most--;

	/* this is so we can tell which lines are valid */
	while (gl < grplst + subjects.i_height)
	    (gl++)->grp = (group_t *)NULL;
    }
#endif /* SUBJFILE */
    else if (nsrc == HELP || nsrc == HEADER)
    {
	char	srcfile[BUFLEN];

	if (nsrc == HELP)
	    (void) strcpy(srcfile, newin);
	else if (nsrc == HEADER)
	    (void) strcpy(srcfile, newin = msgfile());

	/* now hook the input to it, doing appropriate decoration */
	if ((sfp = fopen(srcfile, "r")) == (FILE *)NULL)
	{
	    msg2("Can't read %s file, errno = %d", newin, errno);
	    return(FAIL);
	}
	tfopen(nsrc, sfp);
	while(fgets(bfr, sizeof(bfr), src->i_fp))
	{
	    (void) nstrip(bfr);

	    if (nsrc == HEADER && !isalpha(bfr[0]))
		break;

	    bfr[src->i_width] = '\0';
	    src->i_artlines = tfappend(bfr);
	}

	src->i_nomore = TRUE;
	(void) fclose(src->i_fp);
    }
    return(SUCCEED);
}

/*VARARGS1*/
insrc_t *readfrom(nsrc, newin)
/* switch to another input stream */
insrc_t	*nsrc;	    /* new input mode */
char	*newin;	    /* new input stream to work from */
{
    insrc_t		*oldsrc = src;

    src->i_ustate = action;
    tfswitch(nsrc);
    demand(nsrc->i_ustate | UPDATE | HOLD);
    unset(ROT13);
    if (nsrc == HELP)
    {
	if (helpfile)
	    (void) free(helpfile);
	helpfile = savestr(newin);
    }
    return(oldsrc);
}

void browseinit(startsrc)
/* set up initial conditions for browse */
insrc_t	*startsrc;
{
#ifdef SUBJFILE
    place_t	startmsg;
#ifndef lint
    int	i;

    /* allocate space for the subject list page buffer */
    subjlst = (subjline *)calloc(subjects.i_height,
				 (unsigned int) sizeof(subjline));
    for (i = 0; i < subjects.i_height; i++)
	subjlst[i].text = (char *)calloc(subjects.i_width,
					 (unsigned int) sizeof(char));
#else
    subjlst = (subjline *)NULL;
#endif /* lint */

    tfopen(&subjects, (FILE *)NULL);

    /* initially, we don't have subject span offsets for any group */
    (void) tellmsg(&startmsg);
    ngrewind(TRUE);
    while (ngnext())
	ngactive()->rc_subjstart = NO_INDEX;
    (void) seekmsg(&startmsg);
#endif /* SUBJFILE */

#ifndef lint
    /* allocate space for the group list page buffer */
    grplst = (grpline *)calloc(groups.i_height,
			       (unsigned int) sizeof(grpline));
    for (i = 0; i < groups.i_height; i++)
	grplst[i].text = (char *)calloc(groups.i_width,
					(unsigned int) sizeof(char));
#else
    grplst = (grpline *)NULL;
#endif /* lint */

    if (startsrc != ARTICLE)
	(void) takefrom(startsrc);
    src = startsrc;
    session.action = M_NEXT;

#ifdef CHECKMAIL
    getmailname();
#endif /* CHECKMAIL */
}

private int nextart()
/* move the current location to the next article to view */
{
    /* if this isn't simply a refresh call, search for a good message */
    if (pending(NOMORE) || msgnext() == FAIL)
	return(FALSE);

    /* if the user hung up during the search, clean up */
    return(sigcaught != SIGHUP);
}

void browse(enter)
/* loop reading articles and executing commands until NOMORE is raised */
void	(*enter)();	/* user escape for execution on article entry */
{
    place_t place;			/* last article location looked at */
    group_t *oldgrp = (group_t *)NULL;	/* last group with subjects seen */

    place.m_group = (group_t *)NULL;
    place.m_number = 0;
    
    do {
	vhome();

	/* handle requests to change the current message or mode */
	if (pending(HOLD | HDRONLY))		/* we haven't moved */
	    unset(HOLD | HDRONLY);
	else if (!nextart())
	{
	    request(NOMORE | HOLD);
	    msg0("No more articles");
	    goto doit;	/* go straight to command interpreter */
	}
	else if (session.action != M_PAGE)
	    (*enter)();		/* this might change modes on us */

	/* now update the display for the current location and mode */
	if (tfreading(ARTICLE))
	{
	    if (!AT(place))
	    {
		(void) msgtext();
		(void) takefrom(ARTICLE);
		(void) tellmsg(&place);
	    }
	}
	else if (tfreading(HEADER))
	    (void) takefrom(HEADER);
#ifdef SUBJFILE
	else if (tfreading(SUBJECTS))
	{
	    /* if we moved out of current SUBJECTS section paint new one */
	    if (ngactive() != oldgrp
#ifdef NGUPDATE
				|| ngflag(NG_CHANGED)
#endif /* NGUPDATE */
				|| msgnum() < least || msgnum() > most)
	    {
		(void) takefrom(SUBJECTS);
		oldgrp = ngactive();
#ifdef NGUPDATE
		ngfclear(NG_CHANGED);
#endif /* NGUPDATE */
	    }
	}
#endif /* SUBJFILE */
	else if (tfreading(GROUPS))
	{
	    /* if moved out of current group list piece, paint a new one */
	    if (ngactive() - active.newsgroups < least
			|| ngactive() - active.newsgroups > most)
		(void) takefrom(GROUPS);
	}
	else if (tfreading(HELP))
	    (void) takefrom(HELP, helpfile);

	/* if at article start and we only mean to display a header, do it */
	if (pending(HDRONLY))
	{
	    request(HOLD | UPDATE);
	    session.action = M_PAGE;
	}
	else
	{
	    /* otherwise, grab a section of article and go */
	    if (header.h_intnumlines <= 0 || !tfgrab((int)src->i_height))
		session.action = M_NEXT;
	    else
		session.action = M_PAGE;
#ifdef FEEDBACK
	    if (tfreading(ARTICLE))
		msgrate(F_SEENTEXT);
#endif /* FEEDBACK */
	}

	/* get and interpret a command */
    doit:
	if (pending(HOLD) && !pending(UPDATE))
	    vjunk(V_STATLINE);
	else if (session.action == M_PAGE)
	    vjunk(V_PAGEMOVE);
	else
	    vjunk(V_JUNKALL);
	unset(HOLD | UPDATE);
	vupdate();			/* refresh diplay */
	vcommand(gparse(vgetcmd((char *)NULL)));	/* get and interpret a command */
	vclearin();			/* clear the command buffer */
    } while
	/* now fetch and interpret a command */
	(!sigcaught && !pending(EXIT));
}

#ifdef DEBUG
void browsewrap()
{
    tfclose(&article);
#ifdef SUBJFILE
    tfclose(&subjects);
#endif /* SUBJFILE */
    tfclose(&groups);
    tfclose(&help);
    tfclose(&msghdr);
}
#endif /* DEBUG */

#ifdef ASCII
#define lcaesar(ch, n)	((ch) - 'a' + (n)) % 26 + 'a'
#define ucaesar(ch, n)	((ch) - 'A' + (n)) % 26 + 'A'
#else
static char *miniscule = "abcdefghijklmnopqrstuvxyzabcdefghijklmnopqrstuvxyz";
static char *majuscule = "ABCDEFGHIJKLMNOPQRSTUVXYZABCDEFGHIJKLMNOPQRSTUVWYZ";
#define lcaesar(ch, n)	*(strchr(miniscule, (ch)) + n)
#define ucaesar(ch, n)	*(strchr(majuscule, (ch)) + n)
#endif /* ASCII */

static void rotate13(cp)
/* apply caesar cypher to the given buffer */
char	*cp;
{
    char	ch;

    for (; (ch = *cp) != '\0'; cp++)
	if (islower(ch))
	    *cp = lcaesar(ch, 13);
	else if (isupper(ch))
	    *cp = ucaesar(ch, 13);
}

int textline(i, line)
/* generate the ith line for the text area from current data */
int	i;
char	*line;
{
#ifdef SUBJFILE
    /* if we're in subject mode, mark the current message */
    if (tfreading(SUBJECTS))
    {
	if (subjlst[i].num == FAIL)
	    return(FAIL);
	else
	{
#ifdef DEBUG
	    /* head line with spaces in case we sic a debugger on it */
	    line[0] = ' ';
	    line[1] = ' ';
	    line[PREFSPACE] = '\0';
#endif /* DEBUG */

	    /* snarf the line out of where takefrom() left it */
	    (void) strcpy(line + PREFSPACE, subjlst[i].text);

	    /* and decorate it appropriately */
	    vsubjline(subjlst[i].num, line);
	    return(line[PREFSPACE] != '\0');
	}
    }
#endif /* SUBJFILE */

    if (tfreading(GROUPS))
    {
	if (grplst[i].grp == (group_t *)NULL)
	    return(FAIL);
	else
	{
#ifdef DEBUG
	    /* head line with spaces in case we sic a debugger on it */
	    line[0] = ' ';
	    line[1] = ' ';
	    line[PREFSPACE] = '\0';
#endif /* DEBUG */

	    /* snarf the line out of where takefrom() left it */
	    (void) strcpy(line + PREFSPACE, grplst[i].text);

	    /* and decorate it appropriately */
	    vgroupline(grplst[i].grp, line);
	    return(line[PREFSPACE] != '\0');
	}
    }

    if (tfreading(ARTICLE) || tfreading(HELP) || tfreading(HEADER))
    {
	static int lcount = 0;

	if (i == 0)
	    lcount = tfcount((int)article.i_height);

	if (i >= lcount)
	    return(FAIL);
	else
	{
	    tfget(line, topline() + i, (int)src->i_width);
	    if (pending(ROT13) && topline() + i >= src->i_hdrend)
		rotate13(line);
	    return(TRUE);
	}
    }

    return(FAIL);	/* this should never happen */
}

void backpage(count)
/* page us backard in current source */
int	count;
{
    src->i_topline -= src->i_height * count;
    if (src->i_topline < 0)
	src->i_topline = 0;
    session.action = M_PAGE;
}

void changeline(count)
/* move count lines in current source */
int	count;
{
    src->i_topline += count;
    if (topline() < 0)
	gotoline(0);
    session.action = M_PAGE;
}

void nextpage(count)
/* page us forward in current source */
int	count;
{
    /* changeline(src->i_height * count) doesn't handle digests properly */
    src->i_topline = src->i_botline + src->i_height * (count - 1);
    session.action = M_PAGE;
}

void fliprot()
/* flip us in or out of ROT13, rotating the current screen in the process */
{
    if (pending(ROT13))
	unset(ROT13);
    else
	request(ROT13);
    request(UPDATE | HOLD);
}

void gotohdr()
/* restart the article */
{
    gotoline(0);
    session.action = M_SEEK;
    request(HDRONLY | HOLD);
}

void subjadd()
/* idle function that adds to method indices */
{
    place_t		oldloc;
    insrc_t		*oldmode = src;

    /* avoid grinding forever on missing messages */
    if (header.h_fp == (FILE *)NULL)
	return;

    tfswitch(SUBJECTS);		/* may not be necessary */
    (void) tellmsg(&oldloc);

    /*
     * Run forward from wherever we are now, making subject lines, until
     * we run out of articles to cache this info for. The mksubjline function
     * will skip rewriting non-empty entries.
     *
     * We could optimize this by supporting a this-group-is-up-to-date flag
     * and skipping whole groups at a time.
     */
    if (nextmsg(TRUE, session.reverse) != FAIL)
	mksubjline(ngactive(), msgnum());

    (void) seekmsg(&oldloc);
    tfswitch(oldmode);
}

/* browse.c ends here */
