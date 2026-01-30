/****************************************************************************

NAME
   rfuncs.c -- command common code for interactive news readers

SYNOPSIS
   #include "rfuncs.h"

   bool rexpand(cgetf, cungetf, buf)	-- hook for generic reader %-escapes
   int (cgetf)(), (*cungetf)(); char *buf;

   int procdlines(ltype, line)		-- process .newsrc directives
   int ltype; char *line;

   bool shell_command(conf, line)	-- execute a shell command
   bool conf; char *line;

   char *edited_reply(inc, subf)	-- reply, using editor to compose
   bool inc; char *subf;

   char *direct_reply(subf)		-- reply, don't edit
   char *subf;

   char *stuffit(to, flags)	-- send message to program or file
   char *to; int flags;

   int followup(includep)	-- post follow up to current article
   bool includep;

   int change_group(gname, count, reread, reverse)	-- change current group
   char	*gname; int count; bool reread, reverse;

   char *gotoid(id, count)		-- seek to a message by Internet ID
   char *id; int count;

   char *cancel_command(h, nauth)	-- cancel given message
   hdr_t *h; bool nauth;

   int recmd(mptr, level, scase, cmds)	-- do RE command
   char *mptr; int level; bool scase; char *cmds;

   void vexec(buf, delim)		-- execute commands from a buffer
   char *buf; char *delim;

DESCRIPTION
   This module includes service routines for use by interactive readers.
Most of this code was dissected out of the old visual.c by Kenneth Almquist,
but the screen-handling dependencies have been removed.

ENVIRONMENT VARIABLES
   The stuffit() code uses three, NEWSCMD, NEWSDIR and NEWSBOX. If RNESCAPES is
on, these provide templates into which whatever the user typed is plugged
as %b. Otherwise they simply provide defaults in case the user typed nothing.
See readers(5) or the code for details.
   The edited_reply() function uses MAILER/MAILSERV, and direct_reply() uses
MAILFRONT. Both use SHELL (via apawn.c).

FILES
   /tmp/stuffit??????	-- used for holding program output

REVISED BY
   Eric S. Raymond
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

****************************************************************************/
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
#include "history.h"
#include "libread.h"
#include "spawn.h"
#ifdef RECMDS
#include "regexp.h"
#endif /* RECMDS */
#include "libpost.h"
#include "gcmd.h"
#include "rfuncs.h"
#ifdef COMMUNIST
#include "fascist.h"
#endif /* COMMUNIST */

#define DEFSAVE	"%C.%a"		/* default file part for savebox name */
#define PIPE	'|'		/* save command should pipe to program */

#ifdef RNESCAPES
#ifdef RECMDS
private char lastsave[BUFLEN];	/* last article save file destination */
private regexp *lastre;		/* last regular expression processed */
private char *ulastre;		/* uncompiled form of last r.e. */
#endif /* RECMDS */
#endif /* RNESCAPES */

private char rcbuf[BUFLEN];		/* general-use scratch buffer	*/

bool rexpand(cgetf, cungetf, buf)
/* expand %[bIMuU#/\] into the given buffer */
int (*cgetf)();		/* how to get key character of escape */
int (*cungetf)();	/* how to get key character of escape */
char *buf;		/* buffer to expand esacapes into */
{
    int	c;

    switch((c = (*cgetf)()))
    {
    case 'b':	(void) strcpy(buf, lastsave); break;
    case 'I':	(void) strcpy(buf, inclmark); break;
#ifdef DELAYMARK
    case 'M':	(void) sprintf(buf, "%d", session.ndmarks); break;
#endif /* DELAYMARK */
    case 'u':	(void) sprintf(buf, "%d", ngunread()); break;
    case 'U':	(void) sprintf(buf, "%d",
			       ngunread()
				? ngunread() - getbit(msgnum(), ngactive())
				: 0);
	break;
    case '#':	(void) sprintf(buf, "%ld", (long)session.waiting);
#ifdef RECMDS
    case '/':	if (ulastre) (void) strcpy(buf, ulastre); break;
#endif /* RECMDS */
#ifdef RECMDS
    case '\\':
	if (!isdigit(c = (*cgetf)()))
	{
	    (*cungetf)(c);
	    return(FALSE);
	}
	else
	{
	    char *cp;
	    int	i = 0, j;

	    if ((j = ctoi(c)) == 0)
		for (j = 0; lastre->startp[j] && j < NSUBEXP; j++)
		    continue;
	    for (cp = lastre->startp[j]; cp < lastre->endp[j]; )
		buf[i++] = *cp++;
	    *cp = '\0';
	}
	break;
#endif /* RECMDS */
    default:
	(*cungetf)(c);
	return(FALSE);
    }
    return(TRUE);
}

int procdlines(dtype, line)
/* process directive lines for readers */
int	dtype;	/* directive type */
char	*line;	/* text of directive */
{
    if (dtype == N_OPTIONS)
	return(lprocargs(line));
    else if (dtype == N_MACRO)
    {
	char *trigger = strtok(line, " \t");
	char *text = strtok((char *)NULL, " \t");

	if (text != (char *)NULL && trigger != (char *)NULL)
	    vmacdef(trigger, text);
    }

    return(SUCCEED);
}

bool shell_command(confirm, line)
/*
 * Execute a shell command, recognizing the following special chars:
 *    & at EOL -- run in background
 *    | at EOL -- mail command output to calling user
 * The confirm argument controls whether the code types 'continue? '
 * and waits for a newline after the command execution.
 * Note that this alters the storage pointed at by its argument.
 * TRUE is returned if the command ran in foreground, FALSE otherwise
 */
bool	confirm;
char	*line;
{
    register char *p;
    int flags = 0;

    if (confirm)
	flags = CWAIT;
    if (*(p = line) == '\0')
    {
	char *args[2];

	args[0] = SHELL; args[1] = (char *)NULL;
	(void) spawn(args, FORGRND | NOPRIVS, msgfile());
	return(flags & BAKGRND);
    }
    while (*p)
	p++;
    while (p > line && p[-1] == ' ')
	p--;

    if (*--p == '&')
    {
	*p = '\0';
	flags = BAKGRND;
    }
    else if (*p == '|')
    {
	*p = '\0';
	(void) sprintf(bfr, "(%s) | %s '%s'", line, MAILSERV, username);
	line = bfr;
	flags |= BAKGRND;
    }

    /* here goes the actual command execution */
#ifndef RNESCAPES
    (void) shcmd(line, flags, "");
#else
    fnexpand(line, rcbuf);
    (void) shcmd(rcbuf, flags, "");
#endif /* RNESCAPES */

    return(flags & BAKGRND);
}

char *edited_reply(incl, fsubject)
/* reply to a message, invoking the editor for composition */
bool incl;		/* tells whether or not to include the message text */
char *fsubject;		/* optional subject argument */
{
    static hdr_t reply;	/* must be initially zeroed, or hfree() will bomb */
    char	*freply, *cite;
    int		c;
    FILE	*ofp;

    if (msgtext() == FAIL)
	return("Can't get at article text!");

    hfree(&reply);
    hlcpy(reply.h_to, mailreply(&header));
    if (site.smarthost[0])
	hlcpy(reply.h_path, header.h_path);
    newsubject(&header, &reply, fsubject);
    (void) sprintf(bfr, "your article %s", header.h_ident);
    hlcpy(reply.h_references, bfr);

#ifdef RNESCAPES
    if ((cite = getenv("YOUSAID")) == (char *)NULL)
    {
	cite = "In article %i you write:\n";
	strexpand(cite, bfr);
	cite = savestr(bfr);
    }
#else
    cite = "You wrote:\n"
#endif /* RNESCAPES */

    freply = prepmsg(&reply, incl ? msgfile() : (char*)NULL, cite, inclmark);
#ifdef RNESCAPES
    (void) free(cite);
#endif /* RNESCAPES */
    if (editmsg(freply) == FAIL)
	return("Couldn't find your editor!");
    else
    {
	signmsg(freply);
	hfree(&reply);
	(void) hread(&reply, 0L, fopen(freply, "r"));

	if ((ofp = mailopen(reply.h_to, reply.h_subject)) == (FILE *)NULL)
	{
	    (void) fclose(header.h_fp);
	    return("Couldn't open the mailer.");
	}

	hlfree(reply.h_to);
	hlfree(reply.h_subject);
	hwrite(&reply, ofp, TRUE);

	while ((c = fgetc(reply.h_fp)) != EOF)
	    (void) fputc(c, ofp);
	(void) fclose(reply.h_fp);
	(void) mailclose(ofp);

	return("Mail is being sent");
    }
}

char *direct_reply(bptr)
/* accept a reply to a message, don't invoke the editor for composition */
char	*bptr;
{
    register char *p, *q;
    char *mailer;
    char address[PATHLEN];

    if (msgtext() == FAIL)
	return("Can't get at article text!");

    p = mailreply(&header);
    q = address;
    while (*p != '\0') {
	if (strchr("\"\\$", *p) != 0)
	    *q++ = '\\';
	*q++ = *p++;
    }
    *q++ = '\0';
    if ((mailer = getenv("MAILER")) == (char *)NULL)
	mailer = MAILFRONT;
    (void) sprintf(rcbuf, mailer, header.h_subject);
    if (bptr)
    {
	while (isspace(*bptr))
	    bptr++;
	if (*bptr != '\0')
	    (void) strcpy(rcbuf, bptr);
    }
#ifdef DEBUG
    if (debug)
	/* this is shaky, but it works for the most common front ends */
	(void) sprintf(bfr, "%s -d %s", rcbuf, address);
    else
#endif /* DEBUG */
	(void) sprintf(bfr, "%s %s", rcbuf, address);
 
    if (shcmd(bfr, FORGRND | NOPRIVS, msgfile()) != 0)
    {
	(void) sprintf(bfr, "Couldn't run mailer\n");
	return(bfr);
    }
    else
	return((char *)NULL);
}

char *stuffit(to, flags)
/*
 * Send the current message to a file or program. The program case is
 * distinguished by a leading PIPECHAR on the command. This function returns
 * a pointer to a message describing its success or failure.
 */
char	*to;	/* file or program to send to */
int	flags;	/* append vs. overwrite, save header vs. article-only */
{
    bool    isprogram = FALSE;

    /* skip leading whitespace and an optional pipe character */
    while (isspace(*to))
	to++;
    if (isprogram = (to[0] == PIPE))
	to++;
    while (isspace(*to))
	to++;

    if (msgtext() == FAIL)
	return("Can't get at article text!");

    /* generate a destination */
    if (to[0])
	(void) strcpy(lastsave, to);
    else
	lastsave[0] = '\0';

    if (isprogram)
    {
	if ((to = getenv("NEWSCMD")) == (char *)NULL)
#ifdef RNESCAPES
	    to = "%b";
#else
	    to = lastsave;
#endif /* RNESCAPES */
	fnexpand(to, rcbuf);
    }
    else
    {
	char *defsave = getenv("NEWSBOX");
	char *defdir = getenv("NEWSDIR");
	bool endslash;

	if (defdir == (char *)NULL)
	    defdir = DEFDIR;

	/* prepend ${NEWSDIR} if destination is a file only */
	if (lastsave[0] == '~' || strchr(lastsave, '/') || isdir(lastsave))
	    (void) strcpy(bfr, lastsave);
	else if (lastsave[0])
	{
	    (void) strcpy(bfr, defdir);
	    if (rcbuf[strlen(rcbuf) - 1] != '/')
		(void) strcat(rcbuf, "/");
	    (void) strcat(bfr, lastsave);
	}
	else
	    (void) strcpy(bfr, defdir);
	fnexpand(bfr, rcbuf);

	/* append ${NEWSBOX} if destination is a directory */
	if ((endslash = (rcbuf[strlen(rcbuf) - 1] == '/')) || isdir(rcbuf))
	{
	    if (!endslash)
		(void) strcat(rcbuf, "/");

	    if (defsave == (char *)NULL)
		defsave = DEFSAVE;

	    fnexpand(defsave, rcbuf + strlen(rcbuf));
	}

	/* make sure all directories in the path are created */
	if (to = strrchr(rcbuf, '/'))
	{
	    *to = '\0';
	    if (!isdir(rcbuf))
		(void) mkbranch(rcbuf, 0777);
	    *to = '/';
	}
    }

    if (rcbuf[0] == '\0')
	return("No destination");

    if (isprogram)    /* check the command and generate a temp file name */
    {
	char	temp[BUFLEN];

	(void) strcpy(temp, "/tmp/stuffitXXXXXX");
	(void) mktemp(temp);
	if (saveto(temp, flags | S_OVWRITE) == FAIL)
	    return("Save to tempfile failed");

	/* execute the command on the generated text */
	(void) sprintf(bfr, "(%s) <%s", rcbuf, temp);
	(void) shcmd(bfr, CWAIT, msgfile());
	(void) unlink(temp);
	bfr[0] = '\0';
    }
    else
    {
	int	isold;

#ifdef DEBUG
	if (!debug)
#endif /* DEBUG */
	    /* do the actual save */
	    if ((isold = saveto(rcbuf, flags)) == FAIL)
	    {
		(void) sprintf(bfr,
		    "Save to %s failed, couldn't open article or destination",
		     rcbuf);
		return(rcbuf);
	    }

	/* clean up and report */
	if ((flags & S_OVWRITE) == 0)
	    (void) sprintf(bfr,"file: %s %sed", rcbuf, isold?"append":"creat");
	else
	    (void) sprintf(bfr, "file: %s written", rcbuf);
    }

    return(bfr);
}

void followup(includep)
/* accept a followup to the given message */
bool	includep;	/* whether or not to include the original */
{
    char	cmdbuf[BUFLEN];
#ifdef NGUPDATE
    char	*id;
#define FOPTFORM	" -F %s"	/* appropriate for bare fork/exec */
#else
#define FOPTFORM	" -F '%s'"	/* appropriate for shell spawn */
#endif /* NGUPDATE */

    /* construct the followup command */
    (void) sprintf(cmdbuf, "postnews %s/%ld", ngname(), (long)msgnum());
    if (includep)
	(void) sprintf(cmdbuf + strlen(cmdbuf), FOPTFORM, inclmark);
#undef FOPTFORM

#ifdef DEBUG
    if (debug)
    {
	char *cp;

	for (cp = cmdbuf + strlen(cmdbuf) - 1; cp >= cmdbuf; cp--)
	    cp[5] = cp[0];
	(void) memcpy(cmdbuf, "echo ", 5);
    }
#endif /* DEBUG */

#ifndef NGUPDATE
    (void) shcmd(cmdbuf, FORGRND, msgfile());
#else
    if ((id = ospawn(cmdbuf)) != (char *)NULL && id[0])
	msgadd(id);
#endif /* NGUPDATE */
}

int change_group(gname, count, reread, reverse)
/*
 * Move between groups on user command. Note that the current-message
 * info is invalid between a change_group() call and the next msgnext().
 */
char	*gname;		/* named group to move to */
int	count;		/* number of groups to move */
bool	reread;		/* read already-seen groups */
bool	reverse;	/* move backwards if TRUE */
{
    int		oldcount = count;
    place_t	place;

    /* note our previous location in case the move fails */
    (void) tellmsg(&place);

    /* first go to the named group (if one was named) */
    if (gname && *gname != '\0')
    {
	if (ngselect(ngfind(gname)) == (group_t *)NULL)
	{
	    msg1("No such group as %s", gname);
	    return(FAIL);
	}
	else if (!reread && ngunread() == 0)
	{
	    msg1("No unread messages in %s", gname);
	    (void) seekmsg(&place);
	    return(FAIL);
	}

#ifdef COMMUNIST
	/* maybe we don't *want* the user to find this group? */
	if (!privileged	&& !ngmatch(gname,fascist(username)->n_read))
	    return(FAIL);
#endif /* COMMUNIST */

	(void) gotogrp((char *)NULL);

#ifndef NONLOCAL
	/* now make sure there's something there to land on! */
	if (access(artdir(gname), R_OK | X_OK))
	{
	    if (access(artdir(gname), F_OK) == FAIL)
		msg1("Newsgroup %s's spool directory is missing!", ngname());
	    else
		msg1("Newsgroup %s is not currently accessible.", ngname());

	    /* make this newsgroup temporarily invisible and go back */
	    ngactive()->ng_unread = 0;
	    (void) seekmsg(&place);
	    return(FAIL);
	}
#endif /* NONLOCAL */
    }
    else if (count < 0)		/* go to some previous newsgroup */
    {
	while (count++)
	    if (nextgroup(reread, !reverse) == FAIL)
	    {
		msg1("No previous group, stopped at %s", ngname());
		(void) tellmsg(&place);
		return(FAIL);
	    }
    }
    else if (count > 0)		/* go to next newsgroup */
    {
	while (count--)
	    if (nextgroup(reread, reverse) == FAIL)
	    {
		msg1("No next group, stopped at %s", ngname());
		(void) tellmsg(&place);
		return(FAIL);
	    }
    }

    /* don't go to start of group unless we've actually moved */
    if ((gname && *gname != '\0') || count < oldcount)
	(void) nextmsg(reread, reverse);
    return(SUCCEED);
}

/*ARGSUSED1*/
char *gotoid(id, count)
/* seek to a message by Message-ID */
char	*id;	/* Message-ID to look for */
int	count;	/* which of its references to go to */
{
    if (id[0] != '<')
	(void) sprintf(bfr, "<%s", id);
    else
	(void) strcpy(bfr, id);
    if (!strchr(bfr, '@'))
	(void) strcat(bfr, "@");

    switch (msggoto(bfr))
    {
    case FAIL:
	(void) sprintf(bfr, "%s: no such article", id);
	return(bfr);

    case CANCELLED:
	(void) sprintf(bfr, "%s has been cancelled", id);
	return(bfr);

    case EXPIRED:
	(void) sprintf(bfr, "%s has expired", id);
	return(bfr);

    default /* SUCCEED */:
	return((char *)NULL);
    }
}

char *cancel_command(h)
/* send a cancellation request after an article */
hdr_t	*h;
{
    int	    notauthor;
    int	    n = strlen(username);

    notauthor = strncmp(username, h->h_path, n) || !isspace(h->h_path[n]);

    if (notauthor && !privileged)
	(void) sprintf(bfr, "Can't cancel what you didn't write.\n");
    else if (docancel(&header, notauthor))
    {
	(void) sprintf(bfr, "Cancel succeeded");
	(void) msgmark(READ, GLOBAL);
    }
    else
	(void) sprintf(bfr, "Cancel failed");
    return(bfr);
}

private int docancel(hp, notauthor)
/*
 * Cancel the article whose header is in hp, by posting a control message
 * to cancel it.  The scope of the control message depends on who would
 * really be willing to cancel it.  It is sent as far as it will do any good.
 * notauthor is true iff the person posting this article is not the
 * real author of the article being cancelled.
 */
hdr_t	*hp;
bool	notauthor;
{
    if (notauthor)
	(void) sprintf(bfr,
		"%s/%s -t 'cmsg cancel %s' -n %s -d local < /dev/null",
		site.libdir, "inews", hp->h_ident, hp->h_newsgroups);
    else
    {
	if (hlblank(hp->h_distribution))
	    (void) sprintf(bfr,
		    "%s/%s -t 'cmsg cancel %s' -n %s < /dev/null",
		    site.libdir, "inews", hp->h_ident, hp->h_newsgroups);
	else
	    (void) sprintf(bfr,
		    "%s/%s -t 'cmsg cancel %s' -n %s -d %s < /dev/null",
		    site.libdir, "inews",
		    hp->h_ident, hp->h_newsgroups, hp->h_distribution);
    }
    return(shcmd(bfr, BAKGRND, ""));
}

#ifdef RECMDS
private bool msgmatch(re, level, usecase)
/* check to see if a given article matches a template */
regexp	*re;		/* regular expression to match */
int	level;		/* what things to match against */
bool	usecase;	/* if TRUE, be case-sensitive */
{
    if (level == L_TEXT || level == L_HEADER)
    {
	FILE	*afp = msgopen(msgfile());

	while (fgets(bfr, LBUFLEN, afp))
	{
	    if (level == L_HEADER && bfr[0] == '\n')
		break;

	    if (!usecase)
		lcase(bfr);

	    if (regexec(re, bfr))
	    {
		(void) fclose(afp);
		return(TRUE);
	    }
	}

	(void) fclose(afp);
	return(FALSE);
    }
    else    /* we only need to look at one header line, don't fetch article */
    {
	if (usecase)
	    return(regexec(re, header.h_subject));
	else
	{
	    char	*cp;

	    (void) strcpy(rcbuf, header.h_subject);
	    for (cp = rcbuf; *cp; cp++)
		if (isalpha(*cp))
		    *cp = tolower(*cp);
	    return(regexec(re, rcbuf));
	}
    }
}

int recmd(mptr, level, scase, cmds)
/* execute regular-expression commands */
char	*mptr;	/* the regular expression to use */
int	level;	/* what scope to search */
bool	scase;	/* if TRUE, be case-sensitive */
char	*cmds;	/* commands to execute */
{
    bool	    mcount = 0;
    int		    place;

    if (mptr && !scase)
    {
	(void) strcpy(rcbuf, mptr);
	lcase(rcbuf);
	mptr = rcbuf;
    }

    if (mptr && mptr[0])
    {
	lastre = regcomp(mptr);	/* turn the pattern into a NDFSA */
#ifdef RECMDS
	if (ulastre)
	    (void) free(ulastre);
	ulastre = savestr(mptr);
#endif /* RECMDS */
    }

    if (lastre == (regexp *)NULL)
    {
	msg0("First regular expression must not be null");
	return(FAIL);
    }

    place = msgtell();	/* we may need to come back here */

    /* iterate over messages that match in this pattern */
    do {
	if (msgmatch(lastre, level, scase))
	    if (cmds)
	    {
		mcount++;
		vexec(cmds, ":");
	    }
	    else
	    {
		session.action = M_SEEK;
		return(1);
	    }
	} while
	    (nextmsg(session.reread,session.reverse)!=FAIL && session.ngrp!=1);

    (void) msgseek(place);
    return(mcount);
}

#endif /* RECMDS */

void vexec(buf, delim)
/* execute commands from a buffer */
char	*buf;	/* buffer to execute from */
char	*delim;	/* delimiter to parser for */
{
    char *cp = strtok(buf, delim);

    buf = savestr(buf);
    do {
	char	*tp;

	for (tp = cp; *tp; tp++)
	    venqueue(*tp++);
	venqueue('\0');
	vcommand(gparse(vgetcmd((char *)NULL)));
	vclearin();
    } while
	(cp = strtok(NULL, "\n"));
    (void) free(buf);
}

/* rfuncs.c ends here */
