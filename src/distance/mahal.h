#ifndef __MAHAL_HEADER__
#define __MAHAL_HEADER__

#include <mysql/mysql.h>

int DoMahal(
	MYSQL *conn,
	const unsigned int expt_id,
	const unsigned int func1_id,
	const unsigned int func2_id);

#endif
