#include <ida.hpp>
#include <idp.hpp>
#include <loader.hpp>
#include <xref.hpp>

char IDAP_comment[] = "Lists cross-references at cursor.";
char IDAP_help[] = "xref";
#ifdef __X64__
char IDAP_name[] = "xref64";
#else
char IDAP_name[] = "xref32";
#endif
char IDAP_hotkey[] = "Alt-F8";

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
	// The "meat" of your plug-in
	msg("%s run()\n", IDAP_name);

	xrefblk_t xb;

	// Get the address of the cursor position
	ea_t addr = get_screen_ea();

	// Loop through all cross references
	for(bool res = xb.first_to(addr, XREF_FAR); res; res = xb.next_to())
	{
		msg("[1] From: %a, To: %a, Type: %d, IsCode: %d\n", xb.from, xb.to, xb.type, xb.iscode);
	}

	for(bool res = xb.first_from(addr, XREF_FAR); res; res = xb.next_from())
	{
		msg("[2] From: %a, To: %a, Type: %d, IsCode: %d\n", xb.from, xb.to, xb.type, xb.iscode);
	}
}


