#ifndef __ALIGNER_H__
#define __ALIGNER_H__

#include <log4cpp/Category.hh>
#include <vector>
#include <function.h>
#include <mysql/mysql.h>
#include <fcg.h>
#include <asp.h>
#include <set>
#include <rex.h>
#include <list>
#include <rex.h>
#include <rextranslator.h>

using namespace std;

#ifndef SMALL_FUNC_LIMIT
#define SMALL_FUNC_LIMIT 300
#endif

#define QUERY_WAIT_SECONDS 20
#define UNINIT_FINAL_COST	( (10*COST_PRECISION) - 1 )

typedef map<FCVertex, pair<FCVertex, float> >::iterator	Lib2AppItr;
typedef enum{ UNEXPLORED, PENDING, COMPLETED } EXPLORE_STATUS;

class Aligner
{
public:
	Aligner(
		MYSQL *pConn,
		const unsigned int expt_id,
		RexTranslator *_rextrans,
		log4cpp::Category *const pLog,
		log4cpp::Category *_rexlog );
	~Aligner();

	float ComputeAlignDist3( FCGraph &FCG1, FCGraph &FCG2 );
	float ComputeAlignDist2( FCGraph &FCG1, FCGraph &FCG2 );
	float ComputeAlignDist( FCGraph &FCG1, FCGraph &FCG2 );
	void ComputeSmallFuncDist( FCGraph &FCG1, FCGraph &FCG2 );

private:
	map< FCVertex, set<FCVertex> >	IS_Pairs_fwd;	/// Forward hard pairings (by symex)
	map< FCVertex, set<FCVertex> >	IS_Pairs_rev;	/// Reverse hard pairings (by symex)
	map< FCVertex, set<FCVertex> >	MAY_Pairs_fwd;	/// Vertices that may be paired
	map< FCVertex, set<FCVertex> >	MAY_Pairs_rev;	/// Vertices that may be paired

	map< FCVertex, set< pair< FCVertex, cost > > >	smallCosims;

	map< FCVertex, FCVertex >		mapFringes;
	map< FCVertex, pair<FCVertex, float> >	mapLib2AppFuncs;
	MYSQL					*pConn;
	unsigned int				expt_id;
	log4cpp::Category			*pLog;
	
	EXPLORE_STATUS				**exploreStatus;
	cost					**finalCostMatrix;
	cost					**ppCostMatrix;
	unsigned int				numRows;
	unsigned int				numCols;
	vector< set<FCVertex>* >		clusters;

	log4cpp::Category			*rexLog;
	RexTranslator				*rextrans;
	RexUtils				*rexutils1;
	RexUtils				*rexutils2;

	void FreeCostMatrix(void);

	bool IsRecursive( FCGraph &FCG, FCVertex &F );

	int InitCostMatrix( FCGraph &FCG1, FCGraph &FCG2 );

	float CalcMunkresCost(
		FCGraph &FCG1,
		FCVertex &func1,
		FCGraph &FCG2,
		FCVertex &func2);

	int CalcMinCostMapping( FCGraph &FCG1, FCGraph &FCG2 );

	int SpillCosts( FCGraph &FCG2, const unsigned short nReach );

	float GetMappedScore( FCVertex &F1, FCVertex &F2 );

	float CalcMapChildrenScore( FCGraph &FCG1, FCVertex &F1, FCGraph &FCG2, FCVertex &F2 );

	float CalcMapParentScore( FCGraph &FCG1, FCVertex &F1, FCGraph &FCG2, FCVertex &F2 );

	float CalcMapVertexScore( FCGraph &FCG1, FCVertex &F1, FCGraph &FCG2, FCVertex &F2 );

	float GetClusterScore( FCGraph &FCG11, FCGraph &FCG2, set<FCVertex> *const pCluster );

	int QTCluster( FCGraph &FCG, set< FCVertex > &G, ea_t D );

	REXSTATUS DoSymbolicExec(
		FCGraph &FCG1,
		FCGraph &FCG2,
		FCVertex v1,
		FCVertex v2 );

	RexFunc* MakeRexFunc( FCGraph &FCG, FCVertex V );

	void GetAvailParents(
		FCGraph &FCG,
		FCVertex &func,
		map< FCVertex, set<FCVertex> > &paired,
		vector<FCVertex> &availParents );
	size_t GetAvailChildren(
		FCGraph &FCG,
		FCVertex &func,
		map< FCVertex, set<FCVertex> > &paired,
		vector<FCVertex> &availChild );

	void BiasCostMatrix( cost **ppMatrix, unsigned int rows, unsigned int cols );

	void CalcMunkresParent(
		FCGraph &FCG1, FCVertex &func1,
		FCGraph &FCG2, FCVertex &func2,
		map< FCVertex, FCVertex > &newpairs);
	void CalcMunkresChild(
		FCGraph &FCG1, FCVertex &func1,
		FCGraph &FCG2, FCVertex &func2,
		map< FCVertex, FCVertex > &newpairs);
	void CalcMunkresHelper(
		FCGraph &FCG1, FCVertex &func1,
		FCGraph &FCG2, FCVertex &func2,
		list< cost > &prevCosts );

	void CalcPairedCost( FCGraph &FCG1, FCGraph &FCG2 );

	int InsertBestOfMAYPair(
		FCGraph &FCG1,
		FCVertex &V1,
		FCGraph &FCG2,
		FCVertex &V2 );
	void InsertISPair( FCVertex &V1, FCVertex &V2 );
	void InsertMAYPair( FCVertex &V1, FCVertex &V2 );
	void InsertPair( FCVertex &V1, FCVertex &V2,
		map< FCVertex, set<FCVertex> > &pairsFwd,
		map< FCVertex, set<FCVertex> > &pairsRev );

	void DeleteISPair( FCVertex &V1, FCVertex &V2 );
	void DeleteMAYPair( FCVertex &V1, FCVertex &V2 );
	void DeletePair(
		FCVertex &V1,
		FCVertex &V2,
		map< FCVertex, set<FCVertex> > &pairsFwd,
		map< FCVertex, set<FCVertex> > &pairsRev );

	bool HasISPair( FCVertex &V1, FCVertex &V2 );
	bool HasMAYPair( FCVertex &V1, FCVertex &V2 );
	bool HasPair( FCVertex &V1, FCVertex &V2, map< FCVertex, set<FCVertex> > &pairsFwd );

	cost GetParentPairCost(	FCGraph &FCG1, FCVertex &func1,	FCGraph &FCG2, FCVertex &func2, int depth );
	cost GetChildPairCost( FCGraph &FCG1, FCVertex &func1, FCGraph &FCG2, FCVertex &func2, int depth );
	cost GetPairCost( FCGraph &FCG1, FCVertex &func1, FCGraph &FCG2, FCVertex &func2, int depth );

	void AddSmallestCosims( FCVertex &func1, FCVertex &func2, const cost val );

	void FormGroups(
		FCGraph &FCG1,
		FCGraph &FCG2,
		map<FCVertex, FCVertex> &finalMapFwd,
		map<FCVertex, FCVertex> &finalMapRev,
		map<FCVertex, int> &group,
		unsigned int *ngroups,
		unsigned int *groupcount );

};

#endif
