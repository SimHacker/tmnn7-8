/* bbsauto.h -- types and externs for BBS transaction robots */

/* encapsulate what we need to know about a bbs command set */
typedef struct
{
    char	*typename;	/* name of this service type */
    void	(*bbslogon)();	/* BBS logon function */
    void	(*bbsread)();	/* transaction code for reading from board */
    void	(*bbspost)();	/* per-article post code for BBS */
    void	(*bbslogoff)();	/* BBS logoff function */
    void	(*tonews)();	/* xlate BBS-format news to netnews batch */
    void	(*tomail)();	/* xlate BBS-format mail to V7-format mail */
    void	(*resynch)();	/* try to resynchronize with bbs */
}
bbstype;

#define MAXTOPICS	128	/* max topics we can snarf */
#define IDSIZE		20	/* max len of login ID, password, hostname */

/* use this as a mail message separator when passing captured mail back */
#define MDELIM		"\003\n"

/* encapsulate our account info for a given BBS */
typedef struct
{
    /* variables set by user profile */
    char	*servname;		/* service name */
    char	*dial;			/* dial command */
    char	*servtype;		/* service type */
    bbstype	*type;			/* the real type pointer */
    char	*hostname;		/* hostname to enter (if any) */
    char	*logon;			/* our login ID */
    char	*password;		/* our password info */
    char	*owner;			/* UNIX name of account owner */
    char	*notify;		/* notify list for incoming BBS news */
    char	*alltopics;		/* where the topic data lives */

    /* parsed form of subscription */
    int		ntopics;		/* count of topic names */
    char	*topics[MAXTOPICS];	/* topic subscription */

    /* variables set by BBS read */
    int		nnews;			/* count of articles waiting */
    char	newsfile[BUFSIZ];	/* file containing waiting news */
    int		nmail;			/* count of mail messages waiting */
    char	mailfile[BUFSIZ];	/* file containing waiting mail */

    /* internal per-BBS variables */
    char	lastline[BUFSIZ];	/* last line of input accepted */
}
bbs;

/* upload modes */
#define M_NOXLATE	0	/* no translation */
#define M_LFTOCRLF	1	/* map LF to CR-LF */
#define M_XMODEM	2	/* XMODEM protocol */

extern bbs remote;
extern bool doecho;

#define echo(v)	(doecho = (v))
#define OFF	FALSE
#define ON	TRUE

extern int await();
extern void send();
extern void expect();
extern int strindex();
extern int saw();
extern int capture();
extern int msplit();
extern void upload();
extern void errmail();
extern void punt();

/* bbsauto.h ends here */
