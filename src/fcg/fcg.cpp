#include "fcg.h"
#include <ostream>
#include <iomanip>
#include <utility>
#include <boost/graph/graphviz.hpp>
#include <gsl/gsl_matrix.h>
#include "../utils/utils.h"
#ifdef __IDP__ 
#include <ida.hpp>
#include <idp.hpp>

#define	PRINT msg
#else
#include <stdio.h>
#define PRINT printf
#endif

using namespace std;
using namespace boost;

class FCEdgeLabelWriter
{
public:
	FCEdgeLabelWriter(FCEdgeWeightPropMap _edgeWeightPropMap,
		FCEdgeLoopPropMap _edgeLoopPropMap) :
		edgeWeightPropMap(_edgeWeightPropMap),
		edgeLoopPropMap(_edgeLoopPropMap) {}

	void operator()(ostream &out, const FCEdge e) const
	{
		out << "[weight=\"" << get(edgeWeightPropMap, e) << "\"][loop=\"" << get(edgeLoopPropMap, e) << "\"]";
	}
private:
	FCEdgeWeightPropMap	edgeWeightPropMap;
	FCEdgeLoopPropMap	edgeLoopPropMap;
};

class FCVertexLabelWriter
{
public:
	FCVertexLabelWriter(FCVertexNamePropMap _vertexNamePropMap,
		FCVertexStartPropMap _vertexStartPropMap,
		FCVertexEndPropMap _vertexEndPropMap,
		FCVertexNodesPropMap _vertexNodesPropMap,
		FCVertexEdgesPropMap _vertexEdgesPropMap,
		FCVertexCallsPropMap _vertexCallsPropMap,
		FCVertexInstrsPropMap _vertexInstrsPropMap) :
		vertexNamePropMap(_vertexNamePropMap),
		vertexStartPropMap(_vertexStartPropMap),
		vertexEndPropMap(_vertexEndPropMap),
		vertexNodesPropMap(_vertexNodesPropMap),
		vertexEdgesPropMap(_vertexEdgesPropMap),
		vertexCallsPropMap(_vertexCallsPropMap),
		vertexInstrsPropMap(_vertexInstrsPropMap)
		{}

	void operator()(ostream &out, const FCVertex v) const
	{
		out << "[vertex=\"" << get(vertexNamePropMap, v) << "\"]"
			<< "[start=\"0x" << hex << setw(8) << setfill('0') << get(vertexStartPropMap, v) << "\"]"
			<< "[end=\"0x" << hex <<  setw(8) << setfill('0') << get(vertexEndPropMap, v) <<  "\"]"
			<< "[nodes=\"" << dec << get(vertexNodesPropMap, v) << "\"]"
			<< "[edges=\"" << get(vertexEdgesPropMap, v) << "\"]"
			<< "[calls=\"" << get(vertexCallsPropMap, v) << "\"]"
			<< "[instrs=\"" << get(vertexInstrsPropMap, v) << "\"]";
	}
private:
	FCVertexNamePropMap	vertexNamePropMap;
	FCVertexStartPropMap	vertexStartPropMap;
	FCVertexEndPropMap	vertexEndPropMap;
	FCVertexNodesPropMap	vertexNodesPropMap;
	FCVertexEdgesPropMap	vertexEdgesPropMap;
	FCVertexCallsPropMap	vertexCallsPropMap;
	FCVertexInstrsPropMap	vertexInstrsPropMap;
};

FCGraph::FCGraph()
{
	//pGraphNamePropMap	= new ref_property_map<FCGraph_t*, string>(get_property(*this, graph_name));
	//if(pGraphNamePropMap == NULL) return;

	vertexNamePropMap	= get(vertex_name, *this);
	vertexStartPropMap	= get(vertex_start, *this);	
	vertexEndPropMap	= get(vertex_end, *this);	
	vertexNodesPropMap	= get(vertex_nodes, *this);	
	vertexEdgesPropMap	= get(vertex_edges, *this);	
	vertexCallsPropMap	= get(vertex_calls, *this);	
	vertexInstrsPropMap	= get(vertex_instrs, *this);	
	vertexIdxPropMap	= get(vertex_index, *this);
	edgeWeightPropMap	= get(edge_weight, *this);
	edgeLoopPropMap		= get(edge_loop, *this);
	edgeIdxPropMap		= get(edge_index, *this);

	dp.property("vertex", get(vertex_name, *this));
	dp.property("start", get(vertex_start, *this));
	dp.property("end", get(vertex_end, *this));
	dp.property("nodes", get(vertex_nodes, *this));
	dp.property("edges", get(vertex_edges, *this));
	dp.property("calls", get(vertex_calls, *this));
	dp.property("instrs", get(vertex_instrs, *this));
	dp.property("weight", get(edge_weight, *this));
	dp.property("loop", get(edge_loop, *this));
}

FCGraph::~FCGraph()
{
	//if (pGraphNamePropMap != NULL)
	//	delete pGraphNamePropMap;
}

/*!
Finds the vertex with the specified vname.

Returns FCG_NULL_VERTEX if it doesn't exist.
*/
FCVertex FCGraph::FindVertex(std::string vname)
{
	if(num_vertices(*this) == 0)
		return FCG_NULL_VERTEX;

	std::pair<FCVertexItr, FCVertexItr> vp;
	for(vp = vertices(*this); vp.first != vp.second; ++vp.first)
	{
		if( vname.compare(vertexNamePropMap[*vp.first]) == 0 )
			return *vp.first;	// Found
	}

	// Cannot find vertex
	return FCG_NULL_VERTEX;
}

/*!
Adds vertex with specified name if it doesn't exist.
*/
int FCGraph::AddVertex(
	std::string vname,
	unsigned int start,
	unsigned int end,
	unsigned int num_nodes,
	unsigned int num_edges,
	unsigned int num_calls,
	unsigned int num_instrs)
{
	// Checks if vertex with vname already exists
	if(FindVertex(vname) != FCG_NULL_VERTEX)
		return 0;
	
	FCVertex v = add_vertex(*this);
	vertexNamePropMap[v] = vname;
	vertexStartPropMap[v] = start;
	vertexEndPropMap[v] = end;
	vertexNodesPropMap[v] = num_nodes;
	vertexEdgesPropMap[v] = num_edges;
	vertexCallsPropMap[v] = num_calls;
	vertexInstrsPropMap[v] = num_instrs;

	return 0;
}

/*!
Adds an edge if it doesn't already exist. If it already exists,
add the weight.
*/
int FCGraph::AddEdgeWeight(std::string uname, std::string vname, int weight)
{
	FCVertex u, v;
	FCEdge e;
	bool res;

	// Get vertices u and v
	if((u = FindVertex(uname)) == FCG_NULL_VERTEX) return -1;
	if((v = FindVertex(vname)) == FCG_NULL_VERTEX) return -1;

	tie(e, res) = edge(u, v, *this);
	if(res == false)
	{
		tie(e, res) = add_edge(u, v, *this);
		if(res == true) edgeWeightPropMap[e] = weight;
		return 0;
	}

	// Edge already exists. Add weight.
	edgeWeightPropMap[e] += weight;
	return 0;
}

/*!
Adds an edge if it doesn't already exist. If it already exists,
add the loop.
*/
int FCGraph::AddEdgeLoop(std::string uname, std::string vname, int loop)
{
	FCVertex u, v;
	FCEdge e;
	bool res;

	// Get vertices u and v
	if((u = FindVertex(uname)) == FCG_NULL_VERTEX) return -1;
	if((v = FindVertex(vname)) == FCG_NULL_VERTEX) return -1;

	tie(e, res) = edge(u, v, *this);
	if(res == false)
	{
		tie(e, res) = add_edge(u, v, *this);
		if(res == true) edgeLoopPropMap[e] = loop;
		return 0;
	}

	// Edge already exists. Add weight.
	edgeLoopPropMap[e] += loop;
	return 0;
}

/*!
Writes the graph to the specified DOT file.
The file will be overwritten if it already exists.
*/
void FCGraph::WriteDOT(const string &outfile)
{
	ofstream ofs(outfile.c_str(), ios::app);

	PRINT("Writing %zu vertices %lu edges to \"%s\"...\n", num_vertices(*this), num_edges(*this), outfile.c_str());
	//write_graphviz(ofs, *this, dp, string("id"));
	write_graphviz(ofs, *this, //make_label_writer(vertexNamePropMap),
		FCVertexLabelWriter(vertexNamePropMap, vertexStartPropMap, vertexEndPropMap, vertexNodesPropMap, vertexEdgesPropMap, vertexCallsPropMap, vertexInstrsPropMap),
		//FC_label_writer<FCEdgeWeightPropMap, FCEdgeLoopPropMap>(edgeWeightPropMap, edgeLoopPropMap));
		FCEdgeLabelWriter(edgeWeightPropMap, edgeLoopPropMap));
	ofs.flush();
	ofs.close();
}

/*!
Reads Function Call Graph from specified DOT file.
*/
bool FCGraph::ReadDOT(const string &infile)
{
	bool res;
	ifstream ifs(infile.c_str());
	string vlabel("vertex");
	res = read_graphviz(ifs, *this, dp, vlabel);
	ifs.close();

	return res;
}

/*!
Precomputes the adjusted edge weights, so that we don't have to
keep recomputing it.
*/
void FCGraph::PrecompAdjEdgeWt(const float adjust)
{
	pair<FCEdgeItr, FCEdgeItr> ep;
	for(ep = edges(*this); ep.first != ep.second; ++ep.first)
	{
		if( (float)edgeWeightPropMap[*ep.first] <= adjust )
			adjustedEdgeWeight[*ep.first] = exp(((float)edgeWeightPropMap[*ep.first] - adjust)/adjust);
		else
			adjustedEdgeWeight[*ep.first] =(float)edgeWeightPropMap[*ep.first]  - adjust + (float)1;
	}
}

/*!
Prints all vertices.
*/
void FCGraph::PrintVertices()
{
	pair<FCVertexItr, FCVertexItr> vp;
	PRINT("Printing vertices...\n");
	int i = 0;
	for(vp = vertices(*this); vp.first != vp.second; ++i, ++vp.first)
	{
		PRINT("[%d, %lu] %s\n", i, vertexIdxPropMap[*vp.first], vertexNamePropMap[*vp.first].c_str());
	}
}

/*!
Prints all edges.
*/
void FCGraph::PrintEdges()
{
	pair<FCEdgeItr, FCEdgeItr> ep;
	PRINT("Printing edge weights...\n");
	int i = 0;
	for(ep = edges(*this); ep.first != ep.second; ++i, ++ep.first)
	{
		PRINT("[%d] %d\n", i, edgeWeightPropMap[*ep.first]);
	}
}

/*!
Updates name cost using Levenshtein edit distance.
*/
/*
int FCGraph::AddNameCost(gsl_matrix *pCostMatrix)
{
	pair<FCVertexItr, FCVertexItr> vp;
	//PRINT("Updating name cost...\n");

	int numvert = num_vertices(*this);
	for(int i = 0; i < numvert; ++i)
	{
		FCVertex u = vertex(i, *this);
		for(int j = 0; j < i; ++j)
		{
			FCVertex v = vertex(j, *this);

			int dist = LevenshteinDistance(vertexNamePropMap[u], vertexNamePropMap[v]);
			gsl_matrix_set(pCostMatrix, i, j, gsl_matrix_get(pCostMatrix, i, j) + dist);

			// No need to recalculate distance, results are symmetrical.
cd			gsl_matrix_set(pCostMatrix, j, i, gsl_matrix_get(pCostMatrix, j, i) + dist);	
		}
	}

	return 0;
}
*/

/*!
Updates edge cost. Adds 1 if there is an edge from
vertex i to vertex j.
*/
/*
int FCGraph::AddEdgeCost(gsl_matrix *pCostMatrix)
{
	std::pair<FCEdgeItr, FCEdgeItr> ep;
	for( ep = edges(*this); ep.first != ep.second; ++ep.first)
	{
		int i, j;

		// Get indices of source and target
		i = vertexIdxPropMap[source(*ep.first, *this)];
		j = vertexIdxPropMap[target(*ep.first, *this)];

		float edgecost;
		edgecost = edgeWeightPropMap[*ep.first];
		//PRINT("edgecost = %.2f\n", edgecost);
	
		// Add 1
		gsl_matrix_set(pCostMatrix, i, j, gsl_matrix_get(pCostMatrix, i, j) + edgecost);
	}

	return 0;
}
*/
/*!
Updates user specified cost matrix.
*/
/*
int FCGraph::UpdateCostMatrix(gsl_matrix *pCostMatrix, int costflags)
{
	if(pCostMatrix == NULL)
		return EINVAL;

	if(pCostMatrix->size1 < num_vertices(*this))
		return EINVAL;

	// Set all elements to 0
	gsl_matrix_set_zero(pCostMatrix);

	if(costflags & FCGraph_USENAME)
		AddNameCost(pCostMatrix);

	if(costflags & FCGraph_USEEDGE)
		AddEdgeCost(pCostMatrix);

	return 0;	
}
*/

