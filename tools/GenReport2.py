#/usr/local/bin/python2.6

import sys
import MySQLdb

if len(sys.argv) != 2:
	print "Usage: "+sys.argv[0]+" <outfile>"
	sys.exit(0)

_host = "localhost"
_user = "libmatcher"
_passwd = "l1bm@tcher"
_db = "libmatcher"


outfile = open( sys.argv[1], 'w')
outfile.write("file2 id\tfile2 name\t#SameName Match\tTotal Funcs\n");
outfile.flush()
try:
	conn1 = MySQLdb.connect( host=_host, user=_user, passwd=_passwd, db=_db )
	cur1 = conn1.cursor()

	cur1.execute("SELECT `file2`.file_id, "+\
		"`file2`.name, "+\
		"COUNT(*) AS SameName_Match, "+\
		"tmp2.total "+\
		"FROM `dist`, "+\
		"`function` AS `func1`, "+\
		"`function` AS `func2`, "+\
		"`file` AS `file1`, "+\
		"`file` AS `file2`, "+\
		"("+\
		"SELECT `file`.file_id, COUNT(*) AS total FROM `function`, `file`, "+\
		"( SELECT DISTINCT func2_id FROM `dist`) AS tmp1 "+\
		"WHERE tmp1.func2_id = `function`.func_id "+\
		"AND `function`.file_id = `file`.file_id "+\
		"GROUP BY `file`.file_id"+\
		") AS tmp2 "+\
		"WHERE dist = 1 "+\
		"AND `dist`.func1_id = `func1`.func_id "+\
		"AND `func1`.file_id = `file1`.file_id "+\
		"AND `dist`.func2_id = `func2`.func_id "+\
		"AND `func2`.file_id = `file2`.file_id "+\
		"AND `func1`.func_name = `func2`.func_name "+\
		"AND tmp2.file_id  = `file2`.file_id "+\
		"GROUP BY `func2`.file_id")
	row1 = cur1.fetchone()
	while row1 is not None:
		outfile.write("%d\t%s\t%d\t%d\n" % (row1[0], row1[1], row1[2], row1[3]))
		row1 = cur1.fetchone()

except MySQLdb.Error, e:
	print "Error %d: %s" % (e.args[0], e.args[1])

outfile.close()
