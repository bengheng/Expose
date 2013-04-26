#include "graphgen.h"

#include <boost/graph/properties.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graphviz.hpp>

/*!
graphgen.cpp

Generates graphs incrementally for testing graph matching algorithms.
*/

/*!
Constructor.
*/
GraphGen::GraphGen(bool fAllowDupVert)
{
	this->fAllowDupVertex = fAllowDupVert;

	vertexNamePropMap = get(vertex_name, g);
	vertexIdxPropMap = get(vertex_index, g);
	edgeWeightPropMap = get(edge_weight, g);
	edgeIdxPropMap = get(edge_index, g);
}

/*!
Destructor.
*/
GraphGen::~GraphGen()
{
}

/*!
Finds the vertex with the specified vname.

Returns null_vertex() if it doesn't exist.
*/
GGVertex GraphGen::FindVertex(std::string vname)
{
	if(num_vertices(g) == 0)
		return graph_traits<GGGraph>::null_vertex();

	std::pair<GGVertexItr, GGVertexItr> vp;
	for(vp = vertices(g); vp.first != vp.second; ++vp.first)
	{
		if(vname.compare(vertexNamePropMap[*vp.first]) == 0)
			return *vp.first;	// Found
	}

	// Cannot find vertex
	return graph_traits<GGGraph>::null_vertex();
}

/*!
Returns a random vertex.
*/
GGVertex GraphGen::GetRandomVertex()
{
	int numvert = num_vertices(g);
	int rndvertidx;

	if(numvert == 0)
		return graph_traits<GGGraph>::null_vertex();

	// Generate a random vertex index
	rndvertidx = rand() % numvert;

	std::pair<GGVertexItr, GGVertexItr> vp;
	for(vp = vertices(g);
		vp.first != vp.second && rndvertidx > 0;
		++vp.first, --rndvertidx);

	// Cannot find vertex
	return *vp.first;
}

char GraphGen::GetRandomCharacter()
{
	char n = '?';
	int type;

	type = rand() % 100;
	if(type >= 0 && type < 5)		n = '.';
	else if(type >= 5 && type < 20)		n = '0'+rand()%10;
	else if(type >= 20 && type < 60)	n = '@'+rand()%27;
	else if(type >= 60 && type < 100)	n = 'a'+rand()%26;

	return n;
}

/*!
Adds a random vertex.
*/
int GraphGen::AddRandomVertex()
{
	std::string vname;
	int vnamelen;

	// Get random length
	vnamelen = 3 + (rand() % 29);

	// Generate a random name
	for(; vnamelen > 0; --vnamelen)
	{
		vname += GetRandomCharacter();
	}

	AddVertex(vname);
}

/*!
Adds a vertex with vname to the graph.
*/
int GraphGen::AddVertex(std::string vname)
{
	if(FindVertex(vname) != graph_traits<GGGraph>::null_vertex() && this->fAllowDupVertex == false)
		return 0;

	GGVertex v;
	v = add_vertex(g);
	vertexNamePropMap[v] = vname;

	return 0;
}

/*!
Adds an edge if it doesn't already exist. If it already exists, add the weight.
*/
int GraphGen::AddRandomEdge()
{
	int weight;
	GGVertex u, v;
	GGEdge e;
	bool res;

	// Generate a random weight
	weight = 1 + rand() % 4;

	// Get random vertices u and v
	if((u = GetRandomVertex()) == graph_traits<GGGraph>::null_vertex()) return -1;
	if((v = GetRandomVertex()) == graph_traits<GGGraph>::null_vertex()) return -1;

	// Add the edge if it doesn't already exist
	tie(e, res) = edge(u, v, g);
	if(res == false)
	{
		tie(e, res) = add_edge(u, v, g);
		if(res == true) edgeWeightPropMap[e] = weight;
		return 0;
	}

	edgeWeightPropMap[e] += weight;
	return 0;
}

/*!
Writes the graph to the specified DOT file.
*/
int GraphGen::WriteDOT(std::string outfile)
{
	std::ofstream ofs(outfile.c_str(), std::ios::app);

	write_graphviz(ofs, g, make_label_writer(vertexNamePropMap), make_label_writer(edgeWeightPropMap));
	ofs.flush();
	ofs.close();

	return 0;
}
