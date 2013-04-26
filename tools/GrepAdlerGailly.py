#!/usr/local/bin/python2.6

import subprocess
import MySQLdb
import sys
import os

if len(sys.argv) != 2:
	print "Usage :"+sys.argv[0]+" <expt_id>"
	sys.exit(0)

_host = "localhost"
_user = "libmatcher"
_passwd = "l1bm@tcher"
_db = "libmatcher"


def GetNodeName(expt_id):
	"""
	Returns the nodename of the client for the specified client_id.
	"""
	conn = MySQLdb.connect( host = _host, user = _user, passwd = _passwd, db = _db )
	conn.query("SELECT nodename FROM client, (SELECT client_id FROM expt_approx WHERE expt_id = %d) AS tmp1 WHERE `tmp1`.client_id = `client`.client_id" % expt_id )
	res = conn.store_result()
	nodename = res.fetch_row(0)[0][0]
	conn.close()

	return nodename
	
	
# Gets the nodename for the client
nodename = GetNodeName(int(sys.argv[1]))

conn = MySQLdb.connect( host = _host, user = _user, passwd = _passwd, db = _db )
conn.query("SELECT result_id, `path` FROM `result_approx`, `file` WHERE `file`.file_id = `result_approx`.file_id")
res = conn.store_result()

for r in res.fetch_row(0):
	result_id = r[0]
	path = r[1]

	filepath = os.path.realpath( os.path.join( os.environ.get("HOME"), "libmatcher", "mnt", nodename ) + "/" + path )

	proc_strings = subprocess.Popen(["strings", filepath], stdout=subprocess.PIPE)
	proc_grep_adler = subprocess.Popen( ["grep", "Adler"], stdin = proc_strings.stdout, stdout = subprocess.PIPE )
	str_adler = proc_grep_adler.communicate()[0]

	proc_strings = subprocess.Popen(["strings", filepath], stdout=subprocess.PIPE)
	proc_grep_gailly = subprocess.Popen( ["grep", "Gailly"], stdin = proc_strings.stdout, stdout = subprocess.PIPE )
	str_gailly = proc_grep_gailly.communicate()[0]

	strout = str_adler+str_gailly
	if strout != "":
		print str(result_id)+": \""+strout+"\""
		conn1 = MySQLdb.connect( host = _host, user = _user, passwd = _passwd, db = _db )
		conn1.query("UPDATE `result_approx` SET comments=\""+strout+"\" WHERE `result_id`="+str(result_id))
		conn1.close()

conn.close()

