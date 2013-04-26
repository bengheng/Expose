#include <mysql/mysql.h>
#include <iostream>
#include <FileDist.h>
#include <stdlib.h>
#include <math.h>
#include <log4cpp/Category.hh>

using namespace std;

/*
int QueryCycComp( MYSQL *conn, unsigned int func_id )
{
	char buf[MAXSTR];
	MYSQL_RES	*res;
	MYSQL_ROW	row;

	// Retrieve all functions for file1
	snprintf( buf, sizeof(buf),
		"SELECT `cyccomp` FROM `function` WHERE `func_id`=%u",
		func_id );

	if( mysql_query( conn, buf ) != 0 )
	{
		PRNTSQLERR( conn );
		return -1;
	}

	if( ( res = mysql_store_result( conn ) ) == NULL )
	{
		PRNTSQLERR( conn );
		return -1;
	}

	if( mysql_num_rows(res) != 1 )
	{
		PRNTSQLERR( conn );
		return -1;
	}

	row = mysql_fetch_row(res);
	int cyccomp = atoi(row[0]);

	mysql_free_result( res );

	return cyccomp;
}
*/

bool SAT_cyccomp(
	const unsigned int func1_id,
	const unsigned int func2_id,
	const unsigned int func1_cyccomp,
	const unsigned int func2_cyccomp,
	log4cpp::Category *log )
{
	//int func1cyccomp, func2cyccomp;

	//func1cyccomp = QueryCycComp( conn, func1_id );
	//func2cyccomp = QueryCycComp( conn, func2_id );

	// We relax the constraints for comparing cyclomatic
	// complexities such that the function 2's
	// complexity can be 2 less than function 1's.

	unsigned int upper = (unsigned int) floor(0.2 * func1_cyccomp);
	if( upper < 2 ) upper = 2;

	return func1_cyccomp <= (func2_cyccomp + upper);
	//return func1cyccomp <= (func2cyccomp+2);
}
