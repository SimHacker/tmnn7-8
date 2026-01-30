/****************************************************************************

NAME
   editart.c -- common code for editing an RFC-822 message

SYNOPSIS
    #include "libpost.h"

    void newsubject(hp, fhp, subject)	-- concoct a followup subject
    hdr_t *hp, *fhp; char subject;

    char *prepmsg(hp, pf, cite, imark)	-- prepare a new one
    hdr_t *hp; char *pf, *cite, *imark;

    int editmsg(artfile)		-- edit an existing one
    char *artfile;

    int signmsg(artfile)		-- append user .signature
    char *artfile;

DESCRIPTION
   This is a group of message-composition functions for readers and posting 
programs. It is intended to be relatively user-interface independent.

   The newsubject() function generates an appropriate subject line for a
followup or reply.

   The prepmsg() function creates and initializes a tempfile holding
the contents of the indicated header and (optionally) an include of
a referenced parent article.

   The editmsg() function accepts the name of a temp file to be edited. It
calls the editor specified by the environment variable EDITOR on it; if
that variable is null, it looks for the news configuration define
DFTEDITOR. It tries to concatenate the user's ~/.signature to the result.

   The signmsg() function appends the contents of the current user's
~/.signature file to the given article, if the signature file exists. If
the signature file is executable, standard output from a run is used as
the signature text (the SIGLINES limit in effect, if any, still applies).

ENVIRONMENT
   The editmsg() code tries to find an editor name in EDITOR (first) or
VISUAL (second) before falling back on the default compiled in. This order
allows you to set the editing mode for your ksh independently of your posting
editor.

FILES
   ADM/followup		-- association list of groups, followup targets.
   /tmp/post??????	-- hold the article being composed
   /tmp/copyto??????	-- scratch file for article filtering
   ~/.signature		-- user's signature file

BUGS
   The editmsg() function should know how to take us to the message body
start under GNU and Gosling Emacs versions (the KNOWEMACS code below) but
for various grungy reasons it doesn't yet.

SEE ALSO
   postnews.c		-- uses these functions for article composition
   rfuncs.c		-- uses these functions for reply composition

AUTHOR
   Eric S. Raymond
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

****************************************************************************/
/*LINTLIBRARY*/
#include "news.h"
#include "header.h"
#include "libpost.h"
#include "spawn.h"

void newsubject(hp, fhp, subject)
/* create the appropriate subject line for a reply or followup */
hdr_t	*hp;		/* original message header */
hdr_t	*fhp;		/* new message header */
char	*subject;	/* new subject (may be empty) */
{
    register char	*oldsubj = hp->h_subject;

    if (subject != (char *)NULL && subject[0])
	(void) sprintf(bfr, "%s (was: %s)", subject, oldsubj);
    else if (hlnblank(oldsubj))
    {
	if (!prefix(oldsubj, "Re:"))
	    (void) sprintf(bfr, "Re: %s", oldsubj);
	else
	    (void) strcpy(bfr, oldsubj);
    }
    else
    {
	if (hlnblank(hp->h_from))
	    (void) sprintf(bfr, "Re: orphan response from %s", hp->h_from);
	else
	    (void) strcpy(bfr, "Re: orphan response");
    }

    (void) hlcpy(fhp->h_subject, bfr);
#ifdef ALLOCHDRS
    if (hp == fhp)
	hlfree(oldsubj);
#endif /* ALLOCHDRS */
}

char *prepmsg(hp, parentfile, cite, imark)
/* generate a new article, possibly including text of a parent article */
hdr_t	*hp;
char	*parentfile;
char	*cite;
char	*imark;
{
    FILE *tf, *pf;
    static char	artfile[BUFLEN];

    (void) strcpy(artfile, "/tmp/postXXXXXX");
    (void) mktemp(artfile);

    /* insert a header */
    tf = xfopen(artfile, "w");
    hwrite(hp, tf, FALSE);

    /* if a parent file has been specified, include it and a cite */
    if (parentfile != (char *)NULL && parentfile[0])
    {
	pf = xfopen(parentfile, "r");
	while (fgets(bfr, BUFSIZ, pf) != (char *)NULL)
	    if (bfr[0] == '\n')	/* skip headers */
		break;
	if (cite != (char *)NULL && cite[0])
	    (void) fputs(cite, tf);
	while (fgets(bfr, BUFSIZ, pf) != (char *)NULL)
	    (void) fprintf(tf, "%s%s", imark, bfr);
	(void) fclose(pf);
    }

    (void) fflush(tf);
    hp->h_posttime = modtime(artfile);
    (void) fclose(tf);
    return(artfile);
}

int editmsg(artfile)
/* edit a message */
char	*artfile;
{
    register char *p;
    char *editor, editcmd[BUFLEN], *preflag = "";

    /* first, figure out exactly what our editor is */
    if (!((editor = getenv("EDITOR")) || (editor = getenv("VISUAL"))))
	editor = DFTEDITOR;
    if (editor == (char *)NULL || editor[0] == '\0')
	return(FAIL);
    if (p = strrchr(editor, '/'))
	p++;
    else
	p = editor;
    fnexpand(p, editcmd);

    /*
     * Try and arrange for the editor to go to the start of new text
     * This is soooo bogus that it makes me puke, but there's no
     * better way.
     */
    if (strcmp(editcmd, "vi") == 0)
	preflag = "\'+/^$\'";
#ifdef KNOWEMACS	/* this doesn't work yet */
    else if (strcmp(editcmd, "emacs") == 0)
    {
	(void) sprintf(bfr, "%s/.emacs", userhome);
	if (access(bfr, F_OK) == SUCCEED)	/* probably GNU Emacs */
	    preflag = "-f (re-search-forward \"^$\")";
	else
	    /* probably GOSMACS -- you lose */;
    }
#endif /* KNOWEMACS */

    if (preflag)
    {
	(void) strcat(editcmd, " ");
	(void) strcat(editcmd, preflag);
    }
    (void) strcat(editcmd, " ");
    (void) strcat(editcmd, artfile);
	
    return(shcmd(editcmd, FORGRND | NOPRIVS, artfile));
}

void signmsg(artfile)
/* try to append the user's .signature file if indicated */
char	*artfile;
{
    int siglines = atoi(newsattr("siglines", SIGLINES));

    if (access(artfile, R_OK) == SUCCEED)
    {
	FILE	*article, *signature;
	int	c, slinecount = 0;
	bool	program;

	/* set up to append to article */
	if ((article = fopen(artfile,"a")) == (FILE *)NULL)
	    return;
	else		/* Unisoft 5.1 won't seek to EOF on "a" */
	    (void) fseek(article, (off_t)0, SEEK_END);

	/* open signature-generation file or program */
	(void) sprintf(bfr, "%s/%s", userhome, ".signature");
	if (program = (access(bfr, X_OK) == SUCCEED))
	    signature = popen(bfr, "r");
	else if ((signature = fopen(bfr,"r")) == (FILE *)NULL)
	    return;

	/* snarf characters till we hit our lines limit */
	(void) fprintf(article, "-- \n");	/* To separate */
	while ((c = fgetc(signature)) != EOF)
	{
	    (void) fputc(c, article);
	    if (c == '\n' && siglines > 0 && ++slinecount >= siglines)
		break;
	}

	/* cleanup time */
	(void) fclose(article);
	if (program)
	    (void) pclose(signature);
	else
	    (void) fclose(signature);
    }
}

/* editart.c ends here */
