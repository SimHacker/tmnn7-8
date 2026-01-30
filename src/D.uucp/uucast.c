/*****************************************************************************

NAME
   uucast.c -- multicast a UUCP command to a list of systems

SYNOPSIS
   int uucast(user, file, command, syslist, syscount)
   char *user, *file; char *command; char *syslist[]; int syscount;

DESCRIPTION
   This function allows a caller interested in spawning the same job to
multiple neighbor systems to economize spool space by starting jobs that
share a common data file.
   If all goes well, SUCCEED is returned. Otherwise errno is set and the
number of the system on which the multicast failed is returned.

NOTE
   If the WAITZ symbol is defined, the multicasting code will request
acknowledgement messages from the remote machines for each successful
transaction.

BUGS
   Depends on conventions for naming of the UUCP job control files that
vary from version to version of UUCP and are subject to change without
documented notice. However, this code seems to work OK for the major
versions.

SEE ALSO
   uulib.h	-- configuration defines for uucp manipulation
   uuq.c	-- get size of outgoing uucp queue for a system

AUTHOR
   Eric S. Raymond
adapted this from version 2.1 of Shane McCarron's uucast package.
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

*****************************************************************************/
/* LINTLIBRARY */

#include "news.h"
#include "libuucp.h"

static char	comname[SBUFLEN];	/* name of command file */
static char	remname[SBUFLEN];	/* name of remote command file */
static char	nremname[SBUFLEN];	/* name of remote data file */
static char	locname[SBUFLEN];	/* name of local data file */

int uucast(user, file, command, syslist, syscount)
char	*user;		/* name of user on behalf of which job should run */
char	*file;		/* file to be broadcast */
char	*command;	/* command to be executed */
char	*syslist[];	/* list of systems to multicast to */
int	syscount;	/* count of systems to multicast to */
{
    forward void    makename();
    char    path[BUFLEN];	/* temporary path name */
    FILE    *fptr;		/* file descriptor */
    int	    result;		/* result of link call */
    int	    i;			/* a mere looping dummy */
    int	    seq = getpid();	/* sequence number */
    
    /* for each system in the list, construct uux command files */
    for (i = 0; i < syscount; i++)
    {
	do {
	    makename(syslist[i], seq++);
#ifdef HONEYDANBER
	    (void) sprintf(path, "%s/%s/%s", XDIR, syslist[i], remname);
#else
	    (void) sprintf(path, "%s/%s", XDIR, remname);
#endif /* HONEYDANBER */
	} while
	    (!access(path, F_OK));

#ifdef HONEYDANBER
	(void) sprintf(path, "%s/%s/%s", DDIR, syslist[i], comname);
#else
	(void) sprintf(path, "%s/%s", DDIR, comname);
#endif /* HONEYDANBER */

	if ((result = link(file, path)) == FAIL && (errno == EXDEV))
	{
	    /*
	     * Link failed due to target directory being on
	     * a different file system.  Try doing a copy.
	     * If copy works then move path to file so that it
	     * can be used as a source to link the rest of the
	     * files.
	     */

	    /* changed to do strcpy only if copy works - MHC */

	    if (result = fcopy(file, path) == 0)
		(void) strcpy(file, path);	    
	}
	else if (result == FAIL)
	    return(i + 1);

#ifdef HONEYDANBER
	(void) sprintf(path, "%s/%s/%s", XDIR, syslist[i], remname);
#else
	(void) sprintf(path, "%s/%s", XDIR, remname);
#endif /* HONEYDANBER */
	if ((fptr = fopen(path, "w")) == (FILE *)NULL)
	    return(i + 1);
#ifdef	WANTZ
	(void) fprintf(fptr, "U %s %s\nN\nZ\nR %s\nF %s\nI %s\nC %s\n",
		user, site.nodename, user, comname, comname, command);
#else
	(void) fprintf(fptr, "U %s %s\nF %s\nI %s\nC %s\n",
		user, site.nodename, comname, comname, command);
#endif /* WANTZ */
	(void) fclose(fptr);
#ifdef HONEYDANBER
	(void) sprintf(path, "%s/%s/%s", CDIR, syslist[i], locname);
#else
	(void) sprintf(path, "%s/%s", CDIR, locname);
#endif /* HONEYDANBER */
	if ((fptr = fopen(path, "w")) == (FILE *)NULL)
	    return(i + 1);
#ifdef HONEYDANBER
	(void) fprintf(fptr,
		"S %s %s %s - %s 0666 %s\nS %s %s %s - %s 0666 %s\n",
		comname,comname, user, comname, user, remname, nremname,
		user, remname, user);
#else
	(void) fprintf(fptr,
		"S %s %s %s - %s 0666\nS %s %s %s - %s 0666\n",
		comname, comname, user, comname, remname, nremname,
		user, remname);
#endif /* HONEYDANBER */
	(void) fclose(fptr);
    }
    return(SUCCEED);
}

private void makename(sysnam, num)
/* make the names of the JCL files for UUCP multicasting */
char	*sysnam;			/* system name */
int	num;				/* sequence number */
{
    num %= 1000;

#ifdef HONEYDANBER
    (void) sprintf(comname, "D.%.5s%c%04d", site.nodename, GRADE, num);
    (void) sprintf(remname, "D.%.5s%04da12c", sysnam, num);
    (void) sprintf(nremname, "X.%.7s%c%04d", sysnam, GRADE, num);
    (void) sprintf(locname, "C.%.7s%c%04d", sysnam, GRADE, num);
#else
#ifndef	GRADE2
    (void) sprintf(comname, "D.%s%s%04d", sysnam, GRADE, num);
    (void) sprintf(remname, "D.%s%s%04d", site.nodename, GRADE, num);
    (void) sprintf(nremname, "X.%s%s%04d", site.nodename, GRADE, num);
    (void) sprintf(locname, "C.%s%s%04d", sysnam, GRADE, num);
#else
    /*
     * remname[] and nremname[] use GRADE2 because on some systems (Ultrix)
     * the remote command file must have an X in its name. On normal systems,
     * GRADE and GRADE2 can be the same.
     */
    (void) sprintf(comname, "D.%s%s%04d", sysnam, GRADE, num);
    (void) sprintf(remname, "D.%s%s%04d", site.nodename, GRADE2, num);
    (void) sprintf(nremname, "X.%s%s%04d", site.nodename, GRADE2, num);
    (void) sprintf(locname, "C.%s%s%04d", sysnam, GRADE, num);
#endif	/* !GRADE2 */
#endif	/* HONEYDANBER */
}

/* uucast.c ends here */
