/***************************************************************************

NAME
   expire.c - daemon that runs around and removes all expired articles

SYNOPSIS
   main(argc, argv)	    -- performs the expire function
   int argc, char *argv[];

   catch_t xxit(stat)	    -- supply simple exit for helpers

DESCRIPTION
   See the accompanying documentation. This version incorporates the new
volatile-groups and subject-list features.

NOTE
   If you compile with TMNCONVERT on, explicit expire dates will be ignored.
   If region-locking is used to serialize access to the history database,
expire can run concurrently with rnews.

BUGS
   Expire won't use a user's newsrc unless it's in the standard location.
   If expire is interrupted before wractive() is executed, least-article
numbers will not be updated correctly.

FILES
   ADM/EXPLOCK		-- exists while expire is running
   ~user/.newsrc	-- records of what articles users have seen.

AUTHOR
   Eric S. Raymond
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

****************************************************************************/
/*LINTLIBRARY*/
#include "news.h"
#include "libpriv.h"
#include "active.h"
#include "history.h"
#include "header.h"
#include "newsrc.h"
#include "procopts.h"

#ifdef SPOOLNEWS
#undef ENTRYLOCK
#endif

char *Progname = "expire";  /* used by xerror to identify failing program */

char *NHISTORY;			/* Name of new history file, in case */
				/* of copy mode */
char *OHISTORY;

/* verbosity level minima for various actions */
#define V_SHOWSTATS	1	/* show statistics at end of run */
#define V_SHOWPHASE	1	/* show processing phases */
#define V_SHOWACTS	2	/* show each database action taken */
#define V_SHOWLOC	3	/* show each ID/location processed */

#define DFLTEXP	14*DAYS		/* default expiration period */

/* expire control variables */
private int	ignorexp, ignorold, noexpire;
private int	rebuild, usepost, frflag, nosend, convert, fastmode;
private int	expdays, forgetdays, unlinks_noent, unlinks_failed, hbuilds;
private long	expincr, forgetincr;
private char	arpat[BUFLEN] = "all", ngpat[BUFLEN] = "all";
private char	baduser[BUFLEN], artfile[BUFLEN];
private	bool	havehdr = FALSE, dowractive = FALSE;
private time_t	now;

extern int	rdactcount;	/* defined in rdactive.c */

/* expiry statistics */
private int grpunknown;		/* count of unknown groups encountered */
private nart_t hstcount;	/* total count of history lines processed */
private nart_t hsterrcount;	/* count of bad history lines */
private nart_t forgetcount;	/* articles past forget date */
private nart_t fromtoday;	/* count of articles that came in today */

typedef struct
{
    nart_t implicits;		/* acts assoc'd with default expire */
    nart_t explicits;		/* acts assoc'd with explicit expire dates */
    nart_t volatiles;		/* acts assoc'd with volatile property */
    nart_t total;		/* total acts */
}
cestat;

typedef struct
{
    cestat keeps;		/* track keeps for given control line */
    cestat drops;		/* track drops for given control line */
    nart_t archived;		/* number of articles archived */
    nart_t unlinked;		/* number of articles unlinked */
}
expstat;

private expstat *statistics;	/* malloc'ed statistics array */
private char **ctrllines;	/* the control lines array */
private int ctlines;		/* number of elements in above */

private option_t options[] =
{
/*
optlet  filchar	flag	    from    to	    type     buf
*/
'n', NGDELIM,	NONE,       DNC,    DNC,    STRING,  ngpat,
'a', NGDELIM,	NONE,       DNC,    DNC,    STRING,  arpat,
'e',	 ' ',	&expdays,   DNC,    DNC,    NUMBER,  (char *)NULL,
'E',	 ' ',	&forgetdays,  DNC,    DNC,    NUMBER,  (char *)NULL,
'f',	'\0',	&frflag,    DNC,    DNC,    OPTION,  baduser,
'F',	'\0',	&fastmode,  DNC,    DNC,    OPTION,  (char *)NULL,
'i',	'\0',	&ignorold,  DNC,    DNC,    OPTION,  (char *)NULL,
'I',	'\0',	&ignorexp,  DNC,    DNC,    OPTION,  (char *)NULL,
'p',	'\0',	&usepost,   DNC,    DNC,    OPTION,  (char *)NULL,
'r',	'\0',	&rebuild,   DNC,    DNC,    OPTION,  (char *)NULL,
's',	'\0',	&convert,   DNC,    DNC,    OPTION,  (char *)NULL,
'v',	 ' ',	&verbose,   DNC,    DNC,    NUMBER,  (char *)NULL,
'X',	'\0',	&nosend,    DNC,    DNC,    OPTION,  (char *)NULL,
#ifdef DEBUG
'D',	 ' ',	&debug,    DNC,    DNC,    NUMBER,  (char *)NULL,
#endif /* DEBUG */
'\0',	'\0',	0,	    0,	    0,	    0,       (char *)NULL,
};

main(argc, argv)
int	argc;
char	**argv;
{
    forward bool	obsolesce();
    forward void	cleandirs(), printstats();
    forward int		build();
    int volatiles = 0;

    newsinit();			/* set up defaults and initialize. */
#ifdef DEBUG
    if (!debug)
	loginit();
#endif				/* DEBUG */

    /* tell the logfile about the options */
    if (!isatty(fileno(stdin))) {
	char	**cpp;
	int	ac;

	(void) sprintf(bfr, "running");
	for (ac = argc, cpp = argv; ac--; cpp++) {
	    (void) strcat(bfr, " ");
	    (void) strcat(bfr, *cpp);
	}
	(void) strcat(bfr, "\n");
	log0(bfr);
    }

    /* process command-line options */
    verbose = V_SHOWSTATS;
    if (procopts(argc, argv, DNC, options) == FAIL) {
	(void) fprintf(stderr,
		       "Usage: expire [-v [level]] [-eE days ] [-iIarhpusx] [-f user] [-n groups]\n"
		       );
	exit(1);
    }

#ifdef DEBUG
    if (debug) {
	nosend = TRUE;
	if (verbose < debug)
	    verbose = debug;
    }
#endif				/* DEBUG */

    if (expdays && forgetdays)
	xerror0("can't forget things sooner than they expire.");

    if (expdays)
	expincr = expdays * DAYS;

    if (forgetdays)
	forgetincr = forgetdays * DAYS;
    else
	forgetincr = HISTEXP;

    if (convert)
	rebuild++;
    if (rebuild)
	noexpire++;

    now = time((time_t *)NULL);

    if (chdir(site.textdir))
	xerror1("Cannot chdir to %s", site.textdir);

    if (signal(SIGHUP, SIGCAST(SIG_IGN)) != SIGCAST(SIG_IGN))
	(void) signal(SIGHUP, xxit);
    if (signal(SIGINT, SIGCAST(SIG_IGN)) != SIGCAST(SIG_IGN))
	(void) signal(SIGINT, xxit);
#ifndef DEBUG
    if (signal(SIGQUIT, SIGCAST(SIG_IGN)) != SIGCAST(SIG_IGN))
	(void) signal(SIGQUIT, xxit);
#endif				/* DEBUG */

    /*
     * We have to run sendbatch(8) here to make sure that articles locally
     * posted to volatile groups all get shipped out before they get
     * volatilized away.
     */
    if (!nosend && !noexpire) {
	if (verbose >= V_SHOWPHASE)
	    (void) fprintf(stderr, "Running sendbatch...");
	(void) sprintf(bfr, "%s/sendbatch", site.libdir);
	(void) system(bfr);
	if (verbose >= V_SHOWPHASE)
	    (void) fprintf(stderr, "Done.\n");
    }

    /*
     * If we're not in fastmode, open up the nhistory files
     */
    if (!fastmode)
    {
	Sprint1(NHISTORY, "%s/nhistory", site.admdir);
	Sprint1(OHISTORY, "%s/ohistory", site.admdir);
	touch_nhist("dat");
	touch_nhist("dir");
	touch_nhist("pag");
    }

    /*
     * Phase 1: Preparation. Read in our image of the active and history
     * file data and set up the newsgroup status flags.
     */

    /* read in newsgroup-administration data */
    if (!rebuild || exists(ACTIVE))
        (void) rdactive(NULLPRED);
    else
	rdactcount++;  /* Say read in a null file */
    ctrllines = rdflags(NG_VOLATILE|NG_IGNEXPDATE|NG_ARCHIVEIT|NG_EXPIRE);
    for (ctlines = 0; ctrllines[ctlines]; ctlines++)
	continue;
    if (ctlines >= 1)
#ifndef lint
	statistics = (expstat *) calloc((unsigned int) ctlines + 1,
					(unsigned int) sizeof(expstat));
#else
    statistics = (expstat *)NULL;
#endif				/* lint */
    else
	xerror("Your expiration control file is missing or garbled!");

    dowractive++;
    if (rebuild)
    {
	hstread(rebuild);
	if (fastmode)
	    hstclean();	    /* clean out database so it won't grow forever */
	else
	{
	    /* keep information about unforgotten but expired messages */
	    if ((wrhistdb = dbmopen(NHISTORY)) == (database *)NULL)
		xerror2("Can't open %s database, errno is %d", NHISTORY,errno);
	    dbmtrunc(wrhistdb);	/* Clear out any old cruft */
		 
	    hstrewind();
	    while (hstnext(TRUE) != FAIL)
	    {
		int mtype = hstat();

		if (mtype == EXPIRED || mtype == REFERENCE)
		    hstenter(hstline());
	    }
	}

	/* initmsg() would skip initial empty groups */
	ngreset(TRUE);
    }
    else
    {
	hstread(TRUE);
	if (!fastmode)
	{
	    if ((wrhistdb = dbmopen(NHISTORY)) == (database *)NULL)
		xerror2("Can't open %s database, errno is %d",
			NHISTORY, errno);
	    dbmtrunc(wrhistdb);	/* Clear out any old cruft */
	}
	
	hstrewind();
	(void) hstnext(TRUE);
	(void) fprintf(stdout, "Initially looking at ID %s\n", hstid());
    }

    /* set selection bits for use in phase 2 */
    ngrewind(TRUE);
    while (ngnext())
    {
	/*
	 * Note that this use of the RC_UNSEL bit relies on
	 * the fact that rdnewsrc() won't pick up user -n
	 * options. 
	 */
	if (arpat[0])
	    ngfclear(NG_ARCHIVEIT);
	rcfclear(RC_UNSUB | RC_UNSEL);
	if (!ngmatch(ngname(), ngpat))
	    rcfset(RC_UNSEL);
	else if (arpat[0] && ngmatch(ngname(), arpat))
	    ngfset(NG_ARCHIVEIT);

	if (ngflag(NG_VOLATILE))
	    volatiles++;
    }

    /*
     * if we're doing expiration and the admin file declares
     * anything volatile, gather information telling which
     * articles everyone has seen 
     */
    if (!noexpire && volatiles > 0)
    {
	struct passwd	*pwd;
	int	leastuid = atoi(newsattr("leastuid", LEASTUID));

	/* 
	 * Get articles-read info from each subscribing user.
	 */
#ifndef lint
	setpwent();
#endif				/* lint */
	while (pwd = getpwent())
	    if (pwd->pw_uid >= leastuid) {
		char	newsrc[BUFLEN];

		(void) sprintf(newsrc, "%s/.newsrc", pwd->pw_dir);
		if (rdnewsrc(newsrc) == FAIL && verbose >= V_SHOWACTS)
		    (void) fprintf(stderr,
				   "Couldn't read %s, errno = %d\n",
				   newsrc, errno);
	    }
#ifndef lint
	endpwent();
#endif				/* lint */
    }

    /*
     * Before we can go to Phase 2, we must lock out other rnews
     * and expire instances to be sure our history database write
     * will leave things in a consistent state. The active file
     * doesn't need to be locked, because the rdactive() in the
     * critical region just before termination will update all the
     * greatest-article-number fields and incorporate all new
     * groups.
     */
#ifdef DEBUG
    if (!debug)
#endif /* DEBUG */
#ifdef ENTRYLOCK
	if (rebuild)
#endif				/* defined(ENTRYLOCK) */
	{
	    /* lock rnews/inews and other expires out of the history file */
	    privlock();  
	}

    /*
     * Phase 2: apply normal expiration criteria. We may be
     * stepping through in either history file order or article
     * tree order. There is no noexpire check around obsolesce()
     * because the noexpire logic has to be implemented inside the
     * function.
     */
    if (verbose >= V_SHOWPHASE && !noexpire)
	(void) fprintf(stdout, "Checking for out-of-date messages...\n");
    if (rebuild)
	(void) textwalk(build);
    else
	while (nextentry())
	{
	    (void) artname(&art, artfile);
	    (void) obsolesce();
	}

    /*
     * Phase 3. Cleanup time. Adjust minimum-active-number fields 
     * to account for deleted messages.
     */
#ifndef DEBUG
    if (!noexpire)
#else
	if (!noexpire && !debug)
#endif				/* DEBUG */
	{
	    register int    i;

	    if (verbose >= V_SHOWPHASE)
		(void) fprintf(stdout, "Updating active article bounds...\n");

	    ngrewind(TRUE);
	    while (ngnext())
	    {
		/* don't forget: here the i bit is TRUE if */
		/* article i is obsolete  */
		for (i = ngmin(); i <= ngmax() + 1; i++)
		    if (getbit(i, ngactive()) != TRUE)
			break;
		mkngmin(i);
	    }
	}

#ifdef MARKPARENTS
    /*
     * Phase 4: remove out-of-date missing-parent entries
     */
    cleanparents();
#endif /* MARKPARENTS */

    /*
     * Phase 5: remove empty spool directories
     */
    if (!noexpire)
    {
	if (verbose >= V_SHOWPHASE)
	    (void) fprintf(stdout, "Deleting empty spool directories...\n");
	cleandirs();
    }

    /*
     * Phase 6: if not in fastmode, rename the appropriate files
     */
    if (!fastmode)
    {
	move_nhist("dat");
	move_nhist("dir");
	move_nhist("pag");
    }
	
    /*
     * Phase 7: print statistics to stdout
     */
    if (verbose >= V_SHOWSTATS)
	printstats();

    xxit(0);
    /*NOTREACHED*/
}

#ifdef ENTRYLOCK
#ifndef lint	/* this gets us around a bug in lint on the 3B1 */
private
#endif /* lint */
bool keepminart(grnew, grold)
/* preserve information from an in-core record */
group_t	*grnew, *grold;
{
    grnew->ng_min = grold->ng_min;	/* rescue computed ngmin numbers */
    grnew->ng_age = grold->ng_age;	/* and the last-posting date */
    return(FALSE);
}
#endif

catch_t xxit(i)
{
    /*
     * If we haven't already locked the news database for exclusive use
     * now is the time to do it. And if we're doing it now, let's reread
     * the active file with a hook to pick up any changes in maximum
     * article number made since expire started running.
     */
#ifdef DEBUG
    if (!debug)
#endif /* DEBUG */
    if (!rebuild)
    {
	if (!lockp())
		lock();
#ifdef ENTRYLOCK
	(void) rdactive(keepminart);
#endif
    }

    /*
     * Phase 6: write out the modified active and history data. The
     * noexpire flag isn't checked up here because it applies on a lower
     * level within the update function.
     */
    /* Only write active file if we're in fastmode */
#ifndef SPOOLNEWS
    if (fastmode)
#endif
        if (dowractive)
	    wractive(TRUE);
#ifdef DEBUG
    if (debug)
         hstwrite();	/* write the modified history file */
#endif /* DEBUG */

#ifndef ENTRYLOCK
    (void) privunlock();
#endif /* ENTRYLOCK */

    if (!nosend && !noexpire && sigcaught != SIGQUIT)
    {
	(void) sprintf(bfr, "%s/rnews", site.libdir);
	if (verbose > V_SHOWPHASE)
	    (void) fprintf(stdout, "Running %s -U\n", bfr);
	(void) execl(bfr, "rnews", "-U", (char *)NULL);
	perror("exec of rnews -U failed\n");
    }
#ifdef PROFILE
    monitor(0);
#endif
    exit(i);
}

private bool gethdr(txtfile)
/* bring in the header of a given article file */
char	*txtfile;   /* the article file */
{
    FILE	    *fp;

    /* if we already have the message header, no need to re-fetch */
    if (havehdr)
	return(TRUE);

    /* grab the message header */
    errno = 0;
    hfree(&header);
    if ((fp = msgopen(txtfile)) == (FILE*)NULL || hread(&header, 0L, fp) == 0)
    {
	if (errno == 0)		 /* header read failed */
	{
	    if (filesize(txtfile) == 0)
	    {
		logerr0("article file is zero-length");
#ifdef DEBUG
		if (debug == 0)
#endif /* DEBUG */
		    (void) unlink(txtfile);
	    }
	    else
		logerr1("header on %s looks garbled", txtfile);
		
	    (void) fclose(fp);
	    return(FALSE);
	}

	/*
	 * Missing articles are no sweat if the current group is volatile,
	 * they probably went away in Phase 1.
	 */
	if (errno == ENOENT && ngflag(NG_VOLATILE))
	    return(FALSE);  /* fopen() failed, don't have to fclose() here */
	else
	{
	    logerr2("can't read article %s, errno = %d.", txtfile, errno);
#ifdef DEBUG
	    if (!debug)
#endif /* DEBUG */
	    {
		/*
		 * Usually means disk ran out of space, forget this article
		 * from our history file completely, so we have a chance of
		 * picking it up again from another feed ..
		 */
		hstdrop();
	    }
	}

	return(FALSE);
    }

    (void) msgclose(fp);    /* don't need to see the article text */
    return(TRUE);
}

private bool expired()
/* is the current article expired? */
{
    time_t	creatdat;
    expstat	*stats = &statistics[active.article.m_group->ng_findex];

    /* clean out messages left in by aborted expire runs */
    if (active.article.m_number && (active.article.m_number < ngmin()))
    {
	(void) fprintf(stdout,
	    "Expiring %s/%ld (out of range)\n",
		       ngname(), (long)active.article.m_number);
	stats->drops.total++;
	return(TRUE);
    }

    /* seen messages in volatile groups are considered expired */
    if (ngflag(NG_VOLATILE))
	if (getbit(msgnum(), ngactive()) != FALSE || !rcflag(RC_HASSUBS))
	{
	    if (verbose >= V_SHOWACTS)
		(void) fprintf(stdout,
			       "Expiring %s/%ld (seen by all)\n",
			       ngname(), (long)msgnum());
	    stats->drops.volatiles++;
	    stats->drops.total++;
	    return(TRUE);
	}
	else
	    stats->keeps.volatiles++;

    /*
     * We need to read the message header if the current expiration date
     * formula requires looking at the sender or the posting date field.
     */
    if (frflag || usepost)
	if (!gethdr(artfile))
	{
	    (void) fprintf(stdout,
		"Expiring %s/%ld (couldn't get header)\n",
		ngname(), (long)active.article.m_number);
	    stats->drops.total++;
	    return(TRUE);
	}

    /* if we're tossing everything posted by a given user, check that */
    if (frflag && strcmp(baduser,header.h_from) == 0)
    {
	if (verbose >= V_SHOWACTS)
	    (void) fprintf(stdout,
		"Expiring %s/%ld (posted by %s)\n",
		ngname(), (long)active.article.m_number, baduser);
	stats->drops.total++;
	return(TRUE);
    }

    /* compute the creation time of the article */
    if (usepost)
	creatdat = header.h_posttime;  /* use the post date from the header */
    else
	creatdat = hstdate();	/* use the 'creation' time of the file */

    /* count daily incoming volume */
    if (now <= creatdat + DAYS)
	fromtoday++;

#ifndef TMNCONVERT
    /*
     * if we haven't been told to ignore explicit expire dates
     * and the header contains one, use it.
     */
    if (!ignorexp && !ngflag(NG_IGNEXPDATE) && hstexp()
					&& (!ignorold || hstexp() < creatdat))
    {
	if (now >= hstexp())
	{
	    (void) fprintf(stdout, "Expiring %s (explicit date)\n", artfile);
	    stats->drops.explicits++;
	    stats->drops.total++;
	    return(TRUE);
	}
	else
	    stats->keeps.explicits++;
    }
    else
#endif /* TMNCONVERT */
    {
	/* here's the default expiration formula */
	if (ngflag(NG_EXPIREIT))
	{
	    time_t	lifetime;

	    if (expincr != 0)
		lifetime = expincr;
	    else if (ngexpire() != (time_t)0)
		lifetime = ngexpire();
	    else
		lifetime = DFLTEXP;

	    if (now >= creatdat + lifetime)
	    {
		(void) fprintf(stdout,
			       "Expiring %s/%ld (out-of-date, %ld days)\n",
			       ngname(), (long)active.article.m_number,
			       lifetime / DAYS);
		stats->drops.implicits++;
		stats->drops.total++;
		return(TRUE);
	    }
	}
	else
	    stats->keeps.implicits++;
    }

    /* none of the criteria was met; keep the article */
    if (verbose >= V_SHOWACTS)
	(void) fprintf(stdout, "Keeping %s/%ld\n",
				    ngname(), (long)active.article.m_number);
    stats->keeps.total++;
    return(FALSE);
}

private bool obsolesce()
/*
 * Return TRUE or FALSE according as the message is/is not obsolete.
 * This routine leaves the newsgroups[] bitmasks set so that bits
 * corresponding to expired messages are FALSE, and bits corresponding
 * to unexpired messages are TRUE.
 */
{
    void	    archiveit();

    /*
     * Skip groups marked unsubscribed in pass 1. Note that this use of
     * the RC_UNSEL bit relies on the fact that rdnewsrc() won't pick up
     * user -n options.
     */
    if (rcflag(RC_UNSEL))
	return(FALSE);

    /* now figure out if it's expired */
    if (!noexpire && expired()) {
	/* mark the message deleted */
	(void) setbit(active.article.m_number, ngactive());
	statistics[active.article.m_group->ng_findex].unlinked++;

#ifdef DEBUG
	if (debug)
	{
	    if (ngflag(NG_ARCHIVEIT) && gethdr(artfile))
		archiveit(artfile, header.h_posttime);
	}
	else
#endif /* DEBUG */
	{
	    (void) hstexpire();		/* get rid of the history entry */
	    ngfset(NG_CHANGED);		/* mark current group changed */

	    /* you may want to archive the expired message */
	    if (ngflag(NG_ARCHIVEIT) && gethdr(artfile))
		archiveit(artfile, header.h_posttime);

	    /* perform the actual file unlink */
	    if (unlink(artfile) < 0)
	    {
		unlinks_failed++;
		if (errno == ENOENT)
		    unlinks_noent++;
		logerr2("unlink(%s): %s\n", artfile, errmsg(errno));
	    }
	}

	return(TRUE);
    }
    else	/* the article isn't expired, we want to keep its history */
    {
	/* mark the message kept */
	(void) clearbit(active.article.m_number, ngactive());
	if (!fastmode)
	    hstenter(hstline());	/* Copy it to the new history file */
	return(FALSE);
    }
}

#ifdef MARKPARENTS
private void cleanparents()
/* clean out out-of-date parent references */
{
    char	oldparents[BUFLEN], newparents[BUFLEN];
    FILE	*old, *new;

    (void) sprintf(oldparents, "%s/parents", site.admdir);
    (void) sprintf(newparents, "%s/parents.new", site.admdir);
    if (fopen(oldparents, "r") == NULL || fopen(newparents, "w") == NULL)
	logerr("couldn't open files to clean out parents list");
    else
    {
#ifdef VMS
	(void) vmsdelete(newparents);
#endif /* VMS */

	while (fgets(bfr, BUFLEN, old) != (char *)NULL)
	{
	    (void) strtok(bfr, " \t");	    /* skip parent ID */
	    if (atol(strtok(NULL, " \t")) + HISTEXP >= now)
		(void) fputs(bfr, new);
	}
	(void) fclose(old);
	(void) fclose(new);
#ifdef VMS
	(void) vmsdelete(oldparents);
#endif /* VMS */
	if (rename(newparents, oldparents) < 0)
	    logerr1("Cannot rename new parents file to %s", oldparents);
    }
}
#endif /* MARKPARENTS */

private void cleandirs()
/* clear out empty spool directories */
{
    int	    deletecount;

    /*
     * This should always be last. We scan backwards on the assumption that
     * subgroups are usually listed after their parent groups in the active
     * file, and for directory-removal purposes we want to encounter (say)
     * comp.lang.c before comp.lang (the rest of the code doesn't care).
     * As long as we've deleted a single group we come around and do another
     * pass through the active file, guaranteeing that every empty spool
     * directory will eventually get clobbered.
     */

    do {
	deletecount = 0;
	ngrewind(FALSE);
	while (ngback())
	{
	    /* no active messages? */
	    if (rcsubsc(ngactive()) && ngmax() < ngmin())
	    {
		char *directory = artdir(ngname());

		/* if the directory is already gone, don't bother */
		if (access(directory, F_OK) == FAIL)
		    continue;

#ifdef DEBUG
		if (!debug && rmdir(directory) == SUCCEED)
#endif /* DEBUG */
		if (rmdir(directory) == SUCCEED)
		{ 
		    (void) fprintf(stdout,
			"Directory %s removed (no articles left)\n",directory);
		    deletecount++;
		    if (ngactive()->ng_age != NO_AGE
					&& ngactive()->ng_age + HISTEXP < now)
			ngdelete();
		}
	    }
	}
    } while
	(deletecount);
}

private int nextentry()
/* seek to the message location corresponding to the next article listed */
{
    place_t	exploc;
    int		status;

    havehdr = FALSE;		/* invalidate the previous header info */

    for (;;)
    {
	/* try to pick up an article location from the history file */
	while ((status = hstloc(&exploc)) != SUCCEED)
	{
	    if (status == GARBLED)
	    {
		logerr2("invalid newsgroup/article, line %ld: \"%s\"",
			(long)hstcount, hstline());
		grpunknown++;
	    }
	    else if (status==EXPIRED || status==CANCELLED || status==REFERENCE)
	    {
		time_t	hdate = hstdate();
#ifdef DEBUG
		if (verbose >= V_SHOWLOC)
		    (void) fprintf(stderr,
				   "id %s, line %ld ignored, status %d\n",
				   hstid(), (long)hstcount, status);

#endif
		/*
		 * No locations listed. This happens
		 * when there's no message in the
		 * spool directory, only a forward
		 * reference to or the memory of it in
		 * the history file. Use date in the
		 * history file to decide if we should
		 * keep this history line.
		 */
		if (now >= hdate + forgetincr)
		{
		    if (verbose >= V_SHOWACTS)
			(void) fprintf(stdout,
				       "Dropping history of %s - %s\n",
				       hstid(), arpadate(&hdate));
		    forgetcount++;
#ifdef DEBUG
		    if (!debug)
#endif				/* DEBUG */
			if (fastmode)
			    
			    hstdrop();
		}
		else
		{
		    if (!fastmode)
			/* Copy line over to */
			/* new history file  */ 
			hstenter(hstline());
		}
	    }
	    /*
	     * if we got here, status is FAIL or we've
	     * performed all actions needed to handle
	     * cancelled or expired articles, so we
	     * probably want to go to the next ID if
	     * possible
	     */
	    if ((status = hstnext(TRUE)) == FAIL)
		return(FALSE);
	    else if (status == GARBLED) {
		hstcount++;
		hsterrcount++;
		logerr2("history line %ld no good: %s",
			(long)hstcount, hstline());
	    }
	    else
	    {
		hstcount++;
		if (verbose >= V_SHOWLOC)
		    (void) fprintf(stdout,
				   "Looking at ID %s\n", hstid());
	    }
	}

	/* now attempt to go to the location extracted */
	if (!AOK(exploc.m_group, exploc.m_number))
	{
	    if (verbose >= V_SHOWACTS)
		(void) fprintf(stdout,
			       "article out of range: \"%s/%d\"\n", 
			       exploc.m_group->ng_name, exploc.m_number);
	    /*
	     * if we failed, the article name is
	     * invalid, so drop it hoping we can
	     * pick it up from another feed
	     */
#ifdef DEBUG
	    if (!debug)
#endif				/* DEBUG */
#ifdef DBHISTORY
		if (fastmode)
#endif
		    (void) hstexpire();

	    /*
	     * Also, mark the article for deletion
	     * so that correction of the
	     * minimum-active number in the group
	     * is done correctly
	     */
	    (void) setbit(exploc.m_number, exploc.m_group);
	    continue;
	} else {
	    TO(exploc);
#ifdef DEBUG
	    if (verbose >= V_SHOWLOC)
		(void) fprintf(stdout,
			"Looking at location %s/%ld with flags 0x%x\n",
			ngname(), (long)msgnum(), art.m_group->ng_flags);
#endif /* DEBUG */
	    break;
	}
    }

    /* if we got this far, we've got a valid article */
    return(TRUE);
}

int build(dirname, entryname, mode)
/* this is the function spoolwalk will apply to rebuild the database */
char	*dirname, *entryname;
int	mode;
{
    static nart_t	acount;
    extern group_t	*ngalloc();
    
    switch (mode)
    {
    case TW_DIRECTORY:		/* about to start on a new directory */
	if (verbose >= V_SHOWLOC)
	    (void) fprintf(stdout, "Searching %s\n", dirname);
	acount = (nart_t)0;
	return(SUCCEED);

    case TW_GROUP:		/* looking at a real group */
	if (active.article.m_group == (group_t *)NULL)
	{
	    log1("%s not in active file, adding it", entryname);
	    ngcreate(entryname, FALSE);
	    ngselect(ngfind(entryname));
	    ngfset(NG_CHANGED);
	}
	return(SUCCEED);

    case TW_ARTICLE:		/* looking at a real article */
	if (verbose >= V_SHOWLOC)
	    (void) fprintf(stdout, "Looking at %ld\n", (long)art.m_number);

	/* update the active file information */
	if (art.m_number < art.m_group->ng_min || acount == 0)
	    art.m_group->ng_min = art.m_number;
	if (art.m_number > art.m_group->ng_max)
	    art.m_group->ng_max = art.m_number;
	acount++;

	/* we need the header for any further actions */
	(void) artname(&active.article, artfile);
	if (!gethdr(artfile))
	{
	    logerr1("missing or garbled header on %s", artfile);
	    return(SUCCEED);
	}

	/* add the history file information */
	if (header.h_rectime == (time_t)0)
	    header.h_rectime = modtime(artfile);
	(void) hstadd(header.h_ident,
		      header.h_rectime, header.h_exptime,
		      ngname(), active.article.m_number);
	hbuilds++;

	/*
	 * Per-article conversion code for moving old files to TMNN and later
	 * versions should live here, controlled by command options.
	 */
	if (convert)
	    hstparent(&header);
	return(SUCCEED);

     case TW_BADNUMBER:		/* entry no good */
	logerr2("can't get article \"%s\" in %s", entryname, dirname);
	/*
	 * Mark the article for deletion so that correction of
	 * the minimum-active number in the group is done correctly 
	 */
	(void) setbit(active.article.m_number, active.article.m_group);
	return(SUCCEED);
    }
    /*NOTREACHED*/
}

private void archiveit(fn, adate)
/* archive the given article file if this is called for */
char	*fn;	/* filename to back up */
time_t	adate;	/* date to set as last-access time */
{
    time_t	fntime = modtime(fn);
    struct tm	*tm;
    char    	*grpdir = fn + strlen(site.textdir) + 1;
    char	*unique = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

    (void) sprintf(bfr, "%s/%s", site.archdir, grpdir);

    /*
     * Remove group number and use the creation date of the
     * article. This way we're guaranteed uniqueness even if the
     * active file gets reset. The weird format is to guarantee
     * less than 14-character filenames.  The unique string is used to
     * try to not stomp on an existing archive.
     */
    tm = localtime(&fntime);
    do {
        (void) sprintf(strrchr(bfr, '/') + 1,
#ifdef USG
		"%2.2d%2.2d%2.2d:%2.2d%2.2d.%2.2d%c",
#else /* !USG */
		"%02d%02d%02d:%02d%02d%02d%c",
#endif /* !USG */
		tm->tm_year % 100, tm->tm_mon+1, tm->tm_mday,
		tm->tm_hour, tm->tm_min, tm->tm_sec, *unique);
    } while
	(access(bfr,0) != FAIL && *++unique != '\0');

    if (verbose >= V_SHOWACTS)
	(void) fprintf(stdout, "Archiving %s to %s\n", fn, bfr);

#ifdef	DEBUG
    if (debug)
	return;
#endif

    statistics[active.article.m_group->ng_findex].archived++;
    if (link(fn, bfr) == FAIL)
    {
	char *lastslash;

	lastslash = strrchr(bfr, '/');
	lastslash[0] = '\0';
	if (mkbranch(bfr, 0777) == SUCCEED)
	{
	    lastslash[0] = '/';
	    if (link(fn, bfr) == FAIL)
		(void) fcopy(fn, bfr);
	}
	else
	    xerror2("Can not mkdir %s: %s", bfr, errmsg(errno));
	setmodtime(bfr, adate);
    }
}

private void printstats()
/* print expiration statistics */
{
    register int gc, ec, dc;
    expstat *ep, *totals = &statistics[ctlines];

    gc = ec = dc = 0;
    ngrewind(TRUE);
    while (ngnext())
    {
	if (ngflag(NG_REMOVED))
	{
	    (void) printf("%s removed\n", ngname());
	    dc++;
	}
	if (ngflag(NG_MODERATED))
	    gc++;
	if (ngflag(NG_EXPIRE))
	    ec++;
    }
    if (dc)
	(void) putchar('\n');

    (void) printf("%11ld newsgroups in active file\n", (long) active.ngc);
    (void) printf("%11ld moderated groups in active file\n", (long)gc);
    (void) printf("%11ld groups not in active file\n\n", (long)grpunknown);

    (void) printf("%11ld history lines\n", (long)hstcount);
    (void) printf("%11ld history lines invalid\n", (long)hsterrcount);
    (void) printf("%11ld history lines with default expiry\n\n",
		  (long)active.ngc-ec);

    (void) printf("%11ld new articles came in today\n\n", fromtoday);

    /* figure global totals of stuff */
    for (ep = &statistics[0]; ep < statistics + ctlines; ep++)
    {
	totals->keeps.implicits += ep->keeps.implicits;
	totals->keeps.explicits += ep->keeps.explicits;
	totals->keeps.volatiles += ep->keeps.volatiles;
	totals->keeps.total += ep->keeps.total;

	totals->drops.implicits += ep->drops.implicits;
	totals->drops.explicits += ep->drops.explicits;
	totals->drops.volatiles += ep->drops.volatiles;
	totals->drops.total += ep->drops.total;

	totals->archived += ep->archived;
	totals->unlinked += ep->unlinked;
    }

    /* print out statistics for each control line, then totals */
    for (ep = &statistics[0]; ep <= statistics + ctlines; ep++)
    {
	if (ep == statistics + ctlines)
	    (void) printf("\nTotals:\n");
	else
	{
	    if (ep->keeps.total == 0 && ep->drops.total == 0)
		continue;
	    (void) printf("\nControl line: %s\n", *ctrllines++);
	}

	(void) printf("Kept (default expire): %ld\n",
		      (long)ep->keeps.implicits);
	(void) printf("Kept (explicit expire): %ld\n",
		      (long)ep->keeps.explicits);
	(void) printf("Kept (volatile expire): %ld\n",
		      (long)ep->keeps.volatiles);
	(void) printf("Kept (total): %ld\n",
		      (long)ep->keeps.total);

	if (gc = (ep->keeps.total -
		  ep->keeps.implicits+ep->keeps.explicits+ep->keeps.volatiles))
	    (void) printf("**MISMATCH** %ld additional keeps\n", (long) gc);

	(void) printf("Dropped (default expire): %ld\n",
		      (long)ep->drops.implicits);
	(void) printf("Dropped (explicit expire): %ld\n",
		      (long)ep->drops.explicits);
	(void) printf("Dropped (volatile expire): %ld\n",
		      (long)ep->drops.volatiles);
	(void) printf("Dropped (total): %ld\n",
		      (long)ep->drops.total);

	if (gc = (ep->drops.total -
		  ep->drops.implicits-ep->drops.explicits-ep->drops.volatiles))
	    (void) printf("**MISMATCH** %ld additional drops\n", (long)gc);

	(void) printf("Article links archived: %ld\n", (long)ep->archived);
	(void) printf("Article links unlinked: %ld\n", (long)ep->unlinked);

	if (gc = (ep->archived + ep->unlinked - ep->drops.total))
	    (void) printf("**MISMATCH** %ld too few drops\n", (long)gc);
    }

    if (unlinks_noent)
	(void) printf("%11ld unlinks of nonexistent articles\n",
		      (long)unlinks_noent);
    if (unlinks_failed)
	(void) printf("%11ld article unlinks failed\n", (long)unlinks_failed);
    if (hbuilds)
	(void) printf("%11ld output history lines\n", (long)hbuilds);
    (void) fflush(stdout);
}

#ifdef lint
/*ARGSUSED0*/
void encode(ifp, ofp) FILE *ifp, *ofp; {/*no-op*/}
#endif /* lint */

move_nhist(ext)
	char	*ext;
{
    char	*from, *to, *old;

    Sprint2(from, "%s.%s", NHISTORY, ext);
    Sprint2(to, "%s.%s", HISTORY, ext);
    Sprint2(old, "%s.%s", OHISTORY, ext);
	
#ifdef VMS
    (void) vmsdelete(old);
#endif				/* VMS */
    if (rename(to, old) < 0)
	logerr2("Cannot rename %s to %s", to, old );
    if (rename(from, to) < 0)
	logerr2("Cannot rename %s to %s", from, to );
    free(from);
    free(to);
    free(old);
}

touch_nhist(ext)
char	*ext;
{
    int	fd;
	
    (void) sprintf(bfr, "%s.%s", NHISTORY, ext);
    if ((fd = open(bfr, O_RDONLY|O_CREAT|O_TRUNC, 0644)) < 0)
	xerror2("Can't create %s, errno is %d", bfr, errno);
    if (close(fd) < 0)
	xerror2("Can't close %s, errno is %d", bfr, errno);
    (void) chown(bfr, NEWSUID, NEWSGID);
}

/* expire.c ends here */
