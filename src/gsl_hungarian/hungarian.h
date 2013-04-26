#ifndef _HUNGARIAN_H_
#define _HUNGARIAN_H_

#include <stdbool.h>
#include <gsl/gsl_vector.h>
#include <gsl/gsl_matrix.h>

int Hungarian(const gsl_matrix * const M, const bool convToMinAsg, gsl_matrix * const Assignment);

#endif
