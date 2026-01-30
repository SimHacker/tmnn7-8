/*****************************************************************************

NAME
   clockdaemon.c -- autosave/time/mail-checker daemon for news readers

SYNOPSIS
   #include "libread.h"

   void getmailname();		-- compute current user's mailbox name

   void clockdaemon(interval, tripf);	-- call this to update daemon variables
   int interval; void (*tripf)();

DESCRIPTION
   These functions define a clock daemon useful for autosaving your .newsrc
file at intervals. If CHECKMAIL is defined it will also check your mail. If
SHOWTIME is defined it will make a human-readable time string available.

   Alarm setup or command-loop execution of the clockdaemon is left to the
I/O manager. Ideally it should be fired at 1-second intervals. The argument
is the number of minutes that should be allowed to elapse between autosaves.

ENVIRONMENT VARIABLES
   The getmailname() code looks for a mailbox name to check in MAILFILE or
MAIL before falling back on a system default.

NOTE
   This is all quite UNIX-dependent.

AUTHOR
   Eric S. Raymond
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

*****************************************************************************/
/*LINTLIBRARY*/
#include "news.h"
#include "mailbox.h"
#include "vio.h"

cldat_t	dclock;		/* public area for daemon-updated data */

#ifdef CHECKMAIL
#define MAILINSIST	30	/* remind user of new mail for ~ seconds */
private char *mailname;

void getmailname()
{
    register char *p;

    if ((p = getenv("MAILFILE")) != (char *)NULL)
    {
	fnexpand(p, bfr);
	mailname = savestr(bfr);
	return;
    }
    else if ((p = getenv("MAIL")) != (char *)NULL)
    {
	mailname = savestr(p);
	return;
    }
    else if (!strcmp(newsattr("mailformat", MAILFMT), "MMDF"))
    {
	(void) sprintf(bfr, "%s/mailbox", userhome);
	mailname = savestr(bfr);
    }
    else
	mailname = savestr(mailbox(username));
}
#endif /* CHECKMAIL */

void clockdaemon(tripf, interval)
/* display date and time (if configured) and do autosave if required */
void	(*tripf)();
int	interval;
{
    static time_t   timelastsaved = (time_t)0;	/* time of last .newsrc save */
    time_t tod;

    (void) time(&tod);
#ifdef SHOWTIME
    if (!dclock.clockoff)
    {
	struct tm *t = localtime(&tod);

	(void) sprintf(dclock.time, "%.3s %d %d:%02d",
		       "JanFebMarAprMayJunJulAugSepOctNovDec" + 3 * t->tm_mon,
		       t->tm_mday, t->tm_hour, t->tm_min);
    }
#endif /* SHOWTIME */

#ifdef CHECKMAIL
    if (!dclock.clockoff)
    {
	struct stat statb;
	static long oldmsize = 1000000L;
	static time_t lastcheck = 0;

	/* now check the user's mail */
	if (stat(mailname, &statb) == FAIL)
	    statb.st_size = 0;

	if (statb.st_size > oldmsize)
	    dclock.mailstate = M_NEWMAIL;	/* new mail has arrived */
	else
	{
	    if (statb.st_size == 0)
		dclock.mailstate = M_NOMAIL;	/* no mail */
	    else if (dclock.mailstate!=M_NOMAIL && (lastcheck+MAILINSIST)<tod)
		dclock.mailstate = M_WAITING;	/* new mail still waiting */
	}
	oldmsize = statb.st_size;
	lastcheck = tod;
    }
#endif /* CHECKMAIL */

    /* autosave if indicated */
    if (dclock.justsaved = (interval != 0 && tod - timelastsaved > interval))
    {
	(*tripf)();
	timelastsaved = tod;
    }
}

/* clockdaemon.c ends here */
