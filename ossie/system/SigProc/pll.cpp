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

phase_detect::phase_detect(float _scale_factor) : scale_factor(_scale_factor)

{

}


void phase_detect::do_work(short I_in, short Q_in, short I_nco, short Q_nco, short &out)

{

    float A, B; // temps for mixer output

    A = I_in * Q_nco;
    B = Q_in * I_nco;

    int out_i = (A - B) / scale_factor * SHRT_MAX;

    if (out_i > SHRT_MAX)
        out = SHRT_MAX;
    else if (out_i < SHRT_MIN)
        out = SHRT_MIN;
    else
        out = (short) out_i;
}

} // namespace SigProc
