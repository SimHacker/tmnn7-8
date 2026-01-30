/****************************************************************************

NAME
   newsinit.c -- initialization and least-common-denominator code

SYNOPSIS
   void newsinit()  			-- initialize the universe

   char *newsattr(key, default)		-- get news system attributes
   char *key, default;

DESCRIPTION
   The newsinit() function initializes the strings used for USENET
filenames. The newsattr() functions enables other modules to get at them.

REVISED BY
   Eric S. Raymond
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

*****************************************************************************/
/*LINTLIBRARY*/
#include "news.h"
#include "header.h"
#ifdef RUNTIME
#include "dballoc.h"
#include "alist.h"
#endif /* RUNTIME */

int	sigcaught;		/* hold number of last signal	*/
int	uid, gid;		/* real user/group I.D.		*/
int	duid, dgid;		/* effective user/group I.D.	*/
char	bfr[LBUFLEN];		/* general-use scratch buffer	*/
char	*username, *userhome;	/* data on the executing user	*/
bool	privileged;		/* are we the administrator?	*/
siteblk	site;			/* site parameter list		*/
int	verbose;		/* -v option verbosity level	*/
#ifdef DEBUG
int	debug;			/* -D options' debug switch	*/
#endif /* DEBUG */
#ifdef RUNTIME
ALIST(attributes, 20, 10)	/* news system attributes list	*/
#endif /* RUNTIME */

#ifdef lint
/* we do this to not have to include YACC-generated code from getdate.c */
/*ARGSUSED1*/
time_t getdate(p, now) char *p; struct timeb *now; {return((time_t)p[0]);}
#endif /* lint */

#ifdef RUNTIME
char *newsattr(key, vdefault)
char	*key;		/* the keyword to look for */
char	*vdefault;	/* value to use if we don't find it */
{
    char	*value;

    if (attributes.nextfree
		&& (value = afind(&attributes, key)) != (char*)NULL
		&& value[0] != '\0')
	return(value);
    else
	return(vdefault);
}
#endif /* RUNTIME */

void newsinit()
/* initialize netnews access to the most global things */
{
    struct passwd	    *pw;	/* struct for pw lookup */
    static struct utsname   ubuf;	/* for getting our node name */

    uid = getuid();
    gid = getgid();
    duid = geteuid();
    dgid = getegid();

#ifdef UNIXPC
    tzset();	/* fixes a crock in the 7300/3B1 shared libraries */
#endif

    /* get system name; expire and postnews don't need this, but so what */
#ifndef lint
    (void) uname(&ubuf);	    /* grrr...stupid lint */
#endif /* lint */

#ifdef RUNTIME
    /*
     * Try to read a runtime configuration file from the news user's home
     * directory. If it's not there we'll just use compiled defaults.
     */
    if ((pw = getpwnam(NEWSUSR)) == (struct passwd *)NULL)
	xerror0("Cannot get news user's account data");
    (void) sprintf(bfr, "%s/attributes", pw->pw_dir);
    if (exists(bfr))
    {
	attributes.file = savestr(bfr);
	(void) dbaread(&attributes);
    }
#endif /* RUNTIME */

    /* all tools need to know the following things */
    site.notify = newsattr("notify", NOTIFY);
    site.nodename = ubuf.nodename;
    site.pathname = newsattr("pathname", PATHNAME);
    site.fromname = newsattr("fromname", FROMNAME);
#ifndef RUNTIME
    /* strings may not be writeable, but we need to bash the fromname */
    site.fromname = savestr(site.fromname);
#endif /* RUNTIME */
    lcase(site.fromname);
    site.truename = newsattr("truename", TRUENAME);
    site.smarthost = newsattr("smart-host", SMARTHOST);
    site.libdir = newsattr("libdir", LIBDIR);
    site.admdir = newsattr("admdir", ADMDIR);
    site.leastuid = newsattr("leastuid", LEASTUID);
#ifdef UUPROG
    site.uuprog = newsattr("uuprog", UUPROG);
#else
    site.uuprog = NULL;
#endif
  
    artinit();		/* initialize article-fetch code */

    /*
     * get the low-down on the real user of this program
     * for security, don't get user data from the environment
     */
    if ((pw = getpwuid(uid)) == (struct passwd *)NULL)
	xerror0("Cannot get user's name");
    username = savestr(pw->pw_name);
    userhome = savestr(pw->pw_dir);
}

/* newsinit.c ends here */
