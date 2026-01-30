/*****************************************************************************

NAME
   originate.c -- helper functions for originating news

SYNOPSIS
   #include "libpost.h"

   int originate(hp, moderator)		-- prepare a header for a new message
   hdr_t *hp; char; bool moderator;

   void fixaddress(faddr, addr)		-- canonicalize a mail address
   char *faddr, *addr;

   int checkincl(hp, file)		-- check for too many includes
   hdr_t *hp; char *file;

DESCRIPTION
   User-interface-independent functions for article origination.

   Originate() generates Path, From, Sender and Organization lines based on
the identity of the calling user and the contents of the argument header. If
NONLOCAL is configured it also generates a Message-ID line. The exact rules
are as follows:

   1) If the header contains a From: or Path: line, it assumes the From: and
Path: lines are that of a guest poster. If the From: line looks like an
Internet address, it's left alone. Otherwise an Internet address is faked up
for the guest and inserted in the From: line. Also, if the second argument
is TRUE, any Path: line in the header is left untouched.

   2) Otherwise, it generates standard From: and Path: headers based on the
caller's ID and site.

   If you compile with QUOTELIM on checkincl() will try to reject articles with
too much quoted text, but the algorithm may be shaky. What it tracks is the
number of lines that begin with a non-alphanumeric, non-whitespace printable
character. This is so users can't evade it by using things other than '>' as a
inclusion mark -- of course, tabs and spaces will slip by but rejecting
messages because many lines begin with whitespace seems to go too far. The
check is disabled on 'sources' and 'bugs' groups so we don't choke on diffs.

   If you don't like this, by all means roll your own! Remember that QUOTELIM
wants to be interpreted as a percentage and that LONGTEXT is available as the
presumed article-size threshhold above which the check should be done.

BUGS
   The return value of newpost() (the ID of the new article) lives in a common
static buffer and could get stomped if you don't copy it before using other
library functions.

FILES
   SPOOL/*		-- location for spooled mail

AUTHORS
   Eric S. Raymond
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

*****************************************************************************/
/*LINTLIBRARY*/
#include "news.h"
#include "header.h"
#include "libpost.h"

#define DEFAULTNG	"general"	/* default group to post to */

private void skin(name, full, hfield)
/* extract name and fullname from a header field */
char *name;
char *full;
char *hfield;
{
    register int    c;
    register char  *cp, *cp2;
    char   *bufend;
    int     gotlt, parenlev, lastsp;
    bool    seenfull = FALSE;

    *full = '\0';		/* no full name yet */
    if (strpbrk(hfield, "(< ") == (char *)NULL)
    {				/* include ',' ?? */
	(void) strcpy(name, hfield);
	return;
    }
    gotlt = 0;
    parenlev = 0;
    lastsp = 0;
    bufend = name;
    for (cp = hfield, cp2 = bufend; c = *cp++;)
    {
	switch (c)
	{
	case '(': 
	    /* 
	     * Start of a "comment".
	     * Ignore it, or save it in "full" if we haven't
	     * seen a comment yet.
	     */
	    parenlev++;
	    while ((c = *cp) != 0)
	    {
		cp++;
		switch (c)
		{
		    case '\\': 
			if ((c = *cp) == 0)
			    goto outcm;
			cp++;
			break;
		    case '(': 
			parenlev++;
			break;
		    case ')': 
			parenlev--;
			if (parenlev == 0)
			    goto outcm;
			break;
		}
		if (!seenfull)
		    *full++ = c;
	    }
    outcm: 
	    parenlev = 0;
	    lastsp = 0;
	    if (!seenfull)
	    {
		*full = '\0';
		seenfull = TRUE;/* only extract first comment */
	    }
	    break;

	case '"': 
	    /* 
	     * Start of a "quoted-string".
	     * Copy it in its entirety.
	     */
	    while ((c = *cp) != 0)
	    {
		cp++;
		switch (c)
		{
		    case '\\': 
			if ((c = *cp) == 0)
			    goto outqs;
			cp++;
			break;
		    case '"': 
			goto outqs;
		}
		*cp2++ = c;
	    }
    outqs: 
	    lastsp = 0;
	    break;

	case ' ': 
	    if (cp[0] == 'a' && cp[1] == 't' && cp[2] == ' ')
		cp += 3, *cp2++ = '@';
	    else
	    if (cp[0] == '@' && cp[1] == ' ')
		cp += 2, *cp2++ = '@';
	    else
		lastsp = 1;
	    break;

	case '<': 
	    if (!seenfull)
	    {
		*cp2 = '\0';
		(void) strcpy(full, name);
		seenfull = TRUE;
	    }
	    cp2 = bufend;
	    gotlt++;
	    lastsp = 0;
	    break;

	case '>': 
	    if (gotlt)
	    {
		gotlt = 0;
		/* 
		 * this doesn't seem reasonable, what about(,)
		 * or "," ??
		 */
		while (*cp != ',' && *cp != 0)
		    cp++;
		if (*cp == 0)
		    goto done;
		*cp2++ = ',';
		*cp2++ = ' ';
		bufend = cp2;
		break;
	    }
	    /* Fall into . . . */

	default: 
	    if (lastsp)
	    {
		lastsp = 0;
		*cp2++ = ' ';
	    }
	    *cp2++ = c;
	    break;
	}
    }
done: 
    *cp2 = 0;
}

void fixaddress(addr, faddr)
/*
 * Canonicalize the "From:" line into the form
 *
 * From: <mail-address> (full-name)
 *
 * RFC822 doesn't require the comment to be at the end of the string
 * like that.
 */
register char *addr, *faddr;
{
    char full[BUFLEN];

    skin(faddr, full, addr);    /* remove RFC822-style comments */

    if (full[0] != '\0')
    {
	(void) strcat(faddr, " (");
	(void) strcat(faddr, full);
	(void) strcat(faddr, ")");
    }
}

int originate(hp, moderator)
/*
 * Prepare header for origination of a message; submission date fields are set
 * here, NAME and ORGANIZATION fields are interpreted here and forged names in
 * From fields are handled (fascism checks are done in rnews.c). Note that
 * rnews's calls to hread() will bomb unless it sees a Path and From header and
 * a submission date header; these are generated here.
 */
hdr_t	*hp;	    /* header to fill in */
bool	moderator;	    /* moderator flag value */
{
    register char   *ptr;	/* pointer to rest of buffer		*/
    char	    sender[BUFLEN];

    /* bomb out if there's no title on the article */
    if (hlblank(hp->h_subject))
	return(FAIL);

    /* leave the posting date alone if we're passing a submission through */
    if (!moderator)
	hlfree(header.h_postdate);

    hlfree(header.h_sender);	/* can't let user muck with this! */

    /* compute the identity of the message's real sender */
    (void) sprintf(sender, "%s@%s (%s)",
		   username, site.fromname, fullname(username));

    /* if article already has a From or Path, treat like a guest sender ID */
    if (hlblank(header.h_from) && hlblank(header.h_path))
    {
	/* normal case, no From: or Path: header supplied */
	hlcpy(hp->h_from, sender);
	hlcpy(hp->h_path, username);
    }
    else	/* there's a non-blank From: */
    {
	hlcpy(hp->h_sender, sender);

	/*
	 * If a Path: was supplied, but no From: fake up an Internet
	 * address from the last two components of the path.
	 */
	if (hlblank(hp->h_from) && hlnblank(hp->h_path))
	{
	    char	path[BUFLEN];
	    char	*gsite, *name;

	    (void) strcpy(bfr, hp->h_path);
	    name = bfr + strlen(bfr) - 1;
	    while (name > bfr && *name != '!')
		name--;
	    *name++ = '\0';
	    if (name > bfr)
	    {
		gsite = name - 2;
		while (gsite > bfr && *gsite != '!')
		    gsite--;
	    }
	    else
		gsite = site.fromname;
	    (void) sprintf(path, "%s@%s", name, gsite);
	    hlcpy(hp->h_from, path);
	}

	/*
	 * If the user supplied or we derived something that looks like
	 * an internet address in the From: line, we go
	 * ahead and use it. Otherwise...
	 */
	if (strpbrk(hp->h_from,"@ <") == (char *)NULL)
	{
	    /* fake up an Internet address for the guest */
	    (void) sprintf(bfr, "%s@%s", hp->h_from, site.fromname);
	    hlcpy(hp->h_from, bfr);
	}

	/*
	 * Moderators can authorize faked paths back to the guest.
	 */
	if (!moderator || hlblank(hp->h_path))
	    hlcpy(hp->h_path, username);
    }

    /* enter submission date */
    hp->h_rectime = hp->h_posttime = time((time_t *)NULL);
    hlcpy(hp->h_postdate, arpadate(&hp->h_posttime));

    /* make sure there's a valid newsgroup here */
    if (hlblank(hp->h_newsgroups))
	/*
	 * no newsgroup header -- this may have been mailed in, let's see if
	 * we can kluge one up from the To: header.
	 */
	if (hlnblank(hp->h_to))
	{
	    char *sp, *tp;

	    /* if address is domainist, pick out the 'user' name */
	    if (sp = strrchr(hp->h_to, '@'))
	    {
		*sp = '\0';
		while (sp > hp->h_to && strchr(NETCHRS, *sp) == (char *)NULL)
		    sp--;
	    }

	    /* otherwise, if it's a pathname, use the last segment */
	    else if (sp = strrchr(hp->h_to, '!'))
		sp++;

	    /* no net chars in address, use the whole thing */
	    else
		sp = hp->h_to;

	    /* turn hyphens in the user name into dots */
	    for (tp = bfr; *sp; sp++)
		if (*sp == MAILSEP)
		    *tp++ = NGSEP;
	        else
		    *tp++ = *sp;
	    *tp = '\0';

	    hlcpy(hp->h_newsgroups, bfr);

	    /* group names should be lower case */
	    lcase(hp->h_newsgroups);
	    ptr = strchr(hp->h_newsgroups, '\0');
	    if (ptr[-1] == NGDELIM)
		*--ptr = '\0';
	}
	else
	    hlcpy(hp->h_newsgroups, savestr(DEFAULTNG));

    /*
     * we've extracted what we need from the To: header,
     * so trash it so it won't confuse hwrite()
     */
    hlfree(hp->h_to);

#ifdef MYORG
    /* fill in the Organization: field of the header */
    if (hlblank(hp->h_organization))
	hlcpy(hp->h_organization, organization());
#endif /* MYORG */

    return(SUCCEED);
}

int checkincl(hp, file)
/* return TRUE if the article is below limit on quoted material */
hdr_t	*hp;	/* header of the article (used for newsgroups etc) */
char	*file;	/* text of the article (header included) */
{
    int		includelines = 0, numlines = 0;
    FILE	*fp;
    int		longtext = atoi(newsattr("longtext", LONGTEXT));
    int		quotelim = atoi(newsattr("quotelim", QUOTELIM))/100;

    fp = fopen(file, "r");
    while (fgets(bfr, BUFLEN, fp) != NULL)
    {
	numlines++;
	/*
	 * if you have a different or more elaborate notion of quote inclusion
	 * that you want to detect, express it here with a different method
	 * of computing includelines.
	 */
	if (ispunct(bfr[0]) && !isspace(bfr[0]))
	    includelines++;
    }
    (void) fclose(fp);

    if (!ngmatch(header.h_newsgroups, "all.sources,all.bugs"))
	if (includelines/numlines > quotelim && numlines <= longtext)
	    return(includelines);
    return(0);
}

/* originate.c ends here */
