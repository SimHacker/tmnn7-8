/* libfilt.h -- interface declarations for the news filter library */

#define VPROTO	"V100"	/* protocol version this library implements */

/* filter interface functions and the response info pointer from tofilter.c */
extern int fltinit(), fltarticle(), fltnewsgroup(), fltprogram(), fltbye();
extern char *fltinfo;

/* filter shell, accepts -[pd] options */
extern int filter();

/* filter implementation frame */
extern int lnfilter();

/* score-caching functions for filters */
extern void initscore(), putscore(), wrapscore();
extern int getscore();

#define NULLSCORER (int (*)())NULL	/* null scoring hook */

/*
 * Standard priority levels -- use these as a guide in concocting and
 * interpreting score values.
 */
#define P_IGNORE	-99999	/* for articles that should be ignored */
#define P_VISIBLE	0	/* least score for visible articles */
#define P_INTERESTING	10	/* likely to be interesting */
#define P_NOTEWORTHY	20	/* probably worth saving */
#define P_FASCINATING	30	/* a must-see */

/* libfilt.h ends here */
