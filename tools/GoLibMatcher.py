#!/usr/bin/env python

import subprocess
import os
import time
import MySQLdb
import hashlib
import datetime
import threading
import Queue
import sys
import uuid
import IDAThread
import ClientThread
import LMContext
import multiprocessing
import socket
import fileinfo
from Subject import Subject

if len(sys.argv) != 5 and len(sys.argv) != 6:
	print "Usage: "+sys.argv[0]+" <expt> <expt_id> <ngram#> <mode> <how>"
	print "\t<expt>        \"exact\" - Exact matching experiment."
	print "\t              \"approx\" - Approximate matching experiment."
	print "\t<expt_id>     Experiment ID."
	print "\t<ngram#>      Ngram size."
	print "\t<mode>        \"subject\"\t\t- Only updates subject."
	print "\t              \"replace\"\t\t- Replace scores of existing scores if present. Otherwise insert."
	print "\t              \"resume\"\t\t- Skip existing scores if present. Otherwise insert."
	print "\t              \"repair\"\t\t- Performs update only if no valid match result in database."
	print "\t<how>         \"complete\"\t- Check each file in client."
	print "\t              \"fast\"\t\t- Use pre-generated db files."
	print "\t              \"clean\"\t\t- Delete or unlink non 32/64 bits shared or executable binaries."
	sys.exit(1)

home = os.environ.get("HOME")
LMContext.lmhome = os.path.realpath(home+"/libmatcher/")
LMContext.dotpath = os.path.realpath(LMContext.lmhome+"/dot/")
LMContext.idbpath = os.path.realpath(LMContext.lmhome+"/idb/")
LMContext.mntpath_prefix = os.path.realpath(LMContext.lmhome+"/mnt/")

###############################################################################
# SANITY CHECKS ON USER INPUTS
###############################################################################
if os.sys.platform == "win32":
	print "Not supported on Windows because close_fds cannot be used."
	exit(1)

LMContext.ngram = int(sys.argv[3])

# Get mode, "subject", "replace", "resume" or "repair".
LMContext.mode = sys.argv[4]
if LMContext.mode != "subject" and\
   LMContext.mode != "replace" and\
   LMContext.mode != "resume" and\
   LMContext.mode != "repair":
	print "Wrong mode."
	sys.exit(1)

# Get how (valid only when mode is not "subject"), "complete", "fast" or "clean".
if LMContext.mode != "subject":
	LMContext.how = sys.argv[5]
	if LMContext.how != "complete" and\
	   LMContext.how != "fast" and\
	   LMContext.how != "clean":
		print "Wrong how."
		sys.exit(1)


# Get expt and expt_id
LMContext.expt = sys.argv[1]
if LMContext.expt != "approx" and \
LMContext.expt != "exact":
	print "Wrong expt."
	sys.exit(1) 
LMContext.expt_id = sys.argv[2]

###############################################################################
# INIT MYSQL SERVER PARAMETERS
###############################################################################

# Get hostname. If we're on the same machine as
# MySQL server, use "localhost" as hostname.
LMContext.host = socket.gethostbyaddr(socket.gethostname())[0]
if LMContext.host == "virtual2.eecs.umich.edu":
	LMContext.host = "localhost"
else:
	LMContext.host = "virtual2.eecs.umich.edu"

LMContext.user = "libmatcher"
LMContext.passwd = "l1bm@tcher"
LMContext.db = "expt_"+LMContext.expt_id

###############################################################################
# GET EXPERIMENT INFO FROM DB
###############################################################################

conn_expr = MySQLdb.connect(host=LMContext.host, user=LMContext.user, \
	passwd=LMContext.passwd, db=LMContext.db)
conn_expr.query("SELECT * FROM expt_"+LMContext.expt+" WHERE expt_id="+LMContext.expt_id)
res_expr = conn_expr.store_result()
if res_expr.num_rows() != 1:
	print "Expected 1 expt record, but found "+str(res_expr.num_rows())+" not found!\n"
	sys.exit(1)
LMContext.experiment = res_expr.fetch_row(0)[0]
del res_expr
conn_expr.close()
del conn_expr

LMContext.timeout_sec = LMContext.experiment[6]
print "Timeout (sec): "+str(LMContext.timeout_sec)

###############################################################################
# GET CLIENT INFO FROM DB
###############################################################################
LMContext.cid = LMContext.experiment[1]
print "Client ID:\t"+str(LMContext.cid)

conn_client = MySQLdb.connect(host=LMContext.host, user=LMContext.user, \
	passwd=LMContext.passwd, db=LMContext.db)
conn_client.query("SELECT * FROM client WHERE client_id="+str(LMContext.cid))
res_client = conn_client.store_result()
if res_client.num_rows() != 1:
	print "Expected 1 client record, but found "+str(res_client.num_rows())+"!\n"
	sys.exit(1)
currclient = res_client.fetch_row(0)[0]
del res_client
conn_client.close()
del conn_client

LMContext.bits = currclient[2]
print "Client bits:\t"+str(LMContext.bits)
if LMContext.bits == 32:
	LMContext.idbext = ".idb"
elif LMContext.bits == 64:
	LMContext.idbext = ".i64"

###############################################################################
# GET VULN INFO FROM DB
###############################################################################
#
#Gets vuln info from database
#
conn_vuln = MySQLdb.connect(host=LMContext.host, user=LMContext.user, \
	passwd=LMContext.passwd, db=LMContext.db)
conn_vuln.query("SELECT vuln.vuln_id, vuln.file_id FROM vuln, expt_"+LMContext.expt+" "+\
	"WHERE expt_"+LMContext.expt+".vuln_id = vuln.vuln_id "\
	"AND expt_"+LMContext.expt+".expt_id = "+LMContext.expt_id)
res_vuln = conn_vuln.store_result()
if res_vuln.num_rows() != 1:
	print "Expected 1 vuln record, but found "+str(res_vuln.num_rows())+"!\n"
	sys.exit(1)
LMContext.vuln = res_vuln.fetch_row(0)[0]
del res_vuln

#
# Update vuln file info
#
subject = Subject( LMContext.vuln[0], LMContext.ngram, LMContext.bits, \
	LMContext.host, LMContext.user, LMContext.passwd, LMContext.db )
subject.UpdSbjFile()
subject.AdjustFileAutoInc()

#
# Some simple verifications
#
conn_vuln.query("SELECT * FROM file WHERE file_id="+str(LMContext.vuln[1]))
res_vulnfile = conn_vuln.store_result()
if res_vulnfile.num_rows() != 1:
	print "Expected 1 vuln file record, but found "+str(res_vulnfile.num_rows())+"!\n"
	sys.exit(1)
LMContext.vulnfile = res_vulnfile.fetch_row(0)[0]
del res_vulnfile

conn_vuln.query("SELECT * FROM client WHERE client_id="+str(LMContext.vulnfile[1]))
res_vulnclient = conn_vuln.store_result()
if res_vulnclient.num_rows() != 1:
	print "Expected 1 vuln client record, but found "+str(res_vulnclient.num_rows())+"!\n"
	sys.exit(1)
LMContext.vulnclient = res_vulnclient.fetch_row(0)[0]
del res_vulnclient
conn_vuln.close()
del conn_vuln
print "Vuln file_id:\t"+str(LMContext.vulnfile[0])
print "Vuln client:\t"+str(LMContext.vulnclient[1])
# print "Vuln bits:\t"+str(LMContext.vulnclient[2])

if LMContext.mode == "subject":
	print "Done updating subject."
	sys.exit(0)

###############################################################################
# SET UP IDA INFO
###############################################################################

# Get IDA's path. This may differ on different systems.
if LMContext.bits == 32:
	LMContext.idapath = os.path.realpath(home+"/idaadv/idal")
elif LMContext.bits == 64:
	LMContext.idapath = os.path.realpath(home+"/idaadv/idal64")

while(not os.path.isfile(LMContext.idapath)):
	LMContext.idapath = raw_input("Cannot find \""+LMContext.idapath+"\". Enter absolute path. ")
print "Using \""+LMContext.idapath+"\""

# Set environment variable TVHEADLESS to make IDA Pro Adv happy.
# It cannot run with redirected output.
os.environ["TVHEADLESS"] = "1"



#if LMContext.expt == "approx":
#	LMContext.neighbordepth = LMContext.experiment[9]
#	print "NeighborDepth: "+str(LMContext.neighbordepth)
		
# Compute threshold size for graphs to be considered big
#totalMemory = int( os.popen("free").readlines()[1].split()[1] )
#LMContext.bigThresh = int( round(totalMemory / 12, 0) )
#LMContext.bigThresh = int( round(totalMemory / 50, 0) )
#print "Big Thresh: "+str(LMContext.bigThresh)

LMContext.ClRdyEvt = threading.Event()
LMContext.ClRdyEvt.clear()
# Start ClientThread
clientthd = ClientThread.ClientThread(currclient)
print "Starting ClientThread for "+currclient[1]+"..."
clientthd.start()
# Wait until the client thread is started before proceeding
LMContext.ClRdyEvt.wait()

# Create RdyEvent, for signalling to IDAThread that
# ClientThread has either queued 1 work, or there isn't
# any work to be queued.
LMContext.RdyEvent = threading.Event()
LMContext.RdyEvent.clear()
	
# Create IDAWorkQueueLock
LMContext.semaCount = multiprocessing.cpu_count()
#print "Sema Count: "+str(LMContext.semaCount)
#LMContext.IDAWorkQueueLock = threading.BoundedSemaphore(LMContext.semaCount)
LMContext.IDAWorkQueueLock = threading.Lock()
LMContext.BigFileLock = threading.Lock()
# Create Queue with size equals number of CPUs.
# Insertion is blocked if no slots available.
LMContext.IDAWorkQueue = Queue.Queue(multiprocessing.cpu_count())


time_conn = MySQLdb.connect(host=LMContext.host, user=LMContext.user, \
	passwd=LMContext.passwd, db=LMContext.db)
time_conn.query("UPDATE expt_"+LMContext.expt+" SET start=NOW() WHERE expt_id="+LMContext.expt_id)
time_conn.close()
del time_conn
	
	
# Initialize list for missing dots
#LMContext.missingdots = list()


if LMContext.how != "clean":
	# Start 1 IDAThread for each process
	proc_count = multiprocessing.cpu_count()
	IDAthds = []
	for threadid in range(0, proc_count):
		IDAThd = IDAThread.IDAThread(threadid)
		print "Starting IDAThread #"+str(threadid)+"..."
		IDAThd.start()
		IDAthds.append((threadid, IDAThd))
	
	# Wait for IDAThd
	for threadid, IDAThd in IDAthds:
		IDAThd.join()
		print "#"+str(threadid)+" returned."
		del IDAThd

# Wait for client thread to return
clientthd.join()
print "Waiting for \"" + str(currclient[1]) +"\" to return..."
print "\"" + str(currclient[1]) + "\" returned."
del currclient
del clientthd

print str(LMContext.numfilesqueued)+" files queued."
print str(LMContext.workcount)+" files retrieved."
print str(LMContext.shortlist)+" files shortlisted."
print str(LMContext.filecount)+" scores inserted or replaced."
#print str(len(LMContext.missingdots))+" DOT files missing."
#for m in LMContext.missingdots:
#	print "Missing DOT: "+m
	
time_conn = MySQLdb.connect(host=LMContext.host, user=LMContext.user, \
	passwd=LMContext.passwd, db=LMContext.db)
time_conn.query("UPDATE expt_"+LMContext.expt+" SET stop=NOW() WHERE expt_id="+LMContext.expt_id)
time_conn.close()
del time_conn

print "Completed expt_id "+LMContext.expt_id+"."

