#ifndef __BITAP_H__
#define __BITAP_H__

#include <mysql/mysql.h>

int DoBitap(
	MYSQL *conn,
	const unsigned int expt_id,
	const unsigned int func1_id,
	const unsigned int func2_id );

#endif
