/*****************************************************************************

NAME
   mailbox.c -- mailbox and save file manipulation

SYNOPSIS
   #include "mailbox.h"

   char *mailbox(mname)		-- generate mailbox name corresponding to mname
   char *mname;

   int saveto(fname, flags)	-- file saving function
   char *fname; int flags;

DESCRIPTION
   This function writes the headers and text of the current message to a
save file. Control options may be selected via the flags argument.

   S_SAVEHDR	write out article header
   S_OVWRITE	overwrite the file if it already exists
   S_MAILFMT	save in mail format
   S_NOSIGNAL	block interrupts while saving

The seek offset in the header's file pointer is preserved. This routine
returns FAIL if the copy fails, otherwise TRUE or FALSE according to
whether the file existed before the call.

    Presently, this code only knows about V7 and MMDF save formats. It
depends on newsattr() to tell it which to use. The two symbols it checks
are MAILFORMAT and MAILROUTING.

BUGS
   Should use MAILCHAR to try and *deduce* the format of nonempty mailbox and
save files.

AUTHOR
   Eric S. Raymond
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

*****************************************************************************/
/* LINTLIBRARY */
#include "news.h"
#include "mailbox.h"
#include "header.h"

char *mailbox(mname)
char	*mname;
{
#ifdef USG
    (void) sprintf(bfr, "/usr/mail/%s", mname);
#else /* !USG */
    (void) sprintf(bfr, "/usr/spool/mail/%s", mname);
#endif /* !USG */
    return(bfr);
}

int saveto(fname, flags)
register char *fname;	/* file to save to */
int flags;		/* append/overwrite, save header vs. article-only */
{
    FILE	*ufp;
    long	saveoffs;
    bool	existed;
    static char *marktype = (char *)NULL;
    static bool domainist = FALSE;
    static bool v7mail = FALSE;

    existed = (access(fname, F_OK) == SUCCEED);
    if ((ufp = fopen(fname, (flags & S_OVWRITE) ? "w" : "a")) == (FILE *)NULL)
	return(FAIL);
    else if (header.h_fp == (FILE *)NULL)
	return(FAIL);
    else if (!(flags & S_OVWRITE))
	/* Unisoft 5.1 won't seek to EOF on "a" */
	(void) fseek(ufp, (off_t)0, SEEK_END);

    /* if first time through, compute our save format */
    if ((flags & S_MAILFMT) && marktype == (char *)NULL)
    {
	marktype = newsattr("mailformat", MAILFMT);
	v7mail = strcmp(marktype, "MMDF");
	domainist = strcmp(site.smarthost, "");
    }

    /* generate the proper save header to go with the format */
    if (flags & S_MAILFMT)
    {
	if (strcmp(marktype, "MMDF") && ftell(ufp) != (off_t)0)
	    (void) fprintf(ufp, "\001\001\001\001\n");
	else
	    (void) fprintf(ufp, "From %s %s",
			   domainist ? header.h_from : header.h_path,
			   ctime(&header.h_posttime));
    }

    /* maybe the header should be saved */
    if (flags & S_SAVEHDR)
	hwrite(&header, ufp, TRUE);

    /* OK, tack on the body (uninterruptibly, if requested) */
    saveoffs = ftell(header.h_fp);
    (void) fseek(header.h_fp, (off_t)header.h_textoff, SEEK_SET);
    while (fgets(bfr, sizeof(bfr), header.h_fp) != (char *)NULL)
    {
	if (sigcaught && !(flags & S_NOSIGNAL))
	    break;
#ifdef MSEGMENT
	if (header.h_endoff != (off_t)0 && (header.h_endoff<ftell(header.h_fp))
	    break;
#endif /* MSEGMENT */

	if (flags & S_MAILFMT && v7mail)
	    if (strncmp(bfr, "From ", 5) == 0)
		(void) putc('>', ufp);
	(void) fputs(bfr, ufp);
    }
    if (flags & S_MAILFMT && v7mail)
	(void) putc('\n', ufp);	/* force blank line at end (ugh) */
    (void) fflush(ufp);
    (void) fprintf(ufp, ((sigcaught && !(flags & S_NOSIGNAL)) ? "\n\n":"\n"));
    sigcaught = FALSE;
    (void) fclose(ufp);
    (void) fseek(header.h_fp, (off_t)saveoffs, SEEK_SET);
    return(existed);
}

/* mailbox.c ends here */
