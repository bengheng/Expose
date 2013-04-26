#include <math.h>
#include <gsl/gsl_eigen.h>
#include <gsl/gsl_errno.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_sys.h>
#include "../utils/utils.h"
#ifdef USE_ASP
#include "asp.h"
#else
#include "../gsl_hungarian/hungarian.h"
#endif
#include <iostream>
#include <fstream>
#include <boost/format.hpp>
#include <armadillo>
#include <sys/time.h>

/*!
Quicksort in descending order.
*/
int QuickSort(gsl_vector * const V,
              gsl_vector * const order,
              int start,
              int end)
{
	float pivot, ordpiv;
	unsigned int i, j;

	if(start < end)
	{
		pivot = gsl_vector_get(V, start);
		ordpiv = gsl_vector_get(order, start);
		i = start;
		j = end;
		while(i != j)
		{
			if(gsl_vector_get(V, j) > pivot)
			{
				gsl_vector_set(V, i, gsl_vector_get(V, j));
				gsl_vector_set(V, j, gsl_vector_get(V, i+1));
				gsl_vector_set(order, i, gsl_vector_get(order, j));
				gsl_vector_set(order, j, gsl_vector_get(order, i+1));
				i = i+1;
			}
			else
			{
				j = j - 1;
			}
		}

		gsl_vector_set(V, i, pivot);
		gsl_vector_set(order, i, ordpiv);

		QuickSort(V, order, start, i-1);
		QuickSort(V, order, i+1, end);
	}
	return GSL_SUCCESS;
}

int QuickSort_Arma(arma::fvec * const V,
                   arma::fvec * const order,
                   int start,
                   int end)
{
	float pivot, ordpiv;
	unsigned int i, j;

	if(start < end)
	{
		pivot = (*V)(start);//gsl_vector_get(V, start);
		ordpiv = (*order)(start);//gsl_vector_get(order, start);
		i = start;
		j = end;
		while(i != j)
		{
			if((*V)(j) > pivot)
			{
				//gsl_vector_set(V, i, gsl_vector_get(V, j));
				//gsl_vector_set(V, j, gsl_vector_get(V, i+1));
				//gsl_vector_set(order, i, gsl_vector_get(order, j));
				//gsl_vector_set(order, j, gsl_vector_get(order, i+1));
				(*V)(i) = (*V)(j);
				(*V)(j) = (*V)(i+1);
				(*order)(i) = (*order)(j);
				(*order)(j) = (*order)(i+1);
				i = i+1;
			}
			else
			{
				j = j - 1;
			}
		}

		//gsl_vector_set(V, i, pivot);
		//gsl_vector_set(order, i, ordpiv);
		(*V)(i) = pivot;
		(*order)(i) = ordpiv;

		QuickSort_Arma(V, order, start, i-1);
		QuickSort_Arma(V, order, i+1, end);
	}
	return 0;
}
/*!
Returns true if the matrix has >0 NaN element.
*/
bool HasNaN(const gsl_matrix_complex *const M)
{
	gsl_complex z;
	for(int i = 0; i < M->size1; ++i)
	{
		for(int j = 0; j < M->size2; ++j)
		{
			z = gsl_matrix_complex_get(M, i, j);

			if(gsl_isnan(GSL_REAL(z)) || gsl_isnan(GSL_IMAG(z)))
				return true;
		}
	}

	return false;
}

int ComputeUnitaryMatrix_Arma(gsl_matrix_complex * const M,
                         gsl_matrix * const U)
{
	int result;
	arma::fvec *eval, *order;		//gsl_vector *eval, *order;
	arma::cx_fmat *evec;			//gsl_matrix_complex *evec;
						//gsl_eigen_hermv_workspace *w;
	arma::cx_fmat *M2;
	unsigned int i, j;
	//gsl_complex z;
	//float real, imag;

	// Allocations
	//eval = gsl_vector_alloc(M->size1);
	//order = gsl_vector_alloc(M->size1);
	//evec = gsl_matrix_complex_alloc(M->size1, M->size1);
	//w = gsl_eigen_hermv_alloc(M->size1);
	eval = new arma::fvec(M->size1);
	order = new arma::fvec(M->size1);
	evec = new arma::cx_fmat(M->size1, M->size2);
	M2 = new arma::cx_fmat(M->size1, M->size2);

	#ifdef VERBOSE
	PrintGSLMatrixComplex(M, "Computing Unitary Matrix for M");
	#endif
	//result = gsl_eigen_hermv(M, eval, evec, w);

	// Copy M to M2
	for(i = 0; i < M->size1; ++i)
		for(j = 0; j < M->size2; ++j)
			{
				gsl_complex z = gsl_matrix_complex_get(M, i, j);
				arma::cx_float z2(GSL_REAL(z), GSL_IMAG(z));
				(*M2)(i, j) = z2;
			}

	arma::eig_sym(*eval, *evec, *M2);
	#ifdef VERBOSE
	fprintf(stderr, "Result from gsl_eigen_hermv = %d\n", result);
	#endif
	/*
	if(result != GSL_SUCCESS)
	{
		fprintf(stderr, "ERROR %d when calling gsl_eigen_hermv()!\n", result);
		return -1;
	}
	if(HasNaN(evec) == true)
		return -1;
	*/

	// Get ascending order of values in eval
	// PrintGSLVector(eval, "eval (BEFORE)");
	for(i = 0; i < order->n_rows; i++)
		//gsl_vector_set(order, i, i);
		(*order)(i) = i;
	//QuickSort(eval, order, 0, order->size-1);
	QuickSort_Arma(eval, order, 0, order->n_rows-1);

	#ifdef VERBOSE
	PrintGSLVector(eval, "eval (AFTER)");
	PrintGSLVector(order, "order (AFTER)");	
	PrintGSLMatrixComplex(evec, "evec");
	#endif

	for(i = 0; i < U->size1; i++)
	{
		for(j = 0; j < U->size2; j++)
		{
			//z = gsl_matrix_complex_get(evec, i, gsl_vector_get(order,j));
			//real = GSL_REAL(z);
			//imag = GSL_IMAG(z);
			arma::cx_float z2 = (*evec)(i, j);
			gsl_matrix_set(U, i, j, sqrt(z2.real()*z2.real() + z2.imag()*z2.imag()));
		}
	}	

	// Free
	//gsl_eigen_hermv_free(w);
	//gsl_matrix_complex_free(evec);
	//gsl_vector_free(order);
	//gsl_vector_free(eval);
	delete evec;
	delete order;
	delete eval;
	delete M2;

	return GSL_SUCCESS;
}

int ComputeUnitaryMatrix_GSL(gsl_matrix_complex * const M,
                         gsl_matrix * const U)
{
	int result;
	gsl_vector *eval, *order;
	gsl_matrix_complex *evec;
	gsl_eigen_hermv_workspace *w;
	unsigned int i, j;
	gsl_complex z;
	float real, imag;

	// Allocations
	eval = gsl_vector_alloc(M->size1);
	order = gsl_vector_alloc(M->size1);
	evec = gsl_matrix_complex_alloc(M->size1, M->size1);
	w = gsl_eigen_hermv_alloc(M->size1);

	#ifdef VERBOSE
		PrintGSLMatrixComplex(M, "Computing Unitary Matrix for M");
	#endif
	result = gsl_eigen_hermv(M, eval, evec, w);
	#ifdef VERBOSE
	fprintf(stderr, "Result from gsl_eigen_hermv = %d\n", result);
	#endif
	if(result != GSL_SUCCESS)
	{
		fprintf(stderr, "ERROR %d when calling gsl_eigen_hermv()!\n", result);
		return -1;
	}
	if(HasNaN(evec) == true)
		return -1;

	// Get ascending order of values in eval
	// PrintGSLVector(eval, "eval (BEFORE)");
	for(i = 0; i < order->size; i++)
		gsl_vector_set(order, i, i);
	QuickSort(eval, order, 0, order->size-1);
	
	#ifdef VERBOSE
		PrintGSLVector(eval, "eval (AFTER)");
		PrintGSLVector(order, "order (AFTER)");	
		PrintGSLMatrixComplex(evec, "evec");
	#endif

	for(i = 0; i < U->size1; i++)
	{
		for(j = 0; j < U->size2; j++)
		{
			z = gsl_matrix_complex_get(evec, i, gsl_vector_get(order,j));
			real = GSL_REAL(z);
			imag = GSL_IMAG(z);
			gsl_matrix_set(U, i, j, sqrt(real*real + imag*imag));
		}
	}	

	// Free
	gsl_eigen_hermv_free(w);
	gsl_matrix_complex_free(evec);
	gsl_vector_free(order);
	gsl_vector_free(eval);

	return GSL_SUCCESS;
}

/*!
Computes permutation matrix.
*/
int ComputePermMatrix(gsl_matrix * const UA,
                      gsl_matrix * const UB,
                      gsl_matrix * const P)
{
	gsl_matrix_transpose(UA);

	#ifdef VERBOSE
		PrintGSLMatrix(UA, "WA");
		PrintGSLMatrix(UB, "WB");
	#endif

	MulGSLMatrices(UB, UA, P);
	#ifdef VERBOSE
		PrintGSLMatrix(P, "Permutation Matrix");
	#endif

	return GSL_SUCCESS;
}

int EigenDecomp( gsl_matrix_complex * const AE,
                 gsl_matrix_complex * const BE,
                 gsl_matrix * const P )
{
	gsl_matrix *UA = NULL, *UB = NULL;
	int res = 0;

	if( AE == NULL || BE == NULL || P == NULL )
		return GSL_EINVAL;

	if( AE->size1 != AE->size2 ||
            BE->size1 != BE->size2 ||
            AE->size1 != BE->size1 )
		return GSL_EINVAL;

	// Allocations
	UA = gsl_matrix_alloc(AE->size1, AE->size1);
	UB = gsl_matrix_alloc(BE->size1, BE->size1);

	#ifdef USE_ARMA
	res = ComputeUnitaryMatrix_Arma(AE, UA);
	#else
	res = ComputeUnitaryMatrix_GSL(AE, UA);
	#endif
	if( res == -1 ) goto _exit;
	#ifdef VERBOSE
	PrintGSLMatrix(UA, "Unitary A");
	#endif
	

	#ifdef USE_ARMA
	res = ComputeUnitaryMatrix_GSL(BE, UB);
	#else
	res = ComputeUnitaryMatrix(BE, UB);
	#endif
	if( res == -1 ) goto _exit;
	#ifdef VERBOSE
	PrintGSLMatrix(UB, "Unitary B");
	#endif

	ComputePermMatrix(UA, UB, P);

_exit:
	// Free
	if(UA != NULL) gsl_matrix_free(UA);
	if(UB != NULL) gsl_matrix_free(UB);
	
	return res;
}

int ComputeHermitianMatrix(const gsl_matrix *const M,
                           gsl_matrix_complex * const E,
			   gsl_matrix *const S,
			   gsl_matrix *const N)
{
	unsigned int i, j;
	gsl_complex z;

	// S = (M + M^T)/2
	gsl_matrix_memcpy(S, M);
	gsl_matrix_transpose(S);
	gsl_matrix_add(S, M);
	gsl_matrix_scale(S, 0.5);

	// N = (M - M^T)/2
	gsl_matrix_memcpy(N, M);
	gsl_matrix_transpose(N);
	gsl_matrix_scale(N, -1);
	gsl_matrix_add(N, M);
	gsl_matrix_scale(N, 0.5);

	for(i = 0; i < M->size1; i++)
	{
		for(j = 0 ; j < M->size2; j++)
		{
			GSL_SET_COMPLEX(&z,
			    gsl_matrix_get(S, i, j),
			    gsl_matrix_get(N, i, j));

			gsl_matrix_complex_set(E, i, j, z);
		}
	}


	return GSL_SUCCESS;
}

float CalcVariance(const gsl_matrix *const M)
{
	int i, j;
	float mean, dev, variance;

	// Calculate the mean
	mean = 0;
	for(i = 0; i < M->size1; ++i)
		for(j = 0; j < M->size2; ++j)
			mean += gsl_matrix_get(M, i, j);
	mean /= (M->size1 * M->size2);

	// Calculate the variance
	for(i = 0; i < M->size1; ++i)
		for(j = 0; j < M->size2; ++j)
		{
			dev = gsl_matrix_get(M, i, j) - mean;
			variance += (dev * dev);
		}
	
	variance /= (M->size1, M->size2);

	return variance;
}

float CalcEuclideanNorm(const gsl_matrix * const A,
			const gsl_matrix * const B,
			gsl_matrix * const R1,
			gsl_matrix * const R2,
			const gsl_matrix * const Assg,
			const gsl_matrix * const AssgT)
{
	unsigned int i, j;
	float score = 0;
	float variance;

	// Use AssgT as Assg for multiplication
	MulGSLMatrices(Assg, A, R1);
	// PrintGSLMatrix(A, "1 - A");
	// PrintGSLMatrix(R, "1 - R = Assg x A");

	// Compute ScoreMatrix
	MulGSLMatrices(R1, AssgT, R2);
	// PrintGSLMatrix(AssgT, "AssgT");
	// PrintGSLMatrix(R, "2 - R = Assg x A x AssgT");

	gsl_matrix_sub(R2, B);
	// PrintGSLMatrix(B, "3 - B");
	// PrintGSLMatrix(R, "3 - R = Assg x A x AssgT - B");
	
	// Compute variance (for normalization)
	variance = CalcVariance(R2);

	for(i = 0; i < R2->size1; i++)
	{
		for(j = 0; j < R2->size2; j++)
		{
			float val = gsl_matrix_get(R2, i, j);
			// fprintf(stderr, "[%d, %d] = %f\n", i, j, val);
			//if(variance == (float)0)
				score += val*val;
			//else
			//	score += ((val*val)/variance);
			//score += val*val;
		}
	}
	//score = sqrt(score);

	return score;
}

float CalcScore(const gsl_matrix * const AS,
		const gsl_matrix * const AN,
                const gsl_matrix * const BS,
		const gsl_matrix * const BN,
                const gsl_matrix * const Assg,
		const gsl_matrix * const AssgT)
{
	unsigned int i, j;
	float score = 0;
	gsl_matrix *R1 = NULL, *R2 = NULL;

	// Allocate result matrices R1 and R2.
	// These are "workspace" matrices.
	// We pre-allocate them before calling
	// CalcEuclideanNorm so that we don't
	// have to allocate them twice.
	R1 = gsl_matrix_alloc(Assg->size1, Assg->size2);
	if(R1 == NULL) goto _exit;
	R2 = gsl_matrix_alloc(Assg->size1, Assg->size2);
	if(R2 == NULL) goto _exit;


	score = CalcEuclideanNorm(AS, BS, R1, R2, Assg, AssgT);
	score += CalcEuclideanNorm(AN, BN, R1, R2, Assg, AssgT);

_exit:
	if(R1 != NULL)	gsl_matrix_free(R1);
	if(R2 != NULL)	gsl_matrix_free(R2);

	return score;
}

/*!
Returns true if the input matrix is Hermitian.
*/
bool IsHermitian(const gsl_matrix_complex * const M)
{
	gsl_matrix_complex *N;
	gsl_complex zm, zn;
	int i, j;
	bool isHerm = true;

	N = gsl_matrix_complex_alloc(M->size1, M->size2);

	for(i = 0; i < M->size1; ++i)
	{
		for(j = 0; j < M->size2; ++j)
		{
			zm = gsl_matrix_complex_get(M, i, j);
			GSL_SET_COMPLEX(&zn, GSL_REAL(zm), ((-1)*GSL_IMAG(zm)));
			gsl_matrix_complex_set(N, i, j, zn);
		}
	}

	gsl_matrix_complex_transpose(N);

	for(i = 0; i < M->size1; ++i)
	{
		for(j = 0; j < M->size2; ++j)
		{
			zm = gsl_matrix_complex_get(M, i, j);
			zn = gsl_matrix_complex_get(N, i, j);

			if( GSL_REAL(zm) != GSL_REAL(zn) ||
			    GSL_IMAG(zm) != GSL_IMAG(zn) )
			{
				isHerm = false;
				goto _exit;
			}
		}
	}

_exit:
	gsl_matrix_complex_free(N);

	return isHerm;
}

void WriteComplexMatrixToFile(const gsl_matrix_complex *const M)
{
	std::ofstream outfile;
	gsl_complex z;

	outfile.open("buginput.txt", std::ios::trunc);

	// Writes matrix size
	outfile << M->size1 << "," << M->size2 << std::endl;

	// Writes matrix data
	for(int i = 0; i < M->size1; ++i)
		for(int j = 0; j < M->size2; ++j)
		{
			z = gsl_matrix_complex_get(M, i, j);
			outfile << GSL_REAL(z) << "," << GSL_IMAG(z) << std::endl;
		}

	outfile.close();

	fprintf(stderr, "Written matrix to file.\n");
}

int Umeyama(const gsl_matrix * const A,
            const gsl_matrix * const B,
            gsl_matrix * const Assignment,
            float *score)
{
	int res = 0;
	gsl_matrix_complex *AE = NULL, *BE = NULL;
	gsl_matrix	*P  = NULL,
			*AS = NULL,
			*BS = NULL,
			*AN = NULL,
			*BN = NULL,
			*AssignmentT = NULL;
	#ifdef USE_ASP
	long *col_mate;
	long *row_mate;
	cost **pTempCost;
	int i, j;
	#endif
	struct timeval start, end;
	long mtime, seconds, useconds;

	#ifdef VERBOSE
	PrintGSLMatrix(A, "Umeyama A");
	PrintGSLMatrix(B, "Umeyama B");
	#endif

	AE = gsl_matrix_complex_alloc(A->size1, A->size2);	if(AE == NULL) goto _exit;
	AS = gsl_matrix_alloc(A->size1, A->size2);		if(AS == NULL) goto _exit;
	AN = gsl_matrix_alloc(A->size1, A->size2);		if(AN == NULL) goto _exit;
	ComputeHermitianMatrix(A, AE, AS, AN);
	#ifdef VERBOSE
	PrintGSLMatrix(AS, "AS");
	PrintGSLMatrix(AN, "AN");
	#endif
	if( IsHermitian(AE) == false )
	{
		fprintf(stderr, "FATAL: AE is not Hermitian!\n");
		exit(0);
	}
	#ifdef VERBOSE
	fprintf(stderr, "Verified AE is Hermitian.\n");
	#endif

	BE = gsl_matrix_complex_alloc(B->size1, B->size2);	if(BE == NULL) goto _exit;
	BS = gsl_matrix_alloc(B->size1, B->size2);		if(BS == NULL) goto _exit;
	BN = gsl_matrix_alloc(B->size1, B->size2);		if(BN == NULL) goto _exit;
	ComputeHermitianMatrix(B, BE, BS, BN);
	#ifdef VERBOSE
	PrintGSLMatrix(BS, "BS");
	PrintGSLMatrix(BN, "BN");
	#endif
	if( IsHermitian(BE) == false )
	{
		fprintf(stderr, "FATAL: BE is not Hermitian!\n");
		exit(0);
	}
	#ifdef VERBOSE
	fprintf(stderr, "Verified BE is Hermitian.\n");
	WriteComplexMatrixToFile(BE);
	PrintGSLMatrixComplex(AE, "AE");
	PrintGSLMatrixComplex(BE, "BE");
	#endif
	
	P = gsl_matrix_alloc(A->size1, A->size2);		if(P == NULL) goto _exit;
	res = EigenDecomp(AE, BE, P);
	if( res == -1 ) goto _exit;
	#ifdef VERBOSE
	PrintGSLMatrix(P, "P");
	PrintGSLMatrix(P, "Computing Hungarian");
	#endif

	// Begin timing Hungarian
	gettimeofday(&start, NULL);
	#ifdef USE_ASP
	col_mate = new long[P->size1];
	row_mate = new long[P->size1];

	pTempCost = new cost*[P->size1];
	for(i = 0; i < P->size1; ++i)
		pTempCost[i] = new cost[P->size2];

	for(i = 0; i < P->size1; ++i)
		for(j = 0; j < P->size2; ++j)
			pTempCost[i][j] = gsl_matrix_get(P, i, j);

	asp(P->size1, pTempCost, col_mate, row_mate);

	for(i = 0; i < P->size1; ++i)
		delete[] pTempCost[i];
	delete[] pTempCost;

	// Update assignment matrix
	for(i = 0; i < P->size1; ++i)
		gsl_matrix_set(Assignment, i, col_mate[i], 1);

	delete[] col_mate;
	delete[] row_mate;

	#else
	Hungarian(P, true, Assignment);
	#endif
	// End timing Hungarian
	gettimeofday(&end, NULL);
	seconds = end.tv_sec - start.tv_sec;
	useconds = end.tv_usec - start.tv_usec;
	mtime = ((seconds) * 1000 + useconds/1000.0) + 0.5;
	fprintf(stderr, "%ld ms\t", mtime);

	// PrintGSLMatrixUint(Assignment, "Assignment");

	// Allocate and initialize AssgtT
	AssignmentT = gsl_matrix_alloc(Assignment->size1, Assignment->size2);
	if(AssignmentT == NULL) goto _exit;
	gsl_matrix_memcpy(AssignmentT, Assignment);
	gsl_matrix_transpose(AssignmentT);

	/*
	PrintGSLMatrix(AS, "AS");
	PrintGSLMatrix(AN, "AN");
	PrintGSLMatrix(BS, "BS");
	PrintGSLMatrix(BN, "BN");
	*/
	*score = CalcScore(AS, AN, BS, BN, Assignment, AssignmentT);

_exit:
	if(P != NULL)		gsl_matrix_free(P);
	if(AS != NULL)		gsl_matrix_free(AS);
	if(AN != NULL)		gsl_matrix_free(AN);
	if(BS != NULL)		gsl_matrix_free(BS);
	if(BN != NULL)		gsl_matrix_free(BN);
	if(AE != NULL)		gsl_matrix_complex_free(AE);
	if(BE != NULL)		gsl_matrix_complex_free(BE);
	if(AssignmentT != NULL)	gsl_matrix_free(AssignmentT);

	return res;
}
