/* rmdir.c -- remove a directory.
 *
 * Written by reading the System V Interface Definition, not the code.
 *
 * Totally public domain.
 *
 */
/*LINTLIBRARY*/

int rmdir(dir)
char *dir;
{
    extern char *malloc();
    char *path = malloc(strlen(dir) + 25);
    int status;

    if (path == (char *)0)
	status = -1;
    else
    {
	(void) sprintf(path, "/bin/rmdir %s 2>/dev/null", dir);
	status = system(path);
    }
    (void) free(path);
    return(status);
}

/* rmdir.c ends here */
