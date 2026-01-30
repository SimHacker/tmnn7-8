/*
 *  grpw - retrieve information about a group or user
 *
 *  Grpw is a program that uses the standard getpw/getgr system
 *  routines in order to retrieve information about a group or
 *  user.  This is important for shell scripts that need information
 *  in the /etc/group or /etc/passwd file, but the information really
 *  isn't *in* those files (e.g. YellowPages).
 *
 *  usage 1: grpw -g (group|gid) format
 *
 *  If the -g flag is used, the first argument is interpreted as either
 *  a group name or a group id (if the argument is numeric, then it is
 *  a gid, otherwise it is a name).  The second argument is a printf-like
 *  format string.  It handles \n, \t, \b, and \ooo octal escapes as well 
 *  as several % escapes:
 *
 *    %g	group id
 *    %m	group names of all group members
 *    %n	group name
 *    %p	group password (encrypted)
 *
 *  usage 2: grpw -u (user|uid) format
 *
 *  If the -u flag is used, the first argument is interpreted as either
 *  a user name or a user id (if the argument is numeric, then it is
 *  a uid, otherwise it is a name).  The second argument is a printf-like
 *  format string.  It handles \n, \t, \b, and \ooo octal escapes as well 
 *  as several % escapes:
 *
 *    %c        comment field
 *    %g	group id
 *    %h	home directory
 *    %n	user name
 *    %p	user password (encrypted)
 *    %r	gecos field (usually real name)
 *    %s	login shell
 *    %u	user id
 *
 *  Author: Mark Nagel <nagel@ics.uci.edu>
 *  Modified by Eric. S. Raymond (eric@snark.uu.net) to avoid void.
 */

#include <stdio.h>
#include <ctype.h>
#include <grp.h>
#include <pwd.h>

#ifndef lint
#define void int	/* we'll never lint this on archaic C compiletrs... */
#endif /* lint*/

/*****************************************************************************/

extern struct group *getgrgid(), *getgrnam();
extern struct passwd *getpwuid(), *getpwnam();
extern void exit();

static void printgr(), printpw();

/*****************************************************************************/

#define EOS		'\0'
#define isoctal(c)	(isdigit(c) && (c) != '8' && (c) != '9')

typedef int bool;
#define true	1
#define false	0

/*****************************************************************************/

main(argc, argv)
int argc;
char *argv[];
{
  char *target, *format;
  struct group *gr;
  struct passwd *pw;
  bool dogroup;

  /* check arguments */
  if (argc != 4)
  {
    (void) fputs("usage: grpw -(g|u) (name|id)| format\n", stderr);
    exit(1);
  }
  if (strcmp(argv[1], "-g") == 0)
    dogroup = true;
  else if (strcmp(argv[1], "-u") == 0)
    dogroup = false;
  else
  {
    (void) fputs("usage: grpw -(g|u) (name|id)| format\n", stderr);
    exit(1);
  }
  target = argv[2];
  format = argv[3];

  /* determine type of target (i.e. group or gid) */
  if (dogroup)
  {
    if (isdigit(*target))
    {
      if ((gr = getgrgid(atoi(target))) != NULL)
      {
	printgr(format, gr);
	(void) fflush(stdout);
	exit(0);
      }
    }
    if ((gr = getgrnam(target)) != NULL)
    {
      printgr(format, gr);
      exit(0);
    }

    /* if we make it here, the target was bad */
    (void) fprintf(stderr, "%s: unknown group/gid\n", target);
    exit(1);
  }
  else
  {
    /* determine type of target (i.e. user or uid) */
    if (isdigit(*target))
    {
      if ((pw = getpwuid(atoi(target))) != NULL)
      {
	printpw(format, pw);
	exit(0);
      }
    }
    if ((pw = getpwnam(target)) != NULL)
    {
      printpw(format, pw);
      (void) fflush(stdout);
      exit(0);
    }

    /* if we make it here, the target was bad */
    (void) fprintf(stderr, "%s: unknown user/uid\n", target);
    exit(1);
  }

  exit(0);
  /*NOTREACHED*/
}

/*****************************************************************************/

static void
printgr(format, gr)
  register char *format;
  struct group *gr;
{
  if (format == NULL)
  {
    (void) fprintf(stderr, "internal error: format == NULL\n");
    exit(2);
  }

  while (*format != EOS)
  {
    if (*format == '%')
    {
      switch (*++format)
      {
	case 'g':		/* group id (gid) */
	  (void) printf("%d", gr->gr_gid);
	  break;
	
	case 'm':		/* group members */
	  {
	    char **mem = gr->gr_mem;

	    while (*mem != NULL)
	      (void) printf("%s ", *(mem++));
	  }
	  break;

	case 'n':		/* group name */
	  (void) printf("%s", gr->gr_name);
	  break;
	
	case 'p':		/* password (encrypted) */
	  (void) printf("%s", gr->gr_passwd);
	  break;
	
	case '%':
	  (void) putchar('%');
	
	default:
	  (void) fprintf(stderr, "grpw: illegal format character '%c'\n", 
			 *format);
	  exit(1);
      }
      format++;
    }
    else if (*format == '\\')
    {
      switch (*++format)
      {
	case '0': case '1': case '2': case '3':
	case '4': case '5': case '6': case '7':
	  {
	    int octal = *format - '0';

	    while ((8*octal <= 0370) && (*++format != EOS) && isoctal(*format))
	    {
	      octal *= 8;
	      octal += *format - '0';
	    }
	    (void) putchar(octal);
	  }
	  break;

	case 'b':
	  (void) putchar('\b');
	  break;
	
	case 'n':
	  (void) putchar('\n');
	  break;
	
	case 't':
	  (void) putchar('\t');
	  break;
	
	default:
	  (void) putchar(*format);
	  break;
      }
      if (*format != EOS)
	format++;
    }
    else
    {
      (void) putchar(*format++);
    }
  }
}

/*****************************************************************************/

static void
printpw(format, pw)
  register char *format;
  struct passwd *pw;
{
  if (format == NULL)
  {
    (void) fprintf(stderr, "internal error: format == NULL\n");
    exit(2);
  }

  while (*format != EOS)
  {
    if (*format == '%')
    {
      switch (*++format)
      {
	case 'c':		/* comment */
	  (void) printf("%s", pw->pw_comment);
	  break;
	
	case 'g':		/* group id (gid) */
	  (void) printf("%d", pw->pw_gid);
	  break;
	
	case 'h':		/* home directory */
	  (void) printf("%s", pw->pw_dir);
	  break;

	case 'n':		/* login name */
	  (void) printf("%s", pw->pw_name);
	  break;
	
	case 'p':		/* password (encrypted) */
	  (void) printf("%s", pw->pw_passwd);
	  break;
	
	case 'r':		/* "real" name (gecos) */
	  (void) printf("%s", pw->pw_gecos);
	  break;

	case 's':		/* login shell */
	  (void) printf("%s", pw->pw_shell);
	  break;

	case 'u':		/* user id (uid) */
	  (void) printf("%d", pw->pw_uid);
	  break;
	
	case '%':
	  (void) putchar('%');
	
	default:
	  (void) fprintf(stderr, "grpw: illegal format character '%c'\n", 
			 *format);
	  exit(1);
      }
      format++;
    }
    else if (*format == '\\')
    {
      switch (*++format)
      {
	case '0': case '1': case '2': case '3':
	case '4': case '5': case '6': case '7':
	  {
	    int octal = *format - '0';

	    while ((8*octal <= 0370) && (*++format != EOS) && isoctal(*format))
	    {
	      octal *= 8;
	      octal += *format - '0';
	    }
	    (void) putchar(octal);
	  }
	  break;

	case 'b':
	  (void) putchar('\b');
	  break;
	
	case 'n':
	  (void) putchar('\n');
	  break;
	
	case 't':
	  (void) putchar('\t');
	  break;
	
	default:
	  (void) putchar(*format);
	  break;
      }
      if (*format != EOS)
	format++;
    }
    else
    {
      (void) putchar(*format++);
    }
  }
}

/*****************************************************************************/
