#include <log4cpp/Category.hh>
#include <vector>
#include <function.h>
#include <mysql/mysql.h>
#include <aligner.h>
#include <asp.h>
#include <math.h>
#include <sat_cosim.h>
#include <fcg.h>
#include <rex.h>
#include <log4cpp/FileAppender.hh>
#include <log4cpp/PatternLayout.hh>
#include <sys/time.h>
#include <list>
#include <rextranslator.h>

using namespace std;


Aligner::Aligner(
	MYSQL *pCon,
	const unsigned int exptid,
	RexTranslator *_rextrans,
	log4cpp::Category *const pLg,
	log4cpp::Category *_rexLog )
{
	this->pConn = pCon;
	this->expt_id = exptid;
	this->pLog = pLg;
	this->ppCostMatrix = NULL;
	this->numRows = 0;
	this->numCols = 0;

	this->rexLog = _rexLog;
	this->rextrans = _rextrans;

	// One for each of the two functions
	this->rexutils1 = new RexUtils( this->rexLog );
	this->rexutils2 = new RexUtils( this->rexLog );

	this->pLog->info( "Aligner constructor done.");
}

Aligner::~Aligner()
{
	FreeCostMatrix();

	size_t n = clusters.size();
	while( n > 0 )
	{
		set< FCVertex >* c = clusters.back();
		clusters.pop_back();
		delete c;
		--n;
	}

	delete this->rexutils1;
	delete this->rexutils2;
}

/*!
Releases memory for cost matrix.
*/
void Aligner::FreeCostMatrix(void)
{
	// Free memory for cosim matrix
	if( this->ppCostMatrix != NULL )
	{
		for( unsigned int i = 0; i < numRows; ++i )
		{
			delete [] ppCostMatrix[i];
			delete [] finalCostMatrix[i];
			delete [] exploreStatus[i];
		}
		delete [] ppCostMatrix;
		delete [] finalCostMatrix;
		delete [] exploreStatus;
	}

	ppCostMatrix = NULL;
	finalCostMatrix = NULL;
	exploreStatus = NULL;
}

/*!
Returns true if V is self-recursive.
*/
bool Aligner::IsRecursive( FCGraph &FCG, FCVertex &V)
{
	FCOutEdgeItr b, e;
	for( tie(b, e) = out_edges( V, FCG ); b != e; ++b )
	{
		if( target( *b, FCG ) == V ) return true;
	}
	return false;
}

void Aligner::AddSmallestCosims( FCVertex &func1, FCVertex &func2, const cost val )
{
	map< FCVertex, set< pair< FCVertex, cost > > >::iterator findSmallCosim;

	findSmallCosim = smallCosims.find(func1);
	if( findSmallCosim != smallCosims.end() )
	{
		if( findSmallCosim->second.size() > 5 )
		{
			// If we can find func1 in smallCosims and there's already
			// 10 values, get the largest cosim in the set. If this
			// value is larger than val, boot it.
			
			cost largestCosim;
			set< pair<FCVertex, cost> >::iterator b2, e2, largestItr;
			b2 = findSmallCosim->second.begin();
			e2 = findSmallCosim->second.end();
			largestItr = b2;
			largestCosim = b2->second;
 			for( ; b2 != e2; ++b2 )
			{
				if( b2->second > largestCosim )
				{
					largestCosim = b2->second;
					largestItr = b2;
				}
			}

			if( val >= largestCosim )
			{
				// val is not smaller than any cosim.
				return;
			}

			findSmallCosim->second.erase( largestItr );
		}
	}

	smallCosims[func1].insert( make_pair<FCVertex, cost>( func2, val ) );
}


#define MIN_FUNC_SIZE 200

/*!
Allocate memory and initialize cosim values. If cosim values are absent, INF is used.
If differences between cyccomp and size are too huge between two functions, the cosim
is not updated.

Discounts are provided.
*/

int Aligner::InitCostMatrix( FCGraph &FCG1, FCGraph &FCG2 )
{
	if( this->ppCostMatrix != NULL ) return -1;

	numRows = num_vertices( FCG1 );
	numCols = num_vertices( FCG2 );

	// Allocate memory for ppCostMatrix, exploreStatus
	// and finalCostMatrix.
	ppCostMatrix = new cost*[numRows];
	finalCostMatrix = new cost*[numRows];
	exploreStatus = new EXPLORE_STATUS*[numRows];
	for( unsigned int i = 0; i < numRows; ++i )
	{
		ppCostMatrix[i] = new cost[numCols];
		finalCostMatrix[i] = new cost[numCols];
		exploreStatus[i] = new EXPLORE_STATUS[numCols];
		for( unsigned int j = 0; j < numCols; ++j )
		{
			ppCostMatrix[i][j] = 1*COST_PRECISION;
			finalCostMatrix[i][j] = UNINIT_FINAL_COST;
			exploreStatus[i][j] = UNEXPLORED;
		}
	}

	// Initialize cosim values
	FCVertex func1 = FCG1.GetFirstVertex();
	while( func1 != FCG_NULL_VERTEX )
	{
		unsigned int func1Idx = FCG1.GetVertexIdx(func1);
		unsigned int func1Id = FCG1.GetVertexFuncId(func1);
		unsigned int func1Size = FCG1.GetVertexSize(func1);

		/*
		if( func1Size < MIN_FUNC_SIZE )
		{
			func1 = FCG1.GetNextVertex();
			pLog->info("DEBUG\t1 Func %d too small! (%d bytes)", func1Id, func1Size);
			continue;
		}*/


		unsigned int func1CycComp = FCG1.GetVertexCycComp(func1);
		unsigned int func1Nparams = FCG1.GetVertexNparams(func1);
		unsigned int func1Nvars = FCG1.GetVertexNvars(func1);
		unsigned int func1OutDeg = out_degree( func1, FCG1 );
		bool func1Recurse = IsRecursive( FCG1, func1 );

		FCVertex func2 = FCG2.GetFirstVertex();
		while( func2 != FCG_NULL_VERTEX )
		{
			unsigned int func2Idx = FCG2.GetVertexIdx(func2);
			unsigned int func2Id = FCG2.GetVertexFuncId(func2);
			unsigned int func2Size = FCG2.GetVertexSize(func2);

			/*
			if( func2Size < MIN_FUNC_SIZE )
			{
				func2 = FCG2.GetNextVertex();
				pLog->info("DEBUG\t2 Func %d too small (%d bytes)", func2Id, func2Size);
				continue;
			}*/
			
			//==============
			// CONSTRAINTS
			//==============
			// Skip if the following constraints are not true.
			// 1. Cyccomp differ by more than 50% of func1
			// 2. Func size differ by more than 50% of func1
			unsigned int func2CycComp = FCG2.GetVertexCycComp(func2);
			unsigned int diffCycComp = ( func1CycComp > func2CycComp ?
				func1CycComp - func2CycComp :
				func2CycComp - func1CycComp );
			float diffRatioCycComp = ((float) diffCycComp / (float) func1CycComp );
			/*
			if( func1CycComp > 5 && diffRatioCycComp > 0.5 )
			{
				pLog->info("DEBUG\tincompat cyccomp %s, %s",
					FCG1.GetVertexName(func1).c_str(),
					FCG2.GetVertexName(func2).c_str());
				func2 = FCG2.GetNextVertex();
				continue;
			}
			*/
			unsigned int diffSize = (func1Size > func2Size ?
				func1Size - func2Size :
				func2Size - func1Size );
			float diffRatioSize = ((float)diffSize/(float)func1Size);
			/*
			if( func1Size > 200 && diffRatioSize > 0.5 )
			{
				pLog->info("DEBUG\tincompat size %s, %s",
					FCG1.GetVertexName(func1).c_str(),
					FCG2.GetVertexName(func2).c_str());
				func2 = FCG2.GetNextVertex();
				continue;
			}
			*/
			if( func1Recurse != IsRecursive(FCG2, func2) )
			{
				//pLog->info("DEBUG\tincompat recurse %s, %s",
				//	FCG1.GetVertexName(func1).c_str(),
				//	FCG2.GetVertexName(func2).c_str() );
				func2 = FCG2.GetNextVertex();
				continue;
			}


			//
			// Get cosim. 0 is nearest, 1 is furthest.
			//
			float cosim =  QueryCosim( pConn, expt_id, func1Id, func2Id, pLog );

			//============
			// DISCOUNTS
			//============
			// For significantly high cyclomatic complexity (> 5),
			// discount by 50% if difference ratio is small (< 0.3).
			if( func1Size > 200 &&
			func1CycComp > 5 && diffRatioCycComp < 0.3 )
			{
				cosim = ( cosim > 0.1 ? cosim - 0.1 : 0);
			}

			// For significantly large functions (> 200),
			// discount by 50% if difference ratio is small (< 0.2).
			if( func1Size > 200 && diffRatioSize < 0.2 ) 
			{
				cosim = ( cosim > 0.1 ? cosim - 0.1 : 0);
			}

			// For non-zero numbers of parameters and variables,
			// discount by 50% if they are the same.
			unsigned int func2Nparams = FCG2.GetVertexNparams(func2);
			unsigned int func2Nvars = FCG2.GetVertexNvars(func2);
			unsigned int func2OutDeg = out_degree( func2, FCG2 );

			REXSTATUS symres = REX_ERR_UNK;
			if( func1Size < SMALL_FUNC_LIMIT &&
			func2Size < SMALL_FUNC_LIMIT &&
			func1Nparams == func2Nparams &&
			func1OutDeg == func2OutDeg &&
			//func1Nvars == func2Nvars  &&
			func1CycComp < 15 &&
			func2CycComp < 15 &&
			cosim < 0.8 )
			//diffRatioCycComp < 0. &&
			//diffRatioSize < 0.2 ) )
			{
				struct timeval start, end;
				long elapsed_ms, seconds, useconds;
				gettimeofday(&start, NULL); // Start Timer	

				pLog->info( "SYMEXEC %s, %s",
					FCG1.GetVertexName(func1).c_str(),
					FCG2.GetVertexName(func2).c_str() );

				symres = DoSymbolicExec(
					FCG1, FCG2, func1, func2 );
	
				gettimeofday(&end, NULL);				// Stop Time
				seconds = end.tv_sec - start.tv_sec;
				useconds = end.tv_usec - start.tv_usec;
				elapsed_ms = ( (seconds) * 1000 + (long) ceil(useconds/1000.0) );


				char stat = 'E';
				switch( symres )
				{
				case REX_SE_TRUE: stat = '='; break;
				case REX_SE_FALSE: stat = '!'; break;
				case REX_SE_MISMATCH: stat = 'M'; break;
				case REX_SE_TIMEOUT: stat = 'T'; break;
				case REX_SE_ERR: stat = 'E'; break;
				case REX_TOO_MANY: stat = 'Z'; break;
				case REX_ERR_IR: stat = 'I'; break;
				case REX_ERR_UNK: stat = 'U'; break;
				default: stat = '?';
				}

				pLog->info( "SYMEXEC %c %d ms %s, %s",
					stat, elapsed_ms,
					FCG1.GetVertexName(func1).c_str(),
					FCG2.GetVertexName(func2).c_str() );
			}
			//FCG2.SetVertexWeight(func2, ((float)symres)/2 );
			if( symres == REX_SE_TRUE )
			{
				cosim = ( cosim > 0.3 ? cosim - 0.3 : 0 );
				InsertISPair( func1, func2 );
				FCG2.SetVertexPaired( func2, FCG1.GetVertexName(func1) );
			}
			else 
			{
				//if( symres == 0 )
				//{
				//	cosim = ( cosim < 0.7 ? cosim + 0.3 : 1 );
				//}
			}

			if( func1OutDeg != func2OutDeg )
			{
				cosim = ( cosim < 0.9 ? cosim + 0.1 : 1 );
			}

			if( func1Nparams != 0 && func1Nvars != 0 &&
			func1Nparams == func2Nparams && func1Nvars == func2Nvars )
			{
				cosim = (cosim > 0.2 ? cosim - 0.2 : 0);
			}

			// Discount if same outdegree
			if( out_degree(func1, FCG1) == out_degree(func2, FCG2) )
			{
				cosim = (cosim > 0.05 ? cosim - 0.05 : 0);
				//pLog->info("DISCOUNT outdeg %s,%s",
				//	FCG1.GetVertexName(func1).c_str(),
				//	FCG2.GetVertexName(func2).c_str());
			}

			if( cosim > 0.8 ) cosim = 1;
			if( cosim < 0.2 ) cosim = 0;
			ppCostMatrix[ func1Idx ][ func2Idx ]
				= (cost) floor( cosim * COST_PRECISION + 0.5 );

			AddSmallestCosims( func1, func2, ppCostMatrix[ func1Idx ][ func2Idx ] );
			// DEBUG
			//if( ppCostMatrix[ func1Idx ][ func2Idx ] != 1*COST_PRECISION )
			//{
			//	pLog->info("COSIM %d\t%s, %s", ppCostMatrix[ func1Idx ][ func2Idx ],
			//		FCG1.GetVertexName(func1).c_str(),
			//		FCG2.GetVertexName(func2).c_str() );
			//}
			// GUBED

			func2 = FCG2.GetNextVertex();
		}

		func1 = FCG1.GetNextVertex();
	}


	return 0;
}

void Aligner::BiasCostMatrix(
	cost **ppMatrix,
	unsigned int rows,
	unsigned int cols )
{
	unsigned int i, j;
	map< unsigned int, unsigned int > rowsLo;
	map< unsigned int, unsigned int > colsLo;
	cost lowest;

	// Get the lowest cost for each row
	for( i = 0; i < rows; ++i )
	{
		// Set to first element in i-th row
		lowest = ppMatrix[i][0];
		rowsLo[i] = 0;

		for( j = 1; j < cols; ++j )
		{
			if( ppMatrix[i][j] < lowest )
			{
				rowsLo[i] = j;
				lowest = ppMatrix[i][j];
			}
		}
	}

	// Get the lowest cost for each col
	for( j = 0; j < cols; ++j )
	{
		lowest = ppMatrix[0][j];
		colsLo[j] = 0;

		for( i = 1; i < rows; ++i )
		{
			if( ppMatrix[i][j] < lowest )
			{
				colsLo[j] = i;
				lowest = ppMatrix[i][j];
			}
		}			
	}

	// Now set the intersecting lowest costs to 0
	if( rows > cols )
	{
		for( i = 0; i < rows; ++i )
		{
			if( colsLo[ rowsLo[i] ] == i &&
				ppMatrix[i][rowsLo[i]] < (1*COST_PRECISION) )
			{
				ppMatrix[i][rowsLo[i]] = 0;
			}
		}
	}
	else
	{
		for( j = 0; j < cols; ++j )
		{
			if( rowsLo[ colsLo[j] ] == j &&
			ppMatrix[colsLo[j]][j] < (1*COST_PRECISION) )
			{
				ppMatrix[colsLo[j]][j] = 0;
			}
		}
	}

}

/*!
The Munkres cost is computed over the cosim values between the
current functions and the immediate targets (children).
*/
float Aligner::CalcMunkresCost(
	FCGraph &FCG1,
	FCVertex &func1,
	FCGraph &FCG2,
	FCVertex &func2)
{
	// Apply Munkres on children of func1 against
	// func2 and all its children.

	unsigned int i, j;

	// Init matrix
	unsigned int m = out_degree( func1, FCG1 );
	unsigned int n = out_degree( func2, FCG2 ) + 1;
	unsigned int aspDim = m + n;
	
	cost **aspMatrix = new cost*[aspDim];
	cost **aspCpy = new cost*[aspDim];	// Make a copy. Original will be altered.
	for( i = 0; i < aspDim; ++i )
	{
		aspMatrix[i] = new cost[aspDim];
		aspCpy[i] = new cost[aspDim];
	}
	

	// Initialize 1st Quad
	FCOutEdgeItr b1, e1;
	for( tie(b1, e1) = out_edges( func1, FCG1 ), i = 0;
	b1 != e1; ++b1, ++i )
	{
		unsigned int func1ChildIdx = FCG1.GetVertexIdx( target(*b1, FCG1) );

		// init cosim with func2's children
		FCOutEdgeItr b2, e2;
		for( tie(b2, e2) = out_edges( func2, FCG2 ), j = 0;
		b2 != e2; ++b2, ++j )
		{
			unsigned int func2ChildIdx = FCG2.GetVertexIdx( target(*b2, FCG2) );
			aspMatrix[i][j]	= ppCostMatrix[ func1ChildIdx ][ func2ChildIdx ];
			aspCpy[i][j] = aspMatrix[i][j];
		}

		// init cosim with func2
		aspMatrix[i][j] = ppCostMatrix[ func1ChildIdx ][ FCG2.GetVertexIdx(func2) ];
		aspCpy[i][j] = aspMatrix[i][j];
	}

	// Initialize 2nd Quad (diags to 1, rest to INF)
	for( i = 0; i < m; ++i )
	{
		for( j = n; j < aspDim; ++j )
		{
			aspMatrix[i][j] = INF;
			aspCpy[i][j] = aspMatrix[i][j];
		}
	}
	for( i = 0; i < m; ++i )
	{
		aspMatrix[i][n+i] = 1*COST_PRECISION;
		aspCpy[i][n+i] = aspMatrix[i][n+i];
	}

	// Initialize 3rd Quad (diags to 1, rest to INF)
	for( i = m; i < aspDim; ++i )
	{
		for( j = 0; j < n; ++j )
		{
			aspMatrix[i][j] = INF;
			aspCpy[i][j] = aspMatrix[i][j];
		}
	}
	for( j = 0; j < n; ++j )
	{
		aspMatrix[m+j][j] = 1*COST_PRECISION;
		aspCpy[m+j][j] = aspMatrix[m+j][j];
	}

	// Initialize 4th Quad (dummy to dummy, all to 0)
	for( i = m; i < aspDim; ++i )
	{
		for( j = n; j < aspDim; ++j )
		{
			aspMatrix[i][j] = 0;
			aspCpy[i][j] = 0;
		}
	}

	long *row_mate = new long[aspDim];
	if( row_mate == NULL ) pLog->fatal("row_mate is NULL!");
	long *col_mate = new long[aspDim];
	if( col_mate == NULL ) pLog->fatal("col_mate is NULL!");

	//
	// Apply Munkres
	//
	asp( aspDim, aspMatrix, col_mate, row_mate );

	//
	// If func2 is assigned to another function X,
	// i.e. func2-X, choose the better match between
	// func1-func2 and func2-X.
	//
	cost fn1fn2Cost = ppCostMatrix[ FCG1.GetVertexIdx(func1) ][ FCG2.GetVertexIdx(func2) ];
	cost fn2XCost = 1*COST_PRECISION;
	if( row_mate[n-1] < m )
		fn2XCost = aspCpy[row_mate[n-1]][n-1];
	
	cost fn2UseCost = min( fn1fn2Cost, fn2XCost );

	// Compute average cost
	unsigned int count = 0;
	cost totalCost = 0;
	for( i = 0; i < m; ++i )
	{
		if( col_mate[i] != (n-1))
		{
			totalCost += aspCpy[i][col_mate[i]];
			count++;
		}
	}
	totalCost += fn2UseCost;
	count++;

	float aveCost = (((float) totalCost)/((float) count)) / ((float) COST_PRECISION);

	// Free aspMatrix
	for( i = 0; i < aspDim; ++i )
	{
		delete [] aspMatrix[i];
		delete [] aspCpy[i];
	}
	delete [] aspMatrix;
	delete [] aspCpy;
	delete [] row_mate;
	delete [] col_mate;

	return aveCost;
}

/*!
Computes the best Munkres scores for each function in FCG1.
*/
int Aligner::CalcMinCostMapping( FCGraph &FCG1, FCGraph &FCG2 )
{
	FCVertex func1 = FCG1.GetFirstVertex();
	while( func1 != FCG_NULL_VERTEX )
	{
		FCVertex func2 = FCG2.GetFirstVertex();
		while( func2 != FCG_NULL_VERTEX )
		{
			// Compute score between 2 vertices
			float newcost = CalcMunkresCost( FCG1, func1, FCG2, func2 );

			// DEBUG
			FCG2.SetVertexWeight( func2,
				min( newcost, FCG2.GetVertexWeight( func2 ) ) );

			//
			// If func1 already maps to some app func,
			// update mapping with the better (smaller cost) one.
			//

			map< FCVertex, pair<FCVertex, float> >::iterator matchedAppFunc;
			matchedAppFunc = mapLib2AppFuncs.find( func1 );
			if( matchedAppFunc == mapLib2AppFuncs.end() )
			{
				// No previous mapping. Create new mapping.
				mapLib2AppFuncs[ func1 ] = make_pair<FCVertex, float>(func2, newcost);
			}
			else if( newcost < matchedAppFunc->second.second )
			{
				// Found previous mapping,
				// use the one with smaller cost.
				matchedAppFunc->second.first = func2;
				matchedAppFunc->second.second = newcost;
			}
			func2 = FCG2.GetNextVertex();
		}
		func1 = FCG1.GetNextVertex();
	}

	/*
	map< FCVertex, pair<FCVertex, float> >::iterator b, e;
	for( b = mapLib2AppFuncs.begin(), e = mapLib2AppFuncs.end(); b != e; ++b )
	{
		pLog->info("DEBUG %f\t%s, %s", b->second.second,
			FCG1.GetVertexName(b->first).c_str(),
			FCG2.GetVertexName(b->second.first).c_str() );	
	}
	*/
	return 0;
}


/*!
Spill cost to neighboring edges. Each lib function is mapped to some app func.
*/
int Aligner::SpillCosts( FCGraph &FCG2, const unsigned short nReach )
{
	// For all matches, spread the cost to neighbors
	map< FCVertex, pair<FCVertex, float> >::iterator b1, e1;
	for( b1 = mapLib2AppFuncs.begin(), e1 = mapLib2AppFuncs.end();
	b1 != e1; ++b1 )
	{
		FCG2.SpillWeight( b1->second.first,
			1 - b1->second.second, nReach );
	}

	// Normalize costs
	FCG2.NormalizeEdgeWeights();

	return 0;
}

/*!
Returns the score if F1 is mapped to F2. Otherwise returns -1.
*/
float Aligner::GetMappedScore( FCVertex &F1, FCVertex &F2 )
{
	map< FCVertex, pair<FCVertex, float> >::iterator b, e;
	for( b = mapLib2AppFuncs.begin(), e = mapLib2AppFuncs.end();
	b != e; ++b )
	{
		if( b->first == F1 && b->second.first == F2 )
			return b->second.second;
	}
	// Can't find.
	return (float)-1;
}

float Aligner::CalcMapChildrenScore(
	FCGraph &FCG1, FCVertex &F1,
	FCGraph &FCG2, FCVertex &F2 )
{
	unsigned int numMatch = 0;
	float totalScore = 0;

	// For every outgoing edge of F1,
	FCOutEdgeItr outB1, outE1;
	for( tie(outB1, outE1) = out_edges( F1, FCG1 ); outB1 != outE1; ++outB1 )
	{
		FCVertex F1Child = target( *outB1, FCG1 );

		float maxScore = 0;
		bool hasMatch = false;

		// For every outgoing edge of F2,
		// find best match
		FCOutEdgeItr outB2, outE2;
		for( tie(outB2, outE2) = out_edges( F2, FCG2 ); outB2 != outE2; ++outB2 )
		{
			FCVertex F2Child = target( *outB2, FCG2 );

			// Skip if not spiller or
			// F1Child is not mapped to F2Child.
			if( FCG2.GetVertexNspill(F2Child) == 0 ||
			GetMappedScore( F1Child, F2Child ) == (float)-1 ) continue;

			float score = FCG2.GetEdgeWeight( *outB2 );
			if( score > maxScore )
			{
				maxScore = score;
				hasMatch = true;
			}
		}

		// If there was a match..
		if( hasMatch == true )
		{
			++numMatch;
			totalScore += maxScore;
			//pLog->info("DEBUG CalcMapChildrenScore Adding %f", maxScore);
		}
	}

	//pLog->info("DEBUG CalcMapChildrenScore numMatch %d totalScore %f %d",
	//	numMatch, totalScore, out_degree( F1, FCG1) );

	return ( numMatch == 0 ? 0:  totalScore / (float) numMatch );
	//return ( numMatch == 0 ? 0: totalScore / out_degree(F1, FCG1) );
}

float Aligner::CalcMapParentScore(
	FCGraph &FCG1, FCVertex &F1,
	FCGraph &FCG2, FCVertex &F2 )
{
	unsigned int numMatch = 0;
	float totalScore = 0;

	// For every incoming edge of F1,
	FCInEdgeItr inB1, inE1;
	for( tie(inB1, inE1) = in_edges( F1, FCG1); inB1 != inE1; ++inB1 )
	{
		FCVertex F1Par = source( *inB1, FCG1 );

		float maxScore = 0;
		bool hasMatch = false;

		// For every incoming edge of F2,
		// find best match
		FCInEdgeItr inB2, inE2;
		for( tie(inB2, inE2) = in_edges( F2, FCG2); inB2 != inE2; ++inB2 )
		{
			FCVertex F2Par = source( *inB2, FCG2 );

			// Skip if not spiller or
			// F1Par is not mapped to F2Par.
			if( FCG2.GetVertexNspill(F2Par) == 0 ||
			GetMappedScore( F1Par, F2Par) == (float) -1 ) continue;

			float score = FCG2.GetEdgeWeight( *inB2 );
			if( score > maxScore )
			{
				maxScore = score;
				hasMatch = true;
			}

		}

		// If there was a match..
		if( hasMatch == true )
		{
			++numMatch;
			totalScore += maxScore;
		}
	}

	return ( numMatch == 0 ? 0 : totalScore / (float) numMatch );
}


float Aligner::CalcMapVertexScore(
	FCGraph &FCG1, FCVertex &F1,
	FCGraph &FCG2, FCVertex &F2 )
{
	// float parScore = CalcMapParentScore( FCG1, F1, FCG2, F2 );
	return CalcMapChildrenScore( FCG1, F1, FCG2, F2 );
}


/*!
The cluster's score is computed by finding the average of the non-zero weights
of edges incident to vertices in the cluster.
*/

float Aligner::GetClusterScore(
	FCGraph &FCG1, FCGraph &FCG2,
	set< FCVertex > *const pCluster )
{
	float totalScore = 0;
	unsigned int n = 0;

	// For each vertex in the cluster
	set< FCVertex >::iterator b1, e1;
	for( b1 = pCluster->begin(), e1 = pCluster->end();
	b1 != e1; ++b1 )
	{
		float maxScore = 0;
		// Look for mapping
		map< FCVertex, pair<FCVertex, float> >::iterator b2, e2;
		for( b2 = mapLib2AppFuncs.begin(), e2 = mapLib2AppFuncs.end();
		b2 != e2; ++b2 )
		{
			FCVertex F2 = b2->second.first;
			if( F2 == *b1 ) continue;
			FCVertex F1 = b2->first;

			float currScore = CalcMapVertexScore( FCG1, F1, FCG2, F2 );
			if( currScore > maxScore )
			{
				maxScore = currScore;
			}
		}

		if( maxScore > 0 )
		{
			// Found mapping
			totalScore += maxScore;
			++n;
		}
	}

	pLog->info( "DEBUG GetClusterScore totalScore %f n %d", totalScore, n);

	return ( n == 0 ? 0 : totalScore / n );
}

/*!
Recursive function to cluster vertices.
*/
int Aligner::QTCluster( FCGraph &FCG, set< FCVertex > &G, ea_t D )
{
	// Base cases
	if( G.size() == 0 ) return 0;
	if( G.size() == 1 )
	{
		// Create new cluster and transfer
		// last vertex from G to clusters.
		set<FCVertex> *pClust = new set<FCVertex>();
		pClust->insert( *G.begin() );
		G.erase( G.begin() );
		clusters.push_back(pClust);
		return 0;
	}

	// Candidate clusters
	vector< set<FCVertex>* > candClusts;

	set<FCVertex>::iterator b1, e1;
	for( b1 = G.begin(), e1 = G.end(); b1 != e1; ++b1 )
	{
		bool flag = true;

		// Start a new cluster
		set<FCVertex> *A = new set<FCVertex>();
		candClusts.push_back( A );
		A->insert( *b1 );		
	
		ea_t lo = FCG.GetVertexStartEA( *b1 );
		ea_t hi = FCG.GetVertexEndEA( *b1 );

		// Make a copy of G, whose elements get transferred
		// to A if a minimum diameter is found.
		vector< FCVertex > G2;
		set< FCVertex >::iterator b2, e2;
		for( b2 = G.begin(), e2 = G.end(); b2 != e2; ++b2 )
		{
			G2.push_back( *b2 );
		}

		while( flag == true &&
		A->size() != G.size() )
		{
			ea_t diameter = 0xffffffff;
			ea_t gap = 0xffffffff;

			// Find j such that new diameter is minimum
			vector< FCVertex >::iterator b3, e3, j;
			for( b3 = G2.begin(), e3 = G2.end(); b3 != e3; ++b3 )
			{
				ea_t start = FCG.GetVertexStartEA( *b3 );
				ea_t end = FCG.GetVertexEndEA( *b3 );

				if( end < lo && (lo - end) < gap )
				{
					gap = lo - end;
					diameter = hi - start;
					j = b3;
				}
				else if( start > hi && (start- hi) < gap )
				{
					gap = start - hi;
					diameter = end - lo;
					j = b3;
				}
			}
			
			// If new minimum diameter exceeds D,
			// then we didn't find any suitable vertex.
			if( diameter > D )	flag = false;
			else
			{
				A->insert(*j);
				G2.erase(j);
			}
		}
	}

	set< FCVertex >* maxC = candClusts.back();
	candClusts.pop_back();
	while( candClusts.size() > 0 )
	{
		set< FCVertex > *curC = candClusts.back();
		candClusts.pop_back();
	
		if( curC->size() > maxC->size() )
		{
			delete maxC;
			maxC = curC;
		}
		else
			delete curC;
	}

	clusters.push_back( maxC );

	// Remove elements in maxC from G.
	set< FCVertex >::iterator b4, e4;
	for( b4 = maxC->begin(), e4 = maxC->end(); b4 != e4; ++b4 )
	{
		set< FCVertex >::iterator b5, e5;
		for( b5 = G.begin(), e5 = G.end(); b5 != e5; ++b5 )
		{
			if( *b4 == *b5 )
			{
				G.erase(b5);
				break;
			}
		}
	}

	return QTCluster( FCG, G, D ); // recurse
}

float Aligner::ComputeAlignDist( FCGraph &FCG1, FCGraph &FCG2 )
{
	pLog->info("ComputeAlignDist");
	if( InitCostMatrix( FCG1, FCG2 ) != 0 ) return -1;
	pLog->info("CalcMinCostMapping");
	if( CalcMinCostMapping( FCG1, FCG2 ) != 0 ) return -1;
	pLog->info("SpillCosts");
	if( SpillCosts( FCG2, 1 ) != 0 ) return -1;


	// DEBUG
	//FCG2.PropagateWeights();

	//
	// Cluster the spiller vertices using QT clustering.
	//

	set<FCVertex> setSpillers;
	FCVertex v = FCG2.GetFirstVertex();
	while( v != FCG_NULL_VERTEX )
	{
		if( FCG2.GetVertexNspill(v) > 0 )
			setSpillers.insert(v);

		v = FCG2.GetNextVertex();
	}

	QTCluster( FCG2, setSpillers, FCG1.GetDiameter() );

	float maxScore = 0;
	unsigned int n = 0;
	// For each cluster...
	vector< set<FCVertex>* >::iterator b2, e2;
	for( b2 = clusters.begin(), e2 = clusters.end(); b2 != e2; ++b2 )
	{
		float score = GetClusterScore( FCG1, FCG2, *b2 );
		if( score > maxScore ) maxScore = score;

		pLog->info("Cluster %u Count %d Score %f",
			n, (*b2)->size(), score );

		
		set< FCVertex >::iterator b3, e3;
		for( b3 = (*b2)->begin(), e3 = (*b2)->end(); b3 != e3; ++b3 )
		{
			// Set cluster numbers
			FCG2.SetVertexCluster(*b3, n);

			pLog->info("\t%08x - %08x\t%s",
				FCG2.GetVertexStartEA( *b3 ),
				FCG2.GetVertexEndEA( *b3 ),
				FCG2.GetVertexName( *b3 ).c_str() );

			map< FCVertex, pair<FCVertex, float> >::iterator b1, e1;
			for( b1 = mapLib2AppFuncs.begin(), e1 = mapLib2AppFuncs.end();
			b1 != e1; ++b1 )
			{
				if( *b3 != b1->second.first ) continue;

				pLog->info("\t\t%f\t%s",
					b1->second.second,
					FCG1.GetVertexName(b1->first).c_str() );
			}
		}

		++n;
	}

	//FCG2.PrintVertices();
	//FCG2.PrintEdges();
	char dotname[256];
	snprintf(dotname, sizeof(dotname), "%s.dot", FCG2.GetGraphName().c_str());
	FCG2.WriteDOT(dotname);	

	return maxScore;
}	

//====================================================================================================

void Aligner::GetAvailParents(
	FCGraph &FCG,
	FCVertex &func,
	map< FCVertex, set<FCVertex> > &pairs,
	vector<FCVertex> &availParents )
{
	FCInEdgeItr b, e;
	for( tie(b, e) = in_edges( func, FCG );
	b != e; ++b )
	{
		FCVertex v = source(*b, FCG);
		if( pairs.find( v ) == pairs.end() )
			availParents.push_back( v );	
	}
}

// Saves the available children into availChild
// and returns the number of discarded pairs.
size_t Aligner::GetAvailChildren(
	FCGraph &FCG,
	FCVertex &func,
	map< FCVertex, set<FCVertex> > &pairs,
	vector<FCVertex> &availChild )
{
	size_t discarded = 0;
	FCOutEdgeItr b, e;
	for( tie(b, e) = out_edges( func, FCG );
	b != e; ++b )
	{
		FCVertex v = target(*b, FCG);
		if( pairs.find( v ) == pairs.end() )
		{
			availChild.push_back( v );	
		}
		else
		{
			++discarded;
		}
	}

	return discarded;
}

void Aligner::InsertISPair( FCVertex &V1, FCVertex &V2 )
{
	InsertPair( V1, V2, IS_Pairs_fwd, IS_Pairs_rev );
}

void Aligner::InsertMAYPair( FCVertex &V1, FCVertex &V2 )
{
	InsertPair( V1, V2, MAY_Pairs_fwd, MAY_Pairs_rev );
}

/*!
Inserts V1-V2 pairing if their cosim is better than any value
in the current pairings for V1. V1 is paired to the 3 V2's with
best cosim scores.
*/
int Aligner::InsertBestOfMAYPair(
	FCGraph &FCG1,
	FCVertex &V1,
	FCGraph &FCG2,
	FCVertex &V2 )
{
	cost worstCost = 1*COST_PRECISION;
	cost newCost = ppCostMatrix[ FCG1.GetVertexIdx(V1) ][ FCG2.GetVertexIdx(V2) ];

	map< FCVertex, set<FCVertex> >::iterator mapFind;
	set<FCVertex>::iterator worstItr;

	mapFind = MAY_Pairs_fwd.find(V1);
	if( mapFind != MAY_Pairs_fwd.end() &&
	mapFind->second.size() == 3 )
	{
		worstItr = mapFind->second.end();

		// Get the worst cost for the
		// present pairings.
		set< FCVertex >::iterator b, e;
		for( b = mapFind->second.begin(), e = mapFind->second.end();
		b != e; ++b )
		{
			cost curCost
			= ppCostMatrix[ FCG1.GetVertexIdx(mapFind->first) ][ FCG2.GetVertexIdx(*b) ];

			if( curCost < worstCost )
			{
				worstItr = b;
				worstCost = curCost;
			}
		}

		// Boot the one with the worst cost if the newCost is better,
		// so as to make space for the new cost.
		if( newCost < worstCost &&
		worstItr != mapFind->second.end() )
		{
			FCVertex worstV = *worstItr;
			DeleteMAYPair( V1, worstV );
		}
	}

	// Put in the new cost
	if( newCost < worstCost )
	{
		InsertMAYPair( V1, V2 );
		return 0;
	}

	return -1;
}

void Aligner::InsertPair(
	FCVertex &V1,
	FCVertex &V2,
	map< FCVertex, set<FCVertex> > &pairsFwd,
	map< FCVertex, set<FCVertex> > &pairsRev )
{
	pairsFwd[ V1 ].insert( V2 );
	pairsRev[ V2 ].insert( V1 );
}

void Aligner::DeleteISPair( FCVertex &V1, FCVertex &V2 )
{
	DeletePair( V1, V2, IS_Pairs_fwd, IS_Pairs_rev );
}

void Aligner::DeleteMAYPair( FCVertex &V1, FCVertex &V2 )
{
	DeletePair( V1, V2, MAY_Pairs_fwd, MAY_Pairs_rev );
}

bool Aligner::HasPair( FCVertex &V1, FCVertex &V2,
	map< FCVertex, set<FCVertex> > &pairsFwd )
{
	map< FCVertex, set<FCVertex> >::iterator mapFind;
	set<FCVertex>::iterator setFind;

	// Erase from forward pairing
	mapFind = pairsFwd.find(V1);
	if( mapFind != pairsFwd.end() )
	{
		setFind = mapFind->second.find(V2);
		if( setFind != mapFind->second.end() )
		{
			return true;
		}
	}

	return false;
}

bool Aligner::HasMAYPair( FCVertex &V1, FCVertex &V2 )
{
	return HasPair( V1, V2, MAY_Pairs_fwd );
}

bool Aligner::HasISPair( FCVertex &V1, FCVertex &V2 )
{
	return HasPair( V1, V2, IS_Pairs_fwd );
}

void Aligner::DeletePair(
	FCVertex &V1,
	FCVertex &V2,
	map< FCVertex, set<FCVertex> > &pairsFwd,
	map< FCVertex, set<FCVertex> > &pairsRev )
{
	map< FCVertex, set<FCVertex> >::iterator mapFind;
	set<FCVertex>::iterator setFind;

	// Erase from forward pairing
	mapFind = pairsFwd.find(V1);
	if( mapFind != pairsFwd.end() )
	{
		setFind = mapFind->second.find(V2);
		if( setFind != mapFind->second.end() )
		{
			mapFind->second.erase( setFind );
		}
	}

	// Erase from reverse pairing
	mapFind = pairsRev.find(V2);
	if( mapFind != pairsRev.end() )
	{
		setFind = mapFind->second.find(V1);
		if( setFind != mapFind->second.end() )
		{
			mapFind->second.erase( setFind );
		}
	}

}

/*!
The Munkres cost is computed over the cosim values between the
current functions and the immediate targets (children).
*/
void Aligner::CalcMunkresParent(
	FCGraph &FCG1,
	FCVertex &func1,
	FCGraph &FCG2,
	FCVertex &func2,
	map<FCVertex, FCVertex> &newpairs)
{
	// Apply Munkres on func1 and all of its parents
	// against func2's parents.

	unsigned int i, j;

	vector<FCVertex> availParent1;
	vector<FCVertex> availParent2;

	if( in_degree( func1, FCG1 ) == 0 )
	{
		pLog->info( "1 %s has NO parent.", FCG1.GetVertexName(func1).c_str() );
		return;
	}

	if( in_degree( func2, FCG2 ) == 0 )
	{
		pLog->info( "2 %s has NO parent.", FCG2.GetVertexName(func2).c_str() );
		return;
	}

	// Extract parents that are available, i.e. not in IS_pair.
	GetAvailParents( FCG1, func1, IS_Pairs_fwd, availParent1 );
	if( availParent1.size() == 0 )
	{
		pLog->info( "1 %s has NO AVAIL parent.", FCG1.GetVertexName(func1).c_str() );
		return;
	}

	GetAvailParents( FCG2, func2, IS_Pairs_rev, availParent2 );
	if( availParent2.size() == 0 )
	{
		pLog->info( "2 %s has NO AVAIL parent.", FCG2.GetVertexName(func2).c_str() );
		return;
	}

	// DEBUG - Just printing available parents...
	pLog->info( "1 %s has %zu avail parents.",
		FCG1.GetVertexName(func1).c_str(), availParent1.size() );
	vector<FCVertex>::iterator b3, e3;
	for( b3 = availParent1.begin(), e3 = availParent1.end();
	b3 != e3; ++b3 )
	{
		pLog->info("\t%s", FCG1.GetVertexName(*b3).c_str());
	}
	pLog->info( "2 %s has %zu avail parents.",
		FCG2.GetVertexName(func2).c_str(), availParent2.size() );
	for( b3 = availParent2.begin(), e3 = availParent2.end();
	b3 != e3; ++b3 )
	{
		pLog->info("\t%s", FCG2.GetVertexName(*b3).c_str());
	}
	// GUBED

	// Init matrix
	unsigned int m = availParent1.size() + 1;
	unsigned int n = availParent2.size();
	unsigned int aspDim = m + n;
	
	cost **aspMatrix = new cost*[aspDim];
	cost **aspCpy = new cost*[aspDim]; // Make a copy. Original will be altered.
	for( i = 0; i < aspDim; ++i )
	{
		aspMatrix[i] = new cost[aspDim];
		aspCpy[i] = new cost[aspDim];
	}
	

	// Initialize 1st Quad
	// init cosim with func2's parents
	vector<FCVertex>::iterator b2, e2;
	for( b2 = availParent2.begin(), e2 = availParent2.end(), j = 0;
	b2 != e2; ++b2, ++j )
	{
		unsigned int func2ParentIdx = FCG2.GetVertexIdx( *b2 );

		vector<FCVertex>::iterator b1, e1;
		for( b1 = availParent1.begin(), e1 = availParent1.end(), i = 0;
		b1 != e1; ++b1, ++i )
		{
			unsigned int func1ParentIdx = FCG1.GetVertexIdx( *b1 );

			aspMatrix[i][j]	= ppCostMatrix[ func1ParentIdx ][ func2ParentIdx ];
			aspCpy[i][j] = aspMatrix[i][j];
		}

		assert( i == (m-1) );

		// init cosim with func2
		aspMatrix[i][j] = ppCostMatrix[ FCG1.GetVertexIdx(func1) ][ func2ParentIdx ];
		aspCpy[i][j] = aspMatrix[i][j];
	}

	// DEBUG
	//if( ( strcmp( FCG1.GetVertexName( func1 ).c_str(), "adler32" ) == 0 ) &&
	//( strcmp( FCG2.GetVertexName( func2 ).c_str(), "adler32" ) == 0 ) )
	if( m < 8 && n < 8 )
	{

		for( int i = 0; i < m; ++i )
		{
			char buf[256];
			buf[0] = 0;

			for( int j = 0; j < n; ++j )
			{
				char b[20];
				snprintf( b, 20, "\t%d", aspCpy[i][j] );
				strcat( buf, b );
			}

			pLog->info( "%s", buf );
		}
	}
	// GUBED

	BiasCostMatrix( aspMatrix, m, n );
	BiasCostMatrix( aspCpy, m, n );

	// Initialize 2nd Quad (diags to 1, rest to INF)
	for( i = 0; i < m; ++i )
	{
		for( j = n; j < aspDim; ++j )
		{
			aspMatrix[i][j] = INF;
			aspCpy[i][j] = aspMatrix[i][j];
		}
	}
	for( i = 0; i < m; ++i )
	{
		aspMatrix[i][n+i] = 1*COST_PRECISION;
		aspCpy[i][n+i] = aspMatrix[i][n+i];
	}

	// Initialize 3rd Quad (diags to 1, rest to INF)
	for( i = m; i < aspDim; ++i )
	{
		for( j = 0; j < n; ++j )
		{
			aspMatrix[i][j] = INF;
			aspCpy[i][j] = aspMatrix[i][j];
		}
	}
	for( j = 0; j < n; ++j )
	{
		aspMatrix[m+j][j] = 1*COST_PRECISION;
		aspCpy[m+j][j] = aspMatrix[m+j][j];
	}

	// Initialize 4th Quad (dummy to dummy, all to 0)
	for( i = m; i < aspDim; ++i )
	{
		for( j = n; j < aspDim; ++j )
		{
			aspMatrix[i][j] = 0;
			aspCpy[i][j] = 0;
		}
	}

	long *row_mate = new long[aspDim];
	if( row_mate == NULL ) pLog->fatal("row_mate is NULL!");
	long *col_mate = new long[aspDim];
	if( col_mate == NULL ) pLog->fatal("col_mate is NULL!");

	//
	// Apply Munkres
	//
	asp( aspDim, aspMatrix, col_mate, row_mate );

	// For every parent of func1, update the pairing if it's
	// within the 1st quarter, i.e. paired with some parent of func2.
	for( i = 0; i < (m-1); ++i )
	{
		if( col_mate[i] < n  )
		{
			pLog->info( "%s new pairing with %s", 
				FCG1.GetVertexName( availParent1[i] ).c_str(),
				FCG2.GetVertexName( availParent2[col_mate[i]] ).c_str() );
			newpairs[ availParent1[i] ] = availParent2[ col_mate[i] ];
			//InsertMAYPair( availChild1[row_mate[j]], availChild2[j] );
		}
	}

	// Now, for the special case of func1 itself.
	// If func1 is assigned to a parent function parent2,
	// i.e. func1-parent2, choose the better match between
	// parent1-parent2 and func1-parent2.
	//
	cost fn1BestCost = ppCostMatrix[ FCG1.GetVertexIdx(func1) ][ FCG2.GetVertexIdx(func2) ];
	if( col_mate[m-1] < n )
	{
		if( aspCpy[m-1][col_mate[m-1]] < fn1BestCost )
		{
			fn1BestCost = aspCpy[m-1][col_mate[m-1]];

			newpairs[ func1 ] = availParent2[ col_mate[m-1] ];

			// Reassign func2's pairing.
			//DeleteMAYPair( func1, func2 ); // Delete old pairing
			//InsertMAYPair( availChild1[ row_mate[n-1] ] , func2 ); // Insert new pairing
		}
	}


	/*
	// Compute average cost
	unsigned int count = 0;
	cost totalCost = 0;
	for( i = 0; i < m; ++i )
	{
		if( col_mate[i] != (n-1))
		{
			totalCost += aspCpy[i][col_mate[i]];
			count++;
		}
	}
	totalCost += fn2BestCost;
	count++;

	float aveCost = (((float) totalCost)/((float) count)) / ((float) COST_PRECISION);
	*/

	// Free aspMatrix
	for( i = 0; i < aspDim; ++i )
	{
		delete [] aspMatrix[i];
		delete [] aspCpy[i];
	}
	delete [] aspMatrix;
	delete [] aspCpy;
	delete [] row_mate;
	delete [] col_mate;
}


/*!
The Munkres cost is computed over the cosim values between the
current functions and the immediate targets (children).
*/
void Aligner::CalcMunkresChild(
	FCGraph &FCG1,
	FCVertex &func1,
	FCGraph &FCG2,
	FCVertex &func2,
	map<FCVertex, FCVertex> &newpairs )
{
	// Apply Munkres on children of func1 against
	// func2 and all its children.

	unsigned int i, j;

	vector<FCVertex> availChild1;
	vector<FCVertex> availChild2;

	if( out_degree( func1, FCG1 ) == 0 )
	{
		pLog->info( "1 %s has NO child.", FCG1.GetVertexName(func1).c_str() );
		return;
	}

	if( out_degree( func2, FCG2 ) == 0 )
	{
		pLog->info( "2 %s has NO child.", FCG2.GetVertexName(func2).c_str() );
		return;
	}

	// Extract children that are available, i.e. not in IS_pair.
	GetAvailChildren( FCG1, func1, IS_Pairs_fwd, availChild1 );
	if( availChild1.size() == 0 )
	{
		pLog->info( "1 %s has NO AVAIL child.", FCG1.GetVertexName(func1).c_str() );
		return;
	}

	GetAvailChildren( FCG2, func2, IS_Pairs_rev, availChild2 );
	if( availChild2.size() == 0 )
	{
		pLog->info( "2 %s has AVAIL child.", FCG2.GetVertexName(func2).c_str() );
		return;
	}

	// DEBUG - Just printing available children...
	pLog->info( "1 %s has %zu avail children.",
		FCG1.GetVertexName(func1).c_str(), availChild1.size() );
	vector<FCVertex>::iterator b3, e3;
	for( b3 = availChild1.begin(), e3 = availChild1.end();
	b3 != e3; ++b3 )
	{
		pLog->info("\t%s", FCG1.GetVertexName(*b3).c_str());
	}
	pLog->info( "2 %s has %zu avail children.",
		FCG2.GetVertexName(func2).c_str(), availChild2.size() );
	for( b3 = availChild2.begin(), e3 = availChild2.end();
	b3 != e3; ++b3 )
	{
		pLog->info("\t%s", FCG2.GetVertexName(*b3).c_str());
	}
	// GUBED

	// Init matrix
	unsigned int m = availChild1.size();
	unsigned int n = availChild2.size() + 1;
	unsigned int aspDim = m + n;
	
	cost **aspMatrix = new cost*[aspDim];
	cost **aspCpy = new cost*[aspDim];	// Make a copy. Original will be altered.
	for( i = 0; i < aspDim; ++i )
	{
		aspMatrix[i] = new cost[aspDim];
		aspCpy[i] = new cost[aspDim];
	}
	

	// Initialize 1st Quad
	vector<FCVertex>::iterator b1, e1;
	for( b1 = availChild1.begin(), e1 = availChild1.end(), i = 0;
	b1 != e1; ++b1, ++i )
	{
		unsigned int func1ChildIdx = FCG1.GetVertexIdx( *b1 );

		// init cosim with func2's children
		vector<FCVertex>::iterator b2, e2;
		for( b2 = availChild2.begin(), e2 = availChild2.end(), j = 0;
		b2 != e2; ++b2, ++j )
		{
			unsigned int func2ChildIdx = FCG2.GetVertexIdx( *b2 );
			aspMatrix[i][j]	= ppCostMatrix[ func1ChildIdx ][ func2ChildIdx ];
			aspCpy[i][j] = aspMatrix[i][j];
		}

		assert( j == (n-1) );

		// init cosim with func2
		aspMatrix[i][j] = ppCostMatrix[ func1ChildIdx ][ FCG2.GetVertexIdx(func2) ];
		aspCpy[i][j] = aspMatrix[i][j];
	}


	// DEBUG
	//if( ( strcmp( FCG1.GetVertexName( func1 ).c_str(), "adler32" ) == 0 ) &&
	//( strcmp( FCG2.GetVertexName( func2 ).c_str(), "adler32" ) == 0 ) )
	if( m < 8 && n < 8 )
	{

		for( int i = 0; i < m; ++i )
		{
			char buf[256];
			buf[0] = 0;

			for( int j = 0; j < n; ++j )
			{
				char b[20];
				snprintf( b, 20, "\t%d", aspCpy[i][j] );
				strcat( buf, b );
			}

			pLog->info( "%s", buf );
		}
	}
	// GUBED

	BiasCostMatrix( aspMatrix, m, n );
	BiasCostMatrix( aspCpy, m, n );

	// Initialize 2nd Quad (diags to 1, rest to INF)
	for( i = 0; i < m; ++i )
	{
		for( j = n; j < aspDim; ++j )
		{
			aspMatrix[i][j] = INF;
			aspCpy[i][j] = aspMatrix[i][j];
		}
	}
	for( i = 0; i < m; ++i )
	{
		aspMatrix[i][n+i] = 1*COST_PRECISION;
		aspCpy[i][n+i] = aspMatrix[i][n+i];
	}

	// Initialize 3rd Quad (diags to 1, rest to INF)
	for( i = m; i < aspDim; ++i )
	{
		for( j = 0; j < n; ++j )
		{
			aspMatrix[i][j] = INF;
			aspCpy[i][j] = aspMatrix[i][j];
		}
	}
	for( j = 0; j < n; ++j )
	{
		aspMatrix[m+j][j] = 1*COST_PRECISION;
		aspCpy[m+j][j] = aspMatrix[m+j][j];
	}

	// Initialize 4th Quad (dummy to dummy, all to 0)
	for( i = m; i < aspDim; ++i )
	{
		for( j = n; j < aspDim; ++j )
		{
			aspMatrix[i][j] = 0;
			aspCpy[i][j] = 0;
		}
	}

	long *row_mate = new long[aspDim];
	if( row_mate == NULL ) pLog->fatal("row_mate is NULL!");
	long *col_mate = new long[aspDim];
	if( col_mate == NULL ) pLog->fatal("col_mate is NULL!");

	//
	// Apply Munkres
	//
	asp( aspDim, aspMatrix, col_mate, row_mate );

	// For every child of func2, update the pairing if it's
	// within the 1st quarter, i.e. paired with some child of func1.
	for( j = 0; j < (n-1); ++j )
	{
		if( row_mate[j] < m  )
		{
			pLog->info( "%s new pairing with %s",
				FCG1.GetVertexName( availChild1[row_mate[j]] ).c_str(),
				FCG2.GetVertexName( availChild2[j] ).c_str() );
			newpairs[ availChild1[ row_mate[j] ] ] = availChild2[j];
			//InsertMAYPair( availChild1[row_mate[j]], availChild2[j] );
		}
	}

	// Now, for the special case of func2 itself.
	// If func2 is assigned to a child function child1,
	// i.e. func2-child1, choose the better match between
	// func1-func2 and func2-child1.
	//
	cost fn2BestCost = ppCostMatrix[ FCG1.GetVertexIdx(func1) ][ FCG2.GetVertexIdx(func2) ];
	if( row_mate[n-1] < m )
	{
		if( aspCpy[row_mate[n-1]][n-1] < fn2BestCost )
		{
			fn2BestCost = aspCpy[row_mate[n-1]][n-1];

			newpairs[ availChild1[ row_mate[n-1] ] ] = func2;

			// Reassign func2's pairing.
			//DeleteMAYPair( func1, func2 ); // Delete old pairing
			//InsertMAYPair( availChild1[ row_mate[n-1] ] , func2 ); // Insert new pairing
		}
	}


	/*
	// Compute average cost
	unsigned int count = 0;
	cost totalCost = 0;
	for( i = 0; i < m; ++i )
	{
		if( col_mate[i] != (n-1))
		{
			totalCost += aspCpy[i][col_mate[i]];
			count++;
		}
	}
	totalCost += fn2BestCost;
	count++;

	float aveCost = (((float) totalCost)/((float) count)) / ((float) COST_PRECISION);
	*/

	// Free aspMatrix
	for( i = 0; i < aspDim; ++i )
	{
		delete [] aspMatrix[i];
		delete [] aspCpy[i];
	}
	delete [] aspMatrix;
	delete [] aspCpy;
	delete [] row_mate;
	delete [] col_mate;

}

void Aligner::CalcMunkresHelper(
	FCGraph &FCG1,
	FCVertex &V1,
	FCGraph &FCG2,
	FCVertex &V2,
	list< cost > &prevCosts )
{

	map< FCVertex, FCVertex > newpairs;

	pLog->info( "CalcMunkresHelper: Checking %s vs %s",
		FCG1.GetVertexName(V1).c_str(),
		FCG2.GetVertexName(V2).c_str() );

	// If the current cosim is above threshold of 0.8*COST_PRECISION,
	// and the previous 3 values too, then stop.
	cost fn1fn2Cost = ppCostMatrix[ FCG1.GetVertexIdx(V1) ][ FCG2.GetVertexIdx(V2) ];
	if( (float)fn1fn2Cost > 0.8*COST_PRECISION )
	{
		bool count = 0;
		list< cost >::iterator b, e;
		for( b = prevCosts.begin(), e = prevCosts.end();
		b != e; ++b )
		{
			if( ((float)*b) > 0.8*COST_PRECISION )
				count++;
		}

		if( count == prevCosts.size() )
		{
			pLog->info( "CalcMunkresHelper: Too many bad results. Stopping.");
			return;
		}
	}

	// Save old cost, remove from list
	// and push in new cost.
	cost oldCost;
	if( prevCosts.size() == 2 )
	{
		oldCost = prevCosts.front();
		prevCosts.pop_front();
	}
	prevCosts.push_back( fn1fn2Cost );

	CalcMunkresParent( FCG1, V1, FCG2, V2, newpairs );
	CalcMunkresChild( FCG1, V1, FCG2, V2, newpairs );

	// Recurse for the new pairings
	map< FCVertex, FCVertex >::iterator b, e;
	for( b = newpairs.begin(), e = newpairs.end();
	b != e; ++b )
	{
		FCVertex newfunc1 = b->first;
		FCVertex newfunc2 = b->second;

		if( HasISPair( newfunc1, newfunc2 ) ||
		HasMAYPair( newfunc1, newfunc2 ) ||
		(float)ppCostMatrix[ FCG1.GetVertexIdx(newfunc1) ][ FCG2.GetVertexIdx(newfunc2) ] > 0.8*COST_PRECISION )
			continue;
	
		// If pairing doesn't exist, add pairing and recurse.
		if( InsertBestOfMAYPair( FCG1, newfunc1, FCG2, newfunc2 ) == 0 )
		{
			pLog->info( "New pair: %s, %s",
				FCG1.GetVertexName( newfunc1 ).c_str(),
				FCG2.GetVertexName( newfunc2 ).c_str() );
	
			CalcMunkresHelper( FCG1, newfunc1, FCG2, newfunc2, prevCosts );
		}
	}

	// Restore old cost
	if( prevCosts.size() == 2 ) prevCosts.push_front( oldCost );
	prevCosts.pop_back();
}

void Aligner::CalcPairedCost( FCGraph &FCG1, FCGraph &FCG2 )
{
	// For each pair, calculate the child.

	map< FCVertex, set<FCVertex> >::iterator b1, e1;

	list< cost > prevCosts;

	for( b1 = IS_Pairs_fwd.begin(), e1 = IS_Pairs_fwd.end();
	b1 != e1; ++b1 )
	{
		FCVertex func1 = b1->first;
		pLog->info( "%s has %zu mappings.",
			FCG1.GetVertexName(func1).c_str(),
			b1->second.size() );	

		FCVertex func2 = *(b1->second.begin());
		CalcMunkresHelper( FCG1, func1, FCG2, func2, prevCosts );
	}

}



float Aligner::ComputeAlignDist2( FCGraph &FCG1, FCGraph &FCG2 )
{
	pLog->info("ComputeAlignDist2");
	InitCostMatrix( FCG1, FCG2 );
	pLog->info("CalcPairedCost");
	CalcPairedCost( FCG1, FCG2 );


	// Prints out all the pairings
	pLog->info( "Printing IS pairing");
	map< FCVertex, set<FCVertex> >::iterator b1, e1;
	for( b1 = IS_Pairs_fwd.begin(), e1 = IS_Pairs_fwd.end();
	b1 != e1; ++b1 )
	{
		pLog->info( "%s is paired with", FCG1.GetVertexName( b1->first ).c_str() );

		set<FCVertex>::iterator b2, e2;
		for( b2 = b1->second.begin(), e2 = b1->second.end();
		b2 != e2; ++b2 )
		{
			pLog->info( "\t%s", FCG2.GetVertexName( *b2 ).c_str() );
		}
	}

	// Prints out all the pairings
	pLog->info( "Printing MAY pairing");
	for( b1 = MAY_Pairs_fwd.begin(), e1 = MAY_Pairs_fwd.end();
	b1 != e1; ++b1 )
	{
		pLog->info( "%s is paired with", FCG1.GetVertexName( b1->first ).c_str() );

		set<FCVertex>::iterator b2, e2;
		for( b2 = b1->second.begin(), e2 = b1->second.end();
		b2 != e2; ++b2 )
		{
			//pLog->info( "ppCostMatrix is at %08x. idx1 %d idx2 %d",
			//	ppCostMatrix,
			//	FCG1.GetVertexIdx(b1->first),
			//	FCG2.GetVertexIdx(*b2) );
			pLog->info( "\t%f %s",
				(float)ppCostMatrix[FCG1.GetVertexIdx(b1->first)][FCG2.GetVertexIdx(*b2)] / COST_PRECISION,
				FCG2.GetVertexName( *b2 ).c_str() );
		}
	}

	//FCG2.PrintVertices();
	//FCG2.PrintEdges();
	char dotname[256];
	snprintf(dotname, sizeof(dotname), "%s.dot", FCG2.GetGraphName().c_str());
	FCG2.WriteDOT(dotname);	

	return 0;
}	




//====================================================================================================
RexFunc *Aligner::MakeRexFunc( FCGraph &FCG, FCVertex V )
{
	vector<BasicBlock*> *bbs = FCG.GetVertexBBs(V);
	if( bbs == NULL ) return NULL;

	RexFunc *rexfunc = new RexFunc( rexLog, FCG.GetVertexFuncId(V), FCG.GetVertexStartEA(V) );

	vector<BasicBlock*>::iterator b, e;
	for( b = bbs->begin(), e = bbs->end();
	b != e; ++b )
	{
		BasicBlock *bb = *b;
		//pLog->info( "bb %08x startEA %08x endEA %08x size %zu",
		//	bb, bb->startEA, bb->endEA, bb->size );
		rexfunc->AddBB( bb->insns.bin,
			bb->size,
			bb->startEA,
			bb->nextbb1,
			bb->nextbb2 );
	}

	if( rexfunc->FinalizeBBs() != 0 )
	{
		delete rexfunc;
		return NULL;
	}

	return rexfunc;
}

REXSTATUS Aligner::DoSymbolicExec(
	FCGraph &FCG1,
	FCGraph &FCG2,
	FCVertex v1,
	FCVertex v2 )
{
	REXSTATUS res;

	RexFunc *rexfunc1 = MakeRexFunc( FCG1, v1 );
	if( rexfunc1 == NULL )
		return REX_ERR_UNK;

	RexFunc *rexfunc2 = MakeRexFunc( FCG2, v2 );
	if( rexfunc2 == NULL )
	{
		delete rexfunc1;
		return REX_ERR_UNK;
	}

	Rex rex( rexLog, rexutils1, rexutils2, rextrans );

	#ifdef USE_STP_FILE
	char stpprefix[256];
	snprintf( stpprefix, sizeof(stpprefix), "%s_%s",
		FCG1.GetVertexName(v1).c_str(),
		FCG2.GetVertexName(v2).c_str() );
	if( rexfunc1->GetNumPaths() >= rexfunc2->GetNumPaths() )
		res = rex.SemanticEquiv( stpprefix, *rexfunc1, *rexfunc2, QUERY_WAIT_SECONDS );
	else
		res = rex.SemanticEquiv( stpprefix, *rexfunc2, *rexfunc1, QUERY_WAIT_SECONDS );
	#else

	if( rexfunc1->GetNumPaths() >= rexfunc2->GetNumPaths() )
		res = rex.SemanticEquiv( *rexfunc1, *rexfunc2, QUERY_WAIT_SECONDS );
	else
		res = rex.SemanticEquiv( *rexfunc2, *rexfunc1, QUERY_WAIT_SECONDS );
	#endif

	delete rexfunc1;
	delete rexfunc2;

	return res;
}




void Aligner::ComputeSmallFuncDist( FCGraph &FCG1, FCGraph &FCG2 )
{
	FCVertex v1;
	FCVertex v2;

	
	v1 = FCG1.GetFirstVertex();
	while( v1 != FCG_NULL_VERTEX )
	{
		if( FCG1.GetVertexSize( v1 ) >= SMALL_FUNC_LIMIT ||
		FCG1.GetVertexNumBBs( v1 ) == 0 )
		{
			//pLog->info( "Skipping 1: \"%s\" size %zu #bb %zu",
			//	FCG1.GetVertexName(v1).c_str(),
			//	FCG1.GetVertexSize(v1),
			//	FCG1.GetVertexNumBBs(v1) );
			v1 = FCG1.GetNextVertex();
			continue;
		}

		v2 = FCG2.GetFirstVertex();
		while( v2 != FCG_NULL_VERTEX )
		{
			if( FCG2.GetVertexSize( v2 ) >= SMALL_FUNC_LIMIT ||
			FCG2.GetVertexNumBBs( v2 ) == 0 )
			{
				//pLog->info( "Skipping 2: \"%s\" size %zu #bb %zu",
				//	FCG2.GetVertexName(v2).c_str(),
				//	FCG2.GetVertexSize(v2),
				//	FCG2.GetVertexNumBBs(v2) );
				v2 = FCG2.GetNextVertex();
				continue;
			}

			int symres = DoSymbolicExec(
				FCG1, FCG2, v1, v2 );

			pLog->info( "SYMEXE: %s, %s: %d",
				FCG1.GetVertexName(v1).c_str(),
				FCG2.GetVertexName(v2).c_str(),
				symres );
			//FCG2.GetVertexName( *b3 ).c_str()

			v2 = FCG2.GetNextVertex();
			pLog->info("Got next V2");
		}

		v1 = FCG1.GetNextVertex();
		pLog->info("Got next V1");
	}

}	


//=======================================================================================
/*!
Apply Munkres on func1 and all of its parents against func2's parents.
*/
cost Aligner::GetParentPairCost(
	FCGraph &FCG1,
	FCVertex &func1,
	FCGraph &FCG2,
	FCVertex &func2,
	int depth )
{
	// Fill cost matrix with every parents pairing
	// Compute Munkres
	// Return parent pairing cost

	unsigned int i, j;

	// Init matrix
	unsigned int m = in_degree( func1, FCG1 ) + 1;
	if( m == 0 ) return ppCostMatrix[ FCG1.GetVertexIdx(func1) ][ FCG2.GetVertexIdx(func2) ];

	unsigned int n = in_degree( func2, FCG2 );
	if( n == 0 ) return ppCostMatrix[ FCG1.GetVertexIdx(func1) ][ FCG2.GetVertexIdx(func2) ];;

	unsigned int aspDim = m + n;
	
	cost **aspMatrix = new cost*[aspDim];
	cost **aspCpy = new cost*[aspDim]; // Make a copy. Original will be altered.
	for( i = 0; i < aspDim; ++i )
	{
		aspMatrix[i] = new cost[aspDim];
		aspCpy[i] = new cost[aspDim];
	}
	

	// Initialize 1st Quad
	// init cosim with func2's parents
	FCInEdgeItr b2, e2;
	for( tie(b2, e2) = in_edges( func2, FCG2 ), j = 0;
	b2 != e2; ++b2, ++j )
	{
		FCVertex F2Parent = source( *b2, FCG2 );

		FCInEdgeItr b1, e1;
		for( tie(b1, e1) = in_edges( func1, FCG1), i = 0;
		b1 != e1; ++b1, ++i )
		{
			FCVertex F1Parent = source( *b1, FCG1 );

			aspCpy[i][j]
				= aspMatrix[i][j]
				= GetPairCost( FCG1, F1Parent, FCG2, F2Parent, depth+1 );
		}

		assert( i == (m-1) );
		aspCpy[i][j] = aspMatrix[i][j] = GetPairCost( FCG1, func1, FCG2, F2Parent, depth+1 );
	}

	// DEBUG
	//if( ( strcmp( FCG1.GetVertexName( func1 ).c_str(), "adler32" ) == 0 ) &&
	//( strcmp( FCG2.GetVertexName( func2 ).c_str(), "adler32" ) == 0 ) )
	/*
	if( m < 8 && n < 8 )
	{

		for( int i = 0; i < m; ++i )
		{
			char buf[256];
			buf[0] = 0;

			for( int j = 0; j < n; ++j )
			{
				char b[20];
				snprintf( b, 20, "\t%d", aspCpy[i][j] );
				strcat( buf, b );
			}

			pLog->info( "%s", buf );
		}
	}
	*/
	// GUBED

	BiasCostMatrix( aspMatrix, m, n );
	BiasCostMatrix( aspCpy, m, n );

	// Initialize 2nd Quad (diags to 1, rest to INF)
	for( i = 0; i < m; ++i )
		for( j = n; j < aspDim; ++j )
			aspCpy[i][j] = aspMatrix[i][j] = INF;
	for( i = 0; i < m; ++i )
		aspCpy[i][n+i] = aspMatrix[i][n+i] = 1*COST_PRECISION;

	// Initialize 3rd Quad (diags to 1, rest to INF)
	for( i = m; i < aspDim; ++i )
		for( j = 0; j < n; ++j )
			aspCpy[i][j] = aspMatrix[i][j] = INF;
	for( j = 0; j < n; ++j )
	{
		aspCpy[m+j][j] = aspMatrix[m+j][j] = 1*COST_PRECISION;
	}

	// Initialize 4th Quad (dummy to dummy, all to 0)
	for( i = m; i < aspDim; ++i )
		for( j = n; j < aspDim; ++j )
			aspCpy[i][j] = aspMatrix[i][j] = 0;

	long *row_mate = new long[aspDim];
	if( row_mate == NULL ) pLog->fatal("row_mate is NULL!");
	long *col_mate = new long[aspDim];
	if( col_mate == NULL ) pLog->fatal("col_mate is NULL!");

	// Apply Munkres
	asp( aspDim, aspMatrix, col_mate, row_mate );

	// Sum up the matches
	cost totalCost = 0;
	for( i = 0; i < m; ++i )
	{
		totalCost += aspCpy[i][col_mate[i]];
	}

	// Free aspMatrix
	for( i = 0; i < aspDim; ++i )
	{
		delete [] aspMatrix[i];
		delete [] aspCpy[i];
	}
	delete [] aspMatrix;
	delete [] aspCpy;
	delete [] row_mate;
	delete [] col_mate;

	return (cost)floor((float)totalCost / (float)m); 
}

/*!
Apply Munkres on children of func1 against func2 and all its children.
*/
cost Aligner::GetChildPairCost(
	FCGraph &FCG1,
	FCVertex &func1,
	FCGraph &FCG2,
	FCVertex &func2,
	int depth )
{
	// Fill cost matrix with every children pairing
	// Compute Munkres
	// Return child pairing cost

	unsigned int i, j;

	// Init matrix
	unsigned int m = out_degree( func1, FCG1 );
	if( m == 0 ) return ppCostMatrix[ FCG1.GetVertexIdx(func1) ][ FCG2.GetVertexIdx(func2) ];
	unsigned int n = out_degree( func2, FCG2 ) + 1;
	if( n == 1 ) return ppCostMatrix[ FCG1.GetVertexIdx(func1) ][ FCG2.GetVertexIdx(func2) ];

	unsigned int aspDim = m + n;
	
	cost **aspMatrix = new cost*[aspDim];
	cost **aspCpy = new cost*[aspDim];	// Make a copy. Original will be altered.
	for( i = 0; i < aspDim; ++i )
	{
		aspMatrix[i] = new cost[aspDim];
		aspCpy[i] = new cost[aspDim];
	}
	

	// Initialize 1st Quad
	FCOutEdgeItr b1, e1;
	for( tie(b1, e1) = out_edges( func1, FCG1 ), i = 0;
	b1 != e1; ++b1, ++i )
	{
		FCVertex F1Child = target( *b1, FCG1 );
	
		// init cosim with func2's children
		FCOutEdgeItr b2, e2;
		for( tie(b2, e2) = out_edges( func2, FCG2 ), j = 0;
		b2 != e2; ++b2, ++j )
		{
			FCVertex F2Child = target( *b2, FCG2 );

			aspCpy[i][j]
				= aspMatrix[i][j]
				= GetPairCost( FCG1, F1Child, FCG2, F2Child, depth+1 );
		}

		assert( j == (n-1) );
		aspCpy[i][j] = aspMatrix[i][j] = GetPairCost( FCG1, F1Child, FCG2, func2, depth+1 );
	}


	// DEBUG
	//if( ( strcmp( FCG1.GetVertexName( func1 ).c_str(), "adler32" ) == 0 ) &&
	//( strcmp( FCG2.GetVertexName( func2 ).c_str(), "adler32" ) == 0 ) )
	/*
	if( m < 8 && n < 8 )
	{
		for( int i = 0; i < m; ++i )
		{
			char buf[256];
			buf[0] = 0;

			for( int j = 0; j < n; ++j )
			{
				char b[20];
				snprintf( b, 20, "\t%d", aspCpy[i][j] );
				strcat( buf, b );
			}
			pLog->info( "%s", buf );
		}
	}
	*/
	// GUBED

	BiasCostMatrix( aspMatrix, m, n );
	BiasCostMatrix( aspCpy, m, n );

	// Initialize 2nd Quad (diags to 1, rest to INF)
	for( i = 0; i < m; ++i )
		for( j = n; j < aspDim; ++j )
			aspCpy[i][j] = aspMatrix[i][j] = INF;
	for( i = 0; i < m; ++i )
		aspCpy[i][n+i] = aspMatrix[i][n+i] = 1*COST_PRECISION;

	// Initialize 3rd Quad (diags to 1, rest to INF)
	for( i = m; i < aspDim; ++i )
		for( j = 0; j < n; ++j )
			aspCpy[i][j] = aspMatrix[i][j] = INF;
	for( j = 0; j < n; ++j )
		aspCpy[m+j][j] = aspMatrix[m+j][j] = 1*COST_PRECISION;

	// Initialize 4th Quad (dummy to dummy, all to 0)
	for( i = m; i < aspDim; ++i )
		for( j = n; j < aspDim; ++j )
			aspCpy[i][j] = aspMatrix[i][j] = 0;

	long *row_mate = new long[aspDim];
	if( row_mate == NULL ) pLog->fatal("row_mate is NULL!");
	long *col_mate = new long[aspDim];
	if( col_mate == NULL ) pLog->fatal("col_mate is NULL!");

	// Apply Munkres
	asp( aspDim, aspMatrix, col_mate, row_mate );

	// Sum up the matches
	cost totalCost = 0;
	for( i = 0; i < m; ++i )
	{
		totalCost += aspCpy[i][col_mate[i]];
	}

	// Free aspMatrix
	for( i = 0; i < aspDim; ++i )
	{
		delete [] aspMatrix[i];
		delete [] aspCpy[i];
	}
	delete [] aspMatrix;
	delete [] aspCpy;
	delete [] row_mate;
	delete [] col_mate;

	return (cost)floor((float)totalCost / (float)m); 
}

cost Aligner::GetPairCost(
	FCGraph &FCG1,
	FCVertex &func1,
	FCGraph &FCG2,
	FCVertex &func2,
	int depth )
{
	int fn1Idx = FCG1.GetVertexIdx(func1);
	int fn2Idx = FCG2.GetVertexIdx(func2);

	// If explore status is PENDING, just return cosim value.
	// If explore status is COMPLETED, return final value.
	switch( exploreStatus[fn1Idx][fn2Idx] )
	{
	case PENDING: return ppCostMatrix[fn1Idx][fn2Idx];
	case COMPLETED: return finalCostMatrix[fn1Idx][fn2Idx];
	default:; // Do nothing
	}

	cost curCost = ppCostMatrix[ fn1Idx ][ fn2Idx ];
	if( curCost == 1*COST_PRECISION )
	{	// These two functions can't possibly match.
		return curCost;
	}

	// Don't want to go too deep for 2 reasons.
	// 1. Speed
	// 2. No necessity.
	if( depth > 3 )
	{
		return curCost;
	}

	// Mark explore status as PENDING
	exploreStatus[fn1Idx][fn2Idx] = PENDING;

	// If a pairing for either function is found, but the
	// pairing is not between the two functions, then
	// the two functions are paired to something else
	// and cannot be paired.
	map< FCVertex, set<FCVertex> >::iterator pair1Found, pair2Found;
	pair1Found = IS_Pairs_fwd.find(func1);
	pair2Found = IS_Pairs_rev.find(func2);
	if( ( pair1Found != IS_Pairs_fwd.end() &&
	pair1Found->second.size() > 0 &&
	pair1Found->second.find( func2 ) == pair1Found->second.end() ) ||
	( pair2Found != IS_Pairs_rev.end() &&
	pair2Found->second.size() > 0 &&
	pair2Found->second.find( func1 ) == pair2Found->second.end() ) )
	{	// func1 is IS-paired with something not func2
		// or vice-versa.
		curCost = 1*COST_PRECISION;
	}

	// Get parent pairing cost
	cost parCost = GetParentPairCost( FCG1, func1, FCG2, func2, depth );

	// Get child pairing cost
	cost childCost = GetChildPairCost( FCG1, func1, FCG2, func2, depth );
	
	// DEBUG
	//pLog->info( "GetPairCost: %s, %s : %d, %d, %d",
	//	FCG1.GetVertexName(func1).c_str(), FCG2.GetVertexName(func2).c_str(),
	//	parCost, childCost, curCost );
	// GUBED

	// Return averaged cost
	cost retCost = (cost)floor( (float) (parCost + childCost + curCost) / (float) 3 ) ;

	// Mark explore status as PENDING
	exploreStatus[fn1Idx][fn2Idx] = COMPLETED;
	finalCostMatrix[fn1Idx][fn2Idx] = retCost;
	return retCost;
}

float Aligner::ComputeAlignDist3( FCGraph &FCG1, FCGraph &FCG2 )
{
	pLog->info("File %d vs File %d", FCG1.GetGraphFileId(), FCG2.GetGraphFileId() );
	InitCostMatrix( FCG1, FCG2 );

	// For each MAY-pairing...

	map< FCVertex, set< pair< FCVertex, cost > > >::iterator b1, e1;
	set< pair<FCVertex, cost> >::iterator b2, e2;
	for( b1 = smallCosims.begin(), e1 = smallCosims.end();
	b1 != e1; ++b1 )
	{
		FCVertex func1 = b1->first;

		for( b2 = b1->second.begin(), e2 = b1->second.end();
		b2 != e2; ++b2 )
		{
			FCVertex func2 = b2->first;

			//pLog->info( "GetPairCost: %d\t%s, %s",
			//	b2->second,
			//	FCG1.GetVertexName(func1).c_str(),
			//	FCG2.GetVertexName(func2).c_str() );

			cost pairCost = GetPairCost( FCG1, func1, FCG2, func2, 0 );
			//pLog->info( "ComputeAlignDist3: %d %s %s",
			//	pairCost,
			//	FCG1.GetVertexName(func1).c_str(),
			//	FCG2.GetVertexName(func2).c_str() );
		}
	}

	// Extract minimum values for each vertex
	unsigned int *rowmin = new unsigned int[ num_vertices(FCG1) ];
	for( int i = 0; i < num_vertices( FCG1 ); ++i )
	{
		rowmin[i] = num_vertices(FCG2);

		cost minCost = 1*COST_PRECISION;
		for( int j = 0; j < num_vertices(FCG2); ++j )
		{
			if( finalCostMatrix[i][j] < minCost )
			{
				minCost = finalCostMatrix[i][j];
				rowmin[i] = j;
			}
		}
	}


	unsigned int *colmin = new unsigned int[ num_vertices(FCG2) ];
	for( int j = 0; j < num_vertices( FCG2 ); ++j )
	{
		colmin[j] = num_vertices(FCG1);

		cost minCost = 1*COST_PRECISION;
		for( int i = 0; i < num_vertices(FCG1); ++i )
		{
			if( finalCostMatrix[i][j] < minCost )
			{
				minCost = finalCostMatrix[i][j];
				colmin[j] = i;
			}
		}
	}
	

	map< FCVertex, FCVertex > finalMapFwd;
	map< FCVertex, FCVertex > finalMapRev;
	for( int i = 0; i < num_vertices(FCG1); ++i )
	{
		// Discard functions that are too small
		//if( FCG1.GetVertexSize( FCG1.GetVertexByIdx(i) ) < MIN_FUNC_SIZE )
		//	continue;

		for( int j = 0; j < num_vertices(FCG2); ++j )
		{
			// Discard functions that are too small
			//if( FCG2.GetVertexSize( FCG2.GetVertexByIdx(j) ) < MIN_FUNC_SIZE )
			//	continue;

			if( finalCostMatrix[i][j] != UNINIT_FINAL_COST &&
			rowmin[i] == j && colmin[j] == i )
			{
				finalMapFwd[ FCG1.GetVertexByIdx(i) ] = FCG2.GetVertexByIdx(j);
				finalMapRev[ FCG2.GetVertexByIdx(i) ] = FCG1.GetVertexByIdx(j);

				//pLog->info( "FINAL: %d, %s, %s",
				//	finalCostMatrix[i][j],
				//	FCG1.GetVertexName( FCG1.GetVertexByIdx(i) ).c_str(),
				//	FCG2.GetVertexName( FCG2.GetVertexByIdx(j) ).c_str() );
			}
		}
	}

	map<FCVertex, int> group;
	unsigned int ngroups;
	unsigned int *groupcount = new unsigned int[ num_vertices(FCG1) ];
	memset( groupcount, 0x0, sizeof( unsigned int )*num_vertices(FCG1) );
	FormGroups( FCG1, FCG2, finalMapFwd, finalMapRev, group, &ngroups, groupcount );

	int curCost;
	int totalCost = 0;
	map< FCVertex, FCVertex >::iterator b3, e3;
	for( b3 = finalMapFwd.begin(), e3 = finalMapFwd.end();
	b3 != e3; ++b3 )
	{
		curCost = (int)ceil((float)finalCostMatrix[ FCG1.GetVertexIdx(b3->first) ][ FCG2.GetVertexIdx(b3->second) ] / (float)groupcount[ group[ b3->first ] ]);
		pLog->info( "FINAL: %d, %s, %s, %d, %d, %d",
			curCost,
			FCG1.GetVertexName( b3->first ).c_str(),
			FCG2.GetVertexName( b3->second ).c_str(),
			finalCostMatrix[ FCG1.GetVertexIdx(b3->first) ][ FCG2.GetVertexIdx(b3->second) ],
			group[b3->first],
			groupcount[group[b3->first]] );


		FCG2.SetVertexWeight( b3->second , ((float)curCost)/((float)COST_PRECISION) );
		FCG2.SetVertexPaired( b3->second, FCG1.GetVertexName(b3->first) );

		totalCost += curCost;
	}
	
	float finalCost = ((float) totalCost / (float)finalMapFwd.size()) / (float) (1*COST_PRECISION);
	if( IS_Pairs_fwd.size() > 0 )
		finalCost /= (float) IS_Pairs_fwd.size();

		
	delete groupcount;
	delete rowmin;
	delete colmin;

	//pLog->info( "TotalCost: %d\nSize: %zu\nPrecision: %d\nFinalCost: %f\n",
	//	totalCost, finalMapFwd.size(), 1*COST_PRECISION, finalCost );	

	char dotname[256];
	snprintf(dotname, sizeof(dotname), "%s.dot", FCG2.GetGraphName().c_str());
	FCG2.WriteDOT(dotname);	

	return finalCost;
}


void Aligner::FormGroups(
	FCGraph &FCG1,
	FCGraph &FCG2,
	map<FCVertex, FCVertex> &finalMapFwd,
	map<FCVertex, FCVertex> &finalMapRev,
	map<FCVertex, int> &group,
	unsigned int *ngroups,
	unsigned int *groupcount )
{
	unsigned int groupid = 0;


	map<FCVertex, FCVertex>::iterator b1, e1;
	for( b1 = finalMapFwd.begin(), e1 = finalMapFwd.end();
	b1 != e1; ++b1 )
	{
		int usegrpid = groupid;
		FCVertex W1 = b1->first;
		FCVertex V1 = b1->second;

		map<FCVertex, int>::iterator findgrp;
		findgrp = group.find( W1 );
		if( findgrp == group.end() )
			usegrpid = groupid++;

		// For each parent of mapped vertex in FCG2
		FCInEdgeItr inB1, inE1;
		for( tie( inB1, inE1 ) = in_edges( V1, FCG2 );
		inB1 != inE1; ++inB1 )
		{
			map<FCVertex, FCVertex>::iterator revfind;
			revfind = finalMapRev.find( source(*inB1, FCG2) );
			if( revfind != finalMapRev.end() )
			{
				// Checks if any of the child is the vertex at W1
				FCVertex W2 = revfind->second;

				FCOutEdgeItr outB1, outE1;
				for( tie(outB1, outE1) = out_edges( W2, FCG1 );
				outB1 != outE1; ++outB1 )
				{
					if( target( *outB1, FCG1 ) == W1 )
					{
						group[W2] = usegrpid;
						break;
					}
				}

				break;
			}
		}
	
		// For each child of mapped vertex in FCG2
		FCOutEdgeItr outB2, outE2;
		for( tie( outB2, outE2 ) = out_edges( V1, FCG2 );
		outB2 != outE2; ++outB2 )
		{
			map<FCVertex, FCVertex>::iterator revfind;
			revfind = finalMapRev.find( target(*outB2, FCG2) );
			if( revfind != finalMapRev.end() )
			{
				// Checks if any of the parent is the vertex at W1
				FCVertex W3 = revfind->second;

				FCInEdgeItr inB2, inE2;
				for( tie(inB2, inE2) = in_edges( W3, FCG1 );
				inB2 != inE2; ++inB2 )
				{
					if( source( *inB2, FCG1 ) == W1 )
					{
						group[W3] = usegrpid;
						break;
					}
				}

				break;
			}
		}

		if( findgrp == group.end() )
		{
			groupcount[usegrpid]++;
			group[ W1 ] = usegrpid;
		}
	}

	// DEBUG
	/*
	*ngroups = groupid;
	map<FCVertex, int>::iterator b3, e3;
	for( unsigned int i = 0; i < groupid; ++i )
	{
		for( b3 = group.begin(), e3 = group.end();
		b3 != e3; ++b3 )
		{
			if( b3->second == i )
			{
				pLog->info( "GROUP: %d\t%s",
					i, FCG1.GetVertexName(b3->first).c_str() );
			}
		}
	}
	*/
	// GUBED
}
