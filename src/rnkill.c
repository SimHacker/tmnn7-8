/*****************************************************************************

NAME
   rnkill -- news filter that approximately emulates rn kill language

SYNOPSIS
   rnkill [-pD] [-d homedir]

DESCRIPTION
   This code builds on the lnfilter.c facilities to implement a kill language
resembling that of rn.

AUTHOR
   Eric S. Raymond.
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

*****************************************************************************/
/*LINTLIBRARY*/
#include "news.h"
#include "active.h"
#include "header.h"
#include "regexp.h"
#include "libfilt.h"

#define RNKILLV	"100"	/* version string, increment when we change this */

/* search scopes for regular-expression commands */
#define L_SUBJECT   0	/* match subject */
#define L_HEADER    1	/* match any header */
#define L_TEXT	    2	/* match anywhere in text or header */

private int rnscore(re, cmd, grp, hp)
/* kill language interpreter */
regexp	*re;	/* compiled RE from pattern part of command */
char	*cmd;	/* command, with the RE part (but not delimiters) dropped */
group_t	*grp;	/* current group */
hdr_t	*hp;	/* header of article to be scored (NULL for group scoring) */
{
    bool	caseblind, match;
    int		level;

    /* rn's kill language doesn't do group scoring */
    if (hp == (hdr_t *)NULL)
	return(0);

    /* OK, we're scoring an article, skip the // RE-delimiter pair */
    cmd += 3;

    /* handle the fast simple case first */
    if (*cmd == '\0')
	return(regexec(re, hp->h_subject) ? -1 : 0);

    /* determine the level of the desired match */
    level = L_SUBJECT;
    caseblind = TRUE;
    if (strchr(cmd, ':'))
    {
	for (; *cmd != ':'; cmd++)
	    if (*cmd == 'h')
		level = L_HEADER;
	    else if (*cmd == 'a')
		level = L_TEXT;
	    else if (*cmd == 'c')
		caseblind = FALSE;
	cmd++;
    }

    /* go to it */
    if (level == L_TEXT || level == L_HEADER)
    {
	(void) fseek(hp->h_fp, hp->h_startoff, SEEK_SET);
	while (fgets(bfr, LBUFLEN, hp->h_fp))
	{
	    if (level == L_HEADER && bfr[0] == '\n')
		break;

	    if (caseblind)
		lcase(bfr);

	    if (regexec(re, bfr))
		match = TRUE;
	}

	match = FALSE;
    }
    else    /* we only need to look at one header line, don't fetch article */
    {
	if (!caseblind)
	    match = regexec(re, header.h_subject);
	else
	{
	    (void) strcpy(bfr, header.h_subject);
	    lcase(bfr);
	    match = regexec(re, bfr);
	}
    }
    if (!match)
	return(0);

    /* OK, we have a match, decide what to do next */
    if (cmd[0] == '\0' || cmd[0] == 'k')
	return(-1);	
    else if (cmd[0] == 'j')
	return(-2);

    /*
     * User sent down a more complex command than we can handle yet,
     * we have to punt this.
     */
#ifdef DEBUG
    if (debug)
	(void) fprintf(stderr, "rnkill: punted on %s\n", cmd);
#endif /* DEBUG */
    return(0);
}

private int cmdproc(q, p)
/* kill command interpreter */
char	*q, *p;
{
    (void) strcpy(q, p);
    return(SUCCEED);
}

main(argc, argv)
int argc;
char *argv[];
{
    return(lnfilter("rnkill",RNKILLV, rnscore, cmdproc, argc,argv));
}

catch_t xxit(status) int status; {exit(status);}

#ifdef lint	/* these just shut up lint for when we link with libread.a */
void vupdate() {/*no-op*/}
int vstat;
void vuline(to, from) char *to, *from; {(void)strcpy(to, from);}
# include "gcmd.h"	/* space fakes out GenerateMake about dependency */
/*ARGSUSED0*/
void vcommand(args) cmdarg_t *args; {/*no-op*/}
#endif

/* rnkill.c ends here */
