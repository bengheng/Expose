#include <mysql/mysql.h>
#include <iostream>
#include <list>
#include <FileDist.h>
#include <function.h>
#include <fcg.h>
#include <aligner.h>
#include <sat_impfunc.h>
#include <sat_cyccomp.h>
#include <sat_cosim.h>
#include <cstdio>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <sys/time.h>
#include <set>
#include <rextranslator.h>

using namespace std;

#define INVALID_PFUNCINFO	NULL
#define INVALID_COSIM		-1

typedef struct
{
	unsigned int func_id;
	char func_name[MAXSTR];
	unsigned int startEA;
	unsigned int endEA;
	unsigned int cyccomp;
	unsigned int func_size;
	unsigned int num_ngrams;
} FUNCINFO, *PFUNCINFO;

// This structure contains the various
// data we need to collect to compute
// the quality of the final score.
typedef struct
{
	unsigned int		f1MinStartEA;
	unsigned int		f1MaxStartEA;
	set<unsigned int>	f2CandSet;	// List of candidate addresses from file2
	unsigned int		f2SumCandAddr;	// Sum of candidate addresses from file2
	float			f2CandAddrMean;
	float			f2CandAddrSumDev;
	float			f2CandAddrStdDev;
} QUALITY, *PQUALITY;

/*
 * Initialize function call edges.
 */
static int InitEdges(
	MYSQL *conn,
	FCGraph &fcg,
	FCVertex &parFunc,
	log4cpp::Category *pLog )
{
	char		buf[MAXSTR];
	MYSQL_RES	*res;
	MYSQL_ROW	row;

	// Sanity check
	if( parFunc == FCG_NULL_VERTEX ) return -1;

	snprintf( buf, sizeof(buf),
		"SELECT `fn_child` FROM `fncalls` WHERE `fn_parent`=%u",
		fcg.GetVertexFuncId(parFunc) );
	if( mysql_query( conn, buf) != 0 )
	{
		PRNTSQLERR( pLog, conn );
		return -1;
	}

	if( ( res = mysql_store_result( conn ) ) == NULL )
	{
		PRNTSQLERR( pLog, conn );
		return -1;
	}

	// Insert children
	while( (row = mysql_fetch_row(res) ) != NULL )
	{
		unsigned int childFuncId = (unsigned int) atoi(row[0]);

		// Add edge if found child vertex in fcg
		FCVertex childFunc = fcg.FindVertex( childFuncId );
		if( childFunc != FCG_NULL_VERTEX )
		{
			fcg.AddEdge( parFunc, childFunc ); // Add edge
		}
	}

	mysql_free_result(res);	

	return 0;
}

int InitFuncBBs(
	MYSQL *conn,
	FCGraph &fcg,
	FCVertex &v,
	log4cpp::Category *pLog )
{
	char		buf[MAXSTR];
	MYSQL_RES	*res;
	MYSQL_ROW	row;

	snprintf( buf, sizeof(buf),
		"SELECT SQL_NO_CACHE `bb_startEA`, `bb_endEA`, `insns`, `next1`, `next2` "
		"FROM `basicblock` WHERE `func_id`=%u",
		fcg.GetVertexFuncId(v) );
	
	if( mysql_query( conn, buf ) != 0 )
	{
		PRNTSQLERR( pLog, conn );
		return -1;
	}

	if( ( res = mysql_store_result( conn ) ) == NULL )
	{
		PRNTSQLERR( pLog, conn );
		return -1;
	}


	// The bbs vector is freed in the destructor of fcg.
	vector<BasicBlock*> *bbs = new vector<BasicBlock*>();
	assert( bbs != NULL );
	while( (row = mysql_fetch_row(res) ) != NULL )
	{
		BasicBlock *bb = new BasicBlock();
		bb->startEA = atoi(row[0]);
		bb->endEA = atoi(row[1]);
		bb->size = bb->endEA - bb->startEA;

		bb->insns.bin = new unsigned char[bb->size];
		memcpy( bb->insns.bin, row[2], bb->size );

		bb->nextbb1 = atoi(row[3]);
		bb->nextbb2 = atoi(row[4]);
		bbs->push_back( bb );
	}
	
	mysql_free_result( res );

	return fcg.AddVertexBBs( v, bbs );
}

/*!
 * Initialize function call graph.
 */
int InitFCG(
	MYSQL *conn,	
	const unsigned int file_id,
	const bool SQL_CACHE,
	FCGraph &fcg,
	log4cpp::Category *pLog )
{
	char		buf[MAXSTR];
	MYSQL_RES	*res;
	MYSQL_ROW	row;


	// Retrieve all functions for file1
	snprintf( buf, sizeof(buf),
		"SELECT %s `func_id`, `func_name`, `startEA`, `endEA`, "
		"`func_size`, `cyccomp`, `num_vars`, `num_params`, "
		"`num_ngrams`, `level`, `compo` "
		"FROM `function` WHERE `file_id`=%u",
		SQL_CACHE ? "SQL_CACHE" : "SQL_NO_CACHE",
		file_id );
	
	if( mysql_query( conn, buf ) != 0 )
	{
		PRNTSQLERR( pLog, conn );
		return -1;
	}

	if( ( res = mysql_store_result( conn ) ) == NULL )
	{
		PRNTSQLERR( pLog, conn );
		return -1;
	}

	while( (row = mysql_fetch_row(res) ) != NULL )
	{
		size_t size = atoi(row[4]);

		// Allocate function
		if( fcg.AddVertex(
			(unsigned int) atoi(row[0]),	// function id
			row[1],				// function name
			(ea_t) atoi(row[2]),		// start EA
			(ea_t) atoi(row[3]),		// end EA
			(size_t) atoi(row[4]),		// size
			(unsigned int) atoi(row[5]),	// cyclomatic complexity
			(unsigned int) atoi(row[6]),	// number of vars
			(unsigned int) atoi(row[7]),	// number of params
			(unsigned int) atoi(row[8]),	// number of ngrams
			(unsigned int) atoi(row[9]),	// level
			(unsigned int) atoi(row[10]) )	// component number
		== FCG_NULL_VERTEX )
			return -1;
	}
	
	mysql_free_result( res );


	// Initialize function calls
	FCVertex parFunc = fcg.GetFirstVertex();
	while( parFunc != FCG_NULL_VERTEX )
	{
		if( fcg.GetVertexSize( parFunc ) < SMALL_FUNC_LIMIT )
		{
			InitFuncBBs( conn, fcg, parFunc, pLog );
		}
		InitEdges( conn, fcg, parFunc, pLog );
		parFunc = fcg.GetNextVertex();
	}

	return 0;
}



/*!
Computes pairwise distance between functions in file1 and file2.
*/
static int FuncDist(
	MYSQL *conn,
	const unsigned int expt_id,
	const unsigned file1_id,
	const unsigned file2_id,
	list< PFUNCINFO > &f1funcs,
	list< PFUNCINFO > &f2funcs,
	map< PFUNCINFO, pair<PFUNCINFO, float> > &loCosim,
	QUALITY &qualInfo,
	log4cpp::Category *log )
{

	// Create a satisfiability matrix
	// bool **satMatrix = new bool*[ f1funcs.size() ];
	// for( int i = 0; i < f1funcs.size(); ++i )
	//{
	//	satMatrix[i] = new bool[ f2funcs.size() ];
	//	memset( satMatrix[i], true, sizeof(bool)*f2funcs.size() );
	//}

	const char *preBuf = "INSERT INTO `dist` (expt_id, func1_id, func2_id, dist) VALUES ";
	size_t preBufLen = strlen(preBuf);

	const char *postBuf = " ON DUPLICATE KEY UPDATE dist=VALUES(dist)";
	size_t postBufLen = strlen(postBuf);

	// Allocate 10MB for values so that we can issue multiple
	// inserts to speed up database

	const size_t MAXBUFLEN = 1048576;
	char *buf = NULL;
	if( (buf = new char[MAXBUFLEN]) == NULL )
	{
		log->fatal("%s Line %d: Can't allocate memory for values!", __FILE__, __LINE__);
		return -1;
	}
	snprintf( buf, MAXBUFLEN, "%s", preBuf );
	size_t bufLen = strlen(buf);

	// For each function in file1...
	list< PFUNCINFO >::iterator f1b, f1e;
	f1b = f1funcs.begin();
	f1e = f1funcs.end();
	while( f1b != f1e )
	{
		PFUNCINFO pFI1 = *f1b;

		// For each function in file2...
		list< PFUNCINFO >::iterator f2b, f2e;
		f2b = f2funcs.begin();
		f2e = f2funcs.end();
		while( f2b != f2e )
		{
			PFUNCINFO pFI2 = *f2b;

			// Skip if any of the child of f1b
			// is an ancestor of f2b

			float cosim = 1;

			// Checks for satisfiability of imported functions,
			// cyclomatic complexities and cosine similarities.
			bool OK_impfunc = SAT_impfunc( conn, pFI1->func_id, pFI2->func_id, log );
			bool OK_cyccomp = SAT_cyccomp( pFI1->func_id, pFI2->func_id, pFI1->cyccomp, pFI2->cyccomp, log );
			bool OK_cosim = SAT_cosim( conn, expt_id, pFI1->func_id, pFI2->func_id, &cosim, log );

			bool OK = OK_impfunc && OK_cyccomp && OK_cosim;

			// If constraints are satisfied...
			if( OK == true )
			{
				// update the cosine similarity for f1 to the lower one
				if( cosim < loCosim[pFI1].second )
				{
					loCosim[pFI1].first = pFI2;
					loCosim[pFI1].second = cosim;
				}

				// Append to list of matched addrs if the function size
				// is greater than 200 bytes. If they are too small,
				// they may be inlined, causing a false negative.
				if( pFI1->func_size > 200 &&
				qualInfo.f2CandSet.find(pFI2->startEA) == qualInfo.f2CandSet.end() )
				{
					qualInfo.f2CandSet.insert(pFI2->startEA);
					qualInfo.f2SumCandAddr += pFI2->startEA;
				}
			}

			size_t oneValLen = 0;
			char oneVal[32];
			if( bufLen == preBufLen )
			{
				// Have not inserted any value
				snprintf(oneVal, sizeof(oneVal), "(%u, %u, %u, %d)",
					expt_id, pFI1->func_id, pFI2->func_id, OK);
			}
			else if( bufLen > preBufLen )
			{
				// Not the first, so we need the comma.
				snprintf(oneVal, sizeof(oneVal), ", (%u, %u, %u, %d)",
					expt_id, pFI1->func_id, pFI2->func_id, OK);
			}
			oneValLen = strlen(oneVal);

			// If we can pack in the new value, do it.
			// Otherwise, values is full, so send it.
			if( (bufLen + oneValLen + postBufLen) < MAXBUFLEN )
			{
				snprintf( &buf[bufLen], MAXBUFLEN - bufLen, "%s", oneVal );
				
				assert(strlen(buf) < MAXBUFLEN);
				// Update new bufLen and bufPtr
				bufLen += oneValLen;
			}
			else
			{
				// Reached max capacity of values.
				// Append the postBuf and send it.
				snprintf( &buf[bufLen], MAXBUFLEN - bufLen, "%s", postBuf );

				if( mysql_query( conn, buf ) != 0 )
				{
					PRNTSQLERR( log, conn );
					delete [] buf;
					return -1;
				}

				// Reset buf with preBuf and the new value
				snprintf( buf, MAXBUFLEN, "%s(%u, %u, %u, %d)",
					preBuf, expt_id, pFI1->func_id, pFI2->func_id, OK);
				bufLen = strlen( buf );
			}

			++f2b;
		}
		++f1b;
	}

	// Checks if there's values in buf. If so, send it.
	// This will almost always be true, except for the
	// worst case where there are no functions at all.
	if( bufLen > preBufLen)
	{
		snprintf( &buf[bufLen], MAXBUFLEN - bufLen, "%s", postBuf );
		if( mysql_query( conn, buf ) != 0 )
		{
			PRNTSQLERR( log, conn );
			delete [] buf;
			return -1;
		}
	}

	if( mysql_query( conn, "COMMIT" ) != 0 )
	{
		PRNTSQLERR( log, conn );
		return -1;
	}

	
	delete [] buf;

	return 0;
}

/*!
Insert score into database
*/
int UpdScore( MYSQL *conn,
	const unsigned int file2_id,
	const long elapsed_ms,
	const unsigned int expt_id,
	const unsigned int range,
	const float stddev,
	const float score,
	log4cpp::Category *log  )
{
	char buf[512];
	snprintf(buf, sizeof(buf),
		"INSERT INTO `result_approx` (`file_id`, `elapsed_ms`, `expt_id`, `timestamp`, `range`, `stddev`, `score`) "\
		"VALUES(%u, %lu, %u, NOW(), %u, %f, %f) "\
		"ON DUPLICATE KEY UPDATE `elapsed_ms`=VALUES(`elapsed_ms`), `timestamp`=NOW(), `range`=VALUES(`range`), `stddev`=VALUES(`stddev`), `score`=VALUES(`score`)",
		file2_id, elapsed_ms, expt_id, range, stddev, score );

	log->info("DEBUG Updating score %f\n", score);

	if( mysql_query( conn, buf ) != 0 )
	{
		PRNTSQLERR( log, conn );
		return -1;
	}

	return 0;
}


/*!
Deletes the file's idb/i64 file for for the given file id.
Use this because we are desparate for space!
*/
void DelFile( MYSQL *conn,
	const unsigned int file_id,
	log4cpp::Category *log )
{
	MYSQL_RES	*res;
	MYSQL_ROW	row;
	char buf[512];
	snprintf(buf, sizeof(buf), "SELECT `path_sha1`, `nodename`, `type` FROM `file`, `client` "\
		"WHERE `file_id`=%u AND `client`.client_id = `file`.`file_id`", file_id );
	if( mysql_query( conn, buf ) != 0 )
	{
		PRNTSQLERR( log, conn );
		return;
	}

	if( ( res = mysql_store_result( conn ) ) == NULL )
	{
		PRNTSQLERR( log, conn );
		return;
	}

	if( (row = mysql_fetch_row(res) ) == NULL )
	{
		PRNTSQLERR( log, conn );
		return;
	}

	char relfilepath[512];
	if( strstr( row[2], "64-bit" ) != NULL )
		snprintf(relfilepath, sizeof(relfilepath), "../mnt/idb/approx/%s/%s.idb", row[1], row[0]);
	else
		snprintf(relfilepath, sizeof(relfilepath), "../mnt/idb/approx/%s/%s.i64", row[1], row[0]);
	
	remove(relfilepath);
}

/*!
 * De-allocates memory for all elements in function vector.
 */
void FreeFuncVec( vector< Function * > &funcVec )
{
	size_t n = funcVec.size();
	while( n != 0 )
	{
		Function *pFunc = funcVec.back();
		funcVec.pop_back();
		delete pFunc;
		--n;
	}
}	

/*!
 * */
void ExptOrderByName(
	MYSQL *conn,
	const unsigned int expt_id,
	vector< Function * > *const pFile1FuncVec,
	vector< Function * > *const pFile2FuncVec,
	log4cpp::Category *pLog )
{
	float cosim;
	float totalcosim = 0;
	unsigned int count = 0;

	pLog->info( "=== EXPT: Order By Name ===" );

	// For each function in file 2
	vector< Function * >::iterator b2, e2;
	for( b2 = pFile2FuncVec->begin(), e2 = pFile2FuncVec->end();
	b2 != e2; ++b2 )
	{
		//size_t func2len = strlen((*b2)->GetName());

		// For each funciton in file 1
		vector< Function * >::iterator b1, e1;
		for( b1 = pFile1FuncVec->begin(), e1 = pFile1FuncVec->end();
		b1 != e1; ++b1 )
		{
			// If functions have same name, return cosim
			if( strcmp( (*b2)->GetName(), (*b1)->GetName() ) == 0 )
			{
			
				SAT_cosim( conn, expt_id, (*b1)->GetFuncId(), (*b2)->GetFuncId(), &cosim, pLog );
				pLog->info("%f\t%s, %s", cosim, (*b1)->GetName(), (*b2)->GetName() );
				totalcosim += cosim;
				++count;
			}
		}
	}

	// Result summary
	pLog->info("Total Cosim: %f", totalcosim);
	pLog->info("Num same names: %u", count);
	pLog->info("Ave Cosim: %f", totalcosim/((float) count));
}

/*!
 * List functions from file 1 with corresponding functions from
 * file 2 having lowest cosim.
 */
void ExptOrderByCosim(
	MYSQL *conn,
	const unsigned int expt_id,
	vector< Function * > *const pFile1FuncVec,
	vector< Function * > *const pFile2FuncVec,
	log4cpp::Category *pLog )
{

	pLog->info("=== EXPT: Order By Cosim ===");

	// For each function in file 1
	vector< Function * >::iterator b1, e1;
	for( b1 = pFile1FuncVec->begin(), e1 = pFile1FuncVec->end();
	b1 != e1; ++b1 )
	{

		float lowestCosim = 1;

		// Find lowest cosim in file 2
		vector< Function * >::iterator b2, e2;
		for( b2 = pFile2FuncVec->begin(), e2 = pFile2FuncVec->end();
		b2 != e2; ++b2 )
		{
			float cosim = 1;
			SAT_cosim( conn, expt_id, (*b1)->GetFuncId(), (*b2)->GetFuncId(), &cosim, pLog );
			if( cosim < lowestCosim ) lowestCosim = cosim;
		}


		pLog->info( "%f\t%s :", lowestCosim, (*b1)->GetName() );
		if( lowestCosim == 1 )
		{
			pLog->info("\t\t*");
		}
		else
		{
			// List all functions in file 2 having lowest cosim
			for( b2 = pFile2FuncVec->begin(), e2 = pFile2FuncVec->end();
			b2 != e2; ++b2 )
			{
				float cosim = 1;
				SAT_cosim( conn, expt_id, (*b1)->GetFuncId(), (*b2)->GetFuncId(), &cosim, pLog );
				if( cosim == lowestCosim )
				{
					pLog->info("\t\t%s", (*b2)->GetName() );
				}
			}
		}
	}
	
}


/*!
 * Recursive function to list children ordered by size.
 */
void PreOrderHelper(
	unsigned int n,
	//vector< Function * > &vecParents,
	Function *pParent,
	set< Function *> &track,
	log4cpp::Category *pLog )
{
	vector< Function * > ordFuncs;
	vector< Function * >::iterator b, e;//, parB, parE;

	// For each parent
	//for( parB = vecParents.begin(), parE = vecParents.end();
	//parB != parE; ++parB )
	//{
	//	Function *pParent = *parB;

		// Order children by increasing size
		Function *pChild = pParent->GetFirstChild();
		while( pChild != NULL )
		{
			// If child has been accessed, skip
			if( track.find( pChild ) != track.end() )
			{
				pChild = pParent->GetNextChild();
				continue;
			}


			// Insert child in increasing size
			for( b = ordFuncs.begin(), e = ordFuncs.end();
			b != e; ++b )
			{
				if( pChild->GetSize() < (*b)->GetSize() )
				{
					ordFuncs.insert( b, pChild );
					break;
				}
			}
			if( b == e)
				ordFuncs.push_back( pChild );

			// Child has been inserted
			track.insert( pChild );

			pChild = pParent->GetNextChild();
		}
	//}

	if( ordFuncs.size() == 0 ) return;

	// List child in increasing size order
	for( b = ordFuncs.begin(), e = ordFuncs.end();
	b != e; ++b )
	{
		// track.insert( *b );
		pLog->info("%u\t%u\t%u\t%s", n, (*b)->GetLevel(), (*b)->GetSize(), (*b)->GetName());
	}

	// Recurse
	//PreOrderHelper( ++n, ordFuncs, track, pLog );
	for( b = ordFuncs.begin(), e = ordFuncs.end();
	b != e; ++b )
	{
		PreOrderHelper( n, (*b), track, pLog );
	}
}

/*!
 * For each function at level 0, call helper.
 *
 */
void PreOrder(
	vector< Function * > *const pFuncVec,
	log4cpp::Category *pLog	)
{
	vector< Function * > ordFuncs;
	vector<Function *>::iterator b1, e1, b2, e2;
	
	// Order children by increasing size
	for( b1 = pFuncVec->begin(), e1 = pFuncVec->end();
	b1 != e1; ++b1 )
	{
		if( (*b1)->GetLevel() != 0 )	continue;

		for( b2 = ordFuncs.begin(), e2 = ordFuncs.end();
		b2 != e2; ++b2 )
		{
			if( (*b1)->GetSize() < (*b2)->GetSize() )
			{
				ordFuncs.insert( b2, (*b1) );
				break;
			}
		}
		if( b2 == e2 )
			ordFuncs.push_back( *b1 );

	}

	pLog->info("Number of Level 0 funcs: %d", ordFuncs.size());

	//unsigned int n = 0;

	// List child in increasing size order
	/*
	for( b2 = ordFuncs.begin(), e2 = ordFuncs.end();
	b2 != e2; ++b2 )
	{
		pLog->info("%u\t%u\t%u\t%s", n, (*b2)->GetLevel(), (*b2)->GetSize(), (*b2)->GetName());
	}
	*/

	// Recurse
	set< Function * > track;
	//PreOrderHelper( ++n, ordFuncs, track, pLog );
	
	unsigned int n = 0;
	for( b2 = ordFuncs.begin(), e2 = ordFuncs.end();
	b2 != e2; ++b2 )
	{
		pLog->info("-- %d --", n);
		pLog->info("%u\t%u\t%u\t%s", n, (*b2)->GetLevel(), (*b2)->GetSize(), (*b2)->GetName());

		// Tracks if function already accessed.
		//set<Function *> track;

		track.insert( *b2 );
		PreOrderHelper( n, (*b2), track, pLog );
		n++;
	}
	

}

void ExptPreOrderTraversal(
	vector< Function * > *const pFile1FuncVec,
	vector< Function * > *const pFile2FuncVec,
	log4cpp::Category *pLog )
{
	pLog->info("=== EXPT: Pre-Order Traversal ===");
	pLog->info("--- File 1 ---");
	PreOrder( pFile1FuncVec, pLog );
	pLog->info("--- File 2 ---");
	PreOrder( pFile2FuncVec, pLog );
}

int FileDist(
	MYSQL *conn,
	const unsigned int expt_id,
	FCGraph *const pFCG1,
	const unsigned int file2Id,
	log4cpp::Category *pLog,
	RexTranslator *_rextrans,
	log4cpp::Category *_rexlog )
{
	struct timeval start, end;
	long elapsed_ms, seconds, useconds;
	gettimeofday(&start, NULL);				// Start Timer	

	FCGraph FCG2(file2Id, pLog);
	if( InitFCG( conn, file2Id, false, FCG2, pLog ) == -1 )
	{
		return -1;
	}
	char fcg2Name[256];
	snprintf(fcg2Name, sizeof(fcg2Name), "%d", file2Id);
	FCG2.SetGraphName(fcg2Name);

	
	Aligner *pAligner = new Aligner( conn, expt_id, _rextrans, pLog, _rexlog );
	size_t	maxMatchPathLen = 0;
	float clusterScore = 1;
	float score = pAligner->ComputeAlignDist3( *pFCG1, FCG2 );
	//float score = pAligner->ComputeAlignDist2( *pFCG1, FCG2 );
	//pAligner->ComputeSmallFuncDist( *pFCG1, FCG2 );
	delete pAligner;
	
	gettimeofday(&end, NULL);				// Stop Time
	seconds = end.tv_sec - start.tv_sec;
	useconds = end.tv_usec - start.tv_usec;
	elapsed_ms = ( (seconds) * 1000 + (long) ceil(useconds/1000.0) );

	// Updates score into database
	if( UpdScore( conn, file2Id, elapsed_ms, expt_id,
	maxMatchPathLen, clusterScore, score, pLog ) != 0 )
		return -1;


	// Can comment this out if we have tons of space!
	// DelFile( conn, file2_id, log );

	return 0;
}
