/**************************************************************************

NAME
   session.c -- code to access and maintain a trail of article locations

SYNOPSIS
   #include "session.h"

   void msginit(testhdrs);  -- initialize the message queue
   bool (*testhdrs)();

   int msggoto(id);	    -- go to a location by Message-ID
   char	*id;

   int msgtext()   	    -- get access to header & text of given article

   int msgread()	    -- get the next article

   void msgend()	    -- end the session

DESCRIPTION
   This module is where header.c and the article tree data structures (or the
calls used to simulate them via nntp server requests) connect.

   The standard interface functions msg?() and vdelay() are assumed.

   These functions are intended to be used on top of nextart.c to maintain a
trail of articles with allocated header storage. This is useful for anything
that reads news.

   The action of the msgread() function is controlled by the value in the
move slot of the current session record.

   If this value is M_NEXT, it calls the nextmsg() function from nextmsg.c
repeatedly, looking for valid articles selected by the user's options (unless
we're in a backtrack (see below) in which case it undoes one level of the
backtrack). It then logs the new article into the trail. If session.thread
is TRUE, msgread() follows subject threads; if it is false, normal by-receipt
order is used.

   A value of M_SEEK indicates to msgread() that we have already selected the
next article and the current article should be logged into the trail.

   An argument of M_HOLD is like M_SEEK but suppresses creation of a new trail
record. This is useful when msglast() has been called.

   At any time after the first msgread(), header.h_fp contains a file pointer
to the current article. The msgtext() function arranges access to the article
text. The msgread() code will close the article file pointer with msgclose()
at the appropriate times.

   All fetching of article text is done through the function getart(), which
is expected to take three arguments; the address of a place, the address of
a header block, and the address of a filename buffer. The function is expected
to load the header block with header information for the given article,
including the h_fp, h_startoff, h_textoff and h_endoff fields that give the
reader a real handle on the article text.

   The public variable 'verbose' controls its level of garrulity about
unselected or garbled articles.

   If the DEBUG code is enabled and verbose is greater than 2, records of trail
insertions will be emitted; this may be useful to track any bugs that appear
in the queue and backtracking logic. The newsdb tester includes a mode for
exercising this code.

BUGS
   The backtrack code will become confused if you change the value of the
reverse global while at an unseen article, then later backtrack though that
spot.

   The code assumes that dballoc() allocates a contiguous array of trail
records. If we ever implement other access modes for dballoc() some of
this will have to be recoded to use them (and the parent slot in the
trail structure will have to become a magic cookie rather than a pointer).

   The RUNCOUNT option vastly reduces the amount of storage required for
trail records, but the logic implementing it is messy and complicated. Bugs
may lurk there.

SEE ALSO
   {rd|wr}active.c	-- interface to the active-group data
   {rd|wr}newsrc.c	-- interface to the .newsrc file data
   rdhistory.c		-- interface to the article-history data
   nntp.c		-- nntp server support (mimics history facilities)
   nextart.c		-- supplies the nextmsg() this uses to get new articles

AUTHOR
   Eric S. Raymond
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

**************************************************************************/
/*LINTLIBRARY*/
#include "news.h"
#include "vio.h"
#include "header.h"
#include "active.h"
#include "newsrc.h"
#include "history.h"
#include "dballoc.h"
#include "nextmsg.h"
#include "session.h"
#ifdef NEWSFILTER
#include "libfilt.h"
#endif /* NEWSFILTER */

/* verbosity minima for various messages */
#define V_SHOWREJECT	1	/* show which articles are rejected */
#define V_SHOWINSERT	2	/* show trail record insertions */
#define V_SHOWTEXT	3	/* report on msgtext() calls */
#define V_SHOWRUN	4	/* show runcount handling */
#define V_SHOWSEEK	5	/* show all viewpoint-move actions */

session_t session =	/* things other modules must see */
{
    /* initialize the dballoc() allocation block */
    {
	(char *)NULL,		/* no associated file */
	sizeof(msgrec_t[1]),	/* the records are msgrec_t structures */
	50, 25,			/* allocate 50, more in chunks of 25 */
    }
};

/* private per-session state info */
private bool	(*hdrpred)();	/* header test predicate */

void msginit(testhdrs)
bool	(*testhdrs)();
{
    hstread(TRUE);		/* Get the history file */
    (void) rdactive(NULLPRED);	/* Now pick up the active file */
    hdrpred = testhdrs;		/* Make sure we know how to filter articles */
#ifdef NEWSFILTER
    (void) fltinit((char *)NULL, (char *)NULL, VPROTO);
#endif /* NEWSFILTER */
#ifndef lint
    session.cmsg = session.trail = (msgrec_t *) dballoc(&session.tralloc);
#else
    session.news = session.tralloc.recsize;    /* garbage to fool lint */
#endif /* lint */
    session.action = M_NEXT;
}

#ifdef BYTHREADS
private char *nextchild(msgb)
/* get the next followup from a given message block */
msgrec_t	*msgb;
{
    register char    *this, *next = msgb->follow;

    if (next && *next)
    {
	while (*next && *next != '<')
	    next++;
	this = next;
	while (*next && *next != '>')
	    next++;
	if (*next)
	    next++;
	if (*next)
	    *next++ = '\0';
	msgb->follow = next;
	return(this);
    }
    else
	return(msgb->follow = (char *)NULL);
}

private msgrec_t *jumpnext()
/* go to next article in conversation order */
{
    register msgrec_t	*ir, *parent = (msgrec_t *)NULL;
    char		*child = (char *)NULL;

    /*
     * If the current message has no followups left and no parent,
     * do nothing. If it has followups left, seek us to the first and
     * set parent to the current message. If it has no followups left
     * but does have a parent, climb up its ancestor branch until you
     * find one with followups or run out of ancestors. In any case,
     * parent and child are left NULL if no suitable next is found.
     */
    for (ir = session.cmsg; ;)
	if (ir->follow && ir->follow[0])
	{
	    /* try to seek to next followup at this level */
	    while ((child = nextchild(ir)) && msggoto(child) != SUCCEED)
		if (!visible(session.reread))
		    continue;

	    /* at this point, either child == NULL or msggoto succeeded */
	    if (child == (char *)NULL)
		continue;	/* O.K., go look for a parent article */
	    else
	    {
		parent = ir;
		++session.depth;
#ifdef DEBUG
		if (verbose >= V_SHOWSEEK)
		    msg2("jumpnext: seeking to %s, session.depth = %d", 
			 child, session.depth);
#endif /* DEBUG */
		break;
	    }
	}
	else if (ir->parent)	/* no more followups, pop up a level */
	{
	    ir = ir->parent;
	    parent = ir->parent;
	    (void) seekmsg(&ir->loc);
	    if (session.depth > 0)
		--session.depth;
#ifdef DEBUG
	    if (verbose >= V_SHOWSEEK)
		msg1("jumpnext: session.depth decremented to %d", session.depth);
#endif /* DEBUG */
	}
	else		/* we're at top level with no followups */
	    break;

    return(parent);		/* non-NULL only if a jump succeeded */
}
#endif /* BYTHREADS */

private int getmsg()
/*
 * Get the next article, undoing one level of backtrack if needed.
 * A return of FAIL means there are no more articles. A return of
 * TRUE means an article was found and read O.K. A return of FALSE
 * means an article was found but was unreadable or garbled.
 */
{
    register msgrec_t	*parent = (msgrec_t *)NULL;
#ifdef BYTHREADS
    static place_t	backto;
#endif /* BYTHREADS */

    if (msgnum() < ngmin())	/* handle startup, group changes */
    {
	if (nextmsg(session.reread, session.reverse) == FAIL)
	    return(FAIL);
#ifdef BYTHREADS
	(void) tellmsg(&backto);
#endif /* BYTHREADS */
    }
    else if (session.action == M_NEXT || session.action == M_SKIP)
    {
#ifdef BYTHREADS
	if (session.thread)
	{
	    if (msgtext() == FAIL)
		return(FALSE);

	    if ((parent = jumpnext()) == (msgrec_t *)NULL)
	    {
		/*
		 * We're not in a conversation thread. Have we just exited one?
		 * If so, it may be that we skipped forward during it, missing
		 * normal sequential access to some articles. The fix is to
		 * a place-mark (backto) on each sequential move forward. Each
		 * time we exit a conversation, we seek to the last saved
		 * backto.
		 */
		if (session.cmsg->parent != (msgrec_t *)NULL
			&& backto.m_group != (group_t *)NULL)
		    (void) seekmsg(&backto);
		if (nextmsg(session.reread, session.reverse) == FAIL)
		    return(FAIL);
		(void) tellmsg(&backto);
	    }
	}
	else
#endif /* BYTHREADS */
	    if (nextmsg(session.reread, session.reverse) == FAIL)
		return(FAIL);
    }
    else if (session.action == M_SEEK)	/* a seek out of normal order */
    {
	/* assume we've seeked somewhere out of normal sequence */
	session.cmsg += session.backtrack;	/* go to the trail end */
	session.backtrack = 0;
#ifdef BYTHREADS
	session.depth = 0;	/* and zero depth so n, + won't act funny */
#ifdef DEBUG
	if (verbose >= V_SHOWSEEK)
	    msg0("getmsg: session.depth zeroed");
#endif /* DEBUG */
#endif /* BYTHREADS */
    }
    else			/* value M_HOLD, do nothing */
	return(TRUE);

    /* if we're on the same article as before, do nothing */
    if (!AT(session.cmsg->loc))
    {
	if (msgtext() == FAIL || (hdrpred != NULLPRED && !(*hdrpred)()))
	    return(FALSE);

	/* last chance to see if we moved before bumping message count */
	if (session.cmsg->loc.m_group != ngactive())
	    session.ngrp = 1;
	else
	    session.ngrp++;

#ifdef DEBUG
	if (verbose >= V_SHOWINSERT)
	    (void) msg2("session: inserting %s/%ld", ngname(), (long)msgnum());
#endif /* DEBUG */

#ifdef RUNCOUNT
	/* let's avoid allocating a new record if we can */
	if (session.action != M_SEEK && parent == (msgrec_t *)NULL
#ifdef FEEDBACK
			&& session.cmsg->id == (char *)NULL
#endif /* FEEDBACK */
	    )
	{
#ifdef DEBUG
	    if (verbose >= V_SHOWRUN)
		msg1("getmsg: incrementing runcount at %d",
		     session.cmsg - session.trail);
#endif /* DEBUG */
	    session.cmsg->runcount++;
	}
	else
#endif /* RUNCOUNT */
	{
#ifdef DEBUG
	    if (verbose >= V_SHOWINSERT)
		msg1("getmsg: creating new record at %d",
		     session.cmsg - session.trail + 1);
#endif /* DEBUG */
#ifndef lint
	    /* here's where the actual allocation is done */
	    session.cmsg = (msgrec_t *) dballoc(&session.tralloc);
	    /* reallocation may move the base of the trail array */
	    session.trail = (msgrec_t *) session.tralloc.records;
#endif /* lint */
#ifdef RUNCOUNT
	    session.cmsg->runcount = session.backin = 0;
#endif /* RUNCOUNT */
	}
#ifdef FEEDBACK
	session.cmsg->id = (char *)NULL;
#endif /* FEEDBACK */
#ifdef BYTHREADS
	/* pick up followup info for subject-traversal logic */
	session.cmsg->parent = parent;
	if (session.thread && hlnblank(header.h_backrefs))
	    session.cmsg->follow = savestr(header.h_backrefs);
	else
	    session.cmsg->follow = (char *)NULL;
#endif /* BYTHREADS */
	(void) tellmsg(&(session.cmsg->loc));
    }

    return(TRUE);
}

int msggoto(id)
/* goto a article given its ID */
char	*id;
{
    place_t	artl;
    int		status;

    if ((status = hstseek(id, FALSE)) == SUCCEED)
	do {
	     status = hstloc(&artl);
	} while
	    /* this tries to find us a location in the current group */
	    (status == SUCCEED && artl.m_group != ngactive());
    else
	return(status);

    return(seekmsg(&artl));
}

private bool outside(hp)
/* check each reference to see if we've ud'd its children */
hdr_t	*hp;
{
    register char   *this;

    if (hlnblank(hp->h_references)&&(this=strtok(hp->h_references," ")))
    {
	do {
	    if (ckfollow(this))
	    {
		if (verbose >= V_SHOWREJECT)
		{
		    msg2("Skipping %s followup of %s",hp->h_ident,this);
		    vdelay();
		}
		return(TRUE);
	    }
	} while
	    (this = strtok((char *)NULL, ", "));
    }
    return(FALSE);
}

int msgtext()
/* get us access to message text for current article */
{
    static place_t	oldloc;
    int status;

    if (!AT(oldloc))
    {
#ifdef DEBUG
	if (verbose >= V_SHOWTEXT)
	    (void) printf("msgtext: looking for %s/%d\n", ngname(), msgnum());
#endif /* DEBUG */

	/* fetch article into current session/header block */
	if ((status = getart(&active.article, &header, session.text)) < 0)
	{
	    (void) arterr(status, &active.article, &header, bfr);
	    msg1("Fetch failure: %s", bfr);
	    return(FAIL);
	}
	(void) tellmsg(&oldloc);
    }

    return(SUCCEED);
}

int msgread()
/* read the next valid and selected article, header and all */
{
    int		    msgfound;

    for (;;)
    {
	/* now go look for the next article, break out if there isn't one */
	while ((msgfound = getmsg()) != TRUE)
	{
	    if (msgfound == FALSE)		/* article is garbaged */
	    {
		if (nextmsg(session.reread, session.reverse) == FAIL)
		    return(FAIL);
		session.action = M_SEEK;
		continue;
	    }
	    else				/* no more articles? */
	    {
		if (header.h_fp)
		    (void) msgclose(header.h_fp);
		return(FAIL);
	    }
	}

	/*
	 * If we get here, we've found a valid next article
	 */

	if (session.ngrp == 1 && !rcflag(RC_VISITED))
	{
	    rcfset(RC_VISITED);

#ifdef NEWSFILTER
	    if (fltnewsgroup(ngname()) < 0)
		rcfset(RC_UNSEL);
#endif /* NEWSFILTER */
	}

	if (outside(&header))	/* notice if discussion is unsubscribed */
	    continue;

#ifdef NEWSFILTER
	session.interest = fltarticle(header.h_ident, session.text);
#ifdef DEBUG
	if (debug >= V_SHOWREJECT)
	    msg2("msgread: interest score is %d, response: %s",
		 session.interest, fltinfo);
#endif /* DEBUG */
#endif /* NEWSFILTER */

	/* if not selected by user's current options, throw it back */
	if ((session.action >= M_NEXT) && (!rcsubsc(ngactive())
#ifdef NEWSFILTER
			 || session.interest < P_VISIBLE
#endif /* NEWSFILTER */
			 || (hdrpred != NULLPRED && !(*hdrpred)(&header))))
	{
#ifdef DEBUG
	    if (verbose >= V_SHOWREJECT)
	    {
		msg3("Article %s/%ld is not selected, score %d",
		     ngname(), (long)msgnum(), session.interest);
	        vdelay();
	    }
#endif /* DEBUG */
	    continue;
	}

	session.news++;	/* bump the counts of articles seen */
	return(SUCCEED);	/* and go on to display this one */
    }
}

void msgend()
/* end the session */
{
#ifdef NEWSFILTER
    (void) fltbye((char *)NULL);
#endif /* NEWSFILTER */
    hstclose();
    clsactive();
}

/* session.c ends here */
