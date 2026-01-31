/****************************************************************************

NAME
   postart.c -- common code for news posting front ends

SYNOPSIS
    #include "libpost.h"

    char *mkfollowup(fhp)	-- mung followup headers into *fhp
    hdr_t *fhp;

    int artcopy(hp, fp, artfile, rot)	-- paste together a new copy
    hdr_t *hp; FILE *fp; char *artfile; bool rot;

    int savemsg(artfile)		-- append article to savefile
    char *artfile;

    char *recording(ngrps)	-- return warnings for sensitive groups
    char *ngrps;

DESCRIPTION
   This is a collection of message-composition functions for posting
programs. It is intended to be relatively user-interface independent.

   The mkfollowup() function cooks up a header for news or mail replies
to a given message. The first argument should be the message header; it
will be munged into the appropriate followup header.

   The artcopy() function takes a header block and a file pointer (which
is presumed to be seeked to the start of an article file's text) pastes
them together and puts the result in artfile. The file pointer may have
artfile opened for read; the function does a copy and rename. If rot13 is
TRUE the article text is rotated.

   The savemsg() function tries to save a copy of a given message to a
given location. It is intended for circumstances where the posting
program has to abort.

NOTE
   If new header line types are added to hdr_t, the code in mkfollowup()
will need to be modified correspondingly. It has to clear out all but a
select set of header lines in the header block pointed out by the second
argument, and can't just use hlfree() because we want mkfollowup(hp, hp)
to work.

ENVIRONMENT
   If RNESCAPES is enabled, this function gets the format of the attribution
line from the environment variable ATTRIBUTIONS, a la rn.

SEE ALSO
   postnews.c		-- for the ways these are used

AUTHOR
   Eric S. Raymond
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

****************************************************************************/
/*LINTLIBRARY*/
#include "news.h"
#include "vio.h"
#include "header.h"
#include "dballoc.h"
#include "alist.h"
#include "libpost.h"

ALIST(fgroups, 20, 10)   /* declare the followup control structure */

cite_t *mkfollowup(fhp)
/* modify the header pointed at to a followup, return a cite line */
hdr_t	*fhp;		/* followup header */
{
    static cite_t   cite;
    char	    *p;

    /* sock away the cite line to be used if we quote this */
#ifdef RNESCAPES
    if ((p = getenv("ATTRIBUTION")) == (char *)NULL)
	p = "In %i %e wrote:\n";
    strexpand(p, cite.citeline);
#else
    (void) snprintf(cite.citeline, sizeof(cite.citeline), "In %s ", header.h_ident);
    author(&header, cite.citeline + strlen(cite.citeline));
    (void) strlcat(cite.citeline, " wrote:\n", sizeof(cite.citeline));
#endif /* RNESCAPES */

    (void) strlcpy(cite.address, mailreply(fhp), sizeof(cite.address));
    hlfree(fhp->h_path);
    hlfree(fhp->h_from);

    /* Newsgroups */
    if (fgroups.file == (char *)NULL)
    {
	(void) snprintf(bfr, LBUFLEN, "%s/%s", site.admdir, "followups");
	fgroups.file = savestr(bfr);
	(void) dbaread(&fgroups);
    }

    if (hlnblank(header.h_followto))
    {
	if (fhp != &header)
	    (void) hlcpy(fhp->h_newsgroups, header.h_followto);
    }
    else if (p = afind(&fgroups, header.h_newsgroups))
	(void) hlcpy(fhp->h_newsgroups, p);

    /* Subject */
    newsubject(&header, fhp, (char *)NULL);

    /* References: */
    if (hlnblank(header.h_references))
    {
	(void) strlcpy(bfr, header.h_references, LBUFLEN);
	(void) strlcat(bfr, " ", LBUFLEN);
    }
    else
	bfr[0] = '\0';
    (void) strlcat(bfr, header.h_ident, LBUFLEN);
    (void) hlcpy(fhp->h_references, bfr);

    hlfree(fhp->h_ident);
    hlfree(fhp->h_expdate);
    hlfree(fhp->h_ctlmsg);
    hlfree(fhp->h_sender);
    hlfree(fhp->h_replyto);
    hlfree(fhp->h_followto);
    hlfree(fhp->h_distribution);
    hlfree(fhp->h_organization);
    hlfree(fhp->h_keywords);
    hlfree(fhp->h_summary);
    hlfree(fhp->h_approved);
    hlfree(fhp->h_supersedes);

    fhp->h_posttime=fhp->h_rectime=fhp->h_posttime=fhp->h_exptime=(time_t)0;
    fhp->h_intpriority = fhp->h_intnumlines = 0;

    return(&cite);
}

int artcopy(hp, fp, artfile, rotate)
/* paste a munged header and the body of its article together */
hdr_t	*hp;	    /* pointer to munged header */
FILE	*fp;	    /* file pointer, seeked to body of text */
char	*artfile;   /* new file name */
bool	rotate;	    /* whether to ROT13 the text */
{
    FILE	*wfp;
    int		c;
    char	copyto[BUFLEN];

    (void) strlcpy(copyto, "/tmp/postcopyXXXXXX", sizeof(copyto));
    (void) mktemp(copyto);

    if ((wfp = fopen(copyto, "w")) == (FILE *)NULL)
    {
	(void) msg1("Can't open copy file %s", copyto);
	return(FAIL);
    }
    hwrite(hp, wfp, FALSE);

    if (rotate)	/* we need to ROT13 the body */
    {
	(void) snprintf(bfr, LBUFLEN, "%s/%s 13 >> %s\n", site.libdir, "caesar", copyto);
	wfp = popen(bfr,  "w");
	while ((c = fgetc(fp)) != EOF)
	    (void) fputc(c, wfp);
	(void) pclose(wfp);
    }
    else		/* just append it to the header */
    {
	while ((c = fgetc(fp)) != EOF)
	    (void) fputc(c, wfp);
	(void) fclose(wfp);
    }
    (void) fclose(fp);
    (void) rename(copyto, artfile);

    return(SUCCEED);
}

char *recording(ngrps)
/* return name of a recorded message warning the poor luser what he is doing */
char *ngrps;
{
    static char recbuf[BUFLEN];
    FILE *fp;
    char nglist[BUFLEN], fname[BUFLEN];

    (void) snprintf(recbuf, sizeof(recbuf), "%s/%s", site.admdir, "recording");
    fp = fopen(recbuf, "r");
    if (fp == (FILE *)NULL)
	return((char *)NULL);
    while ((fgets(recbuf, sizeof recbuf, fp)) != (char *)NULL)
    {
	(void) sscanf(recbuf, "%s %s", nglist, fname);
	if (ngmatch(ngrps, nglist))
	{
	    (void) fclose(fp);
	    if (fname[0] == '/')
		(void) strlcpy(recbuf, fname, sizeof(recbuf));
	    else
		(void) snprintf(recbuf, sizeof(recbuf), "%s/%s", site.admdir, fname);
	    return(recbuf);
	}
    }
    return((char *)NULL);
}

void savemsg(article, ccname)
/* save a copy of the article in the ccname (or ccname/author_copy) */
char	*article, *ccname;
{
    FILE *in, *out;
    int c;
    time_t timenow;
    char *today;

    if (isdir(ccname))
	(void) strcat(ccname, "%s/author_copy");

    in = xfopen(article, "r");
    out = xfopen(ccname, "a");
    /* Unisoft 5.1 won't seek to EOF on "a" */
    (void) fseek(out, (off_t)0, SEEK_END);
    timenow = time((time_t *)NULL);
    today = ctime(&timenow);
    (void) fprintf(out,"From postnews %s",today);
    while ((c=getc(in)) != EOF)
	(void) fputc(c, out);
    (void) fputc('\n', out);
    (void) fclose(in);
    (void) fclose(out);
}

/* postart.c ends here */
