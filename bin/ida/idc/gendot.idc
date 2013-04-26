#include <idc.idc>

static main()
{
	Wait();
	RunPlugin("JmpPatch", 0);
	RunPlugin("makedot", 0);
	Exit(0);
}
