/* uucplib.h -- interface file for UUCP hacks library */

/* these may need modification if your UUCP is weird */
#define	CDIR	"/usr/spool/uucp"	/* local command files directory */
#define	DDIR	"/usr/spool/uucp"	/* data files directory */
#define	XDIR	"/usr/spool/uucp"	/* remote command files directory */

#ifndef HONEYDANBER
#define GRADE	"MA"	/* grade to use for uucico files.
			   Should not be used by anyone else */
#define	GRADE2	"XM"	/* other grade to use for files
			   This grade should start with an X. */
#else
#define GRADE	'M'	/* HoneyDanBer job grades are a single character */
#endif

/* library entry points  */
extern int uucast();		/* multicast a UUCP job to several systems */
extern off_t uuq();		/* return size of outgoing UUCP queue */
extern void encode(), decode();	/* 7-bit encode-decode routines */

/* uucplib.h ends here */
