#include <log4cpp/Category.hh>
#include <log4cpp/FileAppender.hh>
#include <log4cpp/SimpleLayout.hh>

#include "lmobject.h"
#include <ida.hpp>
#include <idp.hpp>
#include <loader.hpp>
#include <funcs.hpp>
#include "blacklist.h"
log4cpp::Appender *appender;


char IDAP_comment[] = "Extracts function call graph to dot file.";
char IDAP_help[] = "makedot";
#ifdef __X64__
char IDAP_name[] = "makedot64";
#else
char IDAP_name[] = "makedot32";
#endif
char IDAP_hotkey[] = "Alt-F7";

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

/*
bool NextIsCallToLibcStartMain(ea_t currEA)
{
	xrefblk_t xr;
	ea_t nextEA;

	xr.first_from(currEA, XREF_ALL);
	nextEA = xr.to;

	xrefblk_t xr2;
	for( bool ok = xr2.first_from(nextEA, XREF_ALL); ok; ok = xr2.next_from() )
	{
		func_t *func = get_func(xr2.to);
		if(func == NULL) continue;

		char fname[1024];
		get_func_name(func->startEA, fname, sizeof(fname));
		#ifdef VERBOSE
		info("%x %s\n", func->startEA, fname);
		#endif
		if( strcmp(fname, ".__libc_start_main") == 0 || strcmp(fname, "__libc_start_main") == 0 )
		{
			#ifdef VERBOSE
			info("next is libc_start_main\n");
			#endif
			return true;
		}
	}

	#ifdef VERBOSE
	info("Not libc_start_main at %a\n", nextEA);
	#endif
	return false;
}
*/

bool HasStartType1Sig(ea_t startEA)
{
	unsigned char startSig[] = { 0x31, 0xed, 0x5e, 0x89, 0xe1, 0x83, 0xe4, 0xf0, 0x50, 0x54, 0x52, 0x68,\
		0x0, 0x0, 0x0, 0x0, 0x68, 0x0, 0x0, 0x0, 0x0, 0x51, 0x56, 0x68, 0x0, 0x0, 0x0, 0x0, \
		0xe8, 0x0, 0x0, 0x0, 0x0, 0xf4  };

	int i;
	unsigned char val;

	// Check first 12 bytes
	for(i = 0; i < 12; ++i)
	{
		val = get_byte(startEA + i);
		if(startSig[i] != val )
		{
			#ifdef VERBOSE
			info("%a Byte %d %02x != %02x\n", startEA, i, val, startSig[i]);
			#endif
			return false;
		}
	}

	if( startSig[16] != get_byte(startEA + 16) )
	{
		#ifdef VERBOSE
		info("%a Byte %d %02x != %02x\n", startEA, 16, get_byte(startEA+16), startSig[16]);
		#endif
		return false;
	}
	
	for(i = 21; i < 24; ++i)
	{
		val = get_byte(startEA + i);
		if(startSig[i] != val)
		{
			#ifdef VERBOSE
			info("%a Byte %d %02x != %02x\n", startEA, i, val, startSig[i]);
			#endif
			return false;
		}
	}

	if( startSig[28] != get_byte(startEA + 28) )
	{
		#ifdef VERBOSE
		info("%a Byte %d %02x != %02x\n", startEA, 28, get_byte(startEA+33), startSig[28]);
		#endif
		return false;
	}

	if( startSig[33] != get_byte(startEA + 33) )
	{
		#ifdef VERBOSE
		info("%a Byte %d %02x != %02x\n", startEA, 33, get_byte(startEA+33), startSig[33]);
		#endif
		return false;
	}


	#ifdef VERBOSE
	info("%a has start Type 1 signature.", startEA);
	#endif

	return true;
}

bool HasStartType2Sig(ea_t startEA)
{
	function_analyzer f(startEA);
	f.run_analysis();

	int numcalls = f.get_num_calls();
	for(int i = 1; i <= numcalls; ++i)
	{
		ea_t dstf = f.get_call_dst(i);

		char fname[1024];
		get_func_name(dstf, fname, sizeof(fname));
		if( strcmp(fname, ".init_proc") == 0 ) return true;
	}

	return false;
}

/*!
If startEA is a function, add to blacklist.

If it is NOT a BP based frame, return. Otherwise, for all data/function references,
call AddToBlacklist.
*/
void AddToBlacklist(ea_t startEA, set<ea_t> &blacklist, bool isstart, bool stop, int timeout)
{
	func_t *func;

	if(startEA == BADADDR) return;

	// If already in blacklist, return.
	if( blacklist.find(startEA) != blacklist.end() )
		return;

	// If not function, return.
	func = get_func(startEA);
	if(func == NULL) return;

	char fname[1024];
	get_func_name(func->startEA, fname, sizeof(fname));
	#ifdef VERBOSE
	//msg("Blacklisting %a %s\n", startEA, fname);
	#endif

	// Add to blacklist.
	blacklist.insert(startEA);

	// If this is main, return.
	if(stop || timeout <= 0)
	{
		#ifdef VERBOSE
		info("Stopping.\n");
		#endif
		return;
	}

	// If not BP based frame, return.
	if( ((func->flags & FUNC_NORET) == 0) &&
	((func->flags & FUNC_FRAME) == 0) )
	{
		#ifdef VERBOSE
		msg("Returning...\n");
		#endif
		return;
	}


	bool IsStartType1 = HasStartType1Sig(func->startEA);
	bool IsStartType2 = HasStartType2Sig(func->startEA);
	#ifdef VERBOSE
	if ( isstart && IsStartType1 == false && IsStartType2 == false )
		info("Cannot identify start type.\n");
	#endif

	// For all data/function references,
	// add to blacklist.

	xrefblk_t xr;
	ea_t instrEA = func->startEA;
	bool ok;
	bool PrevIsCallToInitProc = false;
	list<ea_t> processedEA;
	while( find(processedEA.begin(), processedEA.end(), instrEA) == processedEA.end() &&	// Make sure we're not looping in instructions
	(ok = xr.first_from(instrEA, XREF_ALL) == true))
	{
		processedEA.push_back(instrEA);

		func_t *curFunc = get_func(instrEA);
		if(curFunc == NULL || curFunc->startEA != func->startEA) break;

		xrefblk_t xr2;
		for( ok = xr2.first_from(instrEA, XREF_FAR); ok; ok = xr2.next_from() )
		{
			func_t *xfunc = get_func(xr2.to);

			if( xfunc != NULL && xfunc->startEA != func->startEA )
			{
				char fname2[1024];
				get_func_name(xfunc->startEA, fname2, sizeof(fname2));
				
				#ifdef VERBOSE
				//info("%a:%a Blacklisting %s (%a)\n", func->startEA, instrEA, fname2, xfunc->startEA);
				#endif

					
				// 3 cases that we want to stop at next function, to prevent recursive runaway...
				// 1. This function is start with signature 1
				// 2. This function is start with signature 2
				// 3. This function is start but has neither signature 1 nor 2
				ea_t offsetEA = instrEA - func->startEA;
				if( (IsStartType1 == true && (offsetEA == 0x17 || offsetEA == 0x1c)) ||
				( isstart == true && PrevIsCallToInitProc == true ) ||
				( isstart && IsStartType1 == false && IsStartType2 == false ))
				{
					
					#ifdef VERBOSE
					//info("%a:%a Blacklisting %a as main\n", func->startEA, instrEA, xfunc->startEA);
					#endif

					AddToBlacklist(xfunc->startEA, blacklist, false, true, timeout - 1);
					PrevIsCallToInitProc = false;
				}
				else
					AddToBlacklist(xfunc->startEA, blacklist, false, false, timeout - 1);
		
				if( strcmp(fname2, ".init_proc") == 0 )
					PrevIsCallToInitProc = true;
			}

		}

		#ifdef VERBOSE
		// msg("next addr: %a\n", xr.to);
		#endif

		instrEA = xr.to;
	}

}

// The plugin can be passed an integer argument from the plugins.cfg
// file. This can be useful when you want the one plug-in to do
// something different depending on the hot-key pressed or menu
// item selected.
void IDAP_run(int arg)
{
	// The "meat" of your plug-in
	msg("%s run()\n", IDAP_name);

	appender = new log4cpp::FileAppender("FileAppender", "log_makedot");
	log4cpp::Layout * layout = new log4cpp::SimpleLayout();
	log4cpp::Category& log = log4cpp::Category::getInstance("Category");
	msg("Created appender\n");

	appender->setLayout(layout);
	log.setAppender(appender);
	log.setPriority(log4cpp::Priority::INFO);

	log.info("%s started...", IDAP_name);

	LMObject *lmObj = new LMObject(log);

	set<ea_t> blacklist;

	char fname[1024];
	
	// Create blacklist.
	// All functions in XTRN segment, or is referenced
	// from start are blacklisted, except those starting from
	// main, where RETURN flag is set and is a BP frame.
	for (int f = 0; f < get_func_qty(); f++)
	{
		func_t *curFunc = getn_func(f);
		segment_t *seg = getseg(curFunc->startEA);

		// Don't want XTRN
		//if(seg->type == SEG_XTRN)
		//{
		//	#ifdef VERBOSE
		//	//msg("Blacklisting %a\n", curFunc->startEA);
		//	#endif
		//	blacklist.insert(curFunc->startEA);
		//}

		char segname[1024];
		get_segm_name(seg->startEA, segname, sizeof(segname));
		if(strcmp(segname, "_init") == 0 || strcmp(segname, "_fini") == 0)
		{
			AddToBlacklist(curFunc->startEA, blacklist, false, false, 3);
		}

		get_func_name(curFunc->startEA, fname, sizeof(fname));
		if(strcmp(fname, "start") == 0 ||
		strcmp(fname, "_start") == 0)
		{
			AddToBlacklist(curFunc->startEA, blacklist, true, false, 3);
		}
	
	}

	// Add function if not blacklisted.
	for (int f = 0; f < get_func_qty(); f++)
	{
		func_t *curFunc = getn_func(f);
		get_func_name(curFunc->startEA, fname, sizeof(fname));

		if(blacklist.find(curFunc->startEA) == blacklist.end() &&
		IsBlacklistedFunc(fname) == false )
		{
			lmObj->AddFunction(curFunc);
		}
	}
	


	#ifdef FA_DEBUG
	lmObj->PrintFuncCalls();
	#endif
	lmObj->ComputeAdjacencyMatrix();
	lmObj->WriteFCG();

	delete lmObj;
	
	return;
}


