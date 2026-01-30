/* unistd.h -- symbolic constands for POSIX conformance */

#ifndef R_OK	/* 4.2BSD may have got these from /usr/include/sys/file.h */

/* Symbolic constants for the "access" routine: */
#define	R_OK	4	/* Test for Read permission */
#define	W_OK	2	/* Test for Write permission */
#define	X_OK	1	/* Test for eXecute permission */
#define	F_OK	0	/* Test for existence of File */

#endif /* R_OK */

#ifndef F_ULOCK

#define F_ULOCK	0	/* Unlock a previously locked region */
#define F_LOCK	1	/* Lock a region for exclusive use */
#define F_TLOCK	2	/* Test and lock a region for exclusive use */
#define F_TEST	3	/* Test a region for other processes locks */

#endif /* F_ULOCK */

/* Symbolic constants for the "lseek" routine: */
#define	SEEK_SET	0	/* Set file pointer to "offset" */
#define	SEEK_CUR	1	/* Set file pointer to current plus "offset" */
#define	SEEK_END	2	/* Set file pointer to EOF plus "offset" */

/* Path names: */
#define	GF_PATH	"/etc/group"	/* Path name of the "group" file */
#define	PF_PATH	"/etc/passwd"	/* Path name of the "passwd" file */

/* unistd.h ends here */
