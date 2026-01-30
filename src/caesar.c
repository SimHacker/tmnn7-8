/*
 * program to to decrypt caesar(tm) cypher
 * (caesar is a trademark of the roman empire)
 *
 * to compile:
 *
 *	cc decrypt.c -lm -o decrypt.c
 *
 * usage:
 *
 *	decrypt [n] < file
 *
 * where n is an optional forced rotation.
 *
 * authors: Stan King, John Eldridge, based on algorithm suggested by
 *		Bob Morris; 29-Sep-82
 *
 * Recoded for ASCII-independence by Eric Raymond (eric@snark.uu.net)
 *
 * This software is Copyright (C) 1989 by Eric S. Raymond for the sole
 * purpose of protecting free redistribution, see the LICENSE file for
 * details.
 */
/*LINTLIBRARY*/
#include "system.h"
#include <ctype.h>
#include <math.h>

#ifdef ASCII
#define lcaesar(ch, n)	((ch) - 'a' + (n)) % 26 + 'a'
#define ucaesar(ch, n)	((ch) - 'A' + (n)) % 26 + 'A'
#define iupper(ch)	((ch) - 'A')
#define ilower(ch)	((ch) - 'a')
#else
static char *miniscule = "abcdefghijklmnopqrstuvxyzabcdefghijklmnopqrstuvxyz";
static char *majuscule = "ABCDEFGHIJKLMNOPQRSTUVXYZABCDEFGHIJKLMNOPQRSTUVWYZ";
#define lcaesar(ch, n)	*(strchr(miniscule, (ch)) + n)
#define ucaesar(ch, n)	*(strchr(majuscule, (ch)) + n)
#define iupper(ch)	(strchr(majuscule, (ch)) - majuscule)
#define ilower(ch)	(strchr(miniscule, (ch)) - miniscule)
#endif /* ASCII */

main(argc, argv)
int argc;
char *argv[];
{
	/* letter frequencies (taken from some unix(tm) documentation) */
	/* (unix is a trademark of Bell Laboratories) */
	static double stdf[ 26 ] =
	{
		7.97, 1.35, 3.61, 4.78, 12.37, 2.01, 1.46, 4.49,
		6.39, 0.04, 0.42, 3.81, 2.69, 5.92, 6.96, 2.91,
		0.08, 6.63, 8.77, 9.68, 2.62, 0.81, 1.88, 0.23,
		2.07, 0.06,
	};
	int obs[26];
	int bufsize;
	int c, i, try;
	double dot, winnerdot;  /* .. */
	int winner, forced = 0;
	char *inbuf;

	bufsize = 0;
	if( argc > 1 )
		(void) sscanf( argv[1], "%d", &forced );
	if( forced == 0 )
		forced = -1000;
		
	inbuf = calloc( BUFSIZ, 1 );

	/* adjust frequency table to weight low probs REAL low */
	for (i=0; i<26; i++)	{
		stdf[i] = log(stdf[i]) + log(26.0/100.0);
	}

	/* Decode each line separately */
	for (;;) {
		for (i=0; i<=25; obs[i++]=0)
			continue;

		/* get a sample of the text */
		for( i = 0; i < BUFSIZ; i++ ) {
			if( (c = getchar()) == EOF ) {
				exit(0);
			}	
			inbuf[i] = c;
			if (c == '\n') {
				bufsize = i+1; 
				break;
			}
			if (islower(c))
				obs[ilower(c)] += 1;
			else if (isupper(c))
				obs[iupper(c)] += 1;
		}

		/* now "dot" the freqs with the observed letter freqs */
		/*	and keep track of best fit */
		winner = 0;	
		for (try = 0; try<26; try+=13) {
			dot = 0;
			for ( i=0; i<26; i++ ) {
				dot += obs[i] * stdf[ (i+try) % 26 ];
				}
			/* initialize winning score */
			if( try == 0 )
				winnerdot = dot;
			if( dot > winnerdot ) {
				/* got a new winner! */
				winner = try;
				winnerdot = dot;
			}
		}

		if (forced != -1000)
			winner = forced;

		/* print out sample buffer */
		for( i = 0; i < bufsize; i++ )
			(void) putchar( rotate( inbuf[i], winner ) );
	}
 }


static int
rotate( c, perm )
char c;
int perm;
{
	if (isupper(c))
		return(ucaesar(c, perm));
	else if (islower(c)) {
		return(lcaesar(c, perm));
	}
	else return c;
}

/* caesar.c ends here */
