#include <sys/stat.h>
#include <dirent.h>
#include <iostream>
#include "Graph.h"
#include "LMException.hpp"
//#include "GraphMatcher.h"
#include <fcg.h>
#include <sys/time.h>
#include <log4cpp/Category.hh>
#include <log4cpp/FileAppender.hh>
#include <log4cpp/SimpleLayout.hh>
#include <log4cpp/PatternLayout.hh>
#include <pthread.h>
#include <signal.h>
#include <fstream>
#include <FileDist.h>
#include <function.h>
#include <sys/types.h>
#include <unistd.h>

#include <execinfo.h>
#include <signal.h>


using namespace std;

/*!
If file is a file (not a folder), then just add it to the list.
Otherwise enumerate the folder and add all files to list.
*/
int GetFiles(string file, list<string> &files, log4cpp::Category &log)
{
	struct stat fstat;

	if(stat(file.c_str(), &fstat) != 0)
		return 0;	// Error

	// Check if is a regular file
	if(S_ISREG(fstat.st_mode) == true)
		files.push_back(file);

	// Must be a directory
	if(S_ISDIR(fstat.st_mode) == false)
		return 0;

	DIR *dp;
	struct dirent *dirp;
	if((dp = opendir(file.c_str())) == NULL)
	{
		log.fatal( "Error %d opening %s!", errno, file.c_str() );
		return errno;
	}

	while ( (dirp = readdir(dp)) != NULL)
	{
		string name = dirp->d_name;
 		if( name.compare(".") == 0 || name.compare("..") == 0 )
			continue;

		files.push_back(name);
	}

	closedir(dp);
	return 0;
}



void PrintUsage( const char *appname )
{
	cout << "Usage 1: " << appname << " 1 <timeout_sec> <libdot file/dir> <bindot file/dir> <neighbordepth>" << endl;
	cout << "\tUsage 1 uses the old method for computing graph distances." << endl << endl;

	cout << "Usage 2: " << appname << " 2 <timeout_sec> <host> <user> <passwd> <db> <expt_id> <file1_id> <fifo_in> <fifo_out>" << endl;
	cout << "\tUsage 2 uses the new method. This Usage takes in only the library's information from its program argument.\n"\
		<< "\tIt then processes each file by reading from stdin, and stops if \"end\" is read.\n";

	cout << "Usage 3: " << appname << " 3 <timeout_sec> <host> <user> <passwd> <db> <expt_id> <file1_id> <file2_id>" << endl;
	cout << "\tUsage 3 is similar to Usage 2, but does not get file2_id via fifos.\n";

}

int GetUsage( int argc, char **argv )
{
	int usage;

	if( argc != 8 && argc != 11 && argc != 10 )
	{
		return -1;
	}

	usage = atoi( argv[1] );
	if( (argc == 8 && usage != 1) &&
	( argc == 11 && usage != 2) &&
	( argc == 10 && usage != 3) )
		return -1;

	return usage;
}


typedef struct
{
	pthread_cond_t		*cond;
	pthread_mutex_t		*mutex;
	MYSQL			*conn;
	unsigned int		expt_id;	
	FCGraph			*pFCG1;
	unsigned int		file2Id;
	RexTranslator		*rextrans;
	log4cpp::Category	*pLog;
	log4cpp::Category	*rexlog;
} THREADPARAM, *PTHREADPARAM;

void *Usage2ThreadFunc( void *param )
{
	//int oldstate;

	PTHREADPARAM pThdParam = (PTHREADPARAM) param;
	pthread_setcanceltype( PTHREAD_CANCEL_ASYNCHRONOUS, NULL );

	//pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &oldstate);
	pthread_mutex_lock(pThdParam->mutex);


	FileDist( pThdParam->conn,
		pThdParam->expt_id,
		pThdParam->pFCG1,
		pThdParam->file2Id,
		pThdParam->pLog,
		pThdParam->rextrans,
		pThdParam->rexlog );

	//pThdParam->log->info("Signalling condition.");
	pthread_cond_signal( pThdParam->cond );
	pthread_mutex_unlock(pThdParam->mutex);
	return NULL;
}
	
int Usage2(
	MYSQL *conn,
	ifstream &fifo_in,
	ofstream &fifo_out,
	const unsigned long timeout_sec,
	const unsigned int expt_id,
	const unsigned int file1_id,
	log4cpp::Category *const pLog )
{
	//
	// Setup log4cpp
	//
	char logname[32];
	snprintf(logname, sizeof(logname), "log_rex_%d", getpid());
	
	log4cpp::PatternLayout *layout = new log4cpp::PatternLayout();
	layout->setConversionPattern("%m");
	log4cpp::FileAppender *appender = new log4cpp::FileAppender("RexFileAppender", logname);
	appender->setLayout(layout);

	log4cpp::Category *rexlog = &log4cpp::Category::getInstance("RexCategory");
	rexlog->setAppender(appender);
	rexlog->setPriority(log4cpp::Priority::DEBUG);

	RexTranslator *rextrans = new RexTranslator( rexlog );
	rextrans->InitVex();


	// Initialize file1's function call graph
	FCGraph FCG1( file1_id, pLog);
	if( InitFCG( conn, file1_id, true, FCG1, pLog ) == -1 )
	{
		return -1;
	}

	// Continue until "end" is read
	fifo_out.write("ready\n", strlen("ready\n"));
	fifo_out.flush();

	char inbuf[256];
	while( fifo_in.getline(inbuf, sizeof(inbuf), '\n') && (strcmp(inbuf, "end") != 0))
	{
		unsigned int file2_id = atoi(inbuf);


		#ifdef VERBOSE
		cout << "=========================================================" << endl;
		cout << "expt_id " << expt_id << " file1_id " << file1_id << " file2_id " << file2_id << endl;
		cout << "=========================================================" << endl;
		fflush(stdout);
		#endif

		struct timeval	start, end;
		struct timespec	ts;
		long		elapsed_ms, seconds, useconds;
		pthread_t	threadid;
		//pthread_attr_t	threadattr;
		pthread_cond_t	cond = PTHREAD_COND_INITIALIZER;
		pthread_mutex_t	mutex = PTHREAD_MUTEX_INITIALIZER;
		THREADPARAM	thdParam;

		// Initialize thread params
		thdParam.cond		= &cond;
		thdParam.mutex		= &mutex;
		thdParam.conn		= conn;
		thdParam.expt_id	= expt_id;
		thdParam.pFCG1		= &FCG1;
		thdParam.file2Id	= file2_id;
		thdParam.rextrans	= rextrans;
		thdParam.pLog		= pLog;
		thdParam.rexlog		= rexlog;

		// Start Timer
		gettimeofday(&start, NULL);
	
		// Convert from timeval to timespec
		ts.tv_sec = start.tv_sec;		//tp.tv_sec;
		ts.tv_nsec = start.tv_usec * 1000;	//tp.tv_usec * 1000;
		ts.tv_sec += timeout_sec;		// Wait time
												
		// Create thread to do main work
		pthread_mutex_lock( &mutex );
		//pthread_attr_init(&threadattr);
		//pthread_attr_setdetachstate(&threadattr, PTHREAD_CREATE_JOINABLE);
		pthread_create( &threadid, /*&threadattr*/NULL, Usage2ThreadFunc, &thdParam );

		// Timed wait
		if( pthread_cond_timedwait( &cond, &mutex, &ts ) == ETIMEDOUT )
		{
			pLog->warn("File2 ID %u Timedout!", file2_id);
			pthread_cancel( threadid );
			//pthread_kill(threadid, 0);
		}

		pthread_mutex_unlock( &mutex );
		pthread_cond_destroy( &cond );
		pthread_mutex_destroy( &mutex );

		void *result;
		int status = pthread_join( threadid, &result );
		if( status != 0 )
			pLog->fatal("File2 ID %u ERROR joining thread!", file2_id);
		if( result == PTHREAD_CANCELED )
			pLog->warn( "File2 ID %u canceled!", file2_id);

		// Stop Timer
		gettimeofday(&end, NULL);
		seconds = end.tv_sec - start.tv_sec;
		useconds = end.tv_usec - start.tv_usec;
		elapsed_ms = ((seconds) * 1000 + (long) ceil(useconds/1000.0));


		fifo_out.write("ready\n", strlen("ready\n"));
		fifo_out.flush();
	}

	fifo_out.write("Terminate\n", strlen("Terminate\n"));
	fifo_out.flush();

	rexlog->shutdown();
	delete rextrans;

	// Release function vector for file1
	//FreeFuncVec( file1FuncVec );

	return 0;
}

// Usage 3 is almost the same as Usage 2, but doesn't
// use fifo to communicate with caller. Instead, it also
// requires file2_id, and runs only once.
int Usage3(
	MYSQL *conn,
	const unsigned long timeout_sec,
	const unsigned int expt_id,
	const unsigned int file1_id,
	const unsigned int file2_id,
	log4cpp::Category *const pLog )
{
	//
	// Setup log4cpp
	//
	char logname[32];
	snprintf(logname, sizeof(logname), "log_rex_%d", getpid());
	
	log4cpp::PatternLayout *layout = new log4cpp::PatternLayout();
	layout->setConversionPattern("%m");
	log4cpp::FileAppender *appender = new log4cpp::FileAppender("RexFileAppender", logname);
	appender->setLayout(layout);

	log4cpp::Category *rexlog = &log4cpp::Category::getInstance("RexCategory");
	rexlog->setAppender(appender);
	rexlog->setPriority(log4cpp::Priority::DEBUG);

	RexTranslator *rextrans = new RexTranslator( rexlog );
	rextrans->InitVex();

	// Initialize file1's function call graph
	FCGraph FCG1( file1_id, pLog);
	if( InitFCG( conn, file1_id, true, FCG1, pLog ) == -1 )
	{
		return -1;
	}

	#ifdef VERBOSE
	cout << "=========================================================" << endl;
	cout << "expt_id " << expt_id << " file1_id " << file1_id << " file2_id " << file2_id << endl;
	cout << "=========================================================" << endl;
	fflush(stdout);
	#endif

	struct timeval	start, end;
	struct timespec	ts;
	long		elapsed_ms, seconds, useconds;
	pthread_t	threadid;
	//pthread_attr_t	threadattr;
	pthread_cond_t	cond = PTHREAD_COND_INITIALIZER;
	pthread_mutex_t	mutex = PTHREAD_MUTEX_INITIALIZER;
	THREADPARAM	thdParam;

	// Initialize thread params
	thdParam.cond		= &cond;
	thdParam.mutex		= &mutex;
	thdParam.conn		= conn;
	thdParam.expt_id	= expt_id;
	thdParam.pFCG1		= &FCG1;
	thdParam.file2Id	= file2_id;
	thdParam.rextrans	= rextrans;
	thdParam.pLog		= pLog;
	thdParam.rexlog		= rexlog;

	// Start Timer
	gettimeofday(&start, NULL);
	
	// Convert from timeval to timespec
	ts.tv_sec = start.tv_sec;		//tp.tv_sec;
	ts.tv_nsec = start.tv_usec * 1000;	//tp.tv_usec * 1000;
	ts.tv_sec += timeout_sec;		// Wait time
												
	// Create thread to do main work
	pthread_mutex_lock( &mutex );
	//pthread_attr_init(&threadattr);
	//pthread_attr_setdetachstate(&threadattr, PTHREAD_CREATE_JOINABLE);
	pthread_create( &threadid, /*&threadattr*/NULL, Usage2ThreadFunc, &thdParam );

	// Timed wait
	if( pthread_cond_timedwait( &cond, &mutex, &ts ) == ETIMEDOUT )
	{
		pLog->warn("File2 ID %u Timedout!", file2_id);
		pthread_cancel( threadid );
		//pthread_kill(threadid, 0);
	}

	pthread_mutex_unlock( &mutex );
	pthread_cond_destroy( &cond );
	pthread_mutex_destroy( &mutex );

	void *result;
	int status = pthread_join( threadid, &result );
	if( status != 0 )
		pLog->fatal("File2 ID %u ERROR joining thread!", file2_id);
	if( result == PTHREAD_CANCELED )
		pLog->warn( "File2 ID %u canceled!", file2_id);

	// Stop Timer
	gettimeofday(&end, NULL);
	seconds = end.tv_sec - start.tv_sec;
	useconds = end.tv_usec - start.tv_usec;
	elapsed_ms = ((seconds) * 1000 + (long) ceil(useconds/1000.0));


	rexlog->shutdown();
	delete rextrans;

	// Release function vector for file1
	//FreeFuncVec( file1FuncVec );

	return 0;
}
#define STRINGIZE2(s) #s
#define STRINGIZE(s) STRINGIZE2(s)
#define TIMESTAMP_STRING STRINGIZE(TIMESTAMP)

void handler(int sig)
{
	void *array[10];
	size_t size;

	// get void*'s for all entries on the stack
	size = backtrace(array, 10);

	// print out all the frames to stderr
	fprintf(stderr, "Error: signal %d:\n", sig);
	backtrace_symbols_fd(array, size, 2);
	exit(1);
}


 main(int argc, char**argv)
{
	signal(SIGSEGV, handler);
	//
	// Setup log4cpp
	//
	char logname[32];
	snprintf(logname, sizeof(logname), "log_GraphMatcher_%d", getpid());
	log4cpp::FileAppender *appender = new log4cpp::FileAppender("GraphMatcherFileAppender", logname);
	log4cpp::Layout *layout = new log4cpp::SimpleLayout();
	log4cpp::Category &log = log4cpp::Category::getInstance("GraphMatcherCategory");
	appender->setLayout(layout);
	log.setAppender(appender);
	log.setPriority(log4cpp::Priority::DEBUG);

	char args[256];
	args[0] = 0x0;
	for( int i = 0; i < argc; ++i )
	{
		char tmp[256];
		snprintf( tmp, sizeof(tmp), "%s ", argv[i] );
		strncat( args, tmp, sizeof(args) );
	}
	log.info("%s", args);

	#ifdef TIMESTAMP
	log.info( "Compiled on %s", TIMESTAMP_STRING );
	#endif

	int usage =GetUsage( argc, argv );

	if( usage == -1 )
	{
		log.fatal("ERROR: %d input arguments!", argc);
		PrintUsage( argv[0] );
		return -1;
	}


	try
	{
		MYSQL *conn = NULL;
	
		if( usage == 2 || usage == 3 )
		{
			// Connects to MySQL database
			if( (conn = mysql_init(NULL)) == NULL )
			{
				PRNTSQLERR( (&log), conn );
				return -1;
			}

			//log.info("host: %s\nuser: %s\npasswd: %s\n db: %s",
			//	argv[5], argv[6], argv[7], argv[8] );
			if( (conn = mysql_real_connect( conn, argv[3], argv[4], argv[5], argv[6], 0, 0, 0) ) == NULL )
			{
				PRNTSQLERR( (&log), conn );
				mysql_close( conn );
				return -1;
			}
		}


		switch(usage)
		{
			case 1:
				log.fatal("Usage 1 is no longer supported!");
				break;

			case 2:
			{
				// Opens fifo endpoints
				ifstream fifo_in;
				fifo_in.open(argv[9]);

				ofstream fifo_out;
				fifo_out.open(argv[10]);


				Usage2( conn,
					fifo_in, fifo_out,
					atol(argv[2]),	// timeout_sec
					atoi(argv[7]),	// expt_id
					atoi(argv[8]),	// file1_id
					&log );
	
				// Close fifo endpoints
				fifo_in.close();
				fifo_out.close();

				break;
			}
			case 3:
				Usage3( conn,
					atol(argv[2]),	// timeout_sec
					atoi(argv[7]),	// expt_id
					atoi(argv[8]),	// file1_id
					atoi(argv[9]),	// file2_id
					&log );

			default:
				log.fatal( "Invalid usage!" );
		}

		// Close connection to MySQL database
		if( conn != NULL )
		{
			mysql_close( conn );
			conn = NULL;
		}
	}
	catch(LMException e)
	{
		std::cerr << e.getMessage() << std::endl;
	}


}
