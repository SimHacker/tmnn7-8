: newsspace -- print filespace used by each newsgroup, largest-first
:	adapted for 3.0 from G A Moffett's mod.sources submission

if test ! -r ADMDIR/active
then
	echo "$0: can't open active file" 1>&2
	exit 1
fi

if ! -d TEXTDIR
then
	echo "$0: no TEXTDIR directory!" 1>&2
	exit 1
fi

cd TEXTDIR

: the awk program takes these measurements of the directories and
: redistributes the weights of specific newsgroups; that is, the size of
: net/music/gdead is subtracted from the the size of net/music, and so on.

du -s `awk '{ print $1 }' ADMDIR/active | tr '.' '/'` |
awk '
	{ newsgroups[$2] = $1 }

END {
	for (ng in newsgroups) {
		n = split(ng, l, "/")
		tng = l[1]		# parent newsgroups of ng

		: look at each parent (?ancestor) of this newsgroup
		for (i = 2; i <= n; i++)
		{
			: subtract total of this newsgroup from total
			:	of parent newsgroups
			if (newsgroups[tng] > 0)
				newsgroups[tng] -= newsgroups[ng]
			tng = tng "/" l[i]
		}
	}

	: print them out (only non-empty ones, tab-separated)
	for (ng in newsgroups) {
		if (newsgroups[ng] > 0)
			print newsgroups[ng], "\t", ng
	}
}' |
	tr '/' '.'  |		: restore standard newsgroup names
	sort +0nr		: largest newsgroups first

: newsspace ends here
