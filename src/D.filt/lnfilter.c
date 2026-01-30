/*****************************************************************************

NAME
   lnfilter.c -- frame for line-oriented filter languages.

SYNOPSIS
   #include "libfilt.h"

   int lnfilter(lang, vers, ainterp, cinterp)	-- implement a filter
   char *lang, *vers; int (*ainterp)(), (*cinterp)();

DESCRIPTION
   This module does all routine housekeeping for news filters obeying the
following conventions:

   1. Kill commands live one per line in a file in the user's home directory
(or the directory forced by command-line options) named by the kill language
prefixed by a dot.

   2. Kill commands consist of a pattern-action pair separated by whitespace
or a colon (either of these may be backslash-escaped to avoid this
interpretation) or of a /-delimited regular expression followed by arbitrary
text (embedded / characters may be escaped to avoid this).

   Given that these are true, lnfilter() will implement a filter given just
one command interpreter for groups and articles. At startup, the kill
file is digested into core; on each article or group, the interpreter
is called on each kill command in turn. Commands sent down by a P command are
added to the in-core array with no checking. At wrapup time, the (possibly
modified) kill command array is written to the kill file.

   The command interpreter is passed three arguments:

   1. A compiled form of the RE part of the pattern (if any).
   2. The text of the command with any RE removed (but not the delimiters).
   3. A group data pointer for the group being scored.
   4. A pointer to an article header block for the article.

   Argument 4 will be NULL to indicate that the command interpreter is being
called on a group.

   It should return a numeric score. The cumulated score for all commands that
match will be returned to the reader.

NOTE
   All the machinery to cache results from background runs should live here
someday.

AUTHOR
   Eric S. Raymond.
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

*****************************************************************************/
/* LINTLIBRARY */
#include "news.h"
#include "header.h"
#include "dballoc.h"
#include "alist.h"
#include "regexp.h"
#include "active.h"
#include "libfilt.h"

#define LDELIM	'/'	/* delimit regular expressions on the left */
#define RDELIM	'/'	/* delimit regular expressions on the right */
#define ESCAPE	'\\'	/* can be used to escape the above */

static int (*scorehook)();	/* interpreter hook for articles, groups */
static int (*cmdhook)();	/* interpreter hook for commands */
static group_t *cgroup;		/* currently-selected group */
#ifdef DEBUG
static int report;		/* use this for debug messages */
#endif /* DEBUG */

/*
 * First of all, the types and handlers for the command list stuff
 */
typedef struct
{
    char	*whole;		/* the entire command */
    regexp	*re;		/* associated regular expression, if any */
    char	*parsed;	/* command with RE dropped out */
}
cmd_t;

static int ftcread(cmd, fp)
/* put a command in the list */
cmd_t	*cmd;
FILE	*fp;
{
    char	*p, *q, *r, rebuf[BUFLEN], parsebuf[BUFLEN];
    int		c, literal = 0;
    bool	buildre = FALSE;

    p = parsebuf;
    q = bfr;
    r = rebuf;    
    while ((c = fgetc(fp)) != '\n' && (c != EOF))
    {
	if (!literal && c == ESCAPE)
	    literal = 2;
	else
	{
	    *q++ = c;	/* record every character into wholebuf */
	    if (buildre)
	    {
		if (c == RDELIM && --literal != 1)
		{
		    buildre = FALSE;
		    *p++ = RDELIM;
		}
		else
		    *r++ = c;
	    }
	    else
	    {
		if (c == LDELIM && --literal != 1)
		{
		    buildre = TRUE;
		    *p++ = LDELIM;
		}
		else
		    *p++ = c;
	    }
	}
    }
    if (feof(fp))
	return(FAIL);
    *p = '\0';
    *q = '\0';
    *r = '\0';

    cmd->whole = savestr(bfr);
    cmd->parsed = savestr(parsebuf);
    if (rebuf[0])
	cmd->re = regcomp(rebuf);
    else
	cmd->re = (regexp *)NULL;
    return(SUCCEED);
}

int ftcwrite(cmd, fp)
/* dump a command from the list */
cmd_t	*cmd;
FILE	*fp;
{
    (void) fputs(cmd->whole, fp);
}

static dbdef_t killcmds = 
{
    (char *)NULL,	/* no file associated */
    sizeof(cmd_t), 	/* records are RE-command associations */
    20,			/* initially, allocate space for 20 records */
    10,			/* allocate 10 records at a time when we run out */
    ftcread,
    ftcwrite,
};

/* 
 * Now the filter entry points proper.
 */

static int ftstartup(vproto)
/* at startup, snarf kill commands into core where we can find them fast */
char	*vproto;
{
#ifdef DEBUG
    if (debug)
    {
	report = open("/dev/tty", O_WRONLY);
	(void) sprintf(xbf, "lnfilter (%s): ftstartup(%s)\n", Progname,vproto);
	(void) write(report, xbf, (iolen_t)strlen(xbf));
    }
#endif /* DEBUG */
    (void) sprintf(bfr, "%s/.%s", userhome, Progname);
    if (exists(bfr))
    {
	killcmds.file = savestr(bfr);
	(void) dbaread(&killcmds);
    }
#ifdef DEBUG
    if (debug)
    {
	(void) sprintf(xbf,
		       "lnfilter (%s): %d commands read from %s\n",
		       Progname, dbatell(&killcmds), bfr);
	(void) write(report, xbf, (iolen_t)strlen(xbf));
    }
#endif /* DEBUG */
    return(SUCCEED);
}

/*ARGSUSED1*/
static int ftgroupscore(name, response)
/* the group scoring function */
char	*name;
char	*response;
{
    int score = 0;

#ifdef DEBUG
    if (debug)
    {
	(void) sprintf(bfr, "lnfilter (%s): ftgroupscore(%s)\n",
		       Progname, name);
	(void) write(report, bfr, (unsigned)strlen(bfr));
    }
#endif /* DEBUG */

    if ((cgroup = ngfind(name)) == (group_t *)NULL)
	return(P_IGNORE);

    dbarewind(&killcmds);
    while (dbanext(&killcmds) == SUCCEED)
    {
	score += (*scorehook)(cgroup,
#ifndef lint
			      ((cmd_t *)killcmds.cp)->re,
			      ((cmd_t *)killcmds.cp)->parsed,
#else
			      (char *)NULL,
			      (char *)NULL,
#endif /* lint */
			      cgroup,
			      (hdr_t *)NULL);
#ifdef DEBUG
	if (debug)
	{
	    (void) sprintf(bfr,
		       "%s: after \"%s\" score was %d\n",
		       name,
#ifndef lint
		       ((cmd_t *)killcmds.cp)->whole,
#else
		       "dummy",
#endif /* lint */
		       score);
	    (void) write(report, bfr, (unsigned)strlen(bfr));
	}
    }
#endif /* DEBUG */
    return(score);
}

/*ARGSUSED1*/
static int ftartscore(hp, response)
/* call scorehook on every command in the list */
hdr_t   *hp;
char	*response;
{
    int score = 0;

#ifdef DEBUG
    if (debug)
    {
	(void) sprintf(bfr, "lnfilter (%s): ftartscore(%s)\n",
		       Progname, hp->h_ident);
	(void) write(report, bfr, (unsigned)strlen(bfr));
    }
#endif /* DEBUG */

    dbarewind(&killcmds);
    while (dbanext(&killcmds) == SUCCEED)
    {
#ifndef lint
	score += (*scorehook)(
				((cmd_t *)killcmds.cp)->re,
				((cmd_t *)killcmds.cp)->parsed,
			        cgroup,
			        hp);
#endif /* lint */
#ifdef DEBUG
	    if (debug)
	    {
		(void) sprintf(bfr,
			       "%s: after \"%s\" score was %d\n",
			       hp->h_ident,
#ifndef lint
			       ((cmd_t *)killcmds.cp)->whole,
#else
			       "dummy",
#endif /* lint */
			       score);
		(void) write(report, bfr, (unsigned)strlen(bfr));
	    }
#endif /* DEBUG */
    }
    return(score);
}

/*ARGSUSED0*/
static int ftprogram(cmd, response)
/* enter a command */
char	*cmd, *response;
{
#ifdef DEBUG
    if (debug)
    {
	(void) sprintf(bfr, "lnfilter (%s): ftprogram()\n", Progname, cmd);
	(void) write(report, bfr, (unsigned)strlen(bfr));
    }
#endif /* DEBUG */
    if ((*cmdhook)(bfr, cmd) == FAIL)
	(void) strcpy(response, "Command no good.");
    else
    {
	(void) strcpy(response, "Command O.K.");
	dbaenter(&killcmds, bfr);
    }
}

static void ftwrapup(response)
char	*response;
{
#ifdef DEBUG
    if (debug)
    {
	(void) sprintf(bfr, "lnfilter (%s): ftwrapup(%s)\n",
		       Progname, response);
	(void) write(report, bfr, (unsigned)strlen(bfr));
	(void) close(report);
    }
#endif /* DEBUG */
    dbawrite(&killcmds);
}

int lnfilter(lang, vers, score, cmdproc, argc, argv)
/* implement a simple filter */
char	*lang, *vers;
int	(*score)();
int	(*cmdproc)();
int	argc;
char	*argv[];
{
    scorehook = score;
    cmdhook = cmdproc;
    return(filter(lang, vers,
		  ftstartup, ftgroupscore, ftartscore, ftprogram, ftwrapup,
		  argc, argv));
}

#ifdef MAIN
char	*Progname = "lnfilter";

static int score(re, cmd, gp, hp)
/* kill language interpreter */
char	*cmd;	/* command, with the RE part (but not delimiters) dropped */
regexp	*re;	/* compiled RE from pattern part of command */
group_t	*gp;	/* group data pointer fror group */
hdr_t	*hp;	/* header of article to be scored */
{
    if (re != (regexp *)NULL && regexec(re, hp->h_subject))
    {
	(void) fprintf(stdout,
		       "lnfilter (%s): match triggers command %s\n",
		       Progname, cmd);
	return(1);
    }
    else
	return(0);
}

main(argc, argv)
/* filter-testing code */
int	argc;
char	*argv[];
{
    char    cmdline[BUFSIZ], strv[BUFSIZ], strv2[BUFSIZ], filtername[BUFSIZ];
    int	    level;

    newsinit();
    if (rdactive(NULLPRED) == FAIL)
	xerror0("couldn't read active file!");

    scorehook = score;
    cmdhook = (int(*)())strcpy;

    (void) printf("Welcome to the standard filter tester\n");
    while (fputs("> ", stdout), fgets(cmdline, BUFSIZ, stdin) != (char *)NULL)
    {
	if (cmdline[0] == '?')
	{
	    (void) printf("s file     -- read commands from ~/file\n");
	    (void) printf("a id file  -- return score of article\n");
	    (void) printf("g grp      -- return score of group\n");
	    (void) printf("p command  -- enter filtering commmand\n");
#ifdef DEBUG
	    (void) printf("D level    -- set debug level\n");
#endif /* DEBUG */
	    (void) printf("q,x        -- exit the tester\n");
	}
	else if (sscanf(cmdline, "s %s", strv) == 1)
	{
	    ftstartup(strv, strv);
	    (void) fprintf(stdout, "~/.%s: %d\n", strv, dbatell(&killcmds));
	}
	else if (sscanf(cmdline, "a %s %s", strv, strv2) == 2)
	{
	    FILE	*fp;
	    hdr_t	*hp;

	    if (strv2[0] != '/')
	    {
		char	*cp;

		(void) sprintf(bfr, "%s/%s", site.textdir, strv2);
		(void) strcpy(strv2, bfr);
		for (cp = strv2; *cp; cp++)
		    if (*cp == NGSEP)
			*cp = '/';
	    }
	    if ((fp = msgopen(strv2)) == (FILE *)NULL)
		perror("msgopen");
	    else if (hread(&header, 0L, fp) == 0L)
	    {
		(void) fprintf(stdout, "%s: header is unreadable\n", strv2);
		(void) fclose(fp);
	    }
	    else
	    {
		(void) fprintf(stdout, "Score is %d\n",
			       ftartscore(&header, strv, (char *)NULL));
		(void) fclose(fp);
	    }
	}
	else if (sscanf(cmdline, "g %s", strv) == 1)
	    (void) fprintf(stdout, "Score is %d\n",
			   ftgroupscore(strv, (char *)NULL));
	else if (sscanf(cmdline, "p %s", strv) == 1)
	{
	    if (ftprogram(strv) != FAIL)
		(void) fprintf(stdout, "Filter command accepted\n");
	    else
		(void) fprintf(stdout, "Filter command rejected\n");
	}
#ifdef DEBUG
	else if (sscanf(cmdline, "D %d", &level) == 1)
	    debug = level;
#endif /* DEBUG */
	else if (cmdline[0] == 'q' || cmdline[0] == 'x')
	    break;
	else
	    (void) fprintf(stdout, "Illegal command\n");
    }
    ftwrapup("Wrapup executed");
}
#endif /* MAIN */

/* lnfilter.c ends here */
