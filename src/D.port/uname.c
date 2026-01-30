/**********************************************************************

NAME
   uname.c -- get system name

SYNOPSIS
   struct utsname *uname();

DESCRIPTION
   This routine is compatible with the Unix T/S system call uname,
which figures out the name of the local system. It assumes that exactly
one of the following symbols is defined (except that sysname should
be defined if sysname is).

   sysname  -- compile this in as the sitename
   UUNAME   -- the name can be found in some special file
   GHNAME   -- gethostname(2) is handy
   WHOAMI   -- name can be found in the whoami.h file
   HOSTCMD  -- this command will return the sitename on standard input

REVISED BY
   Eric S. Raymond
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

************************************************************************/
/*LINTLIBRARY*/
#include "libport.h"

#ifdef WHOAMI
#include <whoami.h>
#endif

#ifdef sysname
uname(uptr)
struct utsname *uptr;
{
    (void) memcpy(uptr->nodename, sysname, sizeof(uptr->nodename));
    return(SUCCEED)
}
#endif /* sysname */

#ifdef	UUNAME	/* uucp name is stored in /etc/uucpname or /local/uucpname */
uname(uptr)
struct utsname *uptr;
{
    static char *namefiles[] =
    {
	"/etc/uucpname",
	"/local/uucpname",
	"/etc/systemid"
    };
    char **np = namefiles;
    FILE *uucpf = (FILE *)NULL;

    for (np = namefiles; *np; np++)
	if (uucpf = fopen(*np, "r"))
	    break;

    if (uucpf == (FILE *)NULL
	|| fgets(uptr->nodename, sizeof(uptr->nodename),uucpf) == (char*)NULL)
      return(FAIL);
    (void) fclose(uucpf);
    uptr->nodename[strlen(uptr->nodename) - 1] = '\0';
    return(SUCCEED);
}
#endif /* UUNAME */

#ifdef GHNAME
uname(uptr)
struct utsname *uptr;
{
    char	*cp;

    (void) gethostname(uptr->nodename, sizeof(uptr->nodename));
    if (cp = strchr(uptr->nodename, '.'))
	*cp = '\0';
    return(SUCCEED);
}
#endif /* GHNAME */

#ifdef HOSTCMD
uname(uptr)
struct utsname *uptr;
{
    FILE *uucpf;

    if ((uucpf = peopen(HOSTCMD, "r")) == (FILE *)NULL &&
	    fgets(uptr->nodename,sizeof(uptr->nodename),uucpf) == (char*)NULL)
	return(FAIL);
    uptr->nodename[strlen(uptr->nodename)-1] = '\0';
    if (uucpf != (FILE *)NULL)
	(void) peclose(uucpf);
    return(SUCCEED);
}
#endif /* HOSTCMD */

/* uname.c ends here */
