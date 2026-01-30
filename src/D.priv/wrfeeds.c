/***************************************************************************

NAME
   wrfeeds.c -- functions that modify the feeds file

SYNOPSIS
   #include "feeds.h"

   void s_changesub(sitename, subsc, addit)	-- change subscription
   char *site, *subsc; bool addit;

   int s_writerec(sys, fp)			-- write a feeds record
   feed_t *sys; FILE *fp;

   void s_write()				-- write feeds file

DESCRIPTION
   This module provides program-controlled modify access to the feeds file.
The changesub() function permits you to add or delete groups from a
subscription.

   The s_write() function causes the modified version of the feeds file
to be written to disk. Note that it moves all comments preceding or within
a given feed entry to just before the entry. Also note that rewritten logical
lines may be broken into physical lines in a way that differs from the
original.

FILES
   ADM/feeds	-- file that controls your feeds to other sites

NOTE
   For this library module to operate properly, sitenames() must have been
called.

REVISED BY
   Eric S. Raymond
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

****************************************************************************/
/* LINTLIBRARY */
#include "news.h"
#include "dballoc.h"
#include "feeds.h"

/* the maximum line length to assume in dumping the feeds file */
#define LINESIZE	76

void s_changesub(sitename, subsc, addit)
char *sitename;	/* site to alter */
char *subsc;	/* new subscription data */
bool addit;	/* whether to add or delete it */
{
    feed_t	*sp;

    /* find or copy a record for the named site alone */
    if ((sp = s_find(sitename)) == (feed_t *)NULL)
	return;
    else if (strchr(sp->s_name, NGDELIM) != (char *)NULL)
    {
	(void) setsub(sp->s_name, sitename, LISTSEP);
#ifdef lint
	sp = (feed_t *)NULL;
#else
	dbaenter(&feeds, (char *)sp);
	sp = (feed_t *)feeds.cp;
#endif /* lint */
	(void) strcpy(sp->s_name, sitename);
    }

    /* OK, now change the record */
    if (addit)
    {
	char	gcopy[LBUFLEN];
	/*
	 * We can't do the set addition in place, the catenation might step
	 * on the distribution and later fields in the feed record buffer.
	 * If the set addition leads to no changes, no problem; otherwise,
	 * we must resort to a kluge.
	 */
	(void) strcpy(gcopy, sp->s_ngroups);
	if (setadd(gcopy, subsc, LISTSEP) != 0)
	    sp->s_ngroups = savestr(gcopy);
    }
    else if (setsub(sp->s_ngroups, subsc, LISTSEP) == 0)
    {
	char	negate[SBUFLEN];

	negate[0] = NEGCHAR;
	(void) strcpy(negate + 1, subsc);
	(void) s_changesub(sitename, negate, TRUE);
    }
}

#define FOLD
#ifdef FOLD
static char qfputc(c, ofp)
/* queue chars for logical-line output with \-<SPACE>-<TAB> continuations */
char	c;
FILE	*ofp;
{
    static char		qbuf[LINESIZE + 1], *qp = qbuf;

    if (c == '\n')	/* <NL> forces a queue flush */
    {
	(void) fputs(qp = qbuf, ofp);
	(void) fputc('\n', ofp);
	qbuf[0] = '\0';
    }
    else		/* queue up the character for output */
    {
	*qp++ = c;
	*qp = '\0';
    }

    /* is a queue flush needed due to nearing the line length? */
    if (strlen(qbuf) + 1 > LINESIZE)
    {
	char	firstchar;

	/* drop the queue pointer back to the last item break */
	while (*qp != NGDELIM && *qp != S_FLDSEP)
	    qp--;

	/* write everything up to and including the last break character */
	firstchar = qp[1];
	qp[1] = '\0';
	(void) fputs(qbuf, ofp);
	(void) fputs("\\\n", ofp);
	qp[1] = firstchar;

	/* take the un-flushed portion of the line to the buffer start */
	qbuf[0] = ' ';
	(void) strcpy(qbuf + 1, qp + 1);
	qp = qbuf + strlen(qbuf);
    }

    return(c);
}

static char *qfputs(line, ofp)
/* queue line up for max LINESIZE-char output */
char	*line;
FILE	*ofp;
{
    register char	*lp;

    for (lp = line; *lp; lp++)
	(void) qfputc(*lp, ofp);
    return(line);
}

#define fputs	qfputs
#define fputc	qfputc
#endif /* FOLD */

int s_writerec(sys, fp)
/* dump an altered feeds record file to the given file pointer */
feed_t	*sys;
FILE	*fp;
{
    /* comments */
    (void) fwrite(sys->s_comments, sys->s_clength, sizeof(char), fp);

    /* system names field */
    (void) fputs(sys->s_name, fp);
    if (sys->s_nosend[0])
    {
	(void) fputc(S_FLEXSEP, fp);
	(void) fputs(sys->s_nosend, fp);
    }
    (void) fputc(S_FLDSEP, fp);

    /* feed groups field */
    (void) fputs(sys->s_ngroups, fp);
    if (sys->s_gflex != (char *)NULL && strcmp(sys->s_gflex, "all"))
    {
	(void) fputc(S_FLEXSEP, fp);
	(void) fputs(sys->s_gflex, fp);
    }
    (void) fputc(S_FLDSEP, fp);

    /* distributions */
    (void) fputs(sys->s_distribs, fp);
    if (sys->s_dflex != (char *)NULL && strcmp(sys->s_dflex, "all"))
    {
	(void) fputc(S_FLEXSEP, fp);
	(void) fputs(sys->s_dflex, fp);
    }
    (void) fputc(S_FLDSEP, fp);

    /* flags */
    if (sys->s_flags != (char *)NULL)
	(void) fputs(sys->s_flags, fp);
    (void) fputc(S_FLDSEP, fp);

    /* flags */
    if (sys->s_xmit != (char *)NULL)
	(void) fputs(sys->s_xmit, fp);
    (void) fputc(S_FLDSEP, fp);

    (void) fputc('\n', fp);
    return(SUCCEED);
}

#ifdef DEBUG
void s_dump(fp)
FILE	*fp;
{
    feeds.rput = s_writerec;
    (void) dbadump(&feeds, fp);
}
#endif /* DEBUG */

void s_write()
{
    feeds.rput = s_writerec;
    dbawrite(&feeds);
}

/* wrfeeds.c ends here */
