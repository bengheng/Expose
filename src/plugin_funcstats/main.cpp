#include <log4cpp/Category.hh>
#include <log4cpp/FileAppender.hh>
#include <log4cpp/SimpleLayout.hh>
#include <ida.hpp>
#include <idp.hpp>
#include <loader.hpp>
#include <funcs.hpp>
#include <time.h>
#include <mysql.h>
//#include <mysql/mysql.h>
//#include <mysql_connection.h>
//#include <mysql_driver.h>
//#include <cppconn/exception.h>
#include <blacklist.h>
//#include <ngram.h>
//#include <opcode.h>
#include <funcstats.h>
#include <function.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <map>
#include <vector>

using namespace std;

char IDAP_comment[] = "Extract function statistics.";
char IDAP_help[] = "funcstats";
#ifdef __X64__
char IDAP_name[] = "funcstats64";
#else
char IDAP_name[] = "funcstats32";
#endif
char IDAP_hotkey[] = "Alt-F3";

int IDAP_init(void);
void IDAP_term(void);
void IDAP_run(int arg);

plugin_t PLUGIN =
{
	IDP_INTERFACE_VERSION,		// IDA version plug-in is written for
	PLUGIN_PROC,			// Flags
	IDAP_init,			// Initialisation function
	IDAP_term,			// Clean-up function
	IDAP_run,			// Main plug-in body
	IDAP_comment,			// Comment - unused
	IDAP_help,			// As above - unused
	IDAP_name,			// Plug-in name shown in Edit->Plugins menu
	IDAP_hotkey			// Hot key to run the plug-in
};

int IDAP_init(void)
{
	// Do checks here to ensure your plug-in is being used within
	// an environment it was written for. Return PLUGIN_SKIP if the
	// checks fail, otherwise return PLUGIN_KEEP.
	msg("%s init()\n", IDAP_name);

	return PLUGIN_KEEP;
}

void IDAP_term(void)
{
	// Stuff to do when exiting, generally you'd put any sort
	// of clean-up jobs here.
	msg("%s term()\n", IDAP_name);

	return;
}

/*!
 * Returns the path and nodename when the condition is fulfilled.
 *
 * The condition is defined by "libmatcher" followed by "mnt" in fullpath.
 * The subsequent token is the nodename, followed by the file path.
 * For example, for the following fullpath,
 *
 * /a/b/libmatcher/mnt/c/file/path
 *
 * the returned path_buffer is "/file/path" and the nodename is "c".
 *
 */
void GetPath( const char*fullpath,
	char *path_buffer, size_t path_buffer_size,
	char *nodename, size_t nodename_size )
{
	char *token;
	char fullpathcpy[512];
	bool hasCondition = false;

	path_buffer[0] = '\0';			// 'Clear' buffer
	qstrncpy(fullpathcpy, fullpath, 512);	// Make copy of fullpath

  char *saveptr = NULL;
	token = qstrtok(fullpathcpy, "/", &saveptr);
	while(token != NULL)
	{
		
		if( hasCondition == true )
		{
			// If condition is fulfilled, we can start filling up the path_buffer.
			qstrncat(path_buffer, "/", path_buffer_size);
			qstrncat(path_buffer, token, path_buffer_size);
		}
		else if( strncmp(token, "libmatcher", strlen(token)) == 0 )
		{
			token = qstrtok(NULL, "/", &saveptr);
			if( token != NULL && strncmp(token, "mnt", strlen(token)) == 0 )
			{
				hasCondition = true;
				token = qstrtok(NULL, "/", &saveptr);

				// Copy nodename
				qstrncpy(nodename, token, nodename_size);
			}
			else continue;
		}

		if( token != NULL )
			token = qstrtok(NULL, "/", &saveptr);
	}
}

/*!
Gets Fully Qualified Domain Name of current host.
*/
int GetFQDN( char *buf, size_t bufLen, log4cpp::Category &log )
{
	char hostname[1024] = {'\0'};
	gethostname(hostname, sizeof(hostname));

	struct addrinfo hints, *info, *p;
	int gai_result;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;	/* either IPV4 or IPV6 */
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_CANONNAME;

	if( (gai_result = getaddrinfo(hostname, "http", &hints, &info) ) != 0 )
	{
		log.fatal("%s Line %d: getaddrinfo: %s", __FILE__, __LINE__, gai_strerror(gai_result) );
		return -1;
	}

	for( p = info; p != NULL; p = p->ai_next )
	{
		qsnprintf(buf, bufLen, "%s", p->ai_canonname);
	}

	return 0;
}

// The plugin can be passed an integer argument from the plugins.cfg
// file. This can be useful when you want the one plug-in to do
// something different depending on the hot-key pressed or menu
// item selected.
void IDAP_run(int arg)
{
	msg("%s run()\n", IDAP_name);

	//
	// Setup log4cpp
	//
	log4cpp::FileAppender *appender = new log4cpp::FileAppender("FuncstatFileAppender", "log_funcstats");
	log4cpp::Layout *layout = new log4cpp::SimpleLayout();
	log4cpp::Category &log = log4cpp::Category::getInstance("FuncstatCategory");
	appender->setLayout(layout);
	log.setAppender(appender);
	log.setPriority(log4cpp::Priority::DEBUG);

	log.info("DEBUG Started funcstats");

	//
	// Create instruction table. The size of the instruction table
	// should be bounded by the binary's size.
	//

	// Get file size
	char filename[MAXSTR];
	get_input_file_path(filename, MAXSTR);
	struct stat st;
	stat(filename, &st);


	// Allocate memory for instruction table.
	// We use IDA's enumerated instruction values.
	unsigned short *instrTable;
	instrTable = (unsigned short*) malloc(st.st_size * sizeof(unsigned short));
	if( instrTable == NULL )
	{
		msg("Cannot allocate instruction table. Out of memory!\n");
		log.fatal("Cannot allocate instruction table. Out of memory!");
		return;
	}

	//
	// Gets plt segment
	//
	//segment_t *xtrnSeg = NULL;
	//for( int i = 0; i < get_segm_qty(); ++i )
	//{
	//	segment_t *seg = getnseg(i);
	//	if( seg->type == SEG_XTRN )
	//	{
	//		xtrnSeg = seg;
	//		break;
	//	}
	//}

	//
	// Setup MySQL connection
	//
	const char *host = "virtual2.eecs.umich.edu";
	const char *user = "libmatcher";
	const char *pwd = "l1bm@tcher";
  qstring db;
	qgetenv("MYSQLDB", &db);

	log.info("DEBUG MYSQLDB %s", db.c_str());

	/*
	if( GetFQDN(host, sizeof(host), log) != 0 )
	{
		log.fatal("%s Line %d: Can't get host name!");
		return;
	}

	if( strcmp( host, "virtual2.eecs.umich.edu" ) == 0 )
		qsnprintf( host, sizeof(host), "localhost" );
	else
		qsnprintf( host, sizeof(host), "virtual2.eecs.umich.edu" );
	*/

	MYSQL	*conn;
	conn = mysql_init(NULL);
	if( !mysql_real_connect( conn, host, user, pwd, db.c_str(), 0, 0, 0) )
	{
		log.fatal("%s Line %d: (%s, %s, %s, %s) %s",
			__FILE__, __LINE__,
			host, user, pwd, db.c_str(), mysql_error(conn));
		return;
	}

	// Get path and nodename
	char path_buffer[1024] = {'\0'};
	char nodename[32] = {'\0'};
	GetPath(filename, path_buffer, 1024, nodename, 32);

	// Get file_id
	char query_buffer[512];
	qsnprintf( query_buffer, sizeof(query_buffer),
		"SELECT file.file_id FROM client, file "\
		"WHERE client.client_name = \'%s\' "\
		"AND file.client_id = client.client_id "\
		"AND file.path = \'%s\'",
		nodename, path_buffer );
	mysql_query(conn, query_buffer);
	MYSQL_RES *result = mysql_store_result(conn);
	if( result == NULL )
	{
		log.fatal("%s Line %d: %s", __FILE__, __LINE__, mysql_error(conn));
		return;
	}
	MYSQL_ROW row = mysql_fetch_row(result);
	if( row == NULL )
	{
		log.fatal("Cannot find file_id for client \"%s\" and file path \"%s\"!",
			nodename, path_buffer);
		return;
	}
	unsigned int file_id = atoi(row[0]);
	mysql_free_result(result);
	log.info("file id %u", file_id);

	// Convenience vector for accessing all functions.
	vector< Function * > vecFunctions;

	//
	// Populate function vector.
	//
	func_t *pFunc;
	ea_t funcNextEA = 0;
	while( (pFunc = get_next_func(funcNextEA)) != NULL )
	{
		funcNextEA = pFunc->endEA - 1;
		Function *pFn = new Function( file_id, pFunc, &log );

		
		// Skip noise functions.
		if( IsBlacklistedSegFN(pFunc) == true ||
		IsBlacklistedFunc(pFn->GetName()) == true  )
		{
			delete pFn;
			continue;
		}

		log.info("Working on %s", pFn->GetName());

		// Insert ptr to Function into convenience vector
		vecFunctions.push_back( pFn );
	}

	log.info("DEBUG There are %d funcs to analyze", vecFunctions.size());
	AnalyzeFuncs( conn, vecFunctions, instrTable, st.st_size, log );

	free(instrTable);

	//
	// Release Functions
	//
	vector<Function *>::iterator bFn, eFn;
	for( bFn = vecFunctions.begin(), eFn = vecFunctions.end();
	bFn != eFn; ++bFn )
	{
		delete *bFn;
		*bFn = NULL;
	}
	vecFunctions.clear();
	
	//UpdIntrnCall( conn, mapIntrnCall, funcEA2Id, log );
	//UpdFnLvl( conn, mapIntrnCall, funcEA2Id, log );
	mysql_close(conn);

	log.info("DEBUG Exiting funcstats");

	return;
}


