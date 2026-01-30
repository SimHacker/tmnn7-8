/****************************************************************************

NAME
   regcomp and regexec -- compile and test for regular expression matches

SYNOPSIS
   #include "regexp.h"

   regexp *regcomp(exp)		-- compile a regular expression
   char *exp;

   int regexec(prog, string)	-- match a regular expression against a string
   regexp *prog; char *string;

DESCRIPTION
   These routines provide V8-compatible expression matching for the
news code. 

REVISED BY
   Eric S. Raymond
These are a truncated and slightly munged version of Henry Spencer's regexp(3)
package; credit him if they work OK, reserve flames for me if they don't.
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

****************************************************************************/
/*LINTLIBRARY*/
#include "libport.h"
#include "regexp.h"

#define	MAGIC	0234	/* initial magic number for compiled expressions */

/* definition	number	opnd?	meaning */
#define	END	0	/* no	End of program. */
#define	BOL	1	/* no	Match "" at beginning of line. */
#define	EOL	2	/* no	Match "" at end of line. */
#define	ANY	3	/* no	Match any one character. */
#define	ANYOF	4	/* str	Match any character in this string. */
#define	ANYBUT	5	/* str	Match any character not in this string. */
#define	BRANCH	6	/* node	Match this alternative, or the next... */
#define	BACK	7	/* no	Match "", "next" ptr points backward. */
#define	EXACTLY	8	/* str	Match this string. */
#define	NOTHING	9	/* no	Match empty string. */
#define	STAR	10	/* node	Match this (simple) thing 0 or more times. */
#define	PLUS	11	/* node	Match this (simple) thing 1 or more times. */
#define	OPEN	20	/* no	Mark this point in input as start of #n. */
			/*	OPEN+1 is number 1, etc. */
#define	CLOSE	30	/* no	Analogous to OPEN. */

/*
 * A node is one char of opcode followed by two chars of "next" pointer.
 * "Next" pointers are stored as two 8-bit pieces, high order first.  The
 * value is a positive offset from the opcode of the node containing it.
 * An operand, if any, simply follows the node.  (Note that much of the
 * code generation knows about this implicit relationship.)
 *
 * Using two bytes for the "next" pointer is vast overkill for most things,
 * but allows patterns to get big without disasters.
 */
#define	OP(p)	(*(p))
#define	NEXT(p)	(((*((p)+1)&0377)<<8) + *((p)+2)&0377)
#define	OPERAND(p)	((p) + 3)

/*
 * Utility definitions.
 */
#ifndef CHARBITS
#define	UCHARAT(p)	((int)*(unsigned char *)(p))
#else
#define	UCHARAT(p)	((int)*(p)&CHARBITS)
#endif

#define	ABORT(m)	{ xerror(m); return((char *)NULL); }
#define	ISMULT(c)	((c) == '*' || (c) == '+' || (c) == '?')
#define	META	"^$.[()|?+*\\"

/*
 * Flags to be passed up and down.
 */
#define	HASWIDTH	01	/* Known never to match null string. */
#define	SIMPLE		02	/* Simple enough to be STAR/PLUS operand. */
#define	SPSTART		04	/* Starts with * or +. */
#define	WORST		0	/* Worst case. */

/*
 * Global work variables for regcomp().
 */
static char *regparse;		/* Input-scan pointer. */
static int regnpar;		/* () count. */
static char regdummy;
static char *regcode;		/* Code-emit pointer; &regdummy = don't. */
static long regsize;		/* Code size. */

/*
 * Forward declarations for regcomp()'s friends.
 */
static char *reg();
static char *regbranch();
static char *regpiece();
static char *regatom();
static char *regnode();
static char *regnext();
static void regc();
static void reginsert();
static void regtail();
static void regoptail();
#ifndef USG
static int strcspn();
#endif

/*
 - regcomp - compile a regular expression into internal code
 *
 * We can't allocate space until we know how big the compiled form will be,
 * but we can't compile it (and thus know how big it is) until we've got a
 * place to put the code.  So we cheat:  we compile it twice, once with code
 * generation turned off and size counting turned on, and once "for real".
 * This also means that we don't allocate space until we are sure that the
 * thing really will compile successfully, and we never have to move the
 * code and thus invalidate pointers into it.  (Note that it has to be in
 * one piece because free() must be able to free it all.)
 *
 * Beware that the optimization-preparation code in here knows about some
 * of the structure of the compiled regexp.
 */
regexp *
regcomp(exp)
char *exp;
{
    register regexp *r;
    register char *scan;
    register char *longest;
    register int len;
    int flags;

    if (exp == (char *)NULL)
    {
	xerror("regular expression botch, NULL argument");
	return((regexp *) NULL);
    }

    /* First pass: determine size, legality. */
    regparse = exp;
    regnpar = 1;
    regsize = 0;
    regcode = &regdummy;
    regc(MAGIC);
    if (reg(0, &flags) == (char *)NULL)
	return((regexp *)NULL);

    /* Small enough for pointer-storage convention? */
    if (regsize >= 32767L)		/* Probably could be 65535L. */
    {
	xerror("regular expression botch, regexp too big");
	return((regexp *) NULL);
    }

    /* Allocate space. */
#ifndef lint
    r = (regexp *)malloc(sizeof(regexp) + (unsigned)regsize);
#else
    r = (regexp *)NULL;
#endif /* lint */
    if (r == (regexp *)NULL)
    {
	xerror("regular expression botch, out of space");
	return((regexp *) NULL);
    }

    /* Second pass: emit code. */
    regparse = exp;
    regnpar = 1;
    regcode = r->program;
    regc(MAGIC);
    if (reg(0, &flags) == (char *)NULL)
	return((regexp *)NULL);

    /* Dig out information for optimizations. */
    r->regstart = '\0';	/* Worst-case defaults. */
    r->reganch = 0;
    r->regmust = (char *)NULL;
    r->regmlen = 0;
    scan = r->program+1;		/* First BRANCH. */
    if (OP(regnext(scan)) == END) {	/* Only one top-level choice. */
	scan = OPERAND(scan);

	/* Starting-point info. */
	if (OP(scan) == EXACTLY)
	    r->regstart = *OPERAND(scan);
	else if (OP(scan) == BOL)
	    r->reganch++;

	/*
	 * If there's something expensive in the r.e., find the
	 * longest literal string that must appear and make it the
	 * regmust.  Resolve ties in favor of later strings, since
	 * the regstart check works with the beginning of the r.e.
	 * and avoiding duplication strengthens checking.  Not a
	 * strong reason, but sufficient in the absence of others.
	 */
	if (flags&SPSTART) {
	    longest = (char *)NULL;
	    len = 0;
	    for (; scan != (char *)NULL; scan = regnext(scan))
		if (OP(scan) == EXACTLY && strlen(OPERAND(scan)) >= len) {
		    longest = OPERAND(scan);
		    len = strlen(OPERAND(scan));
		}
	    r->regmust = longest;
	    r->regmlen = len;
	}
    }

    return(r);
}

/*
 - reg - regular expression, i.e. main body or parenthesized thing
 *
 * Caller must absorb opening parenthesis.
 *
 * Combining parenthesis handling with the base level of regular expression
 * is a trifle forced, but the need to tie the tails of the branches to what
 * follows makes it hard to avoid.
 */
static char *
reg(paren, flagp)
int paren;			/* Parenthesized? */
int *flagp;
{
    register char *ret;
    register char *br;
    register char *ender;
    register int parno;
    int flags;

    *flagp = HASWIDTH;	/* Tentatively. */

    /* Make an OPEN node, if parenthesized. */
    if (paren) {
	if (regnpar >= NSUBEXP)
	    ABORT("too many ()");
	parno = regnpar;
	regnpar++;
	ret = regnode(OPEN+parno);
    } else
	ret = (char *)NULL;

    /* Pick up the branches, linking them together. */
    br = regbranch(&flags);
    if (br == (char *)NULL)
	return((char *)NULL);
    if (ret != (char *)NULL)
	regtail(ret, br);	/* OPEN -> first. */
    else
	ret = br;
	if (!(flags&HASWIDTH))
	    *flagp &= ~HASWIDTH;
    *flagp |= flags&SPSTART;
    while (*regparse == '|') {
	regparse++;
	br = regbranch(&flags);
	if (br == (char *)NULL)
	    return((char *)NULL);
	regtail(ret, br);	/* BRANCH -> BRANCH. */
	if (!(flags&HASWIDTH))
	    *flagp &= ~HASWIDTH;
	*flagp |= flags&SPSTART;
    }

    /* Make a closing node, and hook it on the end. */
    ender = regnode((paren) ? CLOSE+parno : END);	
    regtail(ret, ender);

    /* Hook the tails of the branches to the closing node. */
    for (br = ret; br != (char *)NULL; br = regnext(br))
	regoptail(br, ender);

    /* Check for proper termination. */
    if (paren && *regparse++ != ')') {
	ABORT("unmatched ()");
    } else if (!paren && *regparse != '\0') {
	if (*regparse == ')') {
	    ABORT("unmatched ()");
	} else
	    ABORT("junk on end");	/* "Can't happen". */
	/* NOTREACHED */
    }

    return(ret);
}

/*
 - regbranch - one alternative of an | operator
 *
 * Implements the concatenation operator.
 */
static char *
regbranch(flagp)
int *flagp;
{
    register char *ret;
    register char *chain;
    register char *latest;
    int flags;

    *flagp = WORST;		/* Tentatively. */

    ret = regnode(BRANCH);
    chain = (char *)NULL;
    while (*regparse != '\0' && *regparse != '|' && *regparse != ')') {
	latest = regpiece(&flags);
	if (latest == (char *)NULL)
	    return((char *)NULL);
	*flagp |= flags&HASWIDTH;
	if (chain == (char *)NULL)	/* First piece. */
	    *flagp |= flags&SPSTART;
	else
	    regtail(chain, latest);
	chain = latest;
    }
    if (chain == (char *)NULL)	/* Loop ran zero times. */
	(void) regnode(NOTHING);

    return(ret);
}

/*
 - regpiece - something followed by possible [*+?]
 *
 * Note that the branching code sequences used for ? and the general cases
 * of * and + are somewhat optimized:  they use the same NOTHING node as
 * both the endmarker for their branch list and the body of the last branch.
 * It might seem that this node could be dispensed with entirely, but the
 * endmarker role is not redundant.
 */
static char *
regpiece(flagp)
int *flagp;
{
    register char *ret;
    register char op;
    register char *next;
    int flags;

    ret = regatom(&flags);
    if (ret == (char *)NULL)
	return((char *)NULL);

    op = *regparse;
    if (!ISMULT(op)) {
	*flagp = flags;
	return(ret);
    }

    if (!(flags&HASWIDTH) && op != '?')
	ABORT("*+ operand could be empty");
    *flagp = (op != '+') ? (WORST|SPSTART) : (WORST|HASWIDTH);

    if (op == '*' && (flags&SIMPLE))
	reginsert(STAR, ret);
    else if (op == '*') {
	/* Emit x* as (x&|), where & means "self". */
	reginsert(BRANCH, ret);			/* Either x */
	regoptail(ret, regnode(BACK));		/* and loop */
	regoptail(ret, ret);			/* back */
	regtail(ret, regnode(BRANCH));		/* or */
	regtail(ret, regnode(NOTHING));		/* null. */
    } else if (op == '+' && (flags&SIMPLE))
	reginsert(PLUS, ret);
    else if (op == '+') {
	/* Emit x+ as x(&|), where & means "self". */
	next = regnode(BRANCH);			/* Either */
	regtail(ret, next);
	regtail(regnode(BACK), ret);		/* loop back */
	regtail(next, regnode(BRANCH));		/* or */
	regtail(ret, regnode(NOTHING));		/* null. */
    } else if (op == '?') {
	/* Emit x? as (x|) */
	reginsert(BRANCH, ret);			/* Either x */
	regtail(ret, regnode(BRANCH));		/* or */
	next = regnode(NOTHING);		/* null. */
	regtail(ret, next);
	regoptail(ret, next);
    }
    regparse++;
    if (ISMULT(*regparse))
	ABORT("nested *?+");

    return(ret);
}

/*
 - regatom - the lowest level
 *
 * Optimization:  gobbles an entire sequence of ordinary characters so that
 * it can turn them into a single node, which is smaller to store and
 * faster to run.  Backslashed characters are exceptions, each becoming a
 * separate node; the code is simpler that way and it's not worth fixing.
 */
static char *
regatom(flagp)
int *flagp;
{
    register char *ret;
    int flags;

    *flagp = WORST;		/* Tentatively. */

    switch (*regparse++) {
    case '^':
	ret = regnode(BOL);
	break;
    case '$':
	ret = regnode(EOL);
	break;
    case '.':
	ret = regnode(ANY);
	*flagp |= HASWIDTH|SIMPLE;
	break;
    case '[': {
	register int class;
	register int classend;

	if (*regparse == '^') {	/* Complement of range. */
	    ret = regnode(ANYBUT);
	    regparse++;
	} else
	    ret = regnode(ANYOF);
	if (*regparse == ']' || *regparse == '-')
	    regc(*regparse++);
	while (*regparse != '\0' && *regparse != ']') {
	    if (*regparse == '-') {
		regparse++;
		if (*regparse == ']' || *regparse == '\0')
		    regc('-');
		else {
		    class = UCHARAT(regparse-2)+1;
		    classend = UCHARAT(regparse);
		    if (class > classend+1)
			ABORT("invalid [] range");
		    for (; class <= classend; class++)
			regc(class);
		    regparse++;
		}
	    } else
		regc(*regparse++);
	}
	regc('\0');
	if (*regparse != ']')
	    ABORT("unmatched []");
	regparse++;
	*flagp |= HASWIDTH|SIMPLE;
	}
	break;
    case '(':
	ret = reg(1, &flags);
	if (ret == (char *)NULL)
	    return((char *)NULL);
	*flagp |= flags&(HASWIDTH|SPSTART);
	break;
    case '\0':
    case '|':
    case ')':
	ABORT("internal urp");	/* Supposed to be caught earlier. */
/*	break;			*/
    case '?':
    case '+':
    case '*':
	ABORT("?+* follows nothing");
/*	break;			*/
    case '\\':
	if (*regparse == '\0')
	    ABORT("trailing \\");
	ret = regnode(EXACTLY);
	regc(*regparse++);
	regc('\0');
	*flagp |= HASWIDTH|SIMPLE;
	break;
    default: {
	register int len;
	register char ender;

	regparse--;
	len = strcspn(regparse, META);
	if (len <= 0)
	    ABORT("internal disaster");
	ender = *(regparse+len);
	if (len > 1 && ISMULT(ender))
	    len--;		/* Back off clear of ?+* operand. */
	*flagp |= HASWIDTH;
	if (len == 1)
	    *flagp |= SIMPLE;
	ret = regnode(EXACTLY);
	while (len > 0) {
	    regc(*regparse++);
	    len--;
	}
	regc('\0');
	}
	break;
    }

    return(ret);
}

/*
 - regnode - emit a node
 */
static char *			/* Location. */
regnode(op)
char op;
{
    register char *ret;
    register char *ptr;

    ret = regcode;
    if (ret == &regdummy) {
	regsize += 3;
	return(ret);
    }

    ptr = ret;
    *ptr++ = op;
    *ptr++ = '\0';		/* Null "next" pointer. */
    *ptr++ = '\0';
    regcode = ptr;

    return(ret);
}

/*
 - regc - emit (if appropriate) a byte of code
 */
static void
regc(b)
char b;
{
    if (regcode != &regdummy)
	*regcode++ = b;
    else
	regsize++;
}

/*
 - reginsert - insert an operator in front of already-emitted operand
 *
 * Means relocating the operand.
 */
static void
reginsert(op, opnd)
char op;
char *opnd;
{
    register char *src;
    register char *dst;
    register char *place;

    if (regcode == &regdummy) {
	regsize += 3;
	return;
    }

    src = regcode;
    regcode += 3;
    dst = regcode;
    while (src > opnd)
	*--dst = *--src;

    place = opnd;		/* Op node, where operand used to be. */
    *place++ = op;
    *place++ = '\0';
    *place++ = '\0';
}

/*
 - regtail - set the next-pointer at the end of a node chain
 */
static void
regtail(p, val)
char *p;
char *val;
{
    register char *scan;
    register char *temp;
    register int offset;

    if (p == &regdummy)
	return;

    /* Find last node. */
    scan = p;
    for (;;) {
	temp = regnext(scan);
	if (temp == (char *)NULL)
	    break;
	scan = temp;
    }

    if (OP(scan) == BACK)
	offset = scan - val;
    else
	offset = val - scan;
    *(scan+1) = (offset>>8)&0377;
    *(scan+2) = offset&0377;
}

/*
 - regoptail - regtail on operand of first argument; nop if operandless
 */
static void
regoptail(p, val)
char *p;
char *val;
{
    /* "Operandless" and "op != BRANCH" are synonymous in practice. */
    if (p == (char *)NULL || p == &regdummy || OP(p) != BRANCH)
	return;
    regtail(OPERAND(p), val);
}

/*
 * regexec and friends
 */

/*
 * Global work variables for regexec().
 */
static char *reginput;		/* String-input pointer. */
static char *regbol;		/* Beginning of input, for ^ check. */
static char **regstartp;	/* Pointer to startp array. */
static char **regendp;		/* Ditto for endp. */

/*
 * Forwards.
 */
static int regtry();
static int regmatch();
static int regrepeat();

/*
 - regexec - match a regexp against a string
 */
int
regexec(prog, string)
register regexp *prog;
register char *string;
{
    register char *s;
    extern char *strchr();

    /* Be paranoid... */
    if (prog == (regexp *)NULL || string == (char *)NULL) {
	xerror("regular expression botch, NULL parameter");
	return(0);
    }

    /* Check validity of program. */
    if (UCHARAT(prog->program) != MAGIC) {
	xerror("regular expression botch, corrupted program");
	return(0);
    }

    /* If there is a "must appear" string, look for it. */
    if (prog->regmust != (char *)NULL) {
	s = string;
	while ((s = strchr(s, prog->regmust[0])) != (char *)NULL) {
	    if (strncmp(s, prog->regmust, prog->regmlen) == 0)
		break;	/* Found it. */
	    s++;
	}
	if (s == (char *)NULL)	/* Not present. */
	    return(0);
    }

    /* Mark beginning of line for ^ . */
    regbol = string;

    /* Simplest case:  anchored match need be tried only once. */
    if (prog->reganch)
	return(regtry(prog, string));

    /* Messy cases:  unanchored match. */
    s = string;
    if (prog->regstart != '\0')
	/* We know what char it must start with. */
	while ((s = strchr(s, prog->regstart)) != (char *)NULL) {
	    if (regtry(prog, s))
		return(1);
	    s++;
	}
    else
	/* We don't -- general case. */
	do {
	    if (regtry(prog, s))
		return(1);
	} while (*s++ != '\0');

    /* Failure. */
    return(0);
}

/*
 - regtry - try match at specific point
 */
static int			/* 0 failure, 1 success */
regtry(prog, string)
regexp *prog;
char *string;
{
    register int i;
    register char **sp;
    register char **ep;

    reginput = string;
    regstartp = prog->startp;
    regendp = prog->endp;

    sp = prog->startp;
    ep = prog->endp;
    for (i = NSUBEXP; i > 0; i--) {
	*sp++ = (char *)NULL;
	*ep++ = (char *)NULL;
    }
    if (regmatch(prog->program + 1)) {
	prog->startp[0] = string;
	prog->endp[0] = reginput;
	return(1);
    } else
	return(0);
}

/*
 - regmatch - main matching routine
 *
 * Conceptually the strategy is simple:  check to see whether the current
 * node matches, call self recursively to see whether the rest matches,
 * and then act accordingly.  In practice we make some effort to avoid
 * recursion, in particular by going through "ordinary" nodes (that don't
 * need to know whether the rest of the match failed) by a loop instead of
 * by recursion.
 */
static int			/* 0 failure, 1 success */
regmatch(prog)
char *prog;
{
    register char *scan;	/* Current node. */
    char *next;		/* Next node. */
    extern char *strchr();

    scan = prog;
    while (scan != (char *)NULL) {
	next = regnext(scan);

	switch (OP(scan)) {
	case BOL:
	    if (reginput != regbol)
		return(0);
	    break;
	case EOL:
	    if (*reginput != '\0')
		return(0);
	    break;
	case ANY:
	    if (*reginput == '\0')
		return(0);
	    reginput++;
	    break;
	case EXACTLY: {
	    register int len;
	    register char *opnd;

	    opnd = OPERAND(scan);
	    /* Inline the first character, for speed. */
	    if (*opnd != *reginput)
		return(0);
	    len = strlen(opnd);
	    if (len > 1 && strncmp(opnd, reginput, len) != 0)
		return(0);
	    reginput += len;
	    }
	    break;
	case ANYOF:
	    if (strchr(OPERAND(scan), *reginput) == (char *)NULL)
		return(0);
	    reginput++;
	    break;
	case ANYBUT:
	    if (strchr(OPERAND(scan), *reginput) != (char *)NULL)
		return(0);
	    reginput++;
	    break;
	case NOTHING:
	    break;
	case BACK:
	    break;
	case OPEN+1:
	case OPEN+2:
	case OPEN+3:
	case OPEN+4:
	case OPEN+5:
	case OPEN+6:
	case OPEN+7:
	case OPEN+8:
	case OPEN+9: {
	    register int no;
	    register char *save;

	    no = OP(scan) - OPEN;
	    save = reginput;

	    if (regmatch(next)) {
		/*
		 * Don't set startp if some later
		 * invocation of the same parentheses
		 * already has.
		 */
		if (regstartp[no] == (char *)NULL)
		    regstartp[no] = save;
		return(1);
	    } else
		return(0);
	}
/*	break;		*/
	case CLOSE+1:
	case CLOSE+2:
	case CLOSE+3:
	case CLOSE+4:
	case CLOSE+5:
	case CLOSE+6:
	case CLOSE+7:
	case CLOSE+8:
	case CLOSE+9: {
	    register int no;
	    register char *save;

	    no = OP(scan) - CLOSE;
	    save = reginput;

	    if (regmatch(next)) {
		/*
		 * Don't set endp if some later
		 * invocation of the same parentheses
		 * already has.
		 */
		if (regendp[no] == (char *)NULL)
		    regendp[no] = save;
		return(1);
	    } else
		return(0);
	}
/*	break;			*/
	case BRANCH: {
	    register char *save;

	    if (OP(next) != BRANCH)		/* No choice. */
		next = OPERAND(scan);	/* Avoid recursion. */
	    else {
		do {
		    save = reginput;
		    if (regmatch(OPERAND(scan)))
			return(1);
		    reginput = save;
		    scan = regnext(scan);
		} while (scan != (char *)NULL && OP(scan) == BRANCH);
		return(0);
		/* NOTREACHED */
	    }
	    }
	    break;
	case STAR:
	case PLUS: {
	    register char nextch;
	    register int no;
	    register char *save;
	    register int min;

	    /*
	     * Lookahead to avoid useless match attempts
	     * when we know what character comes next.
	     */
	    nextch = '\0';
	    if (OP(next) == EXACTLY)
		nextch = *OPERAND(next);
	    min = (OP(scan) == STAR) ? 0 : 1;
	    save = reginput;
	    no = regrepeat(OPERAND(scan));
	    while (no >= min) {
		/* If it could work, try it. */
		if (nextch == '\0' || *reginput == nextch)
		    if (regmatch(next))
			return(1);
		/* Couldn't or didn't -- back up. */
		no--;
		reginput = save + no;
	    }
	    return(0);
	    }
/*	    break;			*/
	case END:
	    return(1);	/* Success! */
/*	    break;			*/
	default:
	    xerror("regular expression botch, memory corruption");
/*	    return(0);		*/
	    break;
	    }

	scan = next;
    }

    /*
     * We get here only if there's trouble -- normally "case END" is
     * the terminating point.
     */
    xerror("regular expression botch, corrupted pointers");
    return(0);
}

/*
 - regrepeat - repeatedly match something simple, report how many
 */
static int
regrepeat(p)
char *p;
{
    register int count = 0;
    register char *scan;
    register char *opnd;

    scan = reginput;
    opnd = OPERAND(p);
    switch (OP(p)) {
    case ANY:
	count = strlen(scan);
	scan += count;
	break;
    case EXACTLY:
	while (*opnd == *scan) {
	    count++;
	    scan++;
	}
	break;
    case ANYOF:
	while (*scan != '\0' && strchr(opnd, *scan) != (char *)NULL) {
	    count++;
	    scan++;
	}
	break;
    case ANYBUT:
	while (*scan != '\0' && strchr(opnd, *scan) == (char *)NULL) {
	    count++;
	    scan++;
	}
	break;
    default:		/* Oh dear.  Called inappropriately. */
	xerror("regular expression botch, regular expression internal foulup");
	count = 0;	/* Best compromise. */
	break;
    }
    reginput = scan;

    return(count);
}

/*
 - regnext - dig the "next" pointer out of a node
 */
static char *
regnext(p)
register char *p;
{
    register int offset;

    if (p == &regdummy)
	return((char *)NULL);

    offset = NEXT(p);
    if (offset == 0)
	return((char *)NULL);

    if (OP(p) == BACK)
	return(p-offset);
    else
	return(p+offset);
}

/* regexp.c ends here */
