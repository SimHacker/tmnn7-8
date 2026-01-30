/* slist.h -- definitions for manipulating allocated string lists */

extern int sgets(), sputs();
extern bool sfind();
/*
 * Declare the control structure for an slist lookup.
 * See dballoc.c for details on what this is actually doing. The expression
 * sizeof((char*)[1]) is intended to compute the size of a character pointer
 * slot in an array (i.e. the size of the char * plus whatever trailing pad
 * the machine architecture requires).
 */
#define SLIST(x,i,j)    dbdef_t x = {NULL,sizeof(char*[1]),i,j,sgets,sputs};

#define senter(db, line)	dbaenter(db, savestr(line))

/* slist.h ends here */
