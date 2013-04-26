#include <ida.hpp>
#include <idp.hpp>
#include <loader.hpp>
#include <xref.hpp>
#include <function_analyzer.h>

char IDAP_comment[] = "Tests basic blocks.";
char IDAP_help[] = "bbs";
#ifdef __X64__
char IDAP_name[] = "bbs64";
#else
char IDAP_name[] = "bbs32";
#endif
char IDAP_hotkey[] = "Alt-F2";

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
	// Get the address of the cursor position
	ea_t addr = get_screen_ea();

	func_t *pfn = get_func(addr);
	function_analyzer f( pfn->startEA );
	f.run_analysis();

	msg( "There are %d nodes.\n", f.get_num_nodes() );

	for( int i = 1; i <= f.get_num_nodes(); ++i )
	{
		msg( "Node %d: %a\n", i, f.get_node(i) );
	}

	msg( "There are %d edges.\n", f.get_num_edges() );
	for( int i = 1; i <= f.get_num_edges(); ++i )
	{
		msg("Edge %d: src %a dst %a\n", i, f.get_edge_src(i), f.get_edge_dst(i) );
	}
}


