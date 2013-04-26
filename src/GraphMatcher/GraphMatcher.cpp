//#include "Graph.h"
#include "GraphMatcher.h"
#include "../fcg/fcg.h"
#include "../utils/utils.h"
#include "../gsl_hungarian/hungarian.h"
#include "asp.h"
#include <iomanip>
#include <map>
#include <mysql/mysql.h>
#include <float.h>

using namespace std;

/*!
Constructor for GraphMatcher. Reads G1 and G2 from specified dot files.
\param	G1DotName	Name of dot file for Graph 1
\param	G2DotName	Name of dot file for Graph 2
*/
GraphMatcher::GraphMatcher(const string &G1DotName, const string &G2DotName) throw (LMException)
{
	// pCost		= NULL;
	// pCostRelabel	= NULL;
	G1.pG = new FCGraph();
	if( G1.pG == NULL ) throw LMException("Out of mem allocating graph 1!\n");
	if(G1.pG->ReadDOT(G1DotName) == false) throw LMException("FATAL: Cannot create Graph 1!");
	G1.nMatched = 0;
	SortVertices(G1, G1Win);

	G2.pG = new FCGraph();
	if( G2.pG == NULL ) throw LMException("Out of mem allocating graph 2!\n");
	if(G2.pG->ReadDOT(G2DotName) == false) throw LMException("FATAL: Cannot create Graph 2!");;
	G2.nMatched = 0;
	SortVertices(G2, G2Win);
}

GraphMatcher::~GraphMatcher() throw()
{
	// if(pCost 	!= NULL)	gsl_matrix_free(pCost);
	// if(pCostRelabel != NULL)	gsl_matrix_free(pCostRelabel);
	if(G1.pG	!= NULL)	delete G1.pG;
	if(G2.pG 	!= NULL)	delete G2.pG;
}


void GraphMatcher::SortVertices(GRAPHINFO &G, WININFO &GWin)
{
	pair<FCVertexItr, FCVertexItr>	GVit;	// G vertex iterators

	for(GVit = vertices(*G.pG); GVit.first != GVit.second; ++GVit.first)
	{
		OrderedIns(G, *GVit.first, GWin);
	}
}

/*!
Inserts vertex ordered by address.
*/
void GraphMatcher::OrderedIns(GRAPHINFO &G, FCVertex v, WININFO &GWin)
{
	unsigned int newAddr;

	// Gets address of vertex
	newAddr = G.pG->GetVertexStart(v);
	FCVertVec::iterator b, e;
	for(b = GWin.VList.begin(), e = GWin.VList.end(); b != e; ++b)
	{
		unsigned int currAddr = G.pG->GetVertexStart(*b);
		if(currAddr > newAddr )
		{
			GWin.VList.insert(b, v);
			return;
		}
	}

	// Didn't manage to insert earlier, so must be the last one.
	GWin.VList.insert(b, v);
}

//======================================================================
// COMPUTES GRAPH DISTANCE
//======================================================================
float GraphMatcher::ComputeGraphDist(unsigned int nNeighborDepth)
{
	float minScore = 1;
	float score;
	int i, j;

	// Precompute pairwise node relabeling cost
	PrecompNormRelabel(G1, G2);

	// We always base the window size on G1
	G1Win.head = G1Win.VList.begin();
	G1Win.tail = G1Win.VList.end();
	G1Win.winsize = 0;
	FCVertVec::iterator b1 = G1Win.head;
	FCVertVec::iterator e1 = G1Win.tail;
	for(; b1 != e1; ++b1) G1Win.winsize++;

	// Precompute adjusted edge weights
	fG1EdgeWeightRange = ComputeEdgeWeightRange(G1, G1Win);
	G1.pG->PrecompAdjEdgeWt(fG1EdgeWeightRange);
	G2.pG->PrecompAdjEdgeWt(fG1EdgeWeightRange);

	// Preallocate discount matrices
	work.discounts = (int**) malloc(sizeof(int*)*G1Win.winsize);
	work.discountsLUT = (int**) malloc(sizeof(int*)*G1Win.winsize);
	for(i = 0; i < G1Win.winsize; ++i)
	{
		work.discounts[i] = (int*) malloc(sizeof(int)*G1Win.winsize);
		work.discountsLUT[i] = (int*) malloc(sizeof(int)*G1Win.winsize);
	}
	work.discountsR = G1Win.winsize;
	work.discountsC = G1Win.winsize;
	work.discountsLUTR = G1Win.winsize;
	work.discountsLUTC = G1Win.winsize;

	// Preallocate neighbor cost matrices
	int sumVertices = num_vertices(*G1.pG) + num_vertices(*G2.pG);
	work.NeighborCost = new cost*[sumVertices];
	work.NeighborCostCpy = new cost*[sumVertices];
	for(i = 0; i < sumVertices; ++i)
	{
		work.NeighborCost[i] = new cost[sumVertices];
		work.NeighborCostCpy[i] = new cost[sumVertices];
	}
	work.NeighborCostR = sumVertices;
	work.NeighborCostC = sumVertices;
	work.NeighborCostCpyR = sumVertices;
	work.NeighborCostCpyC = sumVertices;

	work.col_mate = new long[sumVertices];
	work.row_mate = new long[sumVertices];
	work.col_mate_len = sumVertices;
	work.row_mate_len = sumVertices;

	vector< FCVertVec::iterator > ComputeBoundaries;
	G2Win.head = G2Win.VList.begin();
	while(G2Win.head != G2Win.VList.end())
	{
		G1.nMatched = 0; G2.nMatched = 0;
		// Update tail
		for(i = 0, G2Win.tail = G2Win.head, G2Win.winsize = 0;
		i < G1Win.winsize && G2Win.tail != G2Win.VList.end(); ++i)
		{
			++G2Win.tail; ++G2Win.winsize;
		}

		#ifdef VERBOSE
		cout << "G2Winsize: " << G2Win.winsize << endl;
		#endif

		score = ComputeSubGraphDist(nNeighborDepth, G1, G1Win, G2, G2Win, work);
		if( score == minScore )		ComputeBoundaries.push_back(G2Win.head);
		else if( score < minScore )	minScore = score;

		// Move to next window
		G2Win.head = G2Win.tail;
	}

	// Compute boundaries
	for( i = 0; i < ComputeBoundaries.size(); ++i)
	{
		int halfWin = floor(((float)G1Win.winsize)/2);
	
		//
		// Compute LEFT boundary
		//
	
		G2Win.head = ComputeBoundaries[i];
		for(j = 0; j < halfWin && G2Win.head != G2Win.VList.begin(); ++j)
			G2Win.head--;
		
		G1.nMatched = 0; G2.nMatched = 0;
		// Update tail
		for(j = 0, G2Win.tail = G2Win.head, G2Win.winsize = 0;
		j < G1Win.winsize && G2Win.tail != G2Win.VList.end(); ++j)
		{
			++G2Win.tail; ++G2Win.winsize;
		}
		score = ComputeSubGraphDist(nNeighborDepth, G1, G1Win, G2, G2Win, work);
		if( score < minScore )	minScore = score;

		//
		// Compute RIGHT boundary
		//

		G2Win.head = ComputeBoundaries[i];
		for(j = 0; j < halfWin && G2Win.head != G2Win.VList.end(); ++j)
			G2Win.head++;

		G1.nMatched = 0; G2.nMatched = 0;
		// Update tail
		for(j = 0, G2Win.tail = G2Win.head, G2Win.winsize = 0;
		j < G1Win.winsize && G2Win.tail != G2Win.VList.end(); ++j)
		{
			++G2Win.tail; ++G2Win.winsize;
		}
		score = ComputeSubGraphDist(nNeighborDepth, G1, G1Win, G2, G2Win, work);
		if( score < minScore )	minScore = score;
	}

	for(i = 0; i < G1Win.winsize; ++i)
	{
		free(work.discounts[i]);
		free(work.discountsLUT[i]);
	}
	free(work.discounts);
	free(work.discountsLUT);

	for(i = 0; i < sumVertices; ++i)
	{
		delete [] work.NeighborCost[i];
		delete [] work.NeighborCostCpy[i];
	}
	delete [] work.NeighborCost;
	delete [] work.NeighborCostCpy;

	delete [] work.col_mate;
	delete [] work.row_mate;

	return minScore;
}

/*!
Computes distance between graphs.
*/
float GraphMatcher::ComputeSubGraphDist(
	unsigned int nNeighborDepth,
	GRAPHINFO &G1,
	WININFO &G1Win,
	GRAPHINFO &G2,
	WININFO &G2Win,
	WORKSPACE &work)
{
	FCVertVec::iterator bA = G1Win.head;
	FCVertVec::iterator eA = G1Win.tail;
	for(; bA != eA; ++bA)
	{
		G1.VerticesMap[*bA] = FCG_NULL_VERTEX;
	}
	FCVertVec::iterator bB = G2Win.head;
	FCVertVec::iterator eB = G2Win.tail;
	for(; bB != eB; ++bB)
	{
		G2.VerticesMap[*bB] = FCG_NULL_VERTEX;
	}

	int matched_node_relabeling_cost = FindPairwiseNodeMatches(G1, G1Win, G2, G2Win);

	if( G1Win.winsize == 0 || G2Win.winsize == 0)
		return 1;

	// There are no unmatched nodes if number matched is the same as the size
	// of the VerticesMaps, and they are greater than 0.
	// We cannot consider graphs with 0 nodes as matched.
	if(G1Win.winsize == G1.nMatched || G2Win.winsize == G2.nMatched)
	{	// No unmatched nodes.
		// Means we matched everything!
		return 0;
	}

	// Allocate cost matrices
	int G1UnmatchedCount = G1Win.winsize - G1.nMatched; 
	#ifdef VERBOSE
	cout << "G1 Win Size: " << G1Win.winsize << "\tMatched: " << G1.nMatched << "\tUnmatched: " << G1UnmatchedCount << endl;
	//FCVertMap::iterator it;
	int count = 0;
	FCVertVec::iterator b1 = G1Win.head;
	FCVertVec::iterator e1 = G1Win.tail;
	//for(it = G1.VerticesMap.begin(); it != G1.VerticesMap.end(); ++it)
	for(; b1 != e1; ++b1)
	{
		if( G1.VerticesMap[*b1] != FCG_NULL_VERTEX )
		{
			cout << "Prior match in G1.\n";
			continue;
		}
		count++;
	}
	cout << "G1 unmatched: " << count << endl;
	#endif

	int G2UnmatchedCount = G2Win.winsize - G2.nMatched; 
	#ifdef VERBOSE
	cout << "G2 Win Size: " << G2Win.winsize << "\tMatched: " << G2.nMatched << "\tUnmatched: " << G2UnmatchedCount << endl;
	count = 0;
	FCVertVec::iterator b2 = G2Win.head;
	FCVertVec::iterator e2 = G2Win.tail;
	//for(it = G2.VerticesMap.begin(); it != G2.VerticesMap.end(); ++it)
	for(; b2 != e2; ++b2)
	{
		if( G2.VerticesMap[*b2] != FCG_NULL_VERTEX ) continue;
		count++;
	}
	cout << "G2 unmatched: " << count << endl;
	#endif
	int costDim = G1UnmatchedCount + G2UnmatchedCount; // Cost matrix dimension
	

	#ifdef VERBOSE
	
	
	cout << "costDim: " << costDim << endl;
	#endif

	// Allocate cost matrix
	cost **pCost;
	pCost = new cost*[costDim];
	if(pCost == NULL)
	{
		cout << "ERROR ALLOCATING MEM!" << endl;
		exit(-4);
	}
	for(int i = 0; i < costDim; ++i)
	{
		pCost[i] = new cost[costDim];
		if( pCost[i] == NULL )
		{
			cout << "ERROR ALLOCATING MEM!" << endl;
			exit(-4);
		}		
	}
	for(int i = 0; i < costDim; ++i)
		for(int j = 0; j < costDim; ++j)
			pCost[i][j] = 0;

	FillGlobalCostMatrix(G1, G1Win, G2, G2Win, nNeighborDepth, pCost, costDim, work);

	//
	// Apply Hungarian
	//

	long *row_mate = new long[costDim];
	if( row_mate == NULL ) exit(-4);
	long *col_mate = new long[costDim];
	if( col_mate == NULL ) exit(-4);

	// Calls Hungarian algorithm
	asp(costDim, pCost, col_mate, row_mate);

	// Free cost matrix
	for(int i = 0; i < costDim; ++i) 
		delete [] pCost[i];
	delete [] pCost;


	//
	// Compute final score
	//
	#ifdef VERBOSE
	for(int i = 0; i < costDim; ++i)
		cout << col_mate[i] << " ";
	cout << endl;
	#endif
	CombineMatches(G1, G1Win, G2, G2Win, col_mate);
	float finalScore = ComputeTotalCost(G1, G1Win, G2, G2Win, matched_node_relabeling_cost);

	delete [] row_mate;
	delete [] col_mate;

	return finalScore;
}

/*!
Combine match results from the initial computations based on labels and neighbors
with results from Hungarian.

After FindPairwiseNodeMatches, unmatched nodes were paired with FCG_NULL_VERTEX.
We applied Hungarian on these nodes. Now, we update the pairing for these
unmatched nodes using results from the Hungarian algorithm.

\note We only need to do this for G1, since we are only interested in finding out
how much of G1 is in G2, and not interested in total edit cost between the 2 graphs.
*/
void GraphMatcher::CombineMatches(
	GRAPHINFO &G1,
	WININFO &G1Win,
	GRAPHINFO &G2,
	WININFO &G2Win,
	long *col_mate)
{
	int i = 0;
	#ifdef VERBOSE
	int tmp = 0;
	#endif
	//FCVertMap::iterator it;
	FCVertVec::iterator b1 = G1Win.head;
	FCVertVec::iterator e1 = G1Win.tail;
	//for(it = G1.VerticesMap.begin(); it != G1.VerticesMap.end(); ++it)
	for(; b1 != e1; ++b1)
	{
		// Skip if node has already been matched
		if(G1.VerticesMap[*b1] != FCG_NULL_VERTEX) 
		{
			#ifdef VERBOSE
			cout << "Skipping " << G1.pG->GetVertexName(*b1) << " already matched to " << G2.pG->GetVertexName(G1.VerticesMap[*b1]) << endl;
			#endif
			continue;
		}
		//if(it->second != FCG_NULL_VERTEX) continue;

		// If matched in 1st Quad, then we update.
		// Otherwise it's in 2nd Quad (deletion),
		// so we just leave it as FCG_NULL_VERTEX

		if(col_mate[i] < G2Win.winsize)
		{
			G1.VerticesMap[*b1] = GetUnmatchedByIndex(G2, G2Win, col_mate[i]);
			#ifdef VERBOSE
			cout << "Assigning " << G1.pG->GetVertexName(*b1) << " to " << G2.pG->GetVertexName(G1.VerticesMap[*b1]) << endl;
			#endif
			//it->second = GetUnmatchedByIndex(G2, G2Win, i);

 
			#ifdef VERBOSE
			tmp++;
			#endif
		}
	
		++i;	// Next row
	}
	#ifdef VERBOSE
	cout << "Combined " << tmp << endl;
	#endif
}

/*!
Returns unmatched vertex by index.
*/
FCVertex GraphMatcher::GetUnmatchedByIndex(GRAPHINFO &G, WININFO &GWin, unsigned int index)
{
	int i = 0;
	FCVertMap::iterator it;
	//for(it = G.VerticesMap.begin(); it != G.VerticesMap.end(); ++it)
	FCVertVec::iterator b1 = GWin.head;
	FCVertVec::iterator e1 = GWin.tail;
	for(; b1 != e1; ++b1)
	{
		if(G.VerticesMap[*b1] != FCG_NULL_VERTEX) continue;
		//if(it->second != FCG_NULL_VERTEX) continue;
		//if(i == index) return it->first;	// Found
		if(i == index) return *b1;
		++i;
	}

	return FCG_NULL_VERTEX;
}


/*!
Computes total cost to measure how much of G1 matches with G2.

We only look at the nodes in G1 that are either matched or need to be deleted.
*/
float GraphMatcher::ComputeTotalCost(
	GRAPHINFO &G1,
	WININFO &G1Win,
	GRAPHINFO &G2,
	WININFO &G2Win,
	int matched_node_relabeling_cost)
{
	int nMatchedEdgesCount = 0;
	FCVertVec		vecDelNodes;
	float			fTotalEdgeCost = 0;
	float			fTotalRelabelCost = 0;

	//FCVertMap::iterator it;


	#ifdef VERBOSE
	int numDel = 0;
	int numSub = 0;
	#endif

	// First, compute node del cost
	//for(it = G1.VerticesMap.begin(); it != G1.VerticesMap.end(); ++it)
	FCVertVec::iterator b1 = G1Win.head;
	FCVertVec::iterator e1 = G1Win.tail;
	for(; b1 != e1; ++b1)
	{
		// ComputeNodeEditCost(G1, it->first, G2, it->second,
		//	vecDelNodes,
		//	&fTotalEdgeCost,
		//	&fTotalRelabelCost);
		//if(it->first != FCG_NULL_VERTEX && it->second == FCG_NULL_VERTEX)
		if(*b1 != FCG_NULL_VERTEX && G1.VerticesMap[*b1] == FCG_NULL_VERTEX)
		{
			//FCVertex N1 = it->first;
			FCVertex N1 = *b1;
			ComputeNodeDelCost(
				G1, N1,
				vecDelNodes,
				&fTotalEdgeCost,
				&fTotalRelabelCost);
	
			#ifdef VERBOSE
			numDel++;
			#endif
		}
	}

	// Second, compute node substitution cost
	b1 = G1Win.head;
	e1 = G1Win.tail;
	//for(it = G1.VerticesMap.begin(); it != G1.VerticesMap.end(); ++it)
	for(; b1 != e1; ++b1)
	{
		//if(it->first != FCG_NULL_VERTEX && it->second != FCG_NULL_VERTEX)
		if(*b1 != FCG_NULL_VERTEX && G1.VerticesMap[*b1] != FCG_NULL_VERTEX)
		{
			//FCVertex N1 = it->first;
			//FCVertex N2 = it->second;
			FCVertex N1 = *b1;
			FCVertex N2 = G1.VerticesMap[*b1];
			
			//cout << "Computing subcost for " << G1.pG->GetVertexName(N1) << " and " << G2.pG->GetVertexName(N2) << endl;
			ComputeNodeSubCost(
				G1, N1, G2, N2,
				vecDelNodes,
				&fTotalEdgeCost,
				&fTotalRelabelCost);

			#ifdef VERBOSE
			numSub++;
			#endif
		}
	}

	// cout << "Vertices Map Size: " << G1.VerticesMap.size() << endl;
	// cout << "Total Relabel Cost: " << fTotalRelabelCost << endl;

	// matched_node_relabeling_cost was computed in values of 1.
	// We want to use 0.5 so that we are not biased.
	fTotalRelabelCost += ((float) matched_node_relabeling_cost)/2;


	#ifdef VERBOSE
	cout << "numDel: " << numDel << " numSub: " << numSub << endl;
	cout << "Total Edge Cost: " << fTotalEdgeCost << " Num Edges: "<< num_edges(*G1.pG) << endl;
	cout << "Total Relb Cost: " << fTotalRelabelCost << " Num Verts: " << num_vertices(*G1.pG) << endl;
	#endif

	float fNormEdgeCost = fTotalEdgeCost/num_edges(*G1.pG);
	float fNormRelabelCost = fTotalRelabelCost/num_vertices(*G1.pG);

	#ifdef VERBOSE
	cout << "Norm Edge Cost: " << fNormEdgeCost << endl;
	cout << "Norm RelabelCost: " << fNormRelabelCost << endl;
	#endif

	// TODO: Divide by 2?
	//return (fNormEdgeCost + fNormRelabelCost)/2;
	return (0.5*fNormEdgeCost + 0.5*fNormRelabelCost);
}

/*!
Computes the node edit cost between a node in G1 and another in G2.

If both N1 and N2 are not FCG_NULL_VERTEX, we compute the cost of the
common edges, then take the total cost of all edges minus 2 times
the cost of common edges. We also need to add the node relabeling cost,
normalized to between 0 and 1. If both are "sub_xyz" nodes, we just
assume they have relabeling cost 0.5.

If N1 is not FCG_NULL_VERTEX and N2 is, then the cost for N1 deletion
is computed. When deleting N1, all the incoming and outgoing edges of N1
are considered. Before deleting each edge, we first check if the source
or target (depending if it's an incoming or outgoing edge respectively)
has already been deleted. If so, don't add the edge deletion cost, since
it has already been deleted. Otherwise, add the edge deletion cost.

TODO: WE DON'T CONSIDER INSERTION OF N2!!!
If N1 is FCG_NULL_VERTEX and N2 is not, then the cost for N2 insertion
is computed. When inserting N2, we won't know what the new sources and
targets are. But we'll consider them. One implication is that we may
double count an edge insertion cost, if say later on, we need to insert
another node that has exactly the same edge.

Note we cannot reuse the same algorithm as when we compute the neighbor
edit cost, because then we'll be double counting a lot of edges and
nodes. Moreover, that was for a result over some neighborhood, while
we require to look at each node individually.
*/
//void GraphMatcher::ComputeNodeEditCost(
//	GRAPHINFO &G1, FCVertex N1,
//	GRAPHINFO &G2, FCVertex N2,
//	FCVertVec &vecDelNodes,
//	float *pfTotalEdgeCost,
//	float *pfTotalRelabelCost)
//{
//	if(N1 != FCG_NULL_VERTEX && N2 != FCG_NULL_VERTEX)
//		ComputeNodeSubCost(G1, N1, G2, N2, vecDelNodes, pfTotalEdgeCost, pfTotalRelabelCost);
//	else if(N1 != FCG_NULL_VERTEX && N2 == FCG_NULL_VERTEX)
//		ComputeNodeDelCost(G1, N1, vecDelNodes, pfTotalEdgeCost, pfTotalRelabelCost);
	//else if(N1 == FCG_NULL_VERTEX && N2 != FCG_NULL_VERTEX)
	//	fNodeEditCost = ComputeNodeInsCost(G2, N2);

//}

/*!
 * Returns mean of values in vector.
 * */
float GraphMatcher::ComputeMean( vector<float> &vec )
{
	float sum = 0;

	vector<float>::iterator it;
	for( it = vec.begin(); it != vec.end(); ++it )
	{
		sum += *it;
	}

	return (sum / vec.size());
}

/*!
 * Returns standard deviation.
 * */
float GraphMatcher::ComputeStdDev( vector<float> &vec, float mean )
{
	float variance = 0;

	vector<float>::iterator it;
	for( it = vec.begin(); it != vec.end(); ++it )
	{
		variance += ((*it-mean)*(*it-mean));
	}

	variance /= (vec.size()-1);

	return sqrt(variance);
}


/*!
 * Returns similarity score, computed by summing up the
 * similarity score for each matched edge.
 *
 * A similarity score is computed by finding the standardized
 * edge weight delta score Z. The similarilty score is then
 * 1 - Z/maxZ.
 * */
float GraphMatcher::ComputeSimilarityScore(vector<float> &vec, float mean, float stddev)
{
	vector<float> vecZ;	// Standardized distance for each edge
	float maxZ = 0;
	vector<float>::iterator it;

	//cout << "stddev: " << stddev << endl;
	for( it = vec.begin(); it != vec.end(); ++it )
	{
		float Z = abs(*it - mean)/stddev;
		vecZ.push_back(Z);

		// cout << "Z: " << Z << endl;
		if( Z > maxZ ) maxZ = Z;
	}

	float similarityScore = 0;
	for( it = vecZ.begin(); it != vecZ.end(); ++it )
	{
		float score = 0;
		if( maxZ == 0)	score = 1;
		else		score = (1 - (*it/maxZ) );
	
		// cout << "Edge Similarity Score: " << score << endl;
		similarityScore += score;
	}

	return similarityScore;
}

void GraphMatcher::ComputeNodeSubCost(
	GRAPHINFO &G1,
	FCVertex &N1,
	GRAPHINFO &G2,
	FCVertex &N2,
	FCVertVec &vecDelNodes,
	float *pfTotalEdgeCost,
	float *pfTotalRelabelCost)
{
	//int nCommonEdgesCount = 0;


	//cout << "Computing Node Sub Cost (" << G2.pG->GetVertexName(N2) << ", " << G1.pG->GetVertexName(N1) << ")" << endl;

	//
	// Compute cost of common edges. An edge is common if
	// their source and target match, based on the assignment.
	//

	// Incoming edges

	// For each in-edge of N1...
	/*
	pair<FCInEdgeItr, FCInEdgeItr> in1;
	for(in1 = in_edges(N1, *G1.pG); in1.first != in1.second; ++in1.first)
	{
		FCVertex src1 = source(*in1.first, *G1.pG);
		FCVertex matchedSrc1 = G1.VerticesMap[src1];

		// Continue if src1 doesn't match with anything
		if(matchedSrc1 == FCG_NULL_VERTEX) continue;
		
		// For each in-edge of N2...
		pair<FCInEdgeItr, FCInEdgeItr> in2;
		for(in2 = in_edges(N2, *G2.pG); in2.first != in2.second; ++in2.first)
		{
			FCVertex src2 = source(*in2.first, *G2.pG);

			// Continue if src2 doesn't match with anything
			if(G2.VerticesMap[src2] == FCG_NULL_VERTEX) continue;

			if(matchedSrc1 == src2)
			{	// The matched source of N1 is also a source
				// of N2. Same edge.

				// src2 should also match with src1
				assert(src1 == G2.VerticesMap[src2]);
				nCommonEdgesCount++;
				break;
			}	
		}
	}
	*/


	//cout << "Computing node sub cost between " << G1.pG->GetVertexName(N1) << " and " << G2.pG->GetVertexName(N2) << endl;
	int excludeDel = 0;

	vector<float> matchedEdgeWtDelta;

	// Outgoing edges	
	pair<FCOutEdgeItr, FCOutEdgeItr> out1;
	// For each out-edge of N1...
	for(out1 = out_edges(N1, *G1.pG); out1.first != out1.second; ++out1.first)
	{
		FCVertex tgt1 = target(*out1.first, *G1.pG);
		FCVertex matchedTgt1 = G1.VerticesMap[tgt1];

		// Continue if tgt1 doesn't match with anything,
		// or if it has already been deleted previously.
	
		if( find(vecDelNodes.begin(), vecDelNodes.end(), tgt1) != vecDelNodes.end() )
		{
			++excludeDel;
			continue;
		}

		if(matchedTgt1 == FCG_NULL_VERTEX)
		{
			#ifdef VERBOSE
			cout << "Skipping NULL tgt\n";
			#endif
			continue;
		}

		#ifdef VERBOSE
		cout << G1.pG->GetVertexName(tgt1) << endl;
		#endif

		// For each out-edge of N2...
		pair<FCOutEdgeItr, FCOutEdgeItr> out2;
		for(out2 = out_edges(N2, *G2.pG); out2.first != out2.second; ++out2.first)
		{
			FCVertex tgt2 = target(*out2.first, *G2.pG);

			// Continue if tgt2 doesn't match with anything,
			// or if it has already been deleted previously.
			//if( G2.VerticesMap[tgt2] == FCG_NULL_VERTEX)
			//{
			//	cout << "Skipping no match\n";
			//	continue;
			//}
			if( find( vecDelNodes.begin(), vecDelNodes.end(), tgt2) != vecDelNodes.end() )
			{
				//cout << "Skipping deleted\n";
				continue;
			}


			//cout << "\t" << G2.pG->GetVertexName(matchedTgt1) << "\t" << G2.pG->GetVertexName(tgt2) << endl;
			if(matchedTgt1 == tgt2)
			{	// The matched target of N1 is also a target
				// of N2. Same edge.

				//cout << "Common edge "<< endl;

				// tgt2 should also match with tgt1
				//assert(tgt1 == G2.VerticesMap[tgt2]);
				//nCommonEdgesCount++;

				matchedEdgeWtDelta.push_back( abs(
					G1.pG->GetEdgeWeight(*out1.first) -
					G2.pG->GetEdgeWeight(*out2.first)));

				break;
			}
			
		}
	}

	float similarityScore = 0;
	float fDiffEdgesScore = 1;

	// Check corner case where both nodes have 0 edges.
	int degree1 = in_degree(N1, *G1.pG) + out_degree(N1, *G1.pG);
	int degree2 = in_degree(N2, *G2.pG) + out_degree(N2, *G2.pG);
	if( degree1 == 0 && degree2 == 0)
	{
		fDiffEdgesScore = 0;
	}
	else if( matchedEdgeWtDelta.size() == 0)
	{
		// 0 edge matched
		fDiffEdgesScore = 0;//out_degree(N1, *G1.pG);
	}
	else if( matchedEdgeWtDelta.size() == 1 )
	{
		// Only 1 edge weight delta...
		fDiffEdgesScore = out_degree(N1, *G1.pG) - excludeDel - 1;
		#ifdef VERBOSE
		cout << "DEBUG outdeg: " << out_degree(N1, *G1.pG) << " exclDel: " << excludeDel << endl;
		#endif
	}
	// If there are matched edges, we compute the similarity score.
	// If there are no matches, the similarity score is 0.
	else// if( matchedEdgeWtDelta.size() > 1 )
	{
		float mean = ComputeMean(matchedEdgeWtDelta);
		if(mean != 0)
		{
			float stddev = ComputeStdDev(matchedEdgeWtDelta, mean);
			#ifdef VERBOSE
			cout << "matchedEdgeWtDelta.size: " << matchedEdgeWtDelta.size() << " mean: " << mean << " stddev: " << stddev << endl;
			#endif
			similarityScore = ComputeSimilarityScore(matchedEdgeWtDelta, mean, stddev);
		}

		#ifdef VERBOSE
		cout << "mean: " << mean << " similarityScore: " << similarityScore << endl;
		#endif
		fDiffEdgesScore = out_degree(N1, *G1.pG) - excludeDel - similarityScore;
	}
	assert(fDiffEdgesScore >= 0);
	#ifdef VERBOSE
	cout << "fDiffEdgesScore: " << fDiffEdgesScore << endl;
	#endif
	
	//cout << "Similarity Score: " << similarityScore << endl;

	//int nTotalEdgeCost =
	//	(/*in_degree(N1, *G1.pG) +*/ out_degree(N1, *G1.pG)) +
	//	(/*in_degree(N2, *G2.pG) +*/ out_degree(N2, *G2.pG));
	
	//int nDiffEdgesCount = nTotalEdgeCost - 2*nCommonEdgesCount;

	// Normalize nDiffEdgesCount over number of edges in G1 because
	// we want to measure how much of G1 is in G2.
	// This makes sense since if there is only a few different edges
	// over a large number of total edges, then the cost should be low.
	//
	// Actually, we should multiply both denominator and numerator
	// by COST_DELEDGE and COST_INSEDGE. But since it cancels out,
	// we leave it. Moreover, we don't know which of the edges
	// should be deleted and which ones inserted.

	// TODO: Don't normalize here. We'll normalize after
	// adding total number of different edges.
	*pfTotalEdgeCost += fDiffEdgesScore; // / num_edges(*G1.pG);

	//
	// Compute relabeling cost
	//

	#ifdef VERBOSE
	cout << "Vertex Matched: " << G1.pG->GetVertexName(N1) << ", " << G2.pG->GetVertexName(N2) << " " << fDiffEdgesScore << endl;
	#endif
	float fNormRelabelCost = ComputeNormRelabelCost(
		G1.pG->GetVertexName(N1),
		G2.pG->GetVertexName(N2),
		true);	// Use precomputed value where available

	*pfTotalRelabelCost += fNormRelabelCost;
	
	// TODO: Should we normalize by 2? NO! We cannot do this because
	// then we'll  mix the information about the edge cost
	// with that of the relabel cost.
	//return (fNormEdgeCost + fNormRelabelCost)/2;
}

void GraphMatcher::ComputeNodeDelCost(
	GRAPHINFO &G, FCVertex &N,
	FCVertVec &vecDelNodes,
	float *pfTotalEdgeCost,
	float *pfTotalRelabelCost)
{
	int numDelEdges = 0;

	// For each in-edge of N...
	pair<FCInEdgeItr, FCInEdgeItr> in;
	for(in = in_edges(N, *G.pG); in.first != in.second; ++in.first)
	{
		FCVertex src = source(*in.first, *G.pG);

		// If src has already been deleted previously,
		// skip, so that we don't double count the edge
		// deletion cost.
		if( find(vecDelNodes.begin(), vecDelNodes.end(), src) != vecDelNodes.end() )
			continue;

		numDelEdges++;
	}

	// Outgoing edges
	pair<FCOutEdgeItr, FCOutEdgeItr> out;
	for(out = out_edges(N, *G.pG); out.first != out.second; ++out.first)
	{
		FCVertex tgt = target(*out.first, *G.pG);

		// If tgt has already been deleted previously,
		// skip, so that we don't double count the edge
		// deletion cost.
		if( find(vecDelNodes.begin(), vecDelNodes.end(), tgt) != vecDelNodes.end() )
			continue;

		numDelEdges++;		
	}

	vecDelNodes.push_back(N);

	#ifdef VERBOSE
	cout << "DEBUG numDelEdges: " << numDelEdges << endl;
	#endif

	*pfTotalEdgeCost += numDelEdges;
	*pfTotalRelabelCost += 1;
}

//float GraphMatcher::ComputeNodeInsCost(GRAPHINFO &G2, FCVertex &N2)
//{
//}

//======================================================================
// FINDS PAIRWISE NODE MATCHES
//======================================================================

/*!
Finds pairwise node matches between nodes from the two input graphs.

\param	G1	Info for first graph.
\param	G2	Info for second graph.

\return	Total node re-labeling cost.
*/
int GraphMatcher::FindPairwiseNodeMatches(
	GRAPHINFO &G1,
	WININFO &G1Win,
	GRAPHINFO &G2,
	WININFO &G2Win)
{
	int relabeling_cost = 0;
	pair<FCVertexItr, FCVertexItr> G1Vit;	// G1 vertex iterators
	pair<FCVertexItr, FCVertexItr> G2Vit;	// G2 vertex iterators


	//
	// Insert all nodes into unmatched set
	//
	//FCVertList::iterator b1 = G1Win.head;
	//FCVertList::iterator e1 = G1Win.tail;
	//for(; b1 != e1; ++b1)
	for(G1Vit = vertices(*G1.pG); G1Vit.first != G1Vit.second; ++G1Vit.first)
		G1.VerticesMap[*G1Vit.first] = FCG_NULL_VERTEX;
		//G1.VerticesMap[*b1] = FCG_NULL_VERTEX;

	//FCVertList::iterator b2 = G2Win.head;
	//FCVertList::iterator e2 = G2Win.tail;
	//for(; b2 != e2; ++b2)
	for(G2Vit = vertices(*G2.pG); G2Vit.first != G2Vit.second; ++G2Vit.first)
		G2.VerticesMap[*G2Vit.first] = FCG_NULL_VERTEX;
		//G2.VerticesMap[*b2]= FCG_NULL_VERTEX;

	//
	// First, look for nodes with names that match,
	// except those that begin with "sub_".
	//
	ComputeLabelMatches(G1, G1Win, G2, G2Win);	

	//
	// Now we match remaining functions by constructing their corresponding function
	// signatures. If two functions only invoke matched functions and have the same
	// set of matched functions (by name), then we consider them to be matched!
	//
	relabeling_cost = ComputeTargetMatches(G1, G1Win, G2, G2Win);

	//G1.m_uUnmatchedNodeCount = G1.m_oUnmatchedNodeSet.size();
	//G2.m_uUnmatchedNodeCount = G2.m_oUnmatchedNodeSet.size();	

	#ifdef VERBOSE
	ofstream ofmatch("graphmatch.txt");

	// output results for testing
	ofmatch << "size of G1 Matched Node: " << G1.nMatched/*G1.m_oMatchedNodeSet.size()*/ << endl;
	ofmatch << "size of G2 Matched Node: " << G2.nMatched/*G2.m_oMatchedNodeSet.size()*/ << endl;

	int winsize1 = 0;
	ofmatch << "size of G1 UnMatched Node: " << G1Win.winsize - G1.nMatched/*G1.m_oUnmatchedNodeSet.size()*/ << endl;

	int winsize2 = 0;
	ofmatch << "size of G2 UnMatched Node: " << G2Win.winsize - G2.nMatched/*G2.m_oUnmatchedNodeSet.size()*/ << endl;
	
	ofmatch << "Matched cases: " << endl;
	//b1 = G1Win.head;
	for(FCVertMap::iterator it = G1.VerticesMap.begin(); it != G1.VerticesMap.end(); ++it)
	//for(; b1 != e1; ++b1)
	{
		//if(G1.VerticesMap[*b1] != FCG_NULL_VERTEX)
		if((*it).second != FCG_NULL_VERTEX)
			ofmatch << G1.pG->GetVertexName(/**b1*/it->first) << " "<< G2.pG->GetVertexName(/*G1.VerticesMap[*b1]*/it->second) << endl;
	}

	ofmatch << endl << "UnMatched functions for G1: " << endl;
	//b1 = G1Win.head;
	for(FCVertMap::iterator it = G1.VerticesMap.begin(); it != G1.VerticesMap.end(); ++it)
	//for(; b1 != e1; ++b1)
	{
		//if(G1.VerticesMap[*b1] == FCG_NULL_VERTEX)
		if((*it).second == FCG_NULL_VERTEX)
			ofmatch << G1.pG->GetVertexName(/**b1*/it->first) << endl;
	}

	ofmatch << endl << "UnMatched functions for G2: " << endl;
	//b2 = G2Win.head;
	for(FCVertMap::iterator it = G2.VerticesMap.begin(); it != G2.VerticesMap.end(); ++it)
	//for(; b2 != e2; ++b2)
	{
		//if(G2.VerticesMap[*b2] == FCG_NULL_VERTEX)
		if((*it).second == FCG_NULL_VERTEX)
			ofmatch << G2.pG->GetVertexName(/**b2*/it->first) << endl;
	}

	ofmatch.close();
	#endif

	//cerr << "Matched funcs by symbolic name and heuristic: " << G1.nMatched << endl;
	//cerr << "relabeling cost for matched nodes: " << relabeling_cost << endl;

	return relabeling_cost;
}

/*!
Performs pairwise comparison between labels for all nodes in G1 and G2.
*/
void GraphMatcher::ComputeLabelMatches(
	GRAPHINFO &G1,
	WININFO &G1Win,
	GRAPHINFO &G2,
	WININFO &G2Win)
{
	FCVertPair	P1, P2;

	FCVertVec::iterator b1 = G1Win.head;
	FCVertVec::iterator e1 = G1Win.tail;
	//for(FCVertMap::iterator it1 = G1.VerticesMap.begin();
	//it1 != G1.VerticesMap.end(); ++it1)
	for(; b1 != e1; ++b1)
	{
		//if( it1->second != FCG_NULL_VERTEX ) continue;
		if(G1.VerticesMap[*b1] != FCG_NULL_VERTEX) continue;

		string strLabel1 = G1.pG->GetVertexName(*b1);

		// Skip if using IDA function name for unknown functions
		if( strLabel1.compare(0, 4, "sub_") == 0 ) continue;


		FCVertVec::iterator b2 = G2Win.head;
		FCVertVec::iterator e2 = G2Win.tail;
		//for(FCVertMap::iterator it2 = G2.VerticesMap.begin();
		//it2 != G2.VerticesMap.end(); ++it2)
		for(; b2 != e2; ++b2)
		{
			//if(it2->second != FCG_NULL_VERTEX) continue;
			if(G2.VerticesMap[*b2] != FCG_NULL_VERTEX) continue;

			string strLabel2 = G2.pG->GetVertexName(*b2);

			// Skip if using IDA function name for unknown functions
			if( strLabel2.compare(0, 4, "sub_") == 0 ) continue;

			if( strLabel1.compare(strLabel2) == 0 )
			{
				G1.VerticesMap[*b1] = *b2; G1.nMatched++;
				G2.VerticesMap[*b2] = *b1; G2.nMatched++;
				//it1->second = it2->first; G1.nMatched++;
				//it2->second = it1->first; G2.nMatched++;
				
				//if( FoundInWindow(G2, G2Win, *b2) == false )
				//	G2Win.winsize++;
				//if( FoundInWindow(G1, G1Win, *b1) == false )
				//	G1Win.winsize++;
			}
		}
	}

}


/*!
Computes matches by checking for matching targers.

If two nodes have the same set of targets, but different labels,
we increment the labelling cost.
*/
int GraphMatcher::ComputeTargetMatches(
	GRAPHINFO &G1,
	WININFO &G1Win,
	GRAPHINFO &G2,
	WININFO &G2Win)
{
	int relabeling_cost = 0;
	bool doRematch;


	do
	{
		doRematch = false;

		// For all unmatched nodes in G1...
		FCVertVec::iterator b1 = G1Win.head;
		FCVertVec::iterator e1 = G1Win.tail;
		//for( FCVertMap::iterator it1 = G1.VerticesMap.begin();
		//it1 != G1.VerticesMap.end(); ++it1 )
		for( ; b1 != e1; ++b1 )
		{
			if(G1.VerticesMap[*b1] != FCG_NULL_VERTEX) continue;

			FCVertex N1 = *b1;

			// If N1 has no neighbors or has a
			// neighbor that is unmatched, continue.
			if(out_degree(N1, *G1.pG) == 0 ||
			HasUnMatchedDestNode(G1, G1Win, N1) == true )
				continue;

			// For all unmatched nodes in G2...
			FCVertVec::iterator b2 = G2Win.head;
			FCVertVec::iterator e2 = G2Win.tail;
			//for( FCVertMap::iterator it2 = G2.VerticesMap.begin();
			//it2 != G2.VerticesMap.end(); ++it2 )
			for( ; b2 != e2; ++b2 )
			{
				if(G2.VerticesMap[*b2] != FCG_NULL_VERTEX) continue;

				FCVertex N2 = *b2;

				// If N2 has no neighbors, or it does not have
				// the same out-degree as node1, or it has a
				// neighbor that is unmatched, continue.
				if(out_degree(N2, *G2.pG) == 0 ||
				out_degree(N1, *G1.pG) != out_degree(N2, *G2.pG) ||
				HasUnMatchedDestNode(G2, G2Win, N2) == true )
					continue;

				// Now both N1 and N2's targets contain only matched nodes.
				// Let's match them up if they have exactly the same set of
				// matched nodes. Remember the number of neighbors are the same.

				bool all_node_match = true;
				pair<FCOutEdgeItr, FCOutEdgeItr> n1OutEdges = out_edges(N1, *G1.pG);
				for(; n1OutEdges.first != n1OutEdges.second; ++n1OutEdges.first)
				{
					FCVertex matched_node_in_n2 = G1.VerticesMap[target(*n1OutEdges.first, *G1.pG)];

					// Find a target node for N2 that matches with N1
					pair<FCOutEdgeItr, FCOutEdgeItr> n2OutEdges = out_edges(N2, *G2.pG);
					for(; n2OutEdges.first != n2OutEdges.second; ++n2OutEdges.first)
						if(target(*n2OutEdges.first, *G2.pG) == matched_node_in_n2)
							break;
	
					if(n2OutEdges.first == n2OutEdges.second)
					{	// None of N2's target node matches with that of N1's.
						all_node_match = false;
						break;
					}
				}

				if(all_node_match)
				{
					// N1 and N2 have same matched neighbors.
					// So they are matched.

					if( G1.pG->GetVertexName(N1) != G2.pG->GetVertexName(N2) )
						relabeling_cost++;

					G1.VerticesMap[*b1] = *b2; G1.nMatched++;
					G2.VerticesMap[*b2] = *b1; G2.nMatched++;

					//cout << "G2.nMatched = " << G2.nMatched << endl;
					// Vertex not within G2's window, so we need to increase window.
					//if( FoundInWindow(G2, G2Win, *b2) == false )
					//	G2Win.winsize++;
					//if( FoundInWindow(G1, G1Win, *b1) == false )
					//	G1Win.winsize++;

					// We want to rematch again since we found new matches
					doRematch = true;
					break;
				}

			} // For all unmatched nodes in G2...
		} // For all unmatched nodes in G1...
	}while (doRematch == true);

	return relabeling_cost;
}

/*!
Returns true if N is found in the window.
*/
/*
bool GraphMatcher::FoundInWindow( GRAPHINFO &G, WININFO &GWin, const FCVertex &N )
{
	int startAddr = G.pG->GetVertexStart(N); 
	if( startAddr < G.pG->GetVertexStart(*GWin.head) ||
	startAddr > G.pG->GetVertexStart(*GWin.tail) )
		return false;
	return true;
}
*/
/*!
Returns true if the specified node has destination nodes
which have not been matched.
*/
bool GraphMatcher::HasUnMatchedDestNode(
	GRAPHINFO &G,
	WININFO &GWin,
	const FCVertex &N
)
{
	pair<FCOutEdgeItr, FCOutEdgeItr> eitr = out_edges(N, *G.pG);
	for(; eitr.first != eitr.second; ++eitr.first)
	{
		//FCVertMap::iterator findItr = G.VerticesMap.find(target(*eitr.first, *G.pG)); 
		FCVertVec::iterator b = GWin.head;
		FCVertVec::iterator e = GWin.tail;
		for( ; b != e; ++b )
		{
			if( G.VerticesMap[*b] == FCG_NULL_VERTEX )
				return true;
		}
		//assert(findItr != G.VerticesMap.end());
		//if( findItr != G.VerticesMap.end() &&
		//	findItr->second == FCG_NULL_VERTEX )
		//	return true;
	}

	return false;
}

//======================================================================
// FILLS UP GLOBAL COST MATRIX
//======================================================================
/*!
Fills in the global cost matrix, i.e. the cost matrix for G1 and G2.

We fill up each element with the neighborhood cost between the pairs of nodes.
This should give us a more accurate measurement of the surroundings (neighborhood)
between the nodes in question.
*/
void GraphMatcher::FillGlobalCostMatrix(
	GRAPHINFO &G1,
	WININFO &G1Win,
	GRAPHINFO &G2,
	WININFO &G2Win,
	unsigned int nNeighborDepth,
	cost **pCost,
	int costDim,
	WORKSPACE &work)
{
	unsigned int i, j;
	FCVertVec::iterator b1, e1, b2, e2;
	//cout << "G1 vertices map size: " << G1.VerticesMap.size() << " G1 matched: " << G1.nMatched << endl;
	//cout << "G2 vertices map size: " << G2.VerticesMap.size() << " G2 matched: " << G2.nMatched << endl;

	int G1UnmatchedCount = G1Win.winsize - G1.nMatched; 
	int G2UnmatchedCount = G2Win.winsize - G2.nMatched; 
	int dimension = G1UnmatchedCount + G2UnmatchedCount;
 

	int m, n;
	//int **discounts;
	//int **discLUT;
	//discounts = (int**) calloc(G1UnmatchedCount, sizeof(int*));
	//discLUT = (int**) calloc(G1UnmatchedCount, sizeof(int*));
	assert( G1UnmatchedCount <= work.discountsR && G2UnmatchedCount <= work.discountsC );
	assert( G1UnmatchedCount <= work.discountsLUTR && G2UnmatchedCount <= work.discountsLUTC );
	for(i = 0; i < G1UnmatchedCount; ++i)
	{	
		//discounts[i] = (int*) calloc(G2UnmatchedCount, sizeof(int));
		//discLUT[i] = (int*) calloc(G2UnmatchedCount, sizeof(int));
		for(j = 0; j < G2UnmatchedCount; ++j)
		{
			work.discounts[i][j] = 0;
			work.discountsLUT[i][j] = 0;
		}
	}


	// Precompute discounts in LUT
	int discount = 0;
	i = 0;
	for(b1 = G1Win.head, e1 = G1Win.tail; b2 != e2; ++b2)
	{
		if(G1.VerticesMap[*b1] != FCG_NULL_VERTEX) continue;

		int nodes1 = G1.pG->GetVertexNodes(*b1);
		int edges1 = G1.pG->GetVertexEdges(*b1);
		//int calls1 = G1.pG->GetVertexCalls(*b1);
		int instrs1 = G1.pG->GetVertexInstrs(*b1);

		j = 0;
		for(b2 = G2Win.head, e2 = G2Win.tail; b2 != e2; ++b2)
		{
			if(G2.VerticesMap[*b2] != FCG_NULL_VERTEX) continue;
	
			int nodes2 = G2.pG->GetVertexNodes(*b2);
			int edges2 = G2.pG->GetVertexEdges(*b2);
			//int calls2 = G2.pG->GetVertexCalls(*b2);
			int instrs2 = G2.pG->GetVertexInstrs(*b2);
			
			float diffNodes = 1;
			if( nodes1 != 0 || nodes2 != 0) diffNodes = ((float)abs(nodes1 - nodes2)) / ((float)(nodes1 + nodes2));
		
			float diffEdges = 1;
			if(edges1 != 0 || edges2 != 0) diffEdges = ((float)abs(edges1 - edges2)) / ((float)(edges1 + edges2));

			//float diffCalls = 1;
			//if(calls1 != 0 || calls2 != 0) diffCalls = (abs(calls1 - calls2)) / (calls1 + calls2);

			float diffInstrs = 1;
			if(instrs1 != 0 || instrs2 != 0) diffInstrs = ((float)abs(instrs1 - instrs2)) / ((float)(instrs1 + instrs2));

			if(diffNodes == 0 && diffEdges == 0 ) discount += 50;
			else if(diffNodes < 0.25 && diffEdges < 0.25) discount += 25;

			if(diffInstrs == 0) discount += 50;
			else if(diffInstrs < 0.25) discount += 25;

			work.discountsLUT[i][j] = discount; //ComputeDiscount(G1, *b1, G2, *b2);

			++j;
		}
		++i;
	}

	// Fill in relabeling and neighborhood cost in
	// the top left quadrant of the cost matrix.
	//FCVertMap::iterator it1;
	//for(it1 = G1.VerticesMap.begin(), i = 0; it1 != G1.VerticesMap.end(); ++it1)
	i = 0;
	for(b1 = G1Win.head, e1 = G1Win.tail;
	b1 != e1;
	++b1)
	{
		//if(G1.VerticesMap[it1->first] != FCG_NULL_VERTEX) continue;
		if(G1.VerticesMap[*b1] != FCG_NULL_VERTEX) continue;
		FCVertex N1 = *b1;
		//FCVertex N1 = it1->first;
		//string L1 = G1.pG->GetVertexName(N1);

		//FCVertMap::iterator it2;
		//for(it2 = G2.VerticesMap.begin(), j = 0; it2 != G2.VerticesMap.end(); ++it2)
		j = 0;
		for(b2 = G2Win.head, e2 = G2Win.tail;
		b2 != e2;
		++b2)
		{
			//if(G2.VerticesMap[it2->first] != FCG_NULL_VERTEX) continue;
			if(G2.VerticesMap[*b2] != FCG_NULL_VERTEX) continue;
			FCVertex N2 = *b2;
			//FCVertex N2 = it2->first;
			//string L2 = G2.pG->GetVertexName(N2);

			//
			// Relabeling cost. The neighbor cost
			// will be considered later.
			//
			
			// Now we do the out neighbors.
			// Evaluate the cost based on their neighbors.

			// Disjoint neighbors.
			float neighborCost = ComputeNeighborCost(G1, N1, G2, N2, nNeighborDepth, work);
			//float neighborCost = ComputeNeighborCost2(G1, N1, G2, N2, nNeighborDepth);

			assert(neighborCost >= 0 && neighborCost <= 1*COST_PRECISION);
			assert( i < costDim && j < costDim);
			pCost[i][j] = floor((neighborCost)+0.5) ; // ((float)relabelCost + neighborCost)/2;
	
			assert(pCost[i][j] >= 0 && pCost[i][j] <= 1*COST_PRECISION);
		
			//
			// Compute eligible discounts...
			//


			// For all targets of N1...
			pair<FCOutEdgeItr, FCOutEdgeItr> N1OutEdges = out_edges(N1, *G1.pG);
			for(; N1OutEdges.first != N1OutEdges.second; ++N1OutEdges.first)
			{
				FCVertex N1Tgt = target(*N1OutEdges.first, *G1.pG);			
				m = FindPosInWin(G1, G1Win, N1Tgt);
				if(m == -1) continue;	// Skip if not neighbors
	
				// For all targets of V2...
				pair<FCOutEdgeItr, FCOutEdgeItr> N2OutEdges = out_edges(N2, *G2.pG);
				for(; N2OutEdges.first != N2OutEdges.second; ++N2OutEdges.first)
				{
					FCVertex N2Tgt = target(*N2OutEdges.first, *G2.pG);
					n = FindPosInWin(G2, G2Win, N2Tgt);
					if( n == -1 ) continue;	// Skip if not neighbors

					// Cap total discount at 0.8
					int totalDiscounts = min(80, work.discountsLUT[i][j] + work.discountsLUT[m][n]);
			
					#ifdef VERBOSE	
					//cout << "i: " << i << " j: " << j << " m: " << m << " n: " << n << " G1 Unmatched: " << G1UnmatchedCount << " G1WinSize: " << G1Win.VList.size() << " G2  Unmatched " << G2UnmatchedCount << " G2WinSize: " << G2Win.VList.size() <<  endl;
					#endif
					assert(i < G1UnmatchedCount && j < G2UnmatchedCount);
					assert(m < G1UnmatchedCount && n < G2UnmatchedCount);
					if( work.discounts[i][j] < totalDiscounts ) work.discounts[i][j] = totalDiscounts;
					if( work.discounts[m][n] < totalDiscounts ) work.discounts[m][n] = totalDiscounts;
				}
			}

			// DO NOT MOVE TO LOOP HEADER!
			++j;
		}

		// DO NOT MOVE TO LOOP HEADER!
		++i;		
	}

	// Apply discounts...
	for(i = 0; i < G1UnmatchedCount; ++i)
	{
		for(j = 0; j < G2UnmatchedCount; ++j)
		{
			#ifdef VERBOSE
			cout << "DEBUG pCost[" << i << "][" << j << "] = " << pCost[i][j] << " discounts = " << work.discounts[i][j] << endl;
			#endif
			pCost[i][j] = floor( ((float)((100 - work.discounts[i][j] )*((float)pCost[i][j]))) / ((float)100) );
			assert(pCost[i][j] >= 0 && pCost[i][j] <= 1*COST_PRECISION);

		}
	}

	//for(i = 0; i < G1UnmatchedCount; ++i)
	//{
	//	free(discounts[i]);
	//	free(discLUT[i]);
	//}
	//free(discLUT);
	//free(discounts);

	// Fill top right quadrant (node deletion).
	// The diagonals have value 1,
	// while other elements have INFINITY.
	for(j = G2UnmatchedCount; j < costDim; ++j)
		for(i = 0; i < G1UnmatchedCount; ++i)
		{
			assert( i < costDim && j < costDim );
			pCost[i][j] = ( (i == (j - G2UnmatchedCount))? 1*COST_PRECISION : INF ); 
		}

	// Fill bottom left quadrant (node insertion).
	// The diagonals will have value 1,
	// while other elements have INFINITY.
	for(i = G1UnmatchedCount/*G1.m_oUnmatchedNodeSet.size()*/; i < costDim; ++i)
		for(j = 0; j < G2UnmatchedCount; ++j)
		{
			assert( i < costDim && j < costDim );
			pCost[i][j] = ( (i - G1UnmatchedCount) == j ? 1*COST_PRECISION : INF );
		}

	// Fill in bottom right quadrant.
	// Dummy nodes matching dummy nodes have
	// 0 change in cost.
	for(i = G1UnmatchedCount; i < costDim; ++i)
		for(j = G2UnmatchedCount; j < costDim; ++j)
		{
			assert( i < costDim && j < costDim);
			pCost[i][j] = 0;
		}

	#ifdef VERBOSE
	PrintCostMatrix(pCost, costDim, G1, G1Win, G1UnmatchedCount, G2, G2Win, G2UnmatchedCount);
	#endif
}

void GraphMatcher::PrintCostMatrix(
	cost ** costM,
	int costDim,
	GRAPHINFO &G1,
	WININFO &G1Win,
	int nG1Unmatched,
	GRAPHINFO &G2,
	WININFO &G2Win,
	int nG2Unmatched)
{
	#ifdef VERBOSE
	// Print headers
	cout << "\t\t\t\t";
	for(int j = 0; j < nG2Unmatched; ++j)
	{
		cout << G2.pG->GetVertexName( GetUnmatchedByIndex(G2, G2Win, j)  ) << "\t\t" ;
	}
	cout << endl;

	cout << "G1Win.VList.size() " << G1Win.VList.size() << endl;
	for(int i = 0; i < nG1Unmatched; ++i)
	{

		cout << "DEBUG " << i << " " << costDim << endl;
		cout << G1.pG->GetVertexName ( GetUnmatchedByIndex(G1, G1Win, i));
		for(int j = 0; j < nG2Unmatched; ++j)
		{
			cout << "\t\t\t" << costM[i][j];
		}
		cout << endl;
	}
	#endif
}

//======================================================================
// COMPUTES NEIGHBOR/LOCAL COST
//======================================================================
float GraphMatcher::ComputeNeighborCost2(
	const GRAPHINFO &G1, const FCVertex &N1,
	const GRAPHINFO &G2, const FCVertex &N2,
	unsigned int nNeighborDepth)
{
	/*
	
	vector<PNEIGHBORINFO> V1;	/// Vector of ptr to neighbor nodes for N1
	vector<PNEIGHBORINFO> V2;	/// Vector of ptr to neighbor nodes for N2
	stack<int> G1Deg, G2Deg;

	// Prepare list of candidate nodes
	GetNeighbors( G1, N1, V1, nNeighborDepth, 0, 0, G1Deg ); 	
	GetNeighbors( G2, N2, V2, nNeighborDepth, 0, 0, G2Deg );

	char *p1, *p2;
	char str1[1024];
	char str2[1024];

	p1 = str1;
	int n = G1Deg.size();
	for(int i = 0; i < n && i < (sizeof(str1) - 1); ++i, ++p1)
	{
		snprintf(p1, 2, "%c", 'a'+(unsigned char)G1Deg.top());
		G1Deg.pop();
	}

	p2 = str2;
	n = G2Deg.size();
	for(int i = 0; i < n && i < (sizeof(str2) - 1); ++i, ++p2)
	{
		snprintf(p2, 2, "%c", 'a'+(unsigned char)G2Deg.top());
		G2Deg.pop();
	}




	float normLevinDist = LevenshteinDistance(str1, str2);
	normLevinDist =	((float) 2*normLevinDist / (((float)(strlen(str1) + strlen(str2))) + normLevinDist));
	//cout << str1 << "\t" << str2 << "\t" << normLevinDist << endl;
	*/

	float nodes1 = (float)G1.pG->GetVertexNodes(N1);
	float nodes2 = (float)G2.pG->GetVertexNodes(N2);
	float diffNodes = 1;
	if( nodes1 != 0 || nodes2 != 0) diffNodes = (abs(nodes1 - nodes2)/(nodes1 + nodes2));

	float edges1 = (float)G1.pG->GetVertexEdges(N1);
	float edges2 = (float)G2.pG->GetVertexEdges(N2);
	float diffEdges = 1;
	if(edges1 != 0 || edges2 != 0) diffEdges = (abs(edges1 - edges2)/(edges1 + edges2));

	float calls1 = (float)G1.pG->GetVertexCalls(N1);
	float calls2 = (float)G2.pG->GetVertexCalls(N2);
	float diffCalls = 1;
	if(calls1 != 0 || calls2 != 0) diffCalls = (abs(calls1 - calls2)) / (calls1 + calls2);

	float instrs1 = (float)G1.pG->GetVertexInstrs(N1);
	float instrs2 = (float)G2.pG->GetVertexInstrs(N2);
	float diffInstrs = 1;
	if(instrs1 != 0 || instrs2 != 0) diffInstrs = (abs(instrs1 - instrs2)) / (instrs1 + instrs2);

	return (0.1*diffNodes + 0.1*diffEdges + 0.6*diffCalls + 0.2*diffInstrs  )*COST_PRECISION;
}

/*!
Computes the neighborhood distance up to a specified neighborhood depth.
*/

float GraphMatcher::ComputeNeighborCost(
	const GRAPHINFO &G1, const FCVertex &N1,
	const GRAPHINFO &G2, const FCVertex &N2,
	unsigned int nNeighborDepth,
	WORKSPACE &work)
{
	vector<PNEIGHBORINFO> V1;	/// Vector of ptr to neighbor nodes for N1
	vector<PNEIGHBORINFO> V2;	/// Vector of ptr to neighbor nodes for N2
	//stack<int> G1Deg, G2Deg;
	// Prepare list of candidate nodes
	GetNeighbors( G1, N1, V1, nNeighborDepth, 0, 0/*, G1Deg*/ ); 	
	GetNeighbors( G2, N2, V2, nNeighborDepth, 0, 0/*, G2Deg*/ );


	int dimension = V1.size() + V2.size();
	assert(dimension <= work.NeighborCostR && dimension <= work.NeighborCostC);
	//cost **pNeighborCost = new cost*[dimension];
	//if( pNeighborCost == NULL ) throw LMException("Cannot allocate neighbor cost matrix!");
	//for(int i = 0; i < dimension; ++i)
	//{
	//	pNeighborCost[i] = new cost[dimension];
	//	if(pNeighborCost[i] == NULL) throw LMException("Cannot allocate neighbor cost matrix!");
	//}
	
	// Fills up neighbor cost matrix
	FillNeighborCostMatrix( G1, V1, G2, V2, work.NeighborCost, dimension);

	//
	// Use Hungarian algorithm to solve best assignment
	//

	// Allocate row and col assignment arrays
	//long *col_mate = new long[dimension];
	//if( col_mate == NULL )
	//	throw LMException("Cannot allocate col_mate!");
	
	//long *row_mate = new long[dimension];
	//if( row_mate == NULL )
	//	throw LMException("Cannot allocate neighbor row_mate!");
	

	// Allocate and copy values for neighborhood cost matrix
	//cost **pNeighborCostCopy;
	//pNeighborCostCopy = new cost*[dimension];
	//if( pNeighborCostCopy == NULL ) throw LMException("Cannot allocate neighbor cost matrix!");
	//for(int i = 0; i < dimension; ++i)
	//{
	//	pNeighborCostCopy[i] = new cost[dimension];
	//	if(pNeighborCostCopy[i] == NULL)
	//		throw LMException("Cannot allocate neighbor cost matrix!");
	//}
	for(int i = 0; i < dimension; ++i)
	{
		for(int j = 0; j < dimension; ++j)
		{
			work.NeighborCostCpy[i][j] = work.NeighborCost[i][j];
		}
	}

	asp(dimension, work.NeighborCostCpy, work.col_mate, work.row_mate);

	// Free copy of neighborhood cost matrix
	//for(int i = 0; i < dimension; ++i) delete [] pNeighborCostCopy[i];
	//delete [] pNeighborCostCopy;

	//
	// Compute cost
	//

	float cost = 0;
	int normalizer = 0;

	// Now we compute the score for the assignments.
	// If the assignment is in the upper left quadrant, we use the neighbor cost.
	// If the assignment is in the upper right or lower left quadrant, a node
	// deletion or insertion is required respectively. We increment by 1 in these
	// cases.
	// Assignments in the lower right quadrant are for dummy nodes, so we ignore
	// those assignments.
	for(int i = 0; i < dimension; ++i)
	{
		if( i < V1.size() &&  work.col_mate[i] < V2.size() )
		{	// Upper left quadrant
			cost += work.NeighborCost[i][work.col_mate[i]];
			normalizer++;
		}
		else if(i < V1.size() || work.col_mate[i] < V2.size())
		{	// Upper right or lower left quadrant
			cost++; normalizer++;
		}

		// Piggyback the loop to free cost.
		//delete[] pNeighborCost[i];
	}
	//delete[] pNeighborCost;
	cost = cost/normalizer;

	// Free matrices and arrays
	//delete [] row_mate;
	//delete [] col_mate;

	// Free neighbor infos
	vector<PNEIGHBORINFO>::iterator nit;
	for( nit = V1.begin(); nit != V1.end(); ++nit )
		delete *nit;
	V1.clear();
	for( nit = V2.begin(); nit != V2.end(); ++nit )
		delete *nit;
	V2.clear();

	assert(cost >= 0 && cost <= 1*COST_PRECISION);
	return cost;
}

/*!
Returns true if neighbor N is already in vector.
*/

bool GraphMatcher::HasNeighbor(vector<PNEIGHBORINFO> &V, const FCVertex &N)
{
	vector<PNEIGHBORINFO>::iterator b, e;
	for(b = V.begin(), e = V.end(); b != e; ++b)
	{
		PNEIGHBORINFO pNeighInfo = *b;

		if(pNeighInfo->N == N)
			return true;
	}

	return false;
}

/*!
Recursively inserts node into neighbor set until neighbor's depth is reached.
*/

void GraphMatcher::GetNeighborsHelper(
	const GRAPHINFO &G,
	const FCVertex &N,
	vector<PNEIGHBORINFO> &V,
	int nNeighborDepth,
	int nTotalEdgeHops,
	float fTotalEdgeWeight,
	bool bHasAncestralLink/*,
	stack<int> &degree*/)
{
	// Add node into set
	PNEIGHBORINFO pNeighbor		= new NEIGHBORINFO;
	if( pNeighbor == NULL ) exit(-4);

	// Do not add if neighbor already exist.
	if( HasNeighbor(V, N) == true ) return;

	pNeighbor->N			= N;
	pNeighbor->nTotalEdgeHops	= nTotalEdgeHops;
	pNeighbor->fTotalEdgeWeight	= min(fTotalEdgeWeight, FLT_MAX); // Prevent infinity
	pNeighbor->bHasAncestralLink	= bHasAncestralLink;
	V.push_back(pNeighbor);

	// Already reached depth
	if(nNeighborDepth == 0)
	{
		//degree.push( in_degree(N, *G.pG) + out_degree(N, *G.pG) );
		return;
	}

	if( bHasAncestralLink == true )
	{
		// For all in-edges...
		pair<FCInEdgeItr, FCInEdgeItr> ie;
		for(ie = in_edges(N, *G.pG); ie.first != ie.second; ++ie.first)
		{
			FCVertex src = source(*ie.first, *G.pG);
			GetNeighborsHelper( G, src, V,
				nNeighborDepth-1,
				nTotalEdgeHops + 1,
				fTotalEdgeWeight + G.pG->GetEdgeExpWeight(*ie.first),
				bHasAncestralLink/*,
				degree*/ );	// Insert source
		}

		// For all out-edges...
		pair<FCOutEdgeItr, FCOutEdgeItr> oe;
		for(oe = out_edges(N, *G.pG); oe.first != oe.second; ++oe.first)
		{
			FCVertex tgt = target(*oe.first, *G.pG);
			GetNeighborsHelper( G, tgt, V,
				nNeighborDepth-1,
				nTotalEdgeHops + 1,
				fTotalEdgeWeight + G.pG->GetEdgeExpWeight(*oe.first),
				bHasAncestralLink/*,
				degree*/ );	// Insert target
		}
	}
	else
	{
		// For all out-edges...
		pair<FCOutEdgeItr, FCOutEdgeItr> oe;
		for(oe = out_edges(N, *G.pG); oe.first != oe.second; ++oe.first)
		{
			FCVertex tgt = target(*oe.first, *G.pG);
			GetNeighborsHelper( G, tgt, V,
				nNeighborDepth-1,
				nTotalEdgeHops + 1,
				fTotalEdgeWeight + G.pG->GetEdgeExpWeight(*oe.first),
				bHasAncestralLink/*,
				degree*/ );	// Insert target
		}

		// For all in-edges...
		pair<FCInEdgeItr, FCInEdgeItr> ie;
		for(ie = in_edges(N, *G.pG); ie.first != ie.second; ++ie.first)
		{
			FCVertex src = source(*ie.first, *G.pG);
			GetNeighborsHelper( G, src, V,
				nNeighborDepth-1,
				nTotalEdgeHops + 1,
				fTotalEdgeWeight + G.pG->GetEdgeExpWeight(*ie.first),
				bHasAncestralLink/*,
				degree*/ );	// Insert source
		}

	}

	//degree.push( in_degree(N, *G.pG) + out_degree(N, *G.pG) );
}

/*!
Recursively inserts node into neighbor set until neighbor's depth is reached.
*/

void GraphMatcher::GetNeighbors(
	const GRAPHINFO &G,
	const FCVertex &N,
	vector<PNEIGHBORINFO> &V,
	int nNeighborDepth,
	int nTotalEdgeHops,
	float fTotalEdgeWeight/*,
	stack<int> &degree*/)
{
	// Add node into set
	PNEIGHBORINFO pNeighbor		= new NEIGHBORINFO;
	if( pNeighbor == NULL ) exit(-4);

	// Do not add if neighbor already exist.
	if( HasNeighbor(V, N) == true ) return;

	pNeighbor->N			= N;
	pNeighbor->nTotalEdgeHops	= nTotalEdgeHops;
	pNeighbor->fTotalEdgeWeight	= min(fTotalEdgeWeight, FLT_MAX); // Prevent infinity
	pNeighbor->bHasAncestralLink	= false;
	V.push_back(pNeighbor);

	// Already reached depth
	if(nNeighborDepth == 0)
	{
		//degree.push( in_degree(N, *G.pG) + out_degree(N, *G.pG) );
		return;
	}


	// For all out-edges...
	pair<FCOutEdgeItr, FCOutEdgeItr> oe;
	for(oe = out_edges(N, *G.pG); oe.first != oe.second; ++oe.first)
	{
		FCVertex tgt = target(*oe.first, *G.pG);	
		GetNeighborsHelper( G, tgt, V,
			nNeighborDepth-1,
			nTotalEdgeHops + 1,
			fTotalEdgeWeight + G.pG->GetEdgeExpWeight(*oe.first),
			false/*,
			degree*/ );	// Insert target
	}

	// degree.push( in_degree(N, *G.pG) + out_degree(N, *G.pG) );

	// For all in-edges...
	pair<FCInEdgeItr, FCInEdgeItr> ie;
	for(ie = in_edges(N, *G.pG); ie.first != ie.second; ++ie.first)
	{
		FCVertex src = source(*ie.first, *G.pG);
		
		GetNeighborsHelper( G, src, V,
			nNeighborDepth-1,
			nTotalEdgeHops + 1,
			fTotalEdgeWeight + G.pG->GetEdgeExpWeight(*ie.first),
			true/*,
			degree*/ );	// Insert source
	}
}


/*!
Returns true if both nodes have same structure,
based on in-deg, out-deg,
*/
/*
int GraphMatcher::ComputeDiscount(
	const GRAPHINFO &G1,
	const FCVertex &V1,
	const GRAPHINFO &G2,
	const FCVertex &V2)
{
	
	#ifdef VERBOSE
	cout << "--------------------------------------" << endl;
	cout << G1.pG->GetVertexName(V1) << "\t" << G2.pG->GetVertexName(V2) << endl;
	#endif

	#ifdef VERBOSE
	cout << nodes1 << "\t" << nodes2 << endl;
	cout << edges1 << "\t" << edges2 << endl;
	//cout << calls1 << "\t" << calls2 << endl;
	cout << instrs1 << "\t" << instrs2 << endl;
	#endif

	return discount;
}
*/
int GraphMatcher::FindPosInWin(
	GRAPHINFO &G,
	WININFO &GWin,
	FCVertex &V)
{
	FCVertVec::iterator b, e;
	int i;

	i = 0;
	for( b = GWin.head, e = GWin.tail; b != e; ++b )
	{
		if(G.VerticesMap[*b] != FCG_NULL_VERTEX) continue;
		if( *b == V ) return i;

		++i;
	}

	return -1;
}

int GraphMatcher::FindPos(
	vector<PNEIGHBORINFO> &Vec,
	FCVertex &V)
{
	int i;
	for( i = 0; i < Vec.size(); ++i )
	{
		if( Vec[i]->N == V ) return i;
	}

	return -1;
}
/*
void GraphMatcher::BiasNeighbors(
	const GRAPHINFO &G1,
	vector<PNEIGHBORINFO> &V1,
	const GRAPHINFO &G2,
	vector<PNEIGHBORINFO> &V2,
	cost **const pNeighborCost)
{
	int i, j, m, n;
	int **discounts;

	discounts = (int**) calloc(V1.size(), sizeof(int*));
	for(i = 0; i < V1.size(); ++i)
		discounts[i] = (int*) calloc(V2.size(), sizeof(int));

	for(i = 0; i < V1.size(); ++i)
	{
		// For all targets of V1...
		pair<FCOutEdgeItr, FCOutEdgeItr> v1OutEdges = out_edges(V1[i]->N, *G1.pG);
		for(; v1OutEdges.first != v1OutEdges.second; ++v1OutEdges.first)
		{
			FCVertex V1Tgt = target(*v1OutEdges.first, *G1.pG);			
			m = FindPos(V1, V1Tgt);
			if(m == -1) continue;	// Skip if not neighbors

			for(j = 0; j < V2.size(); ++j)
			{
				// Skip if different structure
				int discount1 = ComputeDiscount(G1, V1[i]->N, G2, V2[j]->N);
				// cout << "Discount: " << G1.pG->GetVertexName(V1[i]->N) << ", " << G2.pG->GetVertexName(V2[j]->N) << "\t" << discount1 << endl;


				// For all targets of V2...
				pair<FCOutEdgeItr, FCOutEdgeItr> v2OutEdges = out_edges(V2[j]->N, *G2.pG);
				for(; v2OutEdges.first != v2OutEdges.second; ++v2OutEdges.first)
				{
					FCVertex V2Tgt = target(*v2OutEdges.first, *G2.pG);
					n = FindPos(V2, V2Tgt);
					if( n == -1 ) continue;	// Skip if not neighbors

					int discount2 = ComputeDiscount(G1, V1Tgt, G2, V2Tgt);

					// Cap total discount at 0.8
					int totalDiscounts = min(80, discount1 + discount2);
					
					if( discounts[i][j] < totalDiscounts ) discounts[i][j] = totalDiscounts;
					if( discounts[m][n] < totalDiscounts ) discounts[m][n] = totalDiscounts;

				}
			}
		}
	}	


	#ifdef VERBOSE
	cout << "DEBUG DISCOUNT" << endl;
	#endif
	for(i = 0; i < V1.size(); ++i)
	{
		for(j = 0; j < V2.size(); ++j)
		{
			#ifdef VERBOSE
			cout << i << "," << j << ":" << discounts[i][j] << "\t";
			#endif
			pNeighborCost[i][j] = floor( ((float)((100 - discounts[i][j] )*pNeighborCost[i][j])) / ((float)100) );
			assert(pNeighborCost[i][j] >= 0 && pNeighborCost[i][j] <= 1*COST_PRECISION);
		}
		#ifdef VERBOSE
		cout << endl;
		#endif
	}

	for(i = 0; i < V1.size(); ++i)
		free(discounts[i]);
	free(discounts);
}
*/
/*!
Fills in the  neighbor cost matrix using the set of neighbors V1 and V2.
*/

void GraphMatcher::FillNeighborCostMatrix(
	const GRAPHINFO &G1,
	vector<PNEIGHBORINFO> &V1,
	const GRAPHINFO &G2,
	vector<PNEIGHBORINFO> &V2,
	cost **const pNeighborCost,
	int costDim)
{
	// Fill top left quadrant
	int i, j;

	for(i = 0; i < V1.size(); ++i)
	{
		string Label1 = G1.pG->GetVertexName(V1[i]->N);
		for(j = 0; j < V2.size(); ++j)
		{
			float cost = 0;

			// Label cost	
			string Label2 = G2.pG->GetVertexName(V2[j]->N);
			float relabelCost = 0.3*(ComputeNormRelabelCost(Label1, Label2, true));
			assert(relabelCost >= 0 && relabelCost <= 1);
			// cout << "Label Cost\t: " << relabelCost << endl; 
			cost += relabelCost;
	
			// Hop cost
			int sumEdgeHops = V1[i]->nTotalEdgeHops + V2[j]->nTotalEdgeHops;
			if( sumEdgeHops != 0 )
			{
				float hopCost = 0.05*( (abs(V1[i]->nTotalEdgeHops - V2[j]->nTotalEdgeHops)) / ((float)sumEdgeHops) );
				assert(hopCost >= 0 && hopCost <= 1);
				// cout << "Hop Cost\t:" << hopCost << endl;
				cost += hopCost;
			}

			// Edge weight cost
			int sumEdgeWeights = V1[i]->fTotalEdgeWeight + V2[j]->fTotalEdgeWeight;
			if( sumEdgeWeights != 0 )
			{
				float edgeWeights = 0.05*((abs(V1[i]->fTotalEdgeWeight - V2[j]->fTotalEdgeWeight)) / ((float)sumEdgeWeights) );
				// cout << "Edge Weights\t:" << edgeWeights << endl;
				assert(edgeWeights >= 0 && edgeWeights <= 1);
				cost += edgeWeights;
			}

			// In-degree cost
			int indegree1 = in_degree(V1[i]->N, *G1.pG);
			int indegree2 = in_degree(V2[j]->N, *G2.pG);
			int sumInDegrees = indegree1 + indegree2;
			if( sumInDegrees != 0 )
			{
				float indegCost = 0.3*((abs( indegree1 - indegree2 )) / ((float)sumInDegrees) );
				// cout << "In-Deg Cost\t: " << indegCost << endl;
				assert(indegCost >= 0 && indegCost <= 1);
				cost += indegCost;	
			}

			// Out-degree cost	
			int outdegree1 = out_degree(V1[i]->N, *G1.pG);
			int outdegree2 = out_degree(V2[j]->N, *G2.pG);
			int sumOutDegrees = outdegree1 + outdegree2;
			if( sumOutDegrees != 0 )
			{
				float outdegCost = 0.3*((abs( outdegree1 - outdegree2 )) / ((float)sumOutDegrees));
				// cout << "Out-Deg Cost\t:" << outdegCost << endl;
				assert(outdegCost >= 0 && outdegCost <= 1);
				cost += outdegCost;
			}

			assert(cost >= 0 && cost <= 1);
			pNeighborCost[i][j] = floor(cost*COST_PRECISION+0.5);
			if(pNeighborCost[i][j] < 0 || pNeighborCost[i][j] > 1*COST_PRECISION)
			{
				cout << "pNeighborCost at (" << i << ", " << j << ") is " << pNeighborCost[i][j];
			}
			assert(pNeighborCost[i][j] >= 0 && pNeighborCost[i][j] <= 1*COST_PRECISION);


		}
	}

	//BiasNeighbors( G1, V1, G2, V2, pNeighborCost);

	// Fill top right quadrant (node deletion). The diagonals have value 1,
	// while other elements have INFINITY.
	for(j = V2.size(); j < costDim; ++j)
		for(i = 0; i < V1.size(); ++i)
		{
			pNeighborCost[i][j] = ( (i == (j - V2.size()))? 1*COST_PRECISION : INF ); 
		}
	
	// Fill bottom left quadrant (node insertion). The diagonals will have value 1,
	// while other elements have INFINITY.
	for(i = V1.size(); i < costDim; ++i)
		for(j = 0; j < V2.size(); ++j)
		{
			pNeighborCost[i][j] =( (i - V1.size()) == j ? 1*COST_PRECISION : INF );
		}

	// Fill in bottom right quadrant. Dummy nodes matching dummy nodes have
	// 0 change in cost.
	for(i = V1.size(); i < costDim; ++i)
		for(j = V2.size(); j < costDim; ++j)
		{
			pNeighborCost[i][j] = 0;
		}

	//PrintCostMatrix(pNeighborCost, costDim);
}

//======================================================================
// COMMON FUNCTIONS
//======================================================================

/*!
Computes the relabeling cost. If any of the labels is prefixed with "sub_",
we return cost 0.5, since we don't know if they are really the same labels.

Otherwise, we compute the Levenshtein Distance and
return the normalized distance which will be between 0 and 1.
*/
float GraphMatcher::ComputeNormRelabelCost(const string &L1, const string &L2, bool UsePrecomp)
{
	if( UsePrecomp == true )
	{
		// If we already precomputed the values, return
		pair<string, string> labelpair(L1, L2);
		map< pair<string, string>, float >::iterator itr;
		itr = precompNormRelabelCost.find(labelpair);
	
		if( itr != precompNormRelabelCost.end()	)
		{
			return itr->second;
		}
	}

	if( L1.compare(0, 4, "sub_") == 0 || L2.compare(0, 4, "sub_") == 0  )
		// If either label start with "sub_" function, return edit
		// cost 0.5. We don't want to be biased to 0 or 1.
		return 0.5;
	
	float normLevinDist = LevenshteinDistance(L1, L2);
	normLevinDist =	((float) 2*normLevinDist / (((float)(L1.length() + L2.length())) + normLevinDist));

	// Different lib
	return  normLevinDist;
}

/*!
Finds the difference between the first and last function address in the window.
*/
float GraphMatcher::ComputeEdgeWeightRange(GRAPHINFO &G, WININFO &GWin)
{
	unsigned int addr1;
	unsigned int addr2;
	FCVertVec::iterator it;

	if(GWin.VList.size() == 0)
		return 0;

	it = GWin.VList.begin();
	addr1 = G.pG->GetVertexStart(*it);
	it = GWin.VList.end(); --it;
	addr2 = G.pG->GetVertexStart(*it);
	assert(addr2 >= addr1);

	return (float)(addr2 - addr1);

	/*
	float fMinWeight;
	float fMaxWeight;

	// Initialize
	fMinWeight = G.pG->GetEdgeWeight(*edges(*G.pG).first);
	fMaxWeight = fMinWeight;

	pair<FCEdgeItr, FCEdgeItr> ei;
	for(ei = edges(*G.pG); ei.first != ei.second; ++ei.first)
	{
		float fEdgeWeight = G.pG->GetEdgeWeight(*ei.first);
	
		if(fMinWeight > fEdgeWeight) fMinWeight = fEdgeWeight;
		if(fMaxWeight < fEdgeWeight) fMaxWeight = fEdgeWeight; 
	}

	return (fMaxWeight - fMinWeight);
	*/
}

/*!
Precompute the normalized relabeling cost between all nodes. It's very expensive,
so we don't want to keep recomputing it whenever we need it.
*/
void GraphMatcher::PrecompNormRelabel(GRAPHINFO &G1, GRAPHINFO &G2)
{
	pair<FCVertexItr, FCVertexItr>	G1Vit;	// G1 vertex iterators
	pair<FCVertexItr, FCVertexItr>	G2Vit;	// G2 vertex iterators

	for(G1Vit = vertices(*G1.pG); G1Vit.first != G1Vit.second; ++G1Vit.first)
	{
		string L1 = G1.pG->GetVertexName(*G1Vit.first);
		for(G2Vit = vertices(*G2.pG); G2Vit.first != G2Vit.second; ++G2Vit.first)
		{
			string L2 = G2.pG->GetVertexName(*G2Vit.first);
			pair<string, string> labelpair(L1, L2);
			precompNormRelabelCost[labelpair] = ComputeNormRelabelCost(L1, L2, false);
		}
	}
}

/*!
Updates the database with the score. We need to do this since we can't return a float
value to the Jython script.

result_id is valid only when mode is "update".
*/
void GraphMatcher::InsertScoreToDB(
	const char* file_id,
	float score,
	long elapsed_ms,
	unsigned int expt_id,
	const char* mode,
	int result_id,
	const char* sqlsvr)
{
	MYSQL		*connection, mysql;
	MYSQL_ROW	row;
	int		query_state;

	mysql_init(&mysql);

	connection = mysql_real_connect(
		&mysql,
		sqlsvr,
		"libmatcher",
		"l1bm@tcher",
		"libmatcher",
		0, 0, 0);
	if( connection == NULL )
	{
		cout << mysql_error(&mysql) << endl;
		return;
	}

	// Insert match score to database. Note we
	// do not use UPDATE, so that we don't
	// accidentally overwrite previous results.

	char query_buffer[512];
	if( strncmp(mode, "update", strlen("update")) == 0 && result_id != -1)
	{
		snprintf(query_buffer,
			sizeof(query_buffer),
			"UPDATE result_approx SET score=%f, elapsed_ms=%lu, timestamp=NOW() WHERE result_id=%u",
			score,
			elapsed_ms,
			result_id);
	}
	else if( strncmp(mode, "new", strlen("new")) == 0 ||
	strncmp(mode, "resume", strlen("resume")) == 0 ||
	(strncmp(mode, "update", strlen("update")) == 0 && result_id == -1))
	{
		snprintf(query_buffer,
			sizeof(query_buffer),
			"INSERT INTO result_approx (file_id, score, elapsed_ms, expt_id, timestamp) VALUES(%s, %f, %lu, %u, NOW())",
			file_id, score, elapsed_ms, expt_id);
	}

	query_state = mysql_query(connection, query_buffer);
	if( query_state != 0)
	{
		cout << mysql_error(connection) << endl;
		return;
	}

	mysql_close(connection);
}
