#!/bin/sh
# recnews -- pass mail to inews for posting
#
#	recnews [newsgroup] [from] [approved-by] [inews-options...]
#
#    This code is supplied mostly for compatibility with older news versions.
#    It is recommended that you always include the to newsgroup, since the
# intuition code in inews is not terrifically smart.  The from user is probably
# appropriate for arpanet mailing lists being funnelled at ucbvax but
# not otherwise.  Sample lines in /usr/lib/aliases (if you run delivermail):
# 
# worldnews: "|/usr/lib/news/mailtonews misc.misc"
# 	Allows you to mail to worldnews rather than using inews.
# 	Intended for humans to mail to.
# 
# post-unix-wizards: "|/usr/lib/news/mailtonews comp.unix.wizards unix-wizards"
# 	Causes mail to post-unix-wizards to be fed into comp.unix.wizards
# 	and the return address forged as unix-wizards on the local
# 	machine.  post-unix-wizards (on the local machine) should
# 	be part of the master mailing list somewhere (on a different
# 	machine.)
# 
# in-gamemasters: "|/usr/lib/news/mailtonews mail.gamemasters '' news"
# 
#     Recnews is primarily useful in remote places on usenet which collect
# mail from mailing lists and funnel them into the network.  It is also
# useful if you like to send mail to some user instead of invoking
# inews -t .. -n .. when you want to submit an article. It is not, however,
# essential to be able to use mailtonews to be able to join usenet.
# 
case $# in
0) exec LIBDIR/inews ;;
1) exec LIBDIR/inews -n $* ;;
2) arg1=$1 shift; exec LIBDIR/inews -n $arg1 -f $* ;;
*) arg1=$1 arg2=$2 shift; shift; exec LIBDIR/inews -n $arg1 -f $arg2 -a $* ;;
esac
# recnews ends here
