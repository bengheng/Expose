#include <sys/time.h>
#include <stdio.h>
#include <gsl/gsl_errno.h>
#include <gsl/gsl_complex.h>
#include <gsl/gsl_matrix.h>
#include "../utils/utils.h"
#include "umeyama.h"


#define GSL_COMPLEX_SET(M,row,col,z,r,i)		\
		GSL_SET_COMPLEX(&z,r, i);		\
		gsl_matrix_complex_set(M,row,col,z);

void testcase1( gsl_matrix_complex **A,
                gsl_matrix_complex **B )
{
	gsl_complex z;

	*A = gsl_matrix_complex_alloc(4, 4);
	GSL_COMPLEX_SET(*A, 0, 0, z, 0, 0);
	GSL_COMPLEX_SET(*A, 0, 1, z, 1.5, 1.5);
	GSL_COMPLEX_SET(*A, 0, 2, z, 2.5, 1.5);
	GSL_COMPLEX_SET(*A, 0, 3, z, 1, 1);
	GSL_COMPLEX_SET(*A, 1, 0, z, 1.5, -1.5);
	GSL_COMPLEX_SET(*A, 1, 1, z, 0, 0);
	GSL_COMPLEX_SET(*A, 1, 2, z, 0.5, 0.5);
	GSL_COMPLEX_SET(*A, 1, 3, z, 1, 1);
	GSL_COMPLEX_SET(*A, 2, 0, z, 2.5, -1.5);
	GSL_COMPLEX_SET(*A, 2, 1, z, 0.5, -0.5);
	GSL_COMPLEX_SET(*A, 2, 2, z, 0, 0);
	GSL_COMPLEX_SET(*A, 2, 3, z, 1, 0);
	GSL_COMPLEX_SET(*A, 3, 0, z, 1, -1);
	GSL_COMPLEX_SET(*A, 3, 1, z, 1, -1);
	GSL_COMPLEX_SET(*A, 3, 2, z, 1, 0);
	GSL_COMPLEX_SET(*A, 3, 3, z, 0, 0);

	*B = gsl_matrix_complex_alloc(4, 4);
	GSL_COMPLEX_SET(*B, 0, 0, z, 0, 0);
	GSL_COMPLEX_SET(*B, 0, 1, z, 2, 2);
	GSL_COMPLEX_SET(*B, 0, 2, z, 1, 1);
	GSL_COMPLEX_SET(*B, 0, 3, z, 2, 2);
	GSL_COMPLEX_SET(*B, 1, 0, z, 2, -2);
	GSL_COMPLEX_SET(*B, 1, 1, z, 0, 0);
	GSL_COMPLEX_SET(*B, 1, 2, z, 1.5, -0.5);
	GSL_COMPLEX_SET(*B, 1, 3, z, 0.5, -0.5);
	GSL_COMPLEX_SET(*B, 2, 0, z, 1, -1);
	GSL_COMPLEX_SET(*B, 2, 1, z, 1.5, 0.5);
	GSL_COMPLEX_SET(*B, 2, 2, z, 0, 0);
	GSL_COMPLEX_SET(*B, 2, 3, z, 2, 0);
	GSL_COMPLEX_SET(*B, 3, 0, z, 2, -2);
	GSL_COMPLEX_SET(*B, 3, 1, z, 0.5, 0.5);
	GSL_COMPLEX_SET(*B, 3, 2, z, 2, 0);
	GSL_COMPLEX_SET(*B, 3, 3, z, 0, 0);
}

// From Umeyama's paper
void testcase2( gsl_matrix **A,
                gsl_matrix **B)
{
	*A = gsl_matrix_alloc(4, 4);
	gsl_matrix_set(*A, 0, 0, 0);
	gsl_matrix_set(*A, 0, 1, 3);
	gsl_matrix_set(*A, 0, 2, 4);
	gsl_matrix_set(*A, 0, 3, 2);
	gsl_matrix_set(*A, 1, 0, 0);
	gsl_matrix_set(*A, 1, 1, 0);
	gsl_matrix_set(*A, 1, 2, 1);
	gsl_matrix_set(*A, 1, 3, 2);
	gsl_matrix_set(*A, 2, 0, 1);
	gsl_matrix_set(*A, 2, 1, 0);
	gsl_matrix_set(*A, 2, 2, 0);
	gsl_matrix_set(*A, 2, 3, 1);
	gsl_matrix_set(*A, 3, 0, 0);
	gsl_matrix_set(*A, 3, 1, 0);
	gsl_matrix_set(*A, 3, 2, 1);
	gsl_matrix_set(*A, 3, 3, 0);

	*B = gsl_matrix_alloc(4, 4);
	gsl_matrix_set(*B, 0, 0, 0);
	gsl_matrix_set(*B, 0, 1, 4);
	gsl_matrix_set(*B, 0, 2, 2);
	gsl_matrix_set(*B, 0, 3, 4);
	gsl_matrix_set(*B, 1, 0, 0);
	gsl_matrix_set(*B, 1, 1, 0);
	gsl_matrix_set(*B, 1, 2, 1);
	gsl_matrix_set(*B, 1, 3, 0);
	gsl_matrix_set(*B, 2, 0, 0);
	gsl_matrix_set(*B, 2, 1, 2);
	gsl_matrix_set(*B, 2, 2, 0);
	gsl_matrix_set(*B, 2, 3, 2);
	gsl_matrix_set(*B, 3, 0, 0);
	gsl_matrix_set(*B, 3, 1, 1);
	gsl_matrix_set(*B, 3, 2, 2);
	gsl_matrix_set(*B, 3, 3, 0);
}

void testcase3( gsl_matrix **A,
                gsl_matrix **B)
{
	*A = gsl_matrix_alloc(4, 4);
	gsl_matrix_set(*A, 0, 0, 0);
	gsl_matrix_set(*A, 0, 1, 1);
	gsl_matrix_set(*A, 0, 2, 1);
	gsl_matrix_set(*A, 0, 3, 0);
	gsl_matrix_set(*A, 1, 0, 0);
	gsl_matrix_set(*A, 1, 1, 0);
	gsl_matrix_set(*A, 1, 2, 0);
	gsl_matrix_set(*A, 1, 3, 1);
	gsl_matrix_set(*A, 2, 0, 0);
	gsl_matrix_set(*A, 2, 1, 0);
	gsl_matrix_set(*A, 2, 2, 0);
	gsl_matrix_set(*A, 2, 3, 0);
	gsl_matrix_set(*A, 3, 0, 0);
	gsl_matrix_set(*A, 3, 1, 0);
	gsl_matrix_set(*A, 3, 2, 0);
	gsl_matrix_set(*A, 3, 3, 0);

	*B = gsl_matrix_alloc(4, 4);
	gsl_matrix_set(*B, 0, 0, 0);
	gsl_matrix_set(*B, 0, 1, 1);
	gsl_matrix_set(*B, 0, 2, 1);
	gsl_matrix_set(*B, 0, 3, 1);
	gsl_matrix_set(*B, 1, 0, 0);
	gsl_matrix_set(*B, 1, 1, 0);
	gsl_matrix_set(*B, 1, 2, 0);
	gsl_matrix_set(*B, 1, 3, 0);
	gsl_matrix_set(*B, 2, 0, 0);
	gsl_matrix_set(*B, 2, 1, 0);
	gsl_matrix_set(*B, 2, 2, 0);
	gsl_matrix_set(*B, 2, 3, 0);
	gsl_matrix_set(*B, 3, 0, 0);
	gsl_matrix_set(*B, 3, 1, 0);
	gsl_matrix_set(*B, 3, 2, 0);
	gsl_matrix_set(*B, 3, 3, 0);
}

void testcase4( gsl_matrix **A,
                gsl_matrix **B)
{
	*A = gsl_matrix_alloc(4, 4);
	gsl_matrix_set(*A, 0, 0, 0);
	gsl_matrix_set(*A, 0, 1, 1);
	gsl_matrix_set(*A, 0, 2, 1);
	gsl_matrix_set(*A, 0, 3, 1);
	gsl_matrix_set(*A, 1, 0, 0);
	gsl_matrix_set(*A, 1, 1, 0);
	gsl_matrix_set(*A, 1, 2, 0);
	gsl_matrix_set(*A, 1, 3, 0);
	gsl_matrix_set(*A, 2, 0, 0);
	gsl_matrix_set(*A, 2, 1, 0);
	gsl_matrix_set(*A, 2, 2, 0);
	gsl_matrix_set(*A, 2, 3, 0);
	gsl_matrix_set(*A, 3, 0, 0);
	gsl_matrix_set(*A, 3, 1, 0);
	gsl_matrix_set(*A, 3, 2, 0);
	gsl_matrix_set(*A, 3, 3, 0);

	*B = gsl_matrix_alloc(4, 4);
	gsl_matrix_set(*B, 0, 0, 0);
	gsl_matrix_set(*B, 0, 1, 1);
	gsl_matrix_set(*B, 0, 2, 1);
	gsl_matrix_set(*B, 0, 3, 0);
	gsl_matrix_set(*B, 1, 0, 0);
	gsl_matrix_set(*B, 1, 1, 0);
	gsl_matrix_set(*B, 1, 2, 0);
	gsl_matrix_set(*B, 1, 3, 0);
	gsl_matrix_set(*B, 2, 0, 0);
	gsl_matrix_set(*B, 2, 1, 0);
	gsl_matrix_set(*B, 2, 2, 0);
	gsl_matrix_set(*B, 2, 3, 1);
	gsl_matrix_set(*B, 3, 0, 0);
	gsl_matrix_set(*B, 3, 1, 0);
	gsl_matrix_set(*B, 3, 2, 0);
	gsl_matrix_set(*B, 3, 3, 0);
}

void testcase5( gsl_matrix **A,
                gsl_matrix **B)
{
	*A = gsl_matrix_alloc(4, 4);
	gsl_matrix_set(*A, 0, 0, 0);
	gsl_matrix_set(*A, 0, 1, 1);
	gsl_matrix_set(*A, 0, 2, 1);
	gsl_matrix_set(*A, 0, 3, 0);
	gsl_matrix_set(*A, 1, 0, 0);
	gsl_matrix_set(*A, 1, 1, 0);
	gsl_matrix_set(*A, 1, 2, 0);
	gsl_matrix_set(*A, 1, 3, 1);
	gsl_matrix_set(*A, 2, 0, 0);
	gsl_matrix_set(*A, 2, 1, 0);
	gsl_matrix_set(*A, 2, 2, 0);
	gsl_matrix_set(*A, 2, 3, 0);
	gsl_matrix_set(*A, 3, 0, 0);
	gsl_matrix_set(*A, 3, 1, 0);
	gsl_matrix_set(*A, 3, 2, 0);
	gsl_matrix_set(*A, 3, 3, 0);

	*B = gsl_matrix_alloc(4, 4);
	gsl_matrix_set(*B, 0, 0, 0);
	gsl_matrix_set(*B, 0, 1, 1);
	gsl_matrix_set(*B, 0, 2, 1);
	gsl_matrix_set(*B, 0, 3, 0);
	gsl_matrix_set(*B, 1, 0, 0);
	gsl_matrix_set(*B, 1, 1, 0);
	gsl_matrix_set(*B, 1, 2, 0);
	gsl_matrix_set(*B, 1, 3, 0);
	gsl_matrix_set(*B, 2, 0, 0);
	gsl_matrix_set(*B, 2, 1, 0);
	gsl_matrix_set(*B, 2, 2, 0);
	gsl_matrix_set(*B, 2, 3, 1);
	gsl_matrix_set(*B, 3, 0, 0);
	gsl_matrix_set(*B, 3, 1, 0);
	gsl_matrix_set(*B, 3, 2, 0);
	gsl_matrix_set(*B, 3, 3, 0);
}

void testcase6( gsl_matrix **A,
                gsl_matrix **B)
{
	*A = gsl_matrix_alloc(4, 4);
	gsl_matrix_set(*A, 0, 0, 0);
	gsl_matrix_set(*A, 0, 1, 1);
	gsl_matrix_set(*A, 0, 2, 1);
	gsl_matrix_set(*A, 0, 3, 0);
	gsl_matrix_set(*A, 1, 0, 1);
	gsl_matrix_set(*A, 1, 1, 0);
	gsl_matrix_set(*A, 1, 2, 0);
	gsl_matrix_set(*A, 1, 3, 0);
	gsl_matrix_set(*A, 2, 0, 0);
	gsl_matrix_set(*A, 2, 1, 0);
	gsl_matrix_set(*A, 2, 2, 0);
	gsl_matrix_set(*A, 2, 3, 0);
	gsl_matrix_set(*A, 3, 0, 0);
	gsl_matrix_set(*A, 3, 1, 0);
	gsl_matrix_set(*A, 3, 2, 0);
	gsl_matrix_set(*A, 3, 3, 0);

	*B = gsl_matrix_alloc(4, 4);
	gsl_matrix_set(*B, 0, 0, 0);
	gsl_matrix_set(*B, 0, 1, 1);
	gsl_matrix_set(*B, 0, 2, 1);
	gsl_matrix_set(*B, 0, 3, 0);
	gsl_matrix_set(*B, 1, 0, 1);
	gsl_matrix_set(*B, 1, 1, 0);
	gsl_matrix_set(*B, 1, 2, 0);
	gsl_matrix_set(*B, 1, 3, 0);
	gsl_matrix_set(*B, 2, 0, 0);
	gsl_matrix_set(*B, 2, 1, 0);
	gsl_matrix_set(*B, 2, 2, 0);
	gsl_matrix_set(*B, 2, 3, 0);
	gsl_matrix_set(*B, 3, 0, 0);
	gsl_matrix_set(*B, 3, 1, 0);
	gsl_matrix_set(*B, 3, 2, 0);
	gsl_matrix_set(*B, 3, 3, 0);
}

void testcase7( gsl_matrix **A,
                gsl_matrix **B)
{
	*A = gsl_matrix_alloc(4, 4);
	gsl_matrix_set(*A, 0, 0, 0);
	gsl_matrix_set(*A, 0, 1, 1);
	gsl_matrix_set(*A, 0, 2, 1);
	gsl_matrix_set(*A, 0, 3, 0);
	gsl_matrix_set(*A, 1, 0, 1);
	gsl_matrix_set(*A, 1, 1, 0);
	gsl_matrix_set(*A, 1, 2, 0);
	gsl_matrix_set(*A, 1, 3, 0);
	gsl_matrix_set(*A, 2, 0, 0);
	gsl_matrix_set(*A, 2, 1, 0);
	gsl_matrix_set(*A, 2, 2, 0);
	gsl_matrix_set(*A, 2, 3, 0);
	gsl_matrix_set(*A, 3, 0, 0);
	gsl_matrix_set(*A, 3, 1, 0);
	gsl_matrix_set(*A, 3, 2, 0);
	gsl_matrix_set(*A, 3, 3, 0);

	*B = gsl_matrix_alloc(4, 4);
	gsl_matrix_set(*B, 0, 0, 0);
	gsl_matrix_set(*B, 0, 1, 0);
	gsl_matrix_set(*B, 0, 2, 1);
	gsl_matrix_set(*B, 0, 3, 1);
	gsl_matrix_set(*B, 1, 0, 0);
	gsl_matrix_set(*B, 1, 1, 0);
	gsl_matrix_set(*B, 1, 2, 0);
	gsl_matrix_set(*B, 1, 3, 0);
	gsl_matrix_set(*B, 2, 0, 0);
	gsl_matrix_set(*B, 2, 1, 0);
	gsl_matrix_set(*B, 2, 2, 0);
	gsl_matrix_set(*B, 2, 3, 0);
	gsl_matrix_set(*B, 3, 0, 0);
	gsl_matrix_set(*B, 3, 1, 0);
	gsl_matrix_set(*B, 3, 2, 0);
	gsl_matrix_set(*B, 3, 3, 0);
}

void testcase8( gsl_matrix **A,
                gsl_matrix **B,
		unsigned int n)
{
	unsigned int i, j;

	srand(time(NULL));

	*A = gsl_matrix_alloc(n, n);
	for(i = 0; i < n; i++)
		for(j = 0; j < n; j++)
			gsl_matrix_set(*A, i, j, rand());

	*B = gsl_matrix_alloc(n, n);
	for(i = 0; i < n; i++)
		for(j = 0; j < n; j++)
			gsl_matrix_set(*B, i, j, rand());

}


int main(int argc, char **argv)
{
	FILE *fid;
	int choice;
	// gsl_matrix_complex *A, *B;
	gsl_matrix *A = NULL, *B = NULL;
	gsl_matrix_uint *Assignment;
	float score;
	unsigned int testcount, steps;
	struct timeval start, stop, elapsed;

	//if((argc != 2 && atoi(argv[1]) != 8) || (argc != 3 && atoi(argv[1]) == 8))
	if(argc != 2)
	{
		printf("Enter test case #.\n");
		return 0;
	}

	fid = fopen("perfd.log", "w");
	if(fid == NULL)
		return 0;

	for(steps = 50; steps <= 1000; steps += 50)
	{
		for(testcount = 0; testcount < 10; testcount++)
		{
			if((gettimeofday(&start, NULL)) == -1)
			{
				perror("gettimeofday");
				exit(1);
			}

			choice = atoi(argv[1]);
			switch(choice)
			{
				case 1:
					//testcase1(&A, &B);
					break;
				case 2:	testcase2(&A, &B); break;
				case 3: testcase3(&A, &B); break;
				case 4: testcase4(&A, &B); break;
				case 5: testcase5(&A, &B); break;
				case 6: testcase6(&A, &B); break;
				case 7: testcase7(&A, &B); break;
				case 8: testcase8(&A, &B, steps); break;
				default:
					printf("Invalid choice.\n");
					return 0;
			}

			if(A != NULL && B != NULL)
			{	
				//PrintGSLMatrixComplex(A, "Input A");
				//PrintGSLMatrixComplex(B, "Input B");
				Assignment = gsl_matrix_uint_alloc(A->size1, A->size2);
				//EigenDecomp(A, B, P);
				score = 0;
				Umeyama(A, B, Assignment, &score);
				//PrintGSLMatrixUint(Assignment, "Assignment");
	
				//printf("SCORE: %f\n", score);
	
				if((gettimeofday(&stop, NULL)) == -1)
				{
					perror("gettimeofday");
					exit(1);
				}

				// Compute elapsed time
				timeval_subtract(&elapsed, &stop, &start);
				fprintf(stderr, "%d\t%d\t%ld\n", steps, testcount, elapsed.tv_sec*1000000 + elapsed.tv_usec);
				fprintf(fid, "%d\t%d\t%ld\n", steps, testcount, elapsed.tv_sec*1000000 + elapsed.tv_usec);
				fflush(stderr); fflush(fid);

				gsl_matrix_uint_free(Assignment);
				//gsl_matrix_complex_free(A);
				//gsl_matrix_complex_free(B);
				gsl_matrix_free(A);
				gsl_matrix_free(B);
			}
		}
	}
	fclose(fid);

	return 0;
}
