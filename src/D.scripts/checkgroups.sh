: 'checkgroups -- check active file for missing or extra newsgroups'

trap 'rm -f /tmp/$$*' 0 1 2 15

if test  ! -s LIBDIR/newsgroups
then
	cp /dev/null LIBDIR/newsgroups
fi
# Read first line of stdin.  If of the form "-n group", then only check
# for the specified group.  Otherwise, assume doing standard groups
sed -e '/^[a-zA-Z-]*: /d' -e '/^$/d' -e '/^[#:]/d' | (
read line
case "${line}" in
-n*)
	# Doing specific group. Extract the group name and make nocheck
	# all of current newsgroups file except for that group.
	group=`echo ${line} | sed -e 's/-n /^/' -e 's/$/\\\\./'`
	egrep -v "${group}" LIBDIR/newsgroups >/tmp/$$nocheck
	cat /tmp/$$nocheck - >LIBDIR/newsgroups
	;;
*)
	egrep -v "GFILTER" LIBDIR/newsgroups >/tmp/$$localgroups
	cat /tmp/$$localgroups >LIBDIR/newsgroups
	echo "${line}" >>LIBDIR/newsgroups
	cat >>LIBDIR/newsgroups
	group="GFILTER|^general|^junk|^control|^to.SITENAME"
	;;
esac

echo general >/tmp/$$goodgroups
echo junk >>/tmp/$$goodgroups
echo control >>/tmp/$$goodgroups
echo to.SITENAME >>/tmp/$$goodgroups
awk "\$1 ~ /${group}/ {print \$1;}" <LIBDIR/newsgroups >>/tmp/$$goodgroups
sort -u /tmp/$$goodgroups -o /tmp/$$goodgroups
awk "\$1 ~ /${group}/ {print \$1;}" <ADMDIR/active | sort -u >/tmp/$$checklist

awk '/[Mm]oderat/ {print $1;}' <LIBDIR/newsgroups | sort -u >/tmp/$$modlist
awk '$4 ~ /m/ {print $1;}' <ADMDIR/active | sort >/tmp/$$modgroups

comm -13 /tmp/$$goodgroups /tmp/$$checklist >/tmp/$$remove
comm -23 /tmp/$$goodgroups /tmp/$$checklist >/tmp/$$add
comm -13 /tmp/$$modlist /tmp/$$modgroups >/tmp/$$ismod
comm -23 /tmp/$$modlist /tmp/$$modgroups >/tmp/$$notmod

if test -s /tmp/$$remove
then
	(
	echo "# The following newsgroups are not valid and should be removed."
	sed "s/^/	/" /tmp/$$remove
	echo ""
	echo "# You can do this by executing the command:"
	echo \	LIBDIR/rmgroup `cat /tmp/$$remove`
	echo ""
	) 2>&1 >/tmp/$$out
fi

if test -s /tmp/$$add
then
	(
	echo "# The following newsgroups were missing and should be added."
	sed "s/^/	/" /tmp/$$add
	echo ""
	echo "# You can do this by executing the command(s):"
	for i in `cat /tmp/$$add`
	do
		echo 'LIBDIR/inews -C '$i' </dev/null'
	done
	echo ""
	) 2>&1 >>/tmp/$$out
fi

if test -s /tmp/$$ismod
then
	(
	echo "# The following newsgroups are not moderated and are marked moderated."
	sed "s/^/	/" /tmp/$$ismod
	echo ""
	echo "# You can correct this by executing the command(s):"
	for i in `cat /tmp/$$ismod`
	do
		echo 'LIBDIR/inews -C '$i' </dev/null'
	done
	echo ""
	) 2>&1 >>/tmp/$$out
fi

if test -s /tmp/$$notmod
then
	(
	echo "# The following newsgroups are moderated and not marked so."
	sed "s/^/	/" /tmp/$$notmod
	echo ""
	echo "# You can correct this by executing the command(s):"
	for i in `cat /tmp/$$notmod`
	do
		echo 'LIBDIR/inews -C '$i' moderated </dev/null'
	done
	echo ""
	) 2>&1 >>/tmp/$$out
fi

if test -s /tmp/$$out
then
	(echo	"Subject: Problems with your active file"
	echo ""
	cat /tmp/$$out
	) | if test $# -gt 0
		then
			mail $1
		else
			cat
		fi	
fi
)

: 'checkgroups ends here'
