/******************************************************************************

NAME
   archnews -- news archiving to floppies for 7300 sites

SYNOPSIS
   archnews archive-directory

DESCRIPTION
   Those who are running SYS V with the Unix utilities on a 7300 with the 20M
drive know what a tight fit it is.  This utility when used in conjunction with
expire allows frequent archiving of the news to floppies while maintaining a
Subject line index which remains online.  It compresses the files with the news
compress utility in an attempt to get maximum density on the floppy (but
frequently the inodes run out before the space does). Requires POSIX-compatible
directory-handling routines.

AUTHORS
   Original by David Albrecht
   Adaptations for 3.0 distribution by Eric S. Raymond

***************************************************************************/
#define MAXSTRING 132
#define MAXDIR 60

#include "news.h"

char newsbuf[512], archdir[MAXDIR], make_dirs[8][MAXDIR], target[MAXDIR];
char in_string[MAXSTRING];

main(argc,argv)
int argc;
char **argv;
{
    char rootdir[MAXDIR], newspath[MAXDIR], newsgroup[MAXDIR];

    getcwd(rootdir, MAXDIR);
    *newspath = '\0';
    *newsgroup = '\0';
    if (argc > 1) {
	(void) strcpy(archdir, argv[1]);
    }
    else {
	(void) fprintf(stderr, "archnews archive_directory\n");
	(void) exit(1);
    }
    procdir(rootdir, newspath, newsgroup, 0);
}

procdir(dirpath, newspath, newsgroup, level)
char *dirpath, *newspath, *newsgroup;
{   DIR *directory;
    FILE *fp, *fp1, *pin, *pout;
    struct direct *dp;
    struct stat buf;
    int ldirpath, lnewspath, lnewsgroup, i, f_status, child_status, pid;
    char entryname[MAXDIR], compressname[MAXDIR], *str, c;

    if (!(directory = opendir(dirpath))) {
	(void) fprintf(stderr, "unable to open directory %s\n", dirpath);
    }
    else {
	while (dp = readdir(directory)) {

	    if (*(dp->d_name) == '.') continue;

	    *entryname= '\0';
	    strncat(entryname, dp->d_name, dp->d_namlen);
	    fp = fopen(entryname, "r");
	    if (!fp) {
		(void) fprintf(stderr, "unable to open file %s/%s\n",
				 dirpath, entryname);
		continue;
	    }

	    (void) fstat(fileno(fp), &buf);
	    if (buf.st_mode & S_IFDIR) {
		(void) fclose(fp);

		ldirpath = strlen(dirpath);
		(void) strcat(dirpath, "/");
		(void) strcat(dirpath, entryname);

		lnewspath = strlen(newspath);
		if (lnewspath) {
		    (void) strcat(newspath, "/");
		}
		(void) strcat(newspath, entryname);

		lnewsgroup = strlen(newsgroup);
		if (lnewsgroup) {
		    (void) strcat(newsgroup, ".");
		}
		(void) strcat(newsgroup, entryname);

		(void) strcpy(target, archdir);
		(void) strcat(target, "/");
		(void) strcat(target, newspath);
		fp = fopen(target, "r");
		if (!fp) {
		    (void) strcpy(&make_dir[level][0], target);
		}
		else {
		    (void) fclose(fp);
		}

		(void) chdir(entryname);

		procdir(dirpath, newspath, newsgroup, level + 1);

		make_dir[level][0] = '\0';
		dirpath[ldirpath] = '\0';
		(void) chdir(dirpath);
		newspath[lnewspath] = '\0';
		newsgroup[lnewsgroup] = '\0';
	    }
	    else {
		for (i = 0; i < level; i++) {
		    if (make_dir[i][0]) {
			if (mkdir(&make_dir[i][0], 0777)) {
			    (void) fprintf(stderr,
				"cannot create directory %s, archive full\n",
				&make_dir[i][0]);
			}
			else {
			    make_dir[i][0] = '\0';
			}
		    }
		}

		(void) strcpy(target, archdir);
		(void) strcat(target, "/");
		(void) strcat(target, newspath);
		(void) strcat(target, "/");
		(void) strcat(target, entryname);
		(void) strcat(target, ".Z");
		fp1 = fopen(target, "w");
		if (!fp1) {
		    (void) fprintf(stderr,
			"unable to create %s, archive full\n", target);
		    exit(1);
		}

		(void) sprintf(compressname, "%s/compress", LIBDIR);
		if (pipe_command(compressname, "compress", &pin, &pout)) {
		    (void) fprintf(stderr,
			"pipe to %s/compress failed\n", LIBDIR);
		}

		if (!(pid = fork())) {

		    (void) fclose(fp);
		    (void) fclose(pin);

		    c = getc(pout);
		    while (!feof(pout)) {
			if ((f_status = putc(c, fp1)) == EOF) {
			    break;
			}
			c = getc(pout);
		    }
		    (void) fclose(fp1);

		    if (f_status == EOF) {
			while (!feof(pout)) getc(pout);
			(void) fclose(pout);
			(void) exit(1);
		    }
		    else {
			(void) fclose(pout);
			(void) exit(0);
		    }
		}
		else {

		    (void) fclose(fp1);
		    (void) fclose(pout);

		    *newsbuf = '\0';
		    str = newsbuf;
		    i = 0;
		    while (fgets(in_string, MAXSTRING - 1, fp)) {
			fputs(in_string, pin);
			if (!strncmp(in_string, "Subject", 7)) {
			    (void) sprintf(str,
				"%s:%s - %s",
				newsgroup, entryname, in_string);
			    str += strlen(str);
			}
			else if (!strncmp(in_string, "Summary", 7)) {
			    (void) sprintf(str,
				"%s:%s - %s",
				newsgroup, entryname, in_string);
			    str += strlen(str);
			}
			else if (!strncmp(in_string, "Keywords", 8)) {
			    (void) sprintf(str,
				"%s:%s - %s",
				newsgroup, entryname, in_string);
			    str += strlen(str);
			}
		    }
		    (void) fclose(fp);
		    (void) fclose(pin);

		    while (wait(&child_status) != pid);
		    if (child_status >> 8) {
			(void) fprintf(stderr,
					"write to %s failed, archive full\n",
					target);
			(void) unlink(target);
		    }
		    else {
			(void) fputs(newsbuf, stdout);
			(void) unlink(entryname);
		    }
		}
	    }
	}
	(void) closedir(directory);
    }
}

pipe_command(path, command, pin, pout)
char *path, *command;
FILE **pin;
FILE **pout;
{
    int fildes1[2], fildes2[2], status = 0;

    if (pipe(fildes1)) {
	(void) fprintf(stderr, "cannot create pipes for %s command\n", path);
	status = 1;
    }
    if (pipe(fildes2)) {
	(void) fprintf(stderr, "cannot create pipes for %s command\n", path);
	status = 1;
    }
    if (!fork()) {
	(void) close(fildes1[1]);
	(void) close(fildes2[0]);
	(void) close(0);
	(void) dup(fildes1[0]);
	(void) close(fildes1[0]);
	(void) close(1);
	(void) dup(fildes2[1]);
	(void) close(fildes2[1]);
	(void) execlp(path, command, (char *) NULL);
	perror(path);
	_exit(1);
    }
    else {
	(void) close(fildes1[0]);
	(void) close(fildes2[1]);
	*pin = fdopen(fildes1[1], "w");
	*pout = fdopen(fildes2[0], "r");
	if (!(*pin) || !(*pout)) status = 1;
    }
    return(status);
}
