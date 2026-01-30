/* server.h -- types for child server handling */

#define MAXSERVERS  1	/* enough for present purposes */

typedef struct
{
    int	    pid;	/* process ID of server */
    int     readsrv;	/* read fd from server */
    int     writesrv;	/* write fd to server */
}
server_t;

extern server_t *srvopen();
extern void srvclose();

#define srvgets(buf, len, srv)	read(srv->readsrv, buf, len)
#define srvputs(buf, srv)	write(srv->writesrv, buf, strlen(buf))
#define srvread(srv, buf, len)	read(srv->readsrv, buf, len)
#define srvwrite(srv, buf, len)	write(srv->writesrv, buf, len)

/* server.h ends here */
