#!/usr/bin/python

import sys
import os
import time

while True:
	time.sleep(60)
	for f in ["log_funcstats", "log_distance", "log_jmppatch", "log_GoLibMatcher"]:
		if os.path.exists(f) == True:
			print "Deleting "+f
			os.remove(f)
