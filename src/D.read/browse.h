/* browse.h -- interface to page browsing service level */

extern insrc_t	article, groups, help, msghdr;
#ifdef SUBJFILE
extern insrc_t	subjects;
#endif /* SUBJFILE */

/* input source types */
#define ARTICLE	    &article	/* viewing article text */
#define SUBJECTS    &subjects	/* looking at a group subject listing */
#define GROUPS      &groups	/* list all current groups */
#define HELP	    &help	/* no transform, view given file */
#define HEADER	    &msghdr	/* view full header of current message */

/* action bitmasks unique to paging modes (masks 0x0F00 reserved for this) */
#define HDRONLY	0x0100	/* display header only on this pass */
#define ROT13	0x0200	/* rot13 displayed article text */

extern int ihflag, pflag, textline();
extern insrc_t *readfrom();
extern void browseinit(), browse(), browsewrap();
extern void backpage(), nextpage(), gotohdr();
extern void changeline(), fliprot(), subjadd();
extern bool bsetsize();

extern least, most;		/* bounds for on-page items */

/* browse.h ends here */
