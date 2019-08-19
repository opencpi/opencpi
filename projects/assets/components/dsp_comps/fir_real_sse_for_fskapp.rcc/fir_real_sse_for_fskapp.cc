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
 *  THIS FILE WAS ORIGINALLY GENERATED ON Tue Jun 18 09:55:55 2019 EDT
 *  BASED ON THE FILE: fir_real_sse_for_fskapp.xml
 *
 *  This file contains the implementation for the fir_real_sse_for_fskapp
 *  worker in C++. The purpose of this worker is to function as a work-a-like
 *  of the fir_real_sse.hdl in the FSK app.
 *
 *  The filter operation is done using liquid dsp library.  The real samples
 *  are coming in from input Port and floating point scaling is done before
 *  passing into liquid API. The resulting samples are truncated into 16 bits
 *  for signed Q0.15 (where Q is a fixed point number format with 15 fractional
 *  bits) output.
 *
 *  Since the RCC worker depends upon even symmetric taps architecture and
 *  liquid API used in this worker does not provide symmetric taps, the required
 *  symmetric taps generation is done in this worker using C++ STD algorithm
 *  library.
 *
 *  The fir_real_sse.hdl has a pipeline latency of num_taps + 4 clock cycles,
 *  and outputs num_taps + 3 zeros before the first filter output. This worker
 *  was written to function similar to fir_real_sse.hdl, and therefore has the
 *  same zeros-before-first-output behavior.
 *
 *  Note:
 *      When run method is called for the first time, num_taps + 3 zeros are
 *      sent to the output port. This is done only ONCE and the output buffer is
 *      resized accordingly.  The filter outputs are sent on the subsequent call
 *      to the run method.
 *
 *      The filter taps value are generated based on OCS properties
 *      (../components/dsp_comp/spec fir_real-sse-spec.xml ).
 *
 *      Libliquid can be build using fftw or with using internal fft algorithms
 *      and depending on the state of the machine that it is built on it could
 *      be either.  If the library is built with fftw, the library is then provided
 *      to rcc workers with undefined symbols when using a static library. The
 *      solution to this is to not use fftw even if it is available on the
 *      compiling system.
 *
 ********************************************************************************/

#include <algorithm>
#include <cmath>
#include <complex> // https://liquidsdr.org/doc/datastructures/ says to
#include "fir_real_sse_for_fskapp-worker.hh"
#include "liquid/liquid.h"

using namespace OCPI::RCC; // for easy access to RCC data types and constants
using namespace Fir_real_sse_for_fskappWorkerTypes;

class Fir_real_sse_for_fskappWorker : public Fir_real_sse_for_fskappWorkerBase {

    /***
     * Please refer to the header comment on pipeline latency and the required number of zero for
     * this worker to behave like hdl worker.
     **/
    static const unsigned int PIPELINE_LATENCY = FIR_REAL_SSE_FOR_FSKAPP_NUM_TAPS_P + FIR_REAL_SSE_FOR_FSKAPP_HDL_WORKER_DELAY;
    static const unsigned int m_num_zeros = PIPELINE_LATENCY -1;

    firfilt_crcf m_firfilt_handler;       // liquid dsp FIR filter object
    bool m_firfilt_handler_valid;         // std::optional would be perfect for this

    const int16_t* m_inData_ptr;
    int16_t* m_outData_ptr;

    /***
     * Filter destructor: destroys a firfilt object, freeing all internally-allocated memory.
     ***/
    RCCResult release(){
        if (m_firfilt_handler_valid) firfilt_crcf_destroy(m_firfilt_handler);
        return RCC_OK;
    }

    /***
     * Pump 0's to the output port before sending the first valid output sample from the filter operation.
     * It's implemented this way to have work-alike feature of HDL worker.
     ***/
    inline void send_zeros(){
        auto end_ptr = m_outData_ptr + static_cast<ptrdiff_t>(m_num_zeros);
        std::fill(m_outData_ptr, end_ptr, 0);
    }

    /***
     * Truncate to 16 bits for signed Q0.15 output (emulates rounding performed
     * on the output of the multipliers in fir_real_sse.hdl - see instance of
     * round_conv in macc_systolic_sym.vhd)
     ***/
    static int16_t perform_truncation(liquid_float_complex out_sample){
        double out_truncated = fmod(out_sample.real(), pow(2,15));
        return static_cast<int16_t>(round(out_truncated));
    }


    static inline float uscale(const int16_t* inData){
        return  static_cast<float>(static_cast<float>(*inData)/(pow(2,15) -1));
    }

    /***
     * Sample by sample computation is performed.  Scaling and re-Scaling
     * is done to the the input and output data to match the resolution
     * between liquid API and realStream samples.
     ***/
    void perform_filter_operation(const size_t num_of_elements) {

        liquid_float_complex liq_sample_in;  //liquid dsp sample data type
        liquid_float_complex liq_sample_out;  //liquid dsp sample data type

        for (size_t i = 0; num_of_elements > i; i++) {
            liq_sample_in.real(uscale(m_inData_ptr++));

            firfilt_crcf_push(m_firfilt_handler, liq_sample_in);
            firfilt_crcf_execute(m_firfilt_handler, &liq_sample_out);
            int16_t sample_out= perform_truncation(liq_sample_out);
            properties().peak = std::max(properties().peak, sample_out);
            *m_outData_ptr++ = sample_out;
    	}
    }

    /***
     * fir_real_sse RCC Worker Entry point
     ***/
    RCCResult run(bool /*timedout*/) {

        m_outData_ptr = out.data().real().data();

        //Initialize the FIR_REAL_SSE filter using the properties taps values at first run
        if (firstRun()){

            const unsigned int double_num_taps = 2 * FIR_REAL_SSE_FOR_FSKAPP_NUM_TAPS_P;    //filter order for symmetric array
            float symmetric_coefficients_array[double_num_taps];                            //float data type is chosen to work with liquid API

            //copy taps values from properties().taps into symmetric_coefficients_array. This fill the first half of the array.
            std::copy(properties().taps, properties().taps + FIR_REAL_SSE_FOR_FSKAPP_NUM_TAPS_P, symmetric_coefficients_array);

            //fill the second half
            std::reverse_copy(properties().taps, properties().taps + FIR_REAL_SSE_FOR_FSKAPP_NUM_TAPS_P,
                              symmetric_coefficients_array + FIR_REAL_SSE_FOR_FSKAPP_NUM_TAPS_P);

            //create FIR filter with the given taps value and the filter length
            m_firfilt_handler = firfilt_crcf_create(symmetric_coefficients_array, double_num_taps);
            m_firfilt_handler_valid = true;

    	    out.data().real().resize(m_num_zeros);
    	    send_zeros();
    	    out.advance(); //advance = send current output buffer to the output port + request a new buffer for the output port
    	    return RCC_OK;
        }else{
    	    const size_t num_of_elements = in.data().real().size();
    	    m_inData_ptr = in.data().real().data();
    	    out.data().real().resize(num_of_elements);
    	    perform_filter_operation(num_of_elements);
        }

        return  RCC_ADVANCE;
    }

public:  
  Fir_real_sse_for_fskappWorker() : m_firfilt_handler_valid(false) {}    

};

FIR_REAL_SSE_FOR_FSKAPP_START_INFO
// Insert any static info assignments here (memSize, memSizes, portInfo)
// e.g.: info.memSize = sizeof(MyMemoryStruct);
FIR_REAL_SSE_FOR_FSKAPP_END_INFO
