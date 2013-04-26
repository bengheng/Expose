#ifndef __SAT_COSIM_HEADER__
#define __SAT_COSIM_HEADER__

#include <mysql/mysql.h>
#include <log4cpp/Category.hh>

float QueryCosim(
	MYSQL *conn,
	const unsigned int expt_id,
	const unsigned int func1_id,
	const unsigned int func2_id,
	log4cpp::Category *log );

bool SAT_cosim(
	MYSQL *conn,
	const unsigned int expt_id,
	const unsigned int func1_id,
	const unsigned int func2_id,
	float *const cosim,
	log4cpp::Category *log );

#endif
