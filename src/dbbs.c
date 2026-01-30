/*
 * dbbs.c -- transaction robot for DBBS systems
 *
 * This assumes you log on in DBBS `Xpert' mode. It also assumes that all
 * conferences enable private mail.
 */
/*LINTLIBRARY*/
#include "news.h"
#include "libpriv.h"
#include "header.h"
#include "bbsauto.h"

#define	LINEWIDTH	72	/* max line width for DBBS messages. */

#define PWDNG	"Password Does Not Match!!!\r\n"
#define MCONT	"****  Hit any key to continue  ****\r\n"
#define MAIN	"MAIN -> A,J,X,F,U,B,W,G,E,C,R,S,P,M,K,H,? > "
#define YESMAIL	""
#define NOMAIL	"No Mail FOR You Today...\r\n"
#define NOCONF	"you cannot join"
#define GENERAL	"general"

forward void bbslogon(), bbsread(), bbspost(), bbslogoff(),
			bbstonews(), bbstomail(), bbsresynch();

bbstype dbbs =
{
    "DBBS",		/* name of service type */
    bbslogon,		/* logon code */
    bbsread,		/* read-side interpreter */
    bbspost,		/* per-article post code */
    bbslogoff,		/* logoff code */
    bbstonews,		/* BBS-news to netnews translation */
    bbstomail,		/* BBS-mail to UNIX-mail format */
    bbsresynch,		/* resynchronization function */
};

static void bbslogon()
/* go through normal login sequence */
{
    char	*cp;
    static char *pwdresp[] = {PWDNG, "#"};

    send("\r\n\r\n");
    expect("FIRST Name: "); send("%s\r\n", remote.hostname);
    expect("LAST Name: "); send("%s\r\n", remote.logon);

    expect("Password -> ");
    echo(OFF);
    for (cp = remote.password; *cp; cp++)
    {
	send("%c", *cp);
	if (!await(2, pwdresp, 10, 1))
	    xerror1("password for %s no good!", remote.servname);
    }
    echo(ON);
    send("\r\n");
    expect(MCONT);
    send("\r\n");
}

static int gotoconf(grp)
/* go to a given forum */
char	*grp;
{
    static char	cgroup[BUFLEN] = GENERAL;
    static char *grpresp[] = {NOCONF, MAIN};

    if (strcmp(grp, cgroup))
    {
	if (strncmp(grp, GENERAL))
	    send("J;%s\r\n", grp);
	else
	    send("J;M\r\n");
	(void) strcpy(cgroup, grp);
	if (await(2, grpresp, 10, 1))
	    return(TRUE);	/* conference OK */
	else
	    return(FAIL);	/* no such conference */
    }
    else
	return(FALSE);		/* we're already there */
}

static void bbsread(newsfp, mailfp)
/* transaction interpreter for CompuServe */
FILE	*newsfp;	/* where to put captured news */
FILE	*mailfp;	/* where to put captured mail */
{
    int	i;
    static char *mailresp[] = {YESMAIL, NOMAIL};

    expect(MAIN);

#ifdef TRYREAD
    /* read personal mail */
    send("P");
    if (await(2, mailresp, 60, 1))
	expect(MAIN);
    else
    {
	/* read mail */
    }

    /* now read `news' (forum postings) */
    for (i = 0; i < remote.ntopics; i++)
    {
	gotoconf(remote.topics[i]);
	send("READ *\r\n");
    }
#endif /* TRYREAD */
}

static void bbspost()
/* post the article or BBS mail defined by a header */
{
    char	c, *cp, *grp;
    off_t	linec, ccnt = header.h_endoff - header.h_startoff;
    bool	priv = FALSE;
    static char ngdel[] = {NGDELIM, ' ', '\0'};

    /* mail gets privately posted to the main message base */
    if (hlblank(header.h_newsgroups))
    {
	(void) sprintf(bfr, "%s.%s", remote.servname, GENERAL);
	(void) hlcpy(header.h_newsgroups, bfr);
	priv = TRUE;
    }

    /* there are no 'cross-posts' in the DBBS world */
    grp = strtok(header.h_newsgroups, ngdel);
    do {
	/*
	 * First, make sure current group is a valid DBBS conference
	 */
	if (!prefix(grp, remote.servname))
	{
	    logerr1("invalid group %s in DBBS posting", grp);
	    break;
	}
	else
	    grp += strlen(remote.servname) + 1;

	/* to post to a forum, we must go there */
	if (gotoconf(grp) == FAIL)
	{
	    errmail("no such conference as %s", grp);
	    continue;
	}

	/* start entry */
	send("E\r\n");
	expect("To: ");
	if (hlblank(header.h_to))
	    send("\r\n");
	else
	    send("%s\r\n", header.h_to);
	expect("Subject: ");
	send("%s\r\n", header.h_subject);
	expect("private? ");
	if (priv)
	    send("Y");
	expect(" 1: ");

	if (fseek(header.h_fp, header.h_textoff, SEEK_SET) == FAIL)
	    xerror1("seek to upload section failed, errno %d", errno);
	cp = bfr;
	linec = (off_t)1;
	while (ccnt-- > 0)
	{
	    *cp++ = c = fgetc(header.h_fp);
	    if (c == '\n' || (cp - bfr) > LINEWIDTH)
	    {
		if (cp[-1] == '\n')
		    cp--;
		*cp = '\0';
		send("%s", bfr);
		send("\r\n");
		(void) sprintf("%2ld: ", ++linec);
		expect(bfr);
		cp = bfr;
	    }
	}

	send("S");
	expect(MAIN);
    } while
	    (grp = strtok((char *)NULL, ngdel));
}

static void bbslogoff()
/* log us off the BBS */
{
    send("G\r\n");
    expect("(click)");
}

static void bbsresynch()
/* try to resynchronize following error */
{
    /* no-op */
}


static void bbstonews(ifp, nfp, mfp)
/* tranform a batch of BBS news into netnews form */
FILE *ifp;	/* BBS news batch */
FILE *nfp;	/* news batch file */
FILE *mfp;	/* mail batch file */
{
    /*
     * This translation will be done after the login session.
     *
     * You want to use this to translate BBS header and prompt lines (if
     * any) appearing in the news capture file into pure RFC-822 headers.
     * Ordinary text lines can usually be passed through unaltered. The
     * BBS-independent code will then insert batch delimiter lines and
     * feed the results to rnews.
     */
}

static void bbstomail(ifp, ofp)
/* mung a batch of BBS mail into RFC822 mailbox form with delimiters */
FILE *ifp, *ofp;
{
    /*
     * This translation will be done after the login session.
     *
     * You want to use this to translate BBS header and prompt lines (if
     * any) appearing in the mail capture file into pure RFC-822 headers.
     * Ordinary text lines can usually be passed through unaltered. The
     * code should insert the MDELIM string at the ends of mail messages
     * (this is so such messages can include RFC-822 headers in the body).
     *
     * BBS-independent code will then call rmail on each batch component,
     * forging the sender address to whatever is in the munged From line.
     */
}

/* dbbs.c ends here */
