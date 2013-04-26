#include <stdbool.h>
#include <float.h>
#include <math.h>
#include <gsl/gsl_vector.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_errno.h>

typedef enum{STEP1, STEP2, STEP3, STEP4, STEP5, STEP6, DONE, ERROR} nextstep;
enum{UNCOVERED=0, COVERED};
enum{UNMASKED=0, STAR, PRIME};

nextstep Step1(gsl_matrix * const M);
nextstep Step2(const gsl_matrix * const M, gsl_matrix_uint * const mask,
               gsl_vector_uint * const rowCov, gsl_vector_uint * const colCov);
nextstep Step3(const gsl_matrix * const M,
               const gsl_matrix_uint * const mask,
               gsl_vector_uint * const colCov);
nextstep Step4(const gsl_matrix * const M,
               const float assignCand,
               gsl_vector_int * const V);
nextstep Step5(gsl_matrix_uint * const mask,
               gsl_matrix_int  * const path,
               gsl_vector_uint * const rowCov,
               gsl_vector_uint * const colCov,
               unsigned int z0_r,
               unsigned int z0_c);
nextstep Step6(gsl_matrix * const M,
               const gsl_vector_uint * const rowCov,
               const gsl_vector_uint * const colCov);

void print_path(const gsl_matrix_int * const path, const gsl_matrix_uint * const mask, const char * const name)
{
	unsigned int i;
	int r, c;

	fprintf(stderr, "%s: ", name == NULL ? "(NULL)":name);	

	fprintf(stderr, "begin");
	for(i = 0; i < path->size1; i++)
	{
		r = gsl_matrix_int_get(path, i, 0);
		c = gsl_matrix_int_get(path, i, 1);  
		if(r != -1 && c != -1)
			fprintf(stderr, " => (%d,%d)[%d]", r, c, gsl_matrix_uint_get(mask, r, c));
	}
	fprintf(stderr, " => end\n");
}

void PrintCover(const gsl_vector_uint * const rowCov,
                const gsl_vector_uint * const colCov,
		const char * const name)
{
	unsigned int i, j;
	unsigned int rCov, cCov;

	fprintf(stderr, "..........................................\n");
	fprintf(stderr, "\t%s\n", name == NULL ? "(NULL)":name);	
	fprintf(stderr, "..........................................\n");

	for(i = 0; i < rowCov->size; i++)
	{
		rCov = gsl_vector_uint_get(rowCov, i);
		for(j = 0; j < colCov->size; j++)
		{
			cCov = gsl_vector_uint_get(colCov, j);
			if( rCov == UNCOVERED && cCov == UNCOVERED )	fprintf(stderr, ".\t");
			else if( rCov == UNCOVERED && cCov == COVERED )	fprintf(stderr, "|\t");
			else if( rCov == COVERED && cCov == UNCOVERED )	fprintf(stderr, "--\t");
			else if( rCov == COVERED && cCov == UNCOVERED )	fprintf(stderr, "+\t");
		}
		fprintf(stderr, "\n");
	}
	fprintf(stderr, "\n");
}

void Matrix_print_with_mask(const gsl_matrix * const M,
                            const gsl_matrix_uint * const mask,
                            const gsl_vector_uint * const rowCov,
                            const gsl_vector_uint * const colCov,
                            const char *const name)
{
	unsigned int i, j, rCov, cCov;

	fprintf(stderr, "====================================================\n");
	fprintf(stderr, "\t\t%s\n", name == NULL ? "(NULL)":name);	
	fprintf(stderr, "====================================================\n");
	for(i = 0; i < M->size1; i++)
	{
		for(j = 0; j < M->size2; j++)
		{
			fprintf(stderr, "%.1f", gsl_matrix_get(M, i, j));
			if(mask != NULL)
			{
				if(gsl_matrix_uint_get(mask, i, j) == STAR)
					fprintf(stderr, "*");
				else if(gsl_matrix_uint_get(mask, i, j) == PRIME)
					fprintf(stderr, "'");
			}
			fprintf(stderr, "\t");
		}
		fprintf(stderr, "\t");

		rCov = gsl_vector_uint_get(rowCov, i);
		for(j = 0; j < M->size2; j++)
		{	
			cCov = gsl_vector_uint_get(colCov, j);
			if( rCov == UNCOVERED && cCov == UNCOVERED )	fprintf(stderr, ".\t");
			else if( rCov == UNCOVERED && cCov == COVERED )	fprintf(stderr, "|\t");
			else if( rCov == COVERED && cCov == UNCOVERED )	fprintf(stderr, "--\t");
			else if( rCov == COVERED && cCov == COVERED )	fprintf(stderr, "+\t");
		}

		fprintf(stderr, "\n");
	}
	fprintf(stderr, "\n");
}

/*!
If necessary, convert the problem from a maximum assignment into a minimum
assignment. We do this by letting C = maximum value in the assignment matrix.
Replace each c_{i,j} with C - c_{i,j}.
*/
int ConvertToMinAsg(gsl_matrix * const M)
{
	float max;
	
	if(M == NULL) return GSL_EINVAL;

	// Find maximum value in matrix
	max = gsl_matrix_max(M);
	for(int i = 0; i < M->size1; ++i)
		for(int j = 0; j < M->size2; ++j)
			// Subtract element values from C
			gsl_matrix_set(M, i, j, max - gsl_matrix_get(M, i, j));

	
	return 0;
}

//==========================================================
//                          STEP 1
//==========================================================

/*!
From each row subtract off the row min.
*/
nextstep Step1(gsl_matrix * const M)
{
	unsigned int i, j;
	float minRow;

	for(i = 0; i < M->size1; i++)
	{
		// Get min value in row i
		minRow = gsl_matrix_get(M, i, 0);
		for(j = 1; j < M->size2; j++)
			if(minRow > gsl_matrix_get(M, i, j))
				minRow = gsl_matrix_get(M, i, j);
		
		// Subtract minRow from all elements in row i
		for(j = 0; j < M->size2; j++)
			gsl_matrix_set(M, i, j, gsl_matrix_get(M, i, j) - minRow);
	}

	return STEP2;
}

//==========================================================
//                          STEP 2
//==========================================================

/*!
Find a zero (Z) in the resulting matrix. If there is no starred zero in its row or column, star Z. Repeat for each element in the matrix. Go to STEP3.
*/
nextstep Step2(const gsl_matrix * const M,
               gsl_matrix_uint * const mask,
               gsl_vector_uint * const rowCov,
               gsl_vector_uint * const colCov)
{
	unsigned int i, j;

	for(i = 0; i < M->size1; i++)
	{
		for(j = 0; j < M->size2; j++)
		{
			if(gsl_vector_uint_get(rowCov, i) == COVERED) break;
                        if(gsl_vector_uint_get(colCov, j) == UNCOVERED &&
			   gsl_matrix_get(M, i, j) == (float) 0 )
			{ 
				gsl_matrix_uint_set(mask, i, j, STAR);
				#ifdef VERBOSE
					fprintf(stderr, "Covering col %d\n", j);
				#endif
				gsl_vector_uint_set(colCov, j, COVERED);
				#ifdef VERBOSE
					fprintf(stderr, "Covering row %d\n", i);
				#endif
				gsl_vector_uint_set(rowCov, i, COVERED);
			}
		}
	}

	gsl_vector_uint_set_all(rowCov, UNCOVERED);
	gsl_vector_uint_set_all(colCov, UNCOVERED);

	return STEP3;
}

//==========================================================
//                          STEP 3
//==========================================================

/*!
Cover each column containing a starred zero. If K columns are covered, the starred zeroes describe a complete set of unique assignments. In this case, go to DONE. Otherwise, go to STEP4.

Once we have searched the entire cost matrix, we count the number of independent zeroes found. If we have found (and starred) K independent zeroes then we are done. If not we proceed to STEP4.
*/
nextstep Step3(const gsl_matrix * const M,
               const gsl_matrix_uint * const mask,
               gsl_vector_uint * const colCov)
{
	unsigned int i, j;

	for(j = 0; j < M->size2; ++j)
	{
		if(gsl_vector_uint_get(colCov, j) == COVERED) continue;

		for(i = 0; i < M->size1; ++i)
		{
			if(gsl_matrix_uint_get(mask, i, j) == STAR)
				gsl_vector_uint_set(colCov, j, COVERED);
		}
	}

	for(j = 0; j < colCov->size; j++)
		if(gsl_vector_uint_get(colCov, j) == UNCOVERED)
			return STEP4;

	return DONE;
}

//==========================================================
//                          STEP 4
//==========================================================

/*!
Find a zero.
*/
void FindAZero(const gsl_matrix * const M,
          const gsl_vector_uint * const rowCov,
          const gsl_vector_uint * const colCov,
          int * const row,
          int * const col)
{
	int i, j;
	bool done;

	*row = -1;
	*col = -1;

	// Don't bother if the minimum value is COVERED
	// since we'll never find an UNCOVERED element.
	// Note this depends on the enum order.
	if(gsl_vector_uint_min(rowCov) == COVERED ||
	   gsl_vector_uint_min(colCov) == COVERED) return;

	#ifdef VERBOSE
	Matrix_print_with_mask(M, NULL, rowCov, colCov, "Finding Zero");
	#endif

	for(i = 0, done = false; i < M->size1 && done == false; ++i)
	{
		if(gsl_vector_uint_get(rowCov, i) != UNCOVERED)
			continue;

		for(j = M->size2-1; j >= 0; --j)
		{
			if( gsl_vector_uint_get(colCov, j) != UNCOVERED )
				continue;

			#ifdef VERBOSE
			fprintf(stderr, "Finding zero at %d, %d\n", i, j);
			#endif


			if(gsl_matrix_get(M, i, j) != (float) 0) continue;
			
			*row = i;
			*col = j;
			done = true;
			break;
		}
	}
}

bool StarInRow(const gsl_matrix_uint * const mask, int row)
{
	unsigned int j;
	for(j = 0; j < mask->size2; ++j)
		if(gsl_matrix_uint_get(mask, row, j) == STAR)
			return true;
	return false;
}

void FindStarInRow(const gsl_matrix_uint * const mask,
                   int row,
                   int * const col)
{
	unsigned int j;

	*col = -1;
	for(j = 0; j < mask->size2; ++j)
		if(gsl_matrix_uint_get(mask, row, j) == STAR)
		{
			*col = j;
			return;
		}
}

/*!
Find a noncovered zero and prime it. If there is no starred zero in the row containing this primed zero, go to STEP5. Otherwise, cover this row and uncover the column containing the starred zero. Continue in this manner until there are no uncovered zeroes left.

Save the smallest uncovered value and go to STEP6.
*/
nextstep Step4(const gsl_matrix * const M,
               gsl_matrix_uint * const mask,
               gsl_vector_uint * const rowCov,
               gsl_vector_uint * const colCov,
               int * const z0_r,
               int * const z0_c)
{
	bool done = false;
	int row, col;
	nextstep next;

	while(done == false)
	{
		FindAZero(M, rowCov, colCov, &row, &col);
		if(row == -1)
		{
			#ifdef VERBOSE
			fprintf(stderr, "Step4: Cannot find zero.\n");
			#endif
			done = true;
			next = STEP6;
		}
		else
		{	
			#ifdef VERBOSE
			fprintf(stderr, "Ste4: Found zero at M[%d, %d]. Setting PRIME.\n", row, col);
			#endif
			gsl_matrix_uint_set(mask, row, col, PRIME);
			if(StarInRow(mask, row) == true)
			{
				FindStarInRow(mask, row, &col);
				gsl_vector_uint_set(rowCov, row, COVERED);
				gsl_vector_uint_set(colCov, col, UNCOVERED);
			}
			else
			{
				done = true;
				next = STEP5;
				*z0_r = row;
				*z0_c = col;
			}
		}
	}

	return next;
}

//==========================================================
//                          STEP 5
//==========================================================
void FindStarInCol(const gsl_matrix_uint * const mask,
                   int * const row,
                   int col)
{
	int i;

	*row = -1;
	for(i = mask->size1-1; i >= 0; --i)
		if(gsl_matrix_uint_get(mask, i, col) == STAR)
		{
			*row = i;
			return;
		}
}

void FindPrimeInRow(const gsl_matrix_uint * const mask,
               int row,
               int * const col)
{
	int j;

	for(j = mask->size2-1; j >= 0; --j)
		if(gsl_matrix_uint_get(mask, row, j) == PRIME)
		{
			*col = j;
			return;
		}
}

void ConvertPath(const gsl_matrix_int * const path,
                 unsigned int count,
                 gsl_matrix_uint * const mask)
{
	int r, c;
	unsigned int i;

	for(i = 0; i < count; i++)
	{
		r = gsl_matrix_int_get(path, i, 0);
		c = gsl_matrix_int_get(path, i, 1);
		if(gsl_matrix_uint_get(mask, r, c) == STAR)
			gsl_matrix_uint_set(mask, r, c, UNMASKED);
		else
			gsl_matrix_uint_set(mask, r, c, STAR);
	}
}

void ErasePrimes(gsl_matrix_uint * const mask)
{
	unsigned i, j;

	for(i = 0; i < mask->size1; i++)
	{
		for(j = 0; j < mask->size2; j++)
		{
			if(gsl_matrix_uint_get(mask, i, j) == PRIME)
				gsl_matrix_uint_set(mask, i, j, UNMASKED);
		}
	}
}

nextstep Step5(gsl_matrix_uint * const mask,
               gsl_matrix_int  * const path,
               gsl_vector_uint * const rowCov,
               gsl_vector_uint * const colCov,
               unsigned int z0_r,
               unsigned int z0_c)
{
	unsigned int count = 0;
	int row, col;

	gsl_matrix_int_set_all(path, -1);

	// Begin with 0' from Step 4	
	gsl_matrix_int_set(path, count, 0, z0_r);
	gsl_matrix_int_set(path, count, 1, z0_c);

	while(count < (path->size1-1))
	{

		
		// Find 0* in col
		FindStarInCol(mask, &row, gsl_matrix_int_get(path, count, 1));
		if(row == -1) break;

		count++;
		gsl_matrix_int_set(path, count, 0, row);
		gsl_matrix_int_set(path, count, 1, gsl_matrix_int_get(path, count-1, 1));
		if(count < (path->size1-1))
		{
			// Find 0' in row
			FindPrimeInRow(mask, gsl_matrix_int_get(path, count, 0), &col);
			count++;
			gsl_matrix_int_set(path, count, 0, gsl_matrix_int_get(path, count-1, 0));
			gsl_matrix_int_set(path, count, 1, col);
		}
	}
	count++;
	ConvertPath(path, count, mask);
	gsl_vector_uint_set_all(rowCov, UNCOVERED);
	gsl_vector_uint_set_all(colCov, UNCOVERED);
	ErasePrimes(mask);

	return STEP3;
}

//==========================================================
//                          STEP 6
//==========================================================

float FindSmallest(const gsl_matrix * const M,
                   const gsl_vector_uint * const rowCov,
                   const gsl_vector_uint * const colCov)
{
	float minval = FLT_MAX;
	unsigned int i, j;

	for(i = 0; i < M->size1; i++)
	{
		if(gsl_vector_uint_get(rowCov, i) != UNCOVERED) continue;
		for(j = 0; j < M->size2; j++)
		{
			if(gsl_vector_uint_get(colCov, j) != UNCOVERED) continue;
			if(minval > gsl_matrix_get(M, i, j))
				minval = gsl_matrix_get(M, i, j);
		}
	}
	return minval;
}

nextstep Step6(gsl_matrix * const M,
               gsl_vector_uint * const rowCov,
               gsl_vector_uint * const colCov)
{
	unsigned int i, j;
	float minval;
	bool hasRowCov;

	minval = FindSmallest(M, rowCov, colCov);
	for(i = 0; i < M->size1; i++)
	{
		hasRowCov = (gsl_vector_uint_get(rowCov, i) == COVERED);
		for(j = 0; j < M->size2; j++)
		{
			if(hasRowCov == true)
				gsl_matrix_set(M, i, j, gsl_matrix_get(M, i, j)+minval);
		
			if(gsl_vector_uint_get(colCov, j) == UNCOVERED)
				gsl_matrix_set(M, i, j, gsl_matrix_get(M, i, j)-minval);
		}
	}

	return STEP4;
}

int UpdateAssignment(const gsl_matrix_uint * const mask,
                     gsl_matrix * const Assignment)
{
	unsigned int i, j;

	gsl_matrix_set_zero(Assignment);
	for(i = 0; i < mask->size1; i++)
	{
		for(j = 0; j < mask->size2; j++)
		{
			if(gsl_matrix_uint_get(mask, i, j) == STAR)
				gsl_matrix_set(Assignment, i, j, 1);
		}
	}
}

/*!
Computes assignment matrix for M.
*/
int Hungarian(const gsl_matrix * const M, const bool convToMinAsg,
              gsl_matrix * const Assignment)
{
	int res, z0_r, z0_c;
	bool done = false;
	unsigned int next = STEP1;
	gsl_vector_uint *rowCov, *colCov;
	gsl_matrix_uint *mask;
	gsl_matrix_int  *path;
	gsl_matrix *MCopy;

	MCopy = gsl_matrix_alloc(M->size1, M->size2);
	gsl_matrix_memcpy(MCopy, M);
	if(convToMinAsg == true)
	{
		res = ConvertToMinAsg(MCopy);
		if(res != GSL_SUCCESS) return res;
	}

	// Allocate memory
	rowCov = gsl_vector_uint_alloc(M->size1);
	colCov = gsl_vector_uint_alloc(M->size2);
	mask = gsl_matrix_uint_alloc(M->size1, M->size2);
	path = gsl_matrix_int_calloc(ceil(((float)(mask->size1*mask->size2))/2), 2 );

	// Initialize
	gsl_vector_uint_set_all(rowCov, UNCOVERED);
	gsl_vector_uint_set_all(colCov, UNCOVERED);
	gsl_matrix_uint_set_all(mask, UNMASKED);

	while(done == false)
	{
		switch(next)
		{
			case STEP1:
				next = Step1(MCopy);
				#ifdef VERBOSE
					Matrix_print_with_mask(MCopy, mask, rowCov, colCov, "Post Step 1");
					PrintCover( rowCov, colCov, "Post Step 1 Cover");
				#endif
				break;
			case STEP2:
				next = Step2(MCopy, mask, rowCov, colCov);
				#ifdef VERBOSE
					Matrix_print_with_mask(MCopy, mask, rowCov, colCov, "Post Step 2");
					PrintCover( rowCov, colCov, "Post Step 2 Cover");
				#endif
				break;
			case STEP3:
				next = Step3(MCopy, mask, colCov);
				#ifdef VERBOSE
					Matrix_print_with_mask(MCopy, mask, rowCov, colCov, "Post Step 3");
					PrintCover( rowCov, colCov, "Post Step 3 Cover");
				#endif
				break;
			case STEP4:
				next = Step4(MCopy, mask, rowCov, colCov, &z0_r, &z0_c);
				#ifdef VERBOSE
					Matrix_print_with_mask(MCopy, mask, rowCov, colCov, "Post Step 4");
					PrintCover( rowCov, colCov, "Post Step 4 Cover");
				#endif
				break;
			case STEP5:
				next = Step5(mask, path, rowCov, colCov, z0_r, z0_c); 
				#ifdef VERBOSE
					Matrix_print_with_mask(MCopy, mask, rowCov, colCov, "Post Step 5");
					PrintCover( rowCov, colCov, "Post Step 5 Cover");
				#endif
				break;
			case STEP6:
				next = Step6(MCopy, rowCov, colCov);
				#ifdef VERBOSE
					Matrix_print_with_mask(MCopy, mask, rowCov, colCov, "Post Step 6");
					PrintCover( rowCov, colCov, "Post Step 6 Cover");
				#endif
				break;
			case DONE:
				#ifdef VERBOSE
					Matrix_print_with_mask(MCopy, mask, rowCov, colCov, "DONE");
				#endif
				UpdateAssignment(mask, Assignment);
				done = true;
				break;
			default:
				done = true;
				fprintf(stderr, "Error!\n");
		}
	}

	// Release memory
	gsl_matrix_free(MCopy);
	gsl_vector_uint_free(rowCov);
	gsl_vector_uint_free(colCov);
	gsl_matrix_uint_free(mask);
	gsl_matrix_int_free(path);

	return GSL_SUCCESS;
}
