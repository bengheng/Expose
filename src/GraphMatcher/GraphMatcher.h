#ifndef __GRAPHMATCHER_H__
#define __GRAPHMATCHER_H__

#include <stack>
#include <set>
#include <map>
#include "Graph.h"
#include "LMException.hpp"
#include <gsl/gsl_matrix.h>
#include "../fcg/fcg.h"
#include "asp.h"

using namespace std;

typedef pair<FCVertex, FCVertex>	FCVertPair;
typedef set<FCVertex>			FCVertSet;
typedef list<FCVertex>			FCVertList;
typedef map<FCVertex, FCVertex>		FCVertMap;
typedef vector<FCVertex>		FCVertVec;
typedef set<FCVertex*>			PFCVertSet;
typedef map<FCVertex*, FCVertex*>	PFCVertMap;

typedef struct
{
	FCGraph			*pG;			/// Ptr to graph
	//FCVertSet		m_oMatchedNodeSet;	/// Set of matched nodes
	//FCVertSet		m_oUnmatchedNodeSet;	/// Set of unmatched nodes
	//int			m_uUnmatchedNodeCount;	/// Number of unmatched nodes
	//FCVertMap		m_oMatchMap;		/// Map of matched nodes between vertices
						/// in this graph and the other graph
	//FCVertVec		m_oVertexVec;		/// Vector of vertices. For quickly accessing vertices by index.
	
	//vector<FCVertPair>	VertPairVec;		/// Vector of vertex pairs
	FCVertMap		VerticesMap;		/// Map of vertices
	unsigned int		nMatched;		/// Number of matched nodes
} GRAPHINFO, *PGRAPHINFO;

typedef struct
{
	FCVertVec		VList;
	FCVertVec::iterator	head;
	FCVertVec::iterator	tail;
	unsigned int		winsize;
} WININFO, *PWININFO;

typedef struct
{
	FCVertex	N;			/// Node
	int		nTotalEdgeHops;		/// Number of edges from center node
	float		fTotalEdgeWeight;	/// Total weight of edges from center node
	bool		bHasAncestralLink;	/// True if linked via an ancestral edge
	//bool		isPerimeter;		/// True if is a perimeter node
} NEIGHBORINFO, *PNEIGHBORINFO;

typedef struct
{
	cost **NeighborCost;
	int NeighborCostR;
	int NeighborCostC;

	cost **NeighborCostCpy;
	int NeighborCostCpyR;
	int NeighborCostCpyC;

	long *col_mate;
	int col_mate_len;
	long *row_mate;
	int row_mate_len;
	
	int **discounts;
	int discountsR;
	int discountsC;

	int **discountsLUT;
	int discountsLUTR;
	int discountsLUTC;	
} WORKSPACE, *PWORKSPACE;

// Costs
#define COST_SUBLIB 	1
#define COST_DIFLIB 	1	// Different library should be deleted, not matched
#define COST_DUMMY	1	// Delete nodes

#define COST_INSNODE	1
#define COST_DELNODE	1

#define COST_INSEDGE	1
#define COST_DELEDGE	1

//#define NEIGHBOR_DEPTH	1	/// Depth of neighbors to compute cost

class GraphMatcher
{
public:
	GraphMatcher(const string &G1DotName, const string &G2DotName) throw (LMException);
	~GraphMatcher() throw();

	float ComputeGraphDist(unsigned int nNeighborDepth);
	void InsertScoreToDB(
		const char* file_id,
		float score,
		long elapsed_ms,
		unsigned int expt_id,
		const char *mode,
		int result_id,
		const char* sqlsvr);
private:
	GRAPHINFO				G1;
	GRAPHINFO				G2;
	WININFO					G1Win;
	WININFO					G2Win;
	WORKSPACE				work;

	// gsl_matrix				*pCost;			/// Cost matrix for Hungarian algorithm
	// gsl_matrix				*pCostRelabel;		/// Relabelling cost matrix

	float					fG1EdgeWeightRange;	/// Edge weight range of G1
	map< pair<string, string>, float >	precompNormRelabelCost;	/// Precomputed norm label cost


	void SortVertices(GRAPHINFO &G, WININFO &GWin);
	void OrderedIns(GRAPHINFO &G, FCVertex v, WININFO &GWin);


	void CombineMatches(
		GRAPHINFO &G1,
		WININFO &G1Win,
		GRAPHINFO &G2,
		WININFO &G2Win,
		long *row_mate);
	FCVertex GetUnmatchedByIndex(GRAPHINFO &G, WININFO &GWin, unsigned int index);

	float ComputeSubGraphDist(
		unsigned int nNeighborDepth,
		GRAPHINFO &G1,
		WININFO &G1Win,
		GRAPHINFO &G2,
		WININFO &G2Win,
		WORKSPACE &work);

	// Computing final cost
	float ComputeTotalCost(
		GRAPHINFO &G1,
		WININFO &G1Win,
		GRAPHINFO &G2,
		WININFO &G2Win,
		int matched_node_relabeling_cost);

	//void ComputeNodeEditCost(
	//	GRAPHINFO &G1, FCVertex N1,
	//	GRAPHINFO &G2, FCVertex N2,
	//	FCVertVec &vecDeletedNodes,
	//	float *pfTotalEdgeCost,
	//	float *pfTotalRelabelCost);


	float ComputeMean(vector<float> &vec);
	float ComputeStdDev(vector<float> &vec, float mean);
	float ComputeSimilarityScore(vector<float> &vec, float mean, float stddev);

	void ComputeNodeSubCost(
		GRAPHINFO &G1, FCVertex &N1,
		GRAPHINFO &G2, FCVertex &N2,
		FCVertVec &vecDeletedNodes,
		float *pfTotalEdgeCost,
		float *pfTotalRelabelCost);

	void ComputeNodeDelCost(
		GRAPHINFO &G, FCVertex &N,
		FCVertVec &vecDeletedNodes,
		float *pfTotalEdgeCost,
		float *pfTotalRelabelCost);

	// Find pairwise node matches
	int FindPairwiseNodeMatches(GRAPHINFO &G1, WININFO &G1Win, GRAPHINFO &G2, WININFO &G2Win);
	void ComputeLabelMatches(GRAPHINFO &G1, WININFO &G1Win, GRAPHINFO &G2, WININFO &G2Win);
	int ComputeTargetMatches(GRAPHINFO &G1, WININFO &G1Win, GRAPHINFO &G2, WININFO &G2Win);
	bool HasUnMatchedDestNode(GRAPHINFO &G, WININFO &GWin, const FCVertex &V);

	// Fill global cost matrix
	void FillGlobalCostMatrix(
		GRAPHINFO &G1,
		WININFO &G1Win,
		GRAPHINFO &G2,
		WININFO &G2Win,
		unsigned int nNeighborDepth,
		cost **pCost,
		int costDim,
		WORKSPACE &work);
		
	// Computing neighbor cost
	
	float ComputeNeighborCost(
		const GRAPHINFO &G1, const FCVertex &N1,
		const GRAPHINFO &G2, const FCVertex &N2,
		unsigned int nNeighborDepth,
		WORKSPACE &work);
	
	float ComputeNeighborCost2(
		const GRAPHINFO &G1, const FCVertex &N1,
		const GRAPHINFO &G2, const FCVertex &N2,
		unsigned int nNeighborDepth);
	
	void GetNeighbors(
		const GRAPHINFO &G,
		const FCVertex &N,
		vector<PNEIGHBORINFO> &V,
		int nNeighborDepth,
		int nTotalEdgeHops,
		float fTotalEdgeWeight/*,
		stack<int> &degree*/);
	
	
	void GetNeighborsHelper(
		const GRAPHINFO &G,
		const FCVertex &N,
		vector<PNEIGHBORINFO> &V,
		int nNeighborDepth,
		int nTotalEdgeHops,
		float fTotalEdgeWeight,
		bool bHasAncestralLink/*,
		stack<int> &degree*/);
	
	
	void FillNeighborCostMatrix(
		const GRAPHINFO &G1,
		vector<PNEIGHBORINFO> &V1,
		const GRAPHINFO &G2,
		vector<PNEIGHBORINFO> &V2,
		cost **const pNeighborCost,
		int costDim);
	

	void PrintCostMatrix(
		cost ** costM,
		int costDim,
		GRAPHINFO &G1,
		WININFO &G1Win,
		int nG1Unmatched,
		GRAPHINFO &G2,
		WININFO &G2Win,
		int G2Unmatched);

	// Common functions
	float ComputeNormRelabelCost(
		const string &L1,
		const string &L2,
		bool usePrecomp);

	float ComputeEdgeWeightRange(GRAPHINFO &G, WININFO &GWin);

	void PrecompNormRelabel(GRAPHINFO &G1, GRAPHINFO &G2);

	bool HasNeighbor(vector<PNEIGHBORINFO> &V, const FCVertex &N);

	//bool FoundInWindow(GRAPHINFO &G, WININFO &GWin, const FCVertex &N);

	int FindPos(vector<PNEIGHBORINFO> &Vec, FCVertex &V);

	int FindPosInWin( GRAPHINFO &G, WININFO &GWin, FCVertex &V);
	
	/*
	void BiasNeighbors(
		const GRAPHINFO &G1,
		vector<PNEIGHBORINFO> &V1,
		const GRAPHINFO &G2,
		vector<PNEIGHBORINFO> &V2,
		cost **const pNeighborCost);
	*/
	/*
	int ComputeDiscount(
		const GRAPHINFO &G1,
		const FCVertex &V1,
		const GRAPHINFO &G2,
		const FCVertex &V2);
	*/
};

#endif
