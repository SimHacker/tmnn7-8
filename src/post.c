/***************************************************************************

NAME
   post.c - functions used by inews and rnews.

SYNOPSIS
   #include "post.h"

   char nosend[];	-- list of systems broadcast should not relay to

   void post()		-- post and retransmit an article

DESCRIPTION
   The main sequence of the article-posting function, and its code for
dispatching articles to remote systems, lives here. The single entry point is
post(), which posts the article attached to the current header.

   The post() code calls 3 functions: checkhdr(), insert(), mailtomod() and 
broadcast(). Checkhdr() does sanity checks. Then insert() (see insert.c)
handles the details of linking the new article into each newsgroup directory.
Mailtomod() extracts post addresses of moderated groups and sends the article
to them. The fourth function, broadcast(), transmits the article to all
neighbor systems.

FILES
   TEXT/.tmp/news??????	-- temp file used to store incoming article(s)

BUGS
   The workfiles above have to be on the same file system as TEXT or the
link calls used in tolocal() will fail.
   The nosend[] global is a kluge, but it permits post to be an argument-less
function. This helps post()'s immediate caller to stay clean.

AUTHOR
   Eric S. Raymond
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

*****************************************************************************/
/*LINTLIBRARY*/
#include "news.h"
#include "libpriv.h"
#include "header.h"
#include "dballoc.h"
#include "alist.h"
#include "feeds.h"
#include "active.h"
#include "history.h"
#include "ngprep.h"
#include "post.h"
#include "fascist.h"	/* for getgrplist() declaration */

#ifdef u370
private feed_t target;
#endif /* u370 */

/* verbosity level minima for various messages */
#define V_SHOWHEADERS	7	/* dump headers as they're processed */

/* rnews options processing may set this */
char	nosend[BUFLEN];		/* list of systems not to xmit to */

/* only we need to see these */
private char	*ARTICLE;	/* the article file */

#ifdef ODEBUG
/*
 * Track workfile deletions in order to nail any weird bugs with
 * file or article-filtering bugs.
 */
private int wfrm(f)
char	*f;
{
    log1("Deleting workfile %s", f);
    return(unlink(f));
}

#define unlink(f)	wfrm(f)

#endif /* DEBUG */

void post()
/* post the article described by the current header to the right places */
{
    forward void	mailtomod(), broadcast();
    FILE	*ofp;
    int		hopcount;
    bool	originator;
#ifdef DOXREFS
    char	doxbuf[BUFLEN];
    dest_t	*dst;
#endif /* DOXREFS */
    dest_t	*dest;

    /* Step 0: make sure we have a suitable tempfile handy */
    if (ARTICLE == (char *)NULL)
    {
	/* article file must be in the text directory so links will work */
	(void) sprintf(bfr, "%s/.tmp/newsXXXXXX", site.textdir);
	(void) mktemp(ARTICLE = savestr(bfr));
    }

#ifdef DEBUG
    if (!debug)
    {
#endif /* DEBUG */
	/*
	 * Step 1: if the message has already been seen, toss it
	 * Also patch back-reference lines here.
	 */
	if (hlnblank(header.h_ident))
	    switch (hstseek(header.h_ident, FALSE))
	    {
	    case SUCCEED:
#ifdef HYPERTEXT
		/* snarf new backreferences before discarding this copy */
		if (hlnblank(header.h_backrefs))
		{
		    char	*hf, backrefs[LBUFLEN];

		    (void) strcpy(backrefs, header.h_backrefs);
		    if ((hf = hstfile(header.h_ident)) != (char *)NULL)
			if (mungread(hf, &header) == SUCCEED)
			    if (setadd(backrefs, header.h_backrefs, " ") == 0)
				(void) mungwrite(FALSE);
			    else
			    {
				log1("new backreferences received for %s",
				     header.h_ident);
				hlcpy(header.h_backrefs, backrefs);
				(void) mungwrite(TRUE);
			    }
		}
#endif /* HYPERTEXT */
		log4("dup_art %s dist %s ng %s path %s",
		     header.h_ident, header.h_distribution,
		     header.h_newsgroups,header.h_path);
		return;

	    case CANCELLED:
		log4("can_art %s dist %s ng %s path %s",
		     header.h_ident, header.h_distribution,
		     header.h_newsgroups,header.h_path);
		return;

	    case EXPIRED:
		log4("exp_art %s dist %s ng %s path %s",
		     header.h_ident, header.h_distribution,
		     header.h_newsgroups,header.h_path);
		return;

	    case REFERENCE:
		log1("fetching reference entry for %s", header.h_ident);
		hlcpy(header.h_backrefs, hstattr());
		break;
	    }
#ifdef DEBUG
    }
    else if (verbose >= V_SHOWHEADERS)
	hwrite(&header, stdout, TRUE);
#endif /* DEBUG */


    /*
     * Step 3: Do sanity checks on the header. The 'header' structure
     * is assumed to hold the header of the current message.
     */
    if (checkhdr() == FAIL)
	return;

    /*
     * Step 4: compile the article's Newsgroups line to internal form.
     * This sets up the destinations array to be used by ngaccept().
     */
    ngprepare();
#ifdef DOXREFS
    /*
     * If we've surrendered to bogosity, also generate new article
     * numbers here and create a local Xref header from this data.
     * We stash them in the ng_nextnum member of the group structure
     * so tolocal() doesn't have to know about the destinations array.
     */
    (void) strcpy(doxbuf, site.pathname);
    (void) strcat(doxbuf, " ");
    for (dst = destinations; dst->d_status != D_NOMORE; dst++)
	if (dst->d_status == D_OK)
	{
	/*
	 * Groups with unknown names won't have XREF headers generated
	 * for them, since we don't know yet if they are Flexgroups or
	 * not.  Thus, their article numbers will get generated in
	 * tolocal when it notices that ng_nextnum has not been
	 * initialized.  Sniff, sniff.  No big loss.  This will also
	 * be the case for articles which get dumped to junk.
	 */
	    dst->d_ptr->ng_nextnum = ngnewart(dst->d_ptr);
	    (void) sprintf(doxbuf + strlen(doxbuf),
		       "%s:%ld ",
		       dst->d_ptr->ng_name,
		       (long)dst->d_ptr->ng_nextnum);
	}
    hlcpy(header.h_xref, doxbuf);
#endif /* DOXREFS */

    /*
     * Step 5: now cons the article text with the prepared header.
     * If there were any errors during the write, reject the article
     * and never write a history entry for it, in hopes we'll pick it
     * up from another feed (we certainly don't want to retransmit it!).
     */
    ofp = xfopen(ARTICLE, "w");
    if (rewrite(&header, ofp))
    {
	logerr1("rewrite of %s failed", header.h_ident);
	(void) fclose(ofp);
	(void) unlink(ARTICLE);
	return;
    }
    else
	(void) fclose(ofp);

#ifdef DEBUG
    if (verbose >= V_SHOWHEADERS)
    {
	(void) printf("After preparation:\n");
	hwrite(&header, stdout, TRUE);
    }
#endif /* DEBUG */

    /*
     * Step 6: if the article has no message-id field, give it one now.
     * If a message-ID field is present and is in 3.0 format, crack out the
     * hash value and check it against reality.
     */
    if (originator = hlblank(header.h_ident))
    {
	char	*id = idmake(ARTICLE);

	if (id == NULL)
	{
	    logerr1("couldn't generate new ID for %s", ARTICLE);
	    return;
	}
	else
	{
	    hlcpy(header.h_ident, id);
	    log2("posting %s to %s", id, header.h_newsgroups);
	}
    }
    else
    {
	artid_t	*ip = idparse(header.h_ident);

	if (ip != (artid_t *)NULL && ip->id_hash != hashart(&header))
	    /*
	     * Someday, rewrite the article with inserted notification header
	     * so the *readers* can know that something's amiss...
	     */
	    logerr0("arrived with incorrect hash");
	else
#ifdef OLDSTYLE
	    log2("%s from %s", header.h_ident, header.h_from);
#else
	log4("art %s dist %s ng %s path %s",
	     header.h_ident, header.h_distribution,
	     header.h_newsgroups,header.h_path);
#endif /* OLDSTYLE */
    }

    /*
     * Step 7: if we originated, mail it to the moguls of extracted moderated
     * groups; if the user is the moderator of an addressed group and there's
     * no Approved header, generate one. The 'header' structure is assumed
     * to hold the header of the current message.
     */
#ifdef DEBUG	/* so test batches can exercise the mod. posting code  */
    if (originator || debug)
#else
    if (originator)
#endif /* DEBUG */
	mailtomod(ARTICLE);

#ifdef DEBUG
    if (verbose >= V_SHOWHEADERS)
    {
	(void) printf("After stripping out moderated groups:\n");
	hwrite(&header, stdout, TRUE);
    }
#endif /* DEBUG */

    /*
     * Step 8: insert the article in the local article tree. The 'header'
     * structure is assumed to hold the header of the current message.
     * Mark each destination-array slot holding a local-only group sent.
     */
    if (insert(ARTICLE, originator) == FAIL)
    {
	(void) fclose(ofp);
	(void) unlink(ARTICLE);
	return;
    }

#ifdef DEBUG
    if (verbose >= V_SHOWHEADERS)
    {
	(void) printf("After local insertion:\n");
	hwrite(&header, stdout, TRUE);
    }
#endif /* DEBUG */

    /*
     * Step 9: check that the number of hops in its backpath is less than the
     * defined by the sys file (this optimizes for leaf node systems).
     */
    hopcount = 0;
    for (dest = destinations; dest->d_status != D_NOMORE; dest++)
	if (dest->d_status == D_MAILED)
	    hopcount++;
#ifdef MODEXCLUDE
    /*
     * Broadcast the article only if Step 8 didn't do any sends to
     * moderated groups (i.e. no slots in the destination array have D_MAILED),
     */
    hopcount = hopcount ? S_MANYHOPS : 0;
#else
    /*
     * Broadcast the article if there are any valid or unknown group names
     * in the destinations array that were not handled by a successful
     * mailtomod() in step 8.
     */
    hopcount = (dest - destinations <= hopcount) ? S_MANYHOPS : 0;
#endif /* MODEXCLUDE */
    if (s_maxhops < S_MANYHOPS)
    {
	char	*local;

	for (local = header.h_path; *local != '\0'; local++)
	    if (*local == PATHSEP)
		hopcount++;
    }
    if (s_maxhops >= hopcount)
    {
#ifdef LEAFNODE
	if (hlnblank(header.h_newsgroups) && !originator)
#else
	if (hlnblank(header.h_newsgroups))
#endif /* LEAFNODE */
	{
#ifdef STRIPHDRS
	    /* trash current copy of the article, we'll write a new version */
	    (void) unlink(ARTICLE);

	    /* strip some headers out of the rebroadcast form */
#ifndef HYPERTEXT
	    hlfree(header.h_backrefs);
#endif /* HYPERTEXT */
#ifdef DOXREFS
	    hlfree(header.h_xref);
#endif /* DOXREFS */

	    /* now cons the article text with the modified header */
	    ofp = xfopen(ARTICLE, "w");
	    if (rewrite(&header, ofp))
		logerr1("strip rewrite of %s failed", header.h_ident);
	    (void) fclose(ofp);
#endif /* STRIPHDRS */

	    /* now do the rebroadcast */
	    broadcast(&header, ARTICLE);
	}
    }

    (void) unlink(ARTICLE);
}

private void broadcast(hp, fname)
/* transmit this article to all interested systems */
hdr_t	*hp;		/* header of the message */
char	*fname;		/* file ptr to article text (with header) */
{
    register char *sptr;
#ifndef u370
    feed_t *sys;
#endif
    char sentbuf[LBUFLEN];
    int nsent = 0;
    static char bang[2] = {PATHSEP, 0};
    char	*backpath;

    /* create the head of the broadcast transmission log entry */
    (void) strcpy(sentbuf, hp->h_ident);
    (void) strcat(sentbuf, " sent to ");
    nsent = 0;

    /*
     * Down below we're going to apply the back-path optimization,
     * to check the target site against places the article has already
     * been. But if the article was moderated, a leading segment of the
     * News-Path header may be a fake intended to help stupid mailers get
     * replies back to the submitter. So what we *really* want to do is
     * pick a sitename out of the Approved header, and look for it in
     * the News-Path; if we find it, only sites *after* it are checked.
     *
     * Yes, yes, I know -- it's a horrible kluge that will break if the
     * Approved line is ever in a funny format. But if it breaks, it will
     * break in the direction that *favors* loop elimination. As long as
     * News-Path has to be used for both broadcast optimization and mail
     * replies, it's about the best we can do.
     */
    backpath = hp->h_path;	/* by default, check against the whole path */
    if (hlblank(hp->h_approved))
    {
	char	modsite[NAMELEN];

	/* try to find a moderator site name */
	(void) strcpy(modsite, hp->h_approved);

	/* if the moderator's site ran 3.x, it should be a domainist address */
	if (sptr = strchr(hp->h_approved, '@'))
	{
	    (void) strcpy(modsite, sptr + 1);
	    (void) strtok(modsite, NETCHRS);	/* trash domain suffixes etc */
	    if (sptr = strchr(modsite, ' '))	/* and comments */
		*sptr = '\0';
	}
	else if (sptr = strchr(modsite, PATHSEP))
	    *sptr = '\0';

	/* search the path for the extracted moderator site name */
	(void) strcpy(bfr, hp->h_path);
	sptr = strtok(bfr, bang);
	do {
	    if (strcmp(modsite, sptr) == 0)
	    {
		/* Found it! Set backpath ptr to start with this site */
		backpath += (sptr - bfr);
		break;
	    }
	} while
	    (sptr = strtok((char *)NULL, bang));
    }

    /* loop once per system. */
    s_rewind();
    while ((sys = s_next()) != (feed_t *)NULL)
    {
	/* don't send to yourself */
	if (strncmp(sys->s_name, site.pathname, SNLN) == 0)
	    continue;

	/* don't send to systems specified in -x option */
	if (ngmatch(sys->s_name, nosend))
	    continue;

	/*
	 * Suppress send if any system on the backpath matches the system
	 * in the current feeds file slot. The strcpy() is so we don't trash
	 * the path header. We must use an ngmatch() here to handle multicast
	 * groups properly.
	 */
	(void) strcpy(bfr, backpath);
	sptr = strtok(bfr, bang);
	do {
	    if (ngmatch(sptr, sys->s_name))
		goto skipit;
	} while
	    (sptr = strtok((char *)NULL, bang));

	/*
         * Also suppress send if a system in the path matches a nosend entry,
	 * This is a shaky way to use ngmatch(), but the old code was ugly and
 	 * had damaging side-effects.
	 */
	if (sys->s_nosend)
	{
	    (void) strcpy(bfr, backpath);
	    sptr = strtok(bfr, bang);
	    do {
		if (ngmatch(sptr, sys->s_nosend))
		    goto skipit;
	    } while
		(sptr = strtok((char *)NULL, bang));
	}

	/*
	 * Make sure the article matches target's group/distribution fields.
	 * The < here is important; we want articles with A_ACCEPT *or*
	 * A_JUNKIT status to be passed on.
	 */
	if (ngaccept(sys) < A_ACCEPT)
	    continue;

	/*
	 * Someday, hack in real support for -x option;
	 * we would do a set-subtract of nosend from the
	 * sys->s_name system list and pass that as the first
	 * argument to dispatch().
	 */

	/* now we've found a system to send this article to */
	if (!dispatch(sys->s_name, sys, hp, fname, TRUE))
	    continue;
	if (nsent++)
	    (void) strcat(sentbuf, LISTSEP);
	(void) strcat(sentbuf, sys->s_name);
skipit:;
    }

    if (nsent)
	log0(sentbuf);
}

private int checkhdr()
/* check that the contents of 'header' looks OK */
{
#ifdef FASCIST
    if (hlblank(header.h_ident) && !privileged
	&& !allmatch(header.h_newsgroups, fascist(username)->n_post))
    {
	logerr2("User %s is not authorized to post to one of %s",
		    username, header.h_newsgroups);
	return(FAIL);
    }
#endif /* FASCIST */

    /* bomb out if there's no title on the article */
    if (hlblank(header.h_subject))
    {
	logerr2("No title, article %s from %s", header.h_ident, header.h_from);
	return(FAIL);
    }

    /* make sure we have a valid submittal date */
    header.h_rectime = time((time_t *)NULL);
    if (hlnblank(header.h_postdate))
    {
	if (header.h_posttime < 0)
	{
	    logerr1("Invalid submittal date '%s'", header.h_postdate);
	    return(FAIL);
	}
    }
    else	/* if no submission date field, assign it the receipt date */
    {
	header.h_posttime = header.h_rectime;
	hlcpy(header.h_postdate, arpadate(&header.h_posttime));
	(void) nstrip(header.h_postdate);
    }

    /* here's how to make control messages, since all.ctl is unblessed */
    if (prefix(header.h_subject, "cmsg ") && hlnblank(header.h_ctlmsg))
	hlcpy(header.h_ctlmsg, &header.h_subject[5]);

    return(hlblank(header.h_ident));
}

private void mailtomod(artfile)
/* mail the registered article to group moderators */
char	*artfile;
{
    char	    mailto[LBUFLEN], *via;	/* groups we must mail to */
    dest_t	    *dest;
    register char   *p;

    /* strip out groups that ought to go out by mail */
    mailto[0] = '\0';
    for (dest = destinations; dest->d_status != D_NOMORE; dest++)
    {
	if (dest->d_status != D_OK)
	    continue;
	else
	    ngselect(dest->d_ptr);

#ifdef FUTURE
	/*
	 * Right now, there's no phase previous to the moderated-groups
	 * handling that could drop an address out of the destinations
	 * list. If there ever is such a phase, this will help.
	 */
	if (dest->d_status != D_UNSENT)
	    continue;
#endif /* FUTURE */

	/* if the group is moderated, more checks needed */
	if (ngflag(NG_MODERATED))
	{
	    /* if this group has a moderator, more checks are needed */
	    if (dest->d_moderator != (char *)NULL)
	    {
		/* we may need the domainist form of the user's name */
		(void) strcpy(bfr, header.h_from);
		if (p = strpbrk(bfr, " ("))
		    *p = '\0';

		/*
		 * if the user is the moderator or a member of a group with
		 * moderator privileges, proceed
		 */
		if (ngmatch(
			    (dest->d_moderator[0] == GRPESCAPE)
					? getgrplist(username) : bfr,
			    dest->d_moderator))
		{
		    /*
		     * add an "Approved:" header and *don't* mark the
		     * address processed -- so it will get broadcast
		     */
		    if (hlnblank(header.h_approved))
			hlcpy(header.h_approved, header.h_from);
		}
		else	/* we want to mail to the moderator */
		{
		    (void) strcat(mailto, dest->d_moderator);
		    (void) strcat(mailto, " ");
		    dest->d_status = D_MAILED;
		}
	    }
	    else if (ngflag(NG_LOCAL))		    /* the group is local */
	    {
		logerr1("can't find moderator for local group %s", ngname());
		dest->d_status = D_CANTMAIL;
	    }
	    else if ((via = newsattr("backbone", BACKBONE)) == (char *)NULL)
	    {
		logerr1("can't mail submission to %s (no backbone link)",
			    ngname());
		dest->d_status = D_CANTMAIL;
	    }
	    else    /* we can mail it out through the backbone */
	    {
		/*
		 * mung the newsgroup name so it's in proper form for an
		 * alias, i.e. no dots
		 */
		(void) strcpy(p = bfr, ngname());
		while (*++p)
		    if (*p == NGSEP)
			*p = MAILSEP;
		/*
		 * plug the alias into the gateway path and tack it onto
		 * the end of the list
		 */
		(void) sprintf(mailto + strlen(mailto), via, bfr);
		(void) strcat(mailto, " ");

		dest->d_status = D_MAILED;
	    }
	}
    }

    /* now do the actual send */
    if (mailto[0])
    {
	char mbuf[BUFLEN];
	FILE *mfp;

	/* notify the user */
	(void) fprintf(stdout,
		       "Your article %s will be mailed to %s.\n",
		       header.h_ident, mailto);

#ifdef DEBUG
	if(!debug) {
#endif /* DEBUG */
	    /* mung the Subject header into proper form for a submission */
	    (void) sprintf(mbuf,
			    "Submission for %s (%s)",
			    header.h_newsgroups, header.h_subject);

	    if ((mfp = mailopen(mailto, mbuf)) == (FILE *)NULL)
		logerr1("Can't send mail to %s", mailto);
	    else
	    {
		FILE    *ifp = xfopen(artfile, "r");
		register int    c, nlcnt = 0;

		(void) fputc('\n', mfp);
		while ((c = fgetc(ifp)) != EOF)
		{
		    /* start printing characters only after seeing \n\n */
		    if (nlcnt == 0 && c == '\n')
			nlcnt = 1;
		    else if (nlcnt == 1)
		    {
			if (c == '\n')
			    nlcnt = 2;
			else
			    nlcnt = 0;
		    }
		    else if (nlcnt == 2)
			(void) fputc(c, mfp);
		}
		(void) fclose(ifp);
		(void) mailclose(mfp);
		log1("Article mailed to %s", mailto);
	    }
#ifdef DEBUG
	}
#endif /* DEBUG */
    }
}

/* post.c ends here */
