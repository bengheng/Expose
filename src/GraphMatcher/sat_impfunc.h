#ifndef __SAT_IMPFUNC_HEADER__
#define __SAT_IMPFUNC_HEADER__

#include <mysql/mysql.h>
#include <list>
#include <FileDist.h>
#include <log4cpp/Category.hh>

using namespace std;

bool SAT_impfunc(
	MYSQL *conn,
	const unsigned int func1_id,
	const unsigned int func2_id,
	log4cpp::Category *log );

#endif
