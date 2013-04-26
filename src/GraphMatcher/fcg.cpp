#include <ostream>
#include <iomanip>
#include <utility>
#include <function.h>
#include <log4cpp/Category.hh>
#include <boost/graph/graphviz.hpp>
#include <gsl/gsl_matrix.h>
#include "../utils/utils.h"

#include <fcg.h>


#define THRESHOLD 0.3

using namespace std;
using namespace boost;

const char *COLORTBL[] = {"red", "green", "yellow", "blue", "orange", "violet", "pink", "purple", "cyan", "magenta"};

class FCEdgeLabelWriter
{
public:
	FCEdgeLabelWriter(FCEdgeWeightPropMap _edgeWeightPropMap) :
		edgeWeightPropMap(_edgeWeightPropMap)
		{}

	void operator()(ostream &out, const FCEdge e) const
	{
		out << "[weight=\"" << get(edgeWeightPropMap, e) << "\"]";
		//out << "[penwidth=\""<< (get(edgeWeightPropMap, e))*5 << "\"]";
	}
private:
	FCEdgeWeightPropMap	edgeWeightPropMap;
};

class FCVertexLabelWriter
{
public:
	FCVertexLabelWriter(FCVertexNamePropMap _vertexNamePropMap,
		FCVertexStartEAPropMap _vertexStartEAPropMap,
		FCVertexEndEAPropMap _vertexEndEAPropMap,
		FCVertexSizePropMap _vertexSizePropMap,
		FCVertexFuncIdPropMap _vertexFuncIdPropMap,
		FCVertexCycCompPropMap _vertexCycCompPropMap,
		FCVertexNvarsPropMap _vertexNvarsPropMap,
		FCVertexNparamsPropMap _vertexNparamsPropMap,
		FCVertexNngramsPropMap _vertexNngramsPropMap,
		FCVertexLevelPropMap _vertexLevelPropMap,
		FCVertexCompNumPropMap _vertexCompNumPropMap,
		FCVertexClusterPropMap _vertexClusterPropMap,
		FCVertexWeightPropMap _vertexWeightPropMap,
		FCVertexPairedPropMap _vertexPairedPropMap) :

		vertexNamePropMap(_vertexNamePropMap),
		vertexStartEAPropMap(_vertexStartEAPropMap),
		vertexEndEAPropMap(_vertexEndEAPropMap),
		vertexSizePropMap(_vertexSizePropMap),
		vertexFuncIdPropMap(_vertexFuncIdPropMap),
		vertexCycCompPropMap(_vertexCycCompPropMap),
		vertexNvarsPropMap(_vertexNvarsPropMap),
		vertexNparamsPropMap(_vertexNparamsPropMap),
		vertexNngramsPropMap(_vertexNngramsPropMap),
		vertexLevelPropMap(_vertexLevelPropMap),
		vertexCompNumPropMap(_vertexCompNumPropMap),
		vertexClusterPropMap(_vertexClusterPropMap),
		vertexWeightPropMap(_vertexWeightPropMap),
		vertexPairedPropMap(_vertexPairedPropMap)
		{}

	void operator()(ostream &out, const FCVertex v) const
	{
		char color[256];

		/*
		if( strcmp( get(vertexNamePropMap, v).c_str(), get(vertexPairedPropMap, v).c_str() ) 
		== 0 )
		{
			snprintf( color, sizeof(color), "\"0.333, %.3f, 1.000\"",
				( get(vertexWeightPropMap, v) <= THRESHOLD ?
					(1 - get(vertexWeightPropMap, v)) : 0 ) );
		}
		else
		{
			snprintf( color, sizeof(color), "\"0.000, %.3f, 1.000\"",
				( get(vertexWeightPropMap, v) <= THRESHOLD ?
					(1 - get(vertexWeightPropMap, v)) : 0 ) );
		}
		*/
		snprintf( color, sizeof(color), "\"%.3f, 1.000, 1.000\"",
				get(vertexWeightPropMap, v)/2 );

		

		string paired = get(vertexPairedPropMap, v);
		out << "[label=\"" << get(vertexNamePropMap, v) << "\\n"
			<< (paired.empty() ? "" :  paired) << " ("
			<< get(vertexWeightPropMap, v) << ") \"]"
			<< "[startEA=\"0x" << hex << setw(8) << setfill('0') << get(vertexStartEAPropMap, v) << "\"]"
			<< "[endEA=\"0x" << hex <<  setw(8) << setfill('0') << get(vertexEndEAPropMap, v) <<  "\"]"
			<< "[size=\"" << dec << get(vertexSizePropMap, v) << "\"]"
			<< "[funcId=\"" << dec << get(vertexFuncIdPropMap, v) << "\"]"
			<< "[cyccomp=\"" << dec << get(vertexCycCompPropMap, v) << "\"]"
			<< "[nvars=\"" << dec << get(vertexNvarsPropMap, v) << "\"]"
			<< "[nparams=\"" << dec << get(vertexNparamsPropMap, v) << "\"]"
			<< "[nngrams=\"" << dec << get(vertexNngramsPropMap, v) << "\"]"
			<< "[level=\"" << dec << get(vertexLevelPropMap, v) << "\"]"
			<< "[compnum=\"" << dec << get(vertexCompNumPropMap, v) << "\"]"
			<< "[vertwt=\"" << dec << get(vertexWeightPropMap, v) << "\"]"
			<< "[style=\"filled\"]"
			<< "[fillcolor=" << color << "]";
			//<< "[fillcolor=\"" << COLORTBL[get(vertexClusterPropMap, v) % 10] << "\"]";
	}
private:
	FCVertexNamePropMap	vertexNamePropMap;
	FCVertexStartEAPropMap	vertexStartEAPropMap;
	FCVertexEndEAPropMap	vertexEndEAPropMap;
	FCVertexSizePropMap	vertexSizePropMap;
	FCVertexFuncIdPropMap	vertexFuncIdPropMap;
	FCVertexCycCompPropMap	vertexCycCompPropMap;
	FCVertexNvarsPropMap	vertexNvarsPropMap;
	FCVertexNparamsPropMap	vertexNparamsPropMap;
	FCVertexNngramsPropMap	vertexNngramsPropMap;
	FCVertexLevelPropMap	vertexLevelPropMap;
	FCVertexCompNumPropMap	vertexCompNumPropMap;
	FCVertexClusterPropMap	vertexClusterPropMap;
	FCVertexWeightPropMap	vertexWeightPropMap;
	FCVertexPairedPropMap	vertexPairedPropMap;
};

FCGraph::FCGraph( const unsigned int fileId, log4cpp::Category *const pLog )
{
	//pGraphNamePropMap	= new ref_property_map<FCGraph_t*, string>(get_property(*this, graph_name));
	//if(pGraphNamePropMap == NULL) return;

	//pGraphFileIdPropMap	= new ref_property_map<FCGraph_t*, unsigned int>(get_property(*this, graph_fileId));
	//if(pGraphFileIdPropMap == NULL) return;

	vertexNamePropMap	= get(vertex_name, *this);
	vertexStartEAPropMap	= get(vertex_startEA, *this);	
	vertexEndEAPropMap	= get(vertex_endEA, *this);	
	vertexSizePropMap	= get(vertex_size, *this);	
	vertexBBsPropMap	= get(vertex_bbs, *this);
	vertexFuncIdPropMap	= get(vertex_funcId, *this);	
	vertexCycCompPropMap	= get(vertex_cyccomp, *this);	
	vertexNvarsPropMap	= get(vertex_nvars, *this);	
	vertexNparamsPropMap	= get(vertex_nparams, *this);
	vertexNngramsPropMap	= get(vertex_nngrams, *this);
	vertexLevelPropMap	= get(vertex_level, *this);
	vertexCompNumPropMap	= get(vertex_compnum, *this);
	vertexNspillPropMap	= get(vertex_nspill, *this);
	vertexClusterPropMap	= get(vertex_cluster, *this);
	vertexTaintPropMap	= get(vertex_taint, *this);
	vertexWeightPropMap	= get(vertex_weight, *this);
	vertexPairedPropMap	= get(vertex_paired, *this);
	vertexIdxPropMap	= get(vertex_index, *this);
	edgeWeightPropMap	= get(edge_weight, *this);
	edgeTaintPropMap	= get(edge_taint, *this);
	edgeNspillPropMap	= get(edge_nspill, *this);
	edgeIdxPropMap		= get(edge_index, *this);

	dp.property("vertex",	get(vertex_name, *this));
	dp.property("startEA",	get(vertex_startEA, *this));
	dp.property("endEA",	get(vertex_endEA, *this));
	dp.property("size",	get(vertex_size, *this));
	dp.property("funcId",	get(vertex_funcId, *this));
	dp.property("cyccomp",	get(vertex_cyccomp, *this));
	dp.property("nvars",	get(vertex_nvars, *this));
	dp.property("nparams",	get(vertex_nparams, *this));
	dp.property("nngrams",	get(vertex_nngrams, *this));
	dp.property("level",	get(vertex_level, *this));
	dp.property("compnum",	get(vertex_compnum, *this));
	dp.property("cluster",	get(vertex_cluster, *this));
	dp.property("vertwt",	get(vertex_weight, *this));
	dp.property("edgewt",	get(edge_weight, *this));

	maxEdgeWeight		= 0;
	minStartEA		= 0xffffffff;
	maxEndEA		= 0;

	SetGraphFileId( fileId );
	this->pLog = pLog;
}

FCGraph::~FCGraph()
{
	// Free basic blocks
	std::pair<FCVertexItr, FCVertexItr> vp;
	for(vp = vertices(*this); vp.first != vp.second; ++vp.first)
	{
		vector<BasicBlock*> *bbs = vertexBBsPropMap[*vp.first];
		if( bbs != NULL )
		{
			std::vector<BasicBlock*>::iterator b1, e1;
			for( b1 = bbs->begin(), e1 = bbs->end();
			b1 != e1; ++b1 )
			{
				BasicBlock *bb = *b1;
				if( bb->insns.bin != NULL ) delete bb->insns.bin;
				delete bb;
			}
		}
		delete bbs;
	}

	//if (pGraphNamePropMap != NULL)		delete pGraphNamePropMap;
	//if (pGraphFileIdPropMap != NULL)	delete pGraphFileIdPropMap;
}

/*!
Finds the vertex with the specified function id.

Returns FCG_NULL_VERTEX if it doesn't exist.
*/
FCVertex FCGraph::FindVertex(const unsigned int funcId)
{
	if(num_vertices(*this) == 0)
		return FCG_NULL_VERTEX;

	std::pair<FCVertexItr, FCVertexItr> vp;
	for(vp = vertices(*this); vp.first != vp.second; ++vp.first)
	{
		if( funcId == vertexFuncIdPropMap[*vp.first] )
			return *vp.first;	// Found
	}

	// Cannot find vertex
	return FCG_NULL_VERTEX;
}

/*!
Gets the first vertex.
Use with GetNextVertex.
*/
FCVertex FCGraph::GetFirstVertex(void)
{
	vp = vertices(*this);
	return GetNextVertex();
}

/*!
Gets next vertex.
Use with GetFirstVertex.
*/
FCVertex FCGraph::GetNextVertex(void)
{
	if( vp.first == vp.second ) return FCG_NULL_VERTEX;
	return *(vp.first++);
}

/*!
Adds vertex with specified name if it doesn't exist.
*/
FCVertex FCGraph::AddVertex(
	const unsigned int	_funcId,
	const char *const	_vname,
	const ea_t		_startEA,
	const ea_t		_endEA,
	const size_t		_size,
	const unsigned int	_cyccomp,
	const unsigned int	_nvars,
	const unsigned int	_nparams,
	const unsigned int	_nngrams,
	const unsigned int	_level,
	const unsigned int	_compnum )
{
	FCVertex v;
	// Checks if vertex with vname already exists
	v = FindVertex(_funcId);
	if(v != FCG_NULL_VERTEX)
		return v;
	
	v = add_vertex(*this);
	vertexFuncIdPropMap[v]	= _funcId;
	vertexNamePropMap[v]	= string(_vname);
	vertexStartEAPropMap[v]	= _startEA;
	vertexEndEAPropMap[v]	= _endEA;
	vertexSizePropMap[v]	= _size;
	vertexCycCompPropMap[v]	= _cyccomp;
	vertexNvarsPropMap[v]	= _nvars;
	vertexNparamsPropMap[v]	= _nparams;
	vertexNngramsPropMap[v]	= _nngrams;
	vertexLevelPropMap[v]	= _level;
	vertexCompNumPropMap[v]	= _compnum;
	vertexNspillPropMap[v]	= 0;
	vertexTaintPropMap[v]	= false;
	vertexWeightPropMap[v]	= 1;

	if( _startEA < minStartEA ) minStartEA = _startEA;
	if( _endEA > maxEndEA ) maxEndEA = _endEA;

	idxToVertex[GetVertexIdx(v)] = v;

	return v;
}

int FCGraph::AddVertexBBs(const FCVertex V, vector <BasicBlock*> *const _bbs)
{
	assert( V != FCG_NULL_VERTEX );
	vertexBBsPropMap[V] = _bbs;
	return 0;
}

int FCGraph::AddEdge( const FCVertex &u, const FCVertex &v  )
{
	if( u == FCG_NULL_VERTEX || v == FCG_NULL_VERTEX )
		return -1;

	FCEdge e;
	bool res;
	tie(e, res) = edge(u, v, *this);
	if(res == true) return 0; // Already exist
	
	// Edge doesn't exist. Add new edge.
	tie(e, res) = add_edge(u, v, *this);
	if(res == false) return -1; // Error
	
	// Init edge properties
	edgeWeightPropMap[e] = 0;
	edgeTaintPropMap[e] = false;
	edgeNspillPropMap[e] = 0;

	return 0;
}


#define SPILL_DECAY_RATE		0.5

/*!
For all incoming and outgoing edges of the vertex v, apply the edge weight.
*/
int FCGraph::SpillWeightHelper(
	const FCVertex &v,
	const float weight,
	const unsigned short nReachLeft,
	set<FCEdge> &setTaintedEdges )
{
	if( nReachLeft == 0 ) return 0;
	if( weight == 0 ) return 0;

	//pLog->info("DEBUG Spilling %f", weight);

	// For all in-edges
	FCInEdgeItr b1, e1;
	for( tie( b1, e1 ) = in_edges( v, *this ); b1 != e1; ++b1 )
	{
		// Skip tainted edges to prevent double spilling
		if( GetEdgeTaint( *b1 ) == true ) continue;

		// If untainted, spill edge weight and taint
		edgeWeightPropMap[*b1] += weight;
		++edgeNspillPropMap[*b1];
		SetEdgeTaint( *b1 );
		setTaintedEdges.insert( *b1 );

		if( edgeWeightPropMap[*b1] > maxEdgeWeight )
			maxEdgeWeight = edgeWeightPropMap[*b1];

		// Repeat with source
		SpillWeightHelper( source(*b1, *this),
			weight*SPILL_DECAY_RATE,
			nReachLeft - 1,
			setTaintedEdges );
	}

	// For all out-edges
	FCOutEdgeItr b2, e2;
	for( tie(b2, e2) = out_edges( v, *this ); b2 != e2; ++b2 )
	{
		// Skip tainted edges to prevent double spilling
		if( GetEdgeTaint( *b2 ) == true ) continue;

		// If untainted, spill edge weight and taint
		edgeWeightPropMap[*b2] += weight;
		++edgeNspillPropMap[*b2];
		SetEdgeTaint( *b2 );
		setTaintedEdges.insert( *b2 );

		if( edgeWeightPropMap[*b2] > maxEdgeWeight )
			maxEdgeWeight = edgeWeightPropMap[*b2];

		// Repeat with target
		SpillWeightHelper( target( *b2, *this ),
			weight*SPILL_DECAY_RATE,
			nReachLeft - 1,
			setTaintedEdges );
	}

	return 0;	
}

/*!
Spills weight to neighboring edges. Weight is discounted such that
if there had been more spills by the vertex, the discount is larger.
*/
int FCGraph::SpillWeight(
	const FCVertex &v,
	const float weight,
	const unsigned short nReach )
{
	set< FCEdge> setTaintedEdges;

	// Discounted weight
	float discWeight = ( (float)(10 - vertexNspillPropMap[v]) / (float) 10 ) * weight;
	if( vertexNspillPropMap[v] < 10 ) ++vertexNspillPropMap[v];

	SpillWeightHelper( v, discWeight, nReach, setTaintedEdges );

	// Untaint all tainted edges
	set< FCEdge >::iterator b2, e2;
	for( b2 = setTaintedEdges.begin(), e2 = setTaintedEdges.end();
	b2 != e2; ++b2 )
	{
		ClearEdgeTaint( *b2 );
	}

	return 0;
}
/*!
Normalizes edge weights.
*/
int FCGraph::NormalizeEdgeWeights(void)
{
	pair<FCEdgeItr, FCEdgeItr> ep;
	for( ep = edges(*this) ; ep.first != ep.second; ++ep.first )
	{
		//edgeWeightPropMap[ *ep.first ] /= maxEdgeWeight;
		edgeWeightPropMap[*ep.first] /= edgeNspillPropMap[*ep.first];
		//if( edgeWeightPropMap[*ep.first] != 0 )
		//	pLog->info( "%f\t%s = %s",
		//		edgeWeightPropMap[*ep.first],
		//		GetVertexName( source(*ep.first, *this) ).c_str(),
		//		GetVertexName( target(*ep.first, *this) ).c_str() );

		// Set to 0 if does not meet threshold
		/*
		if( edgeWeightPropMap[ *ep.first] < 0.2 )
		{
			edgeWeightPropMap[ *ep.first ] = 0;
		}
		else
		{
			activeVertices.insert( source( *ep.first, *this) );
			activeVertices.insert( target( *ep.first, *this) );
		}
		*/
	}
	
	return 0;
}

/*!
Recursive function to propagate weights. Make sure the children's
weights are updated first before getting their weights.
*/
int FCGraph::PropagateWeightsHelper( FCVertex &V )
{
	// Skip tainted vertex.
	if( GetVertexTaint( V ) == true )
		return 0;

	// Set taint so that we don't come back here again.
	// This prevents loops.
	SetVertexTaint( V );

	// Base case. At the lowest level child.
	FCOutEdgeItr b1, e1;
	tie(b1, e1) = out_edges( V, *this );
	if( b1 == e1 )
	{	//
		return 0;
	}

	float totalCost = 0;
	int count = 0;
	// For all out-edges (children), update their weights.
	for( ; b1 != e1; ++b1 )
	{
		FCVertex child = target(*b1, *this );
		PropagateWeightsHelper( child );
		totalCost += GetVertexWeight( child );
		++count;
	}

	totalCost /= count;
	// Now update our own weight and return.
	totalCost = 0.3*totalCost + 0.7*GetVertexWeight( V );
	SetVertexWeight( V, totalCost / 2 );
	return 0;
}

/*!
Propagates weights from the lowest level children upwards.
*/
int FCGraph::PropagateWeights()
{
	// Untaint all vertices
	std::pair<FCVertexItr, FCVertexItr> vp;
	for(vp = vertices(*this); vp.first != vp.second; ++vp.first)
	{
		ClearVertexTaint( *vp.first );
	}

	// For every vertex without incoming edges,
	// propagate their weights.
	for( vp = vertices(*this); vp.first != vp.second; ++vp.first )
	{
		FCInEdgeItr b1, e1;
		tie( b1, e1 ) = in_edges( *vp.first, *this );
		if( b1 != e1 ) continue;

		FCVertex V = *vp.first;
		PropagateWeightsHelper( V );
	}
}


/*
void FCGraph::GetActiveVertices(set<FCVertex> &activeV)
{
	set<FCVertex>::iterator b, e;
	for(b = activeVertices.begin(), e = activeVertices.end(); b != e; ++b)
	{
		activeV.insert( *b );
	}
}
*/

/*!
Adds an edge if it doesn't already exist. If it already exists,
add the loop.
*/
/*
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
*/
/*!
Writes the graph to the specified DOT file.
The file will be overwritten if it already exists.
*/
void FCGraph::WriteDOT(const string &outfile)
{
	ofstream ofs(outfile.c_str(), ios::app);

	pLog->info("Writing %zu vertices %lu edges to \"%s\"...", num_vertices(*this), num_edges(*this), outfile.c_str());
	//write_graphviz(ofs, *this, dp, string("id"));
	write_graphviz(ofs, *this, //make_label_writer(vertexNamePropMap),
		FCVertexLabelWriter(
			vertexNamePropMap,
			vertexStartEAPropMap,
			vertexEndEAPropMap,
			vertexSizePropMap,
			vertexFuncIdPropMap,
			vertexCycCompPropMap,
			vertexNvarsPropMap,
			vertexNparamsPropMap,
			vertexNngramsPropMap,
			vertexLevelPropMap,
			vertexCompNumPropMap,
			vertexClusterPropMap,
			vertexWeightPropMap,
			vertexPairedPropMap ),
		FCEdgeLabelWriter(edgeWeightPropMap) );

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
/*
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
*/
/*!
Prints all vertices.
*/
void FCGraph::PrintVertices()
{
	pair<FCVertexItr, FCVertexItr> vp;
	pLog->info("File %u: Printing vertices...", GetGraphFileId());
	for(vp = vertices(*this); vp.first != vp.second; ++vp.first)
	{
		pLog->info("[%lu, %lu] %s",
			vertexIdxPropMap[*vp.first],
			vertexFuncIdPropMap[*vp.first],
			vertexNamePropMap[*vp.first].c_str());
	}
}

/*!
Prints all edges.
*/
void FCGraph::PrintEdges()
{
	pair<FCEdgeItr, FCEdgeItr> ep;
	pLog->info("File %u: Printing edge weights...", GetGraphFileId());
	for(ep = edges(*this); ep.first != ep.second; ++ep.first)
	{
		if( edgeWeightPropMap[*ep.first] == (float)1 ) continue;
		pLog->info("%f [%s -> %s]",
			edgeWeightPropMap[*ep.first],
			GetVertexName( source(*ep.first, *this) ).c_str(),
			GetVertexName( target(*ep.first, *this) ).c_str() );
	}
}

/*!
Updates name cost using Levenshtein edit distance.
*/
/*
int FCGraph::AddNameCost(gsl_matrix *pCostMatrix)
{
	pair<FCVertexItr, FCVertexItr> vp;
	//pLog->info("Updating name cost...");

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
		//pLog->info("edgecost = %.2f", edgecost);
	
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

const size_t FCGraph::GetVertexNumBBs(const FCVertex &V)
{
	vector<BasicBlock*> *bbs = vertexBBsPropMap[V];
	return ( bbs == NULL ? 0 : bbs->size() );
}
