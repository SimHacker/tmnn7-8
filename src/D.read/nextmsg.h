/* nextmsg.h -- interface for spool-directory traversing functions */

extern void initmsg();
extern int nextmsg();
extern int nextgroup();
extern int gotogrp();
extern int gotomsg();
extern place_t *tellmsg();
extern int seekmsg();
extern bool visible();

/* nextmsg.h ends here */
