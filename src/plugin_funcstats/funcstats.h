#ifndef __FUNCSTATS_HEADER__
#define __FUNCSTATS_HEADER__

#include <map>
#include <vector>
#include <mysql/mysql.h>
#include <log4cpp/Category.hh>
#include <function.h>

using namespace std;

#ifndef SMALL_FUNC_LIMIT
#define SMALL_FUNC_LIMIT 300
#endif

int AnalyzeFuncs(
	MYSQL				*conn,
	vector< Function * >		&vecFunctions,
	unsigned short *const		instrTable,
	const size_t			maxTableElem,
	log4cpp::Category		&log );

/*
int UpdIntrnCall(
	MYSQL *conn,
	map< ea_t, vector<ea_t>* > &mapIntrnCall,
	map< ea_t, unsigned int > &funcEA2Id,
	log4cpp::Category &log );
*/

#endif
