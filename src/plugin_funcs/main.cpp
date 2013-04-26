#include <ida.hpp>
#include <idp.hpp>
#include <loader.hpp>
#include <xref.hpp>

char IDAP_comment[] = "Tests function chunks.";
char IDAP_help[] = "funcs";
#ifdef __X64__
char IDAP_name[] = "funcs64";
#else
char IDAP_name[] = "funcs32";
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

void idaapi printchunk( ea_t ea1, ea_t ea2, void *ud )
{
	func_t *pFn1 = get_func(ea1);
	msg("Chunk at %a - %a (%a - %a, %08x, %s)\n", ea1, ea2,
		pFn1->startEA, pFn1->endEA,
		pFn1->flags,
		pFn1->flags & FUNC_TAIL ? "tail" : "head" );
}

// The plugin can be passed an integer argument from the plugins.cfg
// file. This can be useful when you want the one plug-in to do
// something different depending on the hot-key pressed or menu
// item selected.
void IDAP_run(int arg)
{
	// Get the address of the cursor position
	ea_t addr = get_screen_ea();

	func_t *pFn2 = get_func(addr);
	msg("%a, %08x, %s\n", addr, pFn2->flags, pFn2->flags & FUNC_TAIL? "tail" : "head" );

	func_t *pFn = get_func(addr);
	iterate_func_chunks( pFn, printchunk, NULL, true );
}


