/* procopts.h -- stuff used to encode information on program options */

#define OPTION	0	/* pick up an option string */
#define STRING	1	/* pick up a string of arguments */
#define NUMBER	2	/* pick up an integer argument */
#define MAXARGS	32	/* maximum number of args we can crack out */

#define NONE	(int *)0    /* used to flag options with no value */
#define DNC	    0x00    /* state not set, we don't care */

typedef struct
{
    char	optlet;		/* option character. */
    char	filchar;	/* if to pickup string, fill character. */
    int		*flagaddr;	/* gets set to option value */
    int		oldstate;	/* states we can enter from */
    int		newstate;	/* state we leave the program in */
    int		argtype;	/* STRING if takes arg, else OPTION */
    char	*buf;		/* string buffer */
}
option_t;

/* procopts.h ends here */
