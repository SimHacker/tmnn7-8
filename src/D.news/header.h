/* header.h -- article header format and I/O function definitions */

#ifdef ALLOCHDRS
typedef char	*hdrlin;
#else
typedef char	hdrlin[BUFLEN];
#endif /* ALLOCHDRS */

/* article header */
typedef struct
{
    hdrlin	h_to;		/* To: (for replies)	*/
    hdrlin	h_path;		/* Path:		*/
    hdrlin	h_from;		/* From:		*/
    hdrlin	h_newsgroups;	/* Newsgroups:		*/
    hdrlin	h_subject;	/* Subject:		*/
    hdrlin	h_ident;	/* Message-ID:		*/
    hdrlin	h_postdate;	/* Date: (submission)	*/
    hdrlin	h_expdate;	/* Expires:		*/
    hdrlin	h_references;	/* References:		*/
    hdrlin	h_ctlmsg;	/* Control:		*/
    hdrlin	h_sender;	/* Sender:		*/
    hdrlin	h_replyto;	/* Reply-To:		*/
    hdrlin	h_followto;	/* Followup-To:		*/
    hdrlin	h_distribution;	/* Distribution:	*/
    hdrlin	h_organization;	/* Organization:	*/
    int		h_intnumlines;	/* Lines:		*/
    hdrlin	h_keywords;	/* Keywords:		*/
    hdrlin	h_summary;	/* Summary:		*/
    int		h_intpriority;	/* Priority:		*/
    hdrlin	h_approved;	/* Approved:		*/
    hdrlin	h_supersedes;	/* Supersedes:		*/
    hdrlin	h_contenttype;	/* Content-Type		*/
#ifdef ZAPNOTES
    hdrlin	h_nfid;		/* Nf-ID:		*/
    hdrlin	h_nffrom;	/* Nf-From:		*/
#endif /* ZAPNOTES */
    hdrlin	h_backrefs;	/* Back-References:	*/
#ifdef DOXREFS
    hdrlin	h_xref;		/* Xref:		*/
#endif /* DOXREFS */
    char	*h_other;	/* unrecognized lines	*/
    time_t	h_posttime;	/* postdate in secs	*/
    time_t	h_exptime;	/* expdate in secs	*/
    time_t	h_rectime;	/* receipt date in secs	*/

    /* internal information associated with each article header */
    FILE	*h_fp;		/* pointer to message body */
    off_t	h_startoff;	/* start offset of article in file */
    off_t	h_textoff;	/* start offset of article body in file */
    off_t	h_endoff;	/* end offset of article in file */
}
hdr_t;

#ifdef ALLOCHDRS
#define hlcpy(hf, x)	((hf) ? free(hf) : (char *)NULL, hf = savestr(x))
#define hlfree(x)	(void) (free(x), x = (char *)NULL)
#define hlblank(hf)	(hf == (char *)NULL)
#define hlnblank(hf)	(hf != (char *)NULL)
#else
#define hlcpy(hf, x)	(void) strncpy(hf, x, sizeof(hdrlin) - 1)
#define hlfree(x)	x[0] = '\0'
#define hlblank(hf)	(hf[0] == '\0')
#define hlnblank(hf)	(hf[0] != '\0')
#endif /* ALLOCHDRS */

extern void hfree();		/* free the allocated storage of a header */
extern int hread();		/* read a B format header, return length */
extern void hwrite();		/* write a header to a stream */
extern char *hlget();		/* look up a header by name */
extern void happend();		/* add 'unrecognized' lines to a header */
extern char *tailpath();	/* return a short form of a sender's name */

extern hdr_t header;	/* scratch header for everyone's use */

/* header.h ends here */
