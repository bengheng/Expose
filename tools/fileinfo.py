# fileinfo.py

"""
Class: FileInfo( longpath, shortpath, f_name, cid )
Gets information about file at the absolute path, longpath.
"""

import hashlib
import subprocess
import MySQLdb
import os

class FileInfo:
	def __init__(self, longpath, shortpath, f_name=None, cid=0):
		"""
		FileInfo takes one argument to its constructor. The argument
		specifies the absolute path to the file.
		"""
		self.cid = cid
		self.longpath = longpath
		self.shortpath = shortpath
		if f_name == None:
			self.f_name = os.path.basename(self.shortpath)
		else:
			self.f_name = f_name
		self.f_sha1 = None
		self.p_sha1 = None
		self.f_size = None
		self.f_mtime = None
		self.f_atime = None
		self.f_ctime = None
		self.f_type = None
		self.isFirst = None

	def GetBits(self):
		bits = None
		proc_file = subprocess.Popen(["file", "-b", self.longpath],\
			stdout=subprocess.PIPE, stderr=subprocess.PIPE)
		out = proc_file.communicate()[0]
		del proc_file

		if out.find("32-bit") != -1:
			bits = 32
		elif out.find("64-bit") != -1:
			bits = 64
		del out

		return bits

	def GetCID(self):
		return self.cid

	def GetLongPath(self):
		return self.longpath

	def GetShortPath(self):
		return self.shortpath

	def GetFileName(self):
		return self.f_name

	def GetFileSHA1(self):
		if self.f_sha1 != None:
			return self.f_sha1

		sha1 = hashlib.sha1()
		f = open(self.longpath, 'rb')
		try:
			sha1.update(f.read())
		finally:
			f.close()
			del f
		self.f_sha1 = sha1.hexdigest()
		del sha1
		return self.f_sha1

	def GetPathSHA1(self):
		if self.p_sha1 != None:
			return self.p_sha1

		if self.shortpath != None:
			self.p_sha1 = hashlib.sha1(self.shortpath).hexdigest()
		else:
			self.p_sha1 = "ffffffffffffffffffffffffffffffffffffffff"

		return self.p_sha1

	def GetFileSize(self):
		if self.f_size != None:
			return self.f_size

		proc_fsize = subprocess.Popen('stat -c %s \"'+self.longpath+'\"', shell=True, stdout=subprocess.PIPE, close_fds=True)
		self.f_size = proc_fsize.communicate()[0].rstrip('\n')
		del proc_fsize
		return self.f_size

	def GetFileMTime(self):
		"""
		Returns file modification time.
		"""
		if self.f_mtime != None:
			return self.f_mtime

		proc_mtime = subprocess.Popen('ls -lt \"'+self.longpath+'\" | cut -d \' \' -f6,7', shell=True, stdout=subprocess.PIPE, close_fds=True)
		self.f_mtime = proc_mtime.communicate()[0].rstrip('\n')
		del proc_mtime
		return self.f_mtime

	def GetFileATime(self):
		"""
		Returns file access time.
		"""
		if self.f_atime != None:
			return self.f_atime

		proc_atime = subprocess.Popen('ls -lut \"'+self.longpath+'\" | cut -d \' \' -f6,7', shell=True, stdout=subprocess.PIPE, close_fds=True)
		self.f_atime = proc_atime.communicate()[0].rstrip('\n')
		del proc_atime
		return self.f_atime

	def GetFileCTime(self):
		"""
		Returns file change time. Whenever mtime changes, so does ctime. But ctime changes
		a few extra times. For example, it will change if you change the owner or the
		permissions on the file.
		"""
		if self.f_ctime != None:
			return self.f_ctime

		proc_ctime = subprocess.Popen('ls -lct \"'+self.longpath+'\" | cut -d \' \' -f6,7', shell=True, stdout=subprocess.PIPE, close_fds=True)
		self.f_ctime = proc_ctime.communicate()[0].rstrip('\n')
		del proc_ctime
		return self.f_ctime


	def GetFileType(self):
		if self.f_type != None:
			return self.f_type

		proc_type = subprocess.Popen(["file", "-b", self.longpath], close_fds=True, stdout=subprocess.PIPE )
		self.f_type = proc_type.communicate()[0].rstrip('\n')
		del proc_type
		return self.f_type

	###################################################
	# The following functions are database utilities. #
	###################################################

	def IsFirstFile(self, _host, _user, _passwd, _db):
		"""
		Returns True if there is no other file with the same cid, SHA1 and marked as first file.
		"""
		if self.isFirst != None:
			return self.isFirst

		conn_first = MySQLdb.connect(host=_host, user=_user, passwd=_passwd, db=_db)
		conn_first.query("SELECT * FROM file WHERE client_id="+str(self.cid)+" AND file_sha1=\""+self.GetFileSHA1()+"\" AND first=1")
		results = conn_first.store_result()
		self.isFirst = (results.num_rows() == 0)
		conn_first.close()
		del results
		del conn_first
		return self.isFirst

	def InsertFileIfNotExist(self, _host, _user, _passwd, _db, file_id=0, num_vertices=0, num_edges=0):
		"""
		Extracts file information and stores into database.
		"""
		conn_file = MySQLdb.connect(host=_host, user=_user, passwd=_passwd, db=_db)

		find_sql = None
		useFileID = ""
		if file_id != 0:
			useFileID = " AND file_id="+str(file_id)
 
		find_sql = "SELECT * FROM file WHERE client_id="+str(self.GetCID())+\
			useFileID+\
			" AND path=\""+self.GetShortPath()+\
			"\" AND name=\""+self.GetFileName()+\
			"\" AND type=\""+self.GetFileType()+\
			"\" AND size="+self.GetFileSize()+\
			" AND file_sha1=\""+self.GetFileSHA1()+\
			"\" AND path_sha1=\""+self.GetPathSHA1()+"\""
		conn_file.query(find_sql)
		results = conn_file.store_result()
		if results.num_rows() == 0:
			useFileIDfield = ""
			useFileIDvalue = ""
			if file_id != 0:
				useFileIDfield = "file_id, "
				useFileIDvalue = str(file_id)+","

			# File does not exist. Inserts file info into database
			insert_sql = "INSERT INTO file ("+useFileIDfield+"client_id, path, "+\
				"name, type, "+\
				"access_time, "+\
				"change_time, "+\
				"modify_time, "+\
				"size, "+\
				"file_sha1, path_sha1, "+\
				"num_vertices, num_edges, first) VALUES ("+\
				useFileIDvalue+\
				str(self.GetCID())+",\""+\
				self.GetShortPath()+"\",\""+\
				self.GetFileName()+"\",\""+\
				self.GetFileType()+"\",\'"+\
				self.GetFileATime()+"\',\'"+\
				self.GetFileCTime()+"\',\'"+\
				self.GetFileMTime()+"\',"+\
				self.GetFileSize()+",\""+\
				self.GetFileSHA1()+"\",\""+\
				self.GetPathSHA1()+"\","+\
				str(num_vertices)+","+\
				str(num_edges)+","+\
				str(self.IsFirstFile(_host, _user, _passwd, _db))+")"
			conn_file.query(insert_sql)
			del insert_sql
			conn_file.query(find_sql)
			results = conn_file.store_result()
		elif self.IsFirstFile(_host, _user, _passwd, _db) == True:
			# Mark file as first
			conn_file.query("UPDATE file SET first=1, "+\
				"num_vertices="+str(num_vertices)+", "\
				"num_edges="+str(num_edges)+" WHERE "\
				"client_id="+str(self.GetCID())+\
				" AND path=\""+self.GetShortPath()+\
				"\" AND name=\""+self.GetFileName()+\
				"\" AND type=\""+self.GetFileType()+\
				"\" AND size="+self.GetFileSize()+\
				" AND file_sha1=\""+self.GetFileSHA1()+\
				"\" AND path_sha1=\""+self.GetPathSHA1()+"\"")
		else:
			conn_file.query("UPDATE file SET "+\
				"num_vertices="+str(num_vertices)+", "\
				"num_edges="+str(num_edges)+" WHERE "\
				"client_id="+str(self.GetCID())+\
				" AND path=\""+self.GetShortPath()+\
				"\" AND name=\""+self.GetFileName()+\
				"\" AND type=\""+self.GetFileType()+\
				"\" AND size="+self.GetFileSize()+\
				" AND file_sha1=\""+self.GetFileSHA1()+\
				"\" AND path_sha1=\""+self.GetPathSHA1()+"\"")

		currfile = results.fetch_row(0)[0]
		del find_sql
		del results
		conn_file.close()
		del conn_file
		return currfile


