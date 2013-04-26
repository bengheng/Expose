#include <idc.idc>

static main()
{
	Wait();
	RunPlugin("JmpPatch", 0);
	SaveBase(0, DBFL_BAK);
	Exit(0);
}
