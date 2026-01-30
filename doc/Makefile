# Rules for documentation makes

ROFF = nroff

.SUFFIXES: .mn .lp .lpr

MAKEAGE=howto.lp install.lp hackguide.lp
USAGE=manners.lp newgroup.lp
ALL = $(MAKEAGE) $(USAGE)

all: $(ALL)

# This is the production used for creating the formatted versions
.mn.lp:
	tbl $*.mn | $(ROFF) -Ttn300 tmac.n - | col >$*.lp

# Plug in your favorite printer here
POST = mx80	# This is a local hack for the author's home site
.mn.lpr:
	tbl $*.mn | $(ROFF) -T$(POST) tmac.n - | $(POST) | col -p >$*.lpr

clean:
	rm -f $(MAKEAGE) $(USAGE)
