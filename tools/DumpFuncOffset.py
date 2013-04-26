#!/usr/bin/python

import sys
import os
import subprocess


if len(sys.argv) != 5:
	print "Gets address of first instructions for caller and callee functions for dot files in <dotpath>."
	print "Usage: "+sys.argv[0]+" <dotpath> <caller> <callee> <outfile>"
	sys.exit(0)


outfile = open(sys.argv[4], 'w+')
outfile.write("#dotname\t"+sys.argv[2]+"->"+sys.argv[3]+"\n")
outfile.flush()

for dirpath, dirnames, filenames in os.walk(sys.argv[1]):
	for f in filenames:
		filepath = os.path.join(dirpath, f)
		print filepath
		
		# Get caller's id
		callerid = 1
		proc_caller = subprocess.Popen(["grep", "-w", sys.argv[2], filepath], stdout=subprocess.PIPE)
		proc_cut = subprocess.Popen(["cut", "-d[", "-f1"], stdin=proc_caller.stdout, stdout = subprocess.PIPE)
		callerid_str = proc_cut.communicate()[0]
		if callerid_str=='':
			print "\tNo caller \""+sys.argv[2]+"\"."
			continue
		else:
			callerid = int(callerid_str)
			print "\tCallerid\t: "+str(callerid)

		# Get callee's id
		calleeid = 0
		proc_callee = subprocess.Popen(["grep", "-w", sys.argv[3], filepath], stdout=subprocess.PIPE)
		proc_cut = subprocess.Popen(["cut", "-d[", "-f1"], stdin=proc_callee.stdout, stdout = subprocess.PIPE)
		calleeid_str = proc_cut.communicate()[0]
		if calleeid_str=='':
			print "\tNo callee \""+sys.argv[3]+"\"."
			continue
		else:
			calleeid = int(calleeid_str)
			print "\tCalleeid\t: "+str(calleeid)
				
		# Ensure <caller> does call <callee>
		callstr = str(callerid)+"->"+str(calleeid)
		proc_call = subprocess.Popen(["grep", callstr, filepath], stdout=subprocess.PIPE)
		if proc_call.communicate()[0] == "" :
			# <caller> doesn't call <callee>
			print "\t\""+sys.argv[2]+"\" does not call \""+sys.argv[3]+"\"."
			outfile.write(f+"\t-1\n")
			outfile.flush()
		else:
			# Get <caller>'s start addr
			proc_caller = subprocess.Popen(["grep", "-w", sys.argv[2], filepath], stdout=subprocess.PIPE)
			proc_cut = subprocess.Popen(["cut", "-d\"", "-f4"], stdin=proc_caller.stdout, stdout = subprocess.PIPE)
			calleraddr_str = proc_cut.communicate()[0]
			calleraddr = int(calleraddr_str)
			print "\tCallee addr\t: " + str(calleraddr)

			proc_callee = subprocess.Popen(["grep", "-w", sys.argv[3], filepath], stdout=subprocess.PIPE)
			proc_cut = subprocess.Popen(["cut", "-d\"", "-f4"], stdin=proc_callee.stdout, stdout = subprocess.PIPE)
			calleeaddr_str = proc_cut.communicate()[0]
			calleeaddr = int(calleeaddr_str)
			print "\tCallee addr\t: " + str(calleeaddr)

			outfile.write(f+"\t"+str(calleeaddr - calleraddr)+"\n")
			outfile.flush()

outfile.close()
