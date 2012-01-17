/****************************************************************************
Copyright 2005,2006 Virginia Polytechnic Institute and State University

This file is part of the OSSIE Signal Processing Library.

OSSIE Signal Processing Library is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

OSSIE Signal Processing Library is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with OSSIE Signal Processing Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA


****************************************************************************/

#include "SigProc.h"
#include <cstring>
#include <climits>
namespace SigProc
{

//-----------------------------------------------------------------------------
//
// Design root raised-cosine filter
//
//-----------------------------------------------------------------------------
/** \brief Calculate square-root raised-cosine filter coefficients

  DesignRRCFilter calculates the coefficients for a square-root raised-cosine
  (RRC) finite impulse response (FIR) filter commonly used in digital
  communications.  The input parameters are as follows

    - \f$k\f$ : samples per symbol
    - \f$m\f$ : sample delay
    - \f$\beta\f$ : excess bandwidth (rolloff) factor

  The function returns a pointer to the filter coefficients as well as an
  integer value describing the length of the filter.  The length of the
  filter is always

  \f[
    h_{len} = 2 k m + 1
  \f]

  The filter coefficients themselves are derived from the following equation

  \f[
    h\left[z\right] =
      4\beta \frac{ \cos\left[(1+\beta)\pi z\right] +
                    \sin\left[(1+\beta)\pi z\right] / (4\beta z) }
                  { \pi \sqrt{T}\left[ 1-16\beta^2z^2\right] }
  \f]

  where \f$z=n/k-m\f$, and \f$T=1\f$ for most cases.

  The function compensates for the two cases where \f$h[n]\f$ might be
  undefined in the above equation, viz.

  \f[
    \mathop {\lim }\limits_{z \to 0 } h(z) =
      \frac{ 4\beta \left[ 1 + \frac{1-\beta\pi }{ 4\beta } \right] }
           { \pi\sqrt{T}\left( 1-16\beta^2 z^2 \right) }
  \f]

  and

  \f[
    \mathop {\lim }\limits_{z \to \pm \frac{1}{4\beta} } h(z) =
        \frac{(1+\beta)}{2\pi}\sin\left[\frac{(1+\beta)\pi}{4\beta}\right]
      - \frac{(1-\beta)}{2}\cos\left[\frac{(1-\beta)\pi}{4\beta}\right]
      + \frac{2\beta}{\pi}\sin\left[\frac{(1-\beta)\pi}{4\beta}\right]

  \f]

  \param[in]  k         samples per symbol
  \param[in]  m         symbol delay
  \param[in]  beta      excess bandwidth/rolloff factor ( 0 < beta < 1 )
  \param[in]  h         pointer to filter coefficients

 */

void DesignRRCFilter(
    unsigned int k,      // samples per symbol
    unsigned int m,      // delay
    float beta,          // rolloff factor ( 0 < beta <= 1 )
    float * h            // pointer to filter coefficients
)
{
    unsigned int h_len;

    if ( k < 1 ) {
        std::cerr << "ERROR: SigProc::DesignRRCFilter: k must be greater than 0"
                  << std::endl;
        throw 0;
    } else if ( m < 1 ) {
        std::cerr << "ERROR: SigProc::DesignRRCFilter: m must be greater than 0"
                  << std::endl;
        throw 0;
    } else if ( (beta < 0.0f) || (beta > 1.0f) ) {
        std::cerr << "ERROR: SigProc::DesignRRCFilter: beta must be in [0,1]"
                  << std::endl;
        throw 0;
    } else;

    unsigned int n;
    float z, t1, t2, t3, t4, T(1.0f);
    float pi(3.14159265358979f);

    h_len = 2*k*m+1;

// Calculate filter coefficients
    for (n=0; n<h_len; n++) {

        z = float(n)/float(k)-float(m);
        t1 = cosf((1+beta)*pi*z);
        t2 = sinf((1-beta)*pi*z);

        // Check for special condition where z equals zero
        if ( n == k*m ) {
            t4 = 4*beta/(pi*sqrtf(T)*(1-(16*beta*beta*z*z)));
            h[n] = t4*( 1 + (1-beta)*pi/(4*beta) );
        } else {
            t3 = 1/((4*beta*z));

            float g = 1-16*beta*beta*z*z;
            g *= g;

            // Check for special condition where 16*beta^2*z^2 equals 1
            if ( g < 1e-6 ) {
                float g1, g2, g3, g4;
                g1 = -(1+beta)*pi*sin((1+beta)*pi/(4*beta));
                g2 = cos((1-beta)*pi/(4*beta))*(1-beta)*pi;
                g3 = -sin((1-beta)*pi/(4*beta))*4*beta;
                g4 = -2*pi;

                h[n] = (g1+g2+g3)/g4;
            } else {
                t4 = 4*beta/(pi*sqrtf(T)*(1-(16*beta*beta*z*z)));
                h[n] = t4*( t1 + (t2*t3) );
            }
        }
    }
}

//-----------------------------------------------------------------------------
//
// Design Gaussian filter
//
//-----------------------------------------------------------------------------
/** \brief Calculate Gaussian filter coefficients

  DesignGaussianFilter calculates the coefficients for a square-root raised-cosine
  (RRC) finite impulse response (FIR) filter commonly used in digital
  communications.  The input parameters are as follows

    - \f$k\f$ : samples per symbol
    - \f$m\f$ : sample delay
    - \f$\beta\f$ : excess bandwidth factor

  The function returns a pointer to the filter coefficients as well as an
  integer value describing the length of the filter.  The length of the
  filter is always

  \f[
    h_{len} = 2 k m + 1
  \f]

  The filter coefficients themselves are derived from the following equation

  \f[
    h\left[z\right] = \frac{1}{\sqrt[4]{2\pi\sigma}}
                      e^{\left(-z^2/{4\sigma^2}\right)}
  \f]

  where \f$z=n/k-m\f$ and \f$\sigma=\beta\f$.

  \param[in]  k         samples per symbol
  \param[in]  m         symbol delay
  \param[in]  beta      excess bandwidth/rolloff factor ( 0 < beta < 1 )
  \param[out] h         pointer to filter coefficients
  \param[out] h_len     length of filter, h_len = 2*m*k+1

 */

void DesignGaussianFilter(
    unsigned int k,      // samples per symbol
    unsigned int m,      // delay
    float beta,          // rolloff factor ( 0 < beta )
    float *& h,          // pointer to filter coefficients
    unsigned int & h_len // length of filter (len = 2*m*k+1)
)
{
    h_len = 0;

    if ( k < 1 ) {
        std::cerr << "ERROR: SigProc::DesignGaussianFilter: k must be greater than 0"
                  << std::endl;
        throw 0;
    } else if ( m < 1 ) {
        std::cerr << "ERROR: SigProc::DesignGaussianFilter: m must be greater than 0"
                  << std::endl;
        throw 0;
    } else if ( beta < 0.0f) {
        std::cerr << "ERROR: SigProc::DesignGaussianFilter: beta must be greater than 0"
                  << std::endl;
        throw 0;
    } else;

    h_len = 2*k*m + 1;
    h = new float[h_len];
    float sigma(beta);  ///\todo determine how to calculate sigma from beta
    float pi(3.14159265358979f);
    float g1( 2*sigma*sigma );
    float g2( 1/sqrtf(g1*pi) );
    float z;

    for (unsigned int n=0; n<h_len; n++) {
        // Check for special condition where z equals zero
        if ( n == k*m ) {
            h[n] = g2;
        } else {
            z = float(n)/float(k)-float(m);
            h[n] = expf(-z*z/g1)*g2;
        }
    }
}

//-----------------------------------------------------------------------------
//
// FIR polyphase filter bank
//
//-----------------------------------------------------------------------------

// Initializing constructor
FIRPolyphaseFilterBank::FIRPolyphaseFilterBank(
    char * _type,       // type of filter
    unsigned int _k,    // samples per symbol
    unsigned int _m,    // delay
    float _beta,        // excess bandwidth factor
    unsigned int _Npfb  // number of filters
)
{
    k = _k;
    m = _m;
    beta = _beta;
    Npfb = _Npfb;

    H = NULL;

    if ( strcmp(_type,"rrcos")==0 ) {
        // Square-root raised-cosine filter
        CalculateRRCFilterCoefficients();
        TransposeCoefficientMatrix();
    } else if ( strcmp(_type,"drrcos")==0 ) {
        // Derivative square-root raised-cosine filter
        CalculateRRCFilterCoefficients();
        CalculateDerivativeFilterCoefficients();
        TransposeCoefficientMatrix();
    } else if ( strcmp(_type,"gaussian")==0 ) {
        // Gaussian filter
        CalculateGaussianFilterCoefficients();
        TransposeCoefficientMatrix();
    } else if ( strcmp(_type,"dgaussian")==0 ) {
        // Derivative gaussian filter
        CalculateGaussianFilterCoefficients();
        CalculateDerivativeFilterCoefficients();
        TransposeCoefficientMatrix();
    } else {
        std::cerr << "ERROR: FIRPolyphaseFilterBank: unknown filter type : " << _type << std::endl;
        throw 0;
    }

// At this point, H should be initialized and h_len should store
// the correct length of each filter in the bank.
    v.SetBufferSize(h_len);

// memset input buffer to zeros
    ResetBuffer();
}

// Initializing constructor
FIRPolyphaseFilterBank::FIRPolyphaseFilterBank(
    float * _H,         // filter bank coefficients
    unsigned int _h_len,// length of each filter
    unsigned int _Npfb  // number of filters
)
{
/// \todo perform check on input?

    h_len = _h_len;
    Npfb = _Npfb;

// perform deep copy of memory
    H = new float[Npfb*h_len];
    for (unsigned int i=0; i<(Npfb*h_len); i++)
        H[i] = _H[i];

    v.SetBufferSize(h_len);

// memset input buffer to zeros
    ResetBuffer();
}

// destructor
FIRPolyphaseFilterBank::~FIRPolyphaseFilterBank()
{
// delete filter bank coefficients matrix
    if ( H != NULL )
        delete [] H;

}

// push input value into buffer
void FIRPolyphaseFilterBank::PushInput(short _x)
{
// Add _x to input buffer
    v.Push( _x );
}

// compute filter output from current buffer state using specific filter
void FIRPolyphaseFilterBank::ComputeOutput(
    short &y,           // output sample
    unsigned int _b     // filter bank index
)
{
// Ensure the requested filter bank index does not exceed the number
// of filters actually in the bank
    if ( _b >= Npfb ) {
        std::cerr << "ERROR: SigProc::FIRPolyphaseFitlerBank::ComputeOutput, "
                  << " index exceeds filter bank size ("
                  << _b << " > " << Npfb << ")" << std::endl;
        throw 0;
    }

// Set B to memory block in filter bank array
    unsigned int B;
    B = _b*h_len;

// Compute dot product
    dot_product( H+B, v.GetHeadPtr(), h_len, y);

}

// compute filter output from current buffer state using specific filter
void FIRPolyphaseFilterBank::ComputeOutput(
    short &y,           // output sample
    unsigned int _b,    // filter bank index
    short *_v           // external memory buffer array
)
{
// Ensure the requested filter bank index does not exceed the number
// of filters actually in the bank
    if ( _b >= Npfb ) {
        std::cerr << "ERROR: SigProc::FIRPolyphaseFitlerBank::ComputeOutput, "
                  << " index exceeds filter bank size ("
                  << _b << " > " << Npfb << ")" << std::endl;
        throw 0;
    }

// Set B to memory block in filter bank array
    unsigned int B;
    B = _b*h_len;

// Compute dot product
    dot_product( H+B, _v, h_len, y);

}

// Reset filter buffer
void FIRPolyphaseFilterBank::ResetBuffer()
{
    v.Release();

// Load buffer with zeros
    for (unsigned int i=0; i<h_len; i++)
        v.Push( 0 );
}

// Print filter buffer
void FIRPolyphaseFilterBank::PrintBuffer()
{
    v.Print();
}

// Print filter bank coefficients to the screen
void FIRPolyphaseFilterBank::PrintFilterBankCoefficients()
{
    if ( H == NULL ) {
        std::cout << "ERROR: SigProc::FIRPolyphaseFilterBank: "
                  << "cannot print filter coefficients (matrix empty)"
                  << std::endl;
        return;
    }

    unsigned int r, c, B;
    std::cout << "H = [" << std::endl;
    for (r=0; r<Npfb; r++ ) {
        B = r*h_len;
        printf("  ");

        for (c=0; c<h_len; c++) {
            printf("%E ", H[B + c]);
        }
        printf(";\n");
    }
    std::cout << "   ]" << std::endl;

}

//
void FIRPolyphaseFilterBank::TransposeCoefficientMatrix()
{
// create temporary pointer
    float * H_tmp;
    H_tmp = H;

// allocate new memory for filter bank coefficients
    H = new float[h_len*Npfb];

// reshape matrix
    unsigned int i(0), r, c;
    for (r=0; r<Npfb; r++) {
        for (c=0; c<h_len; c++)
            H[i++] = H_tmp[c*Npfb + r];
    }

    delete [] H_tmp;
}


// Calculate root raised-cosine coefficients
void FIRPolyphaseFilterBank::CalculateRRCFilterCoefficients()
{
    if ( H != NULL )
        delete [] H;

// create over-sampled pulse
    h_len = 2*k*m*Npfb+1;
    H = new float[h_len];
    DesignRRCFilter(Npfb*k, m, beta, H);

// Apply scaling factor to filter coefficients
    float h_scale = float(k);
    for (unsigned int i=0; i<h_len; i++)
        H[i] /= h_scale;

//
    h_len = ( h_len - 1 ) / Npfb;

}

//
void FIRPolyphaseFilterBank::CalculateGaussianFilterCoefficients()
{
    if ( H != NULL )
        delete [] H;

}

// Calculate derivative filter coefficients
void FIRPolyphaseFilterBank::CalculateDerivativeFilterCoefficients()
{
    unsigned int N = h_len*Npfb;

    float * dH = new float[N];

    for (unsigned int i=0; i<N; i++) {
        if ( i==0 ) {
            dH[0] = H[1] - H[N-1];
        } else if ( i==N-1 ) {
            dH[N-1] = H[0] - H[N-2];
        } else {
            dH[i] = H[i+1] - H[i-1];
        }
        dH[i] /= 2.0f;
    }

    delete [] H;
    H = dH;
}


iir_filter::iir_filter(float a_coeff[], unsigned int len_a, float b_coeff[], unsigned int len_b) : len_A(len_a), len_B(len_b), next_v(0)
{

    A = new float[len_a];
    B = new float[len_b];

    for (unsigned int i = 0; i < len_a; ++i)
        A[i] = a_coeff[i];

    for (unsigned int i = 0; i < len_b; ++i)
        B[i] = b_coeff[i];

    if (len_A > len_B)
        len_v = len_A;
    else
        len_v = len_B;

    v = new float[len_v];

    for (unsigned int i = 0; i < len_v; ++i)
        v[i] = 0;

}

void iir_filter::ResetBuffer()
{
    for (unsigned int i = 0; i < len_v; ++i)
        v[i] = 0;
}

void iir_filter::do_work(short x, short &y)
{
// calculate new v[n]
    int v_idx = next_v;

    v[next_v] = x;

    for (unsigned int i = 1; i < len_A; ++i) {

        --v_idx;
        if (v_idx < 0)
            v_idx += len_v;

        v[next_v] -= A[i] * v[v_idx];

    }

// Now calculate the output value
    v_idx = next_v;
    float out = 0;

    for (unsigned int i = 0; i < len_B; ++i) {
        out += (B[i] * v[v_idx]);

        --v_idx;
        if (v_idx < 0)
            v_idx += len_v;
    }

    next_v = (++next_v) % len_v;

    if (out < SHRT_MIN)
        y = SHRT_MIN;
    else if (out > SHRT_MAX)
        y = SHRT_MAX;
    else
        y = (short) out;
}


fir_filter::fir_filter(float a_coeff[], unsigned int len_a) : len_A(len_a), len_v(len_a),next_v(0)
{
#ifdef FPM
    A = new mad_fixed_t[len_a];
    v = new mad_fixed_t[len_v];
#else
    A = new float[len_a];
    v = new short[len_v];
#endif

    for (unsigned int i = 0; i < len_a; ++i)
#ifdef FPM
        A[i] = mad_f_tofixed(a_coeff[i]);
#else
        A[i] = a_coeff[i];
#endif


    for (unsigned int i = 0; i < len_v; ++i)
        v[i] = 0;


}

void fir_filter::do_work(bool run_filter, short x, short &y)
{
// calculate new v[n]
    int v_idx = next_v;

    v[next_v] = x;
    ++next_v;
    if (next_v == len_v)
        next_v = 0;

    if (!run_filter) { // Do not create an output value
        y = 0;
        return;
    }

// Now calculate the output value
#ifdef FPM
    mad_fixed_t out(0);
#else
    float out(0.0);
#endif

    for (unsigned int i = 0; i < len_A; ++i) {
        if (v[v_idx]) {
#ifdef FPM
            out = mad_f_add(out, mad_f_mul(A[i], v[v_idx]));
#else
            out += (A[i] * v[v_idx]);
#endif
        }

        --v_idx;
        if (v_idx < 0)
            v_idx += len_v;
    }

#ifdef FPM
    y = (short) out;
#else
    if (out < SHRT_MIN)
        y = SHRT_MIN;
    else if (out > SHRT_MAX)
        y = SHRT_MAX;
    else
        y = (short) out;
#endif
}

void fir_filter::reset()
{

    for (unsigned int i = 0; i < len_v; ++i)
        v[i] = 0;

    next_v = 0;
}

} // namespace SigProc
