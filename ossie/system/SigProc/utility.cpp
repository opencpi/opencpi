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
#include <cstdlib>
#include <climits>
namespace SigProc
{

//-----------------------------------------------------------------------------
//
// Dot product definitions
//
//-----------------------------------------------------------------------------
/** \brief Calculate dot product between two floating point arrays

    \f[ z = x \otimes y \f]
 */
void dot_product(float *x, float *y, unsigned int N, float &z)
{
    z = 0.0f;
    for (unsigned int i=0; i<N; i++)
        z += x[i]*y[i];
}

/** \brief Calculate dot product between two floating short integer arrays

    \f[ z = x \otimes y \f]
 */
void dot_product(short *x, short *y, unsigned int N, short &z)
{
    float z_tmp(0.0f);
    for (unsigned int i=0; i<N; i++)
        z_tmp += float(x[i])*float(y[i]);
    if ( z_tmp > SHRT_MAX )
        z = SHRT_MAX;
    else
        z = short(z_tmp);

}


/** \brief Calculate dot product between a float and a short array

    \f[ z = x \otimes y \f]
 */
void dot_product(float *x, short *y, unsigned int N, short &z)
{
    float z_tmp;

    dot_product(x, y, N, z_tmp);

    if ( z_tmp > SHRT_MAX )
        z = SHRT_MAX;
    else if ( z_tmp < SHRT_MIN )
        z = SHRT_MIN;
    else
        z = short(z_tmp);
}


/** \brief Calculate dot product between a float and a short array

    \f[ z = x \otimes y \f]
 */
void dot_product(float *x, short *y, unsigned int N, float &z)
{
    z = 0.0f;
    for (unsigned int i=0; i<N; i++)
        z += x[i]*float(y[i]);
}



//-----------------------------------------------------------------------------
//
// Trigonometric functions
//
//-----------------------------------------------------------------------------
/** \brief Calculate inverse tangent

    \f[ \theta = \arctan(y/x);\ \ 0\le\theta< 2\pi \f]
 */
void arctan(float &x, float &y, float &theta)
{
    if (x==0.0f) {
        theta = ( y < 0 ) ? 3*PI/2 : PI/2;
        return;
    }

    theta = atan(y/x);

// rotate angle to positive value
    if (x>0 && y>=0) {
        // Q1: do nothing
    } else if (x<0 && y>=0) {
        // Q2
        theta += PI;
    } else if (x<0 && y<=0) {
        // Q3
        theta += PI;
    } else {
        // Q4
        theta += TWO_PI;
    }
}


void rotate(short I_in, short Q_in, float theta, short *I_out, short *Q_out)
{
///\todo: use CORDIC mixer to perform this operation more efficiently
    float c = cosf(theta);
    float s = sinf(theta);

    *I_out = (short)  ( (float) (I_in*c) - (float) (Q_in*s)  );
    *Q_out = (short)  ( (float) (I_in*s) + (float) (Q_in*c)  );
}


//-----------------------------------------------------------------------------
//
// Random number generators
//
//-----------------------------------------------------------------------------

float randf()
{
    float x = (float) rand();
    return x / (float) RAND_MAX;
}

void randnf(float * i, float * q)
{
// generate two uniform random numbers
    float u1, u2;

// ensure u1 does not equal zero
    do {
        u1 = randf();
    } while (u1 == 0.0f);

    u2 = randf();

    float x = sqrt(-2*logf(u1));
    *i = x * sinf(2*PI*u2);
    *q = x * cosf(2*PI*u2);
}

//-----------------------------------------------------------------------------
//
// Bitwise functions
//
//-----------------------------------------------------------------------------

/** \brief packs octets with one bit of information into a byte

 */
void pack_bytes(
    unsigned char * input,
    unsigned int input_length,
    unsigned char * output,
    unsigned int output_length,
    unsigned int * num_written)
{
    div_t d = div(input_length,8);
    unsigned int req_output_length = d.quot;
    req_output_length += ( d.rem > 0 ) ? 1 : 0;
    if ( output_length < req_output_length ) {
        perror("ERROR: SigProc::pack_bytes: output too short\n");
        return;
    }

    unsigned int i;
    unsigned int N = 0;         // number of bytes written to output
    unsigned char byte = 0;

    for (i=0; i<input_length; i++) {
        byte |= input[i] & 0x01;

        if ( (i+1)%8 == 0 ) {
            output[N++] = byte;
            byte = 0;
        } else {
            byte <<= 1;
        }
    }

    if ( i%8 != 0 )
        output[N++] = byte >> 1;

    *num_written = N;
}



void unpack_bytes(
    unsigned char * input,
    unsigned int input_length,
    unsigned char * output,
    unsigned int output_length,
    unsigned int * num_written)
{
    unsigned int i;
    unsigned int N = 0;
    unsigned char byte;

    if ( output_length < 8*input_length ) {
        perror("ERROR: SigProc::unpack_bytes: output too short\n");
        return;
    }

    for (i=0; i<input_length; i++) {
        byte = input[i];
        output[N++] = (byte >> 7) & 0x01;
        output[N++] = (byte >> 6) & 0x01;
        output[N++] = (byte >> 5) & 0x01;
        output[N++] = (byte >> 4) & 0x01;
        output[N++] = (byte >> 3) & 0x01;
        output[N++] = (byte >> 2) & 0x01;
        output[N++] = (byte >> 1) & 0x01;
        output[N++] =  byte       & 0x01;
    }

    *num_written = N;
}

void repack_bytes(
    unsigned char * input,
    unsigned int input_sym_size,
    unsigned int input_length,
    unsigned char * output,
    unsigned int output_sym_size,
    unsigned int output_length,
    unsigned int * num_written)
{
    div_t d = div(input_length*input_sym_size,output_sym_size);
    unsigned int req_output_length = d.quot;
    req_output_length += ( d.rem > 0 ) ? 1 : 0;
    if ( output_length < req_output_length ) {
        perror("ERROR: SigProc::repack_bytes: output too short\n");
        return;
    }

    unsigned int i;
    unsigned char sym_in = 0;
    unsigned char sym_out = 0;

// there is probably a more efficient way to do this, but...
    unsigned int total_bits = input_length*input_sym_size;
    unsigned int i_in = 0;  // input index
    unsigned int i_out = 0; // output index
    unsigned int k=0;       // input symbol enable
    unsigned int n=0;       // output symbol enable
    unsigned int v;         // bit mask

    for (i=0; i<total_bits; i++) {
        sym_out <<= 1;

        // push input if necessary
        if ( k == 0 )
            sym_in = input[i_in++];

        v = input_sym_size - k - 1;
        sym_out |= (sym_in >> v) & 0x01;

        // push output if available
        if ( n == output_sym_size-1 ) {
            output[i_out++] = sym_out;
            sym_out = 0;
        }

        k = (k+1) % input_sym_size;
        n = (n+1) % output_sym_size;
    }

// condition: mode(input_sym_size*input_length, output_sym_size) != 0
// push last remaining bits of input into last output symbol
    if (n != 0) {
        output[i_out++] = sym_out << (output_sym_size-n);
    }

    *num_written = i_out;
}



dump_data::dump_data(const char *filename, long _start_sample, long _number_of_samples) : start_sample(_start_sample), stop_sample(_start_sample + _number_of_samples), current_sample(0)
{

    out_file = new std::ofstream(filename);

}

dump_data::~dump_data()
{
    delete out_file;
}


void dump_data::write_data(float data, const char *msg)
{
    if ((current_sample < stop_sample) && (current_sample > start_sample)) {
        *out_file << msg << data << std::endl;
    }
    ++current_sample;
}

void dump_data::write_data(float a, float b, const char *msg)
{
    if ((current_sample < stop_sample) && (current_sample > start_sample)) {
        *out_file << msg << a << "  " << b << std::endl;
    }
    ++current_sample;
}

} // namespace SigProc
