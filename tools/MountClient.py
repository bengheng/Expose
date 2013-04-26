#!/usr/bin/python

import os
import sys
import MySQLdb
import subprocess

if len(sys.argv) != 3:
	print "Usage: "+sys.argv[0]+" <clientid> <mntext>"
	sys.exit(0)

db = MySQLdb.connect(host="localhost", user="libmatcher", passwd="l1bm@tcher", db="libmatcher")

home = os.environ.get("HOME")+"/"
lmhome = home+"libmatcher/"
mntpath_prefix = lmhome+"mnt/"
mntext = sys.argv[2]		# Allows user to specify subfolder of client to be mounted, e.g. "bin" folder in client

def IsMounted(mountpath):
	print "Checking if mountpath \""+mountpath+"\" is already mounted..."
	proc = subprocess.Popen("cat /etc/mtab | grep \" "+mountpath+" \"", shell=True, stdout=subprocess.PIPE)
	proc.wait()
	out = repr(proc.communicate()[0])
	if out == "''":
		print "Not mounted."
		return False
	return True

# Check if VM is already running
def IsVMRunning(uuid):
	proc_ckvm = subprocess.Popen(["VBoxManage", "list", "runningvms"], stdout=subprocess.PIPE)

	found = False
	while True:
		line = proc_ckvm.stdout.readline()
		if line == '' and proc_ckvm.poll() != None:
			break
		if line.find(uuid) != -1:
			found = True
			break
	return found

def MountPath(currclient, mntpath):
	if not os.path.exists(mntpath):
		os.mkdir(mntpath);
	print "Mounting "+mntpath
	proc_sshfs = subprocess.Popen(["sshfs", "-o" ,"ro", currclient[5]+"@"+currclient[6]+":/"+mntext, mntpath, "-p", str(currclient[7])], close_fds=True)
	proc_sshfs.wait()

# Get clients
cursor_client = db.cursor()
cursor_client.execute("SELECT * FROM client WHERE active=1 AND cid="+sys.argv[1])

numclients = int(cursor_client.rowcount)
print str(numclients) + " active client found."
if numclients > 0:
	currclient = cursor_client.fetchone()
	mntpath = str(mntpath_prefix+currclient[10]+mntext)
	if IsVMRunning(currclient[3]) == False:
		print "\""+currclient[4]+"\" is not running."
	elif IsMounted(mntpath) == True:
		print "Already mounted "+mntpath+"."
	else:
		MountPath(currclient, mntpath)


