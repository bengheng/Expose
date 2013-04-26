#include <log4cpp/Category.hh>
#include <log4cpp/FileAppender.hh>
#include <log4cpp/SimpleLayout.hh>
#include <ida.hpp>
#include <idp.hpp>
#include <auto.hpp>
#include <loader.hpp>
#include <funcs.hpp>
#include "jmppatch.h"
#include <segment.hpp>
#include <makefuncs.h>

char IDAP_comment[] = "Patches trampoline jmp functions.";
char IDAP_help[] = "JmpPatch";
#ifdef __X64__
char IDAP_name[] = "JmpPatch64";
#else
char IDAP_name[] = "JmpPatch32";
#endif
char IDAP_hotkey[] = "Alt-F6";

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

	// Setup log4cpp
	log4cpp::FileAppender *appender = new log4cpp::FileAppender("JmpPatchFileAppender", "log_jmppatch");
	log4cpp::Layout *layout = new log4cpp::SimpleLayout();
	log4cpp::Category &log = log4cpp::Category::getInstance("JmpPatchCategory");
	appender->setLayout(layout);
	log.setAppender(appender);
	log.setPriority(log4cpp::Priority::INFO);
	log.info("Started JmpPatch...");

	makefuncs();
	autoWait();
	JmpPatch(log);

	return;
}

