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
 * BASED ON THE FILE: rp_cordic.xml
 *
 * This RCC worker is intended to be used in an all-RCC worker version of the FSK application,
 * as it is currently implemented. Where components are incorrectly named to reflect their
 * implementation, rather than based on their functionality, i.e. rp_cordic vs rect_polar.
 * Additionally, this workers implements a secondary function (i.e. FM discriminator), which
 * does not respect the component (1 function) model.
 * For these reasons, and those discussed below, it is highly recommended that the usage
 * of this RCC worker be limited to the all-RCC FSK applications and not used for new applications.
 * 
 * This RCC (C++) worker is a work-a-like to the HDL worker, similarly named rp_cordic.hdl.
 * However, it does NOT implement a CORDIC algorithm (i.e. stages of shifts & adds),
 * but rather utilizes floating point cmath functions to calculate its output.
 * Due to HDL worker implementation decisions, which affect performance, the RCC worker
 * was required to implements additional (restrictive) functionality to emulate the HDL worker's
 * behavior.
 * 
 * The calculations that it does first is the phase of all incoming iq samples. 
 * Then iterates over the phase values and FM discriminates the current and next phase value. 
 * The resulting output is given to the output port
 * 
 * For the magnitude calculation the HDL implements magnitude as a volatile property 
 * however for rcc, accessing the property is only viable at the completion of the run method. 
 * To reduce unnecessary computations the last sample that would be used to calculate the magnitude sets the property value.
 * 
 * The HDL worker will not output the final samples depending on the number of CORDIC stages and pipleline stages. 
 * To match the HDL implementation, the rcc worker implements a stage delay using a deque as a FIFO (m_trailing_data).
 * The FIFO contains the previous input samples that would still be present in the HDL implementation's pipline. 
 * Calculations are made when data is present in the FIFO and enough data on the input port is present to push more data out.
 * Then on the input data phase calculations are made.
 * Finally the remaining input data that would be delayed is passed to the FIFO to be used in the next calculation.
 * 
 * Memory optimization was made to load the FIFO only with samples known to be used on the next run method call.
 * 
 * Considerations are made to unload existing elements from the FIFO 
 * 
 * Limitations:  
 * 1) This worker currently does not work with stage delay set to zero. 
 *      To do so the fm_discrimination calculation cannot be lookahead 
 *      but instead look back at the previous phase value for its calculation.
 *      A variable will have to be maintained of the previous phase value when fm discriminating
 * 2) This worker does not implement the CORDIC (shifts-add) algorithm, 
 *      but equivalent floating point math with fixed-point adjustments (using float to integer truncation)

 */

#include "rp_cordic_for_fskapp-worker.hh"
#include <cmath>
#include <deque>
#include "OcpiOsDebugApi.hh" // OCPI_LOG_INFO

using namespace OCPI::RCC; // for easy access to RCC data types and constants
using namespace Rp_cordic_for_fskappWorkerTypes;

class Rp_cordic_for_fskappWorker : public Rp_cordic_for_fskappWorkerBase {
  // Other implementations can use a variable datawidth however 
  // this worker only outputs data in the 16 bit format to the rstream protocol
  static const unsigned c_data_width = 16; 
  const float c_scaling_factor;
  long total_samples_out;
  std::deque<IqstreamIqData> m_trailing_data;  

public:
  Rp_cordic_for_fskappWorker() : c_scaling_factor(pow(2,c_data_width - 1) / M_PI) { }

  RCCResult start() {
    if (RP_CORDIC_FOR_FSKAPP_STAGEDELAY == 0)
    {  
      setError("RP_CORDIC_STAGEDELAY of 0 not implemented");
      return RCC_FATAL;
    }
    log(OCPI_LOG_DEBUG, "c_scaling_factor : %f", c_scaling_factor);
    total_samples_out = 0;
    return RCC_OK;
  }

  RCCResult run(bool /*timedout*/) {

    const size_t num_of_elements = in.iq().data().size();

    // End Condition: no new elements from input data so nothing to do, end here
    if (num_of_elements == 0)
    {
      log(OCPI_LOG_DEBUG, "Remaining Samples at Done : %zu", m_trailing_data.size());
      log(OCPI_LOG_INFO, "Total Samples Out : %ld", total_samples_out);
      return RCC_ADVANCE_DONE;
    }

    auto samples_remaining_to_calculate = m_trailing_data.size() + num_of_elements;
    auto samples_to_send = (samples_remaining_to_calculate >= RP_CORDIC_FOR_FSKAPP_STAGEDELAY) ?  samples_remaining_to_calculate - RP_CORDIC_FOR_FSKAPP_STAGEDELAY : 0;

    const IqstreamIqData* inData = in.iq().data().data();
    int16_t* outData = out.data().real().data(); //phase_data  
    out.data().real().resize(samples_to_send);

    if (willLog(OCPI_LOG_DEBUG))
    {
      log(OCPI_LOG_DEBUG, "Trailing Data Counts %i" + m_trailing_data.size()); 

      for (auto itr = m_trailing_data.begin(); itr != m_trailing_data.end(); ++itr )
      {
        auto val = *itr;
        log(OCPI_LOG_DEBUG, "m_trailing_data = I: %i Q: %i", val.I , val.Q);
      }
    }
    // Edge condition: when more samples have been pushed to trailing data, may indicate more elements were added than necessary
    if(m_trailing_data.size() > RP_CORDIC_FOR_FSKAPP_STAGEDELAY)
    {  
      setError("Trailing Data larger than stage delay, more samples (%d) were added than expected (%d)", m_trailing_data.size(),RP_CORDIC_FOR_FSKAPP_STAGEDELAY);
      return RCC_FATAL;
    }
    // iq_data is used for magnitude calculation on last sample and storing the current iq for FIFO calculation
    IqstreamIqData iq_data;  
    
    // Calculate phase on previous items, this should match the number from RP_CORDIC_FOR_FSKAPP_STAGEDELAY.
    // This also makes sure that samples from the are m_trailing_data are not sent unless the total number of samples to send is greater than the stage delay.
    for (unsigned phase_values_calculated = 0; samples_to_send > phase_values_calculated && m_trailing_data.size() > 0; phase_values_calculated++)
    {
      // Magnitude could be calculated here because last sample could occur here.
      iq_data = m_trailing_data.front();
      auto phase = calculate_phase(iq_data.I, iq_data.Q);

      log(OCPI_LOG_DEBUG, "Trailing Data Calc = real : %i imag : %i magnitude : %i phase : %f", iq_data.I, iq_data.Q, properties().magnitude, phase);
      *outData++ = phase;
      m_trailing_data.pop_front();        
    }

    if (willLog(OCPI_LOG_DEBUG))
    { 
      log(OCPI_LOG_DEBUG, "Elements Remaining in trailingData : %zu", m_trailing_data.size());
      log(OCPI_LOG_DEBUG, "inData counts : %zu", num_of_elements);
      for ( unsigned i = 0; i < num_of_elements; i++ )
      {   
        log(OCPI_LOG_DEBUG, "inData = I: %i Q: %i", inData[i].I, inData[i].Q);
      }
    }
    
    // Calculate phase on incoming items
    const unsigned elements_to_compute = num_of_elements - RP_CORDIC_FOR_FSKAPP_STAGEDELAY;
    for (unsigned i = 0; num_of_elements > RP_CORDIC_FOR_FSKAPP_STAGEDELAY && i < elements_to_compute; i++)
    {
      iq_data = inData[i];
      auto phase = calculate_phase(iq_data.I, iq_data.Q);
      *outData++ = phase;
      log(OCPI_LOG_DEBUG, "inData Calc = %i imag : %i magnitude : %i phase : %f", iq_data.I, iq_data.Q, properties().magnitude, phase);
    }

    // Calculate magnitude:
    // Only compute magnitude on last sample of message since you will not normally read the value of a volatile property during run phase of a rcc worker
    // Magnitude Volatile property, Data is not accessed via the port/Data plane but from the control plane
    properties().magnitude = calculate_magnitude(iq_data.I, iq_data.Q); 

    log(OCPI_LOG_DEBUG, "Elements computed : %d",elements_to_compute);
    log(OCPI_LOG_DEBUG, "Remaining elements input elements : %zu", num_of_elements);
    
    auto in_data_counts = num_of_elements;
    // copy remaining incoming items
    for (unsigned i = elements_to_compute; i < num_of_elements; i++)
    {
      m_trailing_data.push_back(inData[i]);
      in_data_counts--;
    }
    log(OCPI_LOG_DEBUG, "Copied remaining incoming items to m_trailing_data ");

    // reset pointer location
    outData = out.data().real().data();
    // fm discrimination calculation
    if (samples_to_send > 0)
    {
      size_t i;
      // do one sample from last because this calculation looks ahead and reuses the outData Buffer
      for (i = 0; i < samples_to_send -1; i++)
      {
        outData[i] = fm_discrimination(outData[i], outData[i + 1]);
        total_samples_out++;
      }
      // Implemented lookahead for the last sample
      // lookahead in trailing_data for next phase value so that total output samples match hdl implementation
      // otherwise the total outputs will be the total number of all samples - 1
      // if stage delay is zero then trailing data will have zero elements and you cannot compute the lookahead
      // then you need the value from the previous operation and the fm_discrimination calculation will have to change and a separate buffer used for the previous sample
      if (m_trailing_data.size() > 0)
      {
        auto nextIq_data = m_trailing_data.front();
        auto phase = calculate_phase(nextIq_data.I, nextIq_data.Q);
        // notice implicit cast/truncate from float to int16_t here
        outData[i] = fm_discrimination(outData[i], phase);
        total_samples_out++;

        log(OCPI_LOG_DEBUG, "trailingData size : %zu", m_trailing_data.size());
        log(OCPI_LOG_DEBUG, "Lookahead phase : %lf", phase);
      }
    }
    
    log(OCPI_LOG_DEBUG, "inData Samples : %zu", num_of_elements);    
    log(OCPI_LOG_DEBUG, "Samples sent to outData : %zu", samples_to_send);
    log(OCPI_LOG_DEBUG, "Remaining Samples at RCC_ADVANCE : %zu", m_trailing_data.size());

    return RCC_ADVANCE;
  }

  inline float calculate_phase(int16_t real, int16_t imaginary) const {
    auto phase = atan2(imaginary, real);
    return c_scaling_factor * phase;
  }

  static inline int16_t calculate_magnitude(int16_t i, int16_t q) {
    return sqrt(pow(i, 2.0) + pow(q, 2.0));
  }
  // Handle overflow
  // the integer cast conveniently accounts for the phase wrapping of the discriminator output
  // the input to the cast operation is in the range -64k to 64k
  static inline int16_t fm_discrimination(float prev, float curr) {
    auto val = (curr - prev);
    return val;
  }
};


RP_CORDIC_FOR_FSKAPP_START_INFO
// Insert any static info assignments here (memSize, memSizes, portInfo)
// e.g.: info.memSize = sizeof(MyMemoryStruct);
RP_CORDIC_FOR_FSKAPP_END_INFO

