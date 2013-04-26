#!/usr/bin/python

import sys
import MySQLdb
import subprocess

if len(sys.argv) != 2:
	print "Usage: "+sys.argv[0]+" <clientid>"
	sys.exit(0)

db = MySQLdb.connect(host="localhost", user="libmatcher", passwd="l1bm@tcher", db="libmatcher")


def StartVM(currclient):
	uuid = currclient[3]
	# Check if VM is already running
	proc_ckvm = subprocess.Popen(["VBoxManage", "list", "runningvms"], stdout=subprocess.PIPE)

	found = False
	while True:
		line = proc_ckvm.stdout.readline()
		if line == '' and proc_ckvm.poll() != None:
			break
		if line.find(uuid) != -1:
			found = True
			break
	
	if found is False:
		# If not, start VM	
		print "Starting "+currclient[4]+"."
		proc_vm = subprocess.Popen(["VBoxHeadless", "-startvm", currclient[4],"-p", str(currclient[8])], close_fds=True)
		del proc_vm
	else:
		print "\""+currclient[4]+"\" is already running."

# Get clients
cursor_client = db.cursor()
cursor_client.execute("SELECT * FROM client WHERE active=1 AND cid="+sys.argv[1])

numclients = int(cursor_client.rowcount)
print str(numclients) + " active client found."
if numclients > 0:
	currclient = cursor_client.fetchone()
	StartVM(currclient)

