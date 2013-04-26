#include <ida.hpp>
#include <idp.hpp>
#include <loader.hpp>
#include <kernwin.hpp>
#include <ua.hpp>

char IDAP_comment[] = "Get instruction info.";
char IDAP_help[] = "instrinfo";
#ifdef __X64__
char IDAP_name[] = "instrinfo64";
#else
char IDAP_name[] = "instrinfo32";
#endif
char IDAP_hotkey[] = "Alt-F5";

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

	// Disassemble the instruction at the cursor position, store it in
	// the global accessible "cmd" structure.
	// ua_out(get_screen_ea(), false);
	decode_insn( get_screen_ea() );

	// Display information about the first operand
	msg("itype = %u size = %zu\n", cmd.itype, cmd.size );
	for( unsigned int n = 0; n < UA_MAXOP ; ++n )
	{
		if( cmd.Operands[n].type == o_void ) break;
		msg("Operand %u n = %d type = %d reg = %d value = %a addr = %a\n",
			n,
			cmd.Operands[n].n,
			cmd.Operands[n].type,
			cmd.Operands[n].reg,
			cmd.Operands[n].value,
			cmd.Operands[n].addr);
	}

	char calleeName[MAXSTR];
	get_func_name(cmd.Op1.addr, calleeName, sizeof(calleeName));
	msg("Callee Name: \"%s\"", calleeName);

	return;
}

