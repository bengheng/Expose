#include <idc.idc>

static main()
{
	auto filename, outfile;

	Wait();
	filename = GetInputFile();
	outfile = "/home/bengheng/libmatcher/gdl/"+filename;
	GenCallGdl(outfile, filename, CHART_GEN_GDL);
	Exit(0);
}
