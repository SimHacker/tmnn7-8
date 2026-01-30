/**************************************************************************

NAME
   reader.c -- code for interaction with the article trail

SYNOPSIS
   #include "session.h"

   int msgnext()	    -- get next article or undo backtracking

   int msglast()	    -- backtrack in the trail of opened articles

   int msgparent()	    -- take us to the parent of the current article

   int msgtell()	    -- save a placemark from the queue

   void msgseek(n)	    -- seek back to a saved placemark
   int n;

   void msgmark(mk, xrefs)  -- mark the current msg (and possibly its xrefs)
   int mk, xrefs;

   void msgrate(rating)	    -- praise, condemn or note the current article
   int rating;

   void msgadd(id)	    -- update in-core entries for groups containing id
   char *id;

   void msgsweep()	    -- end-of-session feedback dispatching

DESCRIPTION
   These are functions for interacting with the session article trail that
are needed by interactive readers only.

   The msgnext() function is msgread() with support for backtracking, automatic
check for new messages, and delay-mark support. This is what you want to use
for the main get-next-article call in interactive readers. It also supports a
new action code, M_SKIP.

   The M_SKIP action is like M_NEXT except that if session.thread articles are
skipped until we come back to the same followup depth as the article where the
skip was begun. This mode is used to skip discussion subtrees.

   A msglast() backtracks us one article. It fails if it runs out of old
articles to go to (thus it will fail if no msgnext() has been done). The
old article becomes the current article accessed by header.h_fp and the
active.c entry points.

   The msgparent() comment is for backtracking up parent chains. It returns
NULL when the current message has no parent or one cannot be found locally.

   The msgmark() function is used to mark articles read (READ) or unread
(UNREAD). The second argument allows you to make this global across all
their cross-references; it may be LOCAL, GLOBAL, or (if the DELAYMARK
compilation switch is on) DELAYED. A DELAYED mark is like a LOCAL mark
but doesn't take effect until the group is done.

   The msgrate() and msgsweep() functions support the reader feedback system;
the former is used for registering feedback on articles. The latter writes
collected feedback to a log file. It also takes whatever actions are necessary
to release the active and history files ( see rdhistory.c and nntp.c for
details).

  The msgadd() function takes an article ID and updates the in-core data for
all groups to which the article is posted. This is useful after posting a
followup.

   If the DEBUG code is enabled and debug is greater than 1, records of trail
insertions will be emitted; this may be useful to track any bugs that appear
in the queue and backtracking logic. The newsdb tester includes a mode for
exercising this code.

BUGS
   The backtrack code will become confused if you change the value of the
reverse global while at an unseen article, then later backtrack though that
spot.

   The RUNCOUNT option vastly reduces the amount of storage required for
trail records, but the logic implementing it is messy and complicated. Bugs
may lurk there.

   Global delayed marks aren't supported yet.

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

#ifdef BYSUBJ
/* #define MATCHDEPTH */
#endif /* BYSUBJ */

/* verbosity minima for various messages */
#define V_SHOWDEPTH	2	/* show depth skips */
#define V_SHOWMARK	5	/* show article-marking actions */
#define V_SHOWBACK	5	/* show backtrack actions verbosely */

private void localmark(markread, loc)
/* set the read mark on a article */
int markread;	/* make this nonzero mark it read */
place_t	*loc;	/* article location */
{
    if (markread)
    {
	if (setbit(loc->m_number, loc->m_group) == TRUE)
	    session.waiting--;
    }
    else
    {
	if (clearbit(loc->m_number, loc->m_group) == TRUE)
	    session.waiting++;
    }
#ifdef DEBUG
    if (verbose >= V_SHOWMARK)
	(void)msg2("msgmark: local-marking %s/%d",ngname(),msgnum());
#endif /* DEBUG */
}

private void globalmark(markread)
/* set the read mark on a article and possibly its cross-references */
int markread;	/* make this READ to mark all read, UNREAD for all unread */
{
#ifdef DEBUG
    /*
     * This is a kluge added so the readers can be tested without having
     * converted the history file from 2.11 format. It means that if the
     * user doesn't opt for subject-following, global marking won't be
     * done either.
     */
    if (session.thread)	/* header will have been fetched already */
#endif /* DEBUG */
	/* skip rest of processing if there are no article references */
	{
	    if (hstseek(header.h_ident, FALSE) == SUCCEED)
	    {
		place_t	place;

		while (hstloc(&place) == SUCCEED)
		    if (!AT(place))
			localmark(markread, &place);
	    }
	}
}

#ifdef DELAYMARK
private void dodelayed(oldgrp)
/* perform delayed marks in the given group */
group_t	*oldgrp;
{
    msgrec_t	*dmsg = session.cmsg;

    while (--dmsg >= session.trail && dmsg->loc.m_group == oldgrp)
	if (dmsg->marktype & DELAYED)
	    localmark(dmsg->marktype, &(dmsg->loc));
    session.ndmarks = 0;
}
#endif /* DELAYMARK */
	    
int msgnext()
/* msgread() with backtrack support, depth checking and wraparound */
{
    int	msgstat;
#ifdef MATCHDEPTH
    int	matchdepth = session.depth;

#ifdef DEBUG
    if (verbose >= V_SHOWDEPTH)
	msg1("msgnext: matchdepth set to %d", matchdepth);
#endif /* DEBUG */
#endif /* MATCHDEPTH */

    for (;;)
    {
	/* mark the previous message appropriately (if there is one) */
	if (session.news && ((session.cmsg->marktype & MKTYPE) & GLOBAL))
	    (void) globalmark(session.cmsg->marktype & MARKON);

	/* first, see if we're undoing a previous backtrack */
	if (session.action > 0 && session.backtrack > 0)
	{
	    int	mstat;

	    do {
		--session.backtrack;
#ifdef RUNCOUNT
		if (session.cmsg->runcount && session.backin)
		{
#ifdef DEBUG
		    if (verbose >= V_SHOWBACK)
			(void)printf("msgnext: undoing backtrack via nextmsg()\n");
#endif /* DEBUG */
		    mstat = nextmsg(session.reread, session.reverse);
		}
		else
#endif /* RUNCOUNT */
		{
#ifdef DEBUG
		    if (verbose >= V_SHOWBACK)
			(void)printf("msgnext: undoing backtrack via seekmsg()\n");
#endif /* DEBUG */
		    ++session.cmsg;
		    mstat = seekmsg(&(session.cmsg->loc));
		}
	    } while
		(mstat == SUCCEED
		 && session.backtrack > 0
		 && !visible(session.reread));
	    return(mstat);
	}
	/* we're not undoing a previous backtrack, find next article */
	else if ((msgstat = msgread()) == SUCCEED)
	{
#ifdef MATCHDEPTH
	    /* if we're on a skip, check that we're at same depth as start */
	    if (session.thread && session.action==M_SKIP && session.depth!=matchdepth)
	    {
		(void) globalmark(READ);
#ifdef DEBUG
		if (verbose >= V_SHOWDEPTH)
		    msg2("msgnext: skipping, matchdepth = %d, depth = %d",
			 matchdepth, session.depth);
#endif /* DEBUG */
		continue;		/* the message will be marked */
	    }
#endif /* MATCHDEPTH */
	    break;				/* article OK, go display it */
	}
	else if (msgstat == FAIL)
	{
#ifndef EACHUPDATE
	    if (isatty(fileno(stdin)))
		msg0("Checking for new articles...");
	    if (rcupdate((group_t *)NULL)
			&& (session.waiting = rccount(session.reread)))
	    {
		if (isatty(fileno(stdin)))
		    msg1("%ld new articles arrived while you were reading",
			 (long)session.waiting);
		initmsg(session.reread, session.reverse);
		session.action = M_NEXT;
		continue;
	    }
	    else		/* we're really done, no more articles */
#endif /* NGUPDATE */
	    {
		if (header.h_fp)
		    (void) msgclose(header.h_fp);
		dodelayed(ngactive());
		return(FAIL);
	    }
	}
    }

    /*
     * If we get here, we have a valid article to display
     */

#ifdef DELAYMARK
    if (session.ngrp == 1)
    {
	/* interpret delayed-mark commands for the group we just left */
	group_t	*oldgrp;

	if (session.cmsg == session.trail)
	  oldgrp = (group_t *)NULL;
	else
	  oldgrp = session.cmsg[-1].loc.m_group;

	if (oldgrp != (group_t *)NULL)
	    dodelayed(oldgrp);
    }
#endif /* DELAYMARK */

    return(TRUE);
}

int msglast()
/* backtrack to the last article we visited */
{
#ifdef RUNCOUNT
    if (session.backtrack < session.cmsg->runcount)
    {
#ifdef TDEBUG
	(void) printf("msglast: backtrack via nextmsg()\n");
#endif /* TDEBUG */
	++session.backtrack; ++session.backin;
	(void) nextmsg(TRUE, !session.reverse);
	return(SUCCEED);
    }
    else
#endif /* RUNCOUNT */
	if (session.trail < session.cmsg)
	{
	    --session.cmsg; ++session.backtrack;
#ifdef TDEBUG
	    (void) printf("msglast: backtrack via seekmsg()\n");
#endif /* TDEBUG */
#ifdef RUNCOUNT
	    session.backin = 0;
#endif /* RUNCOUNT */
	    return(seekmsg(&(session.cmsg->loc)));
	}
	else
	    return(FAIL);
}

#ifdef RUNCOUNT
private void forcenew()
/* we need a real record at this spot, force one to exist */
{
    if (session.backin > 0)	/* we need a real trail record here */
    {
	/*
	 * Allocate a new trail record on the end, then shift everything from
	 * cmsg to end of trail one place to the right. We're left with two
	 * copies of cmsg[0], one at cmsg and one at cmsg + 1.
	 */
	msgrec_t *mp;
#ifndef lint
	msgrec_t *newloc = (msgrec_t *) dballoc(&session.tralloc);
#else
	msgrec_t *newloc = (msgrec_t *)NULL;
#endif

#ifdef TDEBUG
	(void) printf("forcenew: splitting record %d\n",
		      session.cmsg - session.trail);
#endif /* TDEBUG */
	for (mp = newloc; mp >= session.cmsg; mp--)
	{
	    (void) memcpy(mp, mp - 1, sizeof(msgrec_t));
#ifdef BYSUBJ
	    /* correct parent pointers to records after the split */
	    if (mp->parent != (msgrec_t) && mp->parent > session.cmsg + 1)
		mp->parent++;
#endif /* BYSUBJ */
	}

	/* now correct the location and run counts for the two copies */
	(void) tellmsg(&(session.cmsg->loc));
	session.cmsg[0].runcount -= session.backin;
	session.cmsg[1].runcount = session.backin;
    }
}
#endif /* RUNCOUNT */

int msgtell()
/* return a placemark in the message queue, -1 if queue is empty */
{
#ifdef RUNCOUNT
    forcenew();	/* because we need to be able to seek back to this */
#endif /* RUNCOUNT */
    return(dbatell(&session.tralloc) - 1);
}

void msgseek(n)
/* seek to a placemark in the message queue */
int	n;
{
    (void) seekmsg(&(session.trail[n].loc));
    dbaseek(&session.tralloc, n + 1);
}

int msgmark(markread, scope)
/* set the read mark on a article and possibly its cross-references */
int markread;	/* make this READ to mark all read, UNREAD for all unread */
int scope;	/* LOCAL for this group only, GLOBAL for all xrefs */
{
    if ((session.cmsg->marktype = scope) != NOMARK)
    {
#ifdef RUNCOUNT
	/* any kind of message mark other than these needs a real record */
	if (scope & ~(LOCAL | GLOBAL))
	    forcenew();
#endif /* RUNCOUNT */

	/* arrange for any global mark to be applied when next we move */
	if (scope & GLOBAL)
	    if (markread)
		session.cmsg->marktype |= MARKON;
	    else
		session.cmsg->marktype &=~ MARKON;

#ifdef DELAYMARK
	if (scope & DELAYED)
	{
	    if (markread)
		session.ndmarks++;
	    else
		session.ndmarks--;
	}
	else
#endif /* DELAYMARK */
	    localmark(markread, &active.article);
    }
    return(SUCCEED);
}

int msgparent()
/* go to the parent of this message */
{
    char    *this, *next;
    char    refbuf[LBUFLEN];

    if (session.cmsg->parent)
	return(seekmsg(&(session.cmsg->parent->loc)));
    else
    {
	(void) strncpy(refbuf, header.h_references, sizeof(refbuf) - 1);
	refbuf[sizeof(refbuf) - 1] = '\0';
	if (hlnblank(refbuf) && (this = strtok(refbuf, " ")))
	{
	    while (next = strtok((char *)NULL, ", "))
		this = next;
	    return(msggoto(this));
	}
	else
	    return(FAIL);
    }
}

#ifdef FEEDBACK
void msgrate(rating)
int rating;
{
#ifdef RUNCOUNT
    forcenew();
#endif /* RUNCOUNT */

    (void) msgtext();

    session.cmsg->id = savestr(header.h_ident);
    switch(rating)
    {
    case F_SEENTEXT:
	break;
    case F_PRAISE:
	session.cmsg->pro++;
	break;
    case F_CONDEMN:
	session.cmsg->con++;
	break;
    }
}
#endif /* FEEDBACK */

#ifdef NGUPDATE
void msgadd(id)
/* update group_t entries of all groups containing a given ID */
char *id;
{
    place_t	svplace;

    /* now global-mark the article corresponding to the returned ID */
    (void) tellmsg(&svplace);
    if (id && id[0] && hstseek(id, FALSE) == SUCCEED)
    {
	place_t	artloc;

	while (hstloc(&artloc) != FAIL)
	{
	    (void) rcupdate(artloc.m_group);
	    (void) setbit(artloc.m_number, artloc.m_group);
	}
	(void) seekmsg(&svplace);
    }
}
#endif /* NGUPDATE */

void msgsweep()
/* if FEEDBACK is on, write data on seen articles */
{
#ifdef FEEDBACK
    int feedback;
    msgrec_t *mp;

    /* mark the last message appropriately (if there was one) */
    if (session.news)
	(void) globalmark(session.cmsg->marktype & MARKON);

    session.cmsg += session.backtrack;	/* undo all backtracking */
    session.backtrack = 0;		/* not really necessary... */

    /* skip slow file I/O if there are no messages marked text-seen */
    feedback = 0;
    for (mp = session.trail + 1; mp < session.trail + session.tralloc.nextfree; mp++)
	if (mp->id != (char *)NULL)
	    feedback++;
    if (feedback == 0)    /* maybe there's nothing that needs logging */
	return;

    /* drop duplicates out of the session trail */
    for (mp = session.trail+session.tralloc.nextfree; mp > session.trail; mp--)
	if (mp->id != (char *)NULL)
	{
	    msgrec_t *np;

	    /* take only the *last* record in the trail for a given id */
	    for (np = mp; np > session.trail; np--)
	    {
		if (np->id != (char *)NULL && strcmp(mp->id, np->id) == 0)
		{
		    (void) free(np->id);
		    np->id = (char *)NULL;
		}
	    }
	}

    /*
     * We may be updating the feedback file concurrently with other aborting
     * readers -- so use open(2) and write(2) for atomicity
     */
    (void) sprintf(bfr, "%s/feedback", site.libdir);
    if ((feedback = open(bfr, O_WRONLY | O_APPEND)) == FAIL)
	return;

    /* now write a feedback record for each registered article in the trail */
    for (mp = session.trail + 1; mp < session.trail + session.tralloc.nextfree; mp++)
	if (mp->id != (char *)NULL
	    && (session.quiet[0] == '\0'
			|| !ngmatch(mp->loc.m_group->ng_name, session.quiet)))
	{
	    /* the quiet option test protects the privacy of readers */
		(void) sprintf(bfr, "%s %s %s/%ld %d %d\n",
			   username, mp->id,
			   mp->loc.m_group->ng_name, (long)mp->loc.m_number,
			   mp->pro, mp->con);
#ifndef DEBUG
	    (void) write(feedback, bfr, (iolen_t)strlen(bfr));
#else
	    if (write(feedback, bfr, (iolen_t)strlen(bfr)) == FAIL)
	    {
		(void) msg1("feedback write failed, errno %d\n", errno);
		break;
	    }
#endif /* DEBUG */
	}

    (void) close(feedback);
#endif /* FEEDBACK */
}

/* reader.c ends here */
