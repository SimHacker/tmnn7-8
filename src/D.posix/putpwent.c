/*
 * putpwent.c -- write a password file entry
 *
 * Written by reading the System V Interface Definition, not the code.
 *
 * Totally public domain.
 */
/*LINTLIBRARY*/
#include <stdio.h>
#include <pwd.h>

int putpwent(pw, fp)
struct passwd *pw;
FILE *fp;
{
    (void) fprintf(fp, "%s:%s", pw->pw_name, pw->pw_passwd);
#ifdef USG
    if (pw->pw_age[0] != '\0')
	(void) fprintf(fp, ",%s", pw->pw_age);
#endif /* USG */
    (void) fprintf(fp, ":%u:%u:%s:%s:%s",
		   pw->pw_uid,
		   pw->pw_gid,
		   pw->pw_gecos,
		   pw->pw_dir,
		   pw->pw_shell);
    (void) putc('\n', fp);
    return(ferror(fp));
}

/* putpwent.c ends here */
