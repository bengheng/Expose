#include <log4cpp/Category.hh>
#include <ida.hpp>
#include <idp.hpp>
#include <funcs.hpp>
#include <strlist.hpp>
#include <allins.hpp>
#include <function_analyzer.h>
#include <set>
#include <map>
#include <vector>
#include <deque>
#include <utility>
#include <string>
#include <mysql/mysql.h>
#include <blacklist.h>
#include <opcode.h>
#include <function.h>
#include <ngram.h>
#include <funcstats.h>

using namespace std;

#define PRNTSQLERR( log, conn )\
	log.fatal( "%s Line %d: %s", __FILE__, __LINE__, mysql_error(conn) )
	

void RemoveControlChars( char *buf, const size_t bufsize )
{
	char bufcpy[MAXSTR] = { '\0' };
	size_t len;

	// Makes a copy of the buffer. The original
	// buffer will be overwritten.
	qstrncpy(bufcpy, buf, sizeof(bufcpy)-1 );
	len = strlen(bufcpy);
	memset(buf, 0, bufsize);

	for(int i = 0, j = 0; i < len && j < bufsize; ++i, ++j)
	{
		char c = bufcpy[i];
		if( c >= 0 && c <= 31 ||
		c == 127 ||
		c == '\\' ||
		c == '\"' ||
		c == '\'' )
		{
			qsnprintf( &buf[j], bufsize - strlen(buf), "\\%02x", c );
			j += 2;
		}
		else
		{
			buf[j] = c;
		}		
	}
}

int UpdBBs(
	MYSQL *conn,
	Function *const pFn,
	log4cpp::Category &log )
{
	unsigned int func_id = pFn->GetFuncId();

	const char* preBuf = "INSERT INTO `basicblock` (func_id, bb_startEA, bb_endEA, insns, next1, next2) VALUES ";
	size_t preBufLen = strlen(preBuf);

	const size_t MAXBUFLEN = 1048576;
	char *buf = NULL;
	if( (buf = new char[MAXBUFLEN]) == NULL )
	{
		log.fatal("%s Line %d: Can't allocate buffer for ngram!", __FILE__, __LINE__);
		return -1;
	}
	qsnprintf( buf, MAXBUFLEN, "%s", preBuf );
	size_t bufLen = strlen(buf);


	log.info( "DEBUG Updating BBs for %s ", pFn->GetName() );


	BasicBlock bb;
	bb.insns.str = new char[4096];
	bb.size = 4096;

	int res = pFn->GetFirstBB( &bb );
	log.info( "DEBUG First BB res is %d", res );
	while( res == 0 )
	{

		log.info("DEBUG\t %08x %08x", bb.startEA, bb.endEA);
		//--------------------------------------------------------
		// Tries to pack as many INSERTs as we can into one query
		//--------------------------------------------------------
		char oneVal[512];
		size_t oneValLen = 0;

		qsnprintf(oneVal, sizeof(oneVal), "%s(%u, %u, %u, 0x%s, %d, %d)",
			bufLen == preBufLen ? "" : ", ",
			func_id, bb.startEA, bb.endEA, bb.insns.str, bb.nextbb1, bb.nextbb2 );

		oneValLen = strlen(oneVal);

		// If we can pack in the new value, do it.
		// Otherwise, oneVal is full, so send it.
		if( (bufLen + oneValLen) < MAXBUFLEN )
		{
			qsnprintf( &buf[bufLen], MAXBUFLEN - bufLen, "%s", oneVal );

			// Update new bufLen
			bufLen += oneValLen;
		}
		else
		{
			// Reached max capacity of values.
			if( mysql_query(conn, buf) != 0 )
			{
				log.fatal( "%s", buf );
				log.fatal("%s Line %d: %s", __FILE__, __LINE__, mysql_error(conn));

				delete [] buf;
				delete bb.insns.str;
				return -1;
			}

			// Reset buf with preBuf and the new value
			qsnprintf( buf, MAXBUFLEN, "%s(%u, %u, %u, 0x%s, %d, %d)",
				preBuf, func_id, bb.startEA, bb.endEA,
				bb.insns.str, bb.nextbb1, bb.nextbb2 );
			bufLen = strlen(buf);
		}

		res = pFn->GetNextBB( &bb );
	}

	// Checks if there's values in buf. If so, send it.
	// This will almost always be true, except for the
	// worst case where there are no functions at all.
	if( bufLen > preBufLen)
	{
		if( mysql_query( conn, buf ) != 0 )
		{
			log.fatal( "%s", buf );
			log.fatal("%s Line %d: %s", __FILE__, __LINE__, mysql_error(conn));

			delete [] buf;
			delete bb.insns.str;
			return -1;
		}
	}

	delete [] buf;
	delete bb.insns.str;
	return 0;
}

// Insert function size, cyclomatic complexity and level.
int UpdStats(
	MYSQL			*conn,
	Function *const		pFn,
	//#ifdef USE_OPCODE
	//const char		*opcodes,
	//#endif
	log4cpp::Category	&log )
{
	MYSQL_ROW	row;
	MYSQL_RES	*result;
	char		buf[MAXSTR];

	if( pFn->GetSize() < SMALL_FUNC_LIMIT )
	{
		qsnprintf( buf, sizeof(buf),
			"INSERT IGNORE INTO `function` (`func_name`, `file_id`, `startEA`, `endEA`, "
			//#ifdef USE_OPCODE
			//"`func_size`, `cyccomp`, `num_vars`, `num_params`, `level`, `compo`, `opcode` ) "
			//"VALUES (\"%s\", %u, %u, %u, %u, %u, %u, %u, %u, %u, 0x%s) "
			//#else
			"`func_size`, `cyccomp`, `num_vars`, `num_params`, `level`, `compo` ) "
			"VALUES (\"%s\", %u, %u, %u, %u, %u, %u, %u, %u, %u) "
			//#endif
			"ON DUPLICATE KEY UPDATE `func_size`=VALUES(`func_size`), "
			"`cyccomp`=VALUES(`cyccomp`), "
			"`num_vars`=VALUES(`num_vars`), "
			"`num_params`=VALUES(`num_params`), "
			"`level`=VALUES(`level`), "
			"`compo`=VALUES(`compo`), "
			//#ifdef USE_OPCODE
			//"`opcode`=VALUES(`opcode`), "
			//#endif
			"`func_id`=LAST_INSERT_ID(`func_id`)",
			pFn->GetName(), pFn->GetFileId(),
			pFn->GetStartEA(), pFn->GetEndEA(),
			pFn->GetSize(),
			pFn->GetCycComp(),
			pFn->GetNumVars(),
			pFn->GetNumParams(),
			pFn->GetLevel(),
			pFn->GetCompoNum()
			//#ifdef USE_OPCODE
			//, opcodes
			//#endif
			);
	}
	else
	{
		qsnprintf( buf, sizeof(buf),
			"INSERT IGNORE INTO `function` (`func_name`, `file_id`, `startEA`, `endEA`, "
			"`func_size`, `cyccomp`, `num_vars`, `num_params`, `level`, `compo` ) "
			"VALUES (\"%s\", %u, %u, %u, %u, %u, %u, %u, %u, %u) "
			"ON DUPLICATE KEY UPDATE `func_size`=VALUES(`func_size`), "
			"`cyccomp`=VALUES(`cyccomp`), "
			"`num_vars`=VALUES(`num_vars`), "
			"`num_params`=VALUES(`num_params`), "
			"`level`=VALUES(`level`), "
			"`compo`=VALUES(`compo`), "
			"`func_id`=LAST_INSERT_ID(`func_id`)",
			pFn->GetName(), pFn->GetFileId(),
			pFn->GetStartEA(), pFn->GetEndEA(),
			pFn->GetSize(),
			pFn->GetCycComp(),
			pFn->GetNumVars(),
			pFn->GetNumParams(),
			pFn->GetLevel(),
			pFn->GetCompoNum() );
	}

	if( mysql_query( conn, buf ) != 0 )
	{
		PRNTSQLERR( log, conn );
		return -1;
	}

	// Set the function id
	pFn->SetFuncId( mysql_insert_id(conn) );

	return 0;
}

// Returns maximum component number.
unsigned short AnalyzeLevelHelper(
	Function *const pFn,
	unsigned short uLvl,
	unsigned short uTnt,
	vector< Function *> &vecCompo,
	log4cpp::Category &log )
{
	unsigned short uMaxCompoNo = 0;

	// Sanity check
	if( pFn == NULL ) return 0;

	// Set function call graph level for function
	if( uLvl > pFn->GetLevel() )
	{
		pFn->SetLevel( uLvl );
	}

	vecCompo.push_back(pFn);

	// Set taint
	pFn->SetTaint( uTnt );

	// For each child, recurse
	Function *pChild = pFn->GetFirstChild();
	while( pChild != NULL )
	{
		if( pChild->GetTaint() < uTnt )
			uMaxCompoNo = max( AnalyzeLevelHelper( pChild, uLvl+1, uTnt, vecCompo, log ), uMaxCompoNo );
		
		pChild = pFn->GetNextChild();
	}

	return max( uMaxCompoNo, pFn->GetCompoNum() );
}

// Updates descendants.
int AnalyzeLevel(
	MYSQL *conn,
	vector< Function * > &vecFunctions,
	log4cpp::Category &log )
{
	vector< Function* > vecCompo;


	// For every rank 2 function, update level of descendants
	unsigned short uTaint = 1;
	unsigned short uNextCompoNum = 0;
	vector< Function * >::iterator bFn, eFn;
	for( bFn = vecFunctions.begin(), eFn = vecFunctions.end();
	bFn != eFn; ++bFn )
	{
		// Skip ranks lower than 2. We only want to begin from
		// the highest ranked function.
		if( (*bFn)->GetRank() < 2 )
		{
			//log.info("DEBUG Skipping rank < 2 function %08x", (*bFn)->GetStartEA() );
			continue;
		}

		unsigned short uCompoNum = AnalyzeLevelHelper( *bFn, 0, uTaint, vecCompo, log );
		if( uCompoNum == 0 )
		{
			uCompoNum = ++uNextCompoNum;
		}

		// Set component number
		vector< Function *>::iterator bCmp, eCmp;
		for( bCmp = vecCompo.begin(), eCmp = vecCompo.end();
		bCmp != eCmp; ++bCmp )
		{
			Function *pFn = *bCmp;
			//log.info("DEBUG Setting comp %08x %u (%s)",
			//	pFn->GetStartEA(), uCompoNum, pFn->GetName() );
			pFn->SetCompoNum(uCompoNum);
		}

		vecCompo.clear();

		++uTaint;
	}

	// Union components
	/*
	unsigned short uCompoNum = 1;
	vector< set<unsigned short>* >::iterator bCompo, eCompo;
	for( bCompo = vecCompo.begin(), eCompo = vecCompo.end();
	bCompo != eCompo; ++bCompo )
	{
		bool fUpdCompoNum = false;

		set<unsigned short> *pTntSet = *bCompo;
		for( bFn = vecFunctions.begin(), eFn = vecFunctions.end();
		bFn != eFn; ++bFn )
		{
			if( pTntSet->find( (*bFn)->GetTaint() ) != pTntSet->end() )
			{
				log.info( "DEBUG COMPO %08x\t%u [%u]",
					(*bFn)->GetStartEA(), uCompoNum, (*bFn)->GetTaint() );
				(*bFn)->SetCompoNum(uCompoNum);
				fUpdCompoNum = true;
			}
		}

		if( fUpdCompoNum == true ) ++uCompoNum;
	}
	*/

	// Remaining functions that have rank 0 are individual components.
	for( bFn = vecFunctions.begin(), eFn = vecFunctions.end();
	bFn != eFn; ++bFn )
	{
		if( (*bFn)->GetRank() == 0 )
		{
			//log.info("DEBUG2 COMPO %08x\t%u", (*bFn)->GetStartEA(), uNextCompoNum);
			(*bFn)->SetCompoNum(++uNextCompoNum);
		}
	}


	//log.info("DEBUG final taint %u final component num %u", uTaint, uNextCompoNum );	

	// Free allocated memory
	/*
	vector<Function*>::iterator bcmp, ecmp;
	for( bcmp = vecCompo.begin(), ecmp = vecCompo.end();	
	bcmp != ecmp; ++bcmp )
	{
		delete (*bcmp);
		*bcmp = NULL;
	}
	*/

	return 0;
}

// Updates database with internal function calls.

int UpdIntrnCall(
	MYSQL *conn,
	vector< Function* > &vecFunctions,
	log4cpp::Category &log )
{
	const char* preBuf = "INSERT INTO `fncalls` (fn_parent, fn_child) VALUES ";
	size_t preBufLen = strlen(preBuf);

	const size_t MAXBUFLEN = 1048576;
	char *buf = NULL;
	if( (buf = new char[MAXBUFLEN]) == NULL )
	{
		log.fatal("%s Line %d: Can't allocate buffer for internal calls!", __FILE__, __LINE__);
		return -1;
	}
	qsnprintf( buf, MAXBUFLEN, "%s", preBuf );
	size_t bufLen = strlen(buf);

	vector<Function*>::iterator b1, e1;
	for( b1 = vecFunctions.begin(), e1 = vecFunctions.end();
	b1 != e1; ++b1 )
	{
		Function *pChild = (*b1)->GetFirstChild();
		while( pChild )
		{
			char oneVal[32];
			size_t oneValLen = 0;

			if( bufLen == preBufLen )
			{
				// Have not inserted any value
				qsnprintf( oneVal, sizeof(oneVal), "(%d, %d)",
					(*b1)->GetFuncId(), pChild->GetFuncId() );
			}
			else
			{
				// Not the first, so we need the comma
				qsnprintf( oneVal, sizeof(oneVal), ", (%d, %d)",
					(*b1)->GetFuncId(), pChild->GetFuncId() );
			}

			oneValLen = strlen(oneVal);

			// If we can pack in the new value, do it.
			// Otherwise, oneVal is full, so send it.
			if( (bufLen + oneValLen) < MAXBUFLEN )
			{
				qsnprintf( &buf[bufLen], MAXBUFLEN - bufLen, "%s", oneVal );

				// Update new bufLen
				bufLen += oneValLen;
			}
			else
			{
				// Reached max capacity of values, send it.
				if( mysql_query(conn, buf) != 0 )
				{
					log.fatal( "%s", buf );
					log.fatal( "%s Line %d: %s",
						__FILE__, __LINE__,
						mysql_error(conn) );

					delete [] buf;
					return -1;
				}

				// Reset buf with preBuf and the new value
				qsnprintf( buf, MAXBUFLEN, "%s(%d, %d)",
					preBuf, (*b1)->GetFuncId(), pChild->GetFuncId() );
				bufLen = strlen(buf);
			}

			// log.info("\t%d -> %d", funcEA2Id[b->first], funcEA2Id[b->second]);

			pChild = (*b1)->GetNextChild();
		}
	}

	// Checks if there's values in buf. If so, send it.
	// This will almost always be true, except for the
	// worst case where there are no functions at all.
	if( bufLen > preBufLen )
	{
		if( (mysql_query(conn, buf)) != 0 )
		{
			log.fatal("%s Line %d: %s",
				__FILE__, __LINE__,
				mysql_error(conn));
			delete [] buf;
			return -1;
		}
	}

	delete [] buf;

	return 0;
}


// Update database with imported functions.
int UpdXtrnCall(
	MYSQL *conn,
	Function *const pFn,
	log4cpp::Category &log)
{
	MYSQL_ROW	row;
	char		buf[MAXSTR];

	set< string > *pXtrnCall = pFn->GetXtrnCall();
	set< string >::iterator xtrnCallB, xtrnCallE;

	for( xtrnCallB = pXtrnCall->begin(), xtrnCallE = pXtrnCall->end();
	xtrnCallB != xtrnCallE; ++xtrnCallB )
	{
		qsnprintf( buf, sizeof(buf),
			"INSERT IGNORE INTO `impfunc` (impfunc_name) VALUE(\"%s\") "\
			"ON DUPLICATE KEY UPDATE impfunc_id=LAST_INSERT_ID(impfunc_id)",
			(*xtrnCallB).c_str() );
		if( mysql_query( conn, buf ) != 0 )
		{
			log.fatal( "%s", buf );
			PRNTSQLERR( log, conn );
			return -1;
		}

		unsigned int impfunc_id = mysql_insert_id(conn);
	
		// Inserts into depfunc table
		qsnprintf( buf, sizeof(buf),
			"INSERT IGNORE INTO `depfunc` (func_id, impfunc_id) VALUES(%u, %u) ",
			pFn->GetFuncId(), impfunc_id );

		if( mysql_query( conn, buf ) != 0 )
		{
			log.fatal( "%s", buf );
			PRNTSQLERR( log, conn );
			return -1;
		}

		mysql_query( conn, "COMMIT" );
	}

	return 0;
}



// Update database with referenced strings.
int UpdRefStr(
	MYSQL *const		conn,
	Function *const		pFn,
	log4cpp::Category	&log)
{
	MYSQL_ROW	row;
	char 		buf[1111];	// The size of buffer is the sum of 1000 bytes and maximum requirement for SQL query.

	set< pair<ea_t, size_t> > *pRefStr = pFn->GetRefStr();
	set< pair<ea_t, size_t> >::iterator refstrB, refstrE;

	for( refstrB = pRefStr->begin(), refstrE = pRefStr->end();
	refstrB != refstrE; ++refstrB )
	{
		unsigned int refstr_id = 0;

		// Gets the string. 1000 is the maximum string length
		// that MySQL can accept for the string to be unique,
		// i.e. used as an index.
		char str[1000];
		if( get_ascii_contents2( (*refstrB).first, (*refstrB).second, ASCSTR_C,
			str, sizeof(str) ) == false )
			continue;

		str[999] = '\0'; // Ensure always NULL terminated

		RemoveControlChars( str, sizeof(str) );

		// Checks if the string is already in the database.
		// If not, insert.
	
		qsnprintf( buf, sizeof(buf),
			"INSERT IGNORE INTO `refstr` (`strval`) VALUE( \"%s\" ) ON DUPLICATE KEY UPDATE refstr_id=LAST_INSERT_ID(refstr_id)",
			str );
		if( mysql_query( conn, buf ) != 0 )
		{
			log.fatal( "%s", buf );
			PRNTSQLERR( log, conn );
			return -1;
		}
		refstr_id = mysql_insert_id( conn );

		// Inserts into depstr table
		qsnprintf( buf, sizeof(buf),
			"INSERT IGNORE INTO `depstr` (func_id, refstr_id) VALUES(%u, %u) ",
			pFn->GetFuncId(), refstr_id );

		if( mysql_query( conn, buf ) != 0 )
		{
			log.fatal( "%s", buf );
			PRNTSQLERR( log, conn );
			return -1;
		}

		mysql_query( conn, "COMMIT" );
	}

	return 0;
}

void InsertFuncStats(
	MYSQL				*conn,
	Function *const			pFn,
	log4cpp::Category 		&log )
{
	/*
	#ifdef USE_OPCODE
	char opcodes[4096];
	opcodes[0] = 0x0;
	if( pFn->GetSize() < SMALL_FUNC_LIMIT )
	{
		for( ea_t addr = pFn->GetStartEA(), offset = 0;
		addr < pFn->GetEndEA() && offset < 4096;
		++addr, offset+=2 )
		{
			size_t len = strlen( opcodes );
			qsnprintf( &opcodes[offset], 4096-len, "%02x", get_byte(addr) );
		}
	}

	//GetOpcode( pFunc, func_id, opcodes, sizeof(opcodes), log );
	if( UpdStats( conn, pFn, opcodes, log ) == -1 )
		return;

	// Insert basic block addresses
	if( pFn->GetSize() < SMALL_FUNC_LIMIT )
	{
		if( UpdBBs( conn, pFn, log ) == -1 )
			return;
	}
	#else
	*/
	if( UpdStats( conn, pFn, log ) == -1 )
		return;
	//#endif

	if( pFn->GetSize() < SMALL_FUNC_LIMIT &&
	UpdBBs( conn, pFn, log ) == -1 )
		return;

	// Insert imported functions
	if( UpdXtrnCall( conn, pFn, log ) == -1 )
		return;

	// Insert referenced strings
	if( UpdRefStr( conn, pFn, log ) == -1 )
		return;

	mysql_query( conn, "COMMIT" );
}

/*!
Analyzes all functions.
*/
int AnalyzeFuncs(
	MYSQL			*conn,
	vector<Function *>	&vecFunctions,
	unsigned short *const	instrTable,
	const unsigned int	maxTableElem,
	log4cpp::Category	&log )
{
	//
	// For every function, compute the following:
	// 1. ngram
	// 2. cyclomatic complexity
	// 3. dependencies
	//
	vector< Function * >::iterator bFn, eFn;
	for( bFn = vecFunctions.begin(), eFn = vecFunctions.end();
	bFn != eFn; ++bFn )
	{
		Function *pFn = *bFn;

		log.info("Analyzing %s", pFn->GetName());

		// Analyzes function size, cyclomatic complexity, dependencies.
		pFn->ParseFuncInstrs( &vecFunctions );
		log.info("Analyzed %s", pFn->GetName());
	}

	// Analyzes function level in function call graph
	log.info("Analyzing levels...");
	AnalyzeLevel( conn, vecFunctions, log );
	log.info("Done anaylzing levels.");
	for( bFn = vecFunctions.begin(), eFn = vecFunctions.end();
	bFn != eFn; ++bFn )
	{
		Function *pFn = *bFn;

		// This will assign func_id
		InsertFuncStats( conn, pFn, log );

		// Extract ngram. Needs to come after InsertFuncStats
		// for func_id to be valid.
		ngram( conn, pFn, instrTable, maxTableElem, log );
	}

	UpdIntrnCall( conn, vecFunctions, log );

	return 0;
}

