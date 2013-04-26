// Function Call Graph

#ifndef __FCG_H__
#define __FCG_H__

#include <math.h>
#include <boost/graph/properties.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/property_map/property_map.hpp>
#include <gsl/gsl_matrix.h>
#include <iostream>
using namespace boost;
using namespace std;

// Install additional vertex property

enum vertex_start_t { vertex_start };
namespace boost{
BOOST_INSTALL_PROPERTY(vertex, start);
}
enum vertex_end_t { vertex_end };
namespace boost{
BOOST_INSTALL_PROPERTY(vertex, end);
}
enum vertex_nodes_t { vertex_nodes };
namespace boost{
BOOST_INSTALL_PROPERTY(vertex, nodes);
}
enum vertex_edges_t { vertex_edges };
namespace boost{
BOOST_INSTALL_PROPERTY(vertex, edges);
}
enum vertex_calls_t { vertex_calls };
namespace boost{
BOOST_INSTALL_PROPERTY(vertex, calls);
}
enum vertex_instrs_t { vertex_instrs };
namespace boost{
BOOST_INSTALL_PROPERTY(vertex, instrs);
}

// Install additional edge property
enum edge_loop_t { edge_loop };
namespace boost{
BOOST_INSTALL_PROPERTY(edge, loop);
}
// Vertex and Edge property types
//typedef property<graph_name_t, string>					FCGraphNameProp;
typedef property<vertex_name_t, string>					FCVertexNameProp;
typedef property<vertex_start_t, unsigned int, FCVertexNameProp>	FCVertexStartProp;
typedef property<vertex_end_t, unsigned int, FCVertexStartProp>		FCVertexEndProp;
typedef property<vertex_nodes_t, unsigned int, FCVertexEndProp>		FCVertexNodesProp;
typedef property<vertex_edges_t, unsigned int, FCVertexNodesProp>	FCVertexEdgesProp;
typedef property<vertex_calls_t, unsigned int, FCVertexEdgesProp>	FCVertexCallsProp;
typedef property<vertex_instrs_t, unsigned int, FCVertexCallsProp>	FCVertexInstrsProp;
typedef property<vertex_index_t, int, FCVertexInstrsProp>		FCVertexIdxProp;
typedef property<edge_weight_t, int>					FCEdgeWeightProp;
typedef property<edge_loop_t, int, FCEdgeWeightProp>			FCEdgeLoopProp;
typedef property<edge_index_t, int, FCEdgeLoopProp>			FCEdgeIdxProp;

// Graph type
typedef adjacency_list<vecS, vecS, bidirectionalS,
	FCVertexIdxProp, FCEdgeIdxProp/*,
	FCGraphNameProp*/>				FCGraph_t;

// Graph property
//typedef graph_property<FCGraph_t, graph_name_t>::type	FCGraphName;

// Vertex and Edge property map types
typedef property_map<FCGraph_t, vertex_name_t>::type 	FCVertexNamePropMap;
typedef property_map<FCGraph_t, vertex_start_t>::type 	FCVertexStartPropMap;
typedef property_map<FCGraph_t, vertex_end_t>::type 	FCVertexEndPropMap;
typedef property_map<FCGraph_t, vertex_nodes_t>::type 	FCVertexNodesPropMap;
typedef property_map<FCGraph_t, vertex_edges_t>::type 	FCVertexEdgesPropMap;
typedef property_map<FCGraph_t, vertex_calls_t>::type 	FCVertexCallsPropMap;
typedef property_map<FCGraph_t, vertex_instrs_t>::type 	FCVertexInstrsPropMap;
typedef property_map<FCGraph_t, vertex_index_t>::type	FCVertexIdxPropMap;
typedef property_map<FCGraph_t, edge_weight_t>::type	FCEdgeWeightPropMap;
typedef property_map<FCGraph_t, edge_loop_t>::type	FCEdgeLoopPropMap;
typedef property_map<FCGraph_t, edge_index_t>::type	FCEdgeIdxPropMap;

// Vertex and Edge descriptors
typedef graph_traits<FCGraph_t>::vertex_descriptor	FCVertex;
typedef graph_traits<FCGraph_t>::edge_descriptor	FCEdge;

// Vertex and Edge iterator types
typedef graph_traits<FCGraph_t>::vertex_iterator	FCVertexItr;
typedef graph_traits<FCGraph_t>::edge_iterator		FCEdgeItr;
typedef graph_traits<FCGraph_t>::out_edge_iterator	FCOutEdgeItr;
typedef graph_traits<FCGraph_t>::in_edge_iterator	FCInEdgeItr;
// NULL vertex
#define  FCG_NULL_VERTEX	graph_traits<FCGraph_t>::null_vertex()

#define FCG_USENAME		(0x1)
#define FCG_USEEDGE		(0x1 << 1)

#define FCG_EDGECOST		1	// Cost of a directed edge

class FCGraph:public FCGraph_t
{
public:
	FCGraph();
	~FCGraph();

	int AddVertex(
		string vname,
		unsigned int start,
		unsigned int end,
		unsigned int nodes,
		unsigned int edges,
		unsigned int calls,
		unsigned int instrs);
	int AddEdgeWeight(string uname, std::string vname, int weight);
	int AddEdgeLoop(string uname, std::string vname, int loop);
	
	void WriteDOT(const string &outfile);
	bool ReadDOT(const string &infile);
	//int GetNumFuncs();
	//int UpdateCostMatrix(gsl_matrix *pCostMatrix, int costflags);

	void PrecompAdjEdgeWt(const float adjust);
	
	//int GetNumNodes();
	//int GetNumEdges();

	void PrintVertices();
	void PrintEdges();


	const string GetGraphName()					{ return /*get_property(*this, graph_name);*/ graphName;	}
	const string GetVertexName(const FCVertex &V)			{ return vertexNamePropMap[V];			}
	const unsigned int GetVertexStart(const FCVertex &V)		{ return vertexStartPropMap[V];			}
	const unsigned int GetVertexEnd(const FCVertex &V)		{ return vertexEndPropMap[V];			}
	const unsigned int GetVertexNodes(const FCVertex &V)		{ return vertexNodesPropMap[V];			}
	const unsigned int GetVertexEdges(const FCVertex &V)		{ return vertexEdgesPropMap[V];			}
	const unsigned int GetVertexCalls(const FCVertex &V)		{ return vertexCallsPropMap[V];			}
	const unsigned int GetVertexInstrs(const FCVertex &V)		{ return vertexInstrsPropMap[V];			}
	const float GetEdgeWeight(const FCEdge &E)			{ return edgeWeightPropMap[E];			}
	const float GetEdgeExpWeight(const FCEdge &E)			{ return adjustedEdgeWeight[E];			}

	void SetGraphName(const string &name)
	{
		//FCGraphName &graphName = get_property(*this, graph_name);
		graphName = name;
	}

private:
  string                  graphName;
	//ref_property_map<FCGraph_t*, string>	*pGraphNamePropMap;	/// Graph name property map
	FCVertexNamePropMap			vertexNamePropMap;	/// Vertex name property map
	FCVertexStartPropMap			vertexStartPropMap;	/// Vertex start property map
	FCVertexEndPropMap			vertexEndPropMap;	/// Vertex end property map
	FCVertexNodesPropMap			vertexNodesPropMap;	/// Vertex nodes property map
	FCVertexEdgesPropMap			vertexEdgesPropMap;	/// Vertex edges property map
	FCVertexCallsPropMap			vertexCallsPropMap;	/// Vertex calls property map
	FCVertexInstrsPropMap			vertexInstrsPropMap;	/// Vertex instrs property map
	FCVertexIdxPropMap			vertexIdxPropMap;	/// Vertex index property map
	FCEdgeWeightPropMap			edgeWeightPropMap;	/// Edge weight property map
	FCEdgeLoopPropMap			edgeLoopPropMap;	/// Edge loop property map
	FCEdgeIdxPropMap			edgeIdxPropMap;		/// Edge index property map
	dynamic_properties			dp;			/// Dynamic properties for reading

	std::map< FCEdge, float >		adjustedEdgeWeight;	/// Adjusted edge weights
	
	FCVertex FindVertex(string vname);		/// Finds vertex
	//int AddNameCost(gsl_matrix *pCostMatrix);
	int AddEdgeCost(gsl_matrix *pCostMatrix);
};

#endif
