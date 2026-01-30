/*
 * robot-proto.c -- transaction robot prototype
 *
 * This is a prototype code frame for building new BBS transaction robots. As
 * written, it implements a bunch of no-op capability functions for a new
 * service type `fubar' which your bbsauto control files know as `FubarServe'.
 * You will want to change this.
 *
 * To build a new robot, make a copy of this file and fill in the blanks.
 * The primitives you have available for construction are described below.
 *
 * It will help if you cast an eyeball over an already-implemented robot
 * module first, then look at the bbsauto.c source. Pay particular attention
 * the debug levels D_* defined at the head of the bbsauto.c file.
 */
/*LINTLIBRARY*/
#include "news.h"
#include "libpriv.h"
#include "header.h"
#include "bbsauto.h"

forward void bbslogon(), bbsread(), cpost(), bbslogoff(),
			bbstonews(), bbstomail(), cresynch();

bbstype fubar =
{
    "FubarServe",	/* name of service */
    bbslogon,		/* logon code */
    bbsread,		/* read-side interpreter */
    bbspost,		/* per-article post code */
    bbslogoff,		/* logoff code */
    bbstonews,		/* BBS-news to netnews translation */
    bbstomail,		/* BBS-mail to UNIX-mail format */
    bbsresynch,		/* resynchronization function */
};

/*
 * Here are your dialogue primitives:
 *
 * echo()		-- enable/disable echo checking
 *
 * Normally, each send() is echo-checked; that is, after sending each string
 * the send() does an expect() of it automatically. This is the right thing for
 * full-duplex hosts. You can turn it off with echo(OFF); you will typically
 * need to do this when sending passwords, which aren't normally echoed. The
 * check can be re-enabled with echo(ON).
 *
 * send()		-- send a string to the bbs
 *
 * Send a string to the BBS. Takes up to five arguments; the first is a
 * sprintf(3)-format string, the others string values to be plugged into
 * whatever %s constructs occur in the format.
 *
 * await()		-- await one of several responses
 *
 * Takes an alternatives count, a string array of alternatives, a timeout,
 * and a max-retry count. Reads characters until one of the alternatives
 * matches or a timeout is raised; repeats this for the given number of
 * retries. Failure returns FAIL, success returns the index of the matching
 * argument in the string array. All characters read are logged to the current
 * logfile (defaults to stdout). The expect() and capture() functions are
 * shells around escape.
 *
 * expect()		-- expect a given prompt
 *
 * See the given string from the BBS host within RESPTIME seconds or retry.
 * After RETRIES retries with no match, punt(). Characters read are logged
 * to the current logfile (defaults to stdout).
 *
 * saw()		-- check if string was in last line received
 *
 * This is a convenience. You have direct access to the last-line buffer at
 * remote.lastline. The saw() function returns FAIL or an index.
 *
 * capture()		-- catch everything up to a given prompt
 *
 * Capture everything up to the given prompt. If you don't see the prompt
 * within CAPTIME seconds, punt().
 *
 * msplit()		-- split current message into chunks
 *
 * Doesn't split the message itself. Rather, it generates a vector of
 * file offsets that break an article into the largest pieces that
 * a) smaller than remote->type.maxpostsize, b) end with newlines. Useful
 * for writing automatic message-splitting code for boards that enforce an
 * upper limit on article size.
 *
 * upload()		-- upload a file section to the BBS
 *
 * Upload a given section of the current message to the BBS. Various
 * upload modes may be supported, see bbsauto.c for details.
 *
 * errmail()		-- send error mail back to message originator
 *
 * Ship current article back to originator inside an error wrapper. You
 * can give it an error type string that will be included in the
 * notification. This is intended for errors that don't abort processing.
 *
 * punt()		-- abort bbs session, error-return all articles
 *
 * Abort the BBS dialogue, intended for use as a last resort inside your
 * bbspost() function. Articles already read or posted will be processed
 * normally, but the current article and all following will be errmailed
 * back to their originators. Note: most robots will *not* need to use this
 * function!
 *
 * When debugging, remember that the -D, -f, -s and -m options are your
 * friends; they will enable you to test the dialogue and translation parts
 * separately. A typical robot implementation sequence should go something like
 * this:
 *
 * 1. Write bbslogon() and bbslogoff() in terms of expect/send, and generate
 *    an appropriate entry in your $HOME/.bbsauto (see news(5) for format
 *    details).
 *
 * 2. Verify that you can connect, log on to and log off of the BBS service by
 *    running bbsauto in `snarf' mode (-s option, dialogue test only, with
 *    captured news and mail going to news.capture and mail.capture
 *    respectively).
 *
 * 3. Write bbspost(). Generate an RFC822 test batch and feed it to bbsauto -p;
 *    watch the resulting transaction unfold on stdout. For more debugging info
 *    use the -D option.
 *
 * 3. Write bbsread(). Test it in snarf mode. Keep the capture files around
 *    for the next step.
 *
 * 4. Write bbstonews(). Invoke bbsauto -f -p news.capture to test the news
 *    translation and posting without calling the BBS. To keep the translation
 *    tempfiles around for examination use the -D option.
 *
 * 5. Write bbstomail(). Invoke bbsauto -f -m -p mail.capture to test the mail
 *    translation and posting without calling the BBS. To keep the translation
 *    tempfiles around for examination use the -D option.
 *
 * Learn from others' experience! Study any robots included in your
 * distribution for usage examples. Your BBS probably isn't *so* different from
 * others already supported.
 */

static void bbslogon()
/* go through normal login sequence */
{
    /*
     * This routine will typically be a straight expect-send sequence
     * with no conditionals.
     */
}

static void bbsread(newsfp, mailfp)
/* transaction interpreter for CompuServe */
FILE	*newsfp;	/* where to put captured news */
FILE	*mailfp;	/* where to put captured mail */
{
    /*
     * This will be the hairy one. It should pick up news and mail both,
     * doing as close to a straight capture as possible (translation should
     * be deferred till you're not on-line).
     */
}

static void bbspost()
/* post the article or BBS mail defined by a header */
{
    /*
     * Post the news or mail defined by the contents of the `header' global.
     * The bbsauto.c code will step through an input batch applying this.
     *
     * You can report non-fatal errors using errmail(); if you punt(), the
     * current and all subsequent articles will be mailed back to the
     * originators. An expect() error within bbspost is treated like a punt().
     */
}

static void bbslogoff()
/* log us off CompuServe */
{
    /*
     * You want this to send whatever commands are needed for a `clean' logoff.
     */
}

static void bbsresynch()
/* try to resynchronize following error */
{
    /*
     * This function is called after each expect() time out, before retry
     * (until you run out of retries, that is). The idea is for it to utter
     * something that will clear the BBS's line input buffer and get you back
     * to a prompt. This may enable you to recover from line noise glitches.
     */
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

/* robot-proto.c ends here */
