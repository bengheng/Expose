#!/usr/bin/env python

"""
Adds a subject to the database by performing the following actions.
1. Create 'dot' file. From the dot file, we can extract the number of edges and vertices.
2. Inserts file information into 'file' table if it doesn't already exist.
3. Inserts vulnerability information related to the file.
"""

import MySQLdb
from fileinfo import FileInfo
import sys
import os
import hashlib
import subprocess
import socket

class Subject:
	def __init__(self, _vuln_id, _ngram, _bits, _host, _user, _passwd, _db):
		self.vuln_id = _vuln_id
		self.ngram = _ngram
		self.bits = _bits
		self.host = _host
		self.user = _user
		self.passwd = _passwd
		self.db = _db
		self.idbpath = os.path.join( os.environ.get("HOME"), "libmatcher", \
			"idb", "approx", "VULN" )
		#self.dotpath = os.path.join( os.environ.get("HOME"), "libmatcher", "dot", "approx" )
		self.client_id = self.GetClientId()

		# Sets up info for IDA
		if self.bits == 32:
			self.idapath = os.path.join( os.environ.get("HOME"), "idaadv", "idal" )
			self.idbext = ".idb"
		elif self.bits == 64:
			self.idapath = os.path.join( os.environ.get("HOME"), "idaadv", "idal64" )
			self.idbext = ".i64"

	def GetClientId(self):
		conn = MySQLdb.connect(host=self.host, user=self.user, \
			passwd=self.passwd, db=self.db)
		conn.query("SELECT client_id FROM client WHERE client_name=\"VULN\"")
		res = conn.store_result()
		if res.num_rows() == 0:
			print "Cannot find client_id for \"VULN\"!"
			conn.close()
			del conn	
			sys.exit(0)

		client_id = res.fetch_row(0)[0][0]
		del res
		conn.close()
		del conn
		return client_id

	def GetSbjFilePath(self):
		"""
		Returns the path of the subject file.
		"""

		# First, get the name of the subject
		conn = MySQLdb.connect(host=self.host, user=self.user, \
			passwd=self.passwd, db=self.db)
		conn.query("SELECT vuln_lib FROM vuln WHERE vuln_id="+str(self.vuln_id))
		res = conn.store_result()
		if res.num_rows() == 0:
			print "Cannot find vuln_id \'"+str(self.vuln_id)+"\'!"
			conn.close()
			del conn	
			sys.exit(0)
		sbjname = res.fetch_row(0)[0][0]
		print "Subject name:\t\t"+sbjname
		del res
		conn.close()
		del conn

		# Then, look for that file
		sbjlongpath = None
		searchpath = os.path.join( os.environ.get("HOME"), "libmatcher", "mnt", "VULN" )
		for dirpath, dirnames, filenames in os.walk(searchpath):
			for f in filenames:
				if f == sbjname:
					sbjlongpath = os.path.join(dirpath, f)

		sbjshortpath = sbjlongpath.split( searchpath )[1]
		return sbjname, sbjshortpath, sbjlongpath


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


	def InsertFuncStats(self, longpath, idb):
		"""
		Inserts funcstats for specified file into database.
		"""
		os.environ["TVHEADLESS"] = "1"
		spath = os.path.join( os.environ.get("HOME"), "libmatcher/bin/ida/idc/funcstats.idc" )
		print "Generating funcstats with %d-gram on %s using:\n\t%s\n\t%s\n\t%s"%\
			( self.ngram, longpath, self.idapath, idb, spath)
		proc_ida = subprocess.Popen([self.idapath, "-A", "-Cg", "-o"+idb, \
			"-S"+spath, "-B", "-P", longpath], \
			stdout=subprocess.PIPE)
		proc_ida.wait()
		del proc_ida


	def InsertOpcode(self, longpath, idb):
		"""
		Inserts Opcode for specified file into database.
		"""
		print "Generating opcode..."
		#os.environ["TVHEADLESS"] = "1"
		spath = os.path.join( os.environ.get("HOME"), "libmatcher/bin/ida/idc/opcode.idc" )
		proc_ida = subprocess.Popen([self.idapath, "-A", "-Cg", "-o"+idb, \
			"-S"+spath, "-B", "-P", longpath], \
			stdout=subprocess.PIPE, stderr=subprocess.PIPE)
		proc_ida.wait()
		del proc_ida


	def UpdSbjFile(self):
		sbjname, sbjshortpath, sbjlongpath = self.GetSbjFilePath()

		print "Subject short path:\t"+sbjshortpath
		print "Subject long path:\t"+sbjlongpath

		#num_vertices, num_edges = MakeDotFile(idbpath, dotpath) 


		fileinfo = FileInfo( longpath=sbjlongpath, \
			shortpath=sbjshortpath, \
			f_name=sbjname, cid=self.client_id )

		filebits = fileinfo.GetBits()
		if filebits != self.bits:
			print "Expected %d-bit subject, but found %d-bit!\n"\
				%( self.bits, filebits )
			sys.exit(1)
		print "Subject bits:\t\t"+str(filebits)

		# Insert file info
		currfile = fileinfo.InsertFileIfNotExist(self.host, self.user, \
			self.passwd, self.db, self.vuln_id)

		if self.vuln_id == 0:	
			self.vuln_id = self.InsertVulnInfo(currfile[0])

		if self.vuln_id > 0:
			# User supplied vuln_id, so we try to associate that with the file.

			# Checks if the file_id is in use. We want the file_id to be same as vuln_id.
			conn = MySQLdb.connect(host=self.host, user=self.user, \
				passwd=self.passwd, db=self.db)
			conn.query("SELECT `name`, `client_id`, `file_sha1` FROM `file` WHERE `file_id`="+str(self.vuln_id))
			res = conn.store_result()
			if res.num_rows() != 0:
				row = res.fetch_row(0)[0]
				if self.client_id != row[1] or fileinfo.GetFileSHA1() != row[2]:
					print "file_id %d already in use by %s (cid=%d, file_sha1=\"%s\")!"\
						%( self.vuln_id, row[0], row[1], row[2] )
					del row
					del res
					conn.close()
					del conn
					sys.exit(0)
			conn.close()
			del res
			del conn


			# Update file_id in vuln
			conn = MySQLdb.connect(host=self.host, user=self.user, \
				passwd=self.passwd, db=self.db)
			conn.query("UPDATE `vuln` SET `file_id`="+str(currfile[0])+" "+\
			"WHERE vuln_id="+str(self.vuln_id) )
			conn.close()
			del conn

		else:
			print "Invalid vuln_id \'"+str(self.vuln_id)+"\'!"

		subjidb = os.path.join(self.idbpath, fileinfo.GetPathSHA1()+self.idbext)

		# Generate Opcode
		# InsertOpcode(sbjlongpath, subjidb)

		# Generate funcstats
		if self.ngram <= 0 or self.ngram > 10:
			print "Unsupported ngram value "+str(self.ngram)+"!"
		else:
			os.environ["NGRAM"] = str(self.ngram)
			os.environ["MYSQLDB"] = self.db
			self.InsertFuncStats(sbjlongpath, subjidb)


	def InsertVulnInfo(self, file_id):
		print "Creating new vuln_id..."

		title = raw_input("Enter title (max 255 chars): ")
		while len(title) > 255:
			print "Too long."
			title = raw_input("Enter title (max 255 chars): ")

		vuln_class = raw_input("Enter vulnerability class (max 63 chars): ")
		while len(vuln_class) > 63:
			print "Too long."
			vuln_class = raw_input("Enter vulnerability class (max 63 chars): ")
	
		vuln_lib = raw_input("Enter vulnerable library (max 31 chars): ")
		while len(vuln_lib) > 31:
			print "Too long."
			vuln_lib = raw_input("Enter vulnerable library (max 31 chars): ")

		vuln_ver = raw_input("Enter vulnerable versions (max 63 chars): ")
		while len(vuln_ver) > 63:
			print "Too long."
			vuln_ver = raw_input("Enter vulnerable versions (max 63 chars): ")

		vuln_file = raw_input("Enter vulnerable file (max 31 chars): ")
		while len(vuln_file) > 31:
			print "Too long."
			vuln_file = raw_input("Enter vulnerable file (max 31 chars): ")

		vuln_func = raw_input("Enter vulnerable function (max 31 chars): ")
		while len(vuln_func) > 31:
			print "Too long."
			vuln_func = raw_input("Enter vulnerable function (max 31 chars): ")

		#IDA_sigfile = raw_input("Enter IDA signature file (max 31 chars): ")
		#while len(IDA_sigfile) > 31:
		#	print "Too long."
		#	IDA_sigfile = raw_input("Enter IDA signature file (max 31 chars): ")

		CVE_id = raw_input("Enter CVE ID (CVE-####-####): ")
		while len(CVE_id) != 13:
			print "Invalid input."
			CVE_id = raw_input("Enter CVE ID (CVE-####-####): ")

		CWE_id = raw_input("Enter CWE ID (max 7 chars): ")
		while len(CWE_id) > 7:
			print "Too long."
			CWE_id = raw_input("Enter CWE ID (max 7 chars): ")

		bugtraq_id = raw_input("Enter Bugtraq ID (max 7 chars): ")
		while len(bugtraq_id) > 7:
			print "Too long."
			bugtraq_id = raw_input("Enter Bugtraq ID (max 7 chars): ")

		vupen_id = raw_input("Enter VUPEN ID (max 31 chars): ")
		while len(vupen_id) > 31:
			print "Too long."
			vupen_id = raw_input("Enter VUPEN ID (max 31 chars): ")
	
		remote = raw_input("Remote? (y/n): ")
		while (remote != "y") and (remote != "n"):
			print "Invalid input."
			remote = raw_input("Remote? (y/n): ")

		local = raw_input("Local? (y/n): ")
		while (local != "y") and (local != "n"):
			print "Invalid input."
			local = raw_input("Local? (y/n): ")
	
		published_date = raw_input("Enter published date (yyyy-mm-dd): ")
		while len(published_date) != 10:
			print "Invalid input."
			published_date = raw_input("Enter published date (yyyy-mm-dd): ")

		updated_date = raw_input("Enter updated date (yyyy-mm-dd): ")
		while len(updated_date) != 10:
			print "Invalid input."
			updated_date = raw_input("Enter updated date (yyyy-mm-dd): ")

		description = raw_input("Enter description (max 1023 chars): ")
		while len(description) > 1023:
			print "Too long."
			description = raw_input("Enter description (max 1023 chars): ")

		reference = raw_input("Enter references (max 1023 chars): ")
		while len(reference) > 1023:
			print "Too long."
			reference = raw_input("Enter references (max 1023 chars): ")

		if remote == "y":
			remote_val = 1
		else:
			remote_val = 0

		if local == "y":
			local_val = 1
		else:
			local_val = 0

		conn = MySQLdb.connect(host=self.host, user=self.user, passwd=self.passwd, db=self.db)
		# execute SQL statement
		sql_stmt = "INSERT INTO vuln (file_id, "\
			"title, "\
			"vuln_class, "\
			"vuln_lib, "\
			"vuln_ver, "\
			"vuln_file, "\
			"vuln_func, "\
			"CVE_id, "\
			"CWE_id, "\
			"bugtraq_id, "\
			"vupen_id, "\
			"remote, "\
			"local, "\
			"published_date, "\
			"updated_date, "\
			"description, "\
			"reference) VALUES (" +\
			str(file_id)+",\""+\
			title +"\",\""+\
			vuln_class +"\",\""+\
			vuln_lib +"\",\""+\
			vuln_ver +"\",\""+\
			vuln_file +"\",\""+\
			vuln_func +"\",\""+\
			CVE_id +"\",\""+\
			CWE_id +"\",\""+\
			bugtraq_id +"\",\""+\
			vupen_id +"\","+\
			str(remote_val) +","+\
			str(local_val) +",\""+\
			published_date +"\",\""+\
			updated_date +"\",\""+\
			description +"\",\""+\
			reference +"\")"

		vuln_id = conn.insert_id()
		conn.query(sql_stmt)
		conn.close()
		return vuln_id

	def AdjustFileAutoInc(self):
		"""
		Always set the auto_increment value of file to 21. We reserve
		first 20 slots for vuln files.
		"""
		conn = MySQLdb.connect(host=self.host, user=self.user, passwd=self.passwd, db=self.db)
		conn.query("SELECT AUTO_INCREMENT FROM information_schema.tables WHERE table_name='file'")
		res = conn.store_result()
		autoincrement = res.fetch_row(0)[0][0]
		print "AUTO_INCREMENT value for `file` is "+str(autoincrement)
		conn.close()
		if autoincrement < 21:
			print "Changing autoincrement value to 21"
			conn = MySQLdb.connect(host=self.host, user=self.user, passwd=self.passwd, db=self.db)
			conn.query("ALTER TABLE `file` AUTO_INCREMENT=21")
			conn.close()

