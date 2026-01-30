/*****************************************************************************

NAME
   gcmd.c -- the generic command interpreter

SYNOPSIS
   cmdarg_t *gparse(bfr)	-- the generic command parser
   char *bfr;

   void gcmd(args)  		-- the generic command interpreter
   cmdarg_t *args;

DESCRIPTION
   This is the generic command interpreter for readnews and vnews.
It expects the following functions to be available:

   msg()	-- printf() with up to 4 args to a message display
   vgetc()	-- get another command character
   vgetline()	-- emit a prompt and return a string of input
   vdelay()	-- delay to be sure a message is read
   vttymode()	-- go to normal tty mode (may be a no-op)
   vcrtmode()	-- go to screen mode or whatever (may be a no-op)
   vexec()	-- execute a command string, suppressing motion commands

The code below always does all the vgetc() calls it needs before at most one
vgetline() call, thus these can be implemented with pointer walks on a line.
The gcmd() uses and sets a status word (the variable 'action') which contains
the following bit fields:

   NOACT	-- all bits zero, do no special actions
   HOLD		-- regenerate article display, save former action
   EXIT		-- exit, no cleanup
   NOMORE	-- no more articles
   CMDERR	-- unrecognized command

COMMANDS
   The following is a list of the commands accepted by this function:

A		Go to the message number given by <count>
b		Back up one article in the current group.
c		Cancel an article you posted.
E		Delayed erase. Takes effect on move to new group.
e		Erase.  Forget that an article was read.
f [title]	Submit a follow up article.
F		Like f, but include text of current article
G		Subscribe to a given group. 
j		Mark current message seen.
J		Mark everything to the end of the current group globally seen.
k		Kill the current article in this group only
K		Mark the rest of the articles in current group as read.
ALT-k		Pass command to news filter
N [newsgroup]	Move forward <count> newsgroups or to the named newsgroup.
n, .		No.  Goes on to next article without printing current one.
P [newsgroup]	Move back <count> newsgroups or to the named newsgroup.
p		Go to parent article of current followup.
q		Quit.  Update .newsrc if -l or -x not used.
R [subject]	Reply, including a copy of the article.
r [subject]	Reply.  Reply to article's author via mail.
ALT-r		Reply direct (call your mailer in interactive mode)
s [file]	Save.  Article is appended to file (default is "Articles").
s | program	Run program with article as standard input.
t		Mark as read all articles with same subject as current one
T		Like t, but also add the t command to the kill file
U [newsgroup]	Unsubscribe.  You won't be shown this newsgroup anymore.
ug [newsgroup]	Same as U
ud		Unsubscribe from discussion; ignore followups of this article.
v		Version.  Print current news version number.
W		Write out your .newsrc file
w, w |		As s, but omit the article header.
x		Exit.  Don't update .newsrc.
-		Go back <count> articles.
+		Skip forward <count> articles, ignoring subject threads
! [command [&]]	Shell escape (final & runs command in background).
!!		Like !, but run with site.textdir as current directory
/		search messages forward
\		search messages backward
#		Show our current location.
&		Interpret switches from the command line.
%		Define a command macro
^		Go to last top node of discussion.
<message-id>	Go to message with given Message-ID
| program	Shorthand for 's |'
ALT-p		Praise current article
ALT-c		Condemn current article

If the count argument to the ! command is zero, the code will emit a
"continue?" prompt after executing the command. If the count argument of the
reply command is zero, the reply is unedited.

   If KEYPAD is enabled, the following keycaps are mapped into commands:

KEY_DOWN		N (next group)
KEY_UP			P (previous group)
KEY_CANCEL		c (cancel)
KEY_COPY		w (save, no headers)
KEY_EXIT		q (quit)
KEY_FIND		f (find)
KEY_HELP		? (help)
KEY_MARK		j (mark)
KEY_MESSAGE		r (reply)
KEY_MOVE		A (move to given article number)
KEY_OPTIONS		& (set options)
KEY_REDO		e (erase)
KEY_REFERENCE		p (parent)
KEY_RESTART		W (write .newsrc)
KEY_SAVE		s (save)
KEY_SUSPEND		! (spawn shell)
KEY_SEXIT		x (exit)
KEY_SFIND		\ (find backward)
KEY_SMESSAGE		R (reply with inclusion)
KEY_SMARK		J (mark to end)
KEY_SREDO		E (delayed erase)

If GDEBUG is enabled and debug is on, each command is logged along with the
article location it starts from.

NOTE
   The 'E' command is enabled by the DELAYMARK compilation switch.

   The '/', '\', 'k', 't' and 'T' commands are enabled by the RECMDS
compilation switch.

   The logic that maps followups to 'poster' to mail replies lives here. It
probably belongs at a lower level.

FILES
   ~/.gcmdlog	-- command log

BUGS
   The CTRL() & ALT() macros may do weird things in a non-ASCII (i.e. EBCDIC)
environment. But if you're using EBCDIC you're probably stuck with 3270-style
block I/O and hence thoroughly f***ed anyhow.
   The r/R command code includes ugly dependencies on specific mailer types.
   The 'T' command causes /<subject>/t to be passed to the current newsfilter.

AUTHOR
   Eric S. Raymond
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

*****************************************************************************/
/*LINTLIBRARY*/
#include "news.h"
#include "mailbox.h"
#include "vio.h"
#include "header.h"
#include "active.h"
#include "newsrc.h"
#include "nextmsg.h"
#include "dballoc.h"	/* session.h needs the dbdef_t type */
#include "session.h"
#ifdef RECMDS
#include "regexp.h"
#endif /* RECMDS */
#ifdef NEWSFILTER
#include "libfilt.h"
#endif /* NEWSFILTER */
#include "rfuncs.h"
#include "gcmd.h"
#include "version.h"

#define V_SHOWRATE	1	/* confirm praise/condemn commands */

int action;			/* what to do next */

#define CMDTEXT	64	/* max chars of kill command we can process */
#define DEFCMD	'\n'	/* default command (use for blank line) */
#define MINUS	'-'	/* postfix char, to be counted for c_minus */
#define PREFCHARS	"0123456789,"

#ifdef GDEBUG
private FILE	*cmdlog;	/* access to command log file */
#endif /* GDEBUG */

cmdarg_t *gparse(vbfr)
/* parse a command out*/
char	*vbfr;
{
    static cmdarg_t args;
    char *cp;

    /* parse the (possibly comma-punctuated) digit sequence in bfr */
    if (vbfr[0] == C_RMARK)		/* range mark followed by whatever */
    {
	args.c_argform = C_UPTO;
	args.c_count = 1;
	if ((args.c_upto = atoi(vbfr + 1)) == 0)
	    args.c_upto = ngmax();
    }
    else if (cp = strchr(vbfr, C_RMARK))	/* stuff + rangemark + stuff */
    {
	args.c_argform = C_RANGE;
	if ((args.c_count = atoi(vbfr)) == 0)
	    args.c_count = msgnum();
	if ((args.c_upto = atoi(cp + 1)) == 0)
	    args.c_upto = ngmax();
    }
    else if (isdigit(vbfr[0]))
    {
	args.c_argform = C_COUNT;
	args.c_count = atoi(vbfr);
    }
    else					/* no prefix */
    {
	args.c_argform = C_NONE;
	args.c_count = 1;
    }

    /* now pick up the command character */
    cp = vbfr + strspn(vbfr, PREFCHARS);
    args.c_char = *cp++;
    args.c_char &= 0xff;	/* in case chars sign-extend */
    vsetbuf(cp);		/* point vgetc() at rest of command, if any */
    return(&args);
}

private void onecmd(args)
/* interpret one single command */
cmdarg_t *args;
{
    char		*mptr;
    register int	cc = args->c_char;
    int			clet2;
    bool		svreread;

#ifdef GDEBUG
    msgrec_t	*mp = session.cmsg;

    if (debug)
    {
	char	cbfr[LBUFLEN];

	if (cmdlog == (FILE *)NULL)
	{
	    (void) sprintf(cbfr, "%s/.gcmdlog", userhome);
	    cmdlog = xfopen(cbfr, "w");
	}

	(void) fprintf(cmdlog,
		  "From {{%s/%ld}, %s, &%d}, command %c\n",
		  mp->loc.m_group->ng_name, (long)mp->loc.m_number,
		  mp->follow ? mp->follow : "<none>",
		  mp->parent ? (mp->parent - TRAILBASE) : -1,
		  args->c_char);
    }
#endif /* GDEBUG */

    /* now interpret a command */
    switch (cc)
    {
    case 'A':
	demand(HOLD);
	if (args->c_count < (long)ngmin())
	    msg0("No longer on line.");
	else if (args->c_count > (long)ngmax())
	    msg0("Not that many articles.");
	else if (gotomsg((nart_t)args->c_count) == SUCCEED)
	    session.action = M_SEEK;
	break;

    case 'b':		/* back up by args->c_count articles */
	session.action = M_SEEK;
	while (args->c_count--)
	    if (nextmsg(TRUE, !session.reverse) == FAIL)
	    {
		msg0("Can't back up further");
		demand(HOLD);
		break;
	    }
	break;

    case 'c':		/* cancel the article. */
	demand(HOLD);
	if ((mptr = vgetline("Cancel? [n] ")) == (char *)NULL)
	    break;
	else if (*mptr == 'y')
	    msg0(cancel_command(&header));
	break;

    case 'e':		/* erase - pretend we haven't seen this article. */
	demand(HOLD);
	(void) msgmark(UNREAD, args->c_count ? GLOBAL : LOCAL);
	msg1("Holding article %d", msgnum());
	break;

#ifdef DELAYMARK
    case 'E':		/* delayed erase -- effective on next group start */
	demand(HOLD);
	(void) msgmark(UNREAD, DELAYED | LOCAL);
	msg1("Holding article %d", msgnum());
	break;
#endif /* DELAYMARK */

    case 'f':		/* post a followup to this article */
    case 'F':
	if (session.thread && hlnblank(header.h_backrefs))
	{
#define FPROMPT	"There are followups you haven't seen; respond anyway? [n] "
	    char	followups[LBUFLEN], *cp;
	    int		unread = 0;
	    place_t	artloc, oldloc;

	    (void) tellmsg(&oldloc);
	    (void) strcpy(followups, header.h_backrefs);
	    cp = strtok(followups, " \t");
	    do {
		if (hstseek(cp, FALSE) == SUCCEED)
		{
		    bool seenit = FALSE;

		    while (hstloc(&artloc) == SUCCEED)
			if (getbit(artloc.m_number, artloc.m_group) == TRUE)
			    seenit = TRUE;
		    if (!seenit)
			unread++;
		}
	    } while
		((cp = strtok((char *)NULL, " \t")) != (char *)NULL);
	    (void) seekmsg(&oldloc);
	    if (unread)
		if ((mptr = vgetline(FPROMPT)) == (char *)NULL || *mptr != 'y')
		    break;
	}

	if (hlblank(header.h_followto) || strcmp(header.h_followto, "poster"))
	{
	    vttymode();
	    followup(cc == 'F');
	    vcrtmode(TRUE);
	    demand(HOLD);
	    break;
	}
	/* if 'Followup-To: poster' fall through and reply by mail */

    case 'r':		/* mail reply */
    case 'R':
    case ALT('r'):
    case ALT('R'):
	/* make clet TRUE if we want to reply direct, FALSE otherwise */
	clet2 = (CHR(cc) == ALT('r') || CHR(cc) == ALT('R'));
	/*
	 * KLUGE: we prompt for subject if we're editing the reply (so the
	 * "(was: ...)" format can be generated), or if the front end is
	 * raw binmail. This is ugly and will lose in a minor way if the
	 * user specifies another front end that's as primitive as /bin/mail,
	 * but it handles the common cases (/bin/mail, mailx, and Mail) OK.
	 */
	if (!clet2 || strcmp(MAILFRONT, "/bin/mail") == 0)
	    mptr = vgetline("Subject? ");
	else
	    mptr = (char *)NULL;
	vttymode();
	if (clet2)
	    mptr = direct_reply(mptr);
	else
	    mptr = edited_reply((CHR(args->c_char) == 'R'), mptr);
	vcrtmode(TRUE);
	if (mptr)
	    msg0(mptr);
	demand(HOLD);
	break;

    case 'j':		/* mark current message seen */
	(void) msgmark(READ, GLOBAL);
	break;

    case 'J':		/* kill (globally) everything to end-of-group */
	while (args->c_count--)
	{
	    group_t	*oldgrp = ngactive();

	    session.action = M_NEXT;
	    do {
		(void) msgmark(READ, GLOBAL);
	    } while
		(msgnext() != FAIL && ngactive() == oldgrp);
	}
	session.action = M_HOLD;
	break;

    case 'k':		/* mark current message seen */
	(void) msgmark(READ, LOCAL);
	break;

    case ALT('k'):
#ifndef NEWSFILTER
	msg0("The news filter features have been disabled, sorry!");
#else
	if ((mptr = vgetline("Filter command? ")) == (char *)NULL)
	{
	    (void) fltprogram(mptr);
	    msg0(mptr);
	}
#endif /* NEWSFILTER */
	break;

    case 'n':		/* No.  Go on to next article. */
    case '.':		/* Useful if you have a keypad */
	session.action = M_SKIP;
	while (args->c_count--)
	{
	    (void) msgmark(READ, GLOBAL);
	    if (msgnext() == FAIL)
	    {
		vdelay();
		demand(NOMORE);
		break;
	    }
	}
	session.action = M_HOLD;
	break;

    case 'P':		/* go to previous newsgroup */
	args->c_count *= -1L;
    case 'N':		/* go to next newsgroup */
    case 'G':		/* subscribe to a group */
	if ((mptr = vgetline("Group? ")) == (char *)NULL)
	    break;
	svreread = (CHR(cc) == 'G');
	if (change_group(mptr, (int)args->c_count,
			 svreread || session.reread,
			 session.reverse) == SUCCEED)
	{
	    if (svreread)
		subscribe();
	    session.action = M_SEEK;
	}
	else
	    demand(HOLD);
	break;

    case 'q':		/* exit - update time to that of most recent msg */
	demand(NOMORE | EXIT | HOLD);
	break;

    case '|':		/* send article to a command (with headers) */
	vungetch('|');
	cc = 's';
	/* FALL THROUGH */
    case 's':		/* write out the article someplace */
    case 'w':
	demand(HOLD);
	if ((mptr = vgetline("File? ")) == (char *)NULL)
	    break;
	clet2 = 0;
	if (cc == 's')
	    clet2 |= (S_SAVEHDR | S_MAILFMT);
	if (args->c_count != 1L)
	    clet2 |= S_OVWRITE;
	if (strchr(mptr, '|'))
	    vttymode();
	msg0(stuffit(mptr, clet2));
	if (strchr(mptr, '|'))
	    vcrtmode(TRUE);
	break;

#ifdef RECMDS
    case 'T':		/* do t, and add it to group's initial commands */
#ifndef NEWSFILTER
	msg("The T command is disabled, sorry");
	break;
#endif /* NEWSFILTER */
	/* fall through */

    case 't':		/* mark all articles with current subject read */
	demand(HOLD);
	args->c_count = (long) recmd(header.h_subject, L_SUBJECT, TRUE, "j");
	msg1("%ld articles marked", args->c_count);
#ifdef NEWSFILTER
	if (args->c_char == 'T')
	{
	    (void) sprintf(bfr, "/%s/t", header.h_subject);
	    (void) fltprogram(bfr);
	    msg0(fltinfo);
	}
#endif /* NEWSFILTER */
	break;
#endif /* RECMDS */

    case 'u':		/* the unsubscribe command */
	if ((clet2 = vgetc()) == 'd')
	{
	    demand(HOLD);
	    dontfollow(header.h_ident);
	    msg1("Unsubscribing from %s followups", header.h_ident);
	    break;
	}
	else if (clet2 == EOF)
	{
	    demand(NOMORE);
	    break;
	}
	else if (clet2 != 'g')
	{
	    demand(HOLD);
	    msg0("To unsubscribe, use 'ug' or 'U'");
	    break;
	}
	/* fall through if user entered ug */

    case 'U':		/* Unsubscribe to the newsgroup and go on to next */
	demand(HOLD);
	if ((mptr = vgetline("Newsgroup? ")) == (char *)NULL)
	    break;
	else if (mptr[0])
	{
	    group_t	*ngp;

	    if (ngp = ngfind(mptr))
		ngselect(ngp);
	    else
	    {
		msg1("%s: no such group", mptr);
		break;
	    }
	}

	if (unsubscribe())
	    msg1("Unsubscribing to newsgroup: %s", ngname());
	else
	    msg1("Sorry, you can't unsubscribe any of %s", ADMSUB);

	/* fall through to mark remainder of articles in the current group */

    case 'K':		/* mark the rest of the articles in this group read */
	while (args->c_count--)
	{
	    group_t	*oldgrp = ngactive();

	    session.action = M_NEXT;
	    do {
		(void) msgmark(READ, LOCAL);
	    } while
		(nextmsg(session.reread, session.reverse) != FAIL
		 && ngactive() == oldgrp);
	}
	session.action = M_SEEK;
	break;

    case ALT('K'):	/* pass command to news filter */
	demand(HOLD);
	if ((mptr = vgetline("Filter command? ")) == (char *)NULL)
	    break;
	else if (mptr[0])
	{
#ifdef NEWSFILTER
	    (void) fltprogram(mptr);
	    msg0(fltinfo);
#else
	    msg0("The newsfilter facility is disabled.");
#endif /* NEWSFILTER */
	}
	break;

    case 'v':		/* Print the current version of news */
	demand(HOLD);
	msg1("News version: %s", NEWS_VERSION);
	break;

    case 'W':		/* write out your .newsrc file */
	demand(HOLD);
	msg0("Writing your .newsrc file...");
	wrnewsrc();
	break;

    case 'x':		/* exit - no time update. */
	demand(EXIT | HOLD);
	break;

    case '!':		/* escape to shell */
	demand(HOLD);
	if (mptr = vgetline("Command? "))
	{
#ifndef NONLOCAL
	    bool tospool;
	    char startdir[BUFLEN];

	    if (tospool = (mptr[0] == '!'))
	    {
		(void) getcwd(startdir, BUFLEN);
		(void) chdir(site.textdir);
		mptr++;
	    }
#endif /* NONLOCAL */
	    vttymode();
	    vcrtmode(!shell_command(args->c_count == 0L, mptr));
#ifndef NONLOCAL
	    if (tospool)
		(void) chdir(startdir);
#endif /* NONLOCAL */
	}
	break;

    case '-':		/* back up into the message trail */
	while (args->c_count-- > 0L)
	    if (msglast() == FAIL)
	    {
		msg0("Can't back up further");
		demand(HOLD);
		break;
	    }
	session.action = M_HOLD;	/* don't perform after-command seek */
	break;

    case '+':		/* skip forwards */
	session.action = M_SKIP;
	while (args->c_count--)
	    if (msgnext() == FAIL)
	    {
		msg0("Can't skip further");
		demand(HOLD);
		break;
	    }
	session.action = M_HOLD;
	break;

    case '^':		/* go to top node of current discussion */
	args->c_count = -1L;
    case 'p':		/* display parent article */
	session.action = M_SEEK;
	while (args->c_count && msgparent() == SUCCEED)
	    args->c_count--;
	break;

    case '<':		/* specific message ID. */
	if (mptr = vgetline("Article-Id? <"))
	{
	    bfr[0] = '<'; bfr[1] = '\0'; (void) strcat(bfr, mptr);
	    if (mptr = gotoid(bfr, (int)args->c_count))
	    {
		msg0(mptr);
		demand(HOLD);
	    }
	    else
		session.action = M_SEEK;    /* we succeeded */
	}
	break;

    case '#':		/* show our location */
	demand(HOLD);
	msg4("Article %s/%ld, %ld more waiting (%ld in this group)",
	    ngname(), (long)msgnum(), (long)session.waiting, (long)ngunread());
	break;

    case '&':		/* interpret switches */
	demand(HOLD);
	if (mptr = vgetline("Switches? "))
	{
	    int	count;

	    for (count = strlen(mptr); count >= 0; count--)
		mptr[count + strlen(Progname) + 1] = mptr[count];
	    (void) strcpy(mptr, Progname);
	    mptr[strlen(Progname)] = ' ';
	    if (lprocargs(mptr) == FAIL)
		msg0("switches are invalid");
	}
	break;

#ifdef RECMDS
    case '\\':
	session.reverse = TRUE;
    case '/':
	{
	    char    cmdbuf[CMDTEXT], *cmds, *opts;

	    demand(HOLD);
	    (void) sprintf(cmdbuf, "Pattern command? %c", cc);
	    if (mptr = vgetline(cmdbuf))
	    {
		/* maybe this is actually a kill command? */
		if (cmdbuf[strlen(cmdbuf) - 1] == 'K')
		{
#ifdef NEWSFILTER
		    (void) fltprogram(cmdbuf);
#else
		    msg0("Sorry, news filter commands are disabled!");
#endif /* NEWSFILTER */
		    cmdbuf[strlen(cmdbuf) - 1] = '\0';
		    break;
		}

		/* we're going to mess with the commands, make local copy */
		(void) strncpy(cmdbuf, mptr, CMDTEXT);
		cmdbuf[CMDTEXT - 1] = '\0';
		mptr = cmdbuf;

		/* handle the fast simple case first */
		if ((opts = strchr(cmdbuf, cc)) == (char *)NULL)
		{
		    if (recmd(mptr, L_SUBJECT, FALSE, (char *)NULL))
			session.action = M_SEEK;
		}
		else    /* user specified options and maybe commands */
		{
		    int	level;

		    *opts++ = '\0';
		    if (cmds = strchr(opts, ':'))
			*cmds++ = '\0';

		    svreread = session.reread;
#ifdef DELAYMARK
		    session.reread = session.reread || strchr(opts, 'r') || strchr(opts, 'E');
#else
		    session.reread = session.reread || strchr(opts, 'r');
#endif /* DELAYMARK */

		    /* figure out what we're supposed to match against */
		    level = L_SUBJECT;
		    if (strchr(opts, 'h'))
			level = L_HEADER;
		    else if (strchr(opts, 'a'))
			level = L_TEXT;

		    /* these kluges implement option-like kill commands */
		    if (cmdbuf[0] || opts[0])
		    {
			(void) strcpy(bfr, cmdbuf);
			cmdbuf[0] = '\0';
			if (strchr(opts, 'j'))
			    (void) strcpy(cmdbuf, "j:");
#ifdef DELAYMARK
			if (strchr(opts, 'm'))
			    (void) strcpy(cmdbuf, "m:");
#endif /* DELAYMARK */
			(void) strcat(cmdbuf, bfr);

			if (recmd(mptr, level, strchr(opts, 'c')!=NULL, cmds))
			    session.action = M_SEEK;

			session.reverse = FALSE;
			session.reread = svreread;
		    }
		}
	    }
	}
	break;
#endif /* RECMDS */

#ifdef MACROS
    case '%':
	demand(HOLD);
	if (mptr = vgetline("New macro? "))
	{
	    char *subtext;

	    (void) strtok(mptr, " \t");
	    if (subtext = strtok((char *)NULL, ""))
		vmacdef(mptr, subtext);
	}
	break;
#endif /* MACROS */

#ifdef FEEDBACK
    case ALT('P'):
	demand(HOLD);
	msgrate(F_PRAISE);	
#ifdef DEBUG
	if (verbose >= V_SHOWRATE)
	    msg1("Your praise of %s is registered", header.h_ident);
#endif /* DEBUG */
	break;

    case ALT('C'):
	demand(HOLD);
	msgrate(F_CONDEMN);
#ifdef DEBUG
	if (verbose >= V_SHOWRATE)
	    msg1("Your condemnation of %s is registered", header.h_ident);
#endif /* DEBUG */
	break;
#endif /* FEEDBACK */

    default:
	demand(CMDERR);
	break;
    }
}

void gcmd(args)
/* interpret a command with a prefix */
cmdarg_t *args;
{
    place_t	loc;
    int		upto;

    switch (CHR(args->c_argform))
    {
    case C_NONE:
    case C_COUNT:
	onecmd(args);
	break;

    case C_RANGE:
	if (args->c_count < (long)ngmin())
	{
	    msg2("%s/%ld no longer on line.", ngname(), (long)msgnum());
	    demand(HOLD);
	    break;
	}
	else if (args->c_count > (long)ngmax())
	{
	    msg2("%s/%ld doesn't exist yet.", ngname(), (long)msgnum());
	    demand(HOLD);
	    break;
	}
	else if (gotomsg((nart_t)args->c_count) == FAIL)
	{
	    msg2("%s/%ld doesn't exist.", ngname(), (long)msgnum());
	    demand(HOLD);
	    break;
	}
	args->c_count = 1;
	/* FALL THROUGH */

    case C_UPTO:
	upto = args->c_upto;
	args->c_argform = C_NONE;
	do {	    
	    (void) tellmsg(&loc);
	    args->c_count = 1;
	    onecmd(args);
	    if (AT(loc))
		(void) nextmsg(session.reread, session.reverse);
	} while
	    (msgnum() <= upto && ngactive() == loc.m_group);
	break;
    }
}

/* gcmd.c ends here */
