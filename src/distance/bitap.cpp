#include <iostream>
#include <mysql/mysql.h>
#include <libbitap.h>
#include <math.h>
#include <distance.h>
#include <cstdio>
#include <string.h>
using namespace std;

/*!
 * Gets the opcode from the database.
 * */
static int GetOpcodes(MYSQL *conn, const unsigned int func_id, char *opcodes, size_t opcodesLen)
{
	MYSQL_RES	*res = NULL;
	MYSQL_ROW	row;
	char		buf[512];

	snprintf( buf, sizeof(buf), "SELECT hex(opcode) FROM `opcode` WHERE func_id=%u", func_id );
	if( mysql_query( conn, buf) != 0 )
	{
		PRNTSQLERR( conn );
		return -1;
	}
	res = mysql_store_result( conn );
	
	if( mysql_num_rows(res) != 1 )
	{
		cout << "Wrong number of rows! " << buf <<  endl;
		return -1;
	}

	while( ( row = mysql_fetch_row( res )  ) != NULL )
	{
		snprintf( opcodes, opcodesLen, "%s", row[0] );
	}

	mysql_free_result( res );
	return 0;
}

int DoBitap(
	MYSQL *conn,
	const unsigned int expt_id,
	const unsigned int func1_id,
	const unsigned int func2_id)

{
	int		i, e, cnt;
	bitapType	b;
	const char	*begin, *end;
	char		opcodes2[4096];
	char		opcodes1[4096];
	char		buf[512];
	MYSQL_RES	*res = NULL;
	MYSQL_ROW	row;

	// Get the opcodes
	if( GetOpcodes( conn, func1_id, opcodes1, sizeof(opcodes1) ) != 0 )
		 return -1;
	if( GetOpcodes( conn, func2_id, opcodes2, sizeof(opcodes2) ) != 0 )
		 return -1;

	// Do Bitap
	NewBitap(&b, opcodes2);

	e = floor( ((float)strlen(opcodes1)) * 0.1 );
	if( NULL == ( end = FindWithBitap(&b, opcodes1, strlen(opcodes1), e, &cnt, &begin) ))
	{
		cnt = -1;
	}

	snprintf( buf, sizeof(buf),
		"INSERT INTO `dist_bitap` (expt_id, func1_id, func2_id, bitap) "\
		"VALUES (%u, %u, %u, %d) "\
		"ON DUPLICATE KEY UPDATE bitap=%d",
		expt_id, func1_id, func2_id, cnt, cnt );

	if( mysql_query( conn, buf) != 0 )
	{
		PRNTSQLERR( conn );
		return -1;
	}

	if( mysql_query( conn, "COMMIT" ) != 0 )
	{
		PRNTSQLERR( conn );
		return -1;
	}
	
	return 0;
}
