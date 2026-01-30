/* fascist.h -- types and constants for permissions file features */

#define F_COMMAND	0
#define F_USER		1
#define F_GROUP		1
#define F_SITE		1
#define F_MEMBERS	2
#define	F_KEEPERS	3
#define F_POSTSUBSC	2
#define F_READSUBSC	3
#define F_MAXFLDS	4

#define F_COMMENT	'#'
#define F_FIELDSEP	":"

typedef struct
{
    char	*n_post;	/* subscription one can post to */
    char	*n_read;	/* subscription one can read */
}
nasty_t;

extern char *getgrplist();
extern nasty_t *fascist();
extern bool allmatch();

/* fascist.h ends here */ 
