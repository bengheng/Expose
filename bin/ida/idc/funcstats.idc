#include <idc.idc>

static main()
{
	Wait();
	RunPlugin("JmpPatch", 0);
	RunPlugin("funcstats", 0);
	Exit(0);
}
