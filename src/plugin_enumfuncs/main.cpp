#include <ida.hpp>
#include <idp.hpp>
#include <loader.hpp>
#include <funcs.hpp>
#include <frame.hpp>
#include <struct.hpp>
#include <time.h>
#include <function_analyzer.h>

using namespace std;

char IDAP_comment[] = "Enumerate functions.";
char IDAP_help[] = "enumfuncs";
#ifdef __X64__
char IDAP_name[] = "enumfuncs64";
#else
char IDAP_name[] = "enumfuncs32";
#endif
char IDAP_hotkey[] = "Alt-F1";

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

// The plugin can be passed an integer argument from the plugins.cfg
// file. This can be useful when you want the one plug-in to do
// something different depending on the hot-key pressed or menu
// item selected.
void IDAP_run(int arg)
{
	msg("%s run()\n", IDAP_name);
	
	func_t *func;
	ea_t funcNextEA = 0;
	while( (func = get_next_func(funcNextEA)) != NULL )
	{
		funcNextEA = func->endEA - 1;

		// Analyze function
		function_analyzer *pFunc = new function_analyzer(func->startEA);
		pFunc->run_analysis();

		msg("%a - %a %s %d %d\n",
			pFunc->get_ea_start(), pFunc->get_ea_end(), pFunc->get_name(),
			func->frsize, func->frregs);

		struc_t *frame = get_frame(func);
		if( frame )
		{
			size_t ret_addr = func->frsize + func->frregs;	// offset to return address
			for( size_t m = 0; m < frame->memqty; ++m )	// loop through members
			{
				char fname[1024];
				get_member_name(frame->members[m].id, fname, sizeof(fname));
				if( frame->members[m].soff < func->frsize )
				{
					msg("  Local variable ");
				}
				else if( frame->members[m].soff > ret_addr )
				{
					msg("  Parameter ");
				}
				msg("%s is at frame offset %x\n", fname, frame->members[m].soff);
				if( frame->members[m].soff == ret_addr )
				{
					msg("  %s is the saved return address\n", fname);
				}
			}
		}
	}

	return;
}


