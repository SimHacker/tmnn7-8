/*****************************************************************************

NAME
    uurec - receive news articles via /bin/mail.

SYNOPSIS
   main(argc, argv)	    -- performs the news-translating function
   int argc, char *argv[];

DESCRIPTION
   This is intended to be used with the M (mailer) option. See the
accompanying documentation for details. If called with the -D option
it reoports on the rnews invocations it would otherwise perform without
actually executing them.

AUTHOR
   Eric S. Raymond
Support for the old A news format has been removed from this TMNN version.
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

*****************************************************************************/
/*LINTLIBRARY*/
#include "news.h"
#include "vio.h"

/* header line types */
#define FROM	01
#define NLIN	02
#define BLANK	03
#define OTHER	04

/* program modes */
#define SKIPPING	010
#define READING		020

#define EOT	'\004'

char	*Progname = "uurec";

/* ARGSUSED */
main(argc, argv)
int argc;
char **argv;
{
    char	    buf[BUFLEN], fbuf[BUFLEN], rnews[BUFLEN];
    register char   *p = (char *)NULL;
    register FILE   *pipe = stdout;
    register int    mode, pathcnt;
    register bool   frmflg = FALSE;
    forward char    *frombreak();

    newsinit();

#ifdef DEBUG
    if (argc > 1 && strcmp(argv[1], "-D") == 0)
	debug = 1;

    /* generate the location of the news program */
    if (debug)
	(void) strcpy(rnews, "cat");
    else
#else
	(void) sprintf(rnews, "%s/rnews", newsattr("libdir", LIBDIR));
#endif /* DEBUG */

    mode = SKIPPING;
    while (fgets(buf, BUFLEN, stdin) != (char *)NULL)
    {
#ifdef ODEBUG
	(void) printf("%o\t%s", mode | type(buf), buf);
#endif
	switch(mode | type(buf))
	{
	    case FROM | SKIPPING:
		if (frmflg)
		    p = frombreak(p, buf);
		else
		    p = fbuf;
		frmflg = TRUE;
		break;

	    case FROM | READING: 
		if (!frmflg)
		{
		    frmflg = TRUE;
		    p = fbuf;
		    (void) peclose(pipe);
		}
		p = frombreak(p, buf);
		break;

	    case NLIN | SKIPPING: 
		mode = READING;

	    case NLIN | READING: 
		if (frmflg) {
		    frmflg = FALSE;
		    --p;
		    while (p >= fbuf && *--p != PATHSEP)
			continue;
		    *++p = '\0';
		    pathcnt = 0;
#ifdef DEBUG
		    if (debug)
			(void) printf("%s/rnews <<_STUFFY_FUNK_\n",
				      site.libdir);
#endif /* DEBUG */
		    if ((pipe = peopen(rnews, "w")) == (FILE *)NULL)
		    {
			perror("uurec: peopen failed");
			exit(1);
		    }
		}
		if (!pathcnt &&
			(!strncmp(buf + 1, "From: ", 6) ||
			!strncmp(buf + 1, "From ", 5)))
		{
		    pathcnt++;
		    (void) fprintf(pipe, "From: %s", fbuf);
		    (void) sscanf(buf, "%s %[^\n]", fbuf, fbuf);
		    (void) fprintf(pipe, "%s\n", fbuf);
		}
		else
		    (void) fputs(buf + 1, pipe);
		break;

	    case OTHER | SKIPPING: 
		break;

	    case OTHER | READING: 
		(void) pclose(pipe);
		mode = SKIPPING;
#ifdef DEBUG
		if (debug)
		    (void) fputs("_STUFFY_FUNK_\n", stdout);
#endif /* DEBUG */
		break;
	}
    }
    if (pipe && pipe != stdout)
	(void) pclose(pipe);
    return(0);
}

private int type(p)
/* return the type of a header line */
register char *p;
{
    while (*p == ' ' || *p == '?')
	++p;

    if (*p == 'N')
	return(NLIN);

    if (strncmp(p, ">From ", 6) == 0)
	return(FROM);

    if (strncmp(p, "From ", 5) == 0)
	return(FROM);

    return(OTHER);
}

private char *frombreak(buf, fbuf)
/* Get the system name out of a from line */
register char *buf, *fbuf;
{
    register char  *p;

    /* break the line into tokens. */
    p = fbuf;
    while (*++p != '\0')
	switch (*p)
	{
	    case '\n': 
	    case '\t': 
	    case ' ': 
		*p = '\0';
		break;
	    case EOT: 
		goto garbled;
	    default: ;
	}
    *++p = EOT;
    *++p = '\0';

    for (p = fbuf; *p != EOT || p[1] != '\0'; p += strlen(p) + 1)
    {
	if (strcmp(p, "forwarded") == 0)
	    return(buf);
	if (strcmp(p, "remote") == 0)
	{
	    p += strlen(p) + 1;
	    if (strcmp(p, "from") == 0)
	    {
		p += strlen(p) + 1;
		(void) strcpy(buf, p);
		(void) strcat(buf, "!");
		return(buf + strlen(buf));
	    }
	}
    }

garbled: 
    (void) strcat(buf, "???!");
    return(buf + 4);
}

catch_t xxit(status) int status; {exit(status);}

/* uurec.c ends here */
