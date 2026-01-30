: 'newsclean -- run this once every once in a while to clear out sludge'

find TEXTDIR -size 0 -o -mtime +90 -exec rm -f {} \;
rm TEXTDIR/.tmp/*
LIBDIR/expire -r

: next, make sure we can see link data
if test -r /usr/lib/uucp/L.sys; then
	links=/usr/lib/uucp/L.sys
elif test -r /usr/lib/uucp/Systems; then
	links=/usr/lib/uucp/Systems
elif test -r /etc/uucp/Systems; then
	links=/etc/uucp/Systems
elif test -r /etc/uucp/L.sys; then
	links=/etc/uucp/L.sys
else
	exit 1
fi

: if the link data is newer than our map entry, prompt the admin
msg=`find LIBDIR -name uucpmap.form ! -mtime 90 \
	-exec echo "Your UUCP map entry hasn't been updated in some time."`
msg=`find LIBDIR -name uucpmap.form ! -newer $links \
	-exec echo "Your UUCP map data is out of date with respect to $links."`

if test "$msg"
then
	/bin/mail NOTIFY <<EOM
Subject: UUCP map update time

$msg

Please help the suffering mailers of the world -- cd to LIBDIR and run
ConfigureMap (no arguments needed) to regenerate and mail a correct map entry.
EOM
fi

: newsclean.sh ends here
