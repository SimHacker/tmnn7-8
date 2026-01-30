/*****************************************************************************

NAME
   articleid.c -- article-ID handling

SYNOPSIS
   #include "news.h"

   int rewrite(hp, ofp)		-- write header and text to file pointer
   hdr_t *hp; FILE *ofp;

   ulong hashart(hp)		-- compute hash of given article
   hdr_t *hp;

   char *idmake(hp)		-- generate a unique Message-ID for an article
   hdr_t *hp;

   artid_t *idparse(ptr)	-- extract hash, etc. from an article ID
   char *ptr;

   bool idvalid(ptr)		-- returns true if the ID conforms to
   char *ptr				   to standards
DESCRIPTION
   This function is used to generate a unique Internet-style Message-ID for
each article posted from this site from the posting time, the posting
process number, and the site's 'true' (domainist) name.

   The format generated here may inflict a performance lose on on SV sites
running older news versions; they depend on the last char of the left-hand
part being a digit for their pseudo-hashing scheme. Thanks to Greg Noel for
pointing this out, but in order to enable user-querying of remote accesses
for the hypertext version we need part of the ID to be a valid user address
for the originating user.

   Do not rely on the precise form of articleIDs generated here; use idparse()
when you want to examine their contents. The general form handled by this
module is:

	<timestamp#hash[+extension-fields]=user@site>

where extension-fields is a list of plus-separated tokens. In 3.0, no extension
fields are generated, but future versions may put helper info for reference
following in them.

NOTE
   It may be that we really want to use the name of the posting site's gateway
machine, not the posting site's `true name'; this won't be clear until more
experience with distributed fetch has accumulated.

AUTHOR
   Eric S. Raymond
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

******************************************************************************/
/* LINTLIBRARY */
#include "news.h"
#include "header.h"

#define LONGLEN	24	/* in base 62, 12 chars will specify up to 48 bits */

/*
 * For RFC822 conformance, these must not be in the set ()<>@,;:\".[]
 * Available chars are thus !#$%^&*-_+={}|~`'?/. Note that . is a special
 * case, permitted in article-ID left sides but (according to RFC822) a
 * word separator.
 */
#define TSTAMPEND	'%'	/* ends timestamp field */
#define EXTFLDEND	'='	/* ends extension fields */
#define EXTFLDSEP	'+'	/* separates extension fields */

/* we leave vowels out to solve the `FUCK' problem */
const char radixtab[] ="0123456789BCDFGHJKLMNOPQRSTVWXYZbcdfghjklmnpqrstvwxyz";
#define RADIX (sizeof(radixtab)-1)

private void rpack(val, erep)
/* pack a 32-bit value in as few alphanumeric characters as we can manage */
ulong	val;
char	*erep;
{
    char	*rp;

    if (val == 0)
	rp = "0";
    else
    {
	char	rep[LONGLEN];

	/* write out external representation if requested */
	*(rp = rep + LONGLEN - 1) = '\0';
	while (val)
	{
	    *--rp = radixtab[val % RADIX];
	    val /= RADIX;
	}
    }

    (void) strcpy(erep, rp);
}

private ulong runpack(rep)
/* unpack a 32-bit value from rpack() form */
char	*rep;
{
    ulong	val = 0L;

    while (*rep)
	val = (RADIX * val) + (strchr(radixtab, *rep++) - radixtab);
    return(val);
}

/*
 * rewrite --- write a give header and file section to a file pointer
 */
int rewrite(hp, ofp)
hdr_t	*hp;	/* header of article to write */
FILE	*ofp;	/* where to write to */
{
    register long   ccount, newstartoff, newtextoff, newendoff;
    char	    buf[BUFSIZ];

    newstartoff = ftell(ofp);
    hwrite(hp, ofp, TRUE);
    newtextoff = ftell(ofp);

    if (fseek(hp->h_fp, (off_t)hp->h_textoff, SEEK_SET) != SUCCEED)
	return(FAIL);

    if (hp->h_endoff == (off_t)0)	/* no section size, go till EOF */
	while (fgets(buf, BUFSIZ, hp->h_fp) != (char *)NULL)
	    (void) fputs(buf, ofp);
    else				/* header specified a section size */
	for (ccount = hp->h_textoff; ccount < hp->h_endoff; ccount++)
	    if (feof(hp->h_fp))
		return(FAIL);
	    else
		(void) fputc(fgetc(hp->h_fp), ofp);
    newendoff = ftell(ofp);

    if (ferror(ofp))
	return(FAIL);

    hp->h_fp = ofp;
    hp->h_startoff = newstartoff;
    hp->h_textoff = newtextoff;
    hp->h_endoff = newendoff;
    return(SUCCEED);
}

ulong hashart(hp)
/*
 * Calculate the article's authentication hash. The hash includes the article  
 * text and sensitive headers. DON'T MUCK WITH THIS! Everybody has to be using
 * the same checks for the authentication scheme to work.
 */
hdr_t	*hp;
{
    ulong	hash;

    (void) fseek(hp->h_fp, (off_t)hp->h_startoff, SEEK_SET);
    hash = checksum(hp->h_fp, (unsigned) 0L);

    /* we don't include From, it may be changed by mailers */
    /* we don't include Newsgroups, it may be changed by aliasing */
    hash = checkstring(hp->h_subject, hash);	/* the Subject line */
    if (hlnblank(hp->h_expdate))
	hash=checkstring(hp->h_expdate, hash);	/* the Expire-Date line */
    if (hlnblank(hp->h_references))
	hash=checkstring(hp->h_references,hash);    /* the References line */
    if (hlnblank(hp->h_ctlmsg))
	hash=checkstring(hp->h_ctlmsg, hash);	/* the Control line */
    /* We don't include Reply-To, may be changed by mailers */
    if (hlnblank(hp->h_organization))
	hash=checkstring(hp->h_organization, hash); /* the Organization line */
    if (hlnblank(hp->h_summary))
	hash=checkstring(hp->h_summary, hash);	/* the Summary line */
    if (hlnblank(hp->h_approved))
	hash=checkstring(hp->h_approved, hash);	/* the Approved line */
    if (hlnblank(hp->h_supersedes))
	hash=checkstring(hp->h_supersedes,hash);   /* the Supersedes line */

    return(hash);
}

char *idmake(file)
/* decorate the article in the given file with a unique Message-ID */
char	*file;
{
    static char	name[NAMELEN], timestamp[LONGLEN], hashrep[LONGLEN];
    hdr_t	hh;
    FILE	*ofp, *fp;
    int 	status;

    if ((fp = fopen(file, "r")) == (FILE *)NULL)
	return((char *)NULL);

    (void) bzero(&hh, sizeof(hdr_t));
    if (hread(&hh, 0L, fp) == 0)
    {
	(void) fclose(fp);
        hfree(&hh);
	return((char *)NULL);
    }

    /* use the hash to make a new ID to plug into the header */
    rpack((ulong)time((time_t *)NULL), timestamp);
    rpack(hashart(&hh), hashrep);
    (void) sprintf(name, "<%s#%s=%s@%s>",
		   timestamp, hashrep, username, site.truename);
    (void) hlcpy(hh.h_ident, name);

    /* write the article with ID to a new tempfile */
    (void) strcpy(bfr, file); (void) strcat(bfr, ".new");
    ofp = xfopen(bfr, "w");
    hh.h_startoff = (off_t)0;
    status = rewrite(&hh, ofp);
    (void) fclose(hh.h_fp); (void) fclose(ofp);
    hfree(&hh);

    if (status)
    {
	(void) unlink(bfr);
	return((char *)NULL);
    }
    else
    {
	(void) rename(bfr, file);
	return(name);
    }
}

artid_t *idparse(ptr)
/* crack an ID into its component parts */
char	*ptr;
{
    static char		idcopy[NAMELEN];
    static artid_t	cracked;
    register char	*base;

    (void) strcpy(idcopy, ptr);
    ptr = idcopy;

    /* for RFC-822 conformance, ID must begin with left angle-bracket */
    if (*ptr != '<')
	return((artid_t *)NULL);

    /* timestamp field must be pure alphanumeric */
    while (*ptr && *ptr != TSTAMPEND)
	if (!isalnum(*ptr++))
	    return((artid_t *)NULL);

    /* and followed by a pound sign */
    if (*ptr != TSTAMPEND)
	return((artid_t *)NULL);
    else
    {
	*ptr++ = '\0';
	cracked.id_timestamp = (time_t) runpack(idcopy);
    }
    base = ptr;

    /* hash field must be pure alphanumeric */
    while (*ptr && *ptr != EXTFLDEND && *ptr != EXTFLDSEP)
	if (!isalnum(*ptr++))
	    return((artid_t *)NULL);

    /* and followed by an extension field end or separator */
    if (*ptr != EXTFLDEND && *ptr != EXTFLDSEP)
	return((artid_t *)NULL);
    else
    {
	*ptr++ = '\0';
	cracked.id_hash = runpack(base);;
    }

#ifdef FOO
    /* model code for extension field parsing */
    base = ptr;
    while (*ptr && *ptr != EXTFLDEND && *ptr != EXTFLDSEP)
	ptr++;
    if (*ptr == '\0')
	return((artid_t *)NULL);
    else
	cracked.id_extended = base;
    if (*ptr == EXTFLDSEP)
	ptr++;
#endif /* FOO */

    /* find the home site name of the article */
    if ((ptr = strchr(idcopy, '@')) == (char *)NULL)
	return((artid_t *)NULL);
    else
	*ptr = '\0';
    cracked.id_homesite = ptr + 1;

    /* now get the user name */
    while (*--ptr != EXTFLDEND && ptr > base)
	continue;
    if (ptr <= base)
	return((artid_t *)NULL);
    else
	cracked.id_username = ptr + 1;

    return(&cracked);
}
/*
 * Returns true if the id passed is valid.  It could be much more
 * rigorous, but unfortunately sites are passing off a lot of things
 * as valid message-ids.....
 */
bool idvalid(ptr)
	char	*ptr;
{
	if (!ptr || (*ptr != '<'))
		return(FALSE);	/* Must begin with a < */
	if (ptr[strlen(ptr) - 1] != '>')
		return(FALSE);	/* Must end with a > */
	if (!strchr(ptr, '@'))
		return(FALSE);	/* Must contain a @ */
	return(TRUE);
}

/* articleid.c ends here */
