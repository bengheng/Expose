#include <mysql/mysql.h>
#include <iostream>
#include <FileDist.h>
#include <cstdio>
#include <stdlib.h>
#include <log4cpp/Category.hh>

using namespace std;

float QueryCosim(
	MYSQL *conn,
	const unsigned int expt_id,
	const unsigned int func1_id,
	const unsigned int func2_id,
	log4cpp::Category *log )
{
	char buf[MAXSTR];
	MYSQL_RES	*res;
	MYSQL_ROW	row;

	// Retrieve all functions for file1
	snprintf( buf, sizeof(buf),
		"SELECT SQL_NO_CACHE `cosim` FROM `dist_cosim` WHERE `expt_id`=%u AND `func1_id`=%u AND func2_id=%u",
		expt_id, func1_id, func2_id );

	if( mysql_query( conn, buf ) != 0 )
	{
		PRNTSQLERR( log, conn );
		return -1;
	}

	if( ( res = mysql_store_result( conn ) ) == NULL )
	{
		PRNTSQLERR( log, conn );
		return -1;
	}

	int num_rows = mysql_num_rows(res);
	if( num_rows != 1 && num_rows != 0 )
	{
		PRNTSQLERR( log, conn );
		return -1;
	}

	// cosim 1 is furthest, 0 is nearest.
	float cosim = 1;
	if( num_rows == 1 )
	{
		row = mysql_fetch_row(res);
		cosim = atof(row[0]);
	}

	mysql_free_result( res );

	return cosim;
}


bool SAT_cosim(
	MYSQL *conn,
	const unsigned int expt_id,
	const unsigned int func1_id,
	const unsigned int func2_id,
	float *const cosim,
	log4cpp::Category *log )
{
	*cosim = QueryCosim( conn, expt_id, func1_id, func2_id, log );

	return (*cosim < 0.4);
}
