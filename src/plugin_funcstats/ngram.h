#ifndef __NGRAM_H__
#define __NGRAM_H__

#include <log4cpp/Category.hh>
#include <ida.hpp>
#include <mysql/mysql.h>
#include <function.h>

void ngram(
	MYSQL *const connection,
	Function *const pFn,
	unsigned short *const instrTable,
	const size_t instrTableMaxElem,
	log4cpp::Category &log );

#endif
