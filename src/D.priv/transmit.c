/***************************************************************************

NAME
   transmit.c -- functions used to transmit articles point-to-point

SYNOPSIS
   int transmit(target, sp, id, fname, uopt, fopt)
   char *target, feed_t *sp; char *id; char *fname; bool uopt, fopt;

   int bymail(fname, dest, name)	-- ship an article to name@dest by mail
   char *fname; char *dest; char *name;

   char *filefilter(fname)		-- prefilter transmission file
   char *fname;

   char *artfilter(fname)		-- prefilter transmitted article
   char *fname;

   bool xmitctrl(target, sp, title, ng, body)    -- transmit control message
   char *target; feed_t *sp; char *title, *ng, *body;

DESCRIPTION
   These functions do point-to-point news transmission. They interpret
the A, C, D, E, F, H, M, S, U, V and X transmission options. The
'T' option is not presently used but is reserved for transport layer
specification.

   Some of this code invokes a UUCP multicast facility, selected by the
combination the X transmission options and no explicit transmit command.
This feature is experimental and fragile; contemplate the implementation in
uucast.c, be prepared to tweak it if necessary, and use at your own risk.

   The xmitmsg() function is for transmitting control messages.

FILES
   ADM/xmitlog		-- where message text goes if D is enabled
   /tmp/squashed??????	-- where compressed text gets put
   /tmp/encoded??????	-- where encoded text gets put
   /tmp/aform??????	-- where articles changed to A format are put
   /tmp/xmsg??????	-- hold a control message for transmission
   /tmp/xmsg??????.new	-- used when generating ID for transmission

SEE ALSO
   sevenbit.c -- for encode()
   uucast.c -- multicast transmission code

AUTHOR
   Eric S. Raymond
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

*****************************************************************************/
/*LINTLIBRARY*/
#include "news.h"
#include "spawn.h"
#include "libpriv.h"
#include "header.h"
#include "post.h"
#include "dballoc.h"
#include "feeds.h"
#include "history.h"
#include "procopts.h"
#include "libuucp.h"

#ifdef lint
#undef UUCAST		/* this works around lint breakage on the 6386 */
#endif /* lint */

#define V_FILTERBLAB	2	/* min debug to be chatty about filtering */

#ifdef u370
private hdr_t hh;
#endif /* u370 */

/*ARGSUSED5*/
int transmit(tp, sp, id, fname, uopt, fopt)
/* transmit a file to another system, possibly a list of systems */
char		*tp;	/* target system list */
register feed_t	*sp;	/* data on the common link to the target system */
char		*id;	/* Message-ID of the message or NULL */
char		*fname;	/* name of article to be transmitted */
bool		uopt;	/* enables uval */
bool		fopt;	/* enables batchit */
{
    char	*mailto, syslist[BUFSIZ], cmdbuf[BUFSIZ], *argv[MAXARGS];
    int		argc, foundfile;
    forward void xmit1();
    register char *ptr, *rcvcmd;

/* C:	compress the article before transmission */
    bool compress = (s_option(sp, 'C') != (char *)NULL);
/* D:	debug: send final transmission command and text to files */
    bool debugopt = (s_option(sp, 'D') != (char *)NULL);
/* E:	make sure all characters are 7 bits only */
    bool eencode = (s_option(sp, 'E') != (char *)NULL);
/* F:	batched transmission */
    bool batchit = fopt && (s_option(sp, 'F') != (char *)NULL);
/* M:	use /bin/mail to the target system (a user name must be given) */
    bool mailit = ((mailto = s_option(sp, 'M')) != (char *)NULL);
/* S:	noshell: don't fork a shell to execute the xmit command */
    bool noshell = (s_option(sp, 'S') != (char *)NULL);
/* U:	useexist: use the -c option to uux to use the existing copy */
    char *uval = s_option(sp, 'U');
/* V:	spawn 2.10.X-style remote agents or other archaisms */
    char *version = s_option(sp, 'V');
/* X:	mxcast: the system list we're transmitting to is a multicast group */
    bool mxcast = (s_option(sp, 'X') != (char *)NULL);

    /* compute the proper remote agent */
    rcvcmd = "rnews";
    if (version != (char *)NULL && strncmp(version, "2.10", 4) == 0)
    {
#ifdef ENCODE
	if (batchit && compress && eencode)
	    rcvcmd = "c7unbatch";
	else
#endif /* ENCODE */
	if (batchit && compress)
	    rcvcmd = "cunbatch";
	else if (batchit)
	    rcvcmd = "unbatch";
#ifdef ENCODE
	else if (compress && eencode)
	    rcvcmd = "un7compress";
#endif /* ENCODE */
	else if (compress)
	    rcvcmd = "uncompress";
    }

    /* smash commas in the systems list into spaces */
    for (ptr = syslist; *tp; ptr++, tp++)
	if (*tp == NGDELIM)
	    *ptr = ' ';
	else
	    *ptr = *tp;
    *ptr = '\0';

    /* mail is a special case, it can always multicast */
    if (mailit)
    {
	if (mailto[0])
	    return(bymail(fname, syslist, mailto));
	else
	    return(bymail(fname, syslist, rcvcmd));
    }

    /*
     * If we're sending to multiple systems which are a multicast group,
     * and no explicit command has been given, try to do multicasting via UUCP
     * by direct diddling of its control files.
     *
     * Warning: this is tricky! Before using, look at uucast.c and ensure that
     * the implementation matches your uucp's job file conventions.
     */
    if (mxcast && sp->s_xmit[0] == '\0')
#ifndef UUCAST
	logerr0("can't multicast without explicit transmission command");
#else
    {
	argc = vcrack(syslist, argv, MAXARGS);
	return(uucast(NEWSUSR, fname, rcvcmd, argv, argc));
    }
#endif /* UUCAST */

    /*
     * O.K., this is either a system list xmit or general-case multicasting.
     * In either case, we need a copy of the xmit command (with everything but
     * the transmission-file name plugged in) available for use as a sprintf(3)
     * template.
     */
    if (sp->s_xmit[0])
	ptr = sp->s_xmit;
    else
	ptr = DFTXMIT;
    cmdbuf[0] = '\0';
    foundfile = 0;
    for (tp = cmdbuf; *ptr; ptr++, tp++)
	if (ptr[0] == '%' && strchr("FIRUS", ptr[1]))
	{	    
	    if (ptr[1] == 'F')		/* %F -> batch name */
	    {
		(void) strcat(tp, fname);
		foundfile++;
	    }
	    else if (ptr[1] == 'I')	/* %I -> article ID */
		(void) strcat(tp, id);
	    else if (ptr[1] == 'R')	/* %R -> rcvr name */
		(void) strcat(tp, rcvcmd);
	    else if (ptr[1] == 'U')	/* %U -> U option */
	    {
		if (uopt && uval != (char *)NULL)
		    if (uval[0])
			(void) strcat(tp, uval);
		    else
			(void) strcat(tp, "-c");
	    }
	    else if (ptr[1] == 'S')	/* %S -> %s (system) */
		(void) strcat(tp, "%s");

	    tp += strlen(tp) - 1;
	    ptr++;
	}
	else				/* not an escape */
	{
	    *tp = *ptr;
	    tp[1] = '\0';
	}

    /* if the command obviously needs shell constructs, override S */
    if (noshell && (strpbrk(sp->s_xmit, "<>$") != (char *)NULL))
	noshell = FALSE;

    /*
     * If the transmission command doesn't take %F as an argument,
     * put the batch file on the command's standard input. Can't do
     * this in noshell mode yet.
     */
    if (foundfile == 0 && !noshell)
    {
	(void) strcat(cmdbuf, " <");
	(void) strcat(cmdbuf, fname);
    }

    /* break transmission to the system list into appropriate single xmits */
    if (mxcast)
	xmit1(cmdbuf, syslist, noshell);
    else
    {
	int	i;

	argc = vcrack(syslist, argv, MAXARGS);
	for (i = 0; i < argc; i++)
	    xmit1(cmdbuf, argv[i], noshell);
    }

    /* If D option is on, dump the data that would be sent to ADM/xmitlog */
    if (debugopt)
    {
	static char	*TRANSLOG;
	FILE		*dfp, *tfp = xfopen(fname, "r");
	int		c;

	if (TRANSLOG == (char *)NULL)
	    Sprint1(TRANSLOG, "%s/xmitlog", site.admdir);

	dfp = xfopen(TRANSLOG, "a");
	/* Unisoft 5.1 won't seek to EOF on "a" */
	(void) fseek(dfp, (off_t)0, SEEK_END);

	while ((c = fgetc(tfp)) != EOF)
	    (void) fputc(c, dfp);
	(void) fprintf(dfp, "\nEOT\n");
	(void) fclose(dfp);
	(void) fclose(tfp);
    }

    return(SUCCEED);
}

static void xmit1(cmd, sys, noshell)
/* perform a single news transmission */
char	*cmd;		/* command, with all but system name plugged in */
char	*sys;		/* system name to insert */
bool	noshell;	/* whether or not to spawn the command direct */
{
    char cmdbuf[BUFLEN];

    /* generate the actual transmission command */
    (void) sprintf(cmdbuf, cmd, sys);

    /* execute the generated transmission command */
    if (noshell)
    {
#ifdef DEBUG
	if (debug)
	    log1("via %s (no shell)", cmdbuf);
	else
#endif /* DEBUG */
	{
	    char *argv[MAXARGS];

	    (void) vcrack(cmdbuf, argv, MAXARGS);
	    (void) spawn(argv, FORGRND, (char *)NULL);
	}
    }
    else
    {
#ifdef DEBUG
	if (debug)
	    log1("via %s", cmdbuf);
	else
#endif /* DEBUG */
	    (void) system(cmdbuf);
    }
}

/*
 * This is the routine to hack if you want to use some bizarre form of e-mail
 * as a transport layer. The only requirement is that the other end understand
 * uurec-format news articles.
 */
private int bymail(fname, dest, name)
/* ship an article to name@dest by mail */
char	*fname;	    /* the article file */
char	*dest;	    /* the destination site */
char	*name;	    /* name of intended recipiant */
{
    FILE *out, *in = xfopen(fname, "r");

    /* generate a mail destination address */
    if (name[0] == '@')				/* an Internet address */
	(void) sprintf(bfr, "%s@%s", ++name, dest);
    else if (name[0] == ':')			/* a Berknet address */
	(void) sprintf(bfr, "%s:%s", ++name, dest);
    else					/* a UUCP address */
	(void) sprintf(bfr, "%s!%s", dest, name);

    /* ship it out through the library mailer inteface */
    if ((out = mailopen(bfr, "USENET articles")) == (FILE *)NULL)
	logerr1("Can't mail articles to %s", dest);
    else
    {
	(void) fputc('\n', out);

	/* now send each text line preceded by 'N', as uurec expects */
	bfr[0] = 'N';
	while (fgets(bfr + 1, sizeof(bfr), in))
	    (void) fputs(bfr, out);
	(void) mailclose(out);
    }

    return(SUCCEED);
}

char *filefilter(sp, fname)
/* do compression and encoding on a file according to C and E options */
feed_t	*sp;
char	*fname;
{
/* C:	compress the article before transmission */
	char	*compress = s_option(sp, 'C');
/* E:	make sure all characters are 7 bits only */
	bool	eencode = (s_option(sp, 'E') != (char *)NULL);
/* V:	spawn 2.10.X-style remote agents or other archaisms */
	char *version = s_option(sp, 'V');

    static char	    squashed[SBUFLEN];

    /* handle compression and encoding options */
    if (compress)
    {
	/* here's where we'll put the compressed version */
	(void) strcpy(squashed, "/tmp/squashedXXXXXX");
	(void) mktemp(squashed);

	/* first, generate archaic headers if necessary */
	if (version != (char *)NULL && strncmp(version, "2.10", 4) == 0)
	{
#ifndef ENCODE
	    if (eencode)
		(void) sprintf(bfr, "echo '#! un7compress' >%s", squashed);
	    else
#endif /* ENCODE */
		(void) sprintf(bfr, "echo '#! uncompress' >%s", squashed);
	    if (system(bfr))
	    {
		logerr1("header generation '%s' failed", bfr);
		return((char *)NULL);
	    }
	}

	/* O.K., now generate the compression command */
	(void) sprintf(bfr, "%s/%s %s%s <%s >>%s",
		site.libdir, COMPRESS,
		*compress ? "-" : "", compress, fname, squashed);

#ifdef DEBUG
	if (verbose >= V_FILTERBLAB)
	    (void) printf("filefilter: %s will be compressed via %s\n",
			  fname, bfr);
#endif /* DEBUG */

	/* now execute it */
	if (system(bfr))
	{
	    logerr1("compression command %s failed", bfr);
	    return((char *)NULL);
	}
	else if (eencode)
	{
#ifndef ENCODE
	    logerr0("The E option must be enabled by compiling with ENCODE");
	    return((char *)NULL);
#else
	    FILE	    *ifp, *ofp;
	    static char	    encoded[SBUFLEN];

#ifdef DEBUG
	if (verbose >= V_FILTERBLAB)
	    (void) printf("filefilter: %s will be encoded\n", fname);
#endif /* DEBUG */

	    (void) strcpy(encoded, "/tmp/encodeXXXXXX");
	    (void) mktemp(encoded);
	    (void) fflush(stdout);	/* V7 system() doesn't flush stdout */
	    if ((ifp = fopen(squashed, "r")) == (FILE *)NULL)
	    {
		logerr1("couldn't get at compressed text %s", squashed);
		return((char *)NULL);
	    }
	    if ((ofp = fopen(encoded, "w")) == (FILE *)NULL)
	    {
		logerr1("couldn't write encoded text %s", encoded);
		return((char *)NULL);
	    }
	    encode(ifp, ofp);
	    (void) fclose(ifp);
	    (void) fclose(ofp);
	    return(encoded);
#endif /* ENCODE */
	}
	else
	{
	    (void) fflush(stdout);	/* V7 system() doesn't flush stdout */
	    return(squashed);
	}
    }
    return((char *)NULL);
}

#ifdef ARTFILTER	/* not currently used, left in for the future */
/*ARGSUSED1*/
char *artfilter(sp, fname)
/* perform per-article filtering */
feed_t	*sp;
char	*fname;
{
/* A:	afmt: the other machine runs an alternate format */
	bool afmt = (s_option(sp, 'A') != (char *)NULL);
/* B:	B format: this is the default, but don't reuse flag */
    if (afmt)
    {
	static char	aform[BUFLEN];	/* transformed name for A option */
	register int    c;
	register FILE   *ifp, *ofp;
#ifndef u370
	static hdr_t hh;	/* must be initially all zeros */
#endif /* u370 */

#ifdef DEBUG
	if (verbose >= V_FILTERBLAB)
	    (void) printf("artfilter: %s changed to alternate form\n", fname);
#endif /* DEBUG */

	/* declare a scratch file to do transformations into */
	(void) strcpy(aform, "/tmp/aformXXXXXX");
	(void) mktemp(aform);

	hfree(&hh);
	if ((ifp=xfopen(fname,"r")) == (FILE *)NULL || hread(&hh, 0L, ifp)==0)
	{
	    logerr1("Couldn't reread header of %s for reformat", fname);
	    return((char *)NULL);
	}

	/*
	 * Whatever code you put in the function ahwrite() is presumed to do
	 * the proper header munging.
	 */
	ahwrite(&hh, ofp = xfopen(aform, "w"));

	while ((c = getc(ifp)) != EOF)
		(void) putc(c, ofp);
	(void) fclose(ifp);
	(void) fclose(ofp);

	return(aform);
    }
    return((char *)NULL);
}
#endif /* ARTFILTER */

int xmitctrl(target, sp, title, ng, body)
/* transmit a control message or notification */
char	*target;		/* system to transmit to */
feed_t	*sp;			/* its feed file entry */
char	*title, *ng, *body;	/* header data */
{
#ifndef u370
    static hdr_t hh;
#endif /* !u370 */
    char *id, xtmp[SBUFLEN];
    FILE *tfp, *infp;
    int st;

    (void) sprintf(bfr, "%s@%s", NEWSUSR, site.fromname);
    hlcpy(hh.h_from, bfr);
    hlcpy(hh.h_path, NEWSUSR);
    hlcpy(hh.h_newsgroups, ng);
    hlcpy(hh.h_ctlmsg, title);
    hlcpy(hh.h_subject, title);
    hh.h_posttime = time((time_t *)NULL);

    (void) strcpy(xtmp, "/tmp/xmsgXXXXXX");
    hwrite(&hh, tfp = xfopen(mktemp(xtmp), "w"), FALSE);
    if (body && (infp = fopen(body, "r")))
    {
	while (fgets(bfr, sizeof(bfr), infp))
	    (void) fputs(bfr, tfp);
	(void) fclose(infp);
    }
    (void) fclose(tfp);

    id = idmake(xtmp);
    st = transmit(target, sp, id, xtmp, TRUE, FALSE);
    (void) unlink(xtmp);
    return(st);
}

/* transmit.c ends here */
