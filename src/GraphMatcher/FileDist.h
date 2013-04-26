#ifndef __FILEDIST_HEADER__
#define __FILEDIST_HEADER__

#include <log4cpp/Category.hh>
#include <vector>
#include <mysql/mysql.h>
#include <function.h>
#include <fcg.h>

#include <rextranslator.h>

#ifndef MAXSTR
#define MAXSTR	1024
#endif

#define PRNTSQLERR( l, c )\
	l->fatal("%s Line %d: %s", __FILE__, __LINE__, mysql_error( c ) )

using namespace std;

/*
int InitFuncVec(
	MYSQL *conn,
	const unsigned int file_id,
	const bool SQL_CACHE,
	vector< Function * > &funcVec,
	log4cpp::Category *pLog );
*/

int InitFCG(
	MYSQL *conn,	
	const unsigned int file_id,
	const bool SQL_CACHE,
	FCGraph &fcg,
	log4cpp::Category *pLog );

void FreeFuncVec( vector< Function * > &funcVec );

int FileDist(
	MYSQL *conn,
	const unsigned int expt_id,
	FCGraph *const pFCG1,
	const unsigned int file2Id,
	log4cpp::Category *log,
	RexTranslator *_rextrans,
	log4cpp::Category *_rexlog );

#endif
