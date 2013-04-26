#ifndef _UMEYAMA_H_
#define _UMEYAMA_H_

#include <gsl/gsl_matrix.h>

int EigenDecomp( gsl_matrix_complex * A,
                 gsl_matrix_complex * B,
                 gsl_matrix * const P );

int Umeyama(const gsl_matrix * const A,
            const gsl_matrix * const B,
            gsl_matrix * const Assignment,
            float *score);
#endif
