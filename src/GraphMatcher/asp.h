//adapted from Dominic Battre's implementation of the Hungarian algorithm by hbc@mit.edu

#ifndef ASP_H
#define ASP_H

//typedef double cost;
typedef int cost;
#define COST_TYPE tFloat64
#define COST_TYPE_NPY NPY_DOUBLE

#define COST_PRECISION 10000
#define INF (1000 * COST_PRECISION)

void asp(int size, cost **Array, long *col_mate, long *row_mate);

#endif
