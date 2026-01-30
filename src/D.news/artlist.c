/****************************************************************************

NAME
   artlist.c -- crack a list of article references into locations

SYNOPSIS
   #include "news.h"

   int refname(ref, place, file)	-- resolve an article reference
   char *ref; place_t *place; char *file;

   void artlstset(lst, lstart)	-- point the article list muncher at a buffer
   lptr_t *lst; char *lstart;

   int artlstloc(lst, exloc)	-- grab next location from the current list
   lptr_t *lst; place_t *exloc;

   int artlstdel(lst)		-- delete the current article list entry
   lptr_t *lst;

DESCRIPTION
   These functions convert between different kinds of article location
representations.
   The refname() function resolves an article reference in <topic>/<number>
form. It always fills in the place_t addressed by the 2nd argument. If the
third argument is non-NULL, the filename of a copy of the message is generated
into it. FAIL is returned if the reference is garbled, or if no valid copy
exists; otherwise FALSE or TRUE is returned according as the new filename
is not or is different from the previous contents of the buffer argument.
   The artlst* functions support sequential access to list of such article
references.

NOTE
   The list block structure that artlstset() operates on must be zeroed out
before the first artlstset() call, or chaos will ensue as it tries to free
a nonexistent buffer copy. This condition is automatic if the list block
structure is external static (except on losing XENIX systems!).

SEE ALSO
   {rd,wr}history.c	-- history file access routines
   nntp.c		-- nntp server access routines

AUTHOR
   Eric S. Raymond
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

**************************************************************************/
/*LINTLIBRARY*/
#include "news.h"
#include "active.h"
#include "history.h"	/* for the hstseek()-style status returns */

int refname(ref, place, file)
/* resolve an article reference in either Message-ID or topic/number form */
char	*ref;	/* the reference */
place_t	*place;	/* the location */
char	*file;	/* the file */
{
    char    *sloc;

    if ((sloc = strchr(ref, '/')) != (char *)NULL && isalnum(ref[0]))
    {
	*sloc = '\0';
	if ((place->m_group = ngfind(ref)) == (group_t *)NULL)
	    return(FAIL);
	place->m_number = atoa(sloc + 1);
	*sloc = '/';
#ifdef notdef
	/*
	 * We shouldn't care if the group number isn't within the
	 * bounds of the active file; if the active file is corrupted
	 * or if the rnews daemon has inserted the article but hasn't
	 * flushed the active file out yet.
	 */
	if (AOK(place->m_group, place->m_number))
#endif
	{
	    if (file == (char *)NULL)
		return(SUCCEED);
	    else
		return(artname(place, file));
	}
#ifdef notdef
	else
	    return(FAIL);
#endif
    }
    else
	return(FAIL);
}

void artlstset(lst, lstart)
/* point the article list muncher at a buffer */
lptr_t	*lst;
char *lstart;
{
    /*
     * We want to have our own copy of the buffer being parsed so we
     * don't lose if it gets freed during the parsing.
     */
    if (lst->l_bufcopy)
	(void) free(lst->l_bufcopy);
    lst->l_next = lst->l_this = lst->l_bufcopy = savestr(lstart);
}

int artlstloc(lst, exloc)
/* grab next location from the current list */
lptr_t	*lst;
place_t *exloc;
{
    static char	argbuf[BUFLEN];

    /* start our scan from the current next-article position */
    lst->l_this = lst->l_next;

    /* skip the next article pointer part the next token */
    while (lst->l_next[0] && !isspace(lst->l_next[0]))
	lst->l_next++;

    /* if there is no next token we're done */
    if (lst->l_this == lst->l_next)
	return(FAIL);

    /* grab a copy of what's between lst->l_this and lst->l_next*/
    (void) strncpy(argbuf, lst->l_this, lst->l_next - lst->l_this);

    /* make sure it's properly string-terminated */
    argbuf[lst->l_next - lst->l_this] = '\0';

    /* skip leading spaces to get to the next token or the end of string */
    while (lst->l_next[0] && isspace(lst->l_next[0]) && lst->l_next[0] != '\n')
	lst->l_next++;

    if (refname(argbuf, exloc, (char *)NULL) == FAIL)
	return(GARBLED);
    else
	return(SUCCEED);
}

int artlstdel(lst)
/* delete the current article list entry */
lptr_t	*lst;
{
    if (lst->l_next <= lst->l_this)
	return(FAIL);
    else
    {
	register char *s, *t;

	/* we can't assume strcpy() handles overlapping copy OK */
	s = lst->l_next;
	t = lst->l_this;
	while (*s)
	    *t++ = *s++;
	*t = '\0';
	lst->l_next = lst->l_this;
	return(SUCCEED);
    }
}

char   	*artlstret(lst)
lptr_t	*lst;
{
	return(lst->l_bufcopy);
}

/* artlist.c ends here */
