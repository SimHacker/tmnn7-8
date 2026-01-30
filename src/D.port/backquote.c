/*****************************************************************************

NAME
   backquote.c -- read output of a shell command into a given buffer

SYNOPSIS
   char *backquote(cmd, buf, len)
   char *cmd, *buf; int len;

DESCRIPTION
   Read the output of a shell command (via popen(3)) into a given buffer.
Return SUCCEED if everything goes OK, otherwise FAIL -- errno will be EINVAL
if len was 0, ERANGE if the command returned more than len characters,
otherwise whatever was set by popen(3) and the command.
   If the command given ended in \c, any trailing newline on the output will
be stripped off.

AUTHOR
   Eric S. Raymond
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

*****************************************************************************/
/* LINTLIBRARY */
#include "libport.h"

char *backquote(cmd, buf, len)
char	*cmd, *buf;
int	len;
{
    FILE	*pfp;
    char	*cp, bfr[BUFSIZ];
    int		c;
    bool	strip;

    if (len <= 0)
    {
	errno = EINVAL;
	return((char *)NULL);
    }
    else if (buf == (char *)NULL)
	buf = malloc((unsigned) len);

    cp = cmd + strlen(cmd);
    if (strip = (cp >= cmd + 2 && cp[-1] == 'c' && cp[-2] == '\\'))
	cp[-2] = '\0';

    (void) strcpy(bfr, cmd);
    (void) strcat(bfr, "</dev/null");
    if ((pfp = popen(bfr, "r")) == (FILE *)NULL)
	return((char *)NULL);

    cp = buf;
    while (--len)	/* need one extra space for terminating '\0' */
	if ((c = fgetc(pfp)) == EOF)
	    break;
	else
	    *cp++ = c;
    *cp = '\0';
    if (strip && (cp = strchr(buf, '\n')) && cp[1] == '\0')
	*cp = '\0';

    (void) pclose(pfp);
    if (feof(pfp))
	return(buf);
    else
    {
	errno = ERANGE;
	return((char *)NULL);
    }
}

/* backquote.c ends here */
