#include "graphgen.h"
#include <sstream>
#include <iostream>

using namespace std;

int main(int argc, char **argv)
{
	int numrounds;
	string dotfname;


	if(argc != 2)
	{
		cout << "Usage: " << argv[0] << " <#nodes>" << endl;
		return 0;
	}

	
	numrounds = atoi(argv[1]);
	if(numrounds == 0) return 0;

	cout << "Generating " << numrounds << " dot files...\n";
	GraphGen g(GG_ALLOW_DUP_VERTEX);
	srand( time(0) );
	for(int i = 0; i < numrounds; ++i)
	{
		g.AddRandomVertex();
		if(i > 0) g.AddRandomEdge();

		ostringstream idx;
		idx << i;
		dotfname = "./dot/test."+idx.str()+".dot";
		g.WriteDOT(dotfname);
	}
	

	return 0;
}
