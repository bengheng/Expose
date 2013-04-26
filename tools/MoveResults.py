#!/tmp/local/Python-2.7/bin/python

import os
import shutil
import time

files = [ "expt_64011.sql", "expt_64012.sql", "expt_64013.sql", "expt_64014.sql",\
	"expt_64111.sql", "expt_64112.sql", "expt_64113.sql", "expt_64114.sql" ]

while True:
	time.sleep(3)
	for f in files:
		if os.path.exists(f):
			src = f
			dst = os.path.join( os.path.expanduser("~/libmatcher/tools/results"), f )
			print "Moving "+f+" to "+dst
			shutil.move(src, dst )

