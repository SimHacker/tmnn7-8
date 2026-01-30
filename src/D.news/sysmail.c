/*****************************************************************************

NAME
   sysmail.c -- mailer interface for news transmission programs

SYNOPSIS
   FILE *mailopen(to, subject)	-- open a pipe to mail
   char *to, *subject;

   int mailclose(mfp)		-- close a mail pipe, sending the message
   FILE *mfp;

   char *mailreply(hptr)	-- compute a reply path for the given header
   hdr_t *hptr;

DESCRIPTION
   These functions provide a reasonably secure interface to the mail back end
for netnews programs. They function also try to do some optimization of address
paths (including a couple of really ugly hacks for specific networks and
machine types that are isolated in the pathopt() function)

NOTE
   Mailopen() writes Date, From, To, and Subject headers, but does not
follow them with a newline. This is a feature, not a bug -- it enables
callers to add their own header lines.

BUGS
   There are some really ugly-looking hacks in pathopt.c inherited from older
news versions. See the #ifdef Sun3 and BERKNET code. Bletch. Left in for
completeness; use at your own risk.

REVISED BY
   Eric S. Raymond
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

****************************************************************************/
/*LINTLIBRARY*/
#include "news.h"
#include "header.h"

#define V_BLABMAIL  1	/* at this verbose level or above, show mail command */
#define D_CATMAIL   2	/* at this debug level and above, just `cat` mail */

#define MAXPOSTS    32	/* maximum # of mail recipients to handle */

private char *pathopt(ptr)
/* try to optimize the given address for some known network quirks */
char	*ptr;
{
    static char address[PATHLEN];

    (void) strcpy(address, ptr);

#ifdef	SunIII
    /* special handling for Sun III networked machines only */
    if (ptr = strrchr(address, '.'))
    {
	if (prefix(++ptr, "OZ"))
	{
	    /* some people only allow it in lower case ... */
	    (void) strcpy(ptr, "oz");
	    return(address);
	}

	if (prefix(ptr, "UUCP") || prefix(ptr, "ARPA") ||
			prefix(ptr, "DEC") || prefix(ptr, "CSNET"))
	{
	    (void) strcat(address, "@munnari.oz");  /* via sun to munnari */
	    return(address);
	}
    }

    /*
     * must(?) have come from a uucp site, lets look see if path passes
     * through munnari, and if so delete the fake uucp path after that.
     */
    for (ptr = address ;; ptr++)
    {
	if (prefix(ptr, "munnari!"))
	{
	    (void) strcpy(address, ptr+8);
	    break;
	}
	ptr = strchr(ptr, PATHSEP);
	if (ptr == (char *)0)
	    break;
    }
    /*
     * now, just send the address we have left to munnari, and
     * hope that something sensible will be done with it there.
     * (This works in more cases than you'd think ...)
     */
    (void) strcat(address, "@munnari.oz");
    return(address);
#endif /* SunIII */

#ifdef BERKNET	/* this is a fossil, I think... -- esr */
    /*
     * Play games stripping off multiple berknet
     * addresses (a!b!c:d:e => a!b!d:e) here.
     */
    for (ptr = address; *ptr; ptr++)
    {
	register char *ptr2;

	if (strchr(NETCHRS,*ptr) && *ptr==':' && (ptr2=strchr(ptr+1, ':')))
	    (void) strcpy(ptr, ptr2);
    }
#endif /* BERKNET */

    /* If we're looking at a @-address, try to find a path to the Internet */
    if ((ptr = strchr(address, '@')) && strcmp(site.smarthost, "%s"))
    {
	char mbuf[BUFLEN];

	/* this avoids absurd routing through Internet if the mail is local */
	if (!strcmp(ptr+1, site.fromname) || !strcmp(ptr+1, site.nodename))
	{
	    *ptr = '\0';
	    return(address);
	}

	/* If we are lucky, there is no ! or @ in the forward address */
	(void) strcpy(mbuf, address);
	if (strpbrk(site.smarthost, "!@") == (char *)NULL)
	    (void) sprintf(address, site.smarthost, mbuf);
	else
	{
	    char *cp = strchr(mbuf, '@');

	    if (strchr(site.smarthost, '@') == (char *)NULL && cp)
	    {
		/* we have to rearrange the address so no @ are in it */
		char aaddress[BUFLEN];

		*cp++ = '\0';
		(void) sprintf(aaddress, "%s!%s", cp, mbuf);
		(void) sprintf(address, site.smarthost, aaddress);
	    }
	    else if (cp)
	    {
		/* some days you don't get lucky. presume the % hack */
		*cp = '%';
		(void) sprintf(address, site.smarthost, mbuf);
	    }
	}
    }

    return(address);
}

FILE *mailopen(sendto, subject)
/*
 * Return a file pointer to a mailer ready to accept a text message.
 * The sendto argument may be a whitespace-separated list of destinations.
 */
char *sendto, *subject;
{
    FILE    *fp;
    time_t  now;
    char    *cp, *addresses, mailcmd[LBUFLEN];

    /* perform all address translations */
    (void) strcpy(mailcmd, MAILSERV);
    addresses = mailcmd + strlen(mailcmd);
    cp = strtok(sendto, " \t");
    do {
	(void) strcat(mailcmd, " ");
	(void) strcat(mailcmd, pathopt(cp));
    } while
	(cp = strtok((char *)NULL, " \t"));

#ifdef DEBUG
    if (verbose >= V_BLABMAIL)
    {
	(void) fprintf(stderr, "sysmail(\"%s\", \"%s\") would do: %s\n",
		      sendto, subject, mailcmd);
	if (debug >= D_CATMAIL)
	    (void) strcpy(mailcmd, "/bin/cat");
    }
#endif /* DEBUG */

    /* call the mailer */
    if ((fp = peopen(mailcmd, "w")) == (FILE *)NULL)
	return((FILE *)NULL);

    /* generate standard headers onto the message */
    now = time((time_t *)NULL);
    (void) fprintf(fp, "Date: %s\n", arpadate(&now));
    (void)fprintf(fp, "From: %s@%s (%s)\n",
			username, site.fromname, fullname(username));
    (void) fprintf(fp, "To: %s\n", addresses);
    (void) fprintf(fp, "Subject: %s\n", subject);

    return(fp);
}

int mailclose(fp)
FILE	*fp;
{
    return(peclose(fp));
}


char *mailreply(hptr)
hdr_t *hptr;
{
    register char *ptr;
    static char address[PATHLEN];

    /* start with path name; throw away the name of this site if it leads */
    ptr = hptr->h_path;
    if (prefix(ptr,site.pathname) && strchr(NETCHRS,ptr[strlen(site.pathname)]))
	ptr = strchr(ptr, PATHSEP) + 1;

    /* use Reply-To: if poster specified it */
    if (hlnblank(hptr->h_replyto))
	ptr = hptr->h_replyto;

    /* if we're on Internet, there'll be an Internet 'user@site' usable */
    else if (site.smarthost[0] && hlnblank(hptr->h_from))
	ptr = hptr->h_from;

    /* discard any trailing full-name comment */
    (void) strcpy(address, ptr);
    if (ptr = strchr(address, '('))
    {
	while (ptr[-1] == ' ')
	    ptr--;
	*ptr = '\0';
    }

    return(pathopt(address));
}

/* sysmail.c ends here */
