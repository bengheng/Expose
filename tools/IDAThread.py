#!/usr/bin/python

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
import LMContext
import signal

#=======================================================================
# IDAThread - Threading class that consumes work from the IDAWorkQueue.
# This is necessary because IDA can only be called serially.
#=======================================================================
class IDAThread(threading.Thread):
	def __init__(self, threadid):
		self.id = threadid
		#self.logfile = logfile
		threading.Thread.__init__(self)

	def run(self):
		print "#"+str(self.id)+ ": Started."

		if LMContext.expt != "exact":
			#self.createFIFO()	
			self.libsig = os.path.join(LMContext.dotpath,\
				LMContext.vulnclient[1],\
				LMContext.vulnfile[10]+".dot")

		sleepcount = 0
		LMContext.RdyEvent.wait()

		while ( LMContext.QCOMPLETE == False ):
			# Acquires IDAWorkQueueLock
			#print "%d Waiting for IDAWorkQueueLock QCOMPLETE IS %d"%(self.id, LMContext.QCOMPLETE)
			#LMContext.IDAWorkQueueLock.acquire()
			#print "%d Acquired IDAWorkQueueLock"%self.id

			#if LMContext.IDAWorkQueue.empty() and LMContext.QCOMPLETE == True:
			#	#LMContext.IDAWorkQueueLock.release()
			#	print "Ending work loop"
			#	break;
	
			# If the work queue is empty, sleep for 1 sec
			# before checking again.  Let the user know if
			# it has been 1 min.	
			if LMContext.IDAWorkQueue.empty():
				# Releases IDAWorkQueueLock
				#LMContext.IDAWorkQueueLock.release()
				time.sleep(0.5)
				sleepcount = sleepcount + 1
				if sleepcount%3 == 0:
					print "#"+str(self.id)+": No work."
				continue	

			# Get work from IDAWorkQueue to work on
			currfile, shortpath, longpath, currclient = LMContext.IDAWorkQueue.get()
			LMContext.workcount = LMContext.workcount + 1
	
			# Releases IDAWorkQueueLock
			#LMContext.IDAWorkQueueLock.release()

			# Do the file matching work
			self.MatchFileWithSubjects(currfile, shortpath, longpath, currclient)

			del currfile
			del longpath

		print "#"+str(self.id)+": complete."
	
	"""
	# Returns True if the work to be done will be huge.
	def WillBeBig(self, libsig, DOTFilename):
		libOrder = self.FindTextCount(libsig, "vertex")
		dotOrder = self.FindTextCount(DOTFilename, "vertex")
		totalOrder = libOrder * dotOrder

		if totalOrder > LMContext.bigThresh:
			print "B"+str(self.id)+": "+DOTFilename+" ("+str(libOrder)+" x "+str(dotOrder)+" = "+str(totalOrder)+")"
			return True
		return False
	"""

	def CreateFIFO(self):
		try:
			self._fifo_in = "/tmp/GraphMatcher"+str(self.id)+"_in"
			self._fifo_out = "/tmp/GraphMatcher"+str(self.id)+"_out"

			# Delete the fifo files if they exist
			if os.path.exists(self._fifo_in):
				os.remove(self._fifo_in)
			if os.path.exists(self._fifo_out):
				os.remove(self._fifo_out)

			os.mkfifo(self._fifo_in)
			os.mkfifo(self._fifo_out)
			
			# Now create the fifo endpoints
	
			self.fifo_out = os.open(self._fifo_out, os.O_RDWR)
			self.fifo_outfile = os.fdopen( self.fifo_out, 'rw')

			self.fifo_in = os.open(self._fifo_in, os.O_RDWR|os.O_NONBLOCK)
			self.fifo_infile = os.fdopen( self.fifo_in, 'w' )

			print "#"+str(self.id)+": Created FIFOs "+self._fifo_out+" and "+self._fifo_in

		except OSError, e:
			print "#"+str(self.id)+": "+str(e)

	def CloseFIFO(self):
		self.fifo_infile.close()
		self.fifo_outfile.close()

	def FindTextCount(self, filename, text):
		proc_grep = subprocess.Popen(["grep", text, filename], stdout=subprocess.PIPE)
		count = 0
		result = proc_grep.communicate()[0].split('\n') 
		for line in result:
			if line.find(text) != -1:
				count = count + 1
		del line
		del result
		del proc_grep
		return count

	#----------------------------------------------------------------------------------	
	# Match a file with all active subjects
	#----------------------------------------------------------------------------------	
	def MatchFileWithSubjects(self, currfile, shortpath, longpath, currclient):
		idbname = os.path.realpath( os.path.join(LMContext.idbpath, LMContext.expt, currclient[1]) )
		try:
			if os.path.exists(idbname) == False:
				os.mkdir(idbname)
		except OSError, e:
			print e
		idbname = os.path.realpath( os.path.join(idbname, currfile[10]) )
		if self.GenMissingDB(idbname, shortpath, longpath, currfile) == False:
			print "X"+str(self.id)+": "+shortpath +" (error)"
			return

		if LMContext.expt == "exact":
			print "@"+str(self.id)+": "+shortpath
			self.DoExactMatch(idbname+LMContext.idbext, shortpath, longpath, currfile)

		elif LMContext.expt == "approx":
			#......................#
			# APPROXIMATE MATCHING #
			#......................#
			print "&"+str(self.id)+": "+shortpath
			self.DoApproxMatch(idbname+LMContext.idbext, shortpath, longpath, currfile, currclient)

	def WaitTimeout(self, proc, endTime, nodename, idbfname, shortpath, longpath, proctype ):
		count = 0
		# While the proc_ida hasn't return and it's not endTime yet,
		# go to sleep for 0.5 ms
		while proc.poll() == None and endTime > time.time():
			time.sleep(0.5)
			if (count % 30) == 0:
				print "#%d: %s %s \"%s\" (%s left %d secs)"%\
					( self.id, nodename, \
					idbfname, shortpath, proctype, endTime-time.time() )
			count = count+1
		# If timeout reached, kill the process
		isTimeout = False
		if endTime < time.time():
			try:
				os.kill( proc.pid, signal.SIGKILL )

				print "#%d: %s %s \"%s\" (%s timeout)"%( self.id, nodename, \
					idbfname, shortpath, proctype )

				isTimeout = True
				log = open("log_GoLibMatcher", "a+")
				log.write("%s\t%s timeout\n"%(longpath, proctype))
				log.close()
			except OSError, e:
				print "#%d: %s %s \"%s\" (%s ERR %d, %s)"%( self.id, nodename, \
					idbfname, shortpath, proctype, e[0], e[1] )

		return isTimeout

	
	#----------------------------------------------------------------------------------	
	# Generates IDA database file if it does not exist.
	#----------------------------------------------------------------------------------	
	def GenMissingDB(self, idbname, shortpath, longpath, currfile):
		idbnameext = idbname+LMContext.idbext
		idbfname = os.path.split( idbnameext )[1]
		nodename = longpath.split(LMContext.mntpath_prefix)[1].split('/')[1]
		
		# Check if IDB file already exists

		if os.path.exists(idbnameext) == True:
			print "#%d: %s %s \"%s\" (exists)"%( self.id, nodename, idbfname, shortpath )
			return

		# Remove tmp files if any...
		if os.path.exists(idbname+".id0") == True:
			os.remove(idbname+".id0")
		if os.path.exists(idbname+".id1") == True:
			os.remove(idbname+".id1")
		if os.path.exists(idbname+".nam") == True:
			os.remove(idbname+".nam")
		if os.path.exists(idbname+".til") == True:
			os.remove(idbname+".til")

		# Generate IDB	
		print "#%d: %s %s \"%s\" (genidb)"%( self.id, nodename, idbfname, shortpath )
		startTime = time.time()
		proc_ida = subprocess.Popen([LMContext.idapath, \
			"-A", "-Cg", "-o"+idbname+LMContext.idbext, "-c", \
			"-S"+os.path.realpath(LMContext.lmhome+"/bin/ida/idc/genidb.idc"), \
			"-B", longpath], \
			stdout=subprocess.PIPE)
		endTime = time.time() + LMContext.timeout_sec
		isTimeout = self.WaitTimeout(proc_ida, endTime, nodename, idbfname, shortpath, longpath, "genidb" )
		proc_ida.wait()
		del proc_ida

		if os.path.exists( idbname+LMContext.idbext ) == False:
			if isTimeout == True:
				self.UpdateScoreError( currfile, LMContext.ERR_TIMEOUT_FUNCSTATS )
			else:
				self.UpdateScoreError( currfile, LMContext.ERR_IDB_MISSING )
				print "#%d: %s %s \"%s\" (missing)"%( self.id, nodename, \
					idbfname, shortpath )

		else:
			print "#%d: %s %s \"%s\" (done)"%( self.id, nodename, idbfname, shortpath )

		return isTimeout == False


	#################################################################################
	# Functions for Approx Matching
	#################################################################################

	def DoApproxMatch(self, idbname_approx, shortpath, longpath, currfile, currclient):

		"""
		# Forms up the DOT filename
		DOTFilename = os.path.realpath(LMContext.dotpath+"/"+currclient[1])
		if os.path.exists(DOTFilename) == False:
			os.mkdir(DOTFilename)
		DOTFilename = DOTFilename+"/"+currfile[10]+".dot"
		"""
		if os.path.exists(idbname_approx) == False:
			print "!"+str(self.id)+": "+shortpath+" (!! MISSING IDB !!!)"
			log = open("log_GoLibMatcher", "a")
			log.write("%s\tnoidb\n"%(longpath))
			log.close()
			return

		# Applies jmppatch, which also saves the idb file.
		if LMContext.how != "fast":

			# Prepares distances
			self.PrepDistance( idbname_approx, shortpath, longpath, currfile )

			"""
			# Creates DOT file folder if absent.
			if os.path.exists(DOTFilename) == False:
				print "#"+str(self.id)+": "+DOTFilename.split(LMContext.lmhome)[1]+\
					" ("+LMContext.experiment[7]+", "+longpath.split(LMContext.mntpath_prefix)[1]+")"
				proc_ida = subprocess.Popen([LMContext.idapath, "-A", "-Cg", "-o"+idbname_approx, \
					"-S"+os.path.realpath(LMContext.lmhome+"/bin/ida/idc/"+LMContext.experiment[8]), \
					"-B", longpath], \
					stdout=subprocess.PIPE, stderr=subprocess.PIPE)
				proc_ida.wait()
				del proc_ida
			else:
				print "#"+str(self.id)+": "+DOTFilename.split(LMContext.lmhome)[1]+" (exists)"

			# Check DOT file again...
			if os.path.exists(DOTFilename) == False:
				print "!"+str(self.id)+": Still can't find "+DOTFilename.split(LMContext.lmhome)[1]+\
					"! Skipping!"
				LMContext.missingdots.append( longpath )
				return
	
			self.UpdateGraphOrder(currfile[0], DOTFilename)
			"""
	
		"""
		# Check subject DOT file...
		if os.path.exists(self.libsig) == False:
			print "!"+str(self.id)+": Missing subject DOT file \""+self.libsig+"\"! Aborting!"
			exit(-1)
		"""

		# Shortlisted one more
		LMContext.shortlist = LMContext.shortlist + 1

		#score = -1	
		#diffTime = 0
		# Check if we need to hold on to all semaphores
		# Only do this for approx matching
		# doBigWork = self.WillBeBig(self.libsig, DOTFilename)
		doBigWork = False
		if doBigWork == True:
			print "#"+str(self.id)+": Skipping big file \""+currfile[2]+"\""
			score = -5
		else:
			#startTime = datetime.datetime.now()

			"""
			self.proc_matcher = subprocess.Popen(\
				["valgrind", "--tool=memcheck", "--leak-check=yes", "--track-fds=yes",  "--log-file=log_valgrind", \
				os.path.realpath(LMContext.lmhome+"/tools/GraphMatcher.exe"), \
				"3",
				str(LMContext.timeout_sec), \
				LMContext.host, LMContext.user, LMContext.passwd, LMContext.db, \
				str(LMContext.experiment[0]), \
				str(LMContext.vulnfile[0]), str(currfile[0]) ], \
				close_fds=True,\
				stdout=subprocess.PIPE, \
				stderr=subprocess.PIPE);
			"""

			self.proc_matcher = subprocess.Popen(\
				[os.path.realpath(LMContext.lmhome+"/tools/GraphMatcher.exe"), \
				"3",
				str(LMContext.timeout_sec), \
				LMContext.host, LMContext.user, LMContext.passwd, LMContext.db, \
				str(LMContext.experiment[0]),
				str(LMContext.vulnfile[0]), str(currfile[0]) ], \
				close_fds=True,\
				stdout=subprocess.PIPE, \
				stderr=subprocess.PIPE);

			self.proc_matcher.wait()

			#endTime = datetime.datetime.now()
			#diffTime = endTime - startTime
			#diffTime = diffTime.days * 86400000 + diffTime.seconds * 1000 + diffTime.microseconds/1000

			# Despatched 1 work, so update counter
			LMContext.filecount = LMContext.filecount + 1

		# Update score
		#self.InsertResult(LMContext.expt, currfile, shortpath, diffTime);

		#print "#"+str(self.id)+"< GraphMatcher.exe "+LMContext.experiment[7]+" "+shortpath+" "+str(score)

		#del libsig
		#del midstr
		#del mid	
		#del DOTFilename


	def UpdateGraphOrder(self, fid, DOTFilename):
		"""
		Updates the graph order (number of vertices).
		"""
		num_vertices = self.FindTextCount(DOTFilename, "vertex")	
		num_edges = self.FindTextCount(DOTFilename, "weight")

		conn_file = MySQLdb.connect(host=LMContext.host, \
			user=LMContext.user, \
			passwd=LMContext.passwd, \
			db=LMContext.db)
		conn_file.query("UPDATE file SET num_vertices="+str(num_vertices)+\
			", num_edges="+str(num_edges)+\
			" WHERE file_id="+str(fid))
		conn_file.close()
		del conn_file
		del num_vertices
		del num_edges

	# -1: genidb timeout
	# -2: funcstats timeout
	# -3: idb still missing
	def UpdateScoreError(self, currfile, errcode):
		conn = MySQLdb.connect(host=LMContext.host, \
			user=LMContext.user, \
			passwd=LMContext.passwd, \
			db=LMContext.db)
		conn.query( "INSERT INTO result_%s (file_id, expt_id, timestamp, score) "%LMContext.expt+\
			"VALUES(%d, %d, NOW(), %d) "%(currfile[0], LMContext.experiment[0], errcode)+\
			"ON DUPLICATE KEY UPDATE score=VALUES(score)" )
		conn.close()
		del conn

	#################################################################################
	# Functions for computing Distances
	#################################################################################

	def NgramCount(self, currfile):
		conn = MySQLdb.connect(host=LMContext.host, \
			user=LMContext.user, \
			passwd=LMContext.passwd, \
			db=LMContext.db)
		conn.query( "SELECT COUNT(*) FROM `frequency`, `function` "+\
			"WHERE `function`.file_id=%d AND "%currfile[0]+\
			"`frequency`.func_id=`function`.func_id" )
		results = conn.store_result()
		count = results.fetch_row()[0][0]
		del results
		conn.close()
		del conn

		return count

	def PrepDistance(self, idbname_dist, shortpath, longpath, currfile):
		idbfname = os.path.split( idbname_dist )[1]
		nodename = longpath.split(LMContext.mntpath_prefix)[1].split('/')[1]


		# Only do funcstats if ngram count is 0
		ngramcnt = self.NgramCount( currfile )
		if ngramcnt == 0:
			# Sets ngram value in environment
			if LMContext.expt == "cosim" or LMContext.expt == "dist":
				os.environ["NGRAM"] = str(LMContext.experiment[7])
			else:
				# Just use default value of 3 for other experiments.
				os.environ["NGRAM"] = str(LMContext.ngram)
		
			# Gathers function statistics
			print "#%d: %s %s \"%s\" (funcstats)"%( self.id, nodename, idbfname, shortpath )
			startTime = time.time()
			idascript = os.path.realpath(LMContext.lmhome+"/bin/ida/idc/funcstats.idc")
			proc_ida = subprocess.Popen([LMContext.idapath, \
				"-A", "-Cg", "-o"+idbname_dist, \
				"-P",
				"-S"+idascript, \
				"-B", longpath], \
				close_fds=True, \
				stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
			endTime = time.time() + LMContext.timeout_sec
			isTimeout = self.WaitTimeout(proc_ida, endTime, nodename, idbfname, shortpath, longpath, "funcstats")
			proc_ida.wait()
			del proc_ida
			if isTimeout == True:
				self.UpdateScoreError( currfile, LMContext.ERR_TIMEOUT_FUNCSTATS )
				return
		else:
			print "#%d: %s %s \"%s\" (%d ngrams)"%( self.id, nodename, idbfname, shortpath, ngramcnt )

		# Calls distance.exe to compute either one of the following distances
		# between functions of the subject and the current file.
		# 1. cosine similarity
		# 2. Euclidean distance
		# 3. bitap
		# 4. Mahalanobis distance
		print "#%d: %s %s \"%s\" (dist)"%( self.id, nodename, idbfname, shortpath )
		startTime = time.time()
		proc_dist = subprocess.Popen([os.path.join(LMContext.lmhome, \
			"tools", "distance.exe"),\
			LMContext.host, "cosim", \
			str(LMContext.vuln[1]), str(currfile[0]),\
			str(LMContext.experiment[0]) ], \
			stdout=subprocess.PIPE, stderr=subprocess.PIPE)
		endTime = time.time() + LMContext.timeout_sec
		isTimeout = self.WaitTimeout(proc_dist, endTime, nodename, idbfname, shortpath, longpath, "dist")
		proc_dist.wait()
		del proc_dist
		if isTimeout == True:
			self.UpdateScoreError( currfile, LMContext.ERR_TIMEOUT_DIST )
			return



	#################################################################################
	# Functions for Exact Matching
	#################################################################################

	def DoExactMatch(self, idbname_exact, shortpath, longpath, currfile):
		startTime = datetime.datetime.now()
		proc_ida = subprocess.Popen([LMContext.idapath, "-A", "-Cg", "-o"+idbname_exact, \
			"-S"+os.path.realpath(LMContext.lmhome+"/bin/ida/idc/"+LMContext.experiment[8]), \
			"-B", longpath], \
			close_fds=True, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
		score = proc_ida.wait()
		endTime = datetime.datetime.now()
		del proc_ida
		diffTime = endTime - startTime
		diffTime = diffTime.days * 86400000 + diffTime.seconds * 1000 + diffTime.microseconds/1000
		# Update score
		self.InsertResult(LMContext.expt, currfile, shortpath, diffTime, score);
		del score


	#################################################################################
	# Common Functions
	#################################################################################
	
	def InsertResult(self, exptname, currfile, shortpath, elapsed_ms, score=0):

		# Set up the right query
		insresult_sql = None
		if exptname == "approx":
			insresult_sql = "INSERT INTO result_"+exptname+\
				" (file_id, expt_id, elapsed_ms, timestamp, score) "+\
				"VALUES("+str(currfile[0])+","+LMContext.expt_id+", "+\
				str(elapsed_ms)+", NOW(), "+str(score)+") "+\
				"ON DUPLICATE KEY UPDATE elapsed_ms="+str(elapsed_ms)+\
				", timestamp=NOW(), score="+str(score)
		else:
			insresult_sql = "INSERT INTO result_"+exptname+\
				" (file_id, expt_id, elapsed_ms, timestamp) "+\
				"VALUES("+str(currfile[0])+","+str(LMContext.expt_id)+\
				", "+str(elapsed_ms)+", NOW()) "+\
				"ON DUPLICATE KEY UPDATE elapsed_ms="+str(elapsed_ms)+", timestamp=NOW()"

		#if result_id != None:
		#	print "#"+str(self.id)+": Updating SQL entry for "+str(result_id)
		#	insresult_sql = "UPDATE result_"+LMContext.expt+" "\
		#		"SET score="+str(score)+", elapsed_ms="+str(elapsed_ms)+", timestamp=NOW() "\
		#		"WHERE result_id="+str(result_id)

		# Execute the query	
		conn = MySQLdb.connect(host=LMContext.host, \
			user=LMContext.user, \
			passwd=LMContext.passwd, \
			db=LMContext.db)
		conn.query(insresult_sql)
		del insresult_sql
		conn.close()
		del conn

