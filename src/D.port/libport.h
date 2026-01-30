/* libport.h -- definitions for the portability library functions */

#include "system.h"

#if defined(__STDC__)

# define P(s)		s

#else /* __STDC__ */

#define ASTERISK	*
#ifdef ENCORE
# define P(s)		()
#else
# define P(s)		(/ASTERISK s ASTERISK/)
#endif /* ENCORE */
#undef ASTERISK
#define const
#define signed
#define volatile

#endif

/* these are assumed by the portlib.a functions */
#define	SUCCEED	0	/* function succeeded				*/
#define	FAIL	-1	/* function failed				*/

/*
 * We use this to label forward declarations of static functions (non-statics
 * are, by convention, declared in the module interface file, which the module
 * invariably includes). In an ANSI C environment this may have to become
 * 'static', but some present compilers won't accept this.
 */
#define forward		/* this space intentionally left blank :-) */

/* 
 * Define a bool type for use as a logical value. The #ifndef TRUE is so we
 * can arrange not to collide with the curses.h definition.
 */
#ifndef TRUE
typedef int	bool;
#define	TRUE	1	/* boolean true					*/
#define	FALSE	0	/* boolean false				*/
#endif /* TRUE */

/*
 * the following horrible cascaded series of macro definitions is a
 * kluge to get around the non-portability of varargs
 */
extern char	xbf[];

#ifndef lint
#define v0(mf,f)		(void) mf(f)
#define v1(mf,f,x1)		(void) (sprintf(xbf, f, x1),mf(xbf))
#define v2(mf,f,x1,x2)		(void) (sprintf(xbf, f, x1,x2),mf(xbf))
#define v3(mf,f,x1,x2,x3)	(void) (sprintf(xbf, f, x1,x2,x3),mf(xbf))
#define v4(mf,f,x1,x2,x3,x4)	(void) (sprintf(xbf, f, x1,x2,x3,x4),mf(xbf))
#else
#define v0(mf,f)		(void) sprintf(xbf, f)
#define v1(mf,f,x1)		(void) sprintf(xbf, f, x1)
#define v2(mf,f,x1,x2)		(void) sprintf(xbf, f, x1, x2)
#define v3(mf,f,x1,x2,x3)	(void) sprintf(xbf, f, x1, x2, x3)
#define v4(mf,f,x1,x2,x3,x4)	(void) sprintf(xbf, f, x1, x2, x3, x4)
#endif /* lint */

extern int xerror();
#define xerror0(f)		v0(xerror, f)
#define xerror1(f, x1)		v1(xerror, f, x1)
#define xerror2(f, x1, x2)	v2(xerror, f, x1, x2)
#define xerror3(f, x1, x2, x3)	v3(xerror, f, x1, x2, x3)

extern	int	more(), peclose();
extern	char	*arpadate(), *savestr(), *errmsg(), *fullname(), *backquote();
extern	time_t	modtime(), getdate(), cgtdate();
extern	void	lcase(), setmodtime();
extern	bool	exists(), isdir(), prefix(), bitbucket(), setenv(), delenv();
extern	ulong	checksum(), checkstring();
extern	off_t	filesize(), df();
extern	FILE	*xfopen(), *peopen();

/* each main()-containing module that uses xerror() must declare this */
extern	char	*Progname;

/* libport.h ends here */
