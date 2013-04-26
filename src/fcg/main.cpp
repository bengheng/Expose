#include "fcg.h"
#include <stdio.h>
#include <string>
#include "../utils/utils.h"

using namespace boost::detail::graph;

int main(int argc, char** argv)
{
	if(argc != 2)
	{
		printf("Usage: %s <dotfile>\n", argv[0]);
	}

	FCGraph fcg;
	std::string infilename = argv[1];
	fcg.ReadDOT(infilename);
	// fcg.PrintVertices();
	// fcg.PrintEdges();


	// gsl_matrix *m = gsl_matrix_calloc(fcg.GetNumFuncs(), fcg.GetNumFuncs());
	// fcg.UpdateCostMatrix(m, FCG_USENAME|FCG_USEEDGE);
	// PrintGSLMatrix(m, "Cost Matrix");	
	// gsl_matrix_free(m);
}
