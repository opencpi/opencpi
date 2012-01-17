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

/*! \file This file describes the bit sequencing classes and functions
 *
 */

#include "SigProc.h"
#include <cstring>
namespace SigProc
{

//-----------------------------------------------------------------------------
//
// P/N Sequence
//
//-----------------------------------------------------------------------------

PNSequence::PNSequence(unsigned int _g, unsigned int _a)
{
    unsigned int i;

// extract shift register length from generator polynomial
    unsigned long g_tmp(_g);
    m = 0;
// this loop effectively counts the placement of the MSB in _g
    for (i=0; i<sizeof(unsigned long)*8; i++) {
        if ( g_tmp & 0x0001 )
            m = i;
        g_tmp >>= 1;
    }

    n = 1;
    for (i=0; i<m; i++)
        n <<= 1;
    n--;

// generating polynomial
    g = new char[m];
    for (i=0; i<m; i++) {
        g[m-i-1] = ( _g & 0x0001 ) ? 1 : 0;
        _g >>= 1;
    }

// initial polynomial state
    a = new char[m];
    for (i=0; i<m; i++) {
        a[m-i-1] = ( _a & 0x0001 ) ? 1 : 0;
        _a >>= 1;
    }

// output
    s = new char[n];
    memset(s, 0x00, n);

}

PNSequence::~PNSequence()
{
    delete [] g;
    delete [] a;
    delete [] s;
}

} // namespace SigProc

