#include <idc.idc>

static main()
{
	auto ret, logFile, resFile, delLog, addr, end, args, locals, frame, firstArg, name, file_name, isDiff;
	
	// Apply Signature
	ret = ApplySig("libz");
	if(ret == 0)
	{
		Exit(-1);
	}
	
	// Wait for the end of autoanalysis
	Wait();
	
	isDiff = 1;
	addr = 0;
	for(addr = NextFunction(addr); addr != BADADDR; addr = NextFunction(addr))
	{
		name = Name(addr);
		if(name == "huft_build")
		{
			isDiff = 0;
		}
	}

	Exit(isDiff);
}
