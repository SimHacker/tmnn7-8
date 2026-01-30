/* rfuncs.h -- common code for interactive interface managers */

/* search scopes for regular-expression commands */
#define L_SUBJECT   0	/* match subject */
#define L_HEADER    1	/* match any header */
#define L_TEXT	    2	/* match anywhere in text or header */

extern bool rexpand();
extern int procdlines();
extern bool shell_command();
extern char *edited_reply();
extern char *direct_reply();
extern char *stuffit();
extern void followup();
extern int change_group();
extern char *gotoid();
extern char *cancel_command();
extern int recmd();
extern char *briefdate();

/* rfuncs.h ends here */
