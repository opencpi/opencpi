/*
 *                            COPYRIGHT
 *
 *  bwlp - Butterworth lowpass filter coefficient calculator
 *  Copyright (C) 2003, 2004, 2005 Exstrom Laboratories LLC
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  A copy of the GNU General Public License is available on the internet at:
 *
 *  http://www.gnu.org/copyleft/gpl.html
 *
 *  or you can write to:
 *
 *  The Free Software Foundation, Inc.
 *  675 Mass Ave
 *  Cambridge, MA 02139, USA
 *
 *  You can contact Exstrom Laboratories LLC via Email at:
 *
 *  info(AT)exstrom.com
 *
 *  or you can write to:
 *
 *  Exstrom Laboratories LLC
 *  P.O. Box 7651
 *  Longmont, CO 80501, USA
 *
 */

#include "SigProc.h"
#include <cstdlib>
namespace SigProc
{

double *binomial_mult( int n, double *p );

void design_butter_lowpass_filter(
    unsigned int order,
    float wc,
    float *b,
    float *a)
{
    int i, n;
    double fcd, fca;
    double *p;
    double *d;
    double pmr, pmi;
    double ppr, ppi;
    double ppmr, ppmi;
    double d1, d2;

    n = order;
    fcd = M_PI * wc;
    fca = tan( fcd / 2.0 );
    p = (double *)calloc( 2 * n, sizeof(double) );

    /* Calculate analog Butterworth coefficients */
    for ( i = 0; i < n; ++i ) {
        p[2*i] = -fca * sin(M_PI*((double)i+0.5)/(double)n);
        p[2*i+1] = fca * cos(M_PI*((double)i+0.5)/(double)n);
    }

    ppmr = 1.0;
    ppmi = 0.0;

    for ( i = 0; i < n; ++i ) {
        pmr = p[2*i] - 1.0;
        pmi = p[2*i+1];
        ppr = p[2*i] + 1.0;
        ppi = p[2*i+1];

        d1 = pmr * pmr + pmi * pmi;
        p[2*i] = (ppr * pmr + ppi * pmi) / d1;
        p[2*i+1] = (ppi * pmr - ppr * pmi) / d1;

        d1 = ppmr;
        d2 = ppmi;
        ppmr = -( d1 * pmr - d2 * pmi );
        ppmi = -( d1 * pmi + d2 * pmr );
    }

    ppmr = pow( fca, (double)n ) / ppmr; /* scaling factor */

    d = binomial_mult( n, p );

    b[0] = (float) ppmr;

    d1 = (double)n;
    d2 = 1.0;

    for ( i = 1; i <= n; ++i ) {
        b[i] = (float) (ppmr*d1/d2);
        d1 *= (double)(n - i);
        d2 *= (double)(i + 1);
    }

    a[0] = 1.0f;

    for ( i = 0; i < n; ++i )
        a[i+1] = (float) (d[2*i]);

    free( p );
    free( d );
}

/**********************************************************************
  binomial_mult - multiplies a series of binomials together and returns
  the coefficients of the resulting polynomial.

  The multiplication has the following form:

  (x+p[0])*(x+p[1])*...*(x+p[n-1])

  The p[i] coefficients are assumed to be complex and are passed to the
  function as a pointer to an array of doubles of length 2n.

  The resulting polynomial has the following form:

  x^n + a[0]*x^n-1 + a[1]*x^n-2 + ... +a[n-2]*x + a[n-1]

  The a[i] coefficients can in general be complex but should in most
  cases turn out to be real. The a[i] coefficients are returned by the
  function as a pointer to an array of doubles of length 2n. Storage
  for the array is allocated by the function and should be freed by the
  calling program when no longer needed.

  Function arguments:

  n  -  The number of binomials to multiply
  p  -  Pointer to an array of doubles where p[2i] (i=0...n-1) is
        assumed to be the real part of the coefficient of the ith binomial
        and p[2i+1] is assumed to be the imaginary part. The overall size
        of the array is then 2n.
*/

double *binomial_mult( int n, double *p )
{
    int i, j;
    double *a;

    a = (double *)calloc( 2 * n, sizeof(double) );

    for ( i = 0; i < n; ++i ) {
        for ( j = i; j > 0; --j ) {
            a[2*j] += p[2*i] * a[2*(j-1)] - p[2*i+1] * a[2*(j-1)+1];
            a[2*j+1] += p[2*i] * a[2*(j-1)+1] + p[2*i+1] * a[2*(j-1)];
        }
        a[0] += p[2*i];
        a[1] += p[2*i+1];
    }
    return( a );
}

} // namespace SigProc
