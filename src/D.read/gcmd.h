/* gcmd.h -- action return types for the generic command interpreter */

/* action mask values (bits 0x00FF reserved for gcmd's use) */
#define HOLD	0x0001	/* hold current action for next cycle */
#define NOMORE	0x0002	/* no more articles */
#define EXIT	0x0004	/* exit, no cleanup */
#define CMDERR	0x0008	/* unrecognized command	*/

/* mark mask values for message mark scope field */
#define REPLY	0x08	/* reply request pending */
#define FOLLOW	0x10	/* followup request pending */

/*
 * This is the structure for returns of user command arguments.
 * Whetever other data may be on the command line is accessible
 * via vgetc() and vgetline().
 */
typedef struct
{
    bool	c_argform;	/* TRUE if command has a prefix */
#define C_NONE	0x00			/* no arguments */
#define C_COUNT	0x01			/* prefix count is present */
#define C_UPTO	0x02			/* number after range separator */
#define C_RANGE	0x03			/* a range */
    long	c_count;	/* numeric prefix count on command */
#define C_RMARK	','			/* range indicator */
    long	c_upto;		/* second numeric argument */
    char	c_char;		/* first character of command */
}
cmdarg_t;

extern int action;		/* what to do next cycle */
extern cmdarg_t *gparse();	/* the generic command parser */
extern void gcmd();		/* the generic command interpreter */

#define pending(n)	((n) & action)
#define request(n)	(action |= (n))
#define demand(n)	(action = (n))
#define unset(n)	(action &=~ (n))

/* gcmd.h ends here */
