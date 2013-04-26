// Computes various distances  between functions of
// the two input files referenced by the file ids.
#include <iostream>
#include <mysql/mysql.h>
#include <math.h>
#include <euclid.h>
#include <cosim.h>
#include <bitap.h>
#include <mahal.h>
#include <distance.h>
#include <map>
#include <log4cpp/FileAppender.hh>
#include <log4cpp/Category.hh>
#include <log4cpp/SimpleLayout.hh>
#include <cstdio>
#include <string.h>
#include <stdlib.h>
using namespace std;

typedef unsigned int FUNCID;
typedef unsigned int NUMNGRAMS;

#define MIN_NUM_NGRAMS_REQUIRED	(NUMNGRAMS)0

/*!
 * Retrieves frequencies for specified func_id.
 * */
int GetFreqs( MYSQL *conn,
	const unsigned int func_id,
	map< NGRAMID, FREQ >  &func_freqs,
	const bool SQL_CACHE,
	log4cpp::Category &log )
{
	unsigned int	denom = 0;
	MYSQL_RES	*res = NULL;
	MYSQL_ROW	row;
	char		buf[512];

	snprintf( buf, sizeof(buf), "SELECT %s ngram_id, freq FROM frequency WHERE func_id=%u",
		SQL_CACHE ? "SQL_CACHE" : "SQL_NO_CACHE",
		func_id);
	if( mysql_query( conn, buf ) != 0 )
	{
		log.fatal( "%s Line %d: %s", __FILE__, __LINE__, mysql_error(conn) );
		return -1;
	}
	res = mysql_store_result( conn );
	while( ( row = mysql_fetch_row( res ) ) != NULL )
	{
		unsigned int ngram_id = atoi( row[0] );
		unsigned int freq = atoi( row[1] );
		
		//denom += (freq * freq);
		func_freqs.insert( pair< NGRAMID, FREQ >( ngram_id, freq ) );
	}
	mysql_free_result( res );

	return 0;
}



int GetFuncIds( MYSQL *conn,
	map<FUNCID, NUMNGRAMS> &func_ids,
	const char *file_id,
	const bool SQL_CACHE,
	log4cpp::Category &log )
{
	MYSQL_RES	*res = NULL;
	MYSQL_ROW	row;
	char		buf[512];

	snprintf(buf, sizeof(buf), "SELECT %s `func_id`, `num_ngrams` FROM `function` WHERE file_id = %s",
		SQL_CACHE ? "SQL_CACHE" : "SQL_NO_CACHE",
		file_id);
	if( mysql_query( conn, buf ) != 0 )
	{
		PRNTSQLERR( conn );
		return -1;
	}

	if( (res = mysql_store_result( conn )) == NULL )
	{
		PRNTSQLERR( conn );
		return -1;
	}

	while( ( row = mysql_fetch_row(res) ) != NULL )	
	{
		unsigned int func_id = (unsigned int) atoi( row[0] );
		unsigned int num_ngrams = (unsigned int) atoi( row[1] );
		func_ids.insert(  pair<FUNCID, NUMNGRAMS>( func_id, num_ngrams ) );
	}

	mysql_free_result( res );

	return 0;
}

int main(int argc, char **argv)
{
	if(argc != 6)
	{
		cout << "Usage: " << argv[0] << " <dbserver> <bitap|cosim|euclid|mahal> <file1_id> <file2_id> <expt_id>" << endl;
		return -1;
	}

	//
	// Setup log4cpp
	//
	log4cpp::FileAppender *appender = new log4cpp::FileAppender("DistFileAppender", "log_distance");
	log4cpp::Layout *layout = new log4cpp::SimpleLayout();
	log4cpp::Category &log = log4cpp::Category::getInstance("DistCategory");
	appender->setLayout(layout);
	log.setAppender(appender);
	log.setPriority(log4cpp::Priority::DEBUG);

	log.info("distance.exe %s %s %s %s %s",
		argv[1], argv[2], argv[3], argv[4], argv[5] );


	const char *usr = "libmatcher";
	const char *pwd = "l1bm@tcher";
	char db[20];
	snprintf(db, sizeof(db)-1, "expt_%s", argv[5]);

	int count = 0;

	MYSQL		*conn;
	int		state;
	map<FUNCID, NUMNGRAMS> func1_ids, func2_ids;

	if( ( conn = mysql_init(NULL) ) == NULL )
	{
		PRNTSQLERR( conn );
		return -1;
	}

	if( !mysql_real_connect( conn, argv[1], usr, pwd, db, 0, 0, 0) )
	{
		PRNTSQLERR( conn );
		mysql_close( conn );
		return -1;
	}

	// Query file 1
	if( GetFuncIds( conn, func1_ids, argv[3], true, log ) == -1 )
	{
		mysql_close( conn );
		return -1;
	}

	// Query file 2
	if( GetFuncIds( conn, func2_ids, argv[4], false, log ) == -1 )
	{
		mysql_close( conn );
		return -1;
	}

	const char *preBuf = "INSERT INTO `dist_cosim` (expt_id, func1_id, func2_id, cosim) VALUES ";
	size_t preBufLen = strlen(preBuf);

	const char *postBuf = " ON DUPLICATE KEY UPDATE cosim=VALUES(cosim)";
	size_t postBufLen = strlen(postBuf);

	const size_t MAXBUFLEN = 1048576;
	char *buf = NULL;
	if( (buf = new char[MAXBUFLEN]) == NULL )
	{
		log.fatal("%s Line %d: Can't allocate buffer for cosim!", __FILE__, __LINE__);
		return -1;
	}
	snprintf( buf, MAXBUFLEN, "%s", preBuf );
	size_t bufLen = strlen(buf);

	// For each function in file 1...
	map<FUNCID, NUMNGRAMS>::iterator f1b, f1e;
	for( f1b = func1_ids.begin(), f1e = func1_ids.end();
	f1b != f1e; ++f1b )
	{
		// If ngram is too small, it'll give rise to many false positives.
		// So, skip.
		if ( f1b->second < MIN_NUM_NGRAMS_REQUIRED )
		{
			log.info("%s %d: func1 %u ngram %u too small",
				__FILE__, __LINE__, f1b->first, f1b->second );
			continue;
		}

		NUMNGRAMS	func1_numngrams = f1b->second;
		NUMNGRAMS	range = (NUMNGRAMS) ceil( func1_numngrams / 2 );
		NUMNGRAMS	upper = func1_numngrams + range;
		NUMNGRAMS	lower = func1_numngrams - range;

	
		// Retrieves frequencies for func1
		map<NGRAMID, FREQ> func1_freqs;
		if( GetFreqs( conn, f1b->first, func1_freqs, true, log ) != 0 ) return -1;
	
		// For each function in file 2...
		map<FUNCID, NUMNGRAMS>::iterator f2b, f2e;
		for( f2b = func2_ids.begin(), f2e = func2_ids.end();
		f2b != f2e; ++f2b )
		{
			// Skip if ngram is too small.
			if ( f2b->second < MIN_NUM_NGRAMS_REQUIRED )
			{
				log.info("%s %d: func2 %u ngram %u too small",
					__FILE__, __LINE__, f2b->first, f2b->second );
				continue;
			}
			
			// If the number of ngrams for func2 is larger than upper
			// or smaller than lower, skip.
			if( ( f2b->second > f1b->second &&
			f2b->second > 3*f1b->second ) ||
			( f1b->second > f2b->second &&
			f1b->second > 3*f2b->second ) )
			//if( f2b->second < lower || f2b->second > upper )
			{
				log.info("%s %d: func1 %u ngram size %u diff too much with "
					"func2 %u ngram size %u (diff is %d)",
					__FILE__, __LINE__,
					f1b->first, f1b->second,
					f2b->first, f2b->second,
					f1b->second > f2b->second ?
						f1b->second - f2b->second :
						f2b->second - f1b->second );
				 continue;
			}

			// Retrieve frequencies for func2
			map<NGRAMID, FREQ> func2_freqs;
			if( GetFreqs( conn, f2b->first, func2_freqs, false, log ) != 0 ) return -1;

			float cosim = 1;
			if( strncmp(argv[2], "cosim", 6) == 0 &&
			GetCosim( func1_freqs, func2_freqs, &cosim, log ) == 0 )
			{

				log.info("DEBUG COSIM %f\t%u, %u", cosim, f1b->first, f2b->first);		

				char oneVal[128];
				size_t oneValLen = 0;
		
				if( bufLen == preBufLen )
				{	// Have not inserted any value
					snprintf(oneVal, sizeof(oneVal), "(%u, %u, %u, %f)",
						atoi(argv[5]), f1b->first, f2b->first, cosim);
				}
				else
				{	// Not the first, so we need the comma.
					snprintf(oneVal, sizeof(oneVal), ", (%u, %u, %u, %f)",
						atoi(argv[5]), f1b->first, f2b->first, cosim);
				}

				oneValLen = strlen(oneVal);
		
				// If we can pack in the new value, do it.
				// Otherwise, values is full, so send it.
				if( (bufLen + oneValLen + postBufLen) < MAXBUFLEN )
				{
					snprintf( &buf[bufLen], MAXBUFLEN - bufLen, "%s", oneVal );

					// Update new bufLen
					bufLen += oneValLen;
				}
				else
				{	// Reached max capacity of values.
					// Append the postBuf and send it.
					snprintf( &buf[bufLen], MAXBUFLEN - bufLen, "%s", postBuf );
					if( mysql_query( conn, buf ) != 0 )
					{
						PRNTSQLERR( conn );
						log.fatal("%s Line %d: %s", __FILE__, __LINE__, mysql_error(conn));

						delete [] buf;
						return -1;
					}

					// Reset buf with preBuf and the new value
					snprintf( buf, MAXBUFLEN, "%s(%u, %u, %u, %f)",
						preBuf, atoi(argv[5]), f1b->first, f2b->first, cosim);
					bufLen = strlen( buf );
				}


				count++;
			}
			else if( strncmp(argv[2], "euclid", 7) == 0 &&
				DoEuclid( conn, (unsigned int)atoi(argv[5]), f1b->first, f2b->first ) == 0 )
			{
				count++;
			}
			else if( strncmp(argv[2], "bitap", 6) == 0 )
			{
				if( DoBitap( conn, (unsigned int)atoi(argv[5]), f1b->first, f2b->first ) == 0 )
				{
					count++;
				}
				else
				{
					cout << "ERROR DoBitap!\n";
				}
			}
			else if( strncmp(argv[2], "mahal", 6) == 0 )
			{
				if( DoMahal( conn, (unsigned int) atoi( argv[5] ), f1b->first, f2b->first ) == 0 )
				{
					count++;
				}
				else
				{
					cout << "ERROR DoMahal!\n";
				}
			}
		}
	}

	// Checks if there's values in buf. If so, send it.
	// This will almost always be true, except for the
	// worst case where there are no functions at all.
	if( bufLen > preBufLen)
	{
		snprintf( &buf[bufLen], MAXBUFLEN - bufLen, "%s", postBuf );
		if( mysql_query( conn, buf ) != 0 )
		{
			log.fatal("%s Line %d: %s", __FILE__, __LINE__, buf);
			PRNTSQLERR( conn );

			delete [] buf;
			return -1;
		}
	}

	delete [] buf;

	if( mysql_query( conn, "COMMIT" ) != 0 )
	{
		PRNTSQLERR( conn );
		return -1;
	}

	log.info("Finished distance.exe %s %s %s %s %s",
		argv[1], argv[2], argv[3], argv[4], argv[5] );

	mysql_close( conn );

	return 0;
}
