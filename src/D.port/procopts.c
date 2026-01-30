/*****************************************************************************

NEWS
   procopts.c -- common options processing code

SYNOPSIS
   #include "procopts.h"

   int procopts(argc, argv, state, options)
   int argc; char **argv; int state; option_t *options;

   int vcrack(line, argv, maxargs)
   char *line, *argv[]; int maxargs;

DESCRIPTION
   The procopts function accepts an argument vector, an initial state, and an
options array and processes the argument vector accordingly. The options array
consists of option_t structures. Each of these defines the following:

1. An option keyletter
   This is the letter which, when recognized, invokes processing of this
option. Option letters are always recognized immediately following a '-'.
Flag options are recognized at any position in a '-'-prefixed argument.

2. A buffer fill character
  See #7 below.

3. A flag address
  If this option is found, the address given is set to TRUE, otherwise it
is FALSE. If the option is of NUMBER type and has a value the value will be
put there.

4. An entry mode
   Procopts keeps running track of a 'mode' which is initially DNC (Do Not
Care). If an option's entry state is other than DNC and doesn't match the
current mode (specifically,  if the & of the entry mode and current mode is 0)
an error is returned.

5. An exit mode
   If this member is other than DNC the current mode is set to it after the
option is processed. This field, together with the previous, can be used to
enforce mutual exclusivity of various sets of options.

6. An argument type
   An OPTION type option simply sets its flag variable. A NUMBER option,
when encountered, loads its flag variable with the value of the next following
token interpreted as a decimal integer (if there is no such token 1, not 0 is
loaded). For STRING options see below.

7. A buffer address
   A STRING option interprets all tokens following the current argument
up to but not including the next '-'-prefixed token as string arguments to
be concatenated into its buffer. One instance of the fill character is
inserted between each pair of strings concatenated.

   If the first argument lacks a leading dash, procopts() acts as though
the keyletter of the first option in the table were inserted in front of it.

   If no option matching a found keyletter is found, FAIL is returned and
processing stops at that point.

   Note that procopts() ignores argv[0].

   The vcrack procedure is provided for parsing argument vectors from text
lines. It does *not* ignore argv[0].

BUGS
   The code insists that the first option in the table be of STRING type.
This is so there will always be a valid buffer pointer to cons strings to
the end of.

AUTHOR
   Eric S. Raymond
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

*****************************************************************************/
/* LINTLIBRARY */
#include "libport.h"
#include "procopts.h"

int procopts(argc, argv, state, options)
register int	argc;		/* count of options to process */
register char	**argv;		/* vector of option strings */
int		state;		/* mode to start in */
option_t	*options;	/* options table to use */
{
    register char *buf, *ptr;
    char filchar, firstopt[3];
    int len, tlen;
    option_t *optpt;

    if (argc == 0)
	return(state);

    /* force the first argument if no - switch is given */
    if (argv[1] != (char *)NULL && argv[1][0] != '-' && argv[1][0] != '+')
    {
	firstopt[0] = '-';
	firstopt[1] = options[0].optlet;
	firstopt[2] = '\0';
	argv[0] = firstopt;
    }
    else
	++argv, --argc;

#ifdef PDEBUG
    (void) fprintf(stderr, "procargs: argc = %d\n", argc);
#endif /* PDEBUG */

    /* loop once per arg. */
    for (; argc--; argv++)
    {
	if (**argv == '-' || **argv == '+')
	{
	    int	    turnon = (**argv == '-');

	    /* find and set a new current option */
	    while (*++*argv != '\0')
	    {
		/* search the options table for a matching letter */
		for (optpt = options; optpt->optlet != '\0'; ++optpt)
		{
		    if (optpt->optlet == **argv)
		    {
			/* first, check that we're in a good state */
			if (state && (state & optpt->oldstate == 0))
			    return(FAIL);
			else if (optpt->newstate)
			    state = optpt->newstate;

			/* state is O.K., fill in the options */
			filchar = optpt->filchar;
			if (optpt->flagaddr != NONE)
			    optpt->flagaddr[0] = turnon;
			ptr = buf = optpt->buf;
			len = BUFSIZ;
			break;
		    }
		}

		if (optpt->optlet == '\0')
		    return(FAIL);		/* unknown option letter */
	    }
	}
	else if (optpt->argtype == OPTION)	/* last option took no args */
	    return(FAIL);			    /* this must be wrong */
	else if (optpt->argtype == NUMBER)
	{
	    if (optpt->flagaddr != NONE)
		if (**argv)
		    optpt->flagaddr[0] = atoi(*argv);
		else
		    optpt->flagaddr[0] = 1;	/* default NUMBERs to 1 */
	}
	else	/* (optpt->argtype == STRING) */
	{
	    /*
	     * Pick up a piece of a string and put it into
	     * the appropriate buffer.
	     */
	    if (strlen(*argv) + 1 >= len)
		xerror("Argument string too long");
	    if (ptr > buf && *(ptr-1) != filchar)
		*ptr++ = filchar;
	    (void) strcpy(ptr, *argv);
	    tlen = strlen(ptr);
	    ptr += tlen; len -= tlen;
	    *ptr = '\0';
	}
    }

    return(state);
}

int vcrack(line, largv, maxarg)
/* crack a line of text into an argument vector */
char	*line;
char    *largv[];
int	maxarg;
{
    int	    largc = 0;
    char    *cp;

    /* crack the argument list into a dope vector */
    for (cp = line; *cp; cp++)
    {
	if (isspace(*cp))
	    *cp = '\0';
	else if (cp == line || cp[-1] == 0)
	{
	    if (largc >= maxarg - 1)
		return(FAIL);

	    largv[largc++] = cp;
	}
    }

    largv[largc] = (char *) NULL;
    return(largc);
}

/* procopts.c ends here */
