/* transact.h -- filter hacks library declarations file */

#define MAXERRSIZE	160

typedef struct
{
    char	type, code;
    char	*arg1, *arg2, *arg3, *arg4, *arg5;
    char	buf[MAXERRSIZE];
}
fresp_t;

#define	NOARG	(char *)NULL

extern void fconnect();
extern int ftransmit();
extern int freceive();

/* transact.h ends here */
