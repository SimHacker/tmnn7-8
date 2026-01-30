#! /bin/sh
#
# newsarchive.sh -- archive a message from the specified group
#
# Archive a source file (from stdin) in the source archive directory.
# You can feed this script directly from rnews via lines in 'feeds'
# 
# sources:comp.sources.unix:world,na::/usr/lib/news/newsarchive mod
# bsdbugs:comp.bugs.4bsd.ucb-fixes:world,na::/usr/lib/news/newsarchive 4bsd-bugs
# 
# and also from users mailing sources in to be archived via some mail
# aliases in /usr/lib/aliases, e.g.:
# 
# 	netsrc: "|/usr/local/lib/news/newsarchive net"
# 	netbug: "|/usr/local/lib/news/newsarchive bug"
# 	netgames: "|/usr/local/lib/news/newsarchive games"
# 
# The original of this script was posted by mcg@omepd.UUCP (Steven McGeady)
#
PATH=/bin:/usr/bin:/usr/ucb:/usr/local/bin
SOURCEDIR=/usr/src/netsrc
LOG=ADMDIR/newsarchive.log
umask 002
if [ $# != 1 ]; then
	d=$SOURCEDIR/misc
else
	d=$SOURCEDIR/$1
fi;
if [ ! -d $d ]; then
	/bin/echo "`date`: $0: can't access $d" >> $LOG
	d=$SOURCEDIR/misc
fi;
cd $d;
# compute the next index number
if [ -s cur ]; then
	cur=`cat cur`
else
	cur=1
fi;
# see if this number is already in use
while [ -f $cur ]; do
	cur=`expr "$cur" + 1`
done;
expr "$cur" + 1 > cur
# save the article, deleting any spurious header info
cat > $cur
chmod 0444 $cur
# do some checking
subject=`head -25 $cur | grep '^Subject: ' | tail -1`
msg_id=`head -50 $cur | grep '^Message-ID: '`
msg_id=`expr "$msg_id" : '^Message-ID:  *<\([^>]*\)>'`
# if the message-id appears in the INDEX, it's a duplicate
if [ "$msg_id" != "" ]; then
	if grep -s "$msg_id" INDEX; then
		rm -f $cur
		/bin/echo "$cur: DUPLICATE $msg_id ($subject)" >> INDEX
		/bin/echo "$cur: DUPLICATE $msg_id ($subject)" >> $LOG
		exit;
	fi;
fi;
# pick first word of Subject line
vol=`expr "$subject" : '^Subject:  *\([^ 	:]\):.*'`

# if it has an 'archive-name' header, save it there
if name=`head -50 $cur | grep '^Archive-name: '`; then
	name=`expr "$name" : '^Archive-name:  *\(.*\)'`
	if [ "$name" != "" ] ; then
		file=`basename "$name"`
		dir=`expr "$name" : "\(.*\)/$file"`
		if [ "$dir" != "" ]; then
			if [ ! -d "$dir" ]; then
				mkdir $dir 2>> $LOG
			fi;
		fi;
		if [ ! -f "$name" ] && cp $cur $name 2>>$LOG; then
			chmod 0444 $name
			rm -f $cur
			/bin/echo "$name: $msg_id: $subject" >> INDEX
		else
			/bin/echo "$cur: '$name' ALREADY EXISTS for $msg_id: $subject" >> INDEX
			/bin/echo "$cur: '$name' ALREADY EXISTS for $msg_id: $subject" >> $LOG
		fi;
	else
		/bin/echo "$cur: $msg_id: $subject" >> INDEX
	fi;
else
	/bin/echo "$cur: $msg_id: $subject" >> INDEX
fi;
# newsarchive.sh ends here
