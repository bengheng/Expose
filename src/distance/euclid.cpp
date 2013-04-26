#include <iostream>
#include <mysql/mysql.h>
#include <math.h>
#include <distance.h>
#include <stdlib.h>
#include <cstdio>

using namespace std;

/*!
 * Sums up the squares of the frequencies of
 * the ngram_id found in func1 and not func2.
 */
unsigned int CompDistFreqSqr(
	MYSQL *conn,
	const unsigned int func1_id,
	const unsigned int func2_id )
{
	unsigned int	sum = 0;
	char		buf[512];
	MYSQL_RES	*res = NULL;
	MYSQL_ROW	row;

	snprintf( buf, sizeof(buf), "SELECT freq "\
		"FROM frequency "\
		"WHERE func_id=%u "\
		"AND ngram_id NOT IN "\
		"(SELECT ngram_id "\
		"FROM frequency "\
		"WHERE func_id=%u)",
		func1_id, func2_id );
	if( mysql_query( conn, buf) != 0 )
	{
		PRNTSQLERR( conn );
		return -1;
	}
	res = mysql_use_result( conn );
	
	while( ( row = mysql_fetch_row( res )  ) != NULL )
	{
		unsigned int a;
		a = atoi(row[0]);
		sum += (a) * (a);
	}
	mysql_free_result( res );

	return sum;
}

/*!
 * A = (a1, a2, ..., an)
 * B = (b1, b2, ..., bn)
 * Euclidean distance = sqrt( (a1 - b1)^2 + (a2-b2)^2 + ... + (an-bn)^2 )
 * */

int DoEuclid(
	MYSQL *conn,	
	const unsigned int expt_id,
	const unsigned int func1_id,
	const unsigned int func2_id )

{
	unsigned int	sum = 0;
	float		euclid = 0;
	MYSQL_RES	*res = NULL;
	MYSQL_ROW	row;
	char		buf[512];


	//
	// Sums up the squares of the frequencies of
	// the ngram_id found in both freq1 and freq2.
	//
	snprintf( buf, sizeof(buf), "SELECT freq1.freq, freq2.freq "\
		"FROM frequency as freq1, frequency as freq2 "\
		"WHERE freq1.func_id=%u "\
		"AND freq2.func_id=%u "\
		"AND freq1.ngram_id = freq2.ngram_id",
		func1_id, func2_id );
	if( mysql_query( conn, buf) != 0 )
	{
		PRNTSQLERR( conn );
		return -1;
	}
	res = mysql_use_result( conn );
	
	// Computes Euclidean distance term for the two frequencies
	while( ( row = mysql_fetch_row( res )  ) != NULL )
	{
		unsigned int a, b;
		a = atoi(row[0]);
		b = atoi(row[1]);
		sum += (a - b) * (a - b);
	}
	mysql_free_result( res );

	sum += CompDistFreqSqr( conn, func1_id, func2_id );
	sum += CompDistFreqSqr( conn, func2_id, func1_id );

	//	
	// Compute Euclidean distance
	//

	euclid = sqrt( sum );
	
	snprintf( buf, sizeof(buf),
		"INSERT INTO `dist_euclid` (expt_id, func1_id, func2_id, euclid) "\
		"VALUES (%u, %u, %u, %f) "\
		"ON DUPLICATE KEY UPDATE euclid=%f",
		expt_id, func1_id, func2_id, euclid, euclid);

	if( mysql_query( conn, buf ) != 0 )
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


