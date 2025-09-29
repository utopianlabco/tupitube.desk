#
# Regular cron jobs for the tupitube-0.2.23 package.
#
0 4	* * *	root	[ -x /usr/bin/tupitube-0.2.23_maintenance ] && /usr/bin/tupitube-0.2.23_maintenance
