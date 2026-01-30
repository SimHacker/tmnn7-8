/* insrc.h -- interface to routines for handling article temporary file */

typedef struct
{
    /* public data */
    /*
     * Maybe these int declarations ought to be longs...on some machines,
     * lint becomes upset because the values of variables that have to be
     * large enough to hold a nart_t get assigned into these.
     */
    int	    i_artlines;	/* # lines already seen, after folding */
    int	    i_topline;	/* source line that is top line on screen */
    int	    i_botline;	/* source line that is bottom line on screen */
    int	    i_nomore;	/* if TRUE entire article has been read in */
    unsigned int	i_width;	/* column width of output window for this stream */
    unsigned int	i_height;	/* row depth of output window for this stream */
#ifdef DIGPAGE
    int     i_dtype;	/* digest type */
#define DT_NOBREAKS	0	/* ordinary article */
#define DT_DIGEST	1	/* ARPANET-style digest */
#define DT_MAILBREAKS	2	/* V7 or other mail-archive format */
#endif /* DIGPAGE */

    /* these public members are not used in insrc.c */
    int	    i_hdrend;	/* index of line at end of header */
    int	    i_ustate;	/* user state info */

    /* these members are private to insrc.c */
#ifdef DEBUG
    char    *i_tfname;	/* name of temp file */
#endif /* DEBUG */
    int	    i_lastlin;	/* number of lines in tempfile */
    int	    i_folded;	/* total line count corrected for folded lines */
    FILE    *i_fp;	/* where to get input */
    FILE    *i_tfp;	/* temporary file pointer */
    int	    i_tfnext;	/* number of next line in i_tfp */
    int	    i_lastwrf;	/* tells whether last op was a write */
#ifdef DIGPAGE
    int	    i_endsuba;	/* end of sub-article in digest */
#endif /* DIGPAGE */
}
insrc_t;

extern insrc_t *src;	/* current input source */

extern void tfopen();	/* switch to a new input source */
extern void tfclose();	/* release a source's tempfile (DEBUG mode only) */
extern bool tfgrab();	/* get the next piece of input */
extern int tfcount();	/* return count of displayable lines for this page */
extern void tfget();	/* get a line from the temp file */
extern void tfput();	/* put a line to the temp file */
extern int tfappend();	/* append a line to the tempfile */
#ifdef DIGPAGE
extern int tfskip();	/* return start line of next digest section */
#endif /* DIGPAGE */

#define tfreading(sp)		(src == (sp))
#define tfswitch(sp)		(src = (sp))
#define tfsetsize(w,r,c)	((w)->i_height = (r), (w)->i_width = (c))

/*
 * The i_folded member is used for calculating percentage text seen, also to
 * distinguish messages that have no text (such as control messages).
 * It must be initialized via tfartlen() with the article line length before
 * folding. Displayed percentages will be too high if the article is longer
 * than one screenfull and has many folded lines.
 */
#define tfartlen()	src->i_folded
#define tfsetlen(len)	(src->i_folded = (len));
#define tfpercent(line)	((((line)-src->i_hdrend) * 100) / src->i_folded)

/*
 * Macros to get and set the source line number of the current top line.
 * These take effect at the time of the next updscr().
 */
#define gotoline(n)	src->i_topline = (n)
#define topline()	src->i_topline

/* insrc.h ends here */
