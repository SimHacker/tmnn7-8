/* libpost.h -- interface file for posting functions library */

typedef struct
{
    char	address[BUFLEN];
    char	citeline[BUFLEN];
}
cite_t;

extern cite_t *mkfollowup();
extern int editmsg(), postmsg(), originate();
extern int checkincl();
extern char *prepmsg(), *newpost(), *recording();
extern void newsubject(), savemsg(), signmsg(), fixaddress();

/* libpost.h ends here */
