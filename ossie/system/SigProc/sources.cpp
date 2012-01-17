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
#include <climits>
namespace SigProc
{

nco::nco(unsigned int max) : freq_index(0), max_out(max)
{

}

nco::nco() : freq_index(0), max_out(-SHRT_MIN)
{

}

void nco::do_work(short cv, short &sine, short &cosine)
{
    const int max_freq_index = 62832;
    const double twopi = 6.2831853;

// cv allowed to be -USHRT_MAX/4 < cv < USHRT_MAX/4
// For large cv this gives four samples per cycle output
    if (cv > USHRT_MAX/4)
        cv = USHRT_MAX/4;
    else if (cv < -USHRT_MAX/4)
        cv = -USHRT_MAX/4;

    freq_index += cv;
    if (freq_index > max_freq_index)
        freq_index = freq_index - max_freq_index;
    else if (freq_index < 0)
        freq_index = freq_index + max_freq_index;

    sine = (short) (max_out * sin(twopi * freq_index / max_freq_index));
    cosine = (short) (max_out * cos(twopi * freq_index / max_freq_index));
}

} // namespace SigProc
