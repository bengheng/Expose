#ifndef __EUCLID_H__
#define __EUCLID_H__

#include <mysql/mysql.h>

int DoEuclid(
	MYSQL *conn,
	const unsigned int expt_id,
	const unsigned int func1_id,
	const unsigned int func2_id);

#endif
