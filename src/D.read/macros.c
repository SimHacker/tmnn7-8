/****************************************************************************

NAME
   macros.c -- user-definable macro support

SYNOPSIS
   #include "vio.h"

   void vmacro()		-- expand a macro in the input stream

   void vmacdef(mac, exp)	-- define a macro
   char *mac, *exp;

DESCRIPTION
   This module implements rn-like user macros in a way usable with any
I/O manager. To use it, call vmacro() at the beginning of any 'command
context' (usually this means just after vgetcmd() has been invoked). It
eats characters with vgetch() until they match a macro key, or until they
can no longer match any key. In the former case, the expansion is catenated
to the input queue via vungetch(); in the latter, the characters input are
so catenated. Following vgetc() and vgetline() calls will see and do the
right thing.

AUTHOR
   Eric S. Raymond
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

****************************************************************************/
/* LINTLIBRARY */
#include "news.h"
#include "vio.h"
#include "dballoc.h"
#include "alist.h"

ALIST(macros, 20, 10)	    /* hold the macro association list */

private int depth;	/* how many characters to try and match */

private char *digraph(str, c1, c2)
/* find the first occurrence of the digraph c1c2 in str */
register char *str;
char c1, c2;
{
    while (str = strchr(str, c1))
	if (str[1] == c2)
	    break;
    return(str);
}

private bool prefmatch(s, t)
char *s, *t;
{
    return (strncmp(s, t, depth) == SUCCEED);
}

void vmacro()
/* expand the given character into the given buffer */
{
    char		mbuf[BUFLEN], expbuf[LBUFLEN], *match, *cp;
    static int		rescan = 0;

    if (rescan)		/* we're looking at an expanded macro */
    {
	rescan--;
	return;
    }

    /* find a match in the macro table keys */
    expbuf[0] = '\0';
    for (depth = 1; ; depth++)
    {
	/* add a character to the partial-match buffer */
	mbuf[depth - 1] = vgetch();
	mbuf[depth] = '\0';

	/* search for the partial match as a prefix in the macro keys */
	if ((match = amatch(&macros, mbuf, prefmatch)) == (char *)NULL)
	    break;	/* no match, throw back queued chacters */

#ifndef lint
	/* if the current 'partial' match is as long as the key, good */
	else if (depth == strlen(akey(&macros)))
	    break;
#endif /* lint */
    }

    /*
     * If match is non-null it is the macro expansion of the text
     */
    if (match == (char *)NULL)
	(void) strcpy(expbuf, mbuf);	/* partial match -> pushback buffer */
    else
    {
	char	*ep;

	(void) strcpy(expbuf, match);

	/* keep expanding the escapes until none are left */
	for (;;)
	{
	    if (cp = digraph(expbuf, '^', '('))
	    {
		*cp++ = '\0';
		*cp++ = '\0';

		/* now expand inline ^( ^) escapes */
		if (ep = digraph(cp, '^', ')'))
		{
		    *ep++ = '\0';
		    *ep++ = '\0';
		}
		else
		    ep = expbuf + strlen(expbuf);

		(void) strcpy(mbuf, ep);
		if (match = afind(&macros, cp))
		    (void) strcat(expbuf, match);
		(void) strcat(expbuf, mbuf);

		continue;	/* check the expansion again */
	    }

#ifdef RNESCAPES
	    /* expand %-escapes */
	    if (strchr(expbuf, RNESCCHR))
	    {
		strexpand(expbuf, mbuf);
		(void) strcpy(expbuf, mbuf);
		continue;	/* check the expansion again */
	    }
#endif /* RNESCAPES */

	    break;	/* no % or ^( escapes, we're done */
	}

	/* disable macro expansion until this input is eaten */
	rescan = strlen(expbuf);
    }

    /* the expansion now sits in expbuf, queue it up */
    for (cp = expbuf + strlen(expbuf) - 1; cp >= expbuf; cp--)
	vungetch(*cp);
}

void vmacdef(mac, exp)
/* define a macro */
char *mac, *exp;
{
    char	emac[BUFLEN];

    escapes(exp, exp);
    (void) aenter(&macros, emac, exp);
}

/* macros.c ends here */
