/* news.h - parameters for everyone in the netnews software */

/* we need this in front of config.h for use in expiration times */
#define DAYS	(60L*60L*24L)
#define WEEKS	(7*DAYS)

#include "libport.h"	/* portlib.h includes configsys.h and system.h */
#include "confignews.h"

/* Things you might want to change once in a blue moon */
#define NEWSRC  ".newsrc"	/* name of .newsrc file (in home dir)	*/
#define DEFDIR	"~/News/"	/* default user news save directory	*/
#define NEGCHAR	'!'	/* newsgroup negation character			*/
#define DEADTIME 45	/* no. of seconds to wait on deadlock		*/
#ifdef SMALL_ADDRESS_SPACE
# define BUFLEN	128	/* standard buffer size				*/
#else
# define BUFLEN	256	/* standard buffer size				*/
#endif /* SMALL_ADDRESS_SPACE */
#define LBUFLEN 1024	/* big buffer size				*/
#define	SBUFLEN 32	/* small buffer size (for system names, etc)	*/
#define SYSPATH	"PATH=/local/bin:/bin:/usr/bin"	/* default, secure, vanilla path */

/* Things you probably won't want to change */
#define CMPMAGIC  "\037\235"	/* 1F 9D -- heads a compressed news file */
/* #undef BIGGROUPS		/* Use longs for article numbers */
#define PATHLEN 512	/* length of longest source string		*/
#define	DATELEN	64	/* length of longest allowed date string	*/
#define	NAMELEN	80	/* length of longest possible message ID	*/
#define NETCHRS	"!:@^%"	/* punct. chars used for various networks	*/
#define PATHSEP	'!'	/* uucp path separator character		*/
#define	NGDELIM	','	/* delimit character in news group line		*/
#define LISTSEP  ", "	/* should be {NGDELIM, ' ', '\0'}		*/
#define NGSEP	'.'	/* newsgroup component separator		*/
#define MAILSEP	'-'	/* sub for NGSEP in mail addresses 		*/
#define JUNKSUF	".junk"	/* common suffix for junk groups		*/

typedef unsigned    bits_t;	/* used by newsrc,FEEDBITS code for bitmasks */

#ifdef BIGGROUPS
typedef long	nart_t;
#define atoa(s)	atol(s)
#else
typedef int	nart_t;
#define atoa(s)	atoi(s)
#endif /* BIGGROUPS */

#ifndef private
#define private static
#endif /* private */

#define Sprint1(where,fmt,arg)	(void)sprintf(bfr,fmt,arg),where=savestr(bfr)
#define Sprint2(where,fmt,a,b)	(void)sprintf(bfr,fmt,a,b),where=savestr(bfr)

/* function(s) and storage owned by newsinit.c */
extern void newsinit();
extern	int	uid, gid, duid, dgid, sigcaught;
extern	bool	privileged;
extern	char	*username, *userhome;
extern	char	bfr[LBUFLEN];
#ifdef DEBUG
extern	int	debug;		/* if >0, don't do database munging */
#endif /* DEBUG */
extern	int	verbose;	/* level of diagnostics to emit */

#ifdef RUNTIME
extern char *newsattr();
#else
#define newsattr(attr, def)	def
#endif /* !RUNTIME */

typedef struct
{
    char    *notify;		/* send notification mail to this address */
    char    *nodename;		/* the system's unmodified sitename */
    char    *pathname;		/* its name for bang paths */
    char    *fromname;		/* the Internet name of its gateway machine */
    char    *truename;		/* its real Internet name */
    char    *smarthost;		/* path to an autouter (if any) */
    char    *libdir;		/* site library directory */
    char    *admdir;		/* site administration directory */
    char    *leastuid;		/* least UID of real users */
    char    *uuprog;		/* name of uuname filter program */

    /* these aren't used in network-based readers */
    char    *spooldir;		/* site spool directory */
    char    *textdir;		/* site article directory */
    char    *batchdir;		/* site batch directory */
    char    *archdir;		/* site archive directory */
}
siteblk;
extern siteblk	site;		/* where to find the site data */

/* escape character for group names in LIB/fascist and LIB/moderators files*/
#define GRPESCAPE	'*'

#define NULLPRED    (bool (*)())NULL

/* external function declarations for newslib.a and miscellaneous modules */
extern	int	linecount(), mailclose();
extern	char	*mailreply(), *organization(), *ospawn();
extern	bool	ngmatch();
extern	FILE	*xfopen(), *msgopen(), *mailopen();
extern	catch_t	xxit();
#define msgclose(fp) (void) fclose(fp)

/* these are actually from getfiles.c or a network service library */
#ifndef NONLOCAL
extern char *artdir();
#endif /* NONLOCAL */
extern void artinit();
extern int artname();

/* these are from the article-getter logic in getart.c */
#define A_LINECOUNT	1	/* article line count is wrong */
#define A_FETCHOK	0	/* article fetched OK */
#define A_MISSING	-1	/* article is missing */
#define A_NOOPEN	-2	/* article file could not be opened */
#define A_GARBLED	-3	/* article header is garbled */
extern int getart(), arterr(), indexline();
extern void author();
#define I_NOART	'~'	/* article is missing */
#define I_LOCAL	'!'	/* article originated by calling user */
#define I_BLANK	' '	/* ordinary article */

#ifdef NONLOCAL
/* these will be from a network service library */
extern char *getactive();
extern char *getgroups();
extern char *getdistribs();
extern int net_post();
#endif /* NONLOCAL */

/* these are from escapes.c */
#define RNESCCHR	'%'	/* escape character for builtin expansion */
extern void escapes();		/* expand C-style escapes */
extern void setexphook();	/* set macroexpansion hook */
extern void seeheader();	/* set 'current header' for macro purposes */
extern void strexpand();	/* expand escape characters in a string */
extern void fnexpand();		/* expand escape characters in a filename */

/* these are from artlist.c */
typedef struct
{
    char *l_this;
    char *l_next;
    char *l_bufcopy;
}
lptr_t;

int refname();
void artlstset();
int artlstloc();
int artlstdel();
char	*artlstret();

/* these are from articleid.c */
typedef struct
{
    time_t	id_timestamp;
    long	id_hash;
    /* extension fields, if any, go here */
    char	*id_username;
    char	*id_homesite;
}
artid_t;
extern int rewrite();
extern ulong hashart();
extern char *idmake();
extern artid_t *idparse();
extern bool idvalid();

#define NO_SIGNAL   0	/* in sigcaught, means no signal is pending */

/* news.h ends here */
