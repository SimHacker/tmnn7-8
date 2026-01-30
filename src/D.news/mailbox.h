/* mailbox.h -- types for mailbox manipulation */

extern char	*mailbox();	/* generate name of user's primary mailbox */

/* flags for saveto() routine */
#define S_SAVEHDR	0x01	/* write out article header */
#define S_OVWRITE	0x02	/* overwrite the file if it already exists */
#define S_MAILFMT	0x04	/* save in mail format */
#define S_NOSIGNAL	0x08	/* block interrupts while saving */
extern int saveto();

typedef struct
{
    char	*mb_file;	/* the digest file location */
    int		mb_style;	/* digest style */
#define D_V7MAIL	0		/* V7 mailbox format */
#define D_MMDF		1		/* MMDF mailbox */
#define D_RFC822	2		/* RFC-822 archive */
#define D_DIGEST	3		/* Internet digest */
    int		mb_artcount;	/* count of articles */
    off_t	*mb_start;	/* start offset list */
    off_t	*mb_length;	/* length list */
    bool	*mb_seen;	/* seen bits */
}
mbox_t;
extern mbox_t	*crackmail();	/* crack and index a mailbox */

/* mailbox.h ends here */

