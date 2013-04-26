#include <idc.idc>

static main()
{
	auto ret, logFile, resFile, delLog, addr, end, args, locals, frame, firstArg, name, file_name, isDiff;
	
	// logFile = fopen("/home/libmatcher/reveal.log", "a+");
	// resFile = fopen("/home/libmatcher/matchresult", "w");
	// delLog = fopen("/home/libmatcher/del.log", "a+");
	
	// Apply Signature
	//file_name = GetIdbPath();
	//fprintf(delLog, file_name+'\n');
	//fprintf(logFile, "\tApplying signature... ");
	ret = ApplySig("libpurple_258");
	if(ret == 0)
	{
		//fprintf(logFile, "ERROR!\n");
		//fclose(logFile);
		Exit(-1);
	}
	
	//fprintf(logFile, "OK\n");
	
	// Wait for the end of autoanalysis
	Wait();
	
	isDiff = 1;
	addr = 0;
	for(addr = NextFunction(addr); addr != BADADDR; addr = NextFunction(addr))
	{
		name = Name(addr);
		if(name == "msn_slplink_process_msg")
		{
			fprintf(logFile, "\t\tFound %s\n", name);
			isDiff = 0;
		}
	}
	// fprintf(resFile, "%d", isDiff);

	// fclose(delLog);
	// fclose(resFile);
	// fclose(logFile);
	Exit(isDiff);
}
