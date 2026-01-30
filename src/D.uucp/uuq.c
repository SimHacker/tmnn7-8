/*****************************************************************************

NAME
   uuq.c -- return size of the outgoing uucp queue for a given system

SYNOPSIS
   off_t uuq(sys)
   char *sys;

DESCRIPTION
   This function tries to return the size of the outgoing uucp queue for a
given system, which is the total size of all data files queued to it.

BUGS
   Depends on conventions for naming of the UUCP job control files that
vary from version to version of UUCP and are subject to change without
documented notice. However, this code seems to work OK for the major
versions.
   Please report any improvements you develop to the news maintainers.

SEE ALSO
   uulib.h	-- configuration defines for uucp manipulation
   uucast.c	-- diddle control files to do uucp multicasting

AUTHOR
   Eric S. Raymond
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

*****************************************************************************/
/* LINTLIBRARY */
#include "news.h"
#include "libuucp.h"

off_t uuq(name)
/* return the amount of message data in the UUCP queue for system named */
char    *name;
{
    off_t retval = 0;
#ifdef BSD4_3
    register char *p2;
    FILE *fp;

    /*
     * I've taken the easy way out and popen'd "uuq" (4.3 BSD UUCP utility)
     * to parse the first line, which looks something like this:
     *
     * <systemname>: <n> jobs, <nnnn> bytes, ....
     *
     * I merely look for the first comma, and sscanf the number following.
     * A proper solution would be to dive in and parse the UUCP queues
     * themselves, but: it's moderately difficult, and it changes from
     * system to system.
     */
    (void) sprintf(bfr, "uuq -l -s%s", name);

    fp = peopen(bfr, "r");
    if (!fgets(bfr, sizeof(bfr), fp))
        return(FALSE);
    (void) peclose(fp);

    p2 = strchr(bfr, ',');
    if (p2 && 1 == sscanf(p2+1, "%ld", &retval))
        return(retval);

    return(FAIL);
#else
    /*
     * O.K., we get to do it the hard way, by looking at the uucp queues
     * themselves. This may well fail as it depends on uucp spool directory
     * locations, but if it does it will return a value that should permit
     * transmission to go forward.
     *
     * If we can't find the remote site directory, bypass the byte counting
     * routine. This is necessary because the cleanup daemon, on some sites,
     * removes the site directory when there's nothing there.
     *
     * This code was swiped from Phil Esposito's nbatcher program.
     */
    static DIR		    *directory;	/* current directory structure */
    static struct dirent    *entry;	/* points to an entry in it */
    struct stat		    st;
    short		    prefix_len;

#ifdef HONEYDANBER
    (void) sprintf(bfr, "%s/%s", DDIR, name);
#else
    (void) strcpy(bfr, DDIR);
#endif /* HONEYDANBER */

    if ((directory = opendir(bfr)) == (DIR *)NULL)
	return(FAIL);

    (void) sprintf(bfr, "D.%s", name);
    prefix_len = (short) strlen(bfr);
    retval = 0;
    while ((entry = readdir(directory)) != (struct dirent *)NULL)
    {
	if (entry->d_ino == 0 || entry->d_name[0] == '.')
	    continue;
	if (strncmp(entry->d_name, bfr, prefix_len))
	    continue;
	if (stat(entry->d_name, &st) < 0)
	    continue;
	retval += st.st_size;
    }
    (void) closedir(directory);

    return(retval);
#endif /* !BSD4_3 */
}

/* uuq.c ends here */
