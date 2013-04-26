#ifndef __LMLOG_H__
#define __LMLOG_H__

//#include <fstream>
#include <stdio.h>
class LMLog
{
public:
	LMLog(const char *const logname)
	{
		logfile = fopen(logname, "w");
	};

	~LMLog()
	{
		if(logfile == NULL)
			return;

		fflush(logfile);
		fclose(logfile);
	};

	//int log();
	//char  *GetBuffer()     { return buffer; };
	//size_t GetBufferSize() { return sizeof(buffer); };

	WriteLog(char *str, ...);
private:
	//char buffer[1024];
	char logname[1024];
	//std::ofstream logfile;
	FILE *logfile;
};

#endif
