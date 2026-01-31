/*****************************************************************************

NAME
   fascist -- perform authorization check on posting usage

SYNOPSIS
   char *getgrplist(user)		-- return list of all groups use is in
   char *user;

   nasty_t *fascist(user)		-- return data on security restrictions
   char *user;

   bool allmatch(grps, restrict)	-- TRUE if all groups match restriction
   char *grps, *restrict;

DESCRIPTION
   The fascist() function returns a pointer to a structure describing security
restrictions on the given user. The contents of the structure is mined out of
the ADM/authorized file.

   The ADM/authorized file consists of a series of lines having the following
format (with all lists comma-separated).

	g[roup]:<name>:<usernames>[:<gatekeepernames>]
	p[ermit]:<group>:<subscription>[:<subscription>]
	f[orbid]:<group>:<subscription>[:<subscription>]
	a[llow]:<user>:<subscription>[:<subscription>]
	d[eny]:<user>:<subscription>[:<subscription>]
	s[uppress]:<sitelist>:<subscription>

   The `g' command declares a security group. The `usernames' and
`gatekeepernames' fields should be lists of user login names. Group
`gatekeepers' have permission to add or delete users from their group.
The gatekeeper field is optional; the news user has gatekeeper privileges for
every security group.

   The `permit' command declares that every user and gatekeeper of the given
group gets the post (and, if present, read) permissions specified. The read
subscription field is optional.

   The `forbid' command acts like a permit, except that it locks matching users
out of groups matching the specified subscriptions.

   The `allow' command declares that the named user gets the post (and, if
present, read) permissions specified. The read subscription field is optional.
If the `named user' is `*', this line is applied to all users.

   The `deny' command acts like an allow, except that it locks the user
out of groups matching the specified subscriptions.

   The `suppress' takes a sitename or list of sitenames and a subscription.
Users at listed sites will not be allowed to post to any group matching the
subscription.

   If a given user matches several entries, the effect will be as if the
subscriptions corresponding to those entries were concatenated.

   If the username is not in the file ADM/authorized then the defaults in the
symbols FASCIST and COMMUNIST are interpreted as the subscription fields of an
`allow' command for that user. Thus, FASCIST and COMMUNIST (both of which can
be overridden from the attributes file if the RUNTIME option is on) act as
global default write and read permissions.

For example:

    a:root:all
    a:miscreant:junk,talk.politics:!local.security
    g:wizards:gandalf,radagast:saruman
    p:wizards:comp.unix.wizards
    s:mordor,orthanc:alt.goodguys

   The first line authorizes root to post to all groups. The second allows user
\fImiscreant\fR to post to junk and talk.politics only, and also forbids
\fImiscreant\fR from reading the local.security newsgroups. The third declares
a group \fIwizards\fR consisting of users gandalf, radagast and saruman, and
gives user saruman the ability to add or delete users from the group. The
fourth allows anyone in the group \fBwizards\fI to post to comp.unix.wizards.
The fifth suppresses attempts by anybody at sites mordor or orthanc to post to
alt.goodguys.

   Permit/deny entries for group names that have not been declared with a
previous group entry are assumed to correspond to real permission groups in
/etc/group, and the group's user list is looked up there. There is no way to
declare gatekeepers for such groups; anyone who can edit /etc/group
automatically has gatekeeper privileges for all of them.

   All text after a # character is ignored. This feature can be used to insert
comment lines.

   An open environment could have FASCIST set to "all:all" and then individual
entries could be made in the authorized file to prevent certain individuals
from posting to such a wide area.

   Note that a distribution of "all" does NOT mean to allow postings only to
local groups -- "all" includes "all.all". Use "all,!all.all" or "any" to
get this behavior.

FILES
   ADM/authorized

AUTHORS
   Original code by Eugene Spafford (spaf@gatech) May 22, 1985, rewritten by
Eric S. Raymond
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

****************************************************************************/
/* LINTLIBRARY */
#include "news.h"
#ifdef FASCIST
#include "fascist.h"
#endif /* FASCIST */

char *getgrplist(user)
/*
 * This routine is meant to be called only once.  On a system with a
 * large /etc/group file, this routine is a HOG!!!!!  In order to save
 * ourselves from pain, this routine will look up groups for "user"
 * the first time.  After that, it will always return the same results...
 */
register char *user;
{
    register struct group *gr;
    register struct passwd *pw;
    register char **cp;
    register int len;
    static int grpdone = FALSE;
    static char grplist[LBUFLEN];
    size_t remaining;

    if (grpdone == FALSE)
    {
	grplist[0] = '\0';
	remaining = sizeof(grplist) - 1;
	pw = getpwnam(user);
#ifndef lint	/* USG and BSD disagree on the type of setgrent() */
	setgrent();
#endif /* lint */
	while (gr = getgrent())
	{
	    len = strlen(gr->gr_name) + 1; /* +1 for comma */
	    if (pw != (struct passwd *)NULL && pw->pw_gid == gr->gr_gid)
	    {
		if (len < remaining) {
		    (void) strcat(grplist, gr->gr_name);
		    (void) strcat(grplist, ",");
		    remaining -= len;
		}
		continue;
	    }	
	    for (cp = gr->gr_mem; cp && *cp; cp++)
		if (strcmp(*cp, user) == 0) {
		    if (len < remaining) {
			(void) strcat(grplist, gr->gr_name);
			(void) strcat(grplist, ",");
			remaining -= len;
		    }
		    break;
		}
	}
	if ((len = strlen(grplist)))
	    grplist[len-1] = '\0';
#ifndef lint	/* USG and BSD disagree on the type of endgrent() */
	endgrent();
#endif /* lint */
	grpdone = TRUE;
    }
    return(grplist);
}

#ifdef FASCIST

/*
 * SAFE_STRCAT: Bounds-checked string concatenation
 * Returns early if buffer would overflow (defensive truncation)
 * 
 * Fixed by ReviewBot-774 (Issue #25)
 * 8 unbounded strcat() calls -> strlcat() with BUFLEN bounds
 */
#define SAFE_STRCAT(dst, src) \
    do { if (strlcat(dst, src, BUFLEN) >= BUFLEN) return; } while(0)

private void addrestrict(neg, rblk, psubsc, rsubsc)
/* add restrictions corresponding to given attribute line to rblk */
bool	neg;
nasty_t	*rblk;
char	*psubsc;
char	*rsubsc;
{
    /* add post restrictions */
    if (rblk->n_post[0])
	SAFE_STRCAT(rblk->n_post, ",");
    else
	rblk->n_post[0] = '\0';
    if (neg)
	SAFE_STRCAT(rblk->n_post, "!{");
    SAFE_STRCAT(rblk->n_post, psubsc);  /* user input - now bounded */
    if (neg)
	SAFE_STRCAT(rblk->n_post, "}");

    /* add read restrictions */
    if (rblk->n_read[0])
	SAFE_STRCAT(rblk->n_read, ",");
    else
	rblk->n_read[0] = '\0';
    if (neg)
	SAFE_STRCAT(rblk->n_read, "!{");
    SAFE_STRCAT(rblk->n_read, rsubsc);  /* user input - now bounded */
    if (neg)
	SAFE_STRCAT(rblk->n_read, "}");
}

#undef SAFE_STRCAT

nasty_t *fascist(user)
register char *user;
{
    FILE *facfp;
    char grplist[BUFLEN], *cp;
    static char svread[BUFLEN], svpost[BUFLEN];
    static nasty_t	result;

    result.n_post = svpost; svpost[0] = '\0';
    result.n_read = svread; svread[0] = '\0';
    (void) strncpy(grplist, getgrplist(user), sizeof(grplist) - 1);
    grplist[sizeof(grplist) - 1] = '\0';

    (void) snprintf(bfr, LBUFLEN, "%s/authorized", site.admdir);
    if ((facfp = fopen(bfr, "r")) != (FILE *)NULL)
    {
	char	*field[F_MAXFLDS + 1];

	while (fgets(bfr, BUFLEN, facfp) != (char *)NULL)
	{
	    bool	negate = FALSE;
	    int		fld;

	    if (cp = strchr(bfr, F_COMMENT))
		*cp = '\0';
	    if (bfr[0] == '\0')
		continue;
	    (void) nstrip(bfr);

	    /* crack line into fields */
	    field[fld = 0] = strtok(bfr, F_FIELDSEP);
	    while ((cp = strtok((char *)NULL, F_FIELDSEP)) != (char *)NULL)
		if (fld < F_MAXFLDS)
		    field[++fld] = cp;

	    /* lines with too few fields are invalid */
	    if (fld >= 2)
		field[++fld] = "";
	    else
		continue;

	    /* match based on entry type */
	    switch (*field[F_COMMAND])
	    {
	    case 'g':	/* group */
		if (ngmatch(user,field[F_MEMBERS]))
		{
		    (void) strcat(grplist, ",");
		    (void) strcat(grplist, field[F_GROUP]);
		}
		continue;

	    case 'f':	/* forbid */
		negate = TRUE;
		/* FALL THROUGH */
	    case 'p':	/* permit */
		if (!ngmatch(field[F_GROUP], grplist))
		    continue;
		break;

	    case 'd':	/* deny */
		negate = TRUE;
		/* FALL THROUGH */
	    case 'a':	/* allow */
		if (strcmp(field[F_USER], "*") && strcmp(field[F_USER], user))
		    continue;
		break;

	    case 's':	/* suppress */
		negate = TRUE;
		if (ngmatch(site.nodename, field[F_SITE]))
		    continue;
		break;

	    default:
		continue;
	    }

	    /* if we got here, we're looking at a matching p|f|a|d|s line */
	    addrestrict(negate, &result,field[F_POSTSUBSC],field[F_READSUBSC]);
	}
	(void) fclose(facfp);
    }

    if (result.n_post[0] == '\0')
	addrestrict(FALSE, &result, newsattr("fascist", FASCIST), "");
#ifdef COMMUNIST
    if (result.n_read[0] == '\0')
	addrestrict(FALSE, &result, "", newsattr("communist", COMMUNIST));
#endif /* COMMUNIST */

    return(&result);
}

bool allmatch(grps, restrict)
/* Step through the grps listed and check each against the restriction list */
char	*grps;
char	*restrict;
{
    char	matchlist[BUFLEN], *nextgrp;

    (void) strncpy(matchlist, grps, sizeof(matchlist) - 1);
    matchlist[sizeof(matchlist) - 1] = '\0';
    nextgrp = strtok(matchlist, LISTSEP);
    do {
	if (!ngmatch(nextgrp, restrict))
	    return(FALSE);
    } while
	((nextgrp = strtok((char *)NULL, LISTSEP)) != (char *)NULL);
    return(TRUE);
}
#endif /* FASCIST */

/* fascist.c ends here */
