#include <mysql/mysql.h>
#include <iostream>
#include <list>
#include <algorithm>
#include <FileDist.h>
#include <log4cpp/Category.hh>

using namespace std;

int QueryImpFuncs(
	MYSQL *conn,
	unsigned int func_id,
	list< unsigned int > &depfuncs,
	log4cpp::Category *log )
{
	char buf[MAXSTR];
	MYSQL_RES	*res;
	MYSQL_ROW	row;

	// Retrieve all functions for func
	snprintf( buf, sizeof(buf),
		"SELECT `impfunc_id` FROM `depfunc` WHERE `func_id`=%u",
		func_id );

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

	while( (row = mysql_fetch_row(res) ) != NULL )
	{
		depfuncs.push_back( (unsigned int) atoi(row[0] ) );
	}

	mysql_free_result(res);

	return 0;
}

/*!
 * Returns true if the imported functions by func1 is a
 * subset of those by func2.
 * */
bool SAT_impfunc2(
	MYSQL *conn,
	unsigned int func1_id,
	unsigned int func2_id,
	log4cpp::Category *log )
{
	list< unsigned int > f1depfuncs;
	list< unsigned int > f2depfuncs;


	// Gets all imported functions for func2
	if( QueryImpFuncs( conn, func2_id, f2depfuncs, log ) != 0 )
	{
		log->fatal("%s Line %d: Error querying imported functions.", __FILE__, __LINE__);
		return false;
	}

	// Retrieve all functions for func1
	char		buf[512];
	MYSQL_RES	*res;
	MYSQL_ROW	row;

	snprintf( buf, sizeof(buf),
		"SELECT `impfunc_id` FROM `depfunc` WHERE `func_id`=%u",
		func1_id );
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

	while( (row = mysql_fetch_row(res) ) != NULL )
	{
		list< unsigned int >::iterator found;
		found = find( f2depfuncs.begin(), f2depfuncs.end(), (unsigned int)atoi(row[0]) );
		if( found == f2depfuncs.end() )
		{
			mysql_free_result( res );
			return false;
		}
	}
	mysql_free_result(res);

	return true;
}


/*!
Returns the number of imported functions.
*/
unsigned int GetNumImp( MYSQL *conn, const unsigned int func_id, log4cpp::Category *log )
{
	char		buf[512];
	MYSQL_RES	*res;
	MYSQL_ROW	row;

	snprintf( buf, sizeof(buf),
		"SELECT COUNT(`impfunc_id`) FROM `depfunc` WHERE `func_id`=%u",
		func_id );
	if( mysql_query( conn, buf ) != 0 ||
	( res = mysql_store_result( conn ) ) == NULL )
	{
		PRNTSQLERR( log, conn );
		return 0;
	}

	unsigned int impcount = 0;
	if( (row = mysql_fetch_row(res)) == NULL )
	{
		PRNTSQLERR( log, conn );
		return 0;
	}

	impcount = (unsigned int) atoi(row[0]);
	mysql_free_result(res);

	return impcount;
}

/*!
 * Returns true if the number of imported functions by func1 is
 * smaller than that for func2.
 * */
bool SAT_impfunc(
	MYSQL *conn,
	const unsigned int func1_id,
	const unsigned int func2_id,
	log4cpp::Category *log )
{
	unsigned int func1_numimps = GetNumImp( conn, func1_id, log );
	unsigned int func2_numimps = GetNumImp( conn, func2_id, log );

	return func1_numimps <= func2_numimps;
}

