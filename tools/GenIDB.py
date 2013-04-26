#! /usr/local/bin/python2.6

import os
import sys
import hashlib
import subprocess
import socket
import time
import multiprocessing
import threading
import signal

if len(sys.argv) != 3:
	print "Usage: %s <nodename> <32/64>" % sys.argv[0]
	sys.exit(0)

# Define WorkThread
class WorkThread(threading.Thread):
	def __init__(self, threadid):
		self.id = threadid
		threading.Thread.__init__(self)

	def run(self):
	
		while len(work) > 0:
			# Pops first work item
			mutex.acquire()
			shortpath, longpath, path_sha1, idbfile = work.pop(0)
			mutex.release()

			id0 = os.path.join( idbpath, path_sha1+".id0" )
			id1 = os.path.join( idbpath, path_sha1+".id1" )
			nam = os.path.join( idbpath, path_sha1+".nam" )
			til = os.path.join( idbpath, path_sha1+".til" )

			if os.path.exists( id0 ) == True:
				os.remove( id0 )
			if os.path.exists( id1 ) == True:
				os.remove( id1 )
			if os.path.exists( nam ) == True:
				os.remove( nam )
			if os.path.exists( til ) == True:
				os.remove( til )

			print "%d %s %s (ida)"%(len(work), idbfile, shortpath)

			startTime = time.time()
			proc_ida = subprocess.Popen( [idapath, "-A", "-Cg", "-o"+idbfile, "-c",\
				"-S"+idcscript, "-B", "-P", longpath], stdout = subprocess.PIPE )

			# Calcs the time the process should end
			endTime = time.time() + timeout_sec

			count = 0

			# While the proc_ida hasn't return and it's not endTime yet,
			# go to sleep for 0.5 ms
			while proc_ida.poll() == None and endTime > time.time():
				time.sleep(0.5)
				if (count % 10) == 0:
					print "%d %s %s (Time left: %d secs)"%(len(work), path_sha1+idbext, shortpath, endTime - time.time())
				count = count+1

			# If timeout reached, kill the process
			isTimeout = False
			if endTime < time.time():
				print "%d %s %s (timeout)"%(len(work), path_sha1+idbext, shortpath)
				logfile.write(path_sha1+idbext+"\ttimeout\n")
				logfile.flush()
				os.kill( proc_ida.pid, signal.SIGKILL )
				isTimeout = True
				

			del proc_ida
		
			if os.path.exists( idbfile ) is False:
				print "Still cannot find %s!" % idbfile
				if isTimeout == False:
					logfile.write(path_sha1+idbext+"\tmissing\n")
					logfile.flush()
			else:
				print "%d %s %s (done)"%(len(work), path_sha1+idbext, shortpath)


home = os.environ.get("HOME")

binpath = os.path.join(home, "libmatcher", "mnt", sys.argv[1])
if os.path.exists(binpath) is False:
	print "Are you sure \"%s\" exists? I can't find it."%binpath
	sys.exit(0)

idbpath = os.path.join("/tmp", sys.argv[1])
if os.path.exists(idbpath) is False:
	print "Creating folder \"%s\"..."%idbpath
	os.makedirs(idbpath)

idbext = None
idapth = None
if sys.argv[2] == "32":
	idbext = ".idb"
	idapath = os.path.join( home, "idaadv", "idal" )
elif sys.argv[2] == "64":
	idbext = ".i64"
	idapath = os.path.join( home, "idaadv", "idal64" )
print "Using "+idapath

idcscript = os.path.join( home, "libmatcher", "bin", "ida", "idc", "genidb.idc" )

os.environ["TVHEADLESS"] = "1"

# Set timeout in secs
timeout_sec = 60 * 60
print "Timeout: "+str(timeout_sec)+" sec"

# Create empty work list
work = list()

# Create semaphore
mutex = threading.BoundedSemaphore(1)


# Log file
logfile = open( os.path.splitext(sys.argv[0])[0]+"."+socket.gethostname().split(".")[0]+".log", "a" )

exists = 0

# Store all the filenames
for dirpath, dirnames, filenames in os.walk(binpath):
	for f in filenames:
		longpath = os.path.join( dirpath, f )

		proc_file = subprocess.Popen( ["file", longpath], \
			stdout = subprocess.PIPE, stdin = subprocess.PIPE )
		out, err = proc_file.communicate()
		del proc_file
		del err
		if out.find( sys.argv[2]+"-bit LSB executable" ) == -1 and\
		out.find( sys.argv[2]+"-bit LSB shared" ) == -1 :
			print "Skipping "+longpath
			del out
			continue
		del out


		shortpath = longpath.split(binpath)[1]
		path_sha1 = hashlib.sha1(shortpath).hexdigest()
		idbfile = os.path.join( idbpath, path_sha1+idbext )

		# Skip if idb already exists
		if os.path.exists( idbfile ) is True:
			print "%s %s (exists in %s)"%(path_sha1+idbext, shortpath, idbfile)
			del shortpath
			del path_sha1
			del idbfile
			exists = exists + 1
			continue
		else:
			print "Adding "+shortpath
			work.append( (shortpath, longpath, path_sha1, idbfile) )

print "%d files already exist. Total %d files to process."%(exists, len(work))

# Create threads
proc_count = multiprocessing.cpu_count()
workThds = []
for threadid in range(0, proc_count):
	workThd = WorkThread(threadid)
	workThd.start()
	workThds.append( (threadid, workThd) )

# Wait for work threads to return
for threadid, workThd in workThds:
	workThd.join()
	del workThd

logfile.close()


