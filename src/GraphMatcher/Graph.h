#ifndef __GRAPH_H__
#define __GRAPH_H__

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/properties.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/property_map/property_map.hpp>
#include "LMException.hpp"

using namespace boost;
using namespace std;

/// Vertex and Edge property types
//typedef property<graph_name_t, std::string>		LMGraphNameProp;
typedef property<vertex_name_t, std::string>		LMVertexNameProp;
typedef property<vertex_index_t, int, LMVertexNameProp>	LMVertexIdxProp;
typedef property<edge_weight_t, int>			LMEdgeWeightProp;
typedef property<edge_index_t, int, LMEdgeWeightProp>	LMEdgeIdxProp;

// Graph type
typedef adjacency_list<vecS, vecS, bidirectionalS,
	LMVertexIdxProp, LMEdgeIdxProp/*,
	LMGraphNameProp*/>				LMGraph_t;

// Graph property
//typedef graph_property<LMGraph_t, graph_name_t>::type	LMGraphName;

// Vertex and Edge property map types
//typedef property_map<LMGraph_t, graph_name_t>::type LMGraphNamePropMap;
typedef property_map<LMGraph_t, vertex_name_t>::type	LMVertexNamePropMap;
typedef property_map<LMGraph_t, vertex_index_t>::type	LMVertexIdxPropMap;
typedef property_map<LMGraph_t, edge_weight_t>::type	LMEdgeWeightPropMap;
typedef property_map<LMGraph_t, edge_index_t>::type	LMEdgeIdxPropMap;

// Vertex and Edge descriptors
typedef graph_traits<LMGraph_t>::vertex_descriptor	LMVertex;
typedef graph_traits<LMGraph_t>::edge_descriptor	LMEdge;

// Vertex and Edge iterator types
typedef graph_traits<LMGraph_t>::vertex_iterator	LMVertexItr;
typedef graph_traits<LMGraph_t>::edge_iterator		LMEdgeItr;
typedef graph_traits<LMGraph_t>::out_edge_iterator	LMOutEdgeItr;

class LMGraph:public LMGraph_t
{
public:
	LMGraph(const string dotName) throw (LMException);
	~LMGraph() throw();
	
	const string& GetGraphName()			{ return /*get_property(*this, graph_name);*/ graphName;	}
	const string GetVertexName(const LMVertex &N)	{ return vertexNamePropMap[N];		}
	const float GetEdgeWeight(const LMEdge &E)	{ return edgeWeightPropMap[E];		}

	void SetGraphName(const std::string &name)
	{
		//LMGraphName &graphName = get_property(*this, graph_name);
		graphName = name;
	}


private:
  std::string           graphName;
	//ref_property_map<LMGraph_t*, string>	*pGraphNamePropMap;	/// Graph name property map
	LMVertexNamePropMap			vertexNamePropMap;	/// Vertex name property map
	LMVertexIdxPropMap			vertexIdxPropMap;	/// Vertex index property map
	LMEdgeWeightPropMap			edgeWeightPropMap;	/// Edge weight property map
	LMEdgeIdxPropMap			edgeIdxPropMap;		/// Edge index property map
	dynamic_properties			dp;			/// Dynamic properties

	bool ReadDot(const string infile);		/// Reads graph from Dot file
};

#endif
