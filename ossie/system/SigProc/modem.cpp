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
namespace SigProc
{

//
// 0  1
//
void ModulateBPSK(
    short symbol_in,
    short &I_out,
    short &Q_out)
{
    Q_out = 0;

    switch ( symbol_in ) {
    case 0:
        I_out = -BPSK_LEVEL;
        break;
    case 1:
        I_out =  BPSK_LEVEL;
        break;
    default:
        std::cerr << "ERROR: Unknown symbol for BPSK: " << symbol_in << std::endl;
        throw 0;
    }
}


//
//    10
// 11    00
//    01
//
void ModulateQPSK(
    short symbol_in,
    short &I_out,
    short &Q_out)
{
    switch ( symbol_in ) {
    case 0:
        I_out =  QPSK_LEVEL;
        Q_out =  0;
        break;

    case 1:
        I_out =  0;
        Q_out = -QPSK_LEVEL;
        break;

    case 2:
        I_out =  0;
        Q_out =  QPSK_LEVEL;
        break;

    case 3:
        I_out = -QPSK_LEVEL;
        Q_out =  0;
        break;
    default:
        std::cerr << "ERROR: Unknown symbol for QPSK: " << symbol_in << std::endl;
        throw 0;
    }
}



//
// 10  00
// 11  01
//
void ModulateQAM4(
    short symbol_in,
    short &I_out,
    short &Q_out)
{
    switch ( symbol_in ) {
    case 0:
        I_out =  QAM4_LEVEL;
        Q_out =  QAM4_LEVEL;
        break;

    case 1:
        I_out =  QAM4_LEVEL;
        Q_out = -QAM4_LEVEL;
        break;

    case 2:
        I_out = -QAM4_LEVEL;
        Q_out =  QAM4_LEVEL;
        break;

    case 3:
        I_out = -QAM4_LEVEL;
        Q_out = -QAM4_LEVEL;
        break;
    default:
        std::cerr << "ERROR: Unknown symbol for QAM4: " << symbol_in << std::endl;
        throw 0;
    }
}

//
//       101
//    100   001
// 110         000
//    111   010
//       011
//
void Modulate8PSK(
    short symbol_in,
    short &I_out,
    short &Q_out)
{
    switch ( symbol_in ) {
    case 0:
        I_out =  PSK8_LEVEL_2;
        Q_out =  0;
        break;

    case 1:
        I_out =  PSK8_LEVEL_1;
        Q_out =  PSK8_LEVEL_1;
        break;

    case 2:
        I_out =  PSK8_LEVEL_1;
        Q_out = -PSK8_LEVEL_1;
        break;

    case 3:
        I_out =  0;
        Q_out = -PSK8_LEVEL_2;
        break;

    case 4:
        I_out = -PSK8_LEVEL_1;
        Q_out =  PSK8_LEVEL_1;
        break;

    case 5:
        I_out =  0;
        Q_out =  PSK8_LEVEL_2;
        break;

    case 6:
        I_out = -PSK8_LEVEL_2;
        Q_out =  0;
        break;

    case 7:
        I_out = -PSK8_LEVEL_1;
        Q_out = -PSK8_LEVEL_1;
        break;
    default:
        std::cerr << "ERROR: Unknown symbol for 8-PSK: " << symbol_in << std::endl;
        throw 0;
    }
}

//
//  0000    0100    1100    1000
//  0001    0101    1101    1001
//  0011    0111    1111    1011
//  0010    0110    1110    1010
//
void Modulate16QAM(
    short symbol_in,
    short &I_out,
    short &Q_out)
{
    switch ( symbol_in ) {
    case 0:
        I_out = -QAM16_LEVEL_2;
        Q_out =  QAM16_LEVEL_2;
        break;

    case 1:
        I_out = -QAM16_LEVEL_2;
        Q_out =  QAM16_LEVEL_1;
        break;

    case 2:
        I_out = -QAM16_LEVEL_2;
        Q_out = -QAM16_LEVEL_2;
        break;

    case 3:
        I_out = -QAM16_LEVEL_2;
        Q_out = -QAM16_LEVEL_1;
        break;

    case 4:
        I_out = -QAM16_LEVEL_1;
        Q_out =  QAM16_LEVEL_2;
        break;

    case 5:
        I_out = -QAM16_LEVEL_1;
        Q_out =  QAM16_LEVEL_1;
        break;

    case 6:
        I_out = -QAM16_LEVEL_1;
        Q_out = -QAM16_LEVEL_2;
        break;

    case 7:
        I_out = -QAM16_LEVEL_1;
        Q_out = -QAM16_LEVEL_1;
        break;

    case 8:
        I_out =  QAM16_LEVEL_2;
        Q_out =  QAM16_LEVEL_2;
        break;

    case 9:
        I_out =  QAM16_LEVEL_2;
        Q_out =  QAM16_LEVEL_1;
        break;

    case 10:
        I_out =  QAM16_LEVEL_2;
        Q_out = -QAM16_LEVEL_2;
        break;

    case 11:
        I_out =  QAM16_LEVEL_2;
        Q_out = -QAM16_LEVEL_1;
        break;

    case 12:
        I_out =  QAM16_LEVEL_1;
        Q_out =  QAM16_LEVEL_2;
        break;

    case 13:
        I_out =  QAM16_LEVEL_1;
        Q_out =  QAM16_LEVEL_1;
        break;

    case 14:
        I_out =  QAM16_LEVEL_1;
        Q_out = -QAM16_LEVEL_2;
        break;

    case 15:
        I_out =  QAM16_LEVEL_1;
        Q_out = -QAM16_LEVEL_1;
        break;
    default:
        std::cerr << "ERROR: Unknown symbol for 16-QAM: " << symbol_in << std::endl;
        throw 0;
    }
}


//
// 10  11  01  00
//
void Modulate4PAM(
    short symbol_in,
    short &I_out,
    short &Q_out)
{
    Q_out = 0;

    switch ( symbol_in ) {
    case 0:
        I_out =  PAM4_LEVEL_2;
        break;
    case 1:
        I_out =  PAM4_LEVEL_1;
        break;
    case 2:
        I_out = -PAM4_LEVEL_2;
        break;
    case 3:
        I_out = -PAM4_LEVEL_1;
        break;
    default:
        std::cerr << "ERROR: Unknown symbol for 4-PAM: " << symbol_in << std::endl;
        throw 0;
    }
}





//
// 0  1
//
void DemodulateBPSK(
    short I_in,
    short Q_in,
    short &symbol_out)
{
    symbol_out = ( I_in > 0 ) ? 1 : 0;
}


//
//    10
// 11    00
//    01
//
void DemodulateQPSK(
    short I_in,
    short Q_in,
    short &symbol_out)
{
// rotate constellation counter-clockwise by 45 degrees
// NOTE: pi/4 = 0.785398163397448
    short I, Q;
    rotate(I_in, Q_in, 0.785398f, &I, &Q);

    DemodulateQAM4(I, Q, symbol_out);
}

//
// 10  00
// 11  01
//
void DemodulateQAM4(
    short I_in,
    short Q_in,
    short &symbol_out)
{
    symbol_out = 0;
    unsigned short b0, b1;
    b0 = ( I_in > 0 ) ? 0 : 1;
    b1 = ( Q_in > 0 ) ? 0 : 1;

    symbol_out = (b0 << 1) + b1;
}

//
//       101
//    100   001
// 110         000
//    111   010
//       011
//
void Demodulate8PSK(
    short I_in,
    short Q_in,
    short &symbol_out)
{
    unsigned short b0, b1, b2;

// rotate constellation counter-clockwise by 22.5 degrees
// NOTE: pi/8 = 0.392699081698724
    short I, Q;
    rotate(I_in, Q_in, 0.392699f, &I, &Q);

    b0 = ( I > 0 ) ? 0 : 1;
    b1 = ( Q > 0 ) ? 0 : 1;
    if ( ( Q>I && -I>Q ) || ( Q>-I && I>Q ) )
        b2 = 0;
    else
        b2 = 1;

    symbol_out = (b0 << 2) + (b1 << 1) + b2;
}

//
//  0000    0100    1100    1000
//  0001    0101    1101    1001
//  0011    0111    1111    1011
//  0010    0110    1110    1010
//
void Demodulate16QAM(
    short I_in,
    short Q_in,
    short &symbol_out)
{
    unsigned short b0, b1, b2, b3;
    b0 = ( I_in > 0 ) ? 1 : 0;
    b1 = ( abs(I_in) > QAM16_THRESHOLD ) ? 0 : 1;
    b2 = ( Q_in > 0 ) ? 0 : 1;
    b3 = ( abs(Q_in) > QAM16_THRESHOLD ) ? 0 : 1;

    symbol_out = (b0 << 3) + (b1 << 2) + (b2 << 1) + b3;
}

//
// 10  11  01  00
//
void Demodulate4PAM(
    short I_in,
    short Q_in,
    short &symbol_out)
{
    unsigned short b0, b1;
    if ( I_in > 0 ) {
        b0 = 0;
        b1 = ( I_in >  PAM4_THRESHOLD ) ? 0 : 1;
    } else {
        b0 = 1;
        b1 = ( I_in < -PAM4_THRESHOLD ) ? 0 : 1;
    }

    symbol_out = (b0 << 1) + b1;
}

} // namespace SigProc
