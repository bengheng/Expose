#include <mysql/mysql.h>
#include <iostream>
#include <map>
#include <math.h>
#include <distance.h>
#include <stdlib.h>
#include <string.h>
#include <cstdio>

using namespace std;

typedef unsigned int			NGRAM_ID;
typedef pair<unsigned int, float>	FREQ;

/*!
 * Retrieves the ngram frequencies for the specified function.
 * Returns 0 if successful.
 * */
static int GetNgramFreq(
	MYSQL *conn,
	const unsigned int func_id,
	map<NGRAM_ID, FREQ> &ngramFreq )
{
	MYSQL_RES	*res = NULL;
	MYSQL_ROW	row;
	char		buf[512];

	snprintf( buf, sizeof(buf), "SELECT ngram_id, freq FROM `frequency` "\
		"WHERE func_id=%u", func_id );
	if( mysql_query( conn, buf ) != 0 )
	{
		PRNTSQLERR( conn );
		return -1;
	}
	res = mysql_store_result( conn );

	while( (row = mysql_fetch_row(res)) != NULL  )
	{
		ngramFreq.insert( pair<NGRAM_ID, FREQ>(atoi(row[0]),
			pair<unsigned int, float>(atoi(row[1]), 0) ) );
	}

	mysql_free_result( res );
	return 0;
}


/*!
 * Retrieves the frequency count for all the ngrams in func1 and func2.
 * Returns negative number if failed.
 */
static int GetFreqCount(
	MYSQL *conn,
	unsigned int func1_id,
	unsigned int func2_id )
{
	MYSQL_RES	*res = NULL;
	MYSQL_ROW	row;
	char		buf[512];
	int		freqcount;

	snprintf( buf, sizeof(buf), "SELECT COUNT(*) FROM ("\
		"SELECT ngram_id FROM `frequency` WHERE func_id=%u UNION "\
		"SELECT ngram_id FROM `frequency` WHERE func_id=%u) AS tmp",
		func1_id, func2_id);
	if( mysql_query( conn, buf ) != 0 )
	{
		PRNTSQLERR( conn );
		return -1;
	}
	res = mysql_store_result( conn );
	row = mysql_fetch_row( res );
	freqcount = atoi(row[0]);

	mysql_free_result( res );
	return freqcount;
}

/*!
 * Returns mean of all the ngram frequencies in ngramFreq.
 * */
static float ComputeMean( map<NGRAM_ID, FREQ> &ngramFreq )
{
	unsigned int totalFreq = 0;

	map<NGRAM_ID, FREQ>::iterator b, e;
	for( b = ngramFreq.begin(), e = ngramFreq.end();
	b != e; ++b )
	{
		totalFreq +=  (*b).second.first;
	}

	return ((float) totalFreq / (float) ngramFreq.size());
}

/*!
 * Centers input frequencies at the mean.
 * */
static void CenterAtMean( map<NGRAM_ID, FREQ> &ngramFreq, float mean )
{
	map<NGRAM_ID, FREQ>::iterator b, e;
	for( b = ngramFreq.begin(), e = ngramFreq.end();
	b != e; ++b )
	{
		(*b).second.second = (*b).second.first - mean;
	}
}

/*!
 * Computes covariance matrix. Since we have a nx1 matrix, it's easy.
 * We just multiply each element by itself.
 * */
static float ComputeCovariance( map<NGRAM_ID, FREQ> &ngramFreq )
{
	float total = 0;
	map<NGRAM_ID, FREQ>::iterator b, e;
	for( b = ngramFreq.begin(), e = ngramFreq.end();
	b != e; ++b )
	{
		total += (*b).second.second * (*b).second.second;
	}

	// cout << "Cov total: " << total << endl;

	return ( total/(float) ngramFreq.size() );
}

static void UpdateDatabase(
	MYSQL *conn,
	const unsigned int expt_id,
	const unsigned int func1_id,
	const unsigned int func2_id,
	const float mahal )
{
	MYSQL_RES	*res = NULL;
	MYSQL_ROW	row;
	char		buf[512];


	snprintf( buf, sizeof(buf),
		"INSERT INTO `dist_mahal` (expt_id, func1_id, func2_id, mahal) "\
		"VALUES (%u, %u, %u, %f) "\
		"ON DUPLICATE KEY UPDATE mahal=%f",
		expt_id, func1_id, func2_id, mahal, mahal);
	// cout << "DEBUG: " << mahal << endl;
	if( mysql_query( conn, buf ) != 0 )
	{
		PRNTSQLERR( conn );
		return;
	}

	if( mysql_query( conn, "COMMIT" ) != 0 )
	{
		PRNTSQLERR( conn );
		return;
	}
}

/*!
 * A = (a1, a2, ..., an)
 * B = (b1, b2, ..., bn)
 * Euclidean distance = sqrt( (a1 - b1)^2 + (a2-b2)^2 + ... + (an-bn)^2 )
 * */
int DoMahal(
	MYSQL *conn,
	const unsigned int expt_id,
	const unsigned int func1_id,
	const unsigned int func2_id)
{
	unsigned int		ngramFreqCount;
	map<NGRAM_ID, FREQ>	ngramFreq1;
	map<NGRAM_ID, FREQ>	ngramFreq2;
	float			ngramF1mean;
	float			ngramF2mean;
	float			ngramMeanDiff;
	float			cov1;
	float			cov2;
	float			pooledCov;
	float			mahal = 0;

	// Get ngram frequencies
	if( GetNgramFreq( conn, func1_id, ngramFreq1 ) != 0 )
	{
		cout << "Error retrieving ngram frequencies for func_id " << func1_id << ".\n";
		return -1;
	}
	if( GetNgramFreq( conn, func2_id, ngramFreq2 ) != 0 )
	{
		cout << "Error retrieving ngram frequencies for func_id " << func2_id << ".\n";
		return -1;
	}

	ngramFreqCount = ngramFreq1.size() + ngramFreq2.size();
	if( ngramFreqCount < 0 )
	{
		cout << "Error getting the number of frequencies.\n";
		return -1;
	}

	// Find means
	ngramF1mean = ComputeMean( ngramFreq1 );
	ngramF2mean = ComputeMean( ngramFreq2 );
	ngramMeanDiff = ngramF1mean - ngramF2mean;
	// cout << "mean1: " << ngramF1mean << "\tmean2: " << ngramF2mean << endl;
	// cout << "meandiff: " << ngramMeanDiff << endl;
	
	// Centers frequencies at mean
	CenterAtMean( ngramFreq1, ngramF1mean );
	CenterAtMean( ngramFreq2, ngramF2mean );

	// Computes Covariance
	cov1 = ComputeCovariance( ngramFreq1 );
	cov2 = ComputeCovariance( ngramFreq2 );

	// Computes pooled covariance
	pooledCov = ( (float)ngramFreq1.size() * cov1 + (float)ngramFreq2.size() * cov2 ) / (float)ngramFreqCount;
	// cout << "pooledCov: " << pooledCov << endl;

	// Computes mahalanobis
	if( ngramMeanDiff != (float)0 || pooledCov != (float)0 )
		mahal = sqrt( ngramMeanDiff * ( (float) 1 / pooledCov ) * ngramMeanDiff );

	// cout << "Mahal for " << sqlfunc1[0] << ", " << sqlfunc2[0] << " is " << mahal << "." << endl;
	UpdateDatabase( conn, expt_id, func1_id, func2_id, mahal );

	return 0;
}
