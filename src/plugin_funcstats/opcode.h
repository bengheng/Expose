#ifndef __OPCODE_HEADER__
#define __OPCODE_HEADER__

#include <log4cpp/Category.hh>
//#include <mysql/mysql.h>
#include <function_analyzer.h>

void GetOpcode(
	//MYSQL *conn,
	function_analyzer *pFunc,
	const unsigned int func_id,
	char *opcode,
	const size_t opcodeLen,
	log4cpp::Category &log );

#endif
