/*************************************************************************

NAME
   nextmsg.c -- functions for walking through the USENET spool directories

SYNOPSIS
   #include "active.h"
   #include "nextmsg.h"

   void initmsg(reread, reverse)	-- re-initialize traversal mode
   bool reread, reverse;

   int nextmsg(reread, reverse)		-- go to next message
   bool reread, reverse;

   int nextgroup(reread, reverse)	-- go to next group
   bool reread, reverse;

   int gotogrp(grpname)		-- go to a named group
   char *grpname;

   int gotomsg(num)		-- go to a given message
   nart_t num;

   place *tellmsg(place)	-- save a placemark
   place_t *place;

   int seekmsg(place)		-- seek back to a placemark
   place_t *place;

   bool visible(reread)		-- tell if current msg is within traverse
   bool reread;

DESCRIPTION
   This module defines functions which assist in traversing the USENET spool
directories. These may be local to the machine the code runs on (the usual
case) or accessed through an nntp server over Internet (in which case the
differences are hidden by code in rdactive.c which this calls).

   The kind of message to be selected (new or both old and new) is
controlled by the argument 'reread' (TRUE to see all messages, FALSE to see
only new ones). The direction of movement is controlled by the argument
'reverse' (TRUE to move backwards, FALSE to move forwards).

   A call to initmsg() resets you to whatever the right first group is for
the direction (controlled by 'reverse') in which you are moving.

   The nextmsg() function finds the next message of the selected type,
entering a new group if necessary. The groups are traversed in the order
given by the in-core active table (if sortactive() has been executed this
may differ from the active file order). Within a group, messages are found
sequentially, lowest number first. The function returns SUCCEED until it
can get no more messages; it then returns FAIL.

   The nextgroup() function may be used to skip the rest of the messages in
the current group. It returns SUCCEED, unless there is no next group, in
which case it returns FAIL and the current group is left unchanged. Note that
after a nextgroup() the current message number information is invalid until
another nextmsg() or gotomsg() is done.

   The gotogrp() function may be used to go to a named group. It returns
SUCCEED, unless there is no such group, in which case it returns FAIL.
Note that after a gotogrp() the current message number information is invalid
until another nextmsg() or gotomsg() is done. A NULL argument to gotogrp()
sets the message file generation code to use the name of the current group.

   The gotomsg() function may be used to seek a given message in the
current group. It returns SUCCEED if the message number is active (this
does not guarantee that the message file exists).

   The tellmsg() and seekmsg() functions allow you to save placemarks
(message locations) and seek back to a previous placemark.

   The following macros are defined:

msgnum()    -- return the current message number
msgfile()   -- return ptr to pathname of the current message

   The reverse() macro may be called to move backwards. If called a second
time we start moving forwards again. If we started off moving backwards the
reverse is true. 

   The public variable verbose may be set to control how chatty this module
gets about what it's doing. There is a test mode for this code in newsdb.

SEE ALSO
   {rd|wr}active.c  -- interface to the active-group data
   {rd|wr}newsrc.c  -- interface to the .newsrc file data
   rdhistory.c -- interface to the article-history data

AUTHOR
   Eric S. Raymond
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

**************************************************************************/
/*LINTLIBRARY*/
#include "news.h"
#ifdef DEBUG
#include "vio.h"
#endif /* DEBUG */
#include "active.h"
#include "newsrc.h"
#include "nextmsg.h"

#define V_NEXTMSG	6	/* min debug level to enable chatty msgs */

extern void vdelay();

void initmsg(reread, reverse)
/* reset everything so we start from the first message */
bool	reread, reverse;
{
    ngrewind(!reverse);
    (void) nextgroup(reread, reverse);
    if (reverse)
	active.article.m_number++;
    else
	active.article.m_number--;
}

int gotogrp(gname)
/* skip to the given group */
char	*gname;
{
    if (gname != (char *)NULL && ngselect(ngfind(gname)) == (group_t *)NULL)
	return(FAIL);

#ifdef EACHUPDATE
    (void) rcupdate(ngactive());	/* update min/max art #s & bitmap */
#endif /* NGUPDATE */

    return(SUCCEED);
}

int nextgroup(reread, reverse)
/* go to the next newsgroup */
bool	reread, reverse;
{
    group_t  *oldgrp = ngactive();

    for (;;)
    {
	if (reverse ? !ngback() : !ngnext())    /* no more newsgroups, */
	{
	    ngselect(oldgrp);
#ifdef DEBUG
	    if (verbose >= V_NEXTMSG)
	    {
		msg0("No groups left");
		vdelay();
	    }
#endif /* DEBUG */
	    return(FAIL);		    /*   tell caller about it */
	}
	else if (ngmax() < ngmin())	    /* if no msgs in this group, */
	{
#ifdef DEBUG
	    if (verbose >= V_NEXTMSG)
	    {
		msg1("No articles in %s", ngname());
		vdelay();
	    }
#endif /* DEBUG */
	    continue;
	}
	else if (ngunread() == 0 && !reread)	/* if no unread msgs */
	{
#ifdef DEBUG
	    if (verbose >= V_NEXTMSG)
	    {
		msg1("No unread articles in %s", ngname());
		vdelay();
	    }
#endif /* DEBUG */
	    continue;
	}
	else if (rcsubsc(ngactive()))		/* we do subscribe to it */
	    break;
    }

    (void) gotogrp((char *)NULL);
    active.article.m_number = (reverse ? ngmax() : ngmin());
    return(SUCCEED);
}

int gotomsg(num)
/* set the msgfilename and status bits for a given message */
nart_t num;
{
    if ((num < 0) || (num >= ngmin() && num <= ngmax()))
    {
	active.article.m_number = num;

#ifdef DEBUG
	if (verbose >= V_NEXTMSG)
	{
	    msg2("Group %s, msg %d", ngname(), num);
	    vdelay();
	}
#endif /* DEBUG */
	return(SUCCEED);
    }
    else
	return(FAIL);
}

int nextmsg(reread, reverse)
/* keep fetching messages till we get a valid one */
bool	reread, reverse;
{
    do {
	/* if no more msgs in this direction, must go to next newsgroup */
	if (reverse ? (active.article.m_number <= ngmin()) : (active.article.m_number >= ngmax()))
	{
	    if (nextgroup(reread, reverse) == FAIL)
		return(FAIL);
	}
	else if (reverse)
	    active.article.m_number--;
	else
	    active.article.m_number++;

	(void) fflush(stdout);	/* necessary, but I don't know why */
    } while
	(!reread && getbit(active.article.m_number, active.article.m_group));

    return(gotomsg(active.article.m_number));
}

place_t *tellmsg(place)
/* save a placemark */
place_t *place;
{
    static place_t	myloc;

    if (place == (place_t *)NULL)
	place = &myloc;

    place->m_group = ngactive();
    place->m_number = msgnum();

    return(place);
}

int seekmsg(place)
/* seek back to a placemark */
place_t *place;
{
    if (place->m_group == (group_t *)NULL)
	return(FAIL);

    active.article.m_group = place->m_group;
    (void) gotogrp((char *)NULL);
    return(gotomsg(place->m_number));
}

bool visible(reread)
bool reread;
{
    return(reread || !getbit(active.article.m_number, ngactive()));
}

/* nextmsg.c ends here */
