/****************************************************************************

NAME
   sevenbit.c -- filter functions for communicating across seven-bit lines

SYNOPSIS
   void encode(ifp, ofp)    -- encode input stream to output stream
   FILE *ifp; *ofp;

   void decode(ifp, ofp)    -- decode input stream to output stream
   FILE *ifp; *ofp;

DESCRIPTION
   These routines enable transmission and reception of 8-bit data (such
as binaries or compressed text) over 7-bit lines. If MAIN is defined a
standalone test version is set up.

REVISED BY
   Eric S. Raymond
Adapted with interface-only changes from the old encode and decode
utilities; the protocol is the same.
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

****************************************************************************/
/* LINTLIBRARY */
#include "libport.h"

#ifndef private
#define private static
#endif /* private */

/* common storage for everyone */
private int cnt = 0;

/*
 * Produce a 7 bit printable encoding of stdin on stdout.
 *
 * Encoding uses ASCII chars from ' ' .. 'z'
 * (040 .. 0172) (0x20 - 0x7a) inclusive
 *
 * Method is to expand 3 chars -> 4 6 bit ones.
 * Then collect 13 6 bit chars, and spread the 13th over
 * the preceding 12, so that each of the 12 chars is now
 * 6.5 bits.  These 2 6.5 bit chars are a little hard
 * to represent on most common machines (one of these days
 * sane hosts will have 1/2 bits just for this program)
 * so we take a pair of them, and represent that in 13 bits.
 * 13 bits (max value 8191) can be represented as
 *	A * 91 + B
 * where A < 91, B < 91  (91^2 == 8281, so it fits!)
 *
 * Each of A and B is encoded as a character by adding 32
 * to make it printable (ie: 0x20).
 *
 * The termination conditions are foul beyond belief.  Don't
 * monkey with them!
 *
 * If you think its a fluke that 040 .. 0171 just happen to
 * be the chars that Piet Beertema's uucp 'f' protocol transmits
 * as single bytes, you're insane.  0172 chars are produced
 * with lower frequency than any other (given random data)
 * so the doubling that occurs with that we will just suffer.
 * (A newer 'f' proto, sometime, will probably not use 0172)
 */

/*
 * the following pair of characters cannot legally occur
 * in normal output (since 90*91 + 90 == 8280, which > 2^13)
 * so we use them to indicate that the data that follows is the
 * terminator.  The character immediately following this
 * pair is the length of the (expanded) terminator (which
 * otherwise might be indeterminable)
 */
#define	ENDMARK1	((90*91 + 90) / 91 + ' ')
#define	ENDMARK2	((90*91 + 90) % 91 + ' ')

void encode(ifp, ofp)
FILE *ifp, *ofp;
{
    register char *p;
    register char *e;
    register int c;
    char b3[3];
    void tocode(), flushout();

    p = b3;
    e = b3 + 3;
    while ((c = fgetc(ifp)) != EOF) {
	*p++ = c;
	if (p == e) {
	    tocode(b3, 3, ofp);
	    p = b3;
	}
    }
    tocode(b3, p - b3, ofp);
    flushout(ofp);

#ifndef lint
    exit(0);
#endif				/* lint */
}

private char b13[13];

private void tocode(c, n, ofp)
register char *c;
register FILE *ofp;
int n;
{
    register char *p;
    register int i = cnt;
    register int j;
    char b4[4];
    void dumpcode();

    p = b4;

    p[0] = (c[0] >> 2) & 0x3f;
    p[1] = ((c[0] & 0x3) << 4) | ((c[1] >> 4) & 0xf);
    p[2] = ((c[1] & 0xF) << 2) | ((c[2] >> 6) & 0x3);
    if (n == 3)
	p[3] = c[2] & 0x3f;
    else
	p[3] = n;

    c = &b13[i];
    for (j = 4; --j >= 0; i++) {
	if (i == 13) {
	    dumpcode(b13, 13, ofp);
	    c = b13;
	    i = 0;
	}
	*c++ = *p++;
    }
    cnt = i;
}

private void flushout(ofp)
FILE	*ofp;
{
    void dumpcode();

    (void) putc(ENDMARK1, ofp);
    (void) putc(ENDMARK2, ofp);
    (void) putc(cnt + ' ', ofp);
    dumpcode(b13, cnt, ofp);
}

private void dumpcode(p, n, ofp)
register char *p;
register int n;
FILE	*ofp;
{
    register int last;
    register int c;

    if (n == 13)
	n--, last = p[12];
    else if (n & 1)
	last = (1 << (6-1));
    else
	last = 0;

    for ( ; n > 0; n -= 2) {
	c = *p++ << 6;
	c |= *p++;
	if (last & (1 << (6-1)))
	    c |= (1 << 12);
	last <<= 1;

	/*
	 * note: 91^2 > 2^13, 90^2 < 2^13, (91 + ' ') is printable
	 */

	/* oh for a compiler that would only do one division... */
	(void) putc((c / 91) + ' ', ofp);
	(void) putc((c % 91) + ' ', ofp);
    }
}

/*
 * This function is the inverse of encode
 *
 * It collects runs of 12 characters, combines pairs of those
 * to form 6 13 bit numbers, extracts the top bit of each of
 * those to make a 13th 6 bit character, and splits each of
 * the remaining 6 12 bit numbers to form 12 6 bit ones.
 *
 * The strings of 6 bit numbers are collected into groups of
 * 4 and converted into 3 8 bit characters.
 *
 * Now all that would be trivial, if we didn't need to worry
 * about ending all this correctly.  About 1/2 of the following
 * code wouldn't be here if the ending didn't matter....
 */

void decode(ifp, ofp)
FILE *ifp, *ofp;
{
    register int c;
    register char *p;
    register int i;
    register int first = 1;
    register int dcnt = 0;
    int errcnt = 0;
    char b12[12];
    char c12[12];
    void pack12();

    p = b12;
    i = 12;

    while ((c = fgetc(ifp)) != EOF) {
	if (c < ' ' || c >= (' ' + 91)) {
	    if (errcnt++ == 0)
		(void) fprintf(stderr, "decode: Bad data\n");
	    continue;
	}
	if (i == 10 && p[-1] == ENDMARK1 && p[-2] == ENDMARK2) {
	    dcnt = c - ' ';
	    i = 12;
	    p -= 2;
	    continue;
	}
	*p++ = c - ' ';
	if (--i == 0) {
	    if (p == &b12[12]) {
		if (!first)
		    pack12(c12, 12, 0, ofp);
		else
		    first = 0;
		p = c12;
	    } else {
		pack12(b12, 12, 0, ofp);
		p = b12;
	    }
	    i = 12;
	}
    }

    if (p >= &b12[0] && p < &b12[12]) {
	if (!first)
	    pack12(c12, 12, i == 12 ? dcnt : 0, ofp);
    } else
	pack12(b12, 12, i == 12 ? dcnt : 0, ofp);

    if (i != 12) {
	if (p >= &b12[0] && p < &b12[12])
	    pack12(b12, 12-i, dcnt, ofp);
	else
	    pack12(c12, 12-i, dcnt, ofp);
    }

#ifndef lint
    exit(0);
#endif				/* lint */
}

private char b4[4];

private void pack12(p, n, last, ofp)
register char *p;
register int n;
int last;
FILE *ofp;
{
    register int i;
    register char *q;
    char pb13[13];
    void pack6bit();

    {
	register int c;
	register int c13;

	q = pb13;
	c13 = 0;

	for (i = 0; i < n; i += 2) {
	    c = *p++ * 91;
	    c += *p++;
	    c13 <<= 1;
	    if (c & (1 << 12))
		c13 |= 1;
	    *q++ = (c >> 6) & 0x3f;
	    *q++ = c & 0x3f;
	}
	*q++ = c13;
	if (last)
	    q = &pb13[last];
    }

    p = pb13;
    n = q - p;
    i = cnt;
    q = &b4[cnt];

    while (--n > 0) {
	*q++ = *p++;
	if (++i == 4) {
	    char b3[3];
	    register char *b = b4;

	    /* inline expansion of pack6bit, to save calls ... */

	    q = b3;
	    *q++ = (b[0] << 2) | ((b[1] >> 4) & 0x3);
	    *q++ = (b[1] << 4) | ((b[2] >> 2) & 0xf);
	    *q = (b[2] << 6) | (b[3] & 0x3f);

	    q = b3;
	    while (--i > 0)
		(void) putc(*q++, ofp);

	    q = b4;
	}
    }

    *q++ = *p++;		/* the last octet */
    ++i;

    if (last || i == 4) {
	pack6bit(b4, i, last, ofp);
	i = 0;
    }

    cnt = i;
}

private void pack6bit(p, n, last, ofp)
register char *p;
register int n;
int last;
FILE *ofp;
{
    register char *q;
    register int i = 3;
    char b3[3];

    if (last) {
	i = p[n-1];
	if (i >= 3) {
	    (void) fprintf(stderr, "Badly encoded file\n");
	    i = 3;		/* do the best we can */
	}
    }

    q = b3;
    *q++ = (p[0] << 2) | ((p[1] >> 4) & 0x3);
    *q++ = (p[1] << 4) | ((p[2] >> 2) & 0xf);
    *q = (p[2] << 6) | (p[3] & 0x3f);

    q = b3;

    while (--i >= 0)
	(void) putc(*q++, ofp);
}

#ifdef MAIN
main(argc, argv)
int argc;
char *argv[];
{
    if (strcmp(argv[1], "-d") == 0)
	decode(stdin, stdout);
    else
	encode(stdin, stdout);
}
#endif /* MAIN */

/* sevenbit.c ends here */
