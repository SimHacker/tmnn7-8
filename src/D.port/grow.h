/* grow.h -- interface declarations for mallocing line reader */

struct grow
{
    int howlong;
    int maxlen;
    char *head;
    char *curptr;
};

extern struct grow *growinit();
extern char *growfp();
extern void growstr();
extern void growchar();

/* grow.h ends here */

