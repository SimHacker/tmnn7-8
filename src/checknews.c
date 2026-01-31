/*****************************************************************************

NAME
   checknews.c - main module of the news checking program

SYNOPSIS
   main(argc, argv)	    -- performs the news-checking function
   int argc, char *argv[];

   catch_t xxit(stat)	   -- exit, cleaning up and returning given status
   int stat;

DESCRIPTION
   See the accompanying documentation. This version is new for TMN-Netnews and
incorporates the x option.

   Because there is no command interpreter, newsgroup initial commands are
ignored. This may mean that checknews reports more messages than the reader
will actually select.

AUTHOR
   Eric S. Raymond
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

*****************************************************************************/
/*LINTLIBRARY*/
#include "news.h"
#include "vio.h"
#include "header.h"
#include "procopts.h"
#include "libread.h"
#include "active.h"
#include "newsrc.h"
#include "dballoc.h"	/* session.h needs the dbdef_t type */
#include "session.h"
#ifdef lint
#include "gcmd.h"
#endif /* lint */

char *Progname = "checknews";	/* used by xerror */

char *usage =
"Usage: checknews [yvenqxN] [ -a date ] [ -n groups ] [ -t titles ] [ -rfv ]";
private char	checkopts[SBUFLEN];

option_t options[] =
{
/* 
optlet  filchar	flag	from	to	type	buf	*/
'c',	'\0',	NONE,	DNC,	DNC,	STRING,	checkopts,
'n', NGDELIM,	NONE,	DNC,	DNC,    STRING,	grouplist,  /* ->checkinit.c */
't',	'\0',	NONE,	DNC,	DNC,    STRING,	titlebuf,   /* ->checkinit.c */
'a',	 ' ',	NONE,	DNC,	DNC,    STRING,	datebuf,    /* ->checkinit.c */
'r',	'\0',   &session.reverse,	DNC,	DNC,    OPTION,	(char *)NULL,
'f',	'\0',   &nofoll,	DNC,	DNC,    OPTION,	(char *)NULL,
#ifdef DEBUG
'D',	'\0',   &debug, DNC,	DNC,    NUMBER,	(char *)NULL,
#endif /* DEBUG */
'\0',	'\0',	0,	0,	0,      0,	(char *)NULL,
};

private int takeopts(ltype, line)
/* process directive lines for checknews */
int	ltype;	/* directive type */
char	*line;	/* text of directive */
{
    if (ltype == N_OPTIONS)
	return(lprocargs(line));
    else
	return(SUCCEED);
}

main(argc, argv)
int	argc;
char	*argv[];
{
    int	    y, e, n, q, x, v, N;	/* old-style option flags */
    char    *copt;

    /* support crufty old-style checknews options (bleaghhh...) */
    y = e = n = q = x = v = N = 0;
    if (argc > 1 && strchr("ynqexvN", argv[1][0]))
    {
	for (copt = argv[1]; *copt; copt++)
	{
	    switch(*copt)
	    {
	    case 'y': y++; break;
	    case 'q': q++; break;
	    case 'n': n++; break;
	    case 'e': e++; break;
	    case 'x': x++; break;
	    case 'v': v++; break;
	    case 'N': N++; break;
	    }
	}
	argv++; argc--;

	if (N)
	{
	    if (argv[1])
		/* Fixed by ReviewBot-774 (Issue #33) */
		(void) strlcpy(grouplist, argv[2], BUFLEN);
	    argv++; argc--;
	}
    }
    if (!n && !e && !y && !q && !x)	/* y is the default option */
	y++;

    nochange = TRUE;	/* suppress .newsrc write on exit */

    /* now parse normal options */
    if ((session.news = checkinit(argc, argv, takeopts)) == FAIL)
	xerror0("command line or .newsrc options are invalid");
    else if (session.news == 0)
    {
	if (n)
	    (void) printf("No news is good news.\n");
	exit(q);
    }
    else if (y && v == 0)
    {
	(void) fprintf(stderr, "There is news (%ld articles)\n",
		       (long)session.news);
	exit(0);
    }

    /* 
     * if no execute option, tell I/O manager we're non-interactive.
     * This enables later tests via isatty(fileno(stdin)).
     */
    if (!e)
	(void) fclose(stdin);

    /* now hunt up new messages */
    for (session.action = M_NEXT; msgread() != FAIL;)
    {
	/*
	 * read the next message, unless the last time around ran us
	 * out of messages (in which case the NOMORE bit is set).
	 */
	if (msgtext() == FAIL)
	    continue;

	if (session.ngrp == 1)
	{
	    /* user may want to know about new active groups */
	    if (!rcflag(RC_NOTED) && v > 2)
		(void) printf("No newsrc line for %s\n", ngname());

	    if (N || (y && v >= 2))
		(void) printf("There is news in %s\n", ngname());
	}

	if (y && session.news == 1 && v == 1 && !x)
		(void) printf("There is news in %s...\n", ngname());

	if (x)
	    (void) fprintf(stdout, "%s/%ld\n", ngname(), (long)msgnum());
    }

    if (e)
    {
	char	*rdn;

	if ((rdn = getenv("READNEWS")) == (char *)NULL)
	    rdn = "readnews";

	(void) execvp(rdn, argv);

	/* can't get here unless the execvp failed */
	(void) perror("Cannot exec your news reader.");
    }

    /* we do *not* call cleanup() here, this is deliberate */
    msgend();
    return(SUCCEED);
}

catch_t xxit(status) int status; {exit(status);}

/*ARGSUSED0*/
void vupdate() {/*no-op*/}

#ifdef lint	/* these just shut up lint for when we link with libread.a */
int vstat;
int vgetc() {return('\0');}
char *vgetline(prompt) char *prompt; {return(prompt);}
/*ARGSUSED0*/
void vcommand(args) cmdarg_t *args; {/*no-op*/}
void vuline(to, from) char *to, *from; {(void)strcpy(to, from);}
#endif

/* checknews.c ends here */
