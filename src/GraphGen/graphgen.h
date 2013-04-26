#ifndef __GRAPHGEN_H__
#define __GRAPHGEN_H__

#include <boost/graph/properties.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graphviz.hpp>

#define GG_ALLOW_DUP_VERTEX 1
#define GG_BLOCK_DUP_VERTEX 0

using namespace boost;

// Vertex and Edge property types
typedef property<vertex_name_t, std::string> GGVertexNameProp;
typedef property<vertex_index_t, int, GGVertexNameProp> GGVertexIdxProp;
typedef property<edge_weight_t, int> GGEdgeWeightProp;
typedef property<edge_index_t, int, GGEdgeWeightProp> GGEdgeIdxProp;

// Graph type
typedef adjacency_list<vecS, vecS, directedS, GGVertexIdxProp, GGEdgeIdxProp> GGGraph;

// Vertex and Edge property map types
typedef property_map<GGGraph, vertex_name_t>::type GGVertexNamePropMap;
typedef property_map<GGGraph, vertex_index_t>::type GGVertexIdxPropMap;
typedef property_map<GGGraph, edge_weight_t>::type GGEdgeWeightPropMap;
typedef property_map<GGGraph, edge_index_t>::type GGEdgeIdxPropMap;

// Vertex and Edge descriptors
typedef graph_traits<GGGraph>::vertex_descriptor GGVertex;
typedef graph_traits<GGGraph>::edge_descriptor GGEdge;

// Vertex and Edge iterator types
typedef graph_traits<GGGraph>::vertex_iterator GGVertexItr;
typedef graph_traits<GGGraph>::edge_iterator GGEdgeItr;

class GraphGen
{
public:
	GraphGen(bool fAllowDupVertex);
	~GraphGen();

	int AddRandomVertex();
	int AddRandomEdge();
	int AddVertex(std::string vname);
	int WriteDOT(std::string outfile);

private:
	GGGraph			g;			/// Graph instance
	GGVertexNamePropMap	vertexNamePropMap;	/// Vertex name property map
	GGVertexIdxPropMap	vertexIdxPropMap;	/// Vertex index property map
	GGEdgeWeightPropMap	edgeWeightPropMap;	/// Edge weight property map
	GGEdgeIdxPropMap	edgeIdxPropMap;		/// Edge index property map
	
	GGVertex FindVertex(std::string vname);		/// Finds vertex
	char GetRandomCharacter();
	GGVertex GetRandomVertex();

	bool	fAllowDupVertex;
};

#endif
