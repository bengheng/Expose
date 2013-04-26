#!/usr/bin/python

########################
# CalcStripScore.py
########################

import MySQLdb
import sys
import os
import shutil
import subprocess

# Get the best matched number of files.

if len(sys.argv) != 5:
	print "Usage: "+sys.argv[0]+" <expt> <expt_id> <#> <dest>"
	print "\t<expt_id> Experiment id."
	print "\t<#>       Number of matched files to extract."
	print "\t<dest>    Destination path to copy files to."
	sys.exit(0)

_home = os.environ.get("HOME")+"/"
_alt = "/backup/libmatcher/"
_host = "virtual2.eecs.umich.edu"
_user = "libmatcher"
_passwd = "l1bm@tcher"
_db = "libmatcher"

_mntpath_prefix = _alt+"mnt/"

conn = MySQLdb.connect(host=_host, user=_user, passwd=_passwd, db=_db)

# Get experiment information
conn.query("SELECT client_id, sig_file, IDA_idcfile, neighdepth FROM `expt_approx` WHERE expt_id="+sys.argv[1])
results = conn.store_result()
result = results.fetch_row(0)
client_id = result[0][0]
sig_file = result[0][1]
IDA_idcfile = result[0][2]
neighdepth = result[0][3]

sigext = os.path.splitext(sig_file)[-1]
if sigext != ".dot":
	print "Experiment uses non-DOT signature. Unsupported experiment type."
	sys.exit(-1)

# Get path
conn.query("SELECT nodename FROM `client` WHERE client_id="+str(client_id))
results = conn.store_result()

# Make dir with client name in dest
result = results.fetch_row(0)
clientname = result[0][0]
destpath = sys.argv[3]+"/"+clientname
if not os.path.exists(destpath):
	os.mkdir(destpath)

# Make dir with expt_id
destpath = destpath+"/approx_"+sys.argv[1]
if not os.path.exists(destpath):
	os.mkdir(destpath)

scorefile = open(destpath+"/scores.log", "w")

# Get list of files from database
conn.query("SELECT `file`.path, `file`.name, `m`.score FROM `file` INNER JOIN (SELECT file_id, score FROM `expt_approx` WHERE `expt_approx`.expt_id="+sys.argv[1]+" ORDER BY `expt_approx`.score ASC LIMIT "+sys.argv[2]+") AS `m` ON `m`.file_id=`file`.file_id AND `file`.num_vertices < 10000")
results = conn.store_result()
i = 0
for result in results.fetch_row(0):
	filesrc = _mntpath_prefix+clientname+"/"+result[0]
	filedest = destpath+"/"+str(i)+"_"+result[1]
	shutil.copyfile(filesrc, filedest)
	print filesrc+" => "+filedest+" "+str(result[2])

	# Strip symbols
	proc_strip = subprocess.Popen(["strip", "-s", filedest, "-o", filedest+".nosym"], close_fds=True)

	print IDA_idcfile
	# Generate DOT file
	proc_ida = subprocess.Popen([_home+"idaadv/idal", "-A", "-Cg", "-S"+_home+"libmatcher/bin/ida/idc/libmatcher.idc", "-B", filedest+".nosym"], close_fds=True)
	proc_ida.wait()
	del proc_ida

	proc_matcher = subprocess.Popen([_home+"libmatcher/tools/GraphMatcher.exe", _home+"libmatcher/dot/sbj/"+sig_file, filedest+".nosym.1.dot", str(neighdepth)], close_fds=True, stdout = subprocess.PIPE, stderr = subprocess.PIPE)
	outstr, errstr = proc_matcher.communicate()
	if errstr != None and errstr != "":
		print "Error."
		scorefile.write(str(result[2])+"\t"+errstr+"\t"+sig_file+"\t"+result[0]+"\n")
	else:
		scorefile.write(str(result[2])+"\t"+outstr+"\t"+sig_file+"\t"+result[0]+"\n")
	scorefile.flush()
	i = i + 1

scorefile.close()	
	

# Copy files


conn.close()
del conn
