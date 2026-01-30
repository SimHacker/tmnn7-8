/*****************************************************************************

NAME
   fullname.c - return user's full name

SYNOPSIS
   char *fullname(userid)	-- return user's name-among-humans
   char *userid;

DESCRIPTION
   This file is made separate so that different local
conventions can be applied.  The stock version understands two
conventions:

(a) Berkeley finger: the gecos field in /etc/passwd begins with
    the full name, terminated with comma, semicolon, or end of
    field.  & expands to the login name.
(b) BTL RJE: the gecos field looks like
       : junk - full name ( junk :
    where the "junk -" is optional.

If you have a different local convention, modify this file accordingly.

FILES
   $HOME/.name	-- stores answer to fullname query (non-GCOS systems only).

AUTHOR
   Eric S. Raymond (enhanced from anonymous code in news 2.10.3)
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.
******************************************************************************/
/* LINTLIBRARY */
#include "libport.h"

#define LUNAME	64  /* maximum length of user full name */

#ifdef GCOS
/*
**  BUILDFNAME -- build full name from RJE-style gecos field.
**	(routine lifted from sendmail)
**
**	Parameters:
**		p -- name to build.
**		login -- the login name of this user (for &).
**		buf -- place to put the result.
**
**	Returns:
**		none.
**
**	Side Effects:
**		none.
*/

static void buildfname(p, login, buf)
register char *p;
char *login;
char *buf;
{
    register char *bp = buf;

    if (*p == '*')
	p++;
    while (*p != '\0' && *p != ',' && *p != ';' && *p != ':' && *p != '(')
    {
	if (*p == '-')
	{
	    bp = buf;
	    p++;
	}
	else if (*p == '&')
	{
	    (void) strcpy(bp, login);
	    if ((bp == buf || !isalpha(bp[-1])) && islower(*bp))
		*bp = toupper(*bp);
	    while (*bp != '\0')
		bp++;
	    p++;
	}
	else
	    *bp++ = *p++;
    }
    while (bp > buf && bp[-1] == ' ')
	--bp;
    *bp = '\0';
}

char *fullname(un)
/*
 * Figure out who is sending the message and sign it.
 * We attempt to look up the user in the gecos field of /etc/passwd.
 */
char *un;
{
    static char inbuf[LUNAME];
    struct passwd *pw;
    char *fullenv = getenv("NAME");

    if (fullenv != (char *)NULL)
	return(fullenv);
    else if ((pw = getpwnam(un)) == (struct passwd *)NULL)
	return(un);
    buildfname(pw->pw_gecos, un, inbuf);
    if (inbuf[0] == 0)
	return(un);
    return(inbuf);
}

#else	/* !GCOS */

/*
 * Alternative version of fullname which asks the user for his full name.
 * This is mainly suitable for systems that don't have a full name
 * database somewhere.  It puts the answer in $HOME/.name
 */
char *
fullname(un)
char *un;
{
    static char inbuf[LUNAME];
    char fbuf[LUNAME], *p, *home;
    FILE *fd;
    int pid;

    /* if we've already called this once, use the stored result */
    if (inbuf[0])
	return(inbuf);

    /* try to mine it out of the environment */
    if ((p = getenv("NAME")) != (char *)NULL)
	return(p);

    /* otherwise if $HOME/.name already exists, read it out of that */
    if ((home = getenv("HOME")) != (char *)NULL)
    {
	(void) sprintf(fbuf, "%s/%s", home, ".name");
	if (fp = fopen(fbuf, "r"))
	{
	    if (fgets(inbuf, LUNAME, fp) != (char *)NULL)
	    {
		if (p = strchr(inbuf, '\n'))
		    *p = 0;
		return(inbuf);
	    }
	}
    }

    /* if calling program is non-interactive, punt */
    if (!isatty(2))
	return(un);

    /* otherwise, ask for name and write it to $HOME/.name
    (void) printf("What is your full name? ");
    (void) fflush(stdout);
    (void) read(2, inbuf, (iolen_t) sizeof(inbuf));
    if (inbuf[0] == 0)
	return(un);
    if (p = strchr(inbuf, '\n'))
	*p = 0;
    if ((p = getenv("HOME")) == (char *)NULL)
    {
	(void) fprintf(stderr,
	"fullname: no HOME environment variable - .name not written\n");
	return(inbuf);
    }
    (void) sprintf(fbuf, "%s/%s", p, ".name");
    if ((pid = vfork()) < 0)
    {
	perror("fullname");
	return inbuf;
    }
    else if (pid != 0)	/* parent side */
	while (wait((wait_t *)NULL) != pid)
	    continue;
    else		/* child side */
    {
	(void) setuid(getuid());	/* become the user */
	if ((fd = fopen(fbuf, "w")) == (FILE *)NULL)
	    (void) fprintf(stderr, "fullname: can't create %s\n", fbuf);
	else
	{
	    (void) fprintf(fd, "%s\n", inbuf);
	    (void) fclose(fd);
	}
	exit(0);
    }
    return(inbuf);
}
#endif	/* GCOS */

/* fullname.c ends here */
