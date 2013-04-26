#include <idc.idc>

static main()
{
	auto ret, logFile, resFile, delLog, addr, name, file_name, isDiff;
	
	logFile = fopen("/home/libmatcher/libmatcher/logs/reveal.log", "a+");
	// resFile = fopen("/home/libmatcher/matchresult", "w");
	delLog = fopen("/home/libmatcher/libmatcher/logs/del.log", "a+");
	
	// Apply Signature
	file_name = GetIdbPath();
	fprintf(delLog, file_name+'\n');
	fprintf(logFile, "\tApplying signature... ");
	ret = ApplySig("libtiff_382");
	if(ret == 0)
	{
		fprintf(logFile, "ERROR!\n");
		fclose(logFile);
		Exit(-1);
	}
	
	fprintf(logFile, "OK\n");
	
	// Wait for the end of autoanalysis
	Wait();
	
	isDiff = 1;
	addr = 0;
	for(addr = NextFunction(addr); addr != BADADDR; addr = NextFunction(addr))
	{
		name = Name(addr);
		if(name == "LZWDecodeCompat")
		{
			fprintf(logFile, "\t\tFound %s\n", name);
			isDiff = 0;
		}
	}
	// fprintf(resFile, "%d", isDiff);

	fclose(delLog);
	// fclose(resFile);
	fclose(logFile);
	Exit(isDiff);
}
