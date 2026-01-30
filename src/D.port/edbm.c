/***********************************************************************

NAME
   edbm.c -- generic database routines

SYNOPSIS
   #include "edbm.h"

   database *dbmopen(file)	    -- opens a database for use
   char   *file;

   int dbmseek(key, keylen, db, wlock)	    -- find entry with specified key
   char *key; int keylen; database *db;

   int dbmdelete(db)		    -- deletes the current entry
   database *db;

   void dbmrewind(db)		    -- rewind, next dbmnext gets 1st key
   database *db;

   int dbmnext(db, wlock)	    -- step to next database element
   database *db; bool wlock;

   int dbmput(key, klen, content, clen, db)    -- write content to an entry
   char *key; unsigned klen;
   char *content; unsigned clen;
   register database *db;

   char *dbmget(clen, db)	    -- get content of current entry
   unsigned *clen;
   database *db;

   void dbmunlock(db)		    -- release lock on current item
   database *db;

   void dbmclose(db)		    -- releases and closes given database
   database *db;

   void dbmtrunc(db)	    	    -- cleans out a given database
   database *db;

DESCRIPTION
   These are data base routines modelled on the V7 Unix dbm routines. They
fetch and store (key, content) pairs from a collection of databases. The size
of content is unlimited. See edbm.h for the data types used.

   The dbmopen() function opens the named database. It needs the files
<file>.dat, <file>.pag and <file>.dir.  Returns a pointer to a database
structure which is used as input to all the other routines.

   The functions dbmrewind() and dbmnext() may be used to step through the
database. After initializing or reinitializing with dbmrewind(), each
dbmnext() call gets a new key (and returns SUCCEED) until you run out of
keys (at which point it returns FAIL). The traversal order is not useful,
unless you really wanted to step through the keys by hash value and
order of storage time within buckets.

   Given a key and content as strings, dbmput() puts them into the given
database. The content can be very large. You may dbmput() to the same key
more than once; each time the content gets larger a new content record is
appended to the data file.

   The dbmget() function returns a pointer to an allocated area of
storage containing the content of the currently-selected database item.
This area will be automatically deallocated by the next dbmseek(), dbmnext(),
or dbmrewind(). If the first (unsigned *) arg of dbmget() is non-NULL, it
will be used as an address to deposit the content length in.

CONCURRENT ACCESS
   On System V Release 3 UNIX (or other versions implementing mandatory file
locking via a POSIX-compatible lockf() call), lockf() can be used to ensure
that writes don't step on each other. Systems with the XENIX 3.0-derived
locking(2) call can implement most of lockf(). Code is included to use these
calls; it is conditional on the LOCKF or LOCKING symbols in config.h.

   On SVr2 systems without lockf() but with mandatory locking via fctl(2), and
BSD systems with FIONCLEX, the entire database file is locked for exclusive
access by dbmopen(); this ensures that other processes attempting to write
to it sleep until the caller exits (this code is untested but simple enough
that it likely works as is; see the text of dbmopen() for details).

   On UNIXes with none of the above, dbmput() interacts sanely with readers
in other processes, but not necessarily with other writers. It is only 
guaranteed to work properly if there is at most one writer active at any given
time.

   If the SHARED symbol is on, all numeric data in the datum structures should
be maintained in a machine-independent byte order for access by heterogenous
machines via NFS or RFS. Naturally, this costs some conversion overhead on
each access.

NOTE
   If your processor has no alignment restrictions on longs, define
NO_ALIGNMENT for a substatial speedup. This is automatically done for
some processors.

BUGS
   The SHARED code doesn't work yet.
   It would be nice if these routines could create the database files and
go when they don't already exist. Unfortunately, trying to use conditional
creat() calls produces weird bugs (specifically, writes to the database don't
complete until after the exit() following the files' creation). Adding sync()
calls doesn't help either. Sigh...
   Because the third arguments of reads() and writes() are ints on V7/BSD
systems (rather than unsigned as on USG systems) content lengths with
the high bits on may do weird things.

FILES
   <file>.dir  -- database key directory
   <file>.pag  -- database page index
   <file>.dat  -- database content file

LEGAL NOTE
   Use of this code does *not* require an AT&T or BSD source license.
   This code is a distant descendant of V7's dbm(3) routines, by way of
James Gosling's modification of BSD ndbm() as found in ancient gosmacs
versions and in the X window system distribution.
   AT&T has permitted use of the gosmacs ndbm code in commercial product
without fee or legal hindrance since 1982 thus implicitly releasing it for
unrestricted use.
   In any case, this version no longer uses the hash constants of V7 ndbm,
nor does it depend on any AT&T or BSD proprietary code. Though the algorithms
remain similar to those of ndbm, the interface and implementation logic differ
from it in significant ways. This code must therefore be legally considered an
independent work of the author.

INTERNALS
   Database tuples are stored in .pag file blocks as (key, offset, len) with
the key and the offset and length of its datum in the .dat file being stored
next to each other. The format of each .pag block is as follows (with s = 
sizeof(short), l = sizeof(long)):

	     0s: item_count
	     1s: od0 (buffer offset of datum 0)
	     2s: od1 (buffer offset of datum 1)
		 ...
	   (n)s: odn (buffer offset of datum n)

		 <free space>

	    odn: .dat seek address of datum n
	  odn+l: length of datum n
	 0dn+2l: start of key of datum n
		 ...
	    od1: .dat seek address of datum 1
	  od1+l: length of datum 1
	 0d1+2l: start of key of datum 1
	    od0: .dat seek address of datum 0
	  od0+l: length of datum 0
	 0d0+2l: start of key of datum 0
		 <end of .pag file block>

   The length of any item can be computed as (offset of previous item) -
(offset of this item), where the `offset' of item number -1 is the block
size.
   The .pag files grow more or less by powers of two (imagine a tree with
the nodes numbered thus:

level	      tree		hash mask (in binary)
0:		0			0000
1:	    0	    1			0001
2:	  0   2   1   3			0011
3:       0 4 2 6 1 5 3 7		0111
	 ------- -------
	  these	   these
	 hash to  hash to
	  XXX0	   XXX1

etc.).  In a fresh database, you start at level 0, and to store an
item, you compute a hash number for it, then use no bits; all items
thus wind up in node 0.  When node zero gets full, you `split' to level
1, recomputing the hash for each item in 0; you now use one bit to move
about half the data to node 1.  If node 0 gets full again, you start
using two bits, and (since the things in it all end with a zero bit)
about half the items move to node 2; if node 1 gets full, you start
using two bits there and about half move to 3.
   It takes one bit per node per level to remember what was split; this bit
can be named by (hash & mask) + mask. Remembering these bits is what the .dir
file is used for. If you get (un)lucky with the hashfunction or have enough
data, the hashing could lean on one path through the tree, making the apparent
database size much larger than it actually (minus holes) is.
   The heart of the whole thing is the loop

	hash = calchash(item);
	for (hash_mask = 0;; hash_mask = (hash_mask << 1) + 1) {
		split_memory_bit = (hash & hash_mask) + hash_mask;
		if (!bit_is_set(split_memory_bit, split_map_table))
			break;
	}
	block = hash & hash_mask;
	( item is either in node `block' or not there at all )
	( read that node and search for it sequentially )

and, of course the hashing function.  (Pseudocode for the split
routine is left as an exercise for the student :-) .)
   The basic purpose of the hash is to turn what might be largely
well-correlated bit patterns into what should be largely random
bit patterns.  The more random the bit patterns, the flatter
the node tree will become.

AUTHOR
   Eric S. Raymond
   The INTERNALS section is partly based on Chris Torek's USENET posting
<11953@mimsy.UUCP>, dated 14 Jun 88 03:27:19 GMT.
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

***********************************************************************/
/*LINTLIBRARY*/
#include "libport.h"
#include "edbm.h"

#if defined(vax) || defined(iAPX386) || defined(i386) || defined(mc68030)
#define NO_ALIGNMENT
#endif

#ifdef SHARED
/* #undef SHARED		/* SHARED format doesn't work yet */
#endif

#define	BYTESIZ	8	/* bits per byte */
#define EXTLEN	5	/* length of .pag, .dir or .dat extension + 1 NUL */

#ifndef private
#define private static
#endif

database *lastdatabase;
forward static int setup_db();

#ifdef UNIXPC
#ifdef lint
int locking(fd, op, size) int fd, op; long size; {return((int)(fd+op+size));}
#endif
#endif

/*
 * These typedefs control the sizes of the numeric items in a .pag block.
 * The left side of the typedef may be one of short, int, or long.
 * If you're trying to share databases between different machines, choose
 * values for these on each machine that give the same byte sizes on all.
 * This might mean making 'sshort' a long or int, or 'slong' an int.
 */
typedef short sshort;	/* 2-byte integral type */
typedef long slong;	/* 4-byte internal type */

#ifdef SHARED
/*
 * This code works by translating .pag blocks out of a fixed big-endian format
 * just after they're read into core, and into it just before they're written
 * out. The .dir and .dat formats are already quite portable, depending only on
 * BYTESIZ. This code will permit sharing between any machines with the same
 * sshort and slong sizes (handling different sizes is a *much* harder problem,
 * sorry about that!).
 */
private void frombigend(rep, size)
/* decode a short or long from big-endian network-independent order */
char	*rep;
int     size;
{
    register char *rp = rep;
    register int i = size;
    long	val = 0L;

    /* read MSB first */
    while (i--)
    {
	val <<= BYTESIZ;	/* woo! first time I've used one of these... */
	val += (*rp++ & 0x0f);	/* if you're confused, look it up in K&R */
    }
    (void) memcpy(rep, (char *)val, size);
}

private void fromshared(buf)
/* translate a .pag block out of SHARED format */
char    buf[PBLKSIZ];
{
    register sshort *sp;
    register int i, offset, datcount;

#ifndef lint
    sp = (sshort *) buf;
#else
    sp = (sshort *) NULL;
#endif
    frombigend(buf, sizeof(sshort));
    datcount = *sp++;
    for (i = 0; i < datcount; i++, sp++)
    {
	int	offset = *sp;

	/* translate page block offsets out of SHARED format */
	frombigend((char *)sp, sizeof(sshort));

	/* translate address and length longs out of SHARED format */
	frombigend(buf + offset, sizeof(slong));
	frombigend(buf + offset + sizeof(slong), sizeof(slong));
    }
}

private void tobigend(val, rep, size)
/* express a unsigned integer or long in network-independent order */
slong	val;
char	*rep;
int     size;
{
    /* write in non-perverse byte order, MSB first */
    while (size--)
	*rep++ = (val >> (BYTESIZ * size)) & 0xff;
}

private void toshared(buf)
/* check .pag block for validity, translate it to SHARED format */
char    buf[PBLKSIZ];
{
    register short *sp;
    register int i, datcount;

#ifndef lint
    sp = (sshort *) buf;
#else
    sp = (sshort *) NULL;
#endif
    tobigend(datcount = *sp++, buf, sizeof(sshort));
    for (i = 0; i < datcount; i++, sp++)
    {
	int	offset;
	slong	*lp;

	/* translate page block offsets into SHARED format */
	tobigend(offset = *sp, (char *)sp, sizeof(sshort));

	/* translate address and length longs into SHARED format */
	lp = (slong *)(buf + offset);
	tobigend(lp[0], buf + offset, sizeof(slong));
	tobigend(lp[1], buf + offset + sizeof(slong), sizeof(slong));
    }
}
#endif /* SHARED */

/*
 * Using this macro for freeing the current data alloc area guarantees that
 * it will always point either to an allocated data area or be NULL.
 */
#define FREE(d)	if(d->freeptr){(void)free(d->freeptr);d->freeptr=(char*)NULL;}

database *dbmopen(file)
/* opens a database for use */
char   *file;
{
    /* we use calloc() because freeptr field must be initially zeroed out */
#ifndef lint
    register database *db = (database *) calloc(sizeof(*db), 1);
#else
    register database *db = (database *)NULL;
#endif /* lint */
    register int	  len;
    struct stat	  statb;

    len = strlen(file) + EXTLEN;
    db->dirnm = malloc((unsigned) (4 * len));
    db->pagnm = db->dirnm + len;
    db->datnm = db->pagnm + len;
    db->dbnm  = db->datnm + len;
    len -= EXTLEN;
    (void) strcpy(db->dirnm, file);
    (void) strcpy(db->dirnm + len, ".dir");
    (void) strcpy(db->pagnm, file);
    (void) strcpy(db->pagnm + len, ".pag");
    (void) strcpy(db->datnm, file);
    (void) strcpy(db->datnm + len, ".dat");
    (void) strcpy(db->dbnm, file);
    db->oldpagb = db->olddirb = -1;
    if (setup_db(db) == FAIL)
    {
	(void) free(db->dirnm);
	(void) free((char *) db);
	return((database *)NULL);
    }
    (void) fstat(db->dirf, &statb);
    db->maxbno = statb.st_size * BYTESIZ - 1;
    return(db);
}

void dbmclose(db)
register    database *db; 
{
    if (db == (database *)NULL)
	return;
    if (lastdatabase == db)
    {
	if (db->dirf > 0)
	    (void) close(db->dirf);
	if (db->pagf > 0)
	    (void) close(db->pagf);
	if (db->datf > 0)
	    (void) close(db->datf);
	db->dirf = db->pagf = db->datf = FAIL;
	lastdatabase = 0;
    }
    (void) free(db->dirnm);
    (void) free((char *)db);
}

private int setup_db(db)
/* ensure that the database files are open or available */
register  database *db; 
{
    extern int	errno;
#ifdef SYSV2
#ifndef LOCKF
    struct flock    dlock;

    dlock.l_type = F_WRLCK;
    dlock.l_start = dlock.l_whence = dlock.l_len = 0;
#endif /* !LOCKF */
#endif /* SYSV2 */

    if (db == (database *)NULL)
	    return(FAIL);
    if (lastdatabase == db)
	    return(SUCCEED);
    if (lastdatabase) {
	if (lastdatabase->dirf > 0)
	    (void) close(lastdatabase->dirf);
	if (lastdatabase->pagf > 0)
	    (void) close(lastdatabase->pagf);
	if (lastdatabase->datf > 0)
	    (void) close(lastdatabase->datf);
	lastdatabase->dirf = lastdatabase->pagf = lastdatabase->datf = -1;
	lastdatabase = (database *)NULL;
    }

    db->dirf = open(db->dirnm, 2);
    db->dbrdonly = FALSE;
    if (db->dirf < 0)
    {
	db->dbrdonly = TRUE;
	db->dirf = open(db->dirnm, 0);
    }

    db->pagf = open(db->pagnm, db->dbrdonly ? 0 : 2);
    db->datf = open(db->datnm, db->dbrdonly ? 0 : 2);

    if (db->dirf < 0 || db->pagf < 0 || db->datf < 0)
    {
	(void) close(db->dirf);
	(void) close(db->pagf);
	(void) close(db->datf);
	return(FAIL);
    }
#ifdef SYSV2
#ifndef LOCKF
    dlock.l_pid = getpid ();
    (void) fcntl ((int) db->dirf, F_SETLK, &dlock);
    (void) fcntl ((int) db->pagf, F_SETLK, &dlock);
    (void) fcntl ((int) db->datf, F_SETLK, &dlock);
#endif /* !LOCKF */
#endif /* SYSV2 */
#ifdef FIOCLEX
#ifndef LOCKF
#ifndef lint
    /* lint can't see this because 4.2 and 4.3 disagree on 2nd arg type */
    (void) ioctl((int) db->dirf, FIOCLEX, (char *)NULL);
    (void) ioctl((int) db->pagf, FIOCLEX, (char *)NULL);
    (void) ioctl((int) db->datf, FIOCLEX, (char *)NULL);
#endif /* !lint */
#endif /* !LOCKF */
#endif /* FIOCLEX */
    lastdatabase = db;
    return(SUCCEED);
}

void dbmtrunc(db)
/* cleans out an existing database */
database	*db;
{
    register int  fd;

#ifdef MAIN
    errno = 0;
#endif /* MAIN */
    if ((fd = creat(db->dirnm, 0777)) != FAIL)
	(void) close(fd);
#ifdef MAIN
    else
	(void) printf("dbmtrunc: errno %d on creat(%s.dir)\n",errno,db->dirnm);
    errno = 0;
#endif /* MAIN */

    if ((fd = creat(db->pagnm, 0777)) != FAIL)
	(void) close(fd);
#ifdef MAIN
    else
	(void) printf("dbmtrunc: errno %d on creat(%s.pag)\n",errno,db->pagnm);
    errno = 0;
#endif /* MAIN */

    if ((fd = creat(db->datnm, 0777)) != FAIL)
	(void) close(fd);
#ifdef MAIN
    else
	(void) printf("dbmtrunc: errno %d on creat(%s.dat)\n",errno,db->datnm);
#endif /* MAIN */
}

void dbmrewind(db)
/* rewind, next dbmnext() gets 1st key */
register database *db;
{
    FREE(db);
    db->current.dptr = (char *)NULL;
}

static long table[][020] =
{
    023742330770L, 003576024251L, 013127742007L, 033652616201L, 
    021621643530L, 033350045504L, 032635460216L, 007061141364L, 
    007156051721L, 016022755352L, 017215767266L, 006000434051L, 
    025337671652L, 007637557061L, 022275345443L, 035463227436L, 
    016264714255L, 016042620630L, 014052611726L, 026263777255L, 
    003012701055L, 014300563627L, 004656271362L, 014404537373L, 
    013274635625L, 001131413175L, 011320003474L, 030246473204L, 
    033754556062L, 006505636142L, 033514310031L, 010360511776L, 
    013746766455L, 012625320116L, 010730567002L, 023204472144L, 
    036625762173L, 007322231264L, 024602242004L, 022264612067L, 
    030137755615L, 024771412714L, 027101263114L, 011641073476L, 
    015632056155L, 035156143376L, 011431706724L, 015343020125L, 
    022072213117L, 010660247176L, 023323516107L, 023620065301L, 
    021025324457L, 034747403475L, 035574315516L, 004550100612L, 
    022760547101L, 015642777522L, 026201547441L, 006263600425L, 
    024314041157L, 005660214721L, 010455573121L, 012364560205L, 
    001324644570L, 017332522050L, 031376626562L, 034015752166L, 
    011406450433L, 037664563560L, 005413054610L, 030127721632L, 
    015542231104L, 007073334051L, 023062652750L, 000351313465L, 
    035337546653L, 015464432177L, 006357461421L, 022532007441L, 
    014162740567L, 015423135154L, 036263770217L, 036373623017L, 
    024240500022L, 007345241252L, 016360347602L, 021606365102L, 
    027143473505L, 021620662367L, 032755747035L, 033533733272L, 
    036252210734L, 020214537366L, 034313365741L, 002100770454L, 
    007540262636L, 003124351634L, 012001214102L, 034205303751L, 
    033712104461L, 020377332573L, 004573367576L, 020237664016L, 
    005470074032L, 025752473107L, 005642207303L, 034214413761L, 
    014257524666L, 032471644742L, 006425470051L, 023270305263L, 
    006052530305L, 027012211770L, 020214110134L, 012607344215L, 
    011410565214L, 035067620101L, 012001016631L, 030767722406L, 
    030265064600L, 001644062374L, 025770775062L, 037167722053L, 
    011104434474L, 014674135762L, 010143633215L, 032603334341L, 
};

/*
 * Calculate the hash val for the given item.
 * Algorithm chosen by Richard Stallman 
 * after discussions with Professor Rivest,
 * without looking at the one formerly distributed with mdbm.
 */
private long calchash(ptr, count)
register char *ptr;
register unsigned int count;
{
    register long accum = 0;
    register long *tabptr = &table[0][0];
    register char byte;
#if (BYTESIZ != 8)
    register char fullbyte;
#endif

    while (count--)
    {
	byte = *ptr++;
#if (BYTESIZ == 8)
	accum += tabptr[byte & 0xf]; tabptr += 0x10;
	accum += tabptr[(byte >> 4) & 0xf]; tabptr += 0x10;
	if (tabptr == (&table[0][0] + (sizeof(table) / sizeof(long))))
	{
	    tabptr = &table[0][0];
	    accum = (accum << 1) + (1 & (accum >> 31));
	}
#else
	fullbyte = -1;
	while (fullbyte)
	{
	    accum += tabptr[byte & 0xf]; tabptr += 0x10;
	    byte >>= 4;
	    fullbyte <<= 4;
	    if (tabptr == (&table[0][0] + (sizeof table / sizeof(long))))
	    {
		tabptr = &table[0][0];
		accum = (accum << 1) + (1 & (accum >> 31));
	    }
	}
#endif /* (BYTESIZ != 8) */
    }
    return(accum);
}

/*
 * The following two functions encapsulate all handling of the .dir file.
 * They implement get/set of the bit specified by db->bitno in the .dir file.
 */
#define read_dirb(db, b) (void) read(db->dirf, (b), (iolen_t)DBLKSIZ)
#define write_dirb(db,b) (void) write(db->dirf, (b), (iolen_t)DBLKSIZ)
#define lock_dirb(db)    (void) lockf((db)->dirf, F_LOCK, (long)DBLKSIZ)
#define unlock_dirb(db)  (void) lockf((db)->dirf, F_ULOCK, (long)DBLKSIZ)

/*ARGSUSED1*/
private int bitget(db, wlock)
/* fetch the directory block indexed by db->bitno */
register database *db;
bool wlock;
{
    long	bytenum;
    register long blocknum, boffset, bit;

    if (db->bitno > db->maxbno)
	return(0);
    bit = db->bitno % BYTESIZ;
    bytenum = db->bitno / BYTESIZ;
    boffset = bytenum % DBLKSIZ;
    blocknum = bytenum / DBLKSIZ;
#ifndef LOCKF	/* can't buffer blocks in memory if multi-user */
    if (blocknum != db->olddirb)
    {
#endif /* LOCKF */
	(void) bzero(db->dirbuf, DBLKSIZ);
	(void) setup_db(db);
#ifdef MAIN
	(void) printf("edbm: seeking to .dir block %ld\n", blocknum);
#endif /* MAIN */
	(void) lseek((db)->dirf, (off_t)(blocknum * DBLKSIZ), SEEK_SET);
#ifdef LOCKF
	if (wlock)
	{
#ifdef MAIN
	    (void) printf("edbm: locking .dir block %ld\n", blocknum);
#endif /* MAIN */
	    lock_dirb(db);
	}
#endif /* LOCKF */
#ifdef MAIN
	(void) printf("edbm: reading .dir block %ld...", blocknum);
	(void) fflush(stdout);
#endif /* MAIN */
	read_dirb(db, db->dirbuf);
#ifdef MAIN
	(void) printf("got it\n");
#endif /* MAIN */
#ifndef LOCKF 
	db->olddirb = blocknum;
    }
#endif /* LOCKF */
    return ((db->dirbuf[boffset] & (1 << bit)) != 0);
}

private int bitset(db)
register    database *db;
{
    long	bytenum;
    register long boffset, bit, blocknum;

    if (db->dbrdonly)
	return(FAIL);
    if (db->bitno > db->maxbno)
    {
	db->maxbno = db->bitno;
	(void) bitget(db, TRUE);	/* this does a lock_dirb() */
    }
    bit = db->bitno % BYTESIZ;
    bytenum = db->bitno / BYTESIZ;
    boffset = bytenum % DBLKSIZ;
    blocknum = bytenum / DBLKSIZ;
    db->dirbuf[boffset] |= (1 << bit);

    (void) setup_db(db);
#ifdef MAIN
    (void) printf("edbm: seeking to .dir block %ld\n", blocknum);
#endif /* MAIN */
    (void) lseek((db)->dirf, (off_t)(blocknum * DBLKSIZ), SEEK_SET);
#ifdef MAIN
    (void) printf("edbm: writing .dir block %ld...", blocknum);
    (void) fflush(stdout);
#endif /* MAIN */
    write_dirb(db, db->dirbuf);
#ifdef MAIN
    (void) printf("did it\n");
#endif /* MAIN */
#ifdef LOCKF
#ifdef MAIN
    (void) printf("edbm: seeking to .dir block %ld\n", blocknum);
#endif /* MAIN */
    (void) lseek((db)->dirf, (off_t)(blocknum * DBLKSIZ), SEEK_SET);    
#ifdef MAIN
    (void) printf("edbm: unlocking .dir block %ld...", blocknum);
    (void) fflush(stdout);
#endif /* MAIN */
    unlock_dirb(db);
#ifdef MAIN
    (void) printf("did it\n");
#endif /* MAIN */
#endif /* LOCKF */
    return(SUCCEED);
}

/*
 * Only the following five functions know about the internal format of
 * .pag blocks.
 */
private datum makdatum(buf, n)
char    buf[PBLKSIZ];
int n;
{
    register short *sp, offset;
    register int t;
    register char 	*lp;
    datum	item;

#ifndef lint
    sp = (sshort *) buf;
#else
    sp = NULL;
#endif
    if (n < 0 || n >= sp[0])
    {
	item.dptr = (char *)NULL;
	item.dsize = 0;
    }
    else
    {
	t = PBLKSIZ;
	if (n > 0)
	    t = sp[n + 1 - 1];
	lp = buf + (offset = sp[n + 1]);
#if defined(lint) || !defined(NO_ALIGNMENT)
	(void) memcpy((char *)&item.daddress, lp, sizeof(slong));
#else
	*((slong *) &item.daddress) = *((slong *) lp);
#endif /* defined(lint) || !defined(NO_ALIGNMENT) */
	lp += sizeof(slong);
#if defined(lint) || !defined(NO_ALIGNMENT)
	(void) memcpy((char *)&item.dlength, lp, sizeof(slong));
#else
	*((slong *) &item.dlength) = *((slong *) lp);
#endif /* defined(lint) || !defined(NO_ALIGNMENT) */
	lp += sizeof(slong);
	item.dptr = lp;
	item.dsize = t - offset - 2 * sizeof(slong);
    }
    return(item);
}

private int delitem(buf, n)
/* delete an item out of a database page */
char    buf[PBLKSIZ];
{
    register short *sp;
    register unsigned    i1, i2, i3;

#ifndef lint
    sp = (sshort *) buf;
#else
    sp = NULL;
#endif
    if (n < 0 || n >= sp[0])
	    return(FAIL);
    i1 = sp[n + 1];
    i2 = PBLKSIZ;
    if (n > 0)
	i2 = sp[n + 1 - 1];
    i3 = sp[sp[0] + 1 - 1];
    if (i2 > i1)
	while (i1 > i3)
	{
	    i1--; i2--;
	    buf[i2] = buf[i1];
	    buf[i1] = 0;
	}
    i2 -= i1;
    for (i1 = n + 1; i1 < sp[0]; i1++)
	    sp[i1 + 1 - 1] = sp[i1 + 1] + i2;
    sp[0]--;
    sp[sp[0] + 1] = 0;
    return(SUCCEED);
}

private int additem(buf, item)
char	buf[PBLKSIZ];
datum	item;
{
    register short *sp;
    register char *p;
    register unsigned i1, i2;

#ifndef lint
    sp = (sshort *) buf;
#else
    sp = NULL;
#endif
    i1 = PBLKSIZ;
    if (sp[0] > 0)
	i1 = sp[sp[0] + 1 - 1];
    i1 -= item.dsize + 2 * sizeof(slong);
    i2 = (sp[0] + 2) * sizeof(sshort);
    if (i1 <= i2)
	return(FAIL);
    sp[sp[0] + 1] = i1;
    p = &buf[i1];
    (void) memcpy(p, (char *)&item.daddress, sizeof(slong));
    p += sizeof(slong);
    (void) memcpy(p, (char *)&item.dlength, sizeof(slong));
    p += sizeof(slong);
    for (i2 = 0; i2 < item.dsize; i2++)
	*p++ = item.dptr[i2];
    sp[0]++;
    return(sp[0] - 1);
}

private int chkblk(buf)
/* check .pag block for validity */
char    buf[PBLKSIZ];
{
    register short *sp;
    register int t, i;

#ifndef lint
    sp = (sshort *) buf;
#else
    sp = NULL;
#endif
    t = PBLKSIZ;
    for (i = 0; i < sp[0]; i++)
    {
	if (sp[i + 1] > t)
	    goto bad;
	t = sp[i + 1];
    }
    if (t >= (sp[0] + 1) * sizeof(sshort))
	return(SUCCEED);
bad:
    (void) bzero(buf, PBLKSIZ);
    return(FAIL);
}

/*
 * The following two functions encapsulate all I/O handling of the .pag file.
 * They implement read and write of page sections.
 */
#define seek_page(db, n) (void) lseek((db)->pagf,(off_t)((n)*PBLKSIZ),SEEK_SET)
#define read_page(db, b) (void) read(db->pagf, (b), (iolen_t) PBLKSIZ)
#define write_page(db,b) (void) write(db->pagf, (b), (iolen_t)PBLKSIZ)
#define lock_page(db)    (void) lockf((db)->pagf, F_LOCK, (long)PBLKSIZ)
#define unlock_page(db)  (void) lockf((db)->pagf, F_ULOCK, (long)PBLKSIZ)

/*ARGSUSED2*/
private int get_page(hash, db, wlock)
/* fetch the first database file bucket corresponding to a given hash value */
register database *db;
long    hash;
int	wlock;
{
    for (db->hmask = 0;; db->hmask = (db->hmask << 1) + 1)
    {
	db->blkno = hash & db->hmask;
	db->bitno = db->blkno + db->hmask;
	if (bitget(db, FALSE) == 0)
	    break;
    }
#ifndef LOCKF		/* can't buffer blocks if multi-user */
    if (db->blkno != db->oldpagb)
    {
#endif /* LOCKF */
	(void) bzero(db->pagbuf, PBLKSIZ);
	(void) setup_db(db);
#ifdef MAIN
	(void) printf("edbm: seeking to .pag block %ld\n", db->blkno);
#endif /* MAIN */
	seek_page(db, db->blkno);
#ifdef LOCKF
	if (wlock)
	{
#ifdef MAIN
	    (void) printf("edbm: locking .pag block %ld\n", db->blkno);
#endif /* MAIN */
	    lock_page(db);
	}
#endif /* LOCKF */
#ifdef MAIN
	(void) printf("edbm: reading .pag block %ld...", db->blkno);
	(void) fflush(stdout);
#endif /* MAIN */
	read_page(db, db->pagbuf);
#ifdef MAIN
	(void) printf("got it\n");
#endif /* MAIN */
#ifdef SHARED
	fromshared(db->pagbuf);
#endif /* SHARED */
	(void) chkblk(db->pagbuf);
#ifndef LOCKF
	db->oldpagb = db->blkno;
    }
#endif /* LOCKF */
}

void put_page(db, blkno, buf)
/* write an updated block to the .pag file */
register database *db;
long blkno;
char *buf;
{
#ifdef MAIN
    (void) printf("edbm: seeking to .pag block %ld\n", blkno);
#endif /* MAIN */
    seek_page(db, blkno);
#ifdef MAIN
    (void) printf("edbm: writing .pag block %ld...", blkno);
    (void) fflush(stdout);
#endif /* MAIN */
#ifdef SHARED
    toshared(buf);
#endif /* SHARED */
    write_page(db, buf);
#ifdef MAIN
	(void) printf("did it\n");
#endif /* MAIN */
#ifdef LOCKF
#ifdef MAIN
    (void) printf("edbm: seeking to .pag block %ld\n", blkno);
#endif /* MAIN */
    seek_page(db, blkno);
#ifdef MAIN
    (void) printf("edbm: unlocking .pag block %ld\n", blkno);
#endif /* MAIN */
    unlock_page(db);
#endif /* LOCKF */
}

#ifdef LOCKF
void dbmunlock(db)
/* if you just need to release the currently locked entry without writing it */
register database *db;
{
#ifdef MAIN
    (void) printf("edbm: seeking to .dir block %ld\n", db->bitno / BYTESIZ);
#endif /* MAIN */
    (void) lseek((db)->dirf, (off_t)(db->bitno / BYTESIZ), SEEK_SET);
#ifdef MAIN
    (void) printf("edbm: unlocking .dir block %ld\n", db->blkno);
#endif /* MAIN */
    unlock_dirb(db);
#ifdef MAIN
    (void) printf("edbm: seeking to .pag block %ld\n", db->blkno);
#endif /* MAIN */
    seek_page(db, db->blkno);
#ifdef MAIN
    (void) printf("edbm: unlocking .pag block %ld\n", db->blkno);
#endif /* MAIN */
    unlock_page(db);
}
#endif /* LOCKF */

/* this function only depends on knowing about the datum structure */

private int cmpdatum(key, keylen, d2)
/* compare two datums by key -- analogous to memcmp() */
char	    *key;
unsigned    keylen;
datum	    d2;
{
    register char  *p1, *p2;
    register int   d;

    if (keylen != d2.dsize)
	return(keylen - d2.dsize);
    if (keylen == 0)
	return(0);
    p1 = key;
    p2 = d2.dptr;
    do
    {
	if (d = *p2++ - *p1++)
	    return(d);
    } while
	(--keylen);
    return(0);
}

/* now we get to the actual reading and writing functions */

int dbmseek(key, keylen, db, wlock)
/* find entry with specified key */
char		    *key;
unsigned	    keylen;
register database   *db;
bool                wlock;
{
    register int    i;
    datum	    item;

    get_page(calchash(key, keylen), db, wlock);
    for (i = 0; ; i++)
    {
	item = makdatum(db->pagbuf, i);
	if (item.dptr == (char *)NULL)
	    return(FAIL);
	if (cmpdatum(key, keylen, item) == 0)
	{
	    FREE(db);
	    db->current = item;
	    return(SUCCEED);
	}
    }
}

int dbmdelete(db)
/* deletes the current entry */
register database *db;
{
    register int	i;
    datum		item;

    if (db->dbrdonly)
	return(FAIL);
    get_page(calchash(db->current.dptr, db->current.dsize), db, TRUE);
    for (i = 0; ; i ++) {
	item = makdatum(db->pagbuf, i);
	if (item.dptr == (char *)NULL)
	{
#ifdef LOCKF
	    dbmunlock(db);
#endif /* LOCKF */
	    return(FAIL);
	}
	if (cmpdatum(db->current.dptr, db->current.dsize, item) == 0)
	{
	    FREE(db);
	    (void) delitem(db->pagbuf, i);
	    break;
	}
    }
    (void) setup_db(db);
    put_page(db, db->blkno, db->pagbuf);
    return(SUCCEED);
}

private long nextbucket(hash, db)
register    database *db;
long    hash;
{
    long    bit;

    hash &= db->hmask;
    bit = db->hmask + 1;
    for (;;)
    {
	bit >>= 1;
	if (bit == 0)
	    return(0);
	if ((hash & bit) == 0)
	    return((long)(hash | bit));
	hash &= ~bit;
    }
}

/*ARGSUSED1*/
int dbmnext(db, wlock)
/* get the next sequential entry in the database */
register database *db;
bool wlock;
{
    register int i;
    datum	item, bitem, firsthash();
    long	hash;
    int		f;

    /* if we're rewound, get the first element we can access */
    if (db->current.dptr == (char *)NULL)
    {
	bitem = firsthash(0L, db, wlock);
	if (bitem.dptr == (char *)NULL)
	    return(FAIL);
    }
    else    /* otherwise, find the next element after the current one */
    {
	hash = calchash(db->current.dptr, db->current.dsize);
	get_page(hash, db, wlock);		/* go to the correct page */
	for (f = i = 0;; i++) {
	    item = makdatum(db->pagbuf, i);
	    if (item.dptr == (char *)NULL)
		break;
	    if (cmpdatum(db->current.dptr, db->current.dsize, item) <= 0)
		continue;
	    if (f++ == 0 || cmpdatum(bitem.dptr, bitem.dsize, item) < 0)
		bitem = item;
	}
	if (f == 0)	    /* get first key on next page or after */
	{
	    if ((hash = nextbucket(hash, db)) == 0)
		return(FAIL);
	    bitem = firsthash(hash, db, wlock);
	}
    }
    FREE(db);
    db->current = bitem;
    return(SUCCEED);
}

int store(newdat, db)
register database *db;
datum	newdat;
{
    register int i;
    datum	item;
    char    ovfbuf[PBLKSIZ];

    if (setup_db(db) < 0)
	return(FAIL);
    if (db->dbrdonly)
	return(FAIL);
loop:
    get_page(calchash(newdat.dptr, newdat.dsize), db, TRUE);
    for (i = 0;; i++)
    {
	item = makdatum(db->pagbuf, i);
	if (item.dptr == (char *)NULL)
	    break;
	if (cmpdatum(newdat.dptr, newdat.dsize, item) == 0)
	{
	    (void) delitem(db->pagbuf, i);
	    break;
	}
    }
    i = additem(db->pagbuf, newdat);
    if (i < 0)
	goto split;
    put_page(db, db->blkno, db->pagbuf);
    return(SUCCEED);

split:
    if (newdat.dsize + 2*sizeof(slong) + 2*sizeof(sshort) >= PBLKSIZ)
	return(FAIL);
    (void) bzero(ovfbuf, PBLKSIZ);
    for (i = 0;;)
    {
	item = makdatum(db->pagbuf, i);
	if (item.dptr == (char *)NULL)
	    break;
	if (calchash(item.dptr, item.dsize) &(db->hmask + 1))
	{
	    (void) additem(ovfbuf, item);
	    (void) delitem(db->pagbuf, i);
	    continue;
	}
	i++;
    }
    put_page(db, db->blkno, db->pagbuf);
    put_page(db, db->blkno + db->hmask + 1L, ovfbuf);
    (void) bitset(db);
    goto loop;
}

datum firsthash(hash, db, wlock)
/* find the first item in or after the bucket specified by hash */
long hash;
register database *db;
bool wlock;
{
    register int i;
    datum	item, bitem;

    do {
	get_page(hash, db, wlock);
	bitem = makdatum(db->pagbuf, 0);
	for (i = 0;; i++) {
	    item = makdatum(db->pagbuf, i);
	    if (item.dptr == (char *)NULL)
		break;
	    if (cmpdatum(bitem.dptr, bitem.dsize, item) < 0)
		bitem = item;
	}
	if (bitem.dptr != (char *)NULL)
	    return(bitem);
    } while
	(hash = nextbucket(hash, db));

    return(item);
}

int dbmput(key, keylen, content, contentlen, db)
/* write content to an entry */
char	    *key;	/* key to associate content with */
unsigned    keylen;	/* length of the key to write to */
char	    *content;	/* the content */
unsigned contentlen;	/* the length of the content */
register database *db;
{
    int	    foundit;
    static char    *malloc_kludge = NULL;
    
    if (malloc_kludge)
	    free(malloc_kludge);
    malloc_kludge = savestr(key);
    (void) setup_db(db);
    foundit = (dbmseek(key, keylen, db, TRUE) == SUCCEED);
#ifdef LENGTHOPTIM	/* this should work, but doesn't */
    if (foundit && contentlen <= db->current.dlength)
    {
	(void) lseek(db->datf, (off_t)db->current.daddress, SEEK_SET);
	db->current.dlength = contentlen;
    }
    else
#endif /* LENGTHOPTIM */
    {
	if (foundit)
	{
	    (void) dbmdelete(db);
	    (void) dbmseek(key, keylen, db, TRUE);
	}
	db->current.dptr = malloc_kludge; /* key */
	db->current.dsize = keylen;
	db->current.daddress = lseek(db->datf, (off_t)0, SEEK_END);
	db->current.dlength = contentlen;
	db->freeptr = (char *)NULL;
    }
    if (store(db->current, db) < 0)
	return(FAIL);
#ifdef LOCKF
    (void) lockf(db->datf, F_LOCK, (long)contentlen);
#endif /* LOCKF */
    (void) write(db->datf, content, (iolen_t) contentlen);
#ifdef LOCKF
    (void) lseek(db->datf, (off_t)db->current.daddress, SEEK_SET);
    (void) lockf(db->datf, F_ULOCK, (long)contentlen);
#endif /* LOCKF */
    return(SUCCEED);
}

char *dbmget(contentlen, db)
unsigned	    *contentlen;
register database   *db;
{
    char    *content;

    (void) setup_db(db);
    if ((content = malloc((unsigned) db->current.dlength+1)) == (char *)NULL)
	return((char *)NULL);
    if (contentlen)
	*contentlen = db->current.dlength;
    (void) lseek(db->datf, (off_t)db->current.daddress, SEEK_SET);
    if (read(db->datf, content,
		(iolen_t) db->current.dlength) == db->current.dlength)
	return(db->freeptr = content);
    else
	return((char *)NULL);
}

#ifdef MAIN
/*
 * An exerciser for these functions.
 */
char	*Progname = "edbm";

main()
{
    char    cmdline[BUFSIZ], strv[BUFSIZ], strv2[BUFSIZ], dbname[BUFSIZ];
    database *mydb;

    (void) printf("This is the edbm database editor, type ? for help\n");
    while (fputs("* ", stdout) != EOF && fgets(cmdline, sizeof(cmdline), stdin) != (char *)NULL)
    {
	if (sscanf(cmdline, "o %s", dbname) == 1)
	{
	    (void) sprintf(strv2,
			"touch %s.dat; touch %s.dir; touch %s.pag",
			dbname, dbname, dbname);
	    (void) system(strv2);
	    if ((mydb = dbmopen(dbname)) == (database *)NULL)
		(void) printf("Open failed\n");
	}
	else if (sscanf(cmdline, "g %s", strv) == 1)
	{
	    static char	    *content;
	    unsigned int    clen;

	    if (dbmseek(strv, (unsigned int) strlen(strv),
			mydb,!mydb->dbrdonly) == FAIL)
		(void) printf("%s: no such key\n", strv);
	    else
	    {
		content = dbmget(&clen, mydb);
		content[clen] = '\0';
		(void) printf("%s: value is %s\n", strv, content);
	    }
	}
	else if (cmdline[0] == 'd')
	    (void) dbmdelete(mydb);
	else if (cmdline[0] == 'r')
	    dbmrewind(mydb);
	else if (cmdline[0] == 'n' || cmdline[0] == '\0')
	{
	    if (dbmnext(mydb, !mydb->dbrdonly) == FAIL)
		(void) fprintf(stderr, "No more keys\n");
	    else
		(void) fprintf(stderr,"Next key is: %s\n",mydb->current.dptr);
	}
#ifdef LOCKF
	else if (cmdline[0] == 'u')
	    dbmunlock(mydb);
#endif /* LOCKF */
	else if (sscanf(cmdline, "p %s %s", strv, strv2) == 2)
	    (void) dbmput(strv, (unsigned int) strlen(strv), strv2,
			  (unsigned int) strlen(strv2), mydb);
	else if (cmdline[0] == 'x')
	    exit(SUCCEED);
	else if (cmdline[0] == 'q')
	{
	    dbmclose(mydb);
	    exit(SUCCEED);
	}
	else if (cmdline[0] == 'w')
	{
	    dbmrewind(mydb);
	    while (dbmnext(mydb, !mydb->dbrdonly) != FAIL)
	    {
		char	*content;
		unsigned int clen;

		content = dbmget(&clen, mydb);
		content[clen] = '\0';
		(void) printf("%s: %s\n", mydb->current.dptr, content);
	    }
	}
	else if (cmdline[0] == 't')
	    dbmtrunc(mydb);
	else if (cmdline[0] == '!')
	{
	    (void) system(cmdline + 1);
	    (void) fflush(stdout);	/* V7 system() doesn't flush stdout */
	}
	else if (cmdline[0] == '?')
	{
	    (void) printf("o name    -- open named database\n");
	    (void) printf("g key     -- see value of key <key>\n");
	    (void) printf("p key val -- set value of key <key>\n");
	    (void) printf("d         -- delete current datum\n");
	    (void) printf("r         -- rewind to first datum\n");
	    (void) printf("n,\\n      -- step to next datum\n");
#ifdef LOCKF
	    (void) printf("u         -- unlock current item\n");
#endif /* LOCKF */
	    (void) printf("w         -- list all key-content pairs\n");
	    (void) printf("t         -- truncate database\n");

	    (void) printf("x         -- exit (close database)\n");
	    (void) printf("q         -- quit\n\n");

	    (void) printf("! cmd     -- execute shell command\n");
	    (void) printf("?         -- print this help message\n");
	}
	else
	    (void) printf("Illegal command -- type ? for help\n");
    }
}
#endif /* MAIN */

/* edbm.c ends here */
