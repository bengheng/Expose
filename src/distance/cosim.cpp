#include <iostream>
#include <mysql/mysql.h>
#include <math.h>
#include <cosim.h>
#include <distance.h>
#include <log4cpp/Category.hh>
#include <cstdio>
#include <stdlib.h>
#include <map>

using namespace std;

/*!
 * A = (a1, a2, ..., an)
 * B = (b1, b2, ..., bn)
 * cosine(theta) = ( a1*b1 + a2*b2 + ... + an*bn ) / [sqrt( (a1^2 + a2^2 + ... + an^2)^1/2 ) sqrt( (b1^2 + b2^2 + ... + bn^2 )^1/2 )]
 * */
int GetCosim(
	//MYSQL *conn,
	//const unsigned int expt_id,
	//const unsigned int func1_id,
	//const unsigned int func2_id,
	map<NGRAMID, FREQ>	&func1_freqs,
	map<NGRAMID, FREQ>	&func2_freqs,
	float *cosim,
	log4cpp::Category &log)
{
	map< NGRAMID, FREQ >::iterator	b, e, it;
	unsigned int			numerator = 0;
	unsigned int			denom1 = 0;
	unsigned int			denom2 = 0;

	// Computes denominator for func1,
	// while in the same loop, computes the numerator.
	// The denominator is simply the sum of the squares of
	// each frequency.
	for( b = func1_freqs.begin(), e = func1_freqs.end();
	b != e; ++b )
	{
		denom1 += b->second * b->second;

		// If we can find the same ngram_id in
		// func2_freqs, multiply the frequencies
		// and add to numerator.

		if( ( it = func2_freqs.find(b->first) ) != func2_freqs.end() )
		{
			numerator += b->second * it->second;
		}
	}

	// Computes denominator for func2 by simply summing up
	// the squares of each frequency.
	for( b = func2_freqs.begin(), e = func2_freqs.end();
	b != e; ++b )
	{
		denom2 += b->second * b->second;
	}

	//
	// Compute cosine distance.
	// 0 is nearest.
	// 1 is furthest.
	//

	if( numerator != 0 )
	{
		*cosim = 1 - numerator / ((sqrt((float)denom1)) * (sqrt((float)denom2)));
	}

	
	return 0;
}

