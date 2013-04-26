// Function Call Graph

#ifndef __FCG_H__
#define __FCG_H__

#include <math.h>
#include <boost/graph/properties.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/property_map/property_map.hpp>
#include <gsl/gsl_matrix.h>
#include <log4cpp/Category.hh>
#include <iostream>
#include <function.h>
using namespace boost;
using namespace std;

// Stuff we need to define ourself
// if not compiled with IDA definitions.
#ifdef __IDP__
#include <ida.hpp>
#include <idp.hpp>
#else
#ifdef __X64__
typedef uint64_t ea_t;	// effective address
#else
typedef uint32_t ea_t;	// effective address
#endif
#endif


// Install additional graph, vertex and edge properties
namespace boost{
//enum graph_fileId_t	{ graph_fileId		};
enum vertex_funcId_t	{ vertex_funcId		};
enum vertex_startEA_t	{ vertex_startEA	};
enum vertex_endEA_t	{ vertex_endEA		};
enum vertex_size_t	{ vertex_size		};
enum vertex_bbs_t	{ vertex_bbs		};
enum vertex_cyccomp_t	{ vertex_cyccomp	};
enum vertex_nvars_t	{ vertex_nvars		};
enum vertex_nparams_t	{ vertex_nparams	};
enum vertex_nngrams_t	{ vertex_nngrams	};
enum vertex_level_t	{ vertex_level		};
enum vertex_compnum_t	{ vertex_compnum	};
enum vertex_nspill_t	{ vertex_nspill		};
enum vertex_cluster_t	{ vertex_cluster	};
enum vertex_taint_t	{ vertex_taint		};
enum vertex_weight_t	{ vertex_weight		};
enum vertex_paired_t	{ vertex_paired		};
enum edge_taint_t	{ edge_taint		};
enum edge_nspill_t	{ edge_nspill		};

//BOOST_INSTALL_PROPERTY( graph, fileId	);
BOOST_INSTALL_PROPERTY( vertex, funcId	);
BOOST_INSTALL_PROPERTY( vertex, startEA	);
BOOST_INSTALL_PROPERTY( vertex, endEA	);
BOOST_INSTALL_PROPERTY( vertex, size	);
BOOST_INSTALL_PROPERTY( vertex, bbs	);
BOOST_INSTALL_PROPERTY( vertex, cyccomp	);
BOOST_INSTALL_PROPERTY( vertex, nvars	);
BOOST_INSTALL_PROPERTY( vertex, nparams	);
BOOST_INSTALL_PROPERTY( vertex, nngrams	);
BOOST_INSTALL_PROPERTY( vertex, level	);
BOOST_INSTALL_PROPERTY( vertex, compnum	);
BOOST_INSTALL_PROPERTY( vertex, nspill	);
BOOST_INSTALL_PROPERTY( vertex, cluster	);
BOOST_INSTALL_PROPERTY( vertex, taint	);
BOOST_INSTALL_PROPERTY( vertex, weight	);
BOOST_INSTALL_PROPERTY( vertex, paired	);
BOOST_INSTALL_PROPERTY( edge, taint	);
BOOST_INSTALL_PROPERTY( edge, nspill	);
}

// Graph property types
//typedef property < graph_fileId_t,	unsigned int					> FCGraphFileIdProp;
//typedef property < graph_name_t,	string,			FCGraphFileIdProp	> FCGraphNameProp;
// Vertex property types
typedef property < vertex_name_t,	string						> FCVertexNameProp;
typedef property < vertex_funcId_t,	unsigned int,		FCVertexNameProp	> FCVertexFuncIdProp;
typedef property < vertex_startEA_t,	ea_t,			FCVertexFuncIdProp	> FCVertexStartEAProp;
typedef property < vertex_endEA_t,	ea_t,			FCVertexStartEAProp	> FCVertexEndEAProp;
typedef property < vertex_size_t,	size_t,			FCVertexEndEAProp	> FCVertexSizeProp;
typedef property < vertex_bbs_t,	vector<BasicBlock*>*,	FCVertexSizeProp	> FCVertexBBsProp;
typedef property < vertex_cyccomp_t,	unsigned int,		FCVertexBBsProp	> FCVertexCycCompProp;
typedef property < vertex_nvars_t,	unsigned int,		FCVertexCycCompProp	> FCVertexNvarsProp;
typedef property < vertex_nparams_t,	unsigned int,		FCVertexNvarsProp	> FCVertexNparamsProp;
typedef property < vertex_nngrams_t,	unsigned int,		FCVertexNparamsProp	> FCVertexNngramsProp;
typedef property < vertex_level_t,	unsigned int,		FCVertexNngramsProp	> FCVertexLevelProp;
typedef property < vertex_compnum_t,	unsigned int,		FCVertexLevelProp	> FCVertexCompNumProp;
typedef property < vertex_nspill_t,	unsigned int,		FCVertexCompNumProp	> FCVertexNspillProp;
typedef property < vertex_cluster_t,	unsigned int,		FCVertexNspillProp	> FCVertexClusterProp;
typedef property < vertex_taint_t,	bool,			FCVertexClusterProp	> FCVertexTaintProp;
typedef property < vertex_weight_t,	float,			FCVertexTaintProp	> FCVertexWeightProp;
typedef property < vertex_paired_t,	string,			FCVertexWeightProp	> FCVertexPairedProp;
typedef property < vertex_index_t,	unsigned int,		FCVertexPairedProp	> FCVertexIdxProp;
// Edge property types
typedef property < edge_weight_t,	float						> FCEdgeWeightProp;
typedef property < edge_taint_t,	bool,			FCEdgeWeightProp	> FCEdgeTaintProp;
typedef property < edge_nspill_t,	unsigned int,		FCEdgeTaintProp		> FCEdgeNspillProp;
typedef property < edge_index_t,	unsigned int,		FCEdgeNspillProp	> FCEdgeIdxProp;

// Graph type
typedef adjacency_list<vecS, vecS, bidirectionalS,
	FCVertexIdxProp, FCEdgeIdxProp/*,
	FCGraphFileIdProp FCGraphNameProp*/>				FCGraph_t;

// Graph properties
//typedef graph_property<FCGraph_t, graph_name_t>::type	FCGraphName;
//typedef graph_property<FCGraph_t, graph_fileId_t>::type	FCGraphFileId;

// Vertex and Edge property accessors
typedef property_map<FCGraph_t, vertex_name_t>::type 	FCVertexNamePropMap;
typedef property_map<FCGraph_t, vertex_funcId_t>::type 	FCVertexFuncIdPropMap;
typedef property_map<FCGraph_t, vertex_startEA_t>::type	FCVertexStartEAPropMap;
typedef property_map<FCGraph_t, vertex_endEA_t>::type 	FCVertexEndEAPropMap;
typedef property_map<FCGraph_t, vertex_size_t>::type 	FCVertexSizePropMap;
typedef property_map<FCGraph_t, vertex_bbs_t>::type	FCVertexBBsPropMap;
typedef property_map<FCGraph_t, vertex_cyccomp_t>::type FCVertexCycCompPropMap;
typedef property_map<FCGraph_t, vertex_nvars_t>::type 	FCVertexNvarsPropMap;
typedef property_map<FCGraph_t, vertex_nparams_t>::type FCVertexNparamsPropMap;
typedef property_map<FCGraph_t, vertex_nngrams_t>::type FCVertexNngramsPropMap;
typedef property_map<FCGraph_t, vertex_level_t>::type 	FCVertexLevelPropMap;
typedef property_map<FCGraph_t, vertex_compnum_t>::type FCVertexCompNumPropMap;
typedef property_map<FCGraph_t, vertex_nspill_t>::type	FCVertexNspillPropMap;
typedef property_map<FCGraph_t, vertex_cluster_t>::type	FCVertexClusterPropMap;
typedef property_map<FCGraph_t, vertex_taint_t>::type	FCVertexTaintPropMap;
typedef property_map<FCGraph_t,	vertex_weight_t>::type	FCVertexWeightPropMap;
typedef property_map<FCGraph_t, vertex_paired_t>::type	FCVertexPairedPropMap;
typedef property_map<FCGraph_t, vertex_index_t>::type	FCVertexIdxPropMap;
typedef property_map<FCGraph_t, edge_weight_t>::type	FCEdgeWeightPropMap;
typedef property_map<FCGraph_t, edge_taint_t>::type	FCEdgeTaintPropMap;
typedef property_map<FCGraph_t, edge_nspill_t>::type	FCEdgeNspillPropMap;
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

class FCGraph:public FCGraph_t
{
public:
	FCGraph( const unsigned int fileId, log4cpp::Category *const pLog );
	~FCGraph();

	FCVertex AddVertex(
		const unsigned int	funcId,
		const char *const	vname,
		const ea_t		startEA,
		const ea_t		endEA,
		const size_t		size,
		const unsigned int	cyccomp,
		const unsigned int	nvars,
		const unsigned int	nparams,
		const unsigned int	nngrams,
		const unsigned int	level,
		const unsigned int	compnum);
	int AddVertexBBs( const FCVertex V, vector<BasicBlock*> *const bbs );
	int AddEdge( const FCVertex &u, const FCVertex &v );
	
	void WriteDOT(const string &outfile);
	bool ReadDOT(const string &infile);
	//int GetNumFuncs();
	//int UpdateCostMatrix(gsl_matrix *pCostMatrix, int costflags);

	//void PrecompAdjEdgeWt(const float adjust);
	
	//int GetNumNodes();
	//int GetNumEdges();

	void PrintVertices();
	void PrintEdges();


	const string&		GetGraphName()					{ return /*get_property(*this, graph_name);*/ graphName;	}
	const unsigned int	GetGraphFileId()				{ return /*get_property(*this, graph_fileId);*/ graphFileId;	}
	const string		GetVertexName(const FCVertex &V)		{ return vertexNamePropMap[V];		}
	const ea_t		GetVertexStartEA(const FCVertex &V)		{ return vertexStartEAPropMap[V];		}
	const ea_t		GetVertexEndEA(const FCVertex &V)		{ return vertexEndEAPropMap[V];			}
	const size_t		GetVertexSize(const FCVertex &V)		{ return vertexSizePropMap[V];			}
	vector<BasicBlock*>*	GetVertexBBs(const FCVertex &V)			{ return vertexBBsPropMap[V];			}
	const unsigned int	GetVertexFuncId(const FCVertex &V)		{ return vertexFuncIdPropMap[V];		}
	const unsigned int	GetVertexCycComp(const FCVertex &V)		{ return vertexCycCompPropMap[V];		}
	const unsigned int	GetVertexNvars(const FCVertex &V)		{ return vertexNvarsPropMap[V];			}
	const unsigned int	GetVertexNparams(const FCVertex &V)		{ return vertexNparamsPropMap[V];		}
	const unsigned int	GetVertexNngrams(const FCVertex &V)		{ return vertexNngramsPropMap[V];		}
	const unsigned int	GetVertexLevel(const FCVertex &V)		{ return vertexLevelPropMap[V];			}
	const unsigned int	GetVertexCompNum(const FCVertex &V)		{ return vertexCompNumPropMap[V];		}
	const unsigned int	GetVertexNspill(const FCVertex &v)		{ return vertexNspillPropMap[v];		}
	const unsigned int	GetVertexCluster(const FCVertex &v)		{ return vertexClusterPropMap[v];		}
	const bool		GetVertexTaint(const FCVertex &v)		{ return vertexTaintPropMap[v];			}
	const float		GetVertexWeight(const FCVertex &V)		{ return vertexWeightPropMap[V];		}
	const string		GetVertexPaired(const FCVertex &V)		{ return vertexPairedPropMap[V];		}
	const unsigned int	GetVertexIdx(const FCVertex &V)			{ return vertexIdxPropMap[V];			}
	const float		GetEdgeWeight(const FCEdge &E)			{ return edgeWeightPropMap[E];			}
	const bool		GetEdgeTaint(const FCEdge &E)			{ return edgeTaintPropMap[E];			}
	const unsigned int	GetEdgeNspill(const FCEdge &E)			{ return edgeNspillPropMap[E];			}
	const unsigned int	GetEdgeIdx(const FCEdge &E)			{ return edgeIdxPropMap[E];			}
	const ea_t		GetDiameter()					{ return maxEndEA - minStartEA;			}


	const size_t		GetVertexNumBBs(const FCVertex &V);

	void			SetVertexWeight(const FCVertex &V, const float weight ) { vertexWeightPropMap[V] = weight;	}
	void			SetVertexPaired(const FCVertex &V, const string paired)	{ vertexPairedPropMap[V] = paired;	}
	void			ClearVertexPaired(const FCVertex &V)		{ vertexPairedPropMap[V].clear();		}
	void			SetVertexTaint(const FCVertex &V)		{ vertexTaintPropMap[V] = true;			}
	void			ClearVertexTaint(const FCVertex &V)		{ vertexTaintPropMap[V] = false;		}
		
	void			SetEdgeTaint(const FCEdge &E)			{ edgeTaintPropMap[E] = true;			}
	void			ClearEdgeTaint(const FCEdge &E)			{ edgeTaintPropMap[E] = false;			}
	void			SetVertexCluster(const FCVertex &V, const unsigned int c) { vertexClusterPropMap[V] = c;	}

	int			SpillWeight( const FCVertex &v, const float weight, const unsigned short nReach );
	int			NormalizeEdgeWeights(void);

	FCVertex		GetVertexByIdx( const unsigned int idx )	{ return idxToVertex[idx]; }	/// Returns vertex by index
	FCVertex		FindVertex(const unsigned int funcId);		/// Finds vertex by function id
	FCVertex		GetFirstVertex(void);
	FCVertex		GetNextVertex(void);
	void			GetActiveVertices(set<FCVertex> &activeV);

	void SetGraphName(const string &name)
	{
		//FCGraphName &graphName = get_property(*this, graph_name);
		graphName = name;
	}

	void SetGraphFileId(const unsigned int fileId)
	{
		//FCGraphFileId &graphFileId = get_property(*this, graph_fileId);
		graphFileId = fileId;
	}

	int PropagateWeights();

private:
  string graphName;
  unsigned int graphFileId;
	//ref_property_map<FCGraph_t*, string>		*pGraphNamePropMap;	/// Graph name property map
	//ref_property_map<FCGraph_t*, unsigned int>	*pGraphFileIdPropMap;	/// Graph file id property map
	FCVertexNamePropMap				vertexNamePropMap;	/// Vertex name property map
	FCVertexStartEAPropMap				vertexStartEAPropMap;	/// Vertex start EA property map
	FCVertexEndEAPropMap				vertexEndEAPropMap;	/// Vertex end EA property map
	FCVertexSizePropMap				vertexSizePropMap;	/// Vertex size property map
	FCVertexBBsPropMap				vertexBBsPropMap;	/// Vertex basic blocks property map
	FCVertexFuncIdPropMap				vertexFuncIdPropMap;	/// Vertex function id property map
	FCVertexCycCompPropMap				vertexCycCompPropMap;	/// Vertex cyclomatic complexity property map
	FCVertexNvarsPropMap				vertexNvarsPropMap;	/// Vertex num of variables property map
	FCVertexNparamsPropMap				vertexNparamsPropMap;	/// Vertex num of parameters property map
	FCVertexNngramsPropMap				vertexNngramsPropMap;	/// Vertex num of ngrams property map
	FCVertexLevelPropMap				vertexLevelPropMap;	/// Vertex level property map
	FCVertexCompNumPropMap				vertexCompNumPropMap;	/// Vertex component number property map
	FCVertexNspillPropMap				vertexNspillPropMap;	/// Vertex num of spills property map
	FCVertexClusterPropMap				vertexClusterPropMap;	/// Vertex cluster property map
	FCVertexTaintPropMap				vertexTaintPropMap;	/// Vertex taint property map
	FCVertexWeightPropMap				vertexWeightPropMap;	/// Vertex weight property map
	FCVertexPairedPropMap				vertexPairedPropMap;	/// Vertex paired property map
	FCVertexIdxPropMap				vertexIdxPropMap;	/// Vertex index property map
	FCEdgeWeightPropMap				edgeWeightPropMap;	/// Edge weight property map
	FCEdgeTaintPropMap				edgeTaintPropMap;	/// Edge taint property map
	FCEdgeNspillPropMap				edgeNspillPropMap;	/// Edge nspill property map
	FCEdgeIdxPropMap				edgeIdxPropMap;		/// Edge index property map
	dynamic_properties				dp;			/// Dynamic properties for reading

	map< unsigned int, FCVertex >			idxToVertex;
	pair<FCVertexItr, FCVertexItr>			vp;			/// Vertex iterators pair
	float						maxEdgeWeight;
	ea_t						minStartEA;		/// Minimum start EA
	ea_t						maxEndEA;		/// Maximum start EA
	set<FCVertex>					activeVertices;		/// Set of active vertices, i.e. non-0 edge weights
	log4cpp::Category				*pLog;			/// Pointer to logger

	int SpillWeightHelper(
		const FCVertex &v,
		const float weight,
		const unsigned short nReachLeft,
		set<FCEdge> &setTaintedEdges );

	int PropagateWeightsHelper( FCVertex &V );

	//int AddNameCost(gsl_matrix *pCostMatrix);
	
	//int AddEdgeCost(gsl_matrix *pCostMatrix);
};

#endif
