/* strpbrk.c -- return ptr to 1st instance of any char in arg 2 within arg 1
 *
 * Written by reading the System V Interface Definition, not the code.
 *
 * Totally public domain.
 */
/* LINTLIBRARY */

char *
strpbrk(str, chars)
register char *str, *chars;
{
	register char *cp;

	do {
		cp = chars - 1;
		while (*++cp) {
			if (*str == *cp)
				return str;
		}
	} while
	    (*str++);
	return (char *)0;
}

/**************************************************************************

If you happen to be a 3b2, 3b5, 3b10, or 3b15 (but not a 3b1!) you can use this

	.file	"strpbrk.u3b"	# think of this line as an offering to the gods

# char *strpbrk(s1, s2)  char *s1, *s2;
# Finds first occurance in s1 of a character in s2.

	.text
	.globl	strpbrk
	.align	4
strpbrk:
	save	&2			# save r7 and r8
	movw	0(%ap), %r0		# r0 = s1
	movw	4(%ap), %r8		# r8 = s2
	movw	&0, %r2			# string termination character
	jmp	L2
L1:					# while (*s1 != '\0') {
	  movw	  %r8, %r7		#   get s2
	  locce	  %r7, %r1, %r2		#   strchr(s2, *s1)
	  je	  L4			#   if found, return
	  addw2	  &1, %r0		#   increment s1
L2:	  movb	  0(%r0), %r1		#   get *s1
	  jne	  L1			#   loop if not at end of string
	movw	&0, %r0			# set s1 to NULL
L4:	ret	&2			# and return s1
**************************************************************************/
/* strpbrk.c ends here */
