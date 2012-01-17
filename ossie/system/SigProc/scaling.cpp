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
// Automatic Gain Control class
//
//-----------------------------------------------------------------------------

// default constructor
AutomaticGainControl::AutomaticGainControl()
{
    energy_lo = 8192.0f;
    energy_hi = 8192.0f;
    energy_av = 8192.0f;
    ka = 0.0001f;
    kr = 0.000025f;
    gmin = 0.0f;
    gmax = 32768.0f;

    gain = 1.0f;
    energy = 0.0f;
    zeta = 0.05;
}

// destructor
AutomaticGainControl::~AutomaticGainControl()
{
}

// set signal processing values
void AutomaticGainControl::SetValues(
    float _elo,
    float _ehi,
    float _ka,
    float _kr,
    float _gmin,
    float _gmax)
{
    energy_lo = (_elo<0.0f) ? 0.0f : _elo;
    energy_hi = (_ehi>32768.0f) ? 32768.0f : _ehi;
    ka = _ka;
    kr = _kr;
    gmin = (_gmin<0.0f) ? 0.0f : _gmin;
    gmax = (_gmax>32768.0f) ? 32768.0f : _gmax;

    if ( gmin > gmax ) {
        std::cout << "WARNING: SigProc::AutomaticGainControl: minimum gain "
                  << "must be less than maximum gain"
                  << std::endl;
    }

    if ( energy_lo > energy_hi ) {
        std::cout << "WARNING: SigProc::AutomaticGainControl: low energy threshold "
                  << "must be less than high energy threshold"
                  << std::endl;
    }

    energy_av = 0.5*( energy_hi + energy_lo );
}

// get signal processing values
void AutomaticGainControl::GetValues(
    float & _elo,
    float & _ehi,
    float & _ka,
    float & _kr,
    float & _gmin,
    float & _gmax)
{
    _elo = energy_lo;
    _ehi = energy_hi;
    _ka = ka;
    _kr = kr;
    _gmin = gmin;
    _gmax = gmax;
}

// get status
void AutomaticGainControl::GetStatus(float & _gain, float & _energy)
{
    _gain = gain;
    _energy = energy;
}

// track signal energy and apply gain (real)
void AutomaticGainControl::ApplyGain(short & I)
{
// update energy value
// TODO: implement IIR low-pass filter
    energy = (1-zeta)*energy + zeta*gain*abs(I);

// update gain value
    ComputeGain();

// apply gain
    I = short( I*gain );
}

// track signal energy and apply gain (complex)
void AutomaticGainControl::ApplyGain(short & I, short & Q)
{
    short abs_I, abs_Q;
    float energy_this, I_tmp, Q_tmp;

// update energy
// TODO: implement IIR low-pass filter
// NOTE: A good approximation to A=sqrt(I^2 + Q^2) is
//   A ~= max(|I|,|Q|) + 0.30059*min(|I|,|Q|)
// This yields a MSE of about -30dB
    abs_I = abs(I);
    abs_Q = abs(Q);
    if ( abs_I > abs_Q )
        energy_this = float(abs_I) + 0.30059f*float(abs_Q);
    else
        energy_this = float(abs_Q) + 0.30059f*float(abs_I);

    energy = (1-zeta)*energy + zeta*gain*energy_this;

// update gain value
    ComputeGain();

// apply gain to I-channel, prevent clipping
    I_tmp = float(I)*gain;
    if ( I_tmp > float(SHRT_MAX) )
        I = SHRT_MAX;
    else if ( I_tmp < -float(SHRT_MAX) )
        I = -SHRT_MAX;
    else
        I = short(I_tmp);

// apply gain to Q-channel, prevent clipping
    Q_tmp = float(Q)*gain;
    if ( Q_tmp > float(SHRT_MAX) )
        Q = SHRT_MAX;
    else if ( Q_tmp < -float(SHRT_MAX) )
        Q = -SHRT_MAX;
    else
        Q = short(Q_tmp);

}

// compute gain value from energy
void AutomaticGainControl::ComputeGain()
{
    if (energy > energy_hi) {
        // energy too high; attack (decrease gain)
        gain *= 1 - ka*(energy - energy_lo)/energy;
    } else if (energy < energy_lo) {
        // energy too low; release (increase gain)
        gain *= 1 + kr*(energy_hi - energy)/energy_hi;
    } else if ( energy > energy_av ) {
        gain *= 1 - ka*(energy - energy_av)/energy_av;
    } else if ( energy < energy_av ) {
        gain *= 1 + kr*(energy_av - energy)/energy_av;
    } else {
        // energy is within acceptable limits; do nothing
    }

// hard limit gain values
    gain = (gain>gmax) ? gmax : gain;
    gain = (gain<gmin) ? gmin : gain;
}


gain::gain()

{

}

void gain::do_work(float gain, short data_in, short &out)
{

    float result = gain * data_in;

    if (result > SHRT_MAX)
        out = SHRT_MAX;
    else if (result < SHRT_MIN)
        out = SHRT_MIN;
    else
        out = (short) result;
}

dc_block::dc_block(float _f) : forget_factor(_f), prev_input(0), prev_output(0)
{

}

void dc_block::do_work(short in, short &out)

{

    int diff = in - prev_input;
    prev_input = in;

    int out_val = diff + prev_output * forget_factor;
    prev_output = out_val;

    out_val *= 10;

    if (out_val > SHRT_MAX)
        out = SHRT_MAX;
    else if (out_val < SHRT_MIN)
        out = SHRT_MIN;
    else
        out = out_val;
}

} // namespace SigProc
