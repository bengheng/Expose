#ifndef _UTILS_H_
#define _UTILS_H_

#include <iostream>
#include <string>
#include <algorithm>
#include <time.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_vector.h>

using namespace std;

int MulGSLMatrices( const gsl_matrix * const A,
                    const gsl_matrix * const B,
                    gsl_matrix * const out );

void PrintGSLMatrixComplex(const gsl_matrix_complex * const M,
                           const char * const name);

void PrintGSLMatrixUint(const gsl_matrix_uint * const M,
                        const char * const name);

void PrintGSLMatrix(const gsl_matrix * const M,
                    const char * const name);

void PrintGSLVector(const gsl_vector * const V, const char * const name);

int timeval_subtract(struct timeval * const result,
                     struct timeval * const x,
                     struct timeval * const y);

int LevenshteinDistance(std::string s1, std::string s2);

/*!
Damerau-Levenshtein Edit Distance. An operation is defined as an insertion,
deletion, or substitution of a single character, or a transposition of 2
characters.
*/
template <typename BufType>
int damerau_levenshtein_distance(
	BufType const* A, size_t numElemA,
	BufType const* B, size_t numElemB)
{
	int** d;

	// i and j are used to iterate over A and B
	int i, j, cost, dist;

	// d is a table with lenStr1+1 rows and lenStr2+1 columns
	d = new int*[numElemA+1];
	for(i = 0; i < numElemA+1; ++i)
		d[i] = new int[numElemB+1];

	// for loop is inclusive, need table 1 row/column larger than string length.
	for(i = 0; i <= numElemA; ++i)
		d[i][0] = i;
	for(j = 0; j <= numElemB; ++j)
		d[0][j] = j;

	for (i = 1; i <= numElemA; i++)
		for (int j = 1; j <= numElemB; j++)
		{
			cost = ( A[i-1] == B[j-1] ? 0 : 1);

			// minimum of deletion, insertion and substitution
			d[i][j] = min( (d[i-1][j]+1),
				min( (d[i][j-1]+1), d[i-1][j-1]+cost ));

			if( i > 0 && j > 0 && A[i-1] == B[j - 2] && A[i - 2] == B[j-1])
				d[i][j] = min( d[i][j], (d[i-2][j-2] + cost));	// transposition
		}

	dist = d[numElemA][numElemB];

	for(i = 0; i < numElemA+1; ++i)
		delete d[i];
	delete d;

	return dist;
}

#endif
