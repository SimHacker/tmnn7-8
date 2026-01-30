/* mkdirtest.c -- test mkdir and rmdir functions */
#include <stdio.h>
#include <errno.h>

main(argc, argv)
int	argc;
char	*argv[];
{
    int dstat, perms;
    extern int errno;

    if (strcmp(argv[1], "-d") == 0)
    {
	if ((dstat = rmdir(argv[2])) != 0)
	    (void) fprintf(stderr,
		    "mkdirtest: cannot rmdir %s: errno = %d\n",argv[1],errno);
	exit(dstat);
    }

    /* give user a chance to set the umask before attempting a mkdir */
    if (strncmp(argv[1], "-u", 2) == 0)
    {
	(void) sscanf(argv[1] + 2, "%o", &perms);
	(void) fprintf(stderr, "mkdirtest: setting umask to %o\n", perms);
	(void) umask(perms);
	++argv; --argc;
    }

    if (argc > 2)
	(void) sscanf(argv[2], "%o", &perms);
    else
	perms = 0777;

    if (mkdir(argv[1], perms) == -1)
	(void) fprintf(stderr, 
		"mkdirtest: cannot mkdir %s: errno = %d\n", argv[1], errno);
}

/* mkdirtest.c ends here */

