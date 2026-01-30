/* libpriv.h -- definitions and declarations of privileged news functions */

#ifdef LOCKF
#ifdef DBHISTORY
#define ENTRYLOCK	/* we can lock individual records */
#endif /* DBHISTORY */
#endif /* LOCKF */

/* textwalk() mode flags */
#define TW_DIRECTORY	0
#define TW_BADNUMBER	1
#define TW_GROUP	2
#define TW_ARTICLE	3
extern int textwalk();

extern void loginit();

/* the v[012] macros are defined in portlib.h */

extern int logx();
#define log0(f)			v0(nlog, f)
#define log1(f, x1)		v1(nlog, f, x1)
#define log2(f, x1, x2)		v2(nlog, f, x1, x2)
#define log3(f, x1, x2, x3)	v3(nlog, f, x1, x2, x3)
#define log4(f, x1, x2, x3, x4)	v4(nlog, f, x1, x2, x3, x4)

extern int logerr();
#define logerr0(f)		v0(logerr, f)
#define logerr1(f, x1)		v1(logerr, f, x1)
#define logerr2(f, x1, x2)	v2(logerr, f, x1, x2)
#define logerr3(f, x1, x2, x3)	v3(logerr, f, x1, x2, x3)

/* things exported by transmit.c */
extern int transmit(), xmitctrl();
extern char *artfilter(), *filefilter();

/* miscellaneous library functions */
extern	int	mungread(), mungwrite();
extern	void	lock(), unlock(), collect();
extern	void	privlock(), privunlock();
extern	bool	privlockcheck(), lockp();

#ifdef OPENDEBUG
#define fopen(n, m)	d_fopen(n, m)
#define fclose(p)	d_fclose(p)
#define open(n, m)	d_open(n, m)
#define close(d)	d_close(d)
extern FILE *d_fopen();
extern int d_fclose(), d_open(), d_close();
#endif /* OPENDEBUG */

/* libpriv.h ends here */
