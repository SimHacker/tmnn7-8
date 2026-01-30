/*****************************************************************************

NAME
   setadd.c -- add or subtract tokens to/from a set list

SYNOPSIS
   int setadd(to, from, seps)
   char *to, from, *seps;

   int setsub(to, from, seps)
   char *to, from, *seps;

DESCRIPTION
   With setadd(), tokens in the from list are concatenated to the to list
except that duplicates are dropped. The third argument is taken to be the list
of token-separating characters; its first char is written as a separator
for the output characters. Thus,

    a = "foo bar baz";
    setadd(a, "qux baz")

leaves a as "foo bar baz qux". The return value is the number of tokens
actually concatenated.

    The setsub() function similarly does set subtraction from the to set.
Only one copy of the from token will be removed on each subtract.

NOTE
   Compiling this module with -DMAIN will produce an interactive tester for
these routines.

AUTHOR
   Eric S. Raymond
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

*****************************************************************************/
/* LINTLIBRARY */
#include "libport.h"

static int setop(to, from, seps, addit)
char *to, *from, *seps;
bool addit;
{
    int	tolen = strlen(to), fromlen = strlen(from);
    char	*fcp, *tcp;
    int		tspc = 2, fspc = 2, changed = 0;

    /* for each token in the from set... */
    for (fcp = from; fcp < from + fromlen; fcp++)
    {
	int	fnext, tnext;

	if (strchr(seps, *fcp) != (char *)NULL)
	    fspc = 2;
	else if (--fspc == 1)	/* first non-sep. character after sep. */
	{
	    bool	matched = FALSE;

	    /* ...look for a match with any token in the to set */
	    for (tcp = to; !matched && tcp < to + tolen; tcp++)
		if (strchr(seps, *tcp) != (char *)NULL)
		    tspc = 2;
		else if (--tspc == 1)
		{
		    fnext = strcspn(fcp, seps);
		    tnext = strcspn(tcp, seps);

		    if (fnext == tnext && strncmp(tcp, fcp, fnext) == 0)
			matched = TRUE;
		}

	    /* if no match, add token at end of to set */
	    if (addit && !matched)
	    {
		if (strlen(to))
		{
		    to[strlen(to) + 1] = '\0';
		    to[strlen(to)] = seps[0];
		}
		(void) strncat(to, fcp, fnext);
		changed++;
	    }

	    /* if subtracting and there's a match, drop out the token */
	    if (!addit && matched)
	    {
		char *ccp;

		while (tcp[tnext - 1] && strchr(seps, tcp[tnext - 1]))
		    tnext++;
		for (ccp = tcp + tnext - 1; *ccp; ccp++)
		    ccp[-tnext] = ccp[0];
		ccp[-tnext] = '\0';
		changed++;
	    }
	}   
    }
    return(changed);
}

int setadd(to, from, seps)
char *to, *from, *seps;
{
    return(setop(to, from, seps, TRUE));
}

int setsub(to, from, seps)
char *to, *from, *seps;
{
    return(setop(to, from, seps, FALSE));
}

#ifdef MAIN

/*
 * An exerciser for this function.
 */
char	*Progname = "setadd";

main(argc, argv)
int	argc;
char	**argv;
{
    char	foo[BUFSIZ];
    int		changed;

    (void) strcpy(foo, argv[1]);

    if (argv[2][0] == '-')
	changed = setsub(foo, argv[3], " ");
    else if (argv[2][0] == '+')
	changed = setadd(foo, argv[3], " ");
    else
	changed = setadd(foo, argv[2], " ");
    (void) printf("%s: %d tokens changed\n", foo, changed);
}
#endif /* MAIN */

/* setadd.c ends here */
