#!/usr/local/bin/python

import subprocess
import os
import time
import MySQLdb
#import hashlib
import datetime
import threading
import Queue
import sys
import uuid
import IDAThread
import LMContext
from fileinfo import FileInfo

#============================================================
# ClientThread - Threading class that mounts the client,
# enumerates the client's files and add them to IDAWorkQueue
# if they qualify.
#============================================================
class ClientThread(threading.Thread):
	def __init__(self, currclient):
		self.mntpath = os.path.realpath(str(LMContext.mntpath_prefix+"/"+currclient[1]))
		self.currclient = currclient
		threading.Thread.__init__(self)
	
	def run(self):
		#if self.IsMounted(self.mntpath) == False:
		#	print "Cannot proceed. "+self.mntpath+" is not mounted."	
		#else:
		print "Computing matches for "+self.currclient[1]

		LMContext.ClRdyEvt.set()

		if LMContext.how == "fast":
			self.QueueFilesFast()
		elif LMContext.how == "complete":
			self.QueueFilesComplete()
		elif LMContext.how == "clean":
			self.CleanFilesComplete()


	# Checks /etc/mtab to see if mountpath is mounted.
	#def IsMounted(self, mountpath):
	#	proc = subprocess.Popen("cat /etc/mtab | grep "+mountpath, shell=True, stdout=subprocess.PIPE)
	#	proc.wait()
	#	out = repr(proc.communicate()[0])
	#	if out == "''":
	#		return False
	#	return True

	def QueueFilesFast(self):
		"""
		Queue files using existing IDB files, rather than enumerating all files.
		"""
		try:
			dbLoc = None
			if LMContext.expt == "exact":
				dbLoc = os.path.realpath(LMContext.idbpath+"/exact/"+self.currclient[1])
			elif LMContext.expt == "approx":
				dbLoc = os.path.realpath(LMContext.dotpath+"/"+self.currclient[1])
			else:
				print "Error: Unrecognized experiment type."
				exit(-1)

			if os.path.exists(dbLoc) is False:
				os.mkdir(dbLoc)

			# For all database files (IDB or DOT)...
			dirList = os.listdir(dbLoc)
			for dbname in dirList:
				try:
					# Make sure this db file corresponds to an entry
					# in the database.
					dbname_noext = os.path.splitext(dbname)[0]
					db = MySQLdb.connect(host=LMContext.host, \
						user=LMContext.user, \
						passwd=LMContext.passwd, \
						db=LMContext.db)
					#buf = "SELECT * FROM file where path_sha1=\""+dbname_noext+\
					#	"\" AND client_id="+str(self.currclient[1])
					#print buf
					db.query("SELECT SQL_CACHE * FROM file where path_sha1=\""+dbname_noext+\
						"\" AND client_id="+str(self.currclient[1]))
					result = db.store_result()	
					if result.num_rows() != 1:
						print "Error: Expected 1 file, but "+str(result.num_rows())+" found."
						exit(0)

					# Put work into IDAWorkQueue
					file_list = result.fetch_row(maxrows=0)
					currfile = file_list[0]
					del file_list

					spath = currfile[2]
					lpath = self.mntpath+spath

					fileinfo = FileInfo(longpath=lpath, shortpath=spath, cid=self.currclient[0])
					self.PutWork(fileinfo, currfile)

					db.close()
					del db
					del dbname_noext

				except OSError, e:
					print self.currclient[1]+": "+dbname+" "+e
					continue	

		except UnicodeDecodeError:
			print self.currclient[1]+": !!!UnicodeDecodeError!!! mntpath = \""+self.mntpath+"\""
		print self.currclient[1]+": Done QueueFilesFast in \""+self.mntpath+"\"."

	def QueueFilesComplete(self):
		try:
			for dirpath, dirnames, filenames in os.walk(self.mntpath):
				print "c"+str(self.currclient[0])+": "+dirpath.split(LMContext.mntpath_prefix)[1]
				# Now check filenames...
				for f in filenames:
					lpath = os.path.realpath(os.path.join(dirpath, f))
					spath = lpath[len(self.mntpath):]

					try:
						fileinfo = FileInfo(longpath=lpath, \
							shortpath=spath, \
							f_name=f, \
							cid=self.currclient[0])
						
						if (fileinfo.GetFileType().find(str(LMContext.bits)+"-bit LSB executable") != -1) or \
						(fileinfo.GetFileType().find(str(LMContext.bits)+"-bit LSB shared") != -1):
							# Put work
							self.PutWork(fileinfo, None)
							LMContext.RdyEvent.set()
						elif (fileinfo.GetFileType().find('symbolic link') != -1):
							print "c"+str(self.currclient[0])+\
								": Unlinking "+\
								lpath.split(LMContext.mntpath_prefix)[1]
							os.unlink(lpath)
						#else:
						#	print "c"+str(self.currclient[0])+\
						#		": Deleting "+\
						#		lpath.split(LMContext.mntpath_prefix)[1]
						#	os.remove(lpath)
		
					except OSError, e:
						print "c"+str(self.currclient[0])+": "+lpath+" "+e
						del lpath
						del spath
						continue

					del lpath
					del spath

			#LMContext.IDAWorkQueueLock.acquire()
			print "Queue complete."
			time.sleep(3)
			LMContext.QCOMPLETE = True
			LMContext.RdyEvent.set()
			#LMContext.IDAWorkQueueLock.release()
	
		except UnicodeDecodeError:
			print self.currclient[1]+": !!!UnicodeDecodeError!!! mntpath = \""+self.mntpath+"\""
		print self.currclient[1]+": Done QueueFilesComplete in \""+self.mntpath+"\"."

	#
	# Puts work into the work queue, depending on whether we satisfy the condition for
	# "resume" or "repair". Also, if currfile is None, we insert file info into database
	# and make sure there is no duplicated checksum.
	#
	def PutWork(self, fileinfo, currfile):
		doWork = True

		if LMContext.mode == "resume":
			# Do work only if there are no matched entries.
			num_match = self.NumMatchedEntries(fileinfo.GetShortPath())
			if num_match > 0:
				doWork = False
			del num_match

		elif LMContext.mode == "repair":
			# Do work only if entry is bad or missing.
			doWork = self.NeedsRepair(self.currclient, fileinfo.GetShortPath())
			if doWork == True:
				print "c"+str(self.currclient[0])+": "+fileinfo.GetShortPath()+" (needs repair)"
			else:
				print "c"+str(self.currclient[0])+": "+fileinfo.GetShortPath()+" (no repair)"

		elif LMContext.mode =="replace":
			# Do work anyhow.
			doWork = True

		# One last check to confirm doWork. If currfile is not instantiated,
		# insert currfile into database and make sure it is not a duplicated
		# file.
		if doWork == True and currfile == None:
			currfile = fileinfo.InsertFileIfNotExist(LMContext.host, \
				LMContext.user, \
				LMContext.passwd, \
				LMContext.db)
			if currfile[13] == 0:
				# Don't do the work if it is duplicated.
				doWork = False
				print "c"+str(self.currclient[0])+": "+fileinfo.GetShortPath()+" (duplicated)"

		if doWork == True:
			# Put work into IDAWorkQueue
			#print "Waiting to acquire IDAWorkQueueLock to put work"
			#LMContext.IDAWorkQueueLock.acquire()
			#print "Acquiried to put work"
			#print fileinfo.GetShortPath()
			#print fileinfo.GetLongPath()
			LMContext.IDAWorkQueue.put( (currfile, \
				fileinfo.GetShortPath(), \
				fileinfo.GetLongPath(), \
				self.currclient) )
			LMContext.numfilesqueued = LMContext.numfilesqueued + 1
			#print "Releasing IDAWorkQueueLock"
			#LMContext.IDAWorkQueueLock.release()
			print "c"+str(self.currclient[0])+": "+fileinfo.GetShortPath()+" (queued)"

		del doWork

	def CleanFilesComplete(self):
		try:
			for dirpath, dirnames, filenames in os.walk(self.mntpath):

				# Unlink symbolic links
				for d in dirnames:
					longpath = os.path.realpath(os.path.join(dirpath, d))
					try:
						proc_type = subprocess.Popen(["file", "-b", longpath], \
							close_fds=True, stdout=subprocess.PIPE)
						proc_type.wait()
						type = proc_type.stdout
						typestr = type.read().rstrip('\n')
		
						if typestr.find("symbolic link") != -1:
							print "c"+str(self.currclient[0])+\
								": Unlinking "+\
								longpath.split(LMContext.mntpath_prefix)[1]
							os.unlink(longpath)

						del typestr
						type.close()
						del type
						del proc_type
					except OSError, e:
						print "c"+str(self.currclient[0])+": "+longpath+" "+e
						del longpath
						continue
				
					del longpath

				# Now check filenames...
				for f in filenames:
					longpath = os.path.join(dirpath, f)
					try:
						shortpath = longpath[len(self.mntpath):]
						
						proc_type = subprocess.Popen(["file", "-b", longpath], \
							close_fds=True, stdout=subprocess.PIPE)
						proc_type.wait()
						type = proc_type.stdout
						typestr = type.read().rstrip('\n')

						if (typestr.find('symbolic link') != -1):
							print "c"+str(self.currclient[0])+\
								": Unlinking "+\
								longpath.split(LMContext.mntpath_prefix)[1]
							os.unlink(longpath)	
						elif (typestr.find(str(LMContext.bits)+"-bit LSB executable") == -1) and \
							(typestr.find(str(LMContext.bits)+"-bit LSB shared") == -1):
							print "c"+str(self.currclient[0])+\
								": Deleting "+\
								longpath.split(LMContext.mntpath_prefix)[1]
							os.remove(longpath)
		
						del typestr
						type.close()
						del type
						del proc_type
						del shortpath
			
					except OSError, e:
						print "c"+str(self.currclient[0])+": "+longpath+" "+e
						del longpath
						continue
					del longpath
	
		except UnicodeDecodeError:
			print self.currclient[1]+": !!!UnicodeDecodeError!!! mntpath = \""+self.mntpath+"\""
		print self.currclient[1]+": Done CleanFilesComplete in \""+self.mntpath+"\"."


	def NumMatchedEntries(self, shortpath):
		db = MySQLdb.connect(LMContext.host, LMContext.user, LMContext.passwd, LMContext.db)
		db.query("SELECT M.* FROM result_"+LMContext.expt+\
			" AS M, file as F WHERE STRCMP(F.path, \""+shortpath+"\")=0"+\
			" AND F.client_id="+str(self.currclient[0])+\
			" AND F.file_id=M.file_id")
		num_match = db.store_result().num_rows()
		db.close()
		del db
		return num_match

	# Returns True if the file needs to be repaired. Repair is required if
	# any of the following is True.
	# 1. A matched entry does not exist.
	# 2. Score is negative.
	# 3. Timestamp is 0.
	def NeedsRepair(self, currclient, shortpath):
		if LMContext.expt != "approx" and LMContext.expt != "exact":
			return True

		# Checks for normal entry
		conn_repair = MySQLdb.connect(host=LMContext.host,
			user=LMContext.user,
			passwd=LMContext.passwd,
			db=LMContext.db)

		conn_repair.query("SELECT COUNT(R.result_id) FROM file AS F, "\
			+"result_"+LMContext.expt+" AS R WHERE "\
			+"R.expt_id="+str(LMContext.experiment[0])+" AND "\
			+"F.path=\""+shortpath+"\" AND "\
			+"F.client_id="+str(currclient[0])+" AND "\
			+"R.file_id=F.file_id AND "\
			+"R.score >= 0 AND "\
			+"R.timestamp != \"0000-00-00 00:00:00\"")
		results = conn_repair.store_result()
		row = results.fetch_row()
		if row[0][0] == 0:
			print "c"+str(self.currclient[0])+": "+shortpath+" (missing entry)"
			# Normal entry missing. Needs repair.
			del row
			del results
			conn_repair.close()
			del conn_repair
			return True
		del row
		del results
		conn_repair.close()
		del conn_repair

		# Checks for abnormal entry
		conn_repair = MySQLdb.connect(host=LMContext.host,
			user=LMContext.user,
			passwd=LMContext.passwd,
			db=LMContext.db)
		conn_repair.query("SELECT R.result_id, R.timestamp, R.score FROM file AS F, "\
			+"result_"+LMContext.expt+" AS R WHERE "\
			+"R.expt_id="+str(LMContext.experiment[0])+" AND "\
			+"F.path=\""+shortpath+"\" AND "\
			+"F.client_id="+str(currclient[0])+" AND "\
			+"R.file_id=F.file_id AND "\
			+"(R.score < 0 OR R.timestamp=\"0000-00-00 00:00:00\")")
		results = conn_repair.store_result()
		if results.num_rows() != 0:
			entry = results.fetch_row()
			print "c"+str(self.currclient[0])+": "+\
				shortpath+" (bad entry result_id="+str(entry[0][0])+\
				", timestamp="+str(entry[0][1])+\
				", score="+str(entry[0][2])+")"
			# Found abnormal entry. Needs repair.
			del results
			conn_repair.close()
			del conn_repair
			return True
		del results
		conn_repair.close()
		del conn_repair
		return False

