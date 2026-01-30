: 'newslogs -- keep log file trimmed'
:
: 'If this is run once a day, it will save the last'
: 'weeks worth of news log files. You can, of course, comment'
: 'out some of the lines to save less.'
cd ADMDIR

mv oldlog.5 oldlog.6
mv oldlog.4 oldlog.5
mv oldlog.3 oldlog.4
mv oldlog.2 oldlog.3
mv oldlog.1 oldlog.2
mv oldlog.0 oldlog.1
mv log oldlog.0
cp /dev/null log

mv olderrs.5 olderrs.6
mv olderrs.4 olderrs.5
mv olderrs.3 olderrs.4
mv olderrs.2 olderrs.3
mv olderrs.1 olderrs.2
mv olderrs.0 olderrs.1
mv errlog olderrs.0
cp /dev/null errlog

/etc/chown NEWSUSR log* history*
