/* bzero.c -- zero out a block in memory */
/*LINTLIBRARY*/
#include "system.h"

#ifndef bzero
void bzero(cp, n)
register char *cp;
register int n;
{
    do {
	*cp++ = 0;
    } while (--n);
}
#endif /* bzero */

/**************************************************************************

If you happen to be using a PDP-11, you can use this assembler version...

/ BZERO(MEM, N)  CHAR *MEM;
/
/	Clear "n" bytes of memory starting at "mem".
/
.globl	_bzero
.globl	csav, cret

.text
_bzero:	jsr	r0, csav	/ save registers
	mov	4(r5), r0	/ get memory pointer...
	mov	6(r5), r1	/ ...and count
	beq	ret		/ return if zero
	bit	$1, r0		/ is address odd?
	beq	1f		/ yes...
	  clrb	  (r0)+		/   clear one byte to make it even
	  dec	  r1		/   and adjust byte count
1:	mov	r1, r3		/ save (low bit of) count
	clr	r2		/ get zero into a register and clear carry
	ror	r1		/ convert count to words.
	asr	r1		/ convert count to double words
	bcc	1f		/ if word count was odd
	  mov	  r2, (r0)+	/   clear an extra word
	  tst	  r1		/   reset condition codes on r1
1:	beq	3f		/ while r1 ~= 0
2:	  mov	  r2, (r0)+	/   clear a word...
	  mov	  r2, (r0)+	/
	  sob	  r1, 2b	/   and update count
3:	asr	r3		/ if count was odd
	bcc	ret		/
	  movb	  r2, (r0)+	/   clear an extra byte
ret:	jmp	cret		/ return

If you happen to be a 3b2, 3b5, 3b10, or 3b15 (but not a 3b1!) you can use this

	.file	"bzero.u3b"	# assembler wants this

# This code relies on the fact that bzero(mem, n) is equivalent to
# strncpy("", mem, n).

	.globl	bzero
	.align	4
bzero:	save	&1		# set up stack frame and save r8
	movw	0(%ap), %r0	# get address
	movw	4(%ap), %r1	# and count
	movw	&0, %r2		# various parameters are zero
	movaw	zero, %r8	# null string
	movccep	%r8,%r1,%r0,%r2,%r2	# strncpy insruction
	ret	&1		# return

	.data
zero:	.byte	0
**************************************************************************/
/* bzero.h ends here */
