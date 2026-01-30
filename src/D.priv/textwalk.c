/****************************************************************************

NAME
   textwalk.c -- article-tree walking function

SYNOPSIS
   #include "libpriv.h"

   void textwalk(func)		-- walk the text directory
   int (*func)();

DESCRIPTION
   This function applies the given function argument to each node in the
test directory tree. At each node, the function is handed as arguments:

   1. The full path name of the parent of the current node.
   2. The last segment of the pathname of the current node.
   3. An integer node type argument (see libpriv.h).

In addition, the current-article global is correct at each article node.
If the node type is TW_GROUP, argument 2 is the dot-separated group name of
the group we are entering, rather than the last segment of the node name;
and the value of the current-group field of the current-article global will
be zero.

FILES
   TEXT/*	-- article text directories
   ADM/active	-- active group information

AUTHOR
   Eric S. Raymond
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

**************************************************************************/
/*LINTLIBRARY*/
#include "news.h"
#include "libpriv.h"
#include "active.h"

static int (*funcptr)();

static int spoolsearch(dirname, group)
char	*dirname, *group;
/*
 * This subroutine performs a recursive search down a spool directory,
 * adding newsgroups to the active file if they are not present.
 */
{
    DIR			*directory;	/* Current directory structure */
    struct dirent	*entry;		/* An entry within it. */
    char		nextdir[BUFLEN]; /* name of lower level directory */
    char		nextgroup[BUFLEN];
    nart_t		artcount = (nart_t)0;
    place_t		save;
    int			res;

    /* we're looking at a new directory */
    if ((*funcptr)(dirname, (char *)NULL, TW_DIRECTORY) == FAIL)
	return(FAIL);
	
    if ((directory = opendir(dirname)) == (DIR *)NULL)
	return(FAIL);
	
    /* now pick the next entry out of the directory */
    while (entry = readdir(directory))
    {
	/* Make dot files go away */
	if (entry->d_name[0] == '.')
	    continue;

	/* If it's a directory, recursively search it. */
	(void) sprintf(nextdir, "%s/%s", dirname, entry->d_name);
	if (isdir(nextdir))
	{
	    (void) sprintf(nextgroup, "%s%s%s", group,
			   group[0] ? "." : "", entry->d_name);
	    save.m_group = active.article.m_group;
	    save.m_number = active.article.m_number;
	    if (spoolsearch(nextdir, nextgroup) == FAIL)
	    {
		(void) closedir(directory);
		return(FAIL);
	    }
	    else
	    {
		active.article.m_group = save.m_group;
		active.article.m_number = save.m_number;
		continue;
	    }
	}

	/* get current article number; if it's zero, ignore and flag it. */
	if ((active.article.m_number = atoa(entry->d_name)) == 0)
	    res = (*funcptr)(dirname, entry->d_name, TW_BADNUMBER);
	else
	{
	    if (artcount++ == 0)
	    {
		active.article.m_group = ngfind(group);
		if ((*funcptr)(dirname, group, TW_GROUP) == FAIL)
		{
		    (void) closedir(directory);
		    return(FAIL);
		}
	    }
	    res = (*funcptr)(dirname, entry->d_name, TW_ARTICLE);
	}
	if (res == FAIL)
	{
	    (void) closedir(directory);
	    return(FAIL);
	}
    }
    (void) closedir(directory);
    return(SUCCEED);
}

int textwalk(func)
/* apply func to each article location */
int	(*func)();
{
    funcptr = func;
    return(spoolsearch(site.textdir, ""));
}

/* textwalk.c ends here */
