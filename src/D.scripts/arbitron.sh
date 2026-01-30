#! /bin/sh
# arbitron -- this program produces rating sweeps for USENET.
#
# Usage: arbitron
#
# This program will produce a readership survey for your machine and mail that
# survey to the ratings collection point, with a cc to you. You must compile
# with the FEEDBACK configuration switch on for this to work. Due to the
# FEEDBACK hooks, this new version is more accurate than previous ones about
# what groups are actually being read.
#
# To participate in the international monthly ratings sweeps, 
# run "arbitron" every month. I will run the statistics program on the last
# day of each month; it will include any report that has reached it by that
# time. To make sure your site's data is included, run the survey program no
# later than the 20th day of each month.
#
# By default, news configures this program to send your summary to $survey
# over your backbone path. If you have a more direct route to the survey
# collection site, please mung the file to use it. The rest of the
# configuration data should not normally need editing.
#
# Brian Reid, DEC Western Research Lab, reid@decwrl
# Rewritten for use with TMN-netnews Configure, config.sed and FEEDBACK stuff
#	Eric S. Raymond, Thyrsus Enterprises
#
###########################################################################
# Configuration information. Edit this section to reflect your site data. #

# uucp path: {ihnp4, decvax, ucbvax}!decwrl!netsurvey
surveysite="netsurvey@decwrl.dec.com"
summarypath=`echo "BACKBONE NOTIFY" | sed /%s/s//$surveysite/`

# Range of /etc/passwd UID's that represent actual people (rather than
# maintenance accounts or daemons or whatever)
lowUID=5
highUID=9999

PATH=LIBDIR:/usr/local/bin:/usr/ucb:/usr/bin:/bin
export PATH
############################################################################
trap "rm -f arb.aux; exit" 0 1 2 3 15
set `date`
reportdate="$2$6"
destination="${MAILFRONT-mail} $summarypath"

################################
# Next, figure out approximately how many people use this machine.
#
if [ GENUS = "USG" ]
then
	# This covers AT&T UNIX versions
	nusers=`who /etc/wtmp | sort -u +0 -1 | wc -l`
else
	case SPECIES in

	# This covers Berkeley versions
	BSD4*) nusers=`last | sort -u +0 -1 | wc -l`;;

	# This covers V7, BSD2.x, and everybody else.
	# It is fast but usually returns too big a number
	*) nusers=`awk </etc/passwd -F: "
BEGIN					{ N = 0 }
\\$3 >= $lowUID && \\$3 <= $highUID	{ N = N + 1 }
END					{ print N }
"`;;
	esac
fi

################################
#
# Set up awk scripts;  these are too large to pass as arguments on most
# systems.
#
# This awk script generates the actual output report.
# We use 'sed' to substitute in the shell variables to save ourselves
# endless hassle trying to find quoting/backslashing problems.
#
sed -e "/^#/d" \
    -e "s/NUSERS/$nusers/g" \
    -e "s/HOSTNAME/SITENAME/g" \
    -e "s/DATE/$reportdate/g" \
> arb.aux << 'DOG'
BEGIN	{realusers = 0;}
NF==5	{
		# Active file line
		# We do this to make sure all groups are reported on
		groups[$1] = 0;
	}
NF!=5	{
		# Feedback file line
		# First, track the number of real users
		if (readers[$1] == 0)
			realusers++;
		users[$1] = 1;

		# Now update the group count
		split($3, artname, "/");
		groups[artname[1]]++;
	}
END	{
	 	for (i in groups)
			printf("%d %s\n", groups[i], i);

		printf("9999 Host\t\t%s\n","HOSTNAME")
		printf("9998 Users\t\t%d\n",NUSERS)
		printf("9997 NetReaders\t%d\n",realusers)
		printf("9996 ReportDate\t%s\n", DATE)
		printf("9995 SystemType\tmnn-arbitron-1.0\n")
	}
DOG

# make and mail the report
awk -f arb.aux ADMDIR/active ADMDIR/feedback | $destination

# we lose some info if someone exited a reader while this was running, oh well
rm ADMDIR/feedback

# arbitron.sh ends here
