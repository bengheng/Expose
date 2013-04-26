/*!
lmlog.cpp

Logging facility for idaplugin.
*/
#include <fstream>
//#include <stdio.h>
#include <ida.hpp>
#include <idp.hpp>
#include "lmlog.h"

//LMLog::LMLog(const char *const lgname)
//{

	//qsnprintf(logname, sizeof(logname), "%s", lgname);
	//msg("Log filename will be %s\n", logname);
	
//}

//LMLog::~LMLog()
//{
//}

int LMLog::WriteLog(char *str, ...)
{
	if(logfile == NULL)
		return -1;

	//logfile.open(logname, std::ofstream::app);
	//logfile << buffer;
	//logfile.close();

	//msg("LOGFILE is %s\n", logname);
	//logfile = fopen(logname, "a");
	//fwrite(buffer, sizeof(char), sizeof(buffer), logfile);
	//fclose(logfile);

	fprintf(logfile, "%d : ", time(NULL));
	va_list arglist;
	va_start(arglist, str);
	vfprintf(logfile, str, arglist);
	va_end(arglist);
	fprintf(logfile, "\n");
	fflush(logfile);

	// Output to IDA window
	//msg(buffer);

	return 0;
}
