/****************************************************************************

NAME
   grow.c -- functions to implement a malloc'ing line reader
	
SYNOPSIS
   #include "grow.h"

   struct grow *growinit(x)	-- set up an input allocation block
   struct grow *x;

   char *growfp(parent, fp)	-- get chars till newline
   struct grow *parent;

   void growstr(parent, newstr)	-- append character string
   struct grow *parent; char *newstr;

   void growchar(parent, c)	-- append character
   struct grow *parent; register char c;

DESCRIPTION
   These functions provide a mallocing line reader not restricted to any
fixed-size buffer.

AUTHOR
   Jean Marie Diaz

**************************************************************************/
/*LINTLIBRARY*/
#include "libport.h"
#include "grow.h"

struct grow *growinit(x)
struct grow *x;
{
    if (x)
    {
	if (x->curptr)
	    free((char *)x->curptr);
	free((char *)x);
    }
#ifndef lint
    x = (struct grow *)malloc((unsigned) sizeof(struct grow));
#else
    x = (struct grow *)NULL;
#endif /* lint */
    if (x == (struct grow *)NULL)
	xerror("Out of memory");
    else
    {
	x->howlong = x->maxlen = 0;
	x->head = x->curptr = (char *)NULL;
    }
    return(x);
}

/*
 * Given a (struct grow *) and a (FILE *), read from the fp until we
 * see a newline.
 */
char *growfp(parent, fp)
struct grow *parent;
FILE *fp;
{
    register int increment = BUFSIZ / 4;
    char *ptr;
	
    for (;;)
    {			/* we return out of here */
	/* get more space */
	if (parent->maxlen == 0) { /* is this the very first time? */
	    parent->head = parent->curptr =
		malloc((unsigned) sizeof(char *) * increment);
	    if (parent->head == (char *) NULL)
		xerror("Out of memory");
	    parent->maxlen = increment;
	}
	else
	{			/* realloc more space */
	    parent->maxlen += increment;
	    parent->head = realloc(parent->head,
				   (unsigned) sizeof(char *)
				   * parent->maxlen);
	    if (parent->head == (char *) NULL)
		xerror("Out of memory");
	    parent->curptr = &(parent->head[parent->howlong]);
	}

	/* now that we've got more space, use it */

	if ((ptr = fgets(parent->curptr,
			 (parent->maxlen - parent->howlong), fp))
	    == (char *)NULL)
	    return((char *)NULL); /* EOF, or error */

	if ((ptr = strchr(parent->curptr, '\n')) != (char *)NULL) {
	    /*
	     * we got a newline, so we've read a whole
	     * line.  Null out the newline and go.
	     */
	    *ptr = NULL;
	    return(parent->head);
	}
	/*
	 * Hmm -- there's more waiting; let's do it again.
	 */
    }
}

#ifdef FUTURE
/*
 * Given a (struct grow *) and a null-terminated string, add it, punting any
 * '\r' along the way.
 */
void growstr(parent, newstr)
struct grow *parent;
char *newstr;
{
    register int i;
    register int len = strlen(newstr);
    /*
     * we want to overwrite the null on the next pass, if there is
     * a next pass.
     */

    if ((parent->howlong + len) >= parent->maxlen) {
	/*
	 * here we stomp a subtle bug: the assumption that 
	 * len < BUFSIZ.  Instead of adding BUFSIZ to
	 * parent->head, add (len + (BUFSIZ / 4)).
	 */
	register int moreroom = len + (BUFSIZ/4);

	parent->maxlen += moreroom;
	if (parent->maxlen == moreroom)
	{		
	    /* ie, was this the first trip through? */
	    parent->curptr = parent->head = malloc((unsigned)
						   sizeof (char *)
						   * parent->maxlen);
	    if (parent->curptr == (char *) NULL)
		xerror("Out of memory");
	}
	else
	{
	    parent->head = realloc(parent->head,((unsigned) sizeof
						 (char *)
						 * parent->maxlen));
	    if (parent->head == (char *) NULL)
		xerror("Out of memory");
	    parent->curptr = &(parent->head[parent->howlong]);
	}
    } 

    for (i = 0; i < len; i++)
    {
	if (newstr[i] != '\r')
	{
	    *parent->curptr++ = newstr[i];
	    parent->howlong++;
	}
    }
} 

/*
 * Given a (struct grow *) and a single character, append it, as long as it is
 * not '\r'.
 */
void growchar(parent, c)
struct grow *parent;
register char c;
{
    if (c == '\r') return;
	
    if ((parent->howlong + 1) >= parent->maxlen) {
		
	parent->maxlen += BUFSIZ;
	if (parent->maxlen == BUFSIZ)
	{		
	    /* ie, was this the first trip through? */
	    parent->head = malloc((unsigned) sizeof (char *)
				  * parent->maxlen);
	    if (parent->head == (char *) NULL)
		xerror("Out of memory");
	}
        else
	{
	    parent->head = realloc(parent->head, ((unsigned) sizeof
						  (char *) * parent->maxlen));
	    if (parent->head == (char *) NULL)
		xerror("Out of memory");
	}
	parent->curptr = parent->head + parent->howlong;
    }

    *parent->curptr++ = c;
    parent->howlong++;
} 
#endif /* FUTURE */

/* grow.c ends here */
