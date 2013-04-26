#include <string>
#include <time.h>
#include <gsl/gsl_errno.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_complex.h>
#include "utils.h"

#ifdef __IDP__
#include <ida.hpp>
#include <idp.hpp>
#define PRINT msg
#else
#define PRINT printf
#endif
//==========================================================
// Misc Utilities for GSL matrices
//==========================================================
int MulGSLMatrices( const gsl_matrix * const A,
                    const gsl_matrix * const B,
                    gsl_matrix * const out )
{
	unsigned int i, j, k;
	float sum;
	bool out_eq_in;

	if ( A == NULL || B == NULL || out == NULL ||
	        A->size2 != B->size1 ||
	        out->size1 != A->size1 ||
	        out->size2 != B->size2 )
		return GSL_EINVAL;

	// Does output matrix point to same memory as
	// either of the inputs?
	out_eq_in = ((A == out) || (B == out));

	gsl_matrix *result;
	if (out_eq_in == true)
	{
		result = gsl_matrix_alloc(A->size1, A->size2);
		if (result == NULL) return GSL_ENOMEM;
	}
	else
	{
		// Out matrix is not any of the input matrices.
		// No need to allocate a result matrix.
		result = out;
	}


	for (i = 0; i < A->size1; i++)
	{
		for (k = 0; k < B->size2; k++)
		{
			sum = 0;
			for (j = 0; j < A->size2; j++)
			{
				if (gsl_matrix_get(A, i, j) == 0 || gsl_matrix_get(B, j, k) == 0)
					continue;

				sum += gsl_matrix_get(A, i, j) * gsl_matrix_get(B, j, k);
			}
			gsl_matrix_set(result, i, k, sum);
		}
	}

	// Copy the values and free tmp
	if (out_eq_in == true)
	{
		gsl_matrix_memcpy(out, result);
		gsl_matrix_free(result);
	}

	return GSL_SUCCESS;
}

//==========================================================
// Utilities to print GSL matrices and vectors
//==========================================================
#define PRINT_NAME_HEADER(name)						\
	PRINT("===========================================\n");	\
	PRINT("\t%s\n", name == NULL ? "<NULL>" : name );		\
	PRINT("===========================================\n");

void PrintGSLMatrixComplex(const gsl_matrix_complex * const M,
                           const char * const name)
{
	unsigned int i, j;
	gsl_complex z;

	PRINT_NAME_HEADER(name);

	for (i = 0; i < M->size1; i++)
	{
		for (j = 0; j < M->size2; j++)
		{
			z = gsl_matrix_complex_get(M, i, j);
			if (GSL_REAL(z) == 0)	PRINT(" .  ");
			else			PRINT("%.2f", GSL_REAL(z));

			PRINT(" + ");

			if (GSL_IMAG(z) == 0)	PRINT(" .  ");
			else			PRINT("%.2f\t", GSL_IMAG(z));
			//PRINT("%.2f + %.2f\t", GSL_REAL(z), GSL_IMAG(z));
		}
		PRINT("\n");
	}
	PRINT("\n");
}

void PrintGSLMatrixUint(const gsl_matrix_uint * const M,
                        const char * const name)
{
	unsigned int i, j;

	PRINT_NAME_HEADER(name);

	for (i = 0; i < M->size1; i++)
	{
		for (j = 0; j < M->size2; j++)
		{
			PRINT("%d\t", gsl_matrix_uint_get(M, i, j));
		}
		PRINT("\n");
	}
	PRINT("\n");
}

void PrintGSLMatrix(const gsl_matrix * const M,
                    const char * const name)
{
	unsigned int i, j;

	PRINT_NAME_HEADER(name);

	for (i = 0; i < M->size1; i++)
	{
		for (j = 0; j < M->size2; j++)
		{
			float val = gsl_matrix_get(M, i, j);
			if (val == (float)0)
				PRINT(".\t");
			else
				PRINT("%.2f\t", val);
		}
		PRINT("\n");
	}
	PRINT("\n");
}

void PrintGSLVector(const gsl_vector * const V, const char * const name)
{
	unsigned int i;

	PRINT_NAME_HEADER(name);

	for (i = 0; i < V->size; i++)
	{
		PRINT("%0.2f\t", gsl_vector_get(V, i));
	}
	PRINT("\n");
}

//==========================================================
// Misc Utilities
//==========================================================

/*!
Computes the time difference between timeval x and y.
The result is stored in result.
*/
int timeval_subtract(struct timeval * const result,
                     struct timeval * const x,
                     struct timeval * const y)
{
	// Perform the carry for the later
	// subtraction by updating y.
	if (x->tv_usec < y->tv_usec)
	{
		int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
		y->tv_usec -= 1000000 * nsec;
		y->tv_usec += nsec;
	}

	if (x->tv_usec - y->tv_usec > 1000000)
	{
		int nsec = (x->tv_usec - y->tv_usec) / 1000000;
		y->tv_usec += 1000000 * nsec;
		y->tv_sec -= nsec;
	}

	// Compute the time remaining to wait.
	// tv_usec is certainly positive.
	result->tv_sec = x->tv_sec - y->tv_sec;
	result->tv_usec = x->tv_usec - y->tv_usec;

	// Return 1 if result is negative
	return x->tv_sec < y->tv_sec;
}

/*!
Computes the Levenshtein Distance between the
two strings.

Adapted from:
http://en.wikipedia.org/wiki/Levenshtein_distance
*/
int LevenshteinDistance(std::string s1, std::string s2)
{
	int i, j;
	int m = s1.size()+1;
	int n = s2.size()+1;
	int result;

	int **levdist;
	//levdist = (int**) calloc(m, sizeof(int*));
	//for (i = 0; i < m; ++i)
	//{
	//	levdist[i] = (int*) calloc(n, sizeof(int));
	//}
	levdist = new int*[m];
	for(i = 0; i < m; ++i) levdist[i] = new int[n];

	for (i = 0; i < m; ++i) levdist[i][0] = i;	// deletion
	for (j = 0; j < n; ++j) levdist[0][j] = j;	// insertion

	for (j = 1; j < n; ++j)
	{
		for (i = 1; i < m; ++i)
		{
			if (s1.c_str()[i-1] == s2.c_str()[j-1])
				levdist[i][j] = levdist[i-1][j-1];
			else
			{
				int delcost = levdist[i-1][j] + 1;
				int inscost = levdist[i][j-1] + 1;
				int subcost = levdist[i-1][j-1] + 1;

				int min = delcost;
				if (inscost < min) min = inscost;
				if (subcost < min) min = subcost;

				levdist[i][j] = min;
			}
		}
	}

	result = levdist[m-1][n-1];

	for(i = 0; i < m; ++i) delete[] levdist[i];
	delete[] levdist;

	return result;
}


