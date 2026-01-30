#!/bin/sh
# mkbatch.sh -- generate test and timing batches from existing news
#
# Takes a list of group names as arguments
#

dirs=`echo $* | sed -e "s/\./\//g" -e "s:^:$nsp/:"`
find $dirs -name '*[0123456789]' -print | \
	grep -v "\.rnews" | grep -v "\.tmp" | \
	while read filename;
	do
		set - `wc -c $filename`
		echo "#! rnews $1"
		cat $filename
	done
# mkbatch.sh ends here
