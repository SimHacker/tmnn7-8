/*****************************************************************************

NAME
   control.c -- handle control messages to inews/rnews

SYNOPSIS
   int controlmain(argc, argv)	-- interpret a control message
   int argc; char *argv;

   int main(argc, argv)		-- interpret a control message
   int argc; char *argv;

DESCRIPTION
   Control message handling code.  Deals with messages which are to be
acted on by netnews itself rather than by people. The main() is only
defined if this module is to form the core of a separate control message
handler under the NEWCTRL option.

   Control messages have the standard format "command [args...]" much like
shell commands. Each site has the option of customizing this code to deal with
control messages as they see fit, but we would like to get copies code,
ifdeffed or otherwise parameterized, to simplify the maintenence issues.

NOTE
   See the news.h symbol NEWS_VERSION for the real version of netnews.
This new version permits root and the netnews administrator to create
and delete groups by posting control messages even if NONEWGROUPS and
MANUALLY are on.

BUGS
   The NEWCTRL version is insecure -- anyone who can feed a doctored
message to its stdin can spoof it.

NOTE
   The voidf typedef is a hack to pacify compilers with half-assed void
support (the immediate culprit is ULTRIX).

FILES
   /tmp/reply??????		-- used for generating sendme messages
   TEXT/.tmp/ctlcpy??????	-- NEWCTRL version keeps a message copy here

REVISED BY
   Eric S. Raymond
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

****************************************************************************/
/*LINTLIBRARY*/
#include "news.h"
#include "libpriv.h"
#include "spawn.h"
#include "header.h"
#include "active.h"
#include "ngprep.h"
#include "post.h"
#include "dballoc.h"
#include "feeds.h"
#include "history.h"
#include "procopts.h"
#include "version.h"

#ifdef NEWCTRL
char	*Progname = "control";	/* so xerror identifies failing program */
#endif /* NEWCTRL */

#ifdef ULTRIX
typedef int voidf;	/* your lint might get mildly annoyed about this */
#else
#define voidf	void	/* 'typedef void voidf' makes some lints upset */
#endif /* ULTRIX */

/* structure for the control message method tables */
typedef struct
{
    char 	*type;
    voidf	(*exec)();
}
method_t;

#ifdef u370
private hdr_t htmp;
#endif /* u370 */

/*
 * It would be nice to put the method table at the end of the module, but some
 * compilers get distressed at attempts to forward-declare it
 */
forward voidf c_ihave(), c_sendme(), c_cancel();
forward voidf c_newgroup(), c_rmgroup(), c_checkgroups();
forward voidf c_sendsys(), c_senduuname(), c_version(), c_addsub(), c_delsub();
forward voidf c_unimp();

private method_t methods[] =
{
    /* name		vector to */
    {"ihave",		c_ihave},
    {"sendme",		c_sendme},
    {"sendbad",		c_sendme},
    {"newgroup",	c_newgroup},
    {"rmgroup",		c_rmgroup},
    {"checkgroups",	c_checkgroups},
    {"sendsys",		c_sendsys},
    {"senduuname",	c_senduuname},
    {"version",		c_version},
    {"cancel",		c_cancel},
    {"delsub",		c_delsub},
    {"addsub",		c_addsub},
#ifdef UNDER_CONSTRUCTION
    {"mvgroup",		c_mvgroup},
#endif /* UNDER_CONSTRUCTION */
    {NULL,		NULL},
};

#ifdef NEWCTRL
int main(argc, argv)
int	argc;
char	**argv;
{
    char	ctlcpy[BUFLEN];

    newsinit();	/* set up defaults and initialize. */
#ifdef DEBUG
    if (!debug)
	loginit();
#endif /* DEBUG */

    argv++, argc--;	/* junk the first (program name) argument */

    (void) rdactive(NULLPRED);

    /* we need to pick the header off the input message */
    /* Fixed by ReviewBot-774 (Issue #33) */
    (void) snprintf(ctlcpy, sizeof(ctlcpy), "%s/.tmp/ctlcpyXXXXXX", TEXT);
    collect((char *)NULL, ctlcpy);
    if (freopen(ctlcpy, "r", stdin) == (FILE *)NULL
					|| hread(&header, 0L, stdin) == 0)
	(void) xerror0("no control message text on stdin!");

    /* hand off to the real dispatcher */
    exit(controlmain(argc, argv, ctlcpy));
}
#endif /* NEWCTRL */

/*VARARGS2*/
private void notify(subj, chdr, cbody, fmt, s1, s2)
/* notify the system administrator of a control message receipt */
char	*subj;	/* subject of the notification */
hdr_t	*chdr;	/* if TRUE, include a copy of the message header */
char	*cbody;	/* if TRUE, include a copy of the message body */
char	*fmt;	/* format for notification message */
char	*s1, *s2;	/* args to be %s-substituted into fmt */
{
    register FILE	*fp, *ifp;

    if ((fp = mailopen(site.notify, subj)) != (FILE*)NULL)
    {
	(void) fputc('\n', fp);
	(void) fprintf(fp, fmt, s1, s2);

	if (chdr != (hdr_t *)NULL)
	{
	    (void) fprintf(fp, "The header of the message follows:\n\n");
	    (void) hwrite(chdr, fp, FALSE);
	}

	if ((cbody != (char*)NULL) && (ifp = fopen(cbody, "r")) != (FILE*)NULL)
	{
	    (void) fprintf(fp, "The message body was:\n");
	    while (fgets(bfr, sizeof(bfr), ifp))
		(void) fputs(bfr, fp);
	    (void) fclose(ifp);
	}

	(void) mailclose(fp);
    }
}

bool controlmain(argc, argv, artbody)
/* dispatch control functions */
int	argc;
char	**argv;
char	*artbody;
{
    register method_t	*mp;

    /* did user ask for debug mode? */
    if (argc > 1 && strcmp(argv[0], "-D") == 0)
    {
#ifdef DEBUG
	debug = 1;
	if (verbose == 0)
	    debug = 1;
#endif /* DEBUG */
	argv++, argc--;	/* junk the option */
    }

    /* here's the method table search */
    for (mp = methods; mp->type != NULL; mp++)
	if (strcmp(argv[0], mp->type) == 0)
		break;

    /*
     * Try to exec unknown messages in control directory, otherwise barf.
     * Right now this code only works for systems that support the !# hack.
     */
    (void) snprintf(bfr, sizeof(bfr), "%s/control/%s", site.libdir, argv[0]);
    if (mp->type == NULL)
    	if (access(bfr, X_OK) == SUCCEED)	/* there's a script for this */
	{
	    log2("control script %s found for %s", bfr, header.h_ident);
	    (void) spawn(argv, FORGRND, (char *)NULL);
	}
	else
	{
	    logerr1("control message %s is of unknown type!", header.h_ident);

#ifndef QUIET
	    notify("Unrecognized Control Message", &header, (char *)NULL,
		   "Currently running news version %s.\n\n", NEWS_VERSION);
#endif /* !QUIET */

	    return(FALSE);
	}

    /* we're authorized, the message is known, go do it */
#ifdef NEWCTRL
    lock();
#endif /* NEWCTRL */
    (*mp->exec)(argc, argv, artbody);
#ifdef NEWCTRL
    unlock();
#endif /* NEWCTRL */
    return(TRUE);
}

/* *** CONTROL MESSAGE METHODS START HERE *** */

/*
 * ihave <artid> ... <remotesys>
 *	or
 * ihave <remotesys>
 *	with <artid>s in message body.
 *
 * The other system is telling you it has articles <artid>..., in case
 * you decide you want it to transmit them to you.
 */
private voidf c_ihave(argc, argv, artbody)
int	argc;
char	**argv;
char	*artbody;
{
    char tl[256], ng[256];
    int i;
    feed_t	*tosys;

    /* check that our link is O.K. */
    if ((tosys = s_find(argv[argc - 1])) == (feed_t *)NULL)
    {
	logerr1("can't transmit sendme to %s", argv[argc - 1]);
	return;
    }

    /*
     * have the other system send over all the messages we don't have
     */
    if (argc == 2)	/* put want list in the message body */
    {
	FILE	*body, *reply;
	char	rtmp[SBUFLEN];
	int	count = 0;

	(void) strlcpy(rtmp, "/tmp/replyXXXXXX", sizeof(rtmp));
	(void) mktemp(rtmp);
	if (!((body = fopen(artbody, "r")) && (reply = fopen(rtmp, "w"))))
	    return;

	while (fgets(bfr, LBUFLEN, body))
	{
            if (*bfr != '<')
                continue;
	    (void) nstrip(bfr);

	    if (hstseek(bfr, FALSE) == FAIL)
	    {
		(void) fprintf(reply, "%s\n", bfr);
		count++;
	    }
	}

	(void) fclose(reply);
	(void) fclose(body);

	if (count)
	{
	    (void) snprintf(ng, sizeof(ng), "to.%s.ctl", argv[1]);
#ifdef DEBUG
	    if (debug)
		log4("ihave calls for xmitctrl(%s, %s, %s, NULL)",
		     argv[1], tl, ng, rtmp);
	    else
#endif /* DEBUG */
		(void) xmitctrl(argv[1], tosys, tl, ng, rtmp);
	}
	(void) unlink(rtmp);
    }
    else	    /* else put it back in the message header */
    {
	(void) strlcpy(tl, "sendme ", sizeof(tl));
	for (i = 1; i < (argc - 1); i++)
	{
	    if (hstseek(argv[i], FALSE) == 0)
	    {
		(void) strlcat(tl, argv[i], sizeof(tl));
		(void) strlcat(tl, " ", sizeof(tl));
	    }
	}
	(void) strlcat(tl, site.nodename, sizeof(tl));
	(void) snprintf(ng, sizeof(ng), "to.%s.ctl", argv[argc - 1]);
#ifdef DEBUG
	if (debug)
	    log3("ihave calls for xmitctrl(%s, %s, %s, NULL)",
		 argv[argc - 1], tl, ng);
	else
#endif /* DEBUG */
	    (void) xmitctrl(argv[argc - 1], tosys, tl, ng, (char *)NULL);
    }
}

/*
 * sendme <artid> ... <remotesys>
 *	or
 * sendme <remotesys>
 *	with <artid>s in message body.
 *
 * The other system wants me to send him article <artid>.
 */
private voidf c_sendme(argc, argv, artbody)
int	argc;
char	**argv;
char	*artbody;
{
    int i;
    FILE *fp;
    char *hf;
    feed_t* sys;

    /* Don't ask for it from myself */
    if (strncmp(site.nodename, argv[argc - 1], SNLN) == 0)
	return;

    /* Find the feeds record */
    s_rewind();
    while ((sys = s_next()) != (feed_t *)NULL)
    {
	FILE	*body;

	if (ngmatch(sys->s_name, argv[argc - 1]))
	    continue;

	if (argc == 2)	/* find a want list in the message body */
	    if ((body = fopen(artbody, "r")) == (FILE *)NULL)
		return;

	/* It's the right system. Send the requested messages. */
	for (i=1; (argc==2)?(fgets(bfr,LBUFLEN,body)!=NULL):(i<argc-1); i++)
	{
	    hf = (argc == 2) ? bfr : argv[i];

	    if ((hf=hstfile(hf))==(char*)NULL ||(fp=msgopen(hf))==(FILE*)NULL)
	    {
		logerr1("Cannot find article %s", argv[i]);
		return;
	    }

	    hfree(&header);
	    if (hread(&header, 0L, fp) == 0)
	    {
		(void) msgclose(fp);
		logerr1("Article %s is garbled.", argv[i]);
		return;
	    }
	    (void) msgclose(fp);

	    /*
	     * Send it if other sys subscribes. It's OK to call ngprepare()
	     * and trash the old destinations info because we don't need
	     * to handle more destination groups after we've seen 'all.ctl'.
	     *
	     * BUG: if we happened to find
	     * only a compressed copy it will be sent compressed. This will
	     * only happen if *all* copies of a message are compressed, but
	     * it will cause problems on non-TMN versions (because rnews won't
	     * detect compression) or to TMN neighbors using F (they'll see
	     * a batch with one garbage member). Haven't decided what the
	     * best way to fix this is yet.
	     */
	    ngprepare();
	    if (ngaccept(sys) >= A_ACCEPT)
#ifdef DEBUG
		if (debug)
		    log1("sendme calls for dispatch of %s", hf);
		else
#endif /* DEBUG */
		    (void) dispatch(argv[argc - 1], sys, &header, hf, FALSE);
	}

	if (argc == 2)		/* we found a want list in the message body */
	    (void) fclose(body);

	return;
    }
    logerr1("Cannot find system %s to send articles to", argv[argc - 1]);
}

/*
 * newgroup <groupname...>
 * One or more newsgroup are to be created.
 * The body of the article, if present, is a description of the
 * purpose of the newsgroup.
 *
 */
private voidf c_newgroup(argc, argv, artbody)
int  argc;
char **argv;
char *artbody;
{
    FILE	*mfp, *ifp;
#ifdef NONEWGROUPS
    int		totargc = argc;
    bool	mkpriv;
#endif /* NONEWGROUPS */

    if (argc < 1)
    {
	logerr0("newgroup: Not enough arguments.");
	return;
    }
    
    (void) snprintf(bfr, sizeof(bfr), "Newgroup request from %s.\n", header.h_path);
    if ((mfp = mailopen(site.notify, bfr)) == (FILE*)NULL)
    {
	logerr0("couldn't open notification file.");
	return;
    }

    /* did some kind of net.deity sponsor this one? */
    if (hlnblank(header.h_approved))
	(void) fprintf(mfp, "Creation approved by %s.\n\n", header.h_approved);
    else
	return;		/* if not, just ignore it */

#ifdef NONEWGROUPS
#ifdef ORGDIST
    mkpriv = privileged || !strcmp(ORGDIST, header.h_distribution);
#else
    mkpriv = privileged;
#endif /* ORGDIST */
#endif /* NONEWGROUPS */

    for (argc--, argv++; argc; argc--, argv++)
    {
	bool    mkmod;
	group_t	*grp;
	char	*grpname;

	grpname = argv[0];
	if (mkmod = (argc > 1) && !strcmp(argv[1], "moderated"))
 	    argv++, argc--;

	/* first, either create the group or notify the user */
	if ((grp = ngfind(grpname)) == (group_t *)NULL)
	{
#ifdef NONEWGROUPS
	    if (mkpriv)
#endif /* NONEWGROUPS */
	    {
		ngcreate(grpname, mkmod);
		grp = ngfind(grpname);
		(void) snprintf(bfr, sizeof(bfr),
				"Newsgroup %s created %smoderated.\n", 
				grpname, mkmod ? "" : "un");
		log0(bfr);
		(void) fprintf(mfp, "%s\n", bfr);
	    }
#ifdef NONEWGROUPS
	    else
	    {
		(void) snprintf(bfr, sizeof(bfr),
		       "Creation of %s (%smoderated) was requested.\n", 
			grpname, mkmod ? "" : "un");
		log0(bfr);
		(void) fprintf(mfp, "%s\n", bfr);

		/*
		 * if there is only one request, we assume this code was called
		 * from inews and get chatty with the caller.
		 */
		if (totargc == 1)
		{
		    (void) fprintf(mfp,
				"You can create the newgroup yourself\n");
#ifdef ORGDIST
		    (void) fprintf(mfp,
				"(with a distribution of %s) by doing:\n",
				ORGDIST);
		    (void) fprintf(mfp,
				"%s/inews -d %s -C %s\n",
				site.libdir, ORGDIST, argv[1]);
#else /* !ORGDIST */
		    (void) fprintf(mfp,
				"with the command:\n%s/inews -C %s\n",
				site.libdir, argv[1]);
#endif /* !ORGDIST */
		}
	    }
	    continue;
#endif /* NONEWGROUPS */
	}
 
	/* O.K., now time to change the moderation bit if so indicated */
	if (mkmod == ((grp->ng_flags & NG_MODERATED) != 0))
	{
	    (void) fprintf(mfp,
		"Newsgroup %s is already %smoderated.\n",
		grpname, mkmod ? "" : "un");
	}
#ifdef NONEWGROUPS
	else if (mkpriv)		/* flip the mod bit */
#else
	else
#endif /* NONEWGROUPS */
	{
#ifndef SPOOLNEWS
#ifdef DEBUG
	    if (!debug)
#endif /* DEBUG */
		lock();
	    (void) ngreread(grp, NULLPRED);
#endif /* SPOOLNEWS */
	
	    /* the active file was wrong about the state of the group */
	    if (mkmod)
		grp->ng_flags |= NG_MODERATED;
	    else
		grp->ng_flags &= ~NG_MODERATED;

#ifndef SPOOLNEWS
	    (void) fseek(active.fp,
			 (off_t)(grp->ng_fseek - strlen(grp->ng_name) - 1),
			 SEEK_SET);
#ifdef DEBUG
	    if (debug)
		log1("c_newgrp()'s update of %s suppressed", grp->ng_name);
	    else
#endif /* DEBUG */
		ngshow(grp, active.fp);
#ifdef DEBUG
	    if (!debug)
#endif /* DEBUG */
		unlock();
#endif /* SPOOLNEWS */

	    if (mkmod)
		(void) snprintf(bfr, sizeof(bfr),
			"Newsgroup %s changed from unmoderated to moderated.",
			 grpname);
	    else
		(void) snprintf(bfr, sizeof(bfr),
			"Newsgroup %s changed from moderated to unmoderated.",
			 grpname);
	    log0(bfr);
	    (void) fprintf(mfp, "%s\n", bfr);
	}
#ifdef NONEWGROUPS
	else
	{
	    (void) snprintf(bfr, sizeof(bfr),
			       "%soderation requested for %s.", 
				mkmod ? "M" : "Unm", grpname);
	    log0(bfr);
	    (void) fprintf(mfp, "%s\n", bfr);
	}
#endif /* NONEWGROUPS */
    }

    /* include the article body */
    if ((artbody != (char*)NULL) && (ifp = fopen(artbody, "r")) != (FILE*)NULL)
    {
	(void) fprintf(mfp, "\nThe message body was:\n");
	while (fgets(bfr, sizeof(bfr), ifp))
	    (void) fputs(bfr, mfp);
	(void) fclose(ifp);
    }

    (void) mailclose(mfp);
}

/*
 * rmgroup <groupname...>
 * Old newsgroups are being cancelled on a network wide basis.
 */
private voidf c_rmgroup(argc, argv)
int  argc;
char **argv;
{
    FILE  *mfp;
#ifdef MANUALLY
    bool  mkpriv;
#endif /* MANUALLY */

    if (argc < 1)
    {
	logerr0("rmgroup: Not enough arguments.");
	return;
    }
    
    (void) snprintf(bfr, sizeof(bfr), "Rmgroup request from %s.\n", header.h_path);
    if ((mfp = mailopen(site.notify, bfr)) == (FILE*)NULL)
    {
	logerr0("couldn't open notification file.");
	return;
    }

    /* did some kind of net.diety sponsor this one? */
    if (hlnblank(header.h_approved))
	(void) fprintf(mfp, "Removal approved by %s.\n\n", header.h_approved);
    else
	return;		/* if not, just ignore it */

#ifdef MANUALLY
#ifdef ORGDIST
    mkpriv = privileged || !strcmp(ORGDIST, header.h_distribution);
#else
    mkpriv = privileged;
#endif /* ORGDIST */
#endif /* MANUALLY */

    for (argc--, argv++; argc; argc--, argv++)
    {
	group_t	*grp;

	if ((grp = ngfind(*argv)) != (group_t *)NULL)
	{
#ifdef MANUALLY
	    if (mkpriv)
#endif /* MANUALLY */
	    {
		ngselect(grp);
		ngdelete();
		(void) snprintf(bfr, sizeof(bfr), "Newsgroup '%s' has been removed.\n",*argv);
	    }
#ifndef MANUALLY
	    else
		(void) snprintf(bfr, sizeof(bfr), "Removal of %s has been requested.\n",*argv);
#endif /* !MANUALLY */
	    log0(bfr);
	    (void) fprintf(mfp, "%s\n", bfr);
	}
    }
    (void) mailclose(mfp);
}

/*
 * Check the active file for old or missing newsgroups
 * Body of article is list of valid groups
 */
/* ARGSUSED0 */
private voidf c_checkgroups(argc, argv, artbody)
int  argc;
char **argv;
char *artbody;
{
    (void) snprintf(bfr, sizeof(bfr), "%s/checkgroups %s <%s",
		   site.libdir, site.notify, artbody);
#ifdef DEBUG
    if (debug)
	log1("checkgroups called for system(%s)", bfr);
    else
#endif /* DEBUG */
	log2("system(%s) status %d", bfr, system(bfr));
}

/*
 * Generate the name of the person responsible for posting this article,
 * in order to check that two articles were posted by the same person.
 */
private char *
senderof(hp)
hdr_t *hp;
{
    char *q, *tp;
    char *tailpath();

    if (hlnblank(hp->h_sender))
	tp = hp->h_sender;
    else if (hlnblank(hp->h_from))
	tp = hp->h_from;
    else
	tp = tailpath(hp);

    /* Remove full name */
    q = strchr(tp, ' ');
    if (q)
	*q = '\0';

    return(savestr(tp));
}

/*
 * cancel <artid>
 * Cancel the named article
 */
private voidf c_cancel(argc, argv)
int  argc;
char **argv;
{
    char *r, *poster;
    register FILE *fp;
    char whatsisname[BUFLEN], nfilename[BUFLEN];
    bool su = FALSE;
    place_t myloc;
#ifndef u370
    static hdr_t htmp;	/* so it will be reliably zeroed out */
#endif /* !u370 */

    if (argc < 1)
	logerr0("cancel: Not enough arguments.");
#ifdef DEBUG
    else if (debug)
	log1("cancel called for cancellation of %s", header.h_ident);
    else
#endif /* DEBUG */
    {
	if (hstseek(argv[1], TRUE) != FAIL)
	{
	    if (hstat() == EXPIRED)
	    {
		log1("%s already expired", argv[1]);
		return;
	    }
	    else if (hstat() == CANCELLED) {
		log1("%s already cancelled", argv[1]);
		return;
	    } 
	    else
		log1("Cancelling %s", argv[1]);

	    (void) strlcpy(whatsisname, senderof(&header), sizeof(whatsisname));

	    /* try to delete all the incarnations of this message */
	    if (privileged && !strcmp(header.h_distribution, "local"))
		su = TRUE;
	    while (hstloc(&myloc) == SUCCEED)
	    {
		(void) artname(&myloc, bfr);
		if ((fp = fopen(bfr, "r")) == (FILE *)NULL)
		{
		    log1("Already removed %s", bfr);
		    continue;
		}
		hfree(&htmp);
		if (hread(&htmp, 0L, fp) == 0)
		{
		    if (bfr[0] == '/')
		    {
			fp = fopen(bfr, "r");
			hfree(&htmp);
			if (fp == (FILE *)NULL || hread(&htmp, 0L, fp) == 0)
			{
			    logerr0("Article is garbled.");
			    return;
			}
		    }
		    else
		    {
			logerr0("Article is garbled.");
			return;
		    }
		}
		else
		{
		    (void) fclose(fp);
		    poster = senderof(&htmp);
		    /* only compare up to '.' or ' ' */
		    r = strchr(poster,'.');
		    if (r == (char *)NULL)
			r = strchr(poster,' ');
		    if (r != (char *)NULL)
			*r = '\0';
		    if (!su && strncmp(whatsisname, poster,strlen(poster)))
		    {
			logerr2("Not contributor: posted by %s, you are %s",
				poster, whatsisname);
			return;
		    }
		    (void) unlink(nfilename);
		}
	    }
	}


#ifndef ENTRYLOCK
	/*
	 * since the cancel message wants to alter an existing history entry,
	 * we have to read in the whole history file and lock out other
	 * instances of inews/rnews and expire until we've written it out
	 * again, which in practice means waiting till xxit().
	 */
	lock();
	(void) hstread(TRUE);
#endif /* ENTRYLOCK */

	/* now write the change to the history array */
	(void) hstcancel(argv[1]);
    }
}

/*
 * addsub site group
 *
 * Add the groups named into the feed for this site.
 * The groups argument may contain commas but no wildcards.
 */
private voidf c_addsub(argc, argv)
int  argc;
char **argv;
{
    feed_t	*sp;

    if (argc < 3)
	logerr1("addsub failed, only %d arguments", argc - 1);
    else if ((sp = s_find(argv[1])) == (feed_t *)NULL)
	logerr1("addsub failed, no such site as %s", argv[1]);
    if (!ngmatch(argv[2], sp->s_gflex))
    {
	logerr1("addsub failed, this addition not authorized", argv[1]);
#ifndef QUIET
	notify("addsub control message", (hdr_t *)NULL, (char *)NULL,
	       "%s requested that you feed it %s, the request failed.\n",
	       argv[1], argv[2]);
#endif /* QUIET */
    }
    else
    {
#ifndef QUIET
	notify("addsub control message", (hdr_t *)NULL, (char *)NULL,
	       "%s requested that you feed it %s.\n",
	       argv[1], argv[2]);
#endif /* QUIET */
	s_changesub(argv[1], argv[2], TRUE);
	s_write();
    }
}

/*
 * delsub site groups
 *
 * Drop the named groups out of the feed for this site.
 * The groups argument may contain commas but no wildcards.
 */
private voidf c_delsub(argc, argv)
int  argc;
char **argv;
{
    if (argc < 3)
	logerr1("delsub failed, only %d arguments", argc - 1);
    else if (s_find(argv[1]) == (feed_t *)NULL)
	logerr1("delsub failed, no such site as %s", argv[1]);
    else
    {
	s_changesub(argv[1], argv[2], FALSE);
	s_write();
#ifndef QUIET
	notify("delsub control message", (hdr_t *)NULL, (char *)NULL,
	       "%s requested that you no longer feed it %s.\n",
	       argv[1], argv[2]);
#endif /* QUIET */
    }
}

/*
 * sendsys [system]
 *
 * Mail the feeds file to the person submitting the article. If
 * an argument is given, send only the information for that system.
 * POLICY: the contents of your feeds file are public information
 * and as such, you should not change this code.  You may feel
 * free to arrange for it to manually notify you, in the event
 * that you want to do something to clean it up before it goes out.
 * Secret sites on the net are expressly frowned on.
 * 
 * The purpose of this command is for making a network map.  The
 * details of your link and which newsgroups are forwarded are not
 * important, in case you want to sanitize them.  Since the definition
 * of USENET is those sites getting news.* , you can disable this
 * on sites not getting net articles, but if you take out the list of
 * forwarded newsgroups, and you have sites that only get local newsgroups,
 * you should make this clear, or remove those sites from what you send out.
 */
/* ARGSUSED */
private voidf c_sendsys(argc, argv)
int  argc;
char **argv;
{
    register FILE *mfp;

    /* ship off the response */
    mfp = mailopen(mailreply(&header), "response to your sendsys request");
    if (mfp != (FILE *)NULL)
    {
	feed_t	*sp;

	(void) fprintf(mfp, "Responding-System: %s\n\n", site.truename);
	if (argc == 1)
	{
	    s_rewind();
	    while ((sp = s_next()) == (feed_t *)NULL)
		if (argc == 0 || ngmatch(argv[1], sp->s_name))
		    (void) s_writerec(sp, mfp);
	}
	(void) mailclose(mfp);
    }

#ifndef QUIET
    notify("sendsys control message", (hdr_t *)NULL, (char *)NULL,
	"%s requested your feeds file. It has been sent\n", header.h_path);
#endif /* QUIET */
}

private jmp_buf croaked;

private catch_t deadchild(signo)
/* catch broken pipes to subprograms */
int	signo;
{
    longjmp(croaked, signo);
}

/*
 * senduuname	(no arguments)
 *
 * Run the "uuname" command and send it back to the person who submitted
 * the article.  The purpose of this control message is for attempting to
 * make a uucp net map.
 *
 * POLICY: If you view this information as not public (because you have
 * a connection you consider secret, or know a site that considers itself
 * secret) you can feel free to change this code in whatever way is
 * appropriate, so long as it sends some response back to the sender.  If
 * you don't run uucp, this code does not make sense, and so an error
 * message (or garbage, such as "research") will be mailed back.
 *
 * If you wish to add or remove sites from the output of uuname, you
 * may wish to use the euuname.sh shell script here.
 */
/* ARGSUSED */
private voidf c_senduuname(argc, argv)
int  argc;
char **argv;
{
    FILE *fp, *ufp;
    catch_t (*oldpbreak)();
    int status;
#ifdef UUPROG
    char	*uuprog = newsattr("uuprog", UUPROG);
#endif /* UUPROG */

    fp = mailopen(mailreply(&header), "response to your senduuname request");
    (void) fprintf(fp, "Responding-System: %s\n\n", site.truename);

#ifdef UUPROG
    if (uuprog[0] == '/')
	(void) strlcpy(bfr, uuprog, sizeof(bfr));
    else
	(void) snprintf(bfr, sizeof(bfr), "%s/%s", site.libdir, uuprog);
#else
    (void) strlcpy(bfr, "uuname", sizeof(bfr));
#endif /* UUPROG */

    /* call uuname or UUPROG with protection in case the exec fails */
    oldpbreak = signal(SIGPIPE, SIGCAST(deadchild));
    if ((status = setjmp(croaked)) == 0)
    {
	if (fp != (FILE *)NULL && (ufp = peopen(bfr, "r")) != (FILE *)NULL)
	{
	    int c;

	    while ((c = getc(ufp)) != EOF)
		(void) putc(c, fp);
	    (void) peclose(ufp);
	}
    }
    else	/* longjmp got called, status holds the signal number */
	logerr2("\"%s\" gave signal %d", bfr, status);
    (void) signal(SIGPIPE, SIGCAST(oldpbreak));

    (void) mailclose(fp);
#ifndef QUIET
    notify("uuname control message", (hdr_t *)NULL, (char *)NULL,
	"%s requested your uuname output. It has been sent.\n", header.h_path);
#endif /* QUIET */
}

/*
 * Send the version number to the right person.
 */
/* ARGSUSED */
private voidf c_version(argc, argv)
int  argc;
char **argv;
{
    register FILE *fp;

    if ((fp = mailopen(mailreply(&header), "Our news version")) == (FILE*)NULL)
	logerr0("Cannot send back error message");
    (void) fprintf(fp, "\nResponding-System: %s\n\n", site.truename);
    (void) fprintf(fp, "Currently running news version %s.\n\n", NEWS_VERSION);
    (void) fprintf(fp, "The header of your message follows:\n\n");
    (void) hwrite(&header, fp, FALSE);
    (void) mailclose(fp);
}

#ifdef UNDER_CONSTRUCTION
/* this command must be capable of copying an entire directory subtree */
#define TREECOPY    "cd %s; tar cf - . | (cd %s; tar xf -)"

/* ARGSUSED */
private voidf c_mvgroup(argc, argv)
/* rename the newsgroup argv[1] to be argv[2] */
int  argc;
char **argv;
{
    /* check that the new name doesn't exist and the old does */
    if (ngfind(argv[2]) == (group_t*)NULL && ngfind(argv[1]))
    {
	char	*newdir = savestr(artdir(argv[2]));

	/* first, create the new directory name */
	if (mkbranch(newdir, 0777) != SUCCEED)
	{
	    logerr1("couldn't create new directory %s for rename", newdir);
	    return;
	}

	/* now move everything underneath the old location to the new one */
	(void) snprintf(bfr, sizeof(bfr), TREECOPY, artdir(argv[1]), newdir);
	(void) free(newdir);
	if (system(bfr))
	{
	    logerr1("couldn't execute %s", bfr);
	    return;
	}

	/* if the article tree transplant worked, change the active file */
	(void) free(active.article.m_group->ng_name);
	active.article.m_group->ng_name = savestr(argv[2]);

	/*
	 * As presently implemented, there is one significant problem with
	 * this message -- the history file or database doesn't know about
	 * the rename. This means that an ordinary expire (driven by the
	 * history file) won't catch old articles in a (nonvolatile) group
	 * that has been renamed.
	*/

	/* now correct the history file */
	hstrewind(TRUE);
	while (hstnext() == FAIL)

	/* now add an aliases entry, no big deal if this fails */
	(void) snprintf(bfr, sizeof(bfr),
	    "echo '%s\t%s' >>%s/aliases", argv[1], argv[2], site.admdir);
	(void) system(bfr);

	/* 
	 * We could aenter() into the in-core aliases list, but then it
	 * would have to be visible from here. To heck with it -- it can
	 * only cause article junking in the remainder of the current batch
	 */

#ifndef QUIET
	notify("mvgroup control message", (hdr_t *)NULL, (char *)NULL,
	   "Attempted mvgroup of %s to %s\n", argv[1], argv[2]);
#endif /* !QUIET */
    }
    else
	logerr2("invalid mvgroup message: %s to %s" , argv[1], argv[2]);
}
#endif /* UNDER_CONSTRUCTION */

#ifdef FLEXGROUPS
/* ARGSUSED */
private voidf c_unimp(argc, argv, artbody)
int  argc;
char **argv;
char *artbody;
{
#ifndef QUIET
    notify("Unimplemented Control Message", &header, artbody,
	    "Currently running news version B %s.\n", NEWS_VERSION);
#endif /* !QUIET */
}
#endif /* FLEXGROUPS */

#ifdef NEWCTRL
catch_t xxit(status)
/* exit and cleanup */
int status;
{
    exit(status);
}
#endif /* NEWCTRL */

/* control.c ends here */
