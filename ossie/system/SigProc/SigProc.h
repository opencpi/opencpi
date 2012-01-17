/****************************************************************************

Copyright 2005, 2006, 2008 Virginia Polytechnic Institute and State University

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

#ifndef SIG_PROC_H
#define SIG_PROC_H

#include <iostream>
#include <fstream>
#include <string>
#include <math.h>

#include "fixed.h"

#define PI      3.14159265358979f
#define TWO_PI  6.28318530717959f

namespace SigProc
{
//-----------------------------------------------------------------------------
//
// Convolutional coding definitions
//
//-----------------------------------------------------------------------------

/** \brief Defines the basic entry for building the trellis at the decoder
*/
class TrellisEntry
{
public:
///Contructor
    TrellisEntry();
///Destructor
    ~TrellisEntry();
    unsigned int previousState; ///< The state prior to the current state, used for tracing back the trellis
    unsigned short int symbolNo; ///< The symbol that caused the change of state to the currrent state
    signed int distance; ///< The total distance of the current state
};

/** \brief Includes all the necessary information for encoding and decoding the symbols
 *
 * The contructor has to be ALWAYS be used. The generator polynomials for each code rate have to be specified in OCTAL format
 *
 * For more information for the meaning of the symbols used and for generator polynomials look at
 * Proakis Digital Communications book 4th Edition, pages 471 & 492 respectively.
*/
class trellisTable
{
public:
/// The contructor has to be always used
    trellisTable(unsigned int * generatorPolynomials, unsigned short int k, unsigned short int n, unsigned short K);

/// =The destructor
    ~trellisTable();

    unsigned short int k;///<the input bits at a time
    unsigned short int n;///< the output bits at a time
    unsigned short int K;///<the contraint length
    unsigned short int numberOfInputStates;///< Based on k, how many different input states exist
    unsigned int numberOfTrellisStates;///<Number of different trellis states, defined by k & K

    unsigned int **output; ///< The rows are the trellis states and the columns the symbols, gives the output given the current state and symbol.
    unsigned int **nextState;///< The same as output, gives the the next state given the current state and synbol

protected:
///Generates the trellis table given the generator polynomials in OCTAL
    void GenerateTrellisTable(unsigned int * generatorPolynomialsOct);

///Performs modulo 2 addition to the bits of the input number (symbol)
    unsigned short int Modulo2BitWiseAdd(unsigned short int inputNumber);

///Converts an Octal Number to a Decimal number
    unsigned int Oct2Dec(unsigned int octNumber);
};

/** \brief Defines the basic functionality for the convolution encoding/decoding
*/

class fec_conv
{
public:
/// It accepts the pointer to the trellis Table
    void SetTrellisTable(trellisTable *theTrellisTableIn);

protected:
/// It converts a decimal numnber (symbol) into a vector of bits
    void Dec2Bin(unsigned int decNumber,unsigned short int * outputData,unsigned short int numberOfBits);

    trellisTable * theTrellisTable;///< It holds the trellis table used for encoding/decoding
};

/** \brief It defines the encoding functionality
 *
 * To use this class, first pass the trellis table pointer
 *
 * To encode: reset the state, feed data & get encoded data, repeat for the next packet.
 *
 * To make the encoder to return at the zero state during encoding, feed K 0 symbols after the encoding of the data is done.
*/
class fec_conv_encoder : public fec_conv
{
public:
///Default constructor
    fec_conv_encoder();

///Default destructor
    ~fec_conv_encoder();

/// Resets the state of the encoder to the zero state
    void ResetState();

///It gets the current state of the encoder
    unsigned int GetState();

///It accepts a vector of k bits and returns a vector of n bits
    void Encode(unsigned short int * inputData,unsigned short int * outputData);

protected:
    unsigned int currentState;///<The current state of the encoder
};

/** \brief It defines the decoding functionality
 *
 * The following cycle should be followed to use this class:
 *
 * 1. Set the trellis table
 *
 * 2. Set the traceback length
 *
 * 3. Set mode
 *
 * 4. Feed symbols into the decoder
 *
 * 5. Trace back trellis
 *
 * 6. Get the decoded symbols
 *
 * 7. Reset and repeat from 4 for the next packet. If the decoding parameters change, might need to repeat from 1.
*/
class fec_conv_decoder:public fec_conv
{
public:
///Default contructor
    fec_conv_decoder();

///Default destructor
    ~fec_conv_decoder();

/// Set the numbers of the input symbols to be decoded, this equals to the total bits on the output of the encoder divided by k
    void SetNoOfSymbols2TraceBack(unsigned int tracebackLength);

/// Feeds a symbol (in a vector of n bits) at time for decoding
    void Symbol2Decode(unsigned short int * inputData);

/// After all symbols were fed in the decoder, it will tracesback the trellis to the begining.
    void TraceBackTrellis();

///After the trellis was traced back, it returns a decoded symbol, starting from the beggining each time is called.
    void GetDecodedSymbol(unsigned short int * outputData);

///It resets the state of the decoder in order to be able to start a new decoding session
    void Reset();

///Sets the mode of the decoder 0: the encoded data start from zero state 1: the encoded data start from the the zero state and end at the zero state.
    void SetMode(unsigned short int mode);

protected:
///Calculates the distance between the input bits (symbol) to the current trellis symbol
    signed int CalculateDistance(unsigned short int inBits, unsigned short int symbol);

    unsigned int currentTrellisIndex;///<The current trellis stage used when building and tracing back the trellis.
    unsigned int decodedSymbolIndex;///<The next symbol to be returned when the GetDecodedSymbol is called.
    unsigned int noOfSymbols2TraceBack;///<Number of symbols to trace back, should be equal to the symbols used in the encoding process.
    unsigned int mode;///<The mode of the encoder

    unsigned int *tracedBackSymbols;///<A vector array with the symbols traced back
    TrellisEntry **theTrellis; ///<A two dimensional array making the trellis, states are rows, and columns are each trellis stage.
};

//-----------------------------------------------------------------------------
//
// Design root raised-cosine filter
//
//-----------------------------------------------------------------------------
void DesignRRCFilter(
    unsigned int k,      // samples per symbol
    unsigned int m,      // delay
    float beta,          // rolloff factor ( 0 < beta <= 1 )
    float * h            // pointer to filter coefficients
);

//-----------------------------------------------------------------------------
//
// Design Gaussian filter
//
//-----------------------------------------------------------------------------
void DesignGaussianFilter(
    unsigned int k,      // samples per symbol
    unsigned int m,      // delay
    float beta,          // rolloff factor ( 0 < beta <= 1 )
    float *& h,          // pointer to filter coefficients
    unsigned int & h_len // length of filter (len = 2*m*k+1)
);

//-----------------------------------------------------------------------------
//
// Design low-pass butterworth filter
//
//-----------------------------------------------------------------------------
void design_butter_lowpass_filter(
    unsigned int order,   // filter order
    float wc,             // cutoff frequency
    float *b,             // feedback
    float *a              // feedforward
);

//-----------------------------------------------------------------------------
//
// Circular buffer
//
//-----------------------------------------------------------------------------
/** \brief Circlar buffer, template class
 *
 * \section CB_basic_description Basic Description
 * The circular buffer template class implementation minimizes memory copies
 * by wrapping the array around to its beginning.  Elements can be added
 * and removed by invoking the Push() and Pop() methods, respectively.
 *
 * \section CB_creating Creating Buffers
 * There are three ways to create a buffer...
 * \code
 * // 1. generate an empty buffer
 * CircularBuffer <short> v1(100);
 *
 * // 2. wrap an existing array
 * short * x = new short[100];
 * for (unsigned int i=0; i<100; i++)
 *     x[i] = i;
 * CircularBuffer <short> v2(x, 100);
 *
 * // 3. copy from another CircularBuffer
 * CircularBuffer <short> v3(v2);
 * \endcode
 *
 * \section CB_resizing_buffers Resizing Buffers
 * CircularBuffer supports dynamic memory allocation as well; if an instance
 * of CircularBuffer is created of a particular size and then later it is
 * determined that the size is too small, invoking SetBufferSize() can be used
 * to increase the length without loss of data.  However, decreasing the buffer
 * size beyond the number of elements in the buffer truncates the data.
 *
 * \section CB_wrapping Wrapping
 * When the buffer is full and another element is pushed, the new element
 * overwrites the last element in the buffer without warning.  Status of the
 * buffer can be checked with the GetBufferSize() and GetNumElements()
 * methods.
 *
 */
template <class T>
class CircularBuffer
{
public:
/// Default constructor
    CircularBuffer();

/// Initializing constructor (empty)
    CircularBuffer(unsigned int _bufferSize);

/// Initializing constructor (array)
    CircularBuffer(T * _v, unsigned int _bufferSize);

/// Copy constructor
    CircularBuffer(CircularBuffer &);

/// destructor
    ~CircularBuffer() {
        delete [] headPtr;
    }

/// \brief Overload the [] operator (indexing)
///
/// Returns the value at the appropriate index as if the buffer were a
/// linear array
    T operator[] (unsigned int i) {
        return headPtr[(i_read + i) % bufferSize ];
    }

/// Push value into the beginning of the buffer, overwrite existing element
/// if buffer is full
    void Push(T _value) {

        // OK to push value
        headPtr[i_head++] = _value;

        // Ensure head index does not equal or exceed bufferSize (wrap)
        i_head = i_head % bufferSize;

        // Check to see if buffer is full
        if ( numElements < bufferSize )
            numElements++;  // buffer not yet full
        else
            i_read++;       // overflow
    }

/// Remove element from the end of the buffer
    T Pop() {
        if ( numElements == 0 ) {
            std::cerr << "ERROR: SigProc::CircularBuffer::Pop() : buffer is empty!"
                      << std::endl;
            throw 0;
        }

        // read value
        T retval = headPtr[i_read++];

        // Ensure read index does not equal or exceed bufferSize (wrap)
        i_read = i_read % bufferSize;

        // Decrement number of elements
        numElements--;

        // RETURN value
        return retval;
    }

/// Releases entire buffer (resets values in buffer to zero)
    void Release() {
        i_head = 0;
        i_read = 0;
        numElements = 0;
        memset(headPtr, 0, bufferSize*sizeof(T));
    }

/// Releases _n elements from buffer
    void Release( unsigned int _n ) {
        if ( _n >= numElements ) {
            Release();
        } else {
            numElements -= _n;
            i_read = (i_read + _n) % bufferSize;
        }
    }

/// Return the number of memory slots allocated to the buffer
    unsigned int GetBufferSize() {
        return bufferSize;
    }

/// Set the buffer size dynamically
    void SetBufferSize(unsigned int _bufferSize);

/// Return the number of elements inside the buffer
    unsigned int GetNumElements() {
        return numElements;
    }

/// \brief Get a pointer to the buffer
///
/// This method actually shifts the elements inside the buffer so that instead
/// of being cyclical they are linear.
    T * GetHeadPtr() {
        Linearize();
        return headPtr;
    }

/// Prints buffer to screen
    void Print() {
        std::cout << " b : ";
        for (unsigned int i=0; i<numElements; i++)
            std::cout << " " << headPtr[(i_read+i) % bufferSize];
        std::cout << std::endl;
    }

protected:
/// Pointer to the beginning of the buffer
    T * headPtr;

/// Head index
    unsigned int i_head;

/// Read index
    unsigned int i_read;

/// Memory slots allocated to the buffer
    unsigned int bufferSize;

/// Number of elements currently in the buffer
    unsigned int numElements;

/// \brief Linearize buffer array
///
/// Shifts the elements in the buffer so that they are organized linearly
/// rather than circularly.  If the buffer is not empty, Linearize creates
/// a new array and copies the old values.
    void Linearize();

};

//-----------------------------------------------------------------------------
//
// P/N Sequence
//
//-----------------------------------------------------------------------------

/// P/N Sequence
class PNSequence
{
public:
/// initializing constructor
    PNSequence(unsigned int _g, unsigned int _a);

/// destructor
    ~PNSequence();

    unsigned long m;    ///< shift register length
    unsigned long n;    ///< output sequence length, \f$ n=2^m-1 \f$
    char * g;           ///< generator polynomial
    char * a;           ///< initial polynomial state
    char * s;           ///< output sequence
};

//-----------------------------------------------------------------------------
//
// Automatic Gain Control class
//
//-----------------------------------------------------------------------------
/** \brief Automatic gain control signal processor
 *
 * \cite  R. G. Lyons, Understanding Digital Signal Processing, 2nd ed. New Jersey:
 * Prentice Hall, 2004.
 */
class AutomaticGainControl
{
public:
/// default constructor
    AutomaticGainControl();

/// Destructor
    ~AutomaticGainControl();

/// Set signal processing values
    void SetValues(
        float _elo,
        float _ehi,
        float _ka,
        float _kr,
        float _gmin,
        float _gmax);

/// Get signal processing values
    void GetValues(
        float & _elo,
        float & _ehi,
        float & _ka,
        float & _kr,
        float & _gmin,
        float & _gmax);

/// Get status
    void GetStatus(float & _gain, float & _energy);

/// track signal energy and apply gain (real)
    void ApplyGain(short & I);

/// track signal energy and apply gain (complex)
    void ApplyGain(short & I, short & Q);

private:
/// disallow copy constructor
    AutomaticGainControl(AutomaticGainControl &);

/// compute necessary gain value from measured energy
    void ComputeGain();

/// low energy threshold
    float energy_lo;

/// high energy threshold
    float energy_hi;

/// attack time constant
    float ka;

/// release time constant
    float kr;

/// minimum gain value
    float gmin;

/// maximum gain value
    float gmax;

/// actual tracking gain value
    float gain;

/// actual tracking average energy value
    float energy;

/// low-pass filter coefficient for estimating average energy
    float zeta;

/// average energy threshold for smoother tracking
    float energy_av;

};

class phase_detect
{

public:
    phase_detect(float scale_factor);

    void do_work(short I_in, short Q_in, short I_nco, short Q_nco, short &out);

private:
    phase_detect(const phase_detect &);

    float scale_factor;
};


class nco
{
public:
    nco();
    nco(unsigned int max_out);

    void do_work(short control_voltage, short &sine, short &cosine);

private:
    nco(const nco &);

    int freq_index;
    int max_out;
};

class gain
{
public:
    gain();

    void do_work(float gain, short data_in, short &out);

private:
    gain(const gain &);

};

class iir_filter
{
public:
    iir_filter(float a[], unsigned int len_a, float b[], unsigned int len_b);

    void do_work(short x, short &y);

    void ResetBuffer();

private:
    iir_filter(const iir_filter &);

    float *A;
    float *B;
    unsigned int len_A, len_B;

    float *v;
    unsigned int len_v;
    unsigned int next_v;
};


//-----------------------------------------------------------------------------
//
// FIR polyphase filter bank
//
//-----------------------------------------------------------------------------
/** \brief Finite impulse response (FIR) polyphase filter bank
 *
 * This class implementes a finite impulse response (FIR) polyphase filter
 * bank useful for decimators that need to interpolate samples in digital
 * receivers.
 *
 * The filter bank can automatically calculate filter coefficients for
 * prototypes commonly used in communications systems. Currently, such
 * supported filter prototypes are
 *   - root raised-cosine
 *
 * Filter prototypes that will eventually be supported are
 *   - raised-cosine
 *   - gaussian
 *   - triangular
 *   - hamming
 *
 * The user can also load filter coefficients that have been calculated
 * externally.
 *
 * \image latex polyphase_rcos_filter_k2_N4.eps "Example filter bank (1)"
 * \image html polyphase_rcos_filter_k2_N4.png  "Example filter bank (1)"
 *
 * \image latex polyphase_rcos_filter_k4_N2.eps "Example filter bank (2)"
 * \image html polyphase_rcos_filter_k4_N2.png  "Example filter bank (2)"
 *
 * \cite M. Rice and fred harris, "Polyphase Filterbanks for Symbol Timing
 * Synchronization in Sampled Data Receivers," in MILCOMM Proceedings, vol.
 * 2, October 2002, pp. 982--986.
 *
 */
class FIRPolyphaseFilterBank
{
public:
/// \brief Initializing constructor
///
/// This constructor calculates the filter coefficients for several
/// different filter types using just a few parameters.  The filters
/// currently supported are:
///   - 'rrcos'  : square-root raised-cosine (RRC)
///   - 'drrcos' : derivative RRC
///
    FIRPolyphaseFilterBank(
        char * _type,       // type of filter
        unsigned int _k,    // samples per symbol
        unsigned int _m,    // delay
        float _beta,        // excess bandwidth factor
        unsigned int _Npfb  // number of filters
    );

/// \brief Initializing constructor
///
/// This constructor loads filter bank coefficients which have
/// been generated externally.  The coefficients are copied from
/// the input array to a new buffer.
    FIRPolyphaseFilterBank(
        float * _H,         // filter bank coefficients
        unsigned int _h_len,// length of each filter
        unsigned int _Npfb  // number of filters
    );

/// destructor
    ~FIRPolyphaseFilterBank();

/// Push input value into buffer
    void PushInput(short _x);

/// Compute filter output from current buffer state using specific
/// filter from filter bank matrix
    void ComputeOutput(
        short &y,           // output sample
        unsigned int _b     // filter bank index
    );

/// Compute filter output from current buffer state using specific
/// filter from filter bank matrix and an external memory buffer
    void ComputeOutput(
        short &y,           // output sample
        unsigned int _b,    // filter bank index
        short *_v           // external memory buffer array
    );

/// Reset filter buffer
    void ResetBuffer();

/// Print filter buffer
    void PrintBuffer();

/// Prints filter bank coefficients to the screen
    void PrintFilterBankCoefficients();

/// Get the length of each filter
    unsigned int GetFilterLength() {
        return h_len;
    }

/// Get the number of filters in the bank
    unsigned int GetNumFilters() {
        return Npfb;
    }

/// Return a pointer to the filter bank coefficients; this is intended
/// for debugging
    float * GetFilterBankCoefficients() {
        return H;
    }

protected:

/// type of filter; can be one of the following
///   - 'rrcos'
///   - 'gaussian'
    char * type;

/// samples per symbol
    unsigned int k;

/// symbol delay
    unsigned int m;

/// excess bandwidth factor
    float beta;

/// number of filters in bank
    unsigned int Npfb;

/// \brief filter bank coefficients matrix
///
/// The coefficients are stored in a one-dimensional array which
/// is realized as a two-dimensional matrix.  The array is of
/// length Npfb*h_len (the number of filters in the bank times
/// the length of each filter).
    float *H;

/// length of each filter
    unsigned int h_len;

/// circular input buffer
    CircularBuffer <short> v;

/// transpose filter bank coefficient matrix
    void TransposeCoefficientMatrix();

// ----- calculate filter bank coefficients -----

/// Calculate root raised-cosine coefficients
    void CalculateRRCFilterCoefficients();

/// Calculate Gaussian filter coefficients
    void CalculateGaussianFilterCoefficients();

/// \brief Calculate derivative filter coefficients
///
/// Approximates the derivative of the template filter
/// \f[ \dot{h}(nT) = \frac{\partial h(nT)}{\partial t} \f]
///
/// using discrete samples, viz.
/// \f[ \dot{h}_m(nT)     = h_{m+1}(nT) - h_{m-1}(nT), \ \ m=1,2,\ldots,...M-2 \f]
/// \f[ \dot{h}_0(nT)     = h_{1}(nT) - h_{M-1}(nT)\f]
/// \f[ \dot{h}_{M-1}(nT) = h_{M-2}(nT) - h_{0}(nT)\f]
///
    void CalculateDerivativeFilterCoefficients();

private:

/// disallow copy constructor
    FIRPolyphaseFilterBank(const FIRPolyphaseFilterBank&);

};


//-----------------------------------------------------------------------------
//
// Dot product definitions
//
//-----------------------------------------------------------------------------
void dot_product(float *x, float *y, unsigned int N, float &z);
void dot_product(short *x, short *y, unsigned int N, short &z);
void dot_product(float *x, short *y, unsigned int N, short &z);
void dot_product(float *x, short *y, unsigned int N, float &z);

//-----------------------------------------------------------------------------
//
// Trigonometric functions
//
//-----------------------------------------------------------------------------
void arctan(float &x, float &y, float &theta);

/// Rotates a complex signal counter-clockwise by \f[\theta\f] radians
/// \f[ \bar{y} = \bar{x} e^{j\theta} \f]
void rotate(short I_in, short Q_in, float theta, short *I_out, short *Q_out);

//-----------------------------------------------------------------------------
//
// Random number generators
//
//-----------------------------------------------------------------------------

/// Uniform random number generator, (0,1]
float randf();

/// Gaussian random number generator, N(0,1)
void randnf(float * i, float * q);

//-----------------------------------------------------------------------------
//
// Byte packing functions
//
//-----------------------------------------------------------------------------
void pack_bytes(
    unsigned char * input,
    unsigned int input_length,
    unsigned char * output,
    unsigned int output_length,
    unsigned int *num_written);

void unpack_bytes(
    unsigned char * input,
    unsigned int input_length,
    unsigned char * output,
    unsigned int output_length,
    unsigned int *num_written);

void repack_bytes(
    unsigned char * input,
    unsigned int input_sym_size,
    unsigned int input_length,
    unsigned char * output,
    unsigned int output_sym_size,
    unsigned int output_length,
    unsigned int *num_written);




class fir_filter
{
public:
    fir_filter(float a[], unsigned int len_a);

    void do_work(bool run_filter, short in_sample, short &out_sample);
    void reset();

private:
    fir_filter(const fir_filter &);

#ifdef FPM
    mad_fixed_t *A;
    mad_fixed_t *v;
#else
    float *A;
    short *v;
#endif
    unsigned int len_A;

    unsigned int len_v;
    unsigned int next_v;
};

class dump_data
{
public:
    dump_data(const char *filename, long start_sample, long number_of_samples);
    ~dump_data();

    void write_data(float data, const char *msg = "");
    void write_data(float a, float b, const char *msg = "");

private:
    dump_data();
    dump_data(const dump_data &);

    std::ofstream *out_file;

    long start_sample, stop_sample;
    long current_sample;

};

class dc_block
{
public:
    dc_block(const float forget_factor);
    ~dc_block();

    void do_work(short in, short &out);

private:
    dc_block();
    dc_block(const dc_block &);

    float forget_factor;
    int prev_input, prev_output;

};

//-----------------------------------------------------------------------------
//
// Circular buffer definitions
//
//-----------------------------------------------------------------------------

// Initializing constructor (empty)
template <class T>
CircularBuffer<T>::CircularBuffer()
{
    bufferSize = 1;
    numElements = 0;
    i_head = 0;
    i_read = 0;
    headPtr = new T[bufferSize];
}

// Initializing constructor (empty)
template <class T>
CircularBuffer<T>::CircularBuffer(unsigned int _bufferSize)
{
    bufferSize = _bufferSize;
    numElements = 0;
    i_head = 0;
    i_read = 0;
    headPtr = new T[bufferSize];
}

// Initializing constructor (array)
template <class T>
CircularBuffer<T>::CircularBuffer(T * _v, unsigned int _bufferSize)
{
    bufferSize = _bufferSize;
    numElements = 0;
    i_head = 0;
    i_read = 0;
    headPtr = new T[bufferSize];
    for (unsigned int i=0; i<bufferSize; i++)
        Push( _v[i] );
}

// Copy constructor
template <class T>
CircularBuffer<T>::CircularBuffer(CircularBuffer & _cb)
{
    bufferSize = _cb.bufferSize;
    numElements = _cb.numElements;
    i_head = _cb.i_head;
    i_read = _cb.i_read;
    headPtr = new T[bufferSize];
    for (unsigned int i=0; i<bufferSize; i++)
        headPtr[i] = _cb.headPtr[i];
}

// Set the buffer size dynamically
template <class T>
void CircularBuffer<T>::SetBufferSize(unsigned int _bufferSize)
{
    if ( _bufferSize < 1 ) {
        std::cerr << "ERROR: SigProc::CircularBuffer::SetBufferSize()" << std::endl
                  << "  => minimum buffer size is 1" << std::endl;
        throw 0;
    }

    if ( _bufferSize == bufferSize ) {
        // Nothing to do
        return;
    } else if ( _bufferSize < bufferSize && numElements > _bufferSize ) {
        // New buffer is too small: copy only newest elements, discard oldest
        i_read = ( i_read + numElements - _bufferSize ) % bufferSize;
        numElements = _bufferSize;
    } else {
        // New buffer is sufficiently large: copy everything
    }

// allocate new buffer memory
    T * tmpHeadPtr = new T[_bufferSize];

    for (unsigned int i=0; i<numElements; i++)
        tmpHeadPtr[i] = headPtr[i_read++ % bufferSize];

// delete old buffer
    delete [] headPtr;

    headPtr = tmpHeadPtr;
    i_head = numElements % bufferSize;
    i_read = 0;

    bufferSize = _bufferSize;
}

// Linearize buffer
template <class T>
void CircularBuffer<T>::Linearize()
{
    if ( numElements == 0 )
        return;

    T * tmpHeadPtr = new T[bufferSize];

    for (unsigned int i=0; i<numElements; i++)
        tmpHeadPtr[i] = headPtr[i_read++ % bufferSize];

    delete [] headPtr;
    headPtr = tmpHeadPtr;
    i_head = numElements % bufferSize;
    i_read = 0;
}

// enum DemodScheme {
//     HARD = 0,
//     SOFT_TRUE = 1,
//     SOFT_STANDARD = 2,
//     SOFT_HIGHSNR = 3
// };

// void DemodQAM(unsigned int M, signed short X, signed short Y, DemodScheme scheme, signed char *bitsOut);

// void DemodPSK(unsigned int M, signed short X, signed short Y, DemodScheme scheme, signed char *bitsOut);


///
enum ModulationScheme {
    UNKNOWN,                                // Unknown modulation scheme
    BPSK,   QPSK,   PSK8,   PSK16,          // Phase shift keying
    DBPSK,  DQPSK,  DPSK8,  DPSK16,         // Differential PSK
    PAM4,   PAM8,   PAM16,  PAM32,          // Pulse amplitude modulation
    BFSK,   FSK4,   FSK8,   FSK16,          // Frequency shift keying
    QAM4, QAM16,  QAM32,  QAM64,  QAM128, QAM256  // Quadrature amplitude modulation
};

#define BPSK_LEVEL      10000   ///< BPSK amplitude (RMS=10000)
#define QPSK_LEVEL      10000   ///< QPSK amplitude (RMS=10000)
#define QAM4_LEVEL      7071    ///< QAM4 amplitude (RMS=10000)
#define PSK8_LEVEL_1    7071    ///< Low 8-PSK amplitude (RMS=10000)
#define PSK8_LEVEL_2    10000   ///< High 8-PSK amplitude (RMS=10000)
#define QAM16_LEVEL_1   3162    ///< Low 16-QAM amplitude (RMS=10000)
#define QAM16_LEVEL_2   9487    ///< High 16-QAM amplitude (RMS=10000)
#define PAM4_LEVEL_1    4472    ///< Low 4-PAM amplitude (RMS=10000)
#define PAM4_LEVEL_2    13416   ///< High 4-PAM amplitude (RMS=10000)

/// Modulates a symbol into an I/Q pair for binary phase shift keying
///
/// \image latex ConstellationBPSK.eps "BPSK constellation"
/// \image html ConstellationBPSK.png "BPSK constellation"
void ModulateBPSK(short symbol_in, short &I_out, short &Q_out);

/// Modulates a symbol into an I/Q pair for quadrature phase shift keying
///
// \image latex ConstellationQPSK.eps "QPSK constellation"
// \image html ConstellationQPSK.png "QPSK constellation"
void ModulateQPSK(short symbol_in, short &I_out, short &Q_out);

/// Modulates a symbol into an I/Q pair for quadrature amplitude shift keying
///
/// \image latex ConstellationQAM4.eps "QAM4 constellation"
/// \image html ConstellationQAM4.png "QAM4 constellation"
void ModulateQAM4(short symbol_in, short &I_out, short &Q_out);

/// Modulates a symbol into an I/Q pair for 8-ary phase shift keying
///
/// \image latex Constellation8PSK.eps "8-PSK constellation"
/// \image html Constellation8PSK.png "8-PSK constellation"
void Modulate8PSK(short symbol_in, short &I_out, short &Q_out);

/// Modulates a symbol into an I/Q pair for 16-point quadrature
/// amplitude modulation
///
/// \image latex Constellation16QAM.eps "16-QAM constellation"
/// \image html Constellation16QAM.png "16-QAM constellation"
void Modulate16QAM(short symbol_in, short &I_out, short &Q_out);

/// Modulates a symbol into an I/Q pair for 4-ary pulse amplitude
/// modulation
///
/// \image latex Constellation4PAM.eps "4-PAM constellation"
/// \image html Constellation4PAM.png "4-PAM constellation"
void Modulate4PAM(short symbol_in, short &I_out, short &Q_out);


#define DEMOD_COS_22_5 0.923879532511287
#define DEMOD_SIN_22_5 0.382683432365090

#define QAM16_THRESHOLD 6324    ///< 16-QAM threshold for RMS=10000 signal
#define PAM4_THRESHOLD  8944    ///< 4-PAM threshold for RMS=10000 signal

///
void DemodulateBPSK(short I_in, short Q_in, short &symbol_out);

///
void DemodulateQPSK(short I_in, short Q_in, short &symbol_out);

///
void DemodulateQAM4(short I_in, short Q_in, short &symbol_out);

///
void Demodulate8PSK(short I_in, short Q_in, short &symbol_out);

///
void Demodulate16QAM(short I_in, short Q_in, short &symbol_out);

///
void Demodulate4PAM(short I_in, short Q_in, short &symbol_out);

}
#endif


