/*
 * aligntest --- test to see if longs have to be aligned or not.
 *
 * Run this program to see whether a long can be stored and retrieved
 * at any arbitrary address, or just at certain multiples of some
 * power of two (usually 4).
 *
 * Written by Theodore Ts'o  (tytso@athena.mit.edu), 6/7/89
 */

#include <stdio.h>
#include <sys/types.h>
#include <signal.h>

#ifdef BSD
#define memcpy(dst, src, len) bcopy(src, dst, len)
typedef int catch_t;
#else
typedef void catch_t;
#endif

/* Type of return value for signal handlers */

static int	test = 0;

#define LONGPROBS "Your system has long alignment problems (test %d)\n"
#define ANSWER "\nYou should probably answer %s to LONG_ALIGN.\n"

catch_t catcher(sig)
int	sig;
{
    (void) printf("Signal %d caught!\n", sig);
    (void) printf(LONGPROBS, test);
#ifdef ANSWER
    (void) printf(ANSWER, "YES");
#endif
    exit(1);
}

main (argc, argv)
int argc;
char **argv;
{
    long	l;
    char	buf[32], *cp;
    int	i;

    /*
     * Catch possible signals that may result from alignment pointers
     * getting messed up.
     *
     * May need to be changed for SYS V machines; if so please add
     * the #ifdefs and let me know.  --- TYT 6/7/89
     */
    signal(SIGBUS, catcher);
    signal(SIGSEGV, catcher);
    signal(SIGILL, catcher);
	
    l = 0x12345678;

    *((long *) buf) = l;
    test++;

    /*
     * While we're at it, try to determine byte order storage
     */
    if (buf[0] == 0x12)
	(void) printf("Your system stores longs MSB first.\n\n");
    else if (buf[0] == 0x78)
	(void) printf("Your system stores longs LSB first.\n\n");
    else
    {
	(void) printf("Your system stores longs very strangely!\n");
	(void) printf("I can't figure it out!\n\n");
    }

    /*
     * Now, rotate the starting position of the long through 16 bytes,
     * just to be sure.
     */
    cp = buf;
    for (i = 0; i < 16; i++) {
	/*
	 * First, try to store in arbitary place
	 */
	*((long *) (++cp)) = l;
	test++;
		
	if (l != *((long *) cp)) {
	    (void) printf(LONGPROBS, test);
#ifdef ANSWER
	    (void) printf(ANSWER, "YES");
#endif
	    exit(1);
	}
	test++;

	/*
	 * Now try copying it in place with memcpy
	 */
	(void) memcpy(cp, &l, sizeof(l));
		
	if (l != *((long *) cp))
	{
	    (void) printf(LONGPROBS, test);
#ifdef ANSWER
	    (void) printf(ANSWER, "YES");
#endif
	    exit(1);
	}
	test++;
    }

    (void) printf("It appears your system can handle long word assignments\n");
    (void) printf("without worrying about alignment problems.\n");
#ifdef ANSWER
    (void) printf(ANSWER, "NO");
#endif
	
    exit(0);
	
}

/* aligntest.c ends here */
