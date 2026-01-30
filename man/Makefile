# Rules for manual page makes

ROFF = nroff

.SUFFIXES: .1 .3 .5 .7 .8 .lp .lpr

MAN1 = checknews.1 ednews.1 edvnews.1 inews.1 mailnews.1 makekits.1 \
	postnews.1 readnews.1 vnews.1
MAN3 = getdate.3
MAN5 = news.5 newsreaders.5
MAN7 = mn.7
MAN8 = expire.8 rnews.8 sendbatch.8 uurec.8
MAN = $(MAN1) $(MAN3) $(MAN5) $(MAN7) $(MAN8) 

# This will fail on some archaic V7/BSD systems. Get a real make!
TARGETS = $(MAN1:.1=.lp) $(MAN3:.3=.lp) $(MAN5:.5=.lp) $(MAN7:.7=.lp) $(MAN8:.8=.lp)
all: $(TARGETS)

# This is the production used for creating the formatted versions
.1.lp .3.lp .5.lp .7.lp .8.lp:
	tbl $< | $(ROFF) -Ttn300 -man - >$*.lp

# Plug in your favorite printer here
POST = mx80	# This is a local hack for the author's home site
.1.lpr .3.lpr .5.lpr .7.lpr .8.lpr:
	tbl $< | $(ROFF) -T$(POST) -man - | $(POST) >$*.lpr

clean:
	rm -f $(TARGETS) *~
