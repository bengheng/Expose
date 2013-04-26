#include <log4cpp/Category.hh>
//#include <mysql/mysql.h>
#include <ida.hpp>
#include <idp.hpp>
#include <function_analyzer.h>

static size_t GetOpcodeSize(ea_t opcodeAddr,
	char *opcodePrefix,
	size_t opcodePrefixLen)
{
	unsigned char byteVal;
	size_t len;

	// If the first byte is not 0x0f,
	// then the opcode length is 1.
	byteVal = get_byte( opcodeAddr );
	if( opcodePrefix != NULL )
	{
		len = strlen(opcodePrefix);
		qsnprintf( opcodePrefix + len, opcodePrefixLen - len, "%02x", byteVal );
	}
	if( byteVal != 0x0f ) return 1;

	
	// If the second byte is not 0x38 and 0x3a,
	// then the opcode length is 2.
	byteVal = get_byte( opcodeAddr + 1 );
	if( opcodePrefix != NULL )
	{
		len = strlen(opcodePrefix);
		qsnprintf( opcodePrefix + len, opcodePrefixLen - len, "%02x", byteVal );
	}
	if( byteVal != 0x38 && byteVal != 0x3a ) return 2;

	// Third byte.
	byteVal = get_byte( opcodeAddr + 2 );
	if( opcodePrefix != NULL )
	{
		len = strlen(opcodePrefix);	
		qsnprintf( opcodePrefix + len, opcodePrefixLen - len, "%02x", byteVal );
	}

	return 3;
}

void GetOpcode(
	function_analyzer *pFunc,
	const unsigned int func_id,
	char *opcodes,
	const size_t opcodesLen,
	log4cpp::Category &log )
{
	qsnprintf(opcodes, opcodesLen, "0x");

	// Loop through instructions in function
	int bytes = 1;
	for(ea_t addr = pFunc->get_ea_start();
	addr <= pFunc->get_ea_end(); addr += bytes)
	{
		bool res = false;
		char mnem[MAXSTR];

		res = pFunc->disasm(addr, mnem);
		if( res == true )
		{
			bytes = cmd.size;
			GetOpcodeSize( addr, opcodes, opcodesLen );
		}
		else
		{
			bytes = 1;
		}
	}
}

