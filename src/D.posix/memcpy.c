/*
 * Copy first n bytes of s to t, return t
 *
 * Written by reading the System V Interface Definition, not the code.
 *
 * Totally public domain.
 *
 */
/*LINTLIBRARY*/
char *memcpy(t, s, n)
register char *t, *s;
register int n;
{
    register char *os = t;

    while (--n >= 0)
	*t++ = *s++;
    return(os);
}

/**************************************************************************

If you happen to be using a PDP-11, you can use this assembler version...

/ MEMCPY(TO, FROM, N)  CHAR *TO, *FROM;
/
/	Copy "n" bytes from "from" to "to".
/
.globl	_memcpy
.globl	csav, cret

.text
_memcpy:	jsr  r0, csav		/ save registers
	mov  r5, r4		/ get arguments...
	cmp  (r4)+, (r4)+	/
	mov  (r4)+, r1		/ to
	mov  (r4)+, r0		/ from
	mov  (r4), r2		/ byte count
	beq  ret		/ if zero then return
	bit  $1, r1		/ is "to" odd
	beq  1f			/ yes...
	  movb (r0)+, (r1)+	/   copy one byte to make it even
	  dec  r2		/   and adjust byte count
1:	mov  r2, r3		/ save (low bit of) count
	asr  r2			/ convert from bytes to words
	beq  3f			/ if any words to copy...
	  bit  $1, r0		/   is "from" even now?
	  beq  2f		/   if not ...
1:	    movb (r0)+, (r1)+	/     copy bytes...
	    movb (r0)+, (r1)+	/     ...
	    sob  r2, 1b		/     ...
	  br   3f		/   else ...
2:	    mov  (r0)+, (r1)+	/     copy words...
	    sob  r2, 2b		/     ...
3:	ror  r3			/ is byte count odd?
	bcc  ret		/ if so...
	   movb (r0)+, (r1)+	/   copy odd byte.
ret:	jmp  cret		/ return

If you happen to be a VAX, you can use this:

# memcpy(to, from, count) char *to, *from; int count;
#
# Copy "count" bytes from "from" to "to"; not guaranteed to
# work if "from" and "to" overlap.

	.align	2
	.globl	_memcpy
_memcpy:
	.word	0
	movl	4(ap), r3		# r3 = to
	movl	8(ap), r1		# r1 = from
L1:
	movzwl	$65535, r0		# while more than 65535 bytes to move
	cmpl	12(ap), r0
	jleq	L2			# if <= 65535, break
	subl2	r0, 12(ap)		# count-=65535 (bytes moved this time)
	movc3	r0, (r1), (r3)		# r1, r3 magically point to next 65K
	brb	L1
L2:
	movc3	12(ap), (r1), (r3)	# move up to 65535 bytes
	ret

If you happen to be a 3b2, 3b5, 3b10, or 3b15 (but not a 3b1!) you can use this

	.file "memcpy.s"		# silly assembler wants this line
	.globl	memcpy
	.align	4
memcpy:	save	&0		# set up stack frame
	movw	0(%ap), %r1	# get source
	movw	4(%ap), %r0	# and destination
	movw	8(%ap), %r2	# get count
	movblb			# this instruction does it all
	ret	&0		# return

**************************************************************************/
