/* strindex.c -- returns location of tx in sx */

int strindex(sx, tx)
char *sx, *tx;
{
    int i, n;

    n = strlen(tx);
    for (i = 0; sx[i] != '\0'; i++)
	if (strncmp(sx + i, tx, n) == 0)
	    return i;
    return(-1);
}

/* strindex.c ends here */

