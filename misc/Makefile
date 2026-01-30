# Makefile for the nntpxfer.c utility

# source directory for TMNN
NEWSDIR=/usr/src/local/news30/src

# news directory destination (LIBDIR/misc)
DESTDIR=/usr/local/lib/news30/misc

nntpxfer:	nntpxfer.o ${NEWSDIR}/libnews.a ${NEWSDIR}/libport.a
	cc nntpxfer.o -o nntpxfer -L${NEWSDIR} -lnews -lport

nntpxfer.o:	nntpxfer.c
	cc -O -I${NEWSDIR} -I${NEWSDIR}/D.port -I${NEWSDIR}/D.news -c nntpxfer.c

clean:
	-rm -f *.o nntpxfer make*.out 

distrib: clean
	rm -rf SCCS

install:	nntpxfer
	cp nntpxfer ${DESTDIR}
	chown news ${DESTDIR}/nntpxfer
	chgrp news ${DESTDIR}/nntpxfer
	chmod 700  ${DESTDIR}/nntpxfer

check:;
