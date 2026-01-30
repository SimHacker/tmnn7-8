/* alist.h -- definitions for manipulating allocated association lists */

typedef struct
{
    char    *key;	/* pointer to the key value */
    char    *value;	/* the associated value text */
}
assoc;

extern int asplit(), adump();
/*
 * Declare the control structure for an alist lookup.
 * See dballoc.c for details on what this is actually doing. The expression
 * sizeof(assoc[1]) is intended to compute the size of an assoc record
 * slot in an array (i.e. the size of the assoc plus whatever trailing pad
 * the machine architecture requires).
 */
#define ALIST(x,i,j)    dbdef_t x = {NULL,sizeof(assoc[1]),i,j,asplit,adump};

void aenter(), areplace();
char *afind(), *amatch();
#define akey(db)	((assoc *) ((db)->cp))->key
#define avalue(db)	((assoc *) ((db)->cp))->value
#define adelete(db)	areplace(db, (char *)NULL)

/* alist.h ends here */
