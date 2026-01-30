# Various utility productions for making distributions --
# don't count on them to be appropriate for your configuration.
SHELL=/bin/sh

MANIFEST:
	cd src; make distclean
	find . -print | sed \
		-e '/beta/d' \
		-e '/junk/d' \
		-e '/notes/d' \
		-e '/profiled/d' \
		-e '/\.[oa]$$/d' \
		-e '/.*~$$/d' \
		-e '/^#.*#/d' \
		-e '/\.ln$$/d' \
		-e '/\.log$$/d' \
		-e '/Makefile$$/d' \
		-e '/config.*\.h/d' \
		-e '/RCS/d' -e '/,v/d' \
		-e '/SCCS/d' -e '/\/s\./d' \
		-e '/D.include/d' \
		-e '/D.port\/getdate.c/d' \
		-e '/D.scrn\/curses.h/d' \
		-e '/D.scripts\/config.sed/d' \
		-e '/TAGS/d' \
		-e '/core/d' \
		-e '/a.out/d' \
		-e '/shlib.ifile/d' \
	| sort >MANIFEST

KITOPTS = -m 50000 -c "compress -b12"
BACKDEV = /dev/dsk/f03ht
TIMEMARK = ../junk/RELEASES
KITDIR = ../beta/kits

ISBACKUP = -name '*~' -o -name "#*"
ISORIG = -name '*\.orig' -o -name '*\.ori' -o -name '*\.or'
ISREJ = -name '*\.rej' -o -name '*\.re' -o -name '*\.r'
JUNK = $(ISBACKUP) -o $(ISORIG) -o $(ISREJ) -o -name '*/core'
distclean:
	cd src; make distclean
	@ find src man doc misc $(JUNK) -exec rm {} \;

backup:
	@ cpio -oBc <MANIFEST >$(BACKDEV)

BACKDIR=beta
cpio: MANIFEST
	cpio -ocv <MANIFEST >$(BACKDIR)/tmnn.cpio

tar:
	make distclean
	cd src; make clean
	mv src/config* .; mv src/Makefile srcMakefile
	tar cvf tmnn.tar READ.ME LICENSE Makefile src man misc doc
	mv config* src; mv srcMakefile src/Makefile
	compress -v tmnn.tar

kits: MANIFEST
	src/makekits $(KITOPTS) -p newskit <MANIFEST
	mv *.Z $(KITDIR)

FILTER = grep -v "\.[oa]$$" | grep -v "^./beta" | grep -v "^./junk"
newer:
	@find . -newer $(TIMEMARK) -type f -print | $(FILTER)
