#include <stdio.h>
#include <time.h>
#include <gsl/gsl_matrix.h>
#include "hungarian.h"
#include "../utils/utils.h"
#include <sys/time.h>

void matrix_print(const gsl_matrix * const M)
{
	int i, j;

	printf("======================================\n");
	for(i = 0; i < M->size1; i++)
	{
		for(j = 0; j < M->size2; j++)
			printf("%0.1f\t", gsl_matrix_get(M, i, j));
		printf("\n");
	}
	printf("\n");
}

void vector_int_print(const gsl_vector_int * const V)
{
	int i;

	printf("======================================\n");
	for(i = 0; i < V->size; i++)
	{
		printf("%d ", gsl_vector_int_get(V, i));
	}
	printf("\n");
}

/*!
Fills random data into matrix.
*/
void matrix_random_fill(gsl_matrix * const M)
{
	int i, j;

	srand(time(NULL));
	for(i = 0; i < M->size1; i++)
		for(j = 0; j < M->size2; j++)
			gsl_matrix_set(M, i, j, rand()%100);
	
}

void testcase1(gsl_matrix **M)
{
	*M = gsl_matrix_alloc(6, 6);

	gsl_matrix_set(*M, 0, 0, 62);
	gsl_matrix_set(*M, 0, 1, 75);
	gsl_matrix_set(*M, 0, 2, 80);
	gsl_matrix_set(*M, 0, 3, 93);
	gsl_matrix_set(*M, 0, 4, 95);
	gsl_matrix_set(*M, 0, 5, 97);
	gsl_matrix_set(*M, 1, 0, 75);
	gsl_matrix_set(*M, 1, 1, 80);
	gsl_matrix_set(*M, 1, 2, 82);
	gsl_matrix_set(*M, 1, 3, 85);
	gsl_matrix_set(*M, 1, 4, 71);
	gsl_matrix_set(*M, 1, 5, 97);
	gsl_matrix_set(*M, 2, 0, 80);
	gsl_matrix_set(*M, 2, 1, 75);
	gsl_matrix_set(*M, 2, 2, 81);
	gsl_matrix_set(*M, 2, 3, 98);
	gsl_matrix_set(*M, 2, 4, 90);
	gsl_matrix_set(*M, 2, 5, 97);
	gsl_matrix_set(*M, 3, 0, 78);
	gsl_matrix_set(*M, 3, 1, 82);
	gsl_matrix_set(*M, 3, 2, 84);
	gsl_matrix_set(*M, 3, 3, 80);
	gsl_matrix_set(*M, 3, 4, 50);
	gsl_matrix_set(*M, 3, 5, 98);
	gsl_matrix_set(*M, 4, 0, 90);
	gsl_matrix_set(*M, 4, 1, 85);
	gsl_matrix_set(*M, 4, 2, 85);
	gsl_matrix_set(*M, 4, 3, 80);
	gsl_matrix_set(*M, 4, 4, 85);
	gsl_matrix_set(*M, 4, 5, 99);
	gsl_matrix_set(*M, 5, 0, 65);
	gsl_matrix_set(*M, 5, 1, 75);
	gsl_matrix_set(*M, 5, 2, 80);
	gsl_matrix_set(*M, 5, 3, 75);
	gsl_matrix_set(*M, 5, 4, 68);
	gsl_matrix_set(*M, 5, 5, 96);
}

void testcase2(gsl_matrix **M)
{
	*M = gsl_matrix_alloc(5, 5);

	gsl_matrix_set(*M, 0, 0, 4);
	gsl_matrix_set(*M, 0, 1, 5);
	gsl_matrix_set(*M, 0, 2, 3);
	gsl_matrix_set(*M, 0, 3, 2);
	gsl_matrix_set(*M, 0, 4, 3);
	gsl_matrix_set(*M, 1, 0, 3);
	gsl_matrix_set(*M, 1, 1, 2);
	gsl_matrix_set(*M, 1, 2, 4);
	gsl_matrix_set(*M, 1, 3, 3);
	gsl_matrix_set(*M, 1, 4, 4);
	gsl_matrix_set(*M, 2, 0, 3);
	gsl_matrix_set(*M, 2, 1, 3);
	gsl_matrix_set(*M, 2, 2, 4);
	gsl_matrix_set(*M, 2, 3, 4);
	gsl_matrix_set(*M, 2, 4, 3);
	gsl_matrix_set(*M, 3, 0, 2);
	gsl_matrix_set(*M, 3, 1, 4);
	gsl_matrix_set(*M, 3, 2, 3);
	gsl_matrix_set(*M, 3, 3, 2);
	gsl_matrix_set(*M, 3, 4, 4);
	gsl_matrix_set(*M, 4, 0, 2);
	gsl_matrix_set(*M, 4, 1, 1);
	gsl_matrix_set(*M, 4, 2, 3);
	gsl_matrix_set(*M, 4, 3, 4);
	gsl_matrix_set(*M, 4, 4, 3);
}

void testcase3(gsl_matrix **M)
{
	*M = gsl_matrix_alloc(3, 3);
	
	gsl_matrix_set(*M, 0, 0, 4);
	gsl_matrix_set(*M, 0, 1, 2);
	gsl_matrix_set(*M, 0, 2, 8);
	gsl_matrix_set(*M, 1, 0, 4);
	gsl_matrix_set(*M, 1, 1, 3);
	gsl_matrix_set(*M, 1, 2, 7);
	gsl_matrix_set(*M, 2, 0, 3);
	gsl_matrix_set(*M, 2, 1, 1);
	gsl_matrix_set(*M, 2, 2, 6);
}

void testcase4(gsl_matrix **M)
{
	*M = gsl_matrix_alloc(3, 3);
	
	gsl_matrix_set(*M, 0, 0, 1);
	gsl_matrix_set(*M, 0, 1, 2);
	gsl_matrix_set(*M, 0, 2, 3);
	gsl_matrix_set(*M, 1, 0, 2);
	gsl_matrix_set(*M, 1, 1, 4);
	gsl_matrix_set(*M, 1, 2, 6);
	gsl_matrix_set(*M, 2, 0, 3);
	gsl_matrix_set(*M, 2, 1, 6);
	gsl_matrix_set(*M, 2, 2, 9);
}

void testcase5(gsl_matrix **M)
{
	*M = gsl_matrix_alloc(4, 4);
	
	gsl_matrix_set(*M, 0, 0, 14);
	gsl_matrix_set(*M, 0, 1, 5);
	gsl_matrix_set(*M, 0, 2, 8);
	gsl_matrix_set(*M, 0, 3, 7);
	gsl_matrix_set(*M, 1, 0, 2);
	gsl_matrix_set(*M, 1, 1, 12);
	gsl_matrix_set(*M, 1, 2, 6);
	gsl_matrix_set(*M, 1, 3, 5);
	gsl_matrix_set(*M, 2, 0, 7);
	gsl_matrix_set(*M, 2, 1, 8);
	gsl_matrix_set(*M, 2, 2, 3);
	gsl_matrix_set(*M, 2, 3, 9);
	gsl_matrix_set(*M, 3, 0, 2);
	gsl_matrix_set(*M, 3, 1, 4);
	gsl_matrix_set(*M, 3, 2, 6);
	gsl_matrix_set(*M, 3, 3, 10);
}
	
int main(int argc, char **argv)
{
	gsl_matrix *M;
	gsl_matrix *Assignment;
	int i, j;
	int choice;

	if(argc != 2)
	{
		printf("Enter test case number.\n");
		return 0;
	}
	
	choice = atoi(argv[1]);
	if(choice == -1)
	{
		printf("Invalid choice \"%s\".\n", argv[1]);
		return 0;
	}

	switch(choice)
	{
		case 1: testcase1(&M); break;
		case 2: testcase2(&M); break;
		case 3: testcase3(&M); break;
		case 4: testcase4(&M); break;
		case 5: testcase5(&M); break;
		default:
			printf("No such option.\n");
			return 0;
	}
	Assignment = gsl_matrix_alloc(M->size1, M->size2);

	matrix_print(M);

	struct timeval start, end;
	long elapsed_ms, seconds, useconds;
	
	gettimeofday(&start, NULL);		// Start Timer	

	for(int i = 0; i < 100000; ++i)
		Hungarian(M, false, Assignment);

	gettimeofday(&end, NULL);		// Stop Time
	seconds = end.tv_sec - start.tv_sec;
	useconds = end.tv_usec - start.tv_usec;
	elapsed_ms = ((seconds) * 1000 + useconds/1000.0) + 0.5;

	printf("elapsed: %ul\n", elapsed_ms);
	
	PrintGSLMatrix(M, "M");
	PrintGSLMatrix(Assignment, "Assignment");

	gsl_matrix_free(Assignment);
	gsl_matrix_free(M);

	return 0;
}
