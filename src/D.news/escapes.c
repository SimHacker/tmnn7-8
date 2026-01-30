/****************************************************************************

NAME
   escapes.c -- %-interpolation functions for strings

SYNOPSIS
   #include "news.h"

   void escapes(src, dst)		-- expand C-like \ escapes
   char *src, *dst;

   void setexphook(hook)		-- set expansion hook
   bool (*hook)();

   void seeheader(hp)			-- set current header for %-escapes
   hdr_t *hp;

   void strexpand(src, dst)		-- expand \ and % escapes 
   char *src, *dst;

   void fnexpand(src, dst)		-- expand a filename
   char *src, *dst;

DESCRIPTION
    This module implements functions to expand escape constructs and (if 
RNESCAPES is on) rn-style %-escapes. C-style escapes understood include \n, \b,
\t, \r, and \nnn (octal). The ^-prefix for control characters is also
understood. A leading ~/ is expanded to the user's home directory; a leading
~user is expanded to the home directory of the given user.

    Rn-like escapes presently supported are %a, %A, %b, %B, %c, %C, %d, %D, %f,
%F, %H, %i, %l, %L, %n, %N, %o, %p, %P, %r, %R, %s, %S, %t, %T, %x, %X, %z, %~,
%., %$, %%, %{, %(, %[, and %`. The behavior of %F differs slightly from rn's
%F.
    An %e escape is provided to try for an article author's name.
    One additional escape, %V, yields the contents of the Progname variable.
    The %#, %b, %B, %I, %k[0-9], %m, %M, %u, %U, %/, and %\[0-9] escapes are
supported separately by tool-specific modules.
    Characters EhOqQwWyYZ!@$&*-=+|<>:;"'? are still available for escape
definition, but hO" collide with rn definitions (see BUGS).

ENVIRONMENT
   DOTDIR is queried for the %. escape.
   NEWSDIR is queried for the %p escape.

BUGS
   The %" escape should be supported here but is not yet implemented. The %h
and %O escapes will not be supported.

AUTHOR
   Eric S. Raymond
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

****************************************************************************/
/* LINTLIBRARY */
#include "news.h"
#include "vio.h"
#include "header.h"
#include "active.h"
#include "newsrc.h"
#ifdef RECMDS
#include "regexp.h"
#endif /* RECMDS */

static hdr_t *machdr = &header;

void escapes(cp, tp)
/* process standard C-style escape sequences in a string */
char	*cp, *tp;
{
    while (*cp)
    {
	int	cval = 0;

	if (*cp == '\\' && strchr("0123456789xX", cp[1]))
	{
	    char *dp, *hex = "00112233445566778899aAbBcCdDeEfF";
	    int dcount = 0;

	    if (*++cp == 'x' || *cp == 'X')
		for (++cp; (dp = strchr(hex, *cp)) && (dcount++ < 2); cp++)
		    cval = (cval * 16) + (dp - hex) / 2;
	    else if (*cp == '0')
		while (strchr("01234567",*cp) != (char*)NULL && (dcount++ < 3))
		    cval = (cval * 8) + (*cp++ - '0');
	    else
		while ((strchr("0123456789",*cp)!=(char*)NULL)&&(dcount++ < 3))
		    cval = (cval * 10) + (*cp++ - '0');
	}
	else if (*cp == '\\')		/* C-style character escapes */
	{
	    switch (*++cp)
	    {
	    case '\\': cval = '\\'; break;
	    case 'n': cval = '\n'; break;
	    case 't': cval = '\t'; break;
	    case 'b': cval = '\b'; break;
	    case 'r': cval = '\r'; break;
	    default: cval = *cp;
	    }
	    cp++;
	}
#ifdef ASCII
	else if (*cp == '^')		/* expand control-character syntax */
	{
	    cval = CTRL(*++cp);
	    cp++;
	}
#endif /* ASCII */
	else
	    cval = *cp++;
	*tp++ = cval;
    }
    *tp = '\0';
}

#ifdef RNESCAPES
private bool (*exphook)() = NULLPRED;

void seeheader(hp)
/* set current-header address for macro-expansion purposes */
hdr_t	*hp;
{
    machdr = hp;
}

void setexphook(ehook)
/* set the current macroexpansion hook */
bool (*ehook)();
{
    exphook = ehook;
}

private void strpget(cgetf, cp, term)
/* snarf everything up to an un-escaped term character into a buffer */
int (*cgetf)();	/* function to grab characters with */
char *cp;	/* where to put the characters */
char term;	/* terminating character */
{
    bool literal = FALSE;
    int c;

    while ((c = (*cgetf)()) != EOF)
	if (c != (int)term || literal)
	    literal = ((*cp++ = c) == '\\');
	else
	    break;
    *cp = '\0';
}

private int lexpand(cgetf, cungetf, buf)
/* expand the given %-construct into the given buffer */
int (*cgetf)();		/* function to grab characters with */
int (*cungetf)();	/* function to push characters back with */
char *buf;		/* buffer to expand esacapes into */
{
    bool	qprefix = '\0';
    char	lprefix[SBUFLEN];
#ifdef RECMDS
    struct regexp *re;
#endif /* RECMDS */
    char *cp, c = (*cgetf)();

    /* snarf up a prefix qualifier, if any */
    if (c == '^' || c == '_')
    {
	qprefix = c;
	c = (*cgetf)();
    }

    /* snarf up a length-limit prefix, if there is one */
    lprefix[1] = '\0';
    if (isdigit(c) || ((c=='.' || c=='-') && isdigit((*cungetf)((*cgetf)()))))
    {
	for (cp = lprefix + 1; isdigit(c) || c=='.' || c=='-'; c = (*cgetf)())
	    *cp++ = c;
	*cp = '\0';
    }

    buf[0] = '\0';

    switch(c)
    {
    case 'a':	(void) sprintf(buf, "%d", msgnum()); break;
    case 'A':	(void) artname(&art, buf); break;
#ifndef NONLOCAL
    case 'c':	(void) strcpy(buf, artdir(ngname())+strlen(site.textdir)+1); break;
    case 'd':	(void) strcpy(buf, artdir(ngname())); break;
#endif /* NONLOCAL */
    case 'C':	(void) strcpy(buf, ngname()); break;
    case 'D':	(void) strcpy(buf, machdr->h_distribution); break;
    case 'e':	author(machdr, buf); break;
    case 'f':
	(void) strcpy(buf,
		      hlnblank(machdr->h_replyto)
		      ? machdr->h_replyto : machdr->h_from);
	break;
    case 'F':
	(void) strcpy(buf,
		      hlnblank(machdr->h_followto)
		      ? machdr->h_followto : machdr->h_newsgroups);
	break;
    case 'H':	(void) strcpy(buf, site.pathname); break;
    case 'i':	(void) strcpy(buf, machdr->h_ident); break;
    case 'l':	(void) strcpy(buf, site.notify); break;
    case 'L':	(void) strcpy(buf, username); break;
    case 'n':	(void) strcpy(buf, machdr->h_newsgroups); break;
    case 'N':	(void) strcpy(buf, fullname(username)); break;
    case 'o':	(void) strcpy(buf, organization()); break;
    case 'p':	(void) strcpy(buf, (cp = getenv("NEWSDIR"))?cp:DEFDIR); break;
#ifndef NONLOCAL
    case 'P':	(void) strcpy(buf, site.textdir); break;
#endif /* NONLOCAL */
    case 'R':	(void) sprintf(buf, "%s %s",
			       machdr->h_references, machdr->h_ident);
	break;
    case 'r':
	if (cp = strrchr(machdr->h_references, '<'))
	    (void) strcpy(buf, cp);
	break;
    case 's':
	for (cp = machdr->h_subject; !strncmp(cp, "Re: ", 4); cp += 4)
	    continue;
	(void) strcpy(buf, cp);
	if (strcmp(buf + strlen(buf) - 7, " - (nf)") == 0)
	    buf[strlen(buf) - 7] = '\0';
	break;
    case 'S':
	cp = machdr->h_subject;
	(void) strcpy(buf, cp + (strcmp(cp, "Re: ") ? 0 : 4));
	break;
    case 'T':
	(void) sprintf(buf, "To: %s\n", machdr->h_path);
	if (cp = strchr(buf, '('))
	    cp[0] = '\0';
	(void) strcat(buf, "\n");
	break;
    case 't':
	(void) sprintf(buf, "To: %s",
			hlnblank(machdr->h_replyto)
		       ? machdr->h_replyto : machdr->h_from);
	if (cp = strchr(buf, '('))
	    cp[0] = '\0';
	(void) strcat(buf, "\n");
	break;
    case 'V':	(void) strcpy(buf, Progname); break;
    case 'x':	(void) strcpy(buf, site.admdir); break;
    case 'X':	(void) strcpy(buf, site.libdir); break;
    case 'z':
	{
	    (void) artname(&art, bfr);
	    (void) sprintf(buf, "%ld", filesize(bfr));
	}
	break;
	    
    case '~':	(void) strcpy(buf, userhome); break;
    case '.':	(void) strcpy(buf,
			      getenv("DOTDIR") ? getenv("DOTDIR") : userhome);
	break;
    case '$':	(void) sprintf(buf, "%d", getpid()); break;
    case '%':	(void) strcpy(buf, "%"); break;
    case '[':
	{
	    char *hd, expbuf[BUFLEN];

	    strpget(cgetf, expbuf, ']');
	    if ((hd = hlget(expbuf)) != (char *)NULL)
		(void) strcpy(buf, hd);
	}
	break;
    case '(':
	{
	    char *ifpart, *thenpart, *elsepart, expbuf[BUFLEN];

	    strpget(cgetf, bfr, ')');	/* stash away the construct */
	    strexpand(bfr, expbuf);	/* string-expand it */

	    /* now parse out the parts */
	    cp = expbuf;
	    while (*cp && (*cp != '=' || cp == expbuf || cp[-1] == '\\'))
		cp++;
	    if (*cp)
		*cp++ = '\0';
	    ifpart = cp;
	    while (*cp && (*cp != '?' || cp == expbuf || cp[-1] == '\\'))
		cp++;
	    if (*cp)
		*cp++ = '\0';
	    thenpart = cp;
	    while (*cp && (*cp != ':' || cp == expbuf || cp[-1] == '\\'))
		cp++;
	    if (*cp)
		*cp++ = '\0';
	    elsepart = cp;

	    /* and evaluate it */
#ifdef RECMDS
	    re = regcomp(expbuf);
	    if (regexec(re, ifpart))
#else
	    if (strcmp(expbuf, ifpart) == SUCCEED)
#endif /* RECMDS */
		(void) strcpy(buf, thenpart);
	    else
		(void) strcpy(buf, elsepart);
	}
	break;
    case '{':
	{
	    char *defpart, expbuf[BUFLEN];

	    /* parse out the parts */
	    strpget(cgetf, expbuf, '}');	/* stash away the construct */
	    cp = expbuf;
	    while (*cp && (*cp != '-' || cp == expbuf || cp[-1] == '\\'))
		cp++;
	    if (*cp)
		*cp++ = '\0';
	    defpart = cp;

	    /* and evaluate it */
	    if (cp = getenv(expbuf))
		(void) strcpy(buf, cp);
	    else if (defpart[0])
		(void) strcpy(buf, cp);
	}
	break;
    case '`':
	{
	    char expbuf[BUFLEN];

	    /* parse out the parts */
	    strpget(cgetf, expbuf, '`');	/* stash away the construct */
	    (void) backquote(expbuf, buf, BUFLEN);
	}
	break;
    default:	/* no go, try expanding tool-specific escapes */
	(*cungetf)(c);
	if (exphook != NULLPRED && !(*exphook)(cgetf, cungetf, buf))
	    return(0);
    }

    /* finally, apply the modifiers */
    if (qprefix)
	if (qprefix == '^')
	    buf[0] = toupper(buf[0]);
	else if (qprefix == '_' && (cp = strchr(buf, '.')) != (char *)NULL)
	    cp[1] = toupper(cp[1]);

    /* interpret the length prefix, if there is one */
    if (lprefix[1])
    {
	char expbuf[BUFLEN];

	lprefix[0] = '%';
	(void) strcat(lprefix, "s");
	(void) sprintf(expbuf, lprefix, buf);
	(void) strcpy(buf, expbuf);
    }
    return(strlen(buf));
}

private char *scp;

private int sgetc()
{
    int	cc;

    if ((cc = *scp++) == '\0')
	return(EOF);
    else
	return(cc);
}

private int sungetc(c)
char c;
{
    return(*--scp = c);
}
#endif /* RNESCAPES */

void strexpand(src, dst)
/* escape-expand a string */
char	*src, *dst;
{
#ifndef RNESCAPES
    (void) strcpy(dst, src);
#else
    char		cc, *scpsave = scp;	/* make function re-entrant */
    register char	*to = dst;

    for (scp = src; *scp; )
	if ((cc = *scp++) == RNESCCHR)
	    to += lexpand(sgetc, sungetc, to);
	else
	    *to++ = cc;
    *to = '\0';
    scp = scpsave;
#endif /* RNESCAPES */

    /* *after* %-expansion so as not to clobber \%s */
    escapes(dst, dst);
}

void fnexpand(src, dst)
/* escape-expand a filename */
char	*src, *dst;
{
    /* expand leading ~/ to user's home directory */
    if (src[0] == '~')
    {
	if (src[1] == '/')
	{
	    (void) strcpy(dst, userhome);
	    (void) strcat(dst, "/");
	    src += 2;
	}
        else
	{
	    struct passwd *pw = getpwnam(src + 1);

	    if (pw != (struct passwd *)NULL)
	    {
		(void) strcpy(dst, pw->pw_dir);
		src += strlen(src);
	    }
	}
	dst += strlen(dst);
    }
    else if (src[0] == '$')	/* expand initial environment variable */
    {
	char *ep, *sp = strchr(src, '/');

	if (sp)
	    *sp = '\0';
	if (ep = getenv(src))
	    (void) strcpy(dst, ep);
	if (sp)
	    *sp = '/';
	src += strlen(src);
	dst += strlen(dst);
    }

    strexpand(src, dst);
}

/* escapes.c ends here */
