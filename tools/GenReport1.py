#!/usr/local/bin/python

import sys
import socket
import MySQLdb

if len(sys.argv) != 5:
	print "Usage: "+sys.argv[0]+" <outfile> <expt_id> <dist> <\"same_funcnames\"/\"diff_funcnames\">"
	sys.exit(0)

_host = "localhost"
_user = "libmatcher"
_passwd = "l1bm@tcher"
_db = "libmatcher"

outfile = open(sys.argv[1], 'w')

expt_id = int(sys.argv[2])
dist = int(sys.argv[3])

if sys.argv[4] == "same_funcnames":
	fnamecmp = "="
elif sys.argv[4] == "diff_funcnames":
	fnamecmp = "!="
else:
	print "Invalid argument 5. Expecting either \"same_funcnames\" or \"diff_funcnames\", but got \""+sys.argv[4]+"\"."
	sys.exit(0)

outfile.write("file1 id\tfunc1 id\tfunc1 name\tfunc1 cyccomp\tfunc1_#imp\t"
	"file2 id\tfunc2 id\tfunc2 name\tfunc2 cyccomp\tfunc2_#imp\t"\
	"cosim\tnumimp\tdist\n")
outfile.flush()

try:
	conn1 = MySQLdb.connect( host=_host, user=_user, passwd=_passwd, db=_db );
	cur1 = conn1.cursor()

	cur1.execute( "SELECT f1.file_id, `dist`.func1_id, f1.func_name, "\
		"f2.file_id, `dist`.func2_id, f2.func_name, "\
		"`dist_cosim`.cosim, `dist`.dist "\
		"FROM `function` AS f1, `function` AS f2, `dist`, `dist_cosim` "\
		"WHERE `dist`.func1_id = f1.func_id "\
		"AND `dist`.func2_id = f2.func_id "\
		"AND `dist`.dist = "+str(dist)+" "\
		"AND `dist`.expt_id = "+str(expt_id)+" "\
		"AND `dist`.func1_id = `dist_cosim`.func1_id "\
		"AND `dist`.func2_id = `dist_cosim`.func2_id "\
		"AND f1.func_name "+fnamecmp+" f2.func_name" )
	row1 = cur1.fetchone()
	while row1 is not None:
		file1_id = row1[0]
		func1_id = row1[1]
		func1_name = row1[2]
		file2_id = row1[3]
		func2_id = row1[4]
		func2_name = row1[5]
		cosim = row1[6]
		dist = row1[7]


		conn2 = MySQLdb.connect( host=_host, user=_user, passwd=_passwd, db=_db );
		cur2 = conn2.cursor()
		
		cur2.execute( "SELECT cyccomp FROM `function` WHERE func_id = "+str(func1_id) )
		row2 = cur2.fetchone()
		func1_cyccomp = row2[0]

		cur2.execute( "SELECT cyccomp FROM `function` WHERE func_id = "+str(func2_id) )
		row2 = cur2.fetchone()
		func2_cyccomp = row2[0]
	
		cur2.execute( "SELECT COUNT(*) FROM `depfunc` "\
			"WHERE `depfunc`.func_id = "+str(func1_id) )
		row2 = cur2.fetchone()
		func1_numimp = row2[0]

		cur2.execute( "SELECT COUNT(*) FROM `depfunc` "\
			"WHERE `depfunc`.func_id = "+str(func2_id) )
		row2 = cur2.fetchone()
		func2_numimp = row2[0]

		cur2.execute( "SELECT COUNT(*) FROM `depfunc` AS df1, `depfunc` AS df2 "\
			"WHERE df1.func_id = "+str(func1_id)+" "\
			"AND df2.func_id = "+str(func2_id)+" "\
			"AND df1.impfunc_id = df2.impfunc_id" )
		row2 = cur2.fetchone()
		numimp = row2[0]

		conn2.close()
	
		outfile.write(str(file1_id)+"\t"+str(func1_id)+"\t"+func1_name+"\t"+str(func1_cyccomp)+"\t"+str(func1_numimp)+"\t"+\
			str(file2_id)+"\t"+str(func2_id)+"\t"+func2_name+"\t"+str(func2_cyccomp)+"\t"+str(func2_numimp)+"\t"+\
			str(cosim)+"\t"+str(numimp)+"\t"+str(dist)+"\n")
		outfile.flush()

		row1 = cur1.fetchone()

	conn1.close()

except MySQLdb.Error, e:
	print "Error %d: %s" % (e.args[0], e.args[1])

outfile.close()
