/*****************************************************************************

NAME
   makekits -- make distribution kits from a collection of files

SYNOPSIS
   makekits [-v] [-c] [-m maxsize] [-p prefix]

DESCRIPTION
   This is the distribution kit maker. See its manual page for details.

AUTHOR
   Eric S. Raymond
Some of this code was lifted from Larry Wall's kitlists program.
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

*****************************************************************************/
/*LINTLIBRARY*/
#include "libport.h"

#ifndef private
#define private static
#endif /* private */

#define MAXKIT		100	/* maximum possible number of kits */
#define MAXKITSIZE	63000	/* default, can be changed with -m option */
#define KITOVERHEAD	620	/* actually, 611 + extra digits */
#define FILEOVERHEAD	90	/* this is pessimistic, it's usually less */
#define FNAMELEN	128	/* maximum length of filename we can handle */
#define BUFLEN		1024

extern int errno;

private long	maxsize = MAXKITSIZE;	/* maximum kit size */
private long	tot[MAXKIT];		/* track sizes of kits */
private FILE	*outfp[MAXKIT];		/* of course, not this many fds */
private int	maxkit = 1;		/* maximum kit number */
private char	filnam[FNAMELEN];	/* common filename buffer */
private char	*kprefix = "kit";	/* kit filename, prefix */

private catch_t cleanup(signo)
/* chuck out all intermediate files */
int	signo;
{
    int	    i;

    for (i = 1; i <= maxkit; i++)
    {
	(void) sprintf(filnam, "%s%d.list", kprefix, i);
	(void) unlink(filnam);
    }

    exit(signo != 0);
}

/*ARGSUSED*/
main(argc,argv)
int argc;
char **argv;
{
    FILE *inp, *outp;
    char buf[BUFLEN], kitnam[FNAMELEN];
    char *opts, *compress = (char *)NULL;
    register int i, vflag = 0;
    long newtot;
    struct stat sbuf;

    /* process command-line options */
    while (*++argv)
	if (argv[0][0] != '-')
	{
	    (void) fprintf(stderr,
	"usage: makekits [-v] [-m size] [-p prefix] [-c compressor]\n");
#ifndef lint
	    exit(1);
#endif
	}
	else
	{
	    opts = argv[0];
	    for (i = 0; i < strlen(opts); i++)
		switch (opts[i])
		{
		case 'v': vflag++; break;
		case 'p': kprefix = (++argv)[0]; break;
		case 'c': compress = (++argv)[0]; break;
		case 'm': maxsize = atol((++argv)[0]); break;
		}
	}

    if (vflag)
	(void) fprintf(stderr,
		       "makekits: maximum kit size is %ld\n",
		       (long)maxsize);

    (void) signal(SIGINT, cleanup);
    (void) signal(SIGHUP, cleanup);
    (void) signal(SIGTERM, cleanup);

    /* now generate the kit lists for each kit */
    while (fgets(buf, BUFLEN, stdin) != (char *)NULL)
    {
	char	*cp = buf;
	int	minkit = 1;

	/* if the line is blank or begins with whitespace, it's a comment */
	if (isspace(buf[0]))
	    continue;

	/* ignore everything after the first field */
	while (*cp && !isspace(*cp))
	    cp++;
	*++cp = '\0';

	buf[strlen(buf) - 1] = '\0';
	(void) stat(buf, &sbuf);

	/* if the filename is a directory, mark it for later creation */
	if (sbuf.st_mode & S_IFDIR)
	{
	    (void) strcat(buf, "/");
	    sbuf.st_size = 0;	    /* it will take up negligible space */
	}

	/* figure out which kit the named file should join */
	for (i = minkit; i < MAXKIT; i++)
	{
	    newtot = tot[i] + sbuf.st_size + FILEOVERHEAD;
	    if (maxsize == 0 || newtot <= maxsize - KITOVERHEAD || !tot[i])
		break;
	}

	if (!tot[i])	/* we've run out of space in existing kits */
	{
#ifdef _NFILE
	    /* here's where we make sure not to run out of descriptors */
	    if (maxkit - minkit + 1 >= _NFILE - 4)
		(void) fclose(outfp[minkit++]);
#endif /* NFILE */

	    (void) sprintf(filnam, "%s%d.list", kprefix, i);
	    if ((outfp[maxkit = i] = fopen(filnam, "w")) == (FILE *)NULL)
	    {
		(void) fprintf(stderr,
			    "makekits: can't open kit %d, errno = %d\n",
			    i, errno);
#ifndef lint
		exit(1);
#endif
	    }
	}

	tot[i] = newtot;
	if (vflag)
	    (void) fprintf(stderr,
			"makekits: adding %s to kit %d giving %d bytes\n",
			buf, i, newtot);
	(void) fprintf(outfp[i], "%s\n", buf);
    }
    (void) fclose(stdin);

    /* make sure the list files get flushed */
    for (i = 1; i <= maxkit; i++)
	(void) fclose(outfp[i]);

    /* make kits from the kit lists */
    for (i = 1; i <= maxkit; i++)
    {
	(void) fclose(outfp[i]);
	(void) sprintf(filnam, "%s%d.list", kprefix, i);
	if ((inp = fopen(filnam, "r")) == (FILE *)NULL)
	{
	    (void) fprintf(stderr, "Can't read %s\n", filnam);
#ifndef lint
	    exit(1);
#endif
	}

	if (maxkit == 1)
	    (void) sprintf(kitnam, "%s.sh", kprefix);
	else
	    (void) sprintf(kitnam, "%s%.2d.sh", kprefix, i);
	if ((outp = fopen(kitnam, "w")) == (FILE *)NULL)
	{
	    (void) fprintf(stderr, "Can't write %s\n", filnam);
#ifndef lint
	    exit(1);
#endif
	}

	if (vflag)
	    (void) fprintf(stderr, "makekits: *** starting kit %d ***\n", i);

	/* first, generate the kit header */
	(void) fprintf(outp, "# This file is in shar format\n");
	(void) fprintf(outp,
	    "echo 'This is kit %d (of %d).  If it is complete, the line'\n",
	    i, maxkit);
	(void) fprintf(outp, 
	    "echo '\"End of kit %d (of %d)\" will echo at the end.'\n\n",
	    i, maxkit);
	(void) fprintf(outp,
	    "export PATH || (echo \"You lose, you didn't use sh.\"; kill $$)\n\n");

	/* now add each file in the current list to the kit */
	while (fgets(filnam, FNAMELEN, inp))
	{
	    FILE	    *cfp;
	    int		    c, lastln;
	    struct stat	    fs;

	    filnam[strlen(filnam) - 1] = '\0';	/* trash terminating \n */

	    /* arrange for things marked as directory names to be created */
	    if (filnam[strlen(filnam) - 1] == '/')
	    {
		filnam[strlen(filnam) - 1] = '\0';
		(void) fprintf(outp,
			"if test ! -d %s; then mkdir %s; fi\n",
			filnam, filnam);
		continue;
	    }

	    /* otherwise we'll need to open the file */
	    if ((cfp = fopen(filnam, "r")) == (FILE *)NULL)
	    {
		(void) fprintf(stderr, "Couldn't get at %s!\n", filnam);
		continue;
	    }

	    (void) fprintf(stderr, "%s\n", filnam);	/* announce intent */

	    (void) fprintf(outp, "echo Extracting %s\n", filnam);

	    (void)fprintf(outp,"sed >%s <<'!STUFFY!FUNK!' -e 's/X//'\n",filnam);

	    lastln = 1;
	    while ((c = getc(cfp)) != EOF)
	    {
		if (lastln)
		    (void) putc('X', outp);
		(void) putc(c, outp);
		lastln = (c == '\n');
	    }
	    (void) fclose(cfp);
	    (void) fprintf(outp, "!STUFFY!FUNK!\n");

	    (void) stat(filnam, &fs);
	    (void) fprintf(outp, "chmod %o %s\n", fs.st_mode & 07777,filnam);
	}
	(void) fclose(inp);

	/* now concatenate the standard kit trailer */
	(void) fprintf(outp, "echo 'End of kit %d (of %d)'\n", i, maxkit);
	(void) fprintf(outp, "cat /dev/null >%s%.2d.done\n", kprefix, i);
	(void) fprintf(outp, "config=true\n");
	(void) fprintf(outp, "for kit in %s[0-9]*.sh\n", kprefix);
	(void) fprintf(outp, "do\n");
	(void) fprintf(outp, "    stem=`expr $kit : '\\(.*\\).sh'`\n");
	(void) fprintf(outp, "    if test -f ${stem}.done; then\n");
	(void) fprintf(outp, "        echo \"You have unpacked $kit.\"\n");
	(void) fprintf(outp, "    else\n");
	(void) fprintf(outp, "        echo \"$kit is still packed.\"\n");
	(void) fprintf(outp, "	      config=false\n");
	(void) fprintf(outp, "    fi\n");
	(void) fprintf(outp, "done\n");
	(void) fprintf(outp, "case $config in\n");
	(void) fprintf(outp, "    true)\n");
	(void) fprintf(outp, "        echo 'You have run all your kits.'\n");
	(void) fprintf(outp, "        echo 'See READ.ME for instructions.'\n");
	(void) fprintf(outp, "        for kit in %s*[0-9]; do\n", kprefix);
	(void) fprintf(outp, "            rm ${kit}.done\n");
	(void) fprintf(outp, "        done\n");
	(void) fprintf(outp, "    ;;\n");
	(void) fprintf(outp, "esac\n");
	(void) fflush(outp);

	/* apply a compressor if one was specified */
	if (compress)
	{
	    (void) sprintf(buf, "%s %s", compress, kitnam);
	    if (vflag)
		(void) fprintf(stderr, "makekits: performing %s\n", buf);
	    (void) system(buf);
	    (void) fflush(stdout);	/* V7 system() doesn't flush stdout */
	}
    }

    cleanup(0);
    /*NOTREACHED*/
}

/* makekits.c ends here */
