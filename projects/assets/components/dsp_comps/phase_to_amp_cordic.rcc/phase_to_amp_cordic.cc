/*
 * This file is protected by Copyright. Please refer to the COPYRIGHT file
 * distributed with this source distribution.
 *
 * This file is part of OpenCPI <http://www.opencpi.org>
 *
 * OpenCPI is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * OpenCPI is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Wed May 15 14:28:12 2019 EDT
 * BASED ON THE FILE: phase_to_amp_cordic.xml
 *
 * This RCC worker is intended to be used in an all-RCC worker version of the FSK application,
 * as it is currently implemented. Where components are incorrectly named to reflect their
 * implementation, rather than based on their functionality, i.e. phase_to_amp_cordic vs polar_rect.
 *
 * Additionally, this worker implements a secondary function (i.e. phase accumulator), which
 * does not respect the component (1 function) model.
 * For these reasons, and those discussed below, it is highly recommended that the usage
 * of this RCC worker be limited to the all-RCC FSK applications and not used for new applications.
 *
 * This RCC (C++) worker is a work-a-like to the HDL worker, similarly named phase_to_amp_cordic.hdl.
 * However, it does NOT implement a CORDIC algorithm (i.e. stages of shifts & adds),
 * but rather utilizes floating point cmath functions to calculate its output.
 * Due to HDL worker implementation decisions, which affect performance, the RCC worker
 * was required to implement additional (restrictive) functionality to emulate the HDL worker's
 * behavior.
 *
 * The calculations that it does first is phase accumulating the incoming real samples.
 * Then iterates over the phase values and converts from phase to radians as the input to the cmath sin and cos functions.
 * The resulting value is multiplied by the magnitude to produce the scaled output given to the output port.
 *
 * The HDL worker will not output the final samples depending on the number of CORDIC stages and pipeline stages.
 * To match the HDL implementation, the rcc worker implements a stage delay using a deque as a FIFO (m_in_FIFO).
 * The FIFO contains the previous input samples that would still be present in the HDL implementation's pipline.
 * Calculations are made when data is present in the FIFO and enough data on the input port is present to push more data out.
 * Then on the input data phase calculations are made.
 *
 * Limitations:
 * This worker does not implement the CORDIC (shifts-add) algorithm,
 * but equivalent floating point math with fixed-point adjustments (using float to integer truncation)
 */


#include "phase_to_amp_cordic-worker.hh"
#include <iostream>
#include <cmath>
#include <deque>
using namespace OCPI::RCC; // for easy access to RCC data types and constants
using namespace Phase_to_amp_cordicWorkerTypes;

class Phase_to_amp_cordicWorker : public Phase_to_amp_cordicWorkerBase
{
  const float c_scaling_factor;
  static const size_t c_elements_to_keep = PHASE_TO_AMP_CORDIC_STAGEDELAY;

  float m_lastPhase;
  size_t m_out_capacity;
  std::deque<int16_t> m_in_FIFO;
  bool m_sameInData;  // m_sameInData is used so that the FIFO is not populated with the same data more then once

  float m_magnitude;

  RCCResult initialize()
  {
    // Resets to zero after reset
    // initial values are assigned here to support older compilers
    m_lastPhase = 0;
    m_out_capacity = 0;
    m_sameInData = false;
    m_magnitude = 0;
    return RCC_OK;
  }

  RCCResult run(bool /*timedout*/)
  {
    const size_t num_of_elements = in.data().real().size();
    const size_t samples_to_send = std::min(in.data().real().size(), out.iq().data().capacity());

    // End Condition: no new elements from input data so nothing to do, end here
    if (num_of_elements == 0)
      return RCC_ADVANCE_DONE;

    m_magnitude = static_cast<float>(properties().magnitude);

    if (firstRun())
      m_out_capacity = out.iq().data().capacity();

    IqstreamIqData *outData = out.iq().data().data();
    const int16_t *inData = in.data().real().data();

    if (!m_sameInData)
      m_in_FIFO.insert(m_in_FIFO.end(), inData, inData+num_of_elements);

    size_t elements_out = 0;
    for (size_t i = 0; i < samples_to_send && m_in_FIFO.size() > c_elements_to_keep; i++)
    {
      if (properties().enable)
        *outData = phaseToAmp(m_in_FIFO.front());
      else
        *outData = bypassMode(m_in_FIFO.front());
      outData++;
      m_in_FIFO.pop_front();
      elements_out++;
    }
    out.iq().data().resize(elements_out);

    // There will always be 1 left over because the math requires two elements
    // the current and the previous.
    // however in the presence of pipeline stage delays this is overcome by the pipeline delay
    // defined in c_elements_to_keep
    if (m_in_FIFO.size() > c_elements_to_keep)
    {
      out.advance();
      m_sameInData = true;
      return RCC_OK;
    }
    else
    {
      m_sameInData = false;
      //reset location for next input run call round
      out.advance();
      in.advance();
      return RCC_OK;
    }
  }

  IqstreamIqData phaseToAmp(const int16_t &current)
  {
    m_lastPhase = m_lastPhase + current;
    const float radians = (m_lastPhase / c_scaling_factor) * M_PI;
    IqstreamIqData iq_data;
    iq_data.I = static_cast<int16_t>(m_magnitude * cos(radians));
    iq_data.Q = static_cast<int16_t>(m_magnitude * sin(radians));
    return iq_data;
  }

  IqstreamIqData bypassMode(const int16_t &current)
  {
    IqstreamIqData iq_data;
    iq_data.I = current;
    iq_data.Q = m_magnitude;
    return iq_data;
  }
public:
  Phase_to_amp_cordicWorker() : c_scaling_factor(pow(2, PHASE_TO_AMP_CORDIC_DATA_WIDTH - 1)){}
};

PHASE_TO_AMP_CORDIC_START_INFO
// Insert any static info assignments here (memSize, memSizes, portInfo)
// e.g.: info.memSize = sizeof(MyMemoryStruct);
PHASE_TO_AMP_CORDIC_END_INFO
