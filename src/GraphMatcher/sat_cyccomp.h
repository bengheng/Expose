#ifndef __SAT_CYCCOMP_HEADER__
#define __SAT_CYCCOMP_HEADER__

#include <mysql/mysql.h>
#include <FileDist.h>
#include <log4cpp/Category.hh>

bool SAT_cyccomp(
	const unsigned int func1_id,
	const unsigned int func2_id,
	const unsigned int func1_cyccomp,
	const unsigned int func2_cyccomp,
	log4cpp::Category *log );

#endif
