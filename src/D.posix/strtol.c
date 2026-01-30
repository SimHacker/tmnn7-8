/*
 * strtol - convert string to long integer.
 *
 * Written by reading the System V Interface Definition, not the code.
 *
 * Totally public domain.
 *
 * Compile with -DTEST to get short interactive main() for testing.
 */

#include <stdio.h>
#include <ctype.h>

long
strtol(s, p, b)
char	*s, **p;
int	b;
{
	int	base = 10, n = 0, sign = 1, valid = 1;

	/*
	 * leading sign?
	 */
	if (*s=='-')
		sign=(-1);
	else
		sign=1;
	if (*s=='+' || *s=='-')
		++s; /* skip sign */

	/*
	 * what base are we really using?
	 */
	if (b == 0) {
		if (strncmp(s, "0x", 2) == 0  ||
		    strncmp(s, "0X", 2) == 0) {
			s += 2;
			base = 16;
		} else
			if (*s == '0')
				base = 8;
	}

	/*
	 * convert the string to a number.
	 */
	while (isascii(*s) && valid) {
		switch(*s) {
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
			n = base*n  +  *s-'0';
			break;
		case '8':
		case '9':
			if (base >8)
				n = base*n  +  *s-'0';
			else
				valid = 0;
			break;
		case 'a':
		case 'b':
		case 'c':
		case 'd':
		case 'e':
		case 'f':
			if (base == 16)
				n = base*n + *s-'a'+10;
			else
				valid = 0;
			break;
		case 'A':
		case 'B':
		case 'C':
		case 'D':
		case 'E':
		case 'F':
			if (base == 16)
				n = base*n + *s-'A'+10;
			else
				valid = 0;
			break;
		default:
			valid = 0;
			break;
		}
		++s;
	}

	/*
	 * if arg `p' is not NULL, a ptr to the character
	 * terminating the scan will be returned in `p'
	 */
	if (*p != (char *)NULL)
		*p = s;

	return sign * n;
}

#ifdef	TEST
main(argc, argv)
int argc;
char **argv;
{
	int i;
	long j, strtol();

	for (i=1; i<argc; i++) {
		j = strtol(argv[i], 0, 0);
		(void) printf("%s -> %ld(%lx)\n", argv[i], j, j);
	}
	exit(0);
}
#endif	/* TEST */
