/***************************************************************************

NAME
   insert.c - routines for local posting of news

SYNOPSIS
   void postinit()			-- initialize the posting code

   int insert(artfile, originator)	-- insert artfile in local article tree
   char *artfile; bool originator;

DESCRIPTION
   These routines do local posting of an article according to the list of
locations set up by a previous ngprepare().

FILES
   TEXT/.tmp/cmpart??????	-- compressed version of an article

NOTE
   The workfiles above have to be on the same file system as TEXT or the
link calls used in tolocal() will fail.
   If DOXREFS is defined, we expect to pick up new article numbers from the
ng_unread slots of their groups rather than generating them here. 
   If DEBUG is on and the file ADM/rejects exists, copies of all rejected
articles are appended to it.

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
#include "active.h"
#include "history.h"
#include "procopts.h"	/* for the MAXARGS define */
#include "ngprep.h"
#include "feeds.h"
#include "post.h"
#ifdef LEAFNODE
#include "newsrc.h"
#endif /* LEAFNODE */

#undef FEEDBITS

/* verbosity level minima for various messages */
#define V_INSERT	1	/* report on each local insertion */

#define JUNKSUFFIX	".junk"

private char	*CMPART;	/* the compressed article file */
private group_t	*junk, *control;
private feed_t	*self;
#ifdef DEBUG
private bool saverejects;
#endif /* DEBUG */

private void need_file(name)
/* check for accessibility of a special file */
char *name;
{
    FILE	    *mfd;	/* mail file file-descriptor		*/

    /*
     * don't worry about the infamous r+ bug on Microport V/AT, Xenix etc;
     * we're not actually going to write anything to this file.
     */
    if (mfd = fopen(name, "r+"))
	(void) fclose(mfd);
    else
    {
	FILE	    *sfd;	/* scratch file file-descriptor		*/

	mfd = mailopen(site.notify,
		       exists(name) ? "Unwritable files!" : "Missing files!");
	if (mfd != (FILE *)NULL)
	{
	    (void) fprintf(mfd,
		"System: %s\n\nThere was a problem with %s!!\n",
		site.truename, name);

	    (void) sprintf(bfr, "touch %s;chmod 666 %s", name, name);
	    (void) system(bfr);
	    if (sfd = fopen(name, "r+"))    /* see above */
	    {
		(void) fprintf(mfd, "The problem has been taken care of.\n");
		(void) fclose(sfd);
	    }
	    else
		(void) fprintf(mfd, "Correction failed - check suid bits.\n");
	    (void) mailclose(mfd);
	}
    }
}

void postinit()
/* initialize the posting code */
{
#ifdef LEAFNODE
    struct passwd	*pwd;
#endif /* LEAFNODE */

#ifdef DEBUG
    (void) sprintf(bfr, "%s/rejects", site.admdir);
    saverejects = exists(bfr);

    if (!debug)
#endif /* DEBUG */
    {
	/* check that we've got access to all the special files we need */
	need_file(ACTIVE);
#ifndef DBHISTORY
	need_file(HISTORY);
#else
	/*
	 * This breaks the layering a bit, but the alternatives are
	 * all grottier.
	 */
	(void) sprintf(bfr, "%s.dat", HISTORY); need_file(bfr);
	(void) sprintf(bfr, "%s.pag", HISTORY); need_file(bfr);
	(void) sprintf(bfr, "%s.dir", HISTORY); need_file(bfr);
#endif	/* DBHISTORY */
    }

    ngprepinit();	/* initialize various lookup tables */

    /* we also need access to the feeds file */
    s_read();
    if ((self = s_find(site.nodename)) == (feed_t *)NULL
		&& (self = s_find(site.pathname)) == (feed_t *)NULL)
	xerror2("Cannot find my name '%s' in %s", site.pathname, feeds.file);

    junk = ngfind("junk");
    control = ngfind("control");
    if (!(junk = ngfind("junk")))
          xerror("Couldn't find newsgroup junk!  Active file corrupted?");
    if (!(control = ngfind("control")))
          xerror("Couldn't find newsgroup control!  Active file corrupted?");
    (void) sprintf(bfr, "%s/.tmp/cmpXXXXXX", site.textdir);
    (void) mktemp(CMPART = savestr(bfr));

#ifdef LEAFNODE
    /* 
     * Get articles-read info from each subscribing user, so we'll know
     * what can be safely ignored.
     */
    while (pwd = getpwent())
    {
	(void) sprintf(bfr, "%s/.newsrc", pwd->pw_dir);
	(void) rdnewsrc(bfr);
    }
    endpwent();
#endif /* LEAFNODE */

    /* so we know where core files will end up */
    (void) sprintf(bfr, "%s/.tmp", site.textdir);
    (void) chdir(bfr);
}

private int tolocal(hp, artfile, gp)
/* link artfile into dir for ng and update active file  */
hdr_t	*hp;		/* pointer to the header of the article */
char	*artfile;	/* name of the text file */
group_t	*gp;		/* where to post the article to */
{
#ifdef DEBUG
    if (debug)
    {
	if (verbose >= V_INSERT)
	    (void) printf("tolocal: %s to %s\n", hp->h_ident, gp->ng_name);
    }
    else
    {
#endif /* DEBUG */

    char    *grpdir;
    nart_t  newart;
    register int    dstat;

    /*
     * Make the directory for a new newsgroup. The mkbranch() function in
     * libport.a plays various games with mkdir(2) and chown to try to
     * ensure that the directory is owned by NEWSUSR and NEWSGRP; this is
     * tough to do if you aren't root.  This will work on a UCB system (which
     * has mkdir(2)) or on a USG system (which allows you to give away files
     * you own with chown(2)), otherwise you have to change your kernel to
     * allow one of these things or run with your TEXT subdirectories 0777
     * (rwxrwxrwx) so that it doesn't matter who owns them.
     */
    if (dstat = mkbranch(grpdir = artdir(gp->ng_name), 0777))
    {
	logerr3("Cannot mkdir %s: errno = %d on seg %d", grpdir,errno,dstat);
	return(FAIL);
    }

    /* if the group we're localizing to has the 'compressed' flag, do it */
    if (ngflag(NG_COMPRESSED))
    {
	/* someday we'll only do this at most once per article */
	(void) sprintf(bfr, "%s/%s <%s >%s",
		       site.libdir, COMPRESS, artfile, CMPART);
	(void) system(bfr);
	(void) fflush(stdout);	/* V7 system() doesn't flush stdout */
	artfile = CMPART;
    }

    /* O.K., go after a valid number until be can localize */
    do {
#ifndef DOXREFS
	/* generate a new article number */
	if ((newart = ngnewart(gp)) == FAIL)
#else
	/* already generated the new article number for the Xrefs header */
	if (((newart=gp->ng_nextnum) <= 0) && ((newart=ngnewart(gp)) == FAIL))
#endif /* DOXREFS */
	{
	    logerr2("Couldn't localize to %s/%ld", gp->ng_name, (long)newart);
	    return(FAIL);
	}

	/* now make the name of the local copy and link it */
	(void) strcpy(bfr, grpdir);
	if (bfr[strlen(bfr) - 1] != '/')
	    (void) strcat(bfr, "/");
	(void) sprintf(bfr + strlen(bfr), "%ld", (long)newart);
#ifdef DOXREFS
	gp->ng_nextnum = 0;	/* we'll know to gen a new number next time */
#endif /* DOXREFS */
	errno = 0;
#ifdef VMS
	if (vmslink(artfile, bfr))
#else /* !VMS */
	if (link(artfile, bfr))	/* artfile must be on TEXT's file system */
#endif /* !VMS */
	{
	    /* the link failed, find out if we can show why */
	    if (errno == EACCES)
		logerr3("Link of %s to %s failed (%s); check permissions",
			hp->h_ident, bfr, errmsg(errno));
	    else {
		logerr3("Cannot install %s as %s: %s",
			hp->h_ident, bfr, errmsg(errno));
		if (errno == EEXIST)
		{
		    /*
		     * Oh dear, we're in trouble.  Probably the active
		     * file has gotten out of sync (not been updated).
		     * Still, all is not lost.  We can request a new
		     * article number and try again.  The only problem is,
		     * if DOXREFS is being used, updating the XREFS field
		     * will be a pain.  We won't bother, since this is an
		     * unusual case anyway, one that should theoretically
		     * never happen.
		     */
		    logerr2("active file entry for %s out of sync at %d!",
			    gp->ng_name, newart);
		}		
	    }
	    return(FAIL);
	}
    } while
	(errno == EEXIST);

    /* if we generated a compressed version, clean it up */
    if (ngflag(NG_COMPRESSED))
	(void) unlink(CMPART);

    (void) hstadd(hp->h_ident,hp->h_rectime,hp->h_exptime,gp->ng_name,newart);

    if (verbose >= V_INSERT)
	(void) printf("tolocal: %s to %s/%ld\n",
		      hp->h_ident, gp->ng_name, (long)newart);
#ifdef DEBUG
    }
#endif /* DEBUG */
    return(SUCCEED);
}

private bool docontrol(ctlmsgtext, artbody)
/* accept and process a control message header and body */
char *ctlmsgtext;   /* text of the control message */
char *artbody;	    /* the name of a file containing the message body */
{
#ifndef NEWCONTROL
    char		*argv[MAXARGS];
    char 		argbuf[BUFLEN];
    int			argc;
#endif /* NEWCONTROL */

    log3("control %s from %s: %s", header.h_ident, header.h_from, ctlmsgtext);

#ifndef NEWCONTROL
#ifdef DEBUG
    if (debug)
	(void) strcpy(argbuf, "-D ");
    else
	argbuf[0] = '\0';
#endif /* DEBUG */
    (void) strcat(argbuf, ctlmsgtext);
    argc = vcrack(argbuf, argv, MAXARGS);
    return(controlmain(argc, argv, artbody));
#else
    /* pack the message off to the special control handler */
#ifdef DEBUG
    if (debug)
	(void) sprintf(bfr, "%s/control -D %s <%s", LIB, ctlmsgtext, artbody);
    else
#endif /* DEBUG */
	(void) sprintf(bfr, "%s/control %s <%s", LIB, ctlmsgtext, artbody);
    return(system(bfr));
#endif /* NEWCONTROL */
}

private void saveit(svfile, artfile, reason)
/* save an article we couldn't localize to the failure file in batch format */
char	*svfile, *artfile, *reason;
{
    char failures[BUFLEN];
    FILE	*ifp, *ofp;
    register int c;

    if (!exists(artfile))
	return;

    (void) sprintf(failures, "%s/%s", site.admdir, svfile);
    if ((ifp = fopen(artfile, "r")) && (ofp = fopen(failures, "a")))
    {
	/* Unisoft 5.1 won't seek to EOF on 'a' */
	(void) fseek(ofp, (off_t)0, SEEK_END);
	(void) fprintf(ofp, "#! rnews %ld (%s)\n",
		       (long)filesize(artfile), reason);

	while ((c = fgetc(ifp)) != EOF)
	    (void) fputc(c, ofp);
    }

    (void) fclose(ifp);
    (void) fclose(ofp);
}

static void mk_flexgroup(ng, appr)
/* make a new flexgroup */
char	*ng;	/* name of new group */
char	*appr;	/* name of person who approved it */
{
    FILE *fp;

    /*
     * Someday we should check the mailpaths file for a moderator
     * as well as looking for an Approved header.
     */
    ngcreate(ng, appr[0] != '\0');
    if (appr[0] != '\0')
    {
	FILE *mfp;

	(void) sprintf(bfr, "%s/mailpaths", site.admdir);
	if ((mfp = fopen(bfr, "a")) == NULL)
	{
	    (void) sprintf(bfr, "New flexgroup '%s' created", ng);
	    fp = mailopen(site.notify, bfr);
	    (void) fprintf(fp, "but I can't find the mailpaths file!\n");
	    (void) mailclose(fp);
	}
	else
	{
	    /* Unisoft 5.1 won't seek to EOF on 'a' */
	    (void) fseek(mfp, (off_t)0, SEEK_END);
	    if (fprintf(mfp,"%s\t%s", ng, appr) < 0)
	    {
		(void) sprintf(bfr, "New flexgroup '%s' created", ng);
		fp = mailopen(site.notify, bfr);
		(void) fprintf(fp,
		       "but a try at updating the mailpaths failed!\n");
		(void) mailclose(fp);
	    }
	    (void) fclose(mfp);
	}
    }
}

int insert(artfile, originator)
/* insert artfile in the right places in the local article tree */
char	*artfile;
bool	originator;
{
    int		astat, postcount = 0;
    dest_t	*dest;
    time_t	now = time((time_t *)NULL);

    if (!originator && (now > header.h_posttime + HISTEXP))
    {
	log0("Article too old, routed to junk");
	(void) tolocal(&header, artfile, junk);
	return(FAIL);
    }

    /* process the Supersedes header (if present) only once */
    if (hlnblank(header.h_supersedes))
    {
	char	crackbuf[LBUFLEN];

	(void) sprintf(crackbuf, "cancel %s\n", header.h_supersedes);
	(void) docontrol(crackbuf, artfile);
    }

    if (hlnblank(header.h_ctlmsg) || ngmatch(header.h_newsgroups,"all.ctl"))
    {
	char		*ctlmsgtext;

	if (hlnblank(header.h_ctlmsg))
	    ctlmsgtext = header.h_ctlmsg;
	else if (strncmp(header.h_subject, "cmsg ", 5) == 0)
	    ctlmsgtext = header.h_subject + 5;

	/* bomb out if we're trying to originate an invalid control message */
	if (!docontrol(ctlmsgtext, artfile))
	{
	    if (originator)
	    {
		(void)printf("Get your netnews person to do this for you.\n");
		xxit(0);
	    }
	}
	(void) tolocal(&header, artfile, control);
	return(FAIL);
    }
    else if (destinations[0].d_status == D_NOMORE)
    {
	log0("Article will be relayed and junked (no valid groups)");
	(void) tolocal(&header, artfile, junk);
	return(SUCCEED);
    }
    else if (!originator && (astat = ngaccept(self)) != A_ACCEPT)
    {
	if (astat == A_DISTNG)
	{
	    log1("distribution %s unsubscribed", header.h_distribution);
#ifdef DEBUG
	    if (saverejects)
		saveit("rejects", artfile, "unsubscribed distribution");
#endif /* DEBUG */
	}
	else if (astat == A_GRPSNG)
	{
	    log1("newsgroup(s) %s unsubscribed", header.h_newsgroups);
#ifdef DEBUG
	    if (saverejects)
		saveit("rejects", artfile, "unsubscribed newsgroup");
#endif /* DEBUG */
	}
	else /* (astat == A_JUNKIT) */
	    if (tolocal(&header, artfile, junk) == FAIL)
		saveit("failures", artfile, "couldn't localize to junk");
	return(SUCCEED);
    }
    else    /* the destination array is valid and nonempty */
    {
#ifdef DEBUG
	if (!debug)
#endif /* DEBUG */
	    hstparent(&header);   /* if it's a followup, mark its parents */

	for (dest = destinations; dest->d_status != D_NOMORE; dest++)
	{
	    dest_t  *odest;

	    /*
	     * skip destinations that are known to be no good (i.e. moderated
	     * groups that we couldn't generate a path to)
	     */
	    if (dest->d_status == D_CANTMAIL)
		continue;

	    /*
	     * mailing to moderator has to count as a valid post, otherwise,
	     * when MODEXCLUDE is off, articles that are *only* posted to
	     * moderated groups will get junked and rebroadcast
	     */
	    if (dest->d_status == D_MAILED)
	    {
		postcount++;
		continue;
	    }

	    /* we don't necessarily give up on groups with unknown names */
	    if (dest->d_status == D_UNKNOWN)
	    {
		/* because maybe they're in flexgroups hierarchies */
		if (ngmatch(dest->d_name, self->s_gflex))
		{
		    /* this one is, create it and treat like normal group */
		    mk_flexgroup(dest->d_name, header.h_approved);
		    dest->d_status = D_OK;
		    (void) strcpy(bfr, dest->d_name);
		    (void) free(dest->d_name);
		    dest->d_name = NULL;
		    dest->d_ptr = ngfind(bfr);
		}
		else
		{
		    char	*cp;

		    /* map the name to the proper junk group */
		    (void) strcpy(bfr, dest->d_name);
		    while (cp = strrchr(bfr, NGSEP))
		    {
			group_t	*ngp;

			(void) strcpy(cp, JUNKSUFFIX);
			if ((ngp = ngfind(bfr)) == (group_t *)NULL)
			    *cp = '\0';
			else
			{
			    (void) free(dest->d_name);
			    dest->d_ptr = ngp;
			    (void) free(dest->d_name);
			    dest->d_name = NULL;
			    dest->d_ptr = ngp;
			}
		    }

		    if (dest->d_status != D_OK)
		    {
			/* no such junk group exists, discard the name */
			log1("Unknown group '%s' ignored", dest->d_name);
			continue;
		    }
		}
	    }
	    ngselect(dest->d_ptr);

   	    /*
	     * Leave out local groups if we didn't originate the message. Such
	     * groups could make it in if the sender cross-posted to a netwide
	     * group and one local to his/her site, i.e. 'misc.misc,general'
	     */
	    if (!originator && ngflag(NG_LOCAL))
		continue;

	    /* don't insert to the same group twice */
	    for (odest = destinations; odest < dest; odest++)
              if ((odest->d_status != D_UNKNOWN) &&
                  (odest->d_ptr == ngactive()))
                      continue;

#ifdef FEEDBITS
	    /* don't localize crossposts to groups we don't accept */
	    if (ngfeed(ngactive(), 0) && !ngfeed(ngactive(), s_tell(self)))
		continue;
#endif /* FEEDBITS */

	    /* if we got here all is OK, insert to group */
	    if (tolocal(&header, artfile, dest->d_ptr) != FAIL)
		postcount++;

	    /* otherwise save *one* copy to the failure file */
	    else if (postcount == 0)	
		saveit("failures", artfile, "no valid posting sites");

#ifdef FUTURE
	    /*
	     * If there's ever a post-insertion phase that looks
	     * at individual destination slots, it will probably want to see
	     * non-local groups only. This will help.
	     */
	    if (ngflag(NG_LOCAL)
		dest->d_sent = D_MAILED;
#endif /* FUTURE */
	}

	if (postcount == 0)
	{
	    log0("no valid newsgroups");
	    if (tolocal(&header, artfile, junk) == FAIL)
		saveit("failures", artfile, "no valid newsgroups");
	}
	return(SUCCEED);
    }
    /*NOTREACHED*/
}

/* insert.c ends here */
