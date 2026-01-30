/* system.h -- this file defines our interfaces to the OS */

#ifndef SHELL		/* we may already have included configsys.h */
#include "configsys.h"	/* so we'll know the system type */
#endif /* SHELL */

#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <grp.h>
#include <pwd.h>
#include <sys/stat.h>
#include <ctype.h>
#include <errno.h>
#include <sys/ioctl.h>

#ifdef lint
struct Generic {char *a;};	/* copes with sys/ioctl.h lossage */
#endif

#if !defined(SYSV3) && !defined(SYSV4) && !defined(MACII)
typedef unsigned long	ulong;	/* need this for unsigned 32-bit quantities */
#endif /* !defined(SYSV3) && !defined(SYSV4) !defined(MACII) */

typedef unsigned char	uchar;	/* For bit flicking..... */

#ifdef DIRENT
#include <dirent.h>	/* we may have to say ld ... -lposix for this */
#else
#define dirent	direct	/* KLUGE: I hope this doesn't bite us... */
#ifdef LIBNDIR
#include <sys/ndir.h>
#else
#include <sys/dir.h>
#endif /* LIBNDIR */
#endif /* DIRENT */

#ifdef SCO_XENIX
#define SYSV2
#endif

/* don't use slightly slower code for various EBCDIC manipulations */
#define ASCII	/* we are, thank Goddess, not victims of IBM braindamage */

/* SV has memory.c, otherwise we must use (possibly slower) bcopy()/bzero() */
#if defined(SYSV1) || defined(SYSV2) || defined(SYSV3)
#define SYSV
#define bcopy(s, t, n)	memcpy(t, s, n)
#define bzero(s, n)	memset(s, '\0', n)
#else
extern int bcopy(), bzero();
#endif

/* AT&T versions use (unsigned) for I/O lengths, others use (int) */
#if defined(SIII) || defined(SYSV) && !defined(USG)
#define USG
typedef unsigned iolen_t;	/* use this to cast read()/write() lengths */
#ifndef SCCSID			/* only USG systems guarantee SCCS */
#define SCCSID
#endif /* SCCSID */
#else	/* V7/BSD */
typedef int iolen_t;		/* use this to cast read()/write() lengths */
#endif

/* Terminal control -  AT&T versions are vastly different from V7/BSD */
#ifdef TERMIO
#include <termio.h>
#else /* !TERMIO */
#ifndef _IOCTL_		/* avoid including ioctl.h twice on SunOS 4.0 */
#include <sgtty.h>
#endif /* _IOCTL_ */
#endif /* !TERMIO */

/* 4.3BSD acts like 4.2 most ways */
#ifdef BSD4_3
#undef BSD4_3	/* because param.h is going to define this */
#include <sys/param.h>
#define BSD4_2
#ifndef _NFILE
#define _NFILE	NOFILE
#endif /* _NFILE */
#endif /* BSD4.3 */

/* BSD versions are like Version 7 for most code-generation purposes */
#if defined(BSD4_1) || defined(BSD4_1C) || defined(BSD4_2)
#define V7
#endif /* defined(BSD4_1) || defined(BSD4_1C) || defined(BSD4_2) */

/* get constants for fcntl() calls if we have them */
#ifdef BSD4_2
#include <sys/file.h>	/* use this rather than fcntl.h to also get LOCK_EX */
#else
#if defined(USG) || defined(BSD4_1C) || defined(FCNTL)
#include <fcntl.h>
#endif /* !v7 */
#endif

/* find time types */
#if defined(BSD4_2) || defined(BSD4_1C)
#include <sys/time.h>
#else
#include <time.h>
#endif /* defined(BSD4_2) || defined(BSD4_1C) */

/* deal with Zilog Z8000 System III brain damage */
#if defined(z8000) && !defined(SYSV1)
#define setjmp setret
#define longjmp longret
#define jmp_buf ret_buf
#include <setret.h>	/* WARNING: longret won't restore register vars */
#else
#include <setjmp.h>
#endif

/*
 * The test for XENIXness here takes care of a weird case -- XENIX has a native
 * struct utsname but its uname(3) result is garbage.
 */
#if defined(UNAME) || defined(M_XENIX)
#include <sys/utsname.h>
#else
/*
 * 9 bytes is for compatibility with USG, in case you forget to define UNAME.
 * 33 bytes in nodename because many sites have names longer than 8 chars.
 */

struct utsname
{
    char    sysname[9];
    char    nodename[33];
    char    release[9];
    char    version[9];
};
#endif /* defined(UNAME) || defined(M_XENIX) */

#ifdef USG
struct timeb
{
	time_t	time;
	unsigned short millitm;
	short	timezone;
	short	dstflag;
};
#else
#include <sys/timeb.h>
#endif

#ifdef BSD4_2
#ifndef sigmask
#define sigmask(m) (1<<((m)-1))
#endif /* !sigmask */
#endif /* BSD4_2 */

#ifdef SYSV3
/* use the superior BSD-like signals in SVr3 */
#define signal(sig, func)	sigset(sig, func)
#endif /* SYSV3*/

/*
 * Here's how we con lint into accepting signal(2) expressions about which
 * it is notoriously picky. Random second arguments of signal calls get
 * wrapped with the SIGCAST macro, which does the Right Thing for your local
 * lint library.
 */
#ifdef VOIDCATCH
#define catch_t	void
#else
#define catch_t	int
#endif /* VOIDCATCH */
#define SIGCAST(x)	((catch_t (*)()) x)

/* another cultural difference */
#if defined(SIGCHLD) && !defined(SIGCLD)
#define SIGCLD	SIGCHLD
#endif /* SIGCHILD */

#if defined(LOCKF) || defined(LOCKING)	/* see xlockf.c in libport.a */
extern int lockf();
#ifndef LOCKF
#define LOCKF
#endif /* LOCKF */
#endif /* LOCKF */

#ifdef VMS
#define link(a,b)	vmslink(a,b)
#define unlink(a,b)	vmsunlink(a,b)
#endif /* VMS */

/* pacify lint about argument type differences in wait(2) */
#ifdef BSD4_2
#include <sys/wait.h>
typedef union wait wait_t;
#else
typedef union {int w_status;} wait_t;
#endif
#define WAIT_STATUS(v)	  (((v).w_status >> 8) & 0xff)	/* status of child */
#define WAIT_SIGNALLED(v) ((v).w_status & 0xff)		/* sig that hit it */
#define WAIT_STOPPED(v)	  (((v).w_status & 0xff)==0x7f)	/* was it stopped? */

/* map portable srand() and rand() calls onto the best local facilities */
#ifdef DRAND48
extern long	lrand48();
extern void	srand48();
#define rand()		(int) lrand48()
#define srand(n)	srand48(n)
typedef long	seed_t;
#else /* !DRAND */
typedef iolen_t	seed_t;
#ifdef BSD4_2
extern long random();
extern int srandom();
#define rand()		(int)random()
#define srand(n)	srandom(n)
#else
extern int	rand();
extern void	srand();
#endif /* BSD4_2 */
#endif /* DRAND48 */

#ifdef GETCWD
char *getcwd();
#else
#define getcwd(b, s)	backquote("pwd\\c", (b), (s))
#endif /* GETCWD */

#include <unistd.h>

/* UNIX services (they live in portlib.a if your version doesn't have them) */
extern	char	*strchr(), *strrchr();
extern	char	*strcpy(), *strncpy(), *strcat(), *strncat();
extern	char	*strpbrk(), *strtok();
extern	char	*memcpy(), *memset();
extern	char	*gets(), *ctime(), *mktemp(), *getenv(), *asctime();
extern	char	*malloc(), *calloc(), *realloc();
extern	time_t	time();
extern	long	atol(), lseek();
extern	struct	tm *localtime(), *gmtime();
extern	struct	passwd *getpwnam(), *getpwuid(), *getpwent();
extern	struct	group *getgrnam(), *getgrent();
extern	int	open(), close(), stat(), fstat(), rename(), mkdir(), rmdir();
extern	int	errno;
extern	FILE	*popen();
extern	iolen_t	alarm(), sleep();
#ifdef SYSV3
extern	unsigned short	getuid(), geteuid(), getgid(), getegid();
#else
#if BSD4_3
extern uid_t getuid(), geteuid();
extern gid_t getgid(), getegid();
#endif
#endif /* !SYSV3 */
extern	struct dirent *readdir(); 

/*
 * This conditional deals with places like Pyramid OSx's ucb universe,
 * which supports void in the compiler but has BSD lint libraries that
 * don't use it. This way lint sees what it wants to see.
 */
#ifndef USG
extern	int
#else
extern	void
#endif
	free(), perror(), qsort(), exit(), _exit(),
        setgrent(), endgrent(), setpwent(), endpwent();

/* system.h ends here */
