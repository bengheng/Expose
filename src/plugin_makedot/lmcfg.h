#ifndef __LMCFG_H__
#define __LMCFG_H__

#include <boost/graph/properties.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/dominator_tree.hpp>
#include <algorithm>
#include <ida.hpp>
#include <funcs.hpp>
#include "function_analyzer.h"

using namespace boost;


// Property type
typedef property<vertex_index1_t, ea_t> VertexIndexProperty;

// Graph type
typedef adjacency_list<vecS, vecS, bidirectionalS/*directedS*/, VertexIndexProperty> Graph;

// Vertex and Edge descriptors
typedef graph_traits<Graph>::vertex_descriptor Vertex;
typedef graph_traits<Graph>::edge_descriptor Edge;

// Vertex iterator type
typedef graph_traits<Graph>::vertex_iterator vertex_iter;
typedef graph_traits<Graph>::edge_iterator edge_iter;

typedef property_map<Graph, vertex_index_t>::type IndexMap;
typedef iterator_property_map<std::vector<Vertex>::iterator, IndexMap> PredMap;


class LMCFG
{
public:
	LMCFG();
	~LMCFG();

	int GenGraph(function_analyzer *pFuncAnalyzer);
	int ShowGraph();
	//int ComputeDomTree();
	//int ComputeLoops();

	void PrintVertices();
	//void PrintIdoms();
private:
	function_analyzer *pFA;					/// Ptr to Function analyzer
	Graph g;						/// Graph instance
	property_map<Graph, vertex_index1_t>::type vertex_idx;	/// Property accessor
	IndexMap indexMap;
	//std::vector<Vertex> domTreePredVector;
	//PredMap domTreePredMap;
	//std::vector<ea_t> idom;

	int AddVertices();
	int AddVertex(ea_t nodeaddr);
	int AddEdges();
	Vertex FindVertex(ea_t addr);
	bool FindEdge(Vertex u, Vertex v, Edge *e);
	int GetVertexIdx(Vertex v, int starti);

	template < typename Graph, typename Loops > void
	find_loops(typename graph_traits < Graph >::vertex_descriptor entry, 
		const Graph & g, 
		Loops & loops);    // A container of sets of vertices

	template < typename Graph, typename Set > void
	compute_loop_extent(typename graph_traits <
		Graph >::edge_descriptor back_edge, const Graph & g,
		Set & loop_set);
};

#endif
