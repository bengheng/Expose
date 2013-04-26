#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/properties.hpp>
#include <boost/graph/graphviz.hpp>
#include "Graph.h"
#include "LMException.hpp"

using namespace boost;
using namespace std;

/*!
Constructor.
\param	dotName	Name of Dot file from which to read graph.
*/
LMGraph::LMGraph(const string dotName) throw (LMException)
{

	//pGraphNamePropMap	= new ref_property_map<LMGraph_t*, string>(get_property(*this, graph_name));
	vertexNamePropMap	= get(vertex_name, *this);
	vertexIdxPropMap	= get(vertex_index, *this);
	edgeWeightPropMap	= get(edge_weight, *this);
	edgeIdxPropMap		= get(edge_index, *this);

	// Read from Dot file
	if(ReadDot(dotName) == false) throw LMException("FATAL: Cannot read Dot file!");
}

/*!
Destructor.
*/
LMGraph::~LMGraph() throw()
{
	//delete pGraphNamePropMap;
}

bool LMGraph::ReadDot(const string infile)
{
	ifstream ifs(infile.c_str());
	//dp.property("name", /**pGraphNamePropMap*/ graphName);
	dp.property("label", vertexNamePropMap);
	dp.property("label", edgeWeightPropMap);

	return read_graphviz(ifs, *this, dp, "label");
}

