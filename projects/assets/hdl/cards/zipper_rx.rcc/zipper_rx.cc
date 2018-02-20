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
 * THIS FILE WAS ORIGINALLY GENERATED ON Wed Mar  2 12:36:30 2016 EST
 * BASED ON THE FILE: zipper_rx.xml
 * YOU *ARE* EXPECTED TO EDIT IT
 *
 * This file contains the implementation skeleton for the zipper_rx worker in C++
 */

#include "zipper_rx-worker.hh"
#include "OcpiApi.h"
#include <iostream>
#include <string.h>
#include <sstream>
#include <vector>

using namespace OCPI::RCC; // for easy access to RCC data types and constants
using namespace Zipper_rxWorkerTypes;
using namespace std;
namespace OA = OCPI::API;

class Zipper_rxWorker : public Zipper_rxWorkerBase {

  bool isStarting;

  RCCResult initialize()
  {
    OA::Application &app  = getApplication();

    // need to setup some hardcoded things that won't change on the zipper
    app.setProperty("rf_rx_proxy", "post_mixer_dc_offset_i", "0x43");
    app.setProperty("rf_rx_proxy", "post_mixer_dc_offset_q", "0x06");
    app.setProperty("rf_rx", "rx_vcm", "0x32");

    return RCC_OK;
  }

  RCCResult start()
  {
    isStarting=true;
    // Now that all properties have been set, verify 'written' values
    RCCResult res = rf_gain_dB_written();
    if (res != RCC_OK)
    {
      return res;
    }
    res = bb_gain_dB_written();
    if (res != RCC_OK)
    {
      return res;
    }
    res = frequency_MHz_written();
    if (res != RCC_OK)
    {
      return res;
    }
    res = sample_rate_MHz_written();
    if (res != RCC_OK)
    {
      return res;
    }
    res = bb_cutoff_frequency_MHz_written();
    if (res != RCC_OK)
    {
      return res;
    }
    return RCC_OK;
  }

  RCCResult run(bool /*timedout*/)
  {
    return RCC_ADVANCE;
  }

  RCCResult rf_gain_dB_written()
  {
    // We only want to do this if running (all other properties stable)
    if (not (isOperating() or isSuspended() or isStarting))
      return RCC_OK;

    if (m_properties.rf_gain_dB > m_properties.rf_gain_max_dB)
    {
      return setError("RX rf gain too high (\"%f dB\") can only be %f to %f",
                      m_properties.rf_gain_dB,
                      m_properties.rf_gain_min_dB,
                      m_properties.rf_gain_max_dB);
    }

    if (m_properties.rf_gain_dB < m_properties.rf_gain_min_dB)
    {
      return setError("RX rf gain too low  (\"%f dB\") can only be %f to %f",
                      m_properties.rf_gain_dB,
                      m_properties.rf_gain_min_dB,
                      m_properties.rf_gain_max_dB);
    }

    OA::Application &app  = getApplication();


    if (m_properties.rf_gain_dB > 2)
    {
      app.setProperty("rf_rx_proxy", "input_gain_db", "6");
    }
    else if (m_properties.rf_gain_dB < -2)
    {
      app.setProperty("rf_rx_proxy", "input_gain_db", "-6");
    }
    else
    {
      app.setProperty("rf_rx_proxy", "input_gain_db", "0");
    }

    return RCC_OK;
  }

  RCCResult bb_gain_dB_written()
  {
    // We only want to do this if running (all other properties stable)
    if (not (isOperating() or isSuspended() or isStarting))
      return RCC_OK;

    if (m_properties.bb_gain_dB > m_properties.bb_gain_max_dB)
    {
      return setError("RX baseband gain too high (\"%f dB\") can only be %f to %f",
                      m_properties.bb_gain_dB,
                      m_properties.bb_gain_min_dB,
                      m_properties.bb_gain_max_dB);
    }

    if (m_properties.bb_gain_dB < m_properties.bb_gain_min_dB)
    {
      return setError("RX baseband gain too low  (\"%f dB\") can only be %f to %f",
                      m_properties.bb_gain_dB,
                      m_properties.bb_gain_min_dB,
                      m_properties.bb_gain_max_dB);
    }

    OA::Application &app  = getApplication();
    std::ostringstream strs;

    double curr_difference = m_properties.bb_gain_dB;

    if (curr_difference >= 30)
    {
      app.setProperty("rf_rx_proxy", "pre_lpf_gain_db", "30");
      curr_difference = curr_difference - 30.0;
    }
    else if (curr_difference >= 19)
    {
      app.setProperty("rf_rx_proxy", "pre_lpf_gain_db", "19");
      curr_difference = curr_difference - 19.0;
    }
    else
    {
      app.setProperty("rf_rx_proxy", "pre_lpf_gain_db", "5");
      curr_difference = curr_difference - 5.0;
    }

    // need to force this to the closest multiple of 3
    int modValue = (int) curr_difference % 3;

    strs << curr_difference - modValue;
    std::string propStr = strs.str();

    // cout << " trying to do : " << curr_difference
    //      << " but we are doing mod: " << modValue
    //      << " so we really do" << propStr << endl;
    app.setProperty("rf_rx_proxy", "post_lpf_gain_db", propStr.c_str());

    return RCC_OK;
  }

  RCCResult frequency_MHz_written()
  {
    // We only want to do this if running (all other properties stable)
    if (not (isOperating() or isSuspended() or isStarting))
      return RCC_OK;

    if (m_properties.frequency_MHz > m_properties.frequency_max_MHz)
    {
      return setError("RX frequency too high (\"%f MHz\") can only be %f to %f",
                      m_properties.frequency_MHz,
                      m_properties.frequency_min_MHz,
                      m_properties.frequency_max_MHz);
    }

    if (m_properties.frequency_MHz < m_properties.frequency_min_MHz)
    {
      return setError("RX frequency too low  (\"%f MHz\") can only be %f to %f",
                      m_properties.frequency_MHz,
                      m_properties.frequency_min_MHz,
                      m_properties.frequency_max_MHz);
    }

    OA::Application &app  = getApplication();
    std::ostringstream strs;

    strs << m_properties.frequency_MHz * 1e6;
    std::string propStr = strs.str();
    app.setProperty("rf_rx_proxy", "center_freq_hz", propStr.c_str());
    app.setProperty("rf_rx_proxy",   "input_select", "3");

    /*
    if(m_properties.frequency_MHz <= 849) && (m_properties.frequency_MHz >= 824)
    {
      app.setProperty("rf_rx_proxy", "input_select", "1");
      set gpio worker ?
    }
    else if (m_properties.frequency_MHz <= 1447) && (m_properties.frequency_MHz >= 1427)
    {
      app.setProperty("rf_rx_proxy", "input_select", "2");
      set gpio worker ?
    }
    else
    {
      app.setProperty("rf_rx_proxy",   "input_select", "3");
      set gpio worker ?
    }
    */
    return RCC_OK;
  }

  RCCResult sample_rate_MHz_written()
  {
    // We only want to do this if running (all other properties stable)
    if (not (isOperating() or isSuspended() or isStarting))
      return RCC_OK;

    if (m_properties.sample_rate_MHz > m_properties.sample_rate_max_MHz)
    {
      return setError("RX sample rate too high (\"%f MHz\") can only be %f to %f",
                      m_properties.sample_rate_MHz,
                      m_properties.sample_rate_min_MHz,
                      m_properties.sample_rate_max_MHz);
    }

    if (m_properties.sample_rate_MHz < m_properties.sample_rate_min_MHz)
    {
      return setError("RX sample rate too low  (\"%f MHz\") can only be %f to %f",
                      m_properties.sample_rate_MHz,
                      m_properties.sample_rate_min_MHz,
                      m_properties.sample_rate_max_MHz);
    }

    OA::Application &app  = getApplication();
    std::ostringstream strs;

    strs << (m_properties.sample_rate_MHz * 1e6) * 2;
    std::string propStr(strs.str());
    strs.str(""); // re-use
    std::string txClkStr;
    std::string value;

    //cout << "0" << endl;
    app.getProperty("clock_gen", "channels", value);

    //cout << "1" << endl;
    { // anon block
      // Find out how many in response
      std::vector<size_t> indx;
      size_t in = 0;
      do {
        indx.push_back(in);
      } while ((in = value.find('{',in+1)) != std::string::npos);
      // std::cout << indx.size() << " responses to parse" << std::endl;

      if (indx.size() < 5)
      {
        txClkStr = "0";
        //cout << "rxif" << endl;
      }
      else {
	    //cout << "input is: " << value << endl;
	    //cout << "index is " << endl;
	    //for (int i=0; i < indx.size(); i++)
	    //{
	    	//cout << indx[i] << endl;
	    //}
        // Everything between the first space and the first comma
        size_t space = value.find(' ',indx[4]);
        size_t comma = value.find(',',indx[4]);
        txClkStr = value.substr(space+1, comma-space-1);
        //cout << "rxelse space:" << space << " comma "<< comma
        	// << " " << space+1 << " to " << comma-space-1 << endl;
      }
    }

    //cout << "set rx to" << propStr << endl;

    // There should be a cleaner way to do this...

    // CH 4&5 tx CH 2&3 rx

    //cout << "my propSTR is : " << propStr << endl;
    //cout << "my txSTR is : "   << txClkStr << endl;
    strs <<
      // CH0
      "{output_hz 0,source 0x0,inverted false," <<
      "spread none,spreadAmount 0,disabled_mode z}," <<
      // CH1
      "{output_hz 0,source 0x0,inverted false," <<
      "spread none,spreadAmount 0,disabled_mode z}," <<
      // CH2
      "{output_hz " << propStr <<
      ",source 0x0,inverted false," <<
      "spread none,spreadAmount 0,disabled_mode z}," <<
      // CH3
      "{output_hz " << propStr <<
      ",source 0x0,inverted false," <<
      "spread none,spreadAmount 0,disabled_mode z}," <<
      // CH4
      "{output_hz " << txClkStr <<
      ",source 0x0,inverted false," <<
      "spread none,spreadAmount 0,disabled_mode z}," <<
      // CH5
      "{output_hz " << txClkStr <<
      ",source 0x0,inverted false," <<
      "spread none,spreadAmount 0,disabled_mode z}," <<
      // CH6
      "{output_hz 0,source 0x0,inverted false," <<
      "spread none,spreadAmount 0,disabled_mode z}," <<
      // CH7
      "{output_hz 0,source 0x0,inverted false," <<
      "spread none,spreadAmount 0,disabled_mode z}";

    //cout << "set prop to: " << strs.str() << endl;

    app.setProperty("clock_gen", "channels", strs.str().c_str());

    return RCC_OK;
  }

  RCCResult bb_cutoff_frequency_MHz_written()
  {
    // We only want to do this if running (all other properties stable)
    if (not (isOperating() or isSuspended() or isStarting))
      return RCC_OK;

    if (m_properties.bb_cutoff_frequency_MHz > m_properties.bb_cutoff_frequency_max_MHz)
    {
      return setError("RX baseband cut-off frequency too high (\"%f MHz\") can only be %f to %f",
                      m_properties.bb_cutoff_frequency_MHz,
                      m_properties.bb_cutoff_frequency_min_MHz,
                      m_properties.bb_cutoff_frequency_max_MHz);
    }

    if (m_properties.bb_cutoff_frequency_MHz < m_properties.bb_cutoff_frequency_min_MHz)
    {
      return setError("RX baseband cut-off frequency too low  (\"%f MHz\") can only be %f to %f",
                      m_properties.bb_cutoff_frequency_MHz,
                      m_properties.bb_cutoff_frequency_min_MHz,
                      m_properties.bb_cutoff_frequency_max_MHz);
    }

    OA::Application &app  = getApplication();
    std::string propStr;

    // availible values are : {14, 10, 7, 6, 5, 4.375, 3.5, 3, 2.75,
    //                         2.5, 1.92, 1.5, 1.375, 1.25, 0.875, 0.75, 0}

    if (m_properties.bb_cutoff_frequency_MHz > 10.001)
    {
      propStr = "14000000";
    }
    else if (m_properties.bb_cutoff_frequency_MHz > 7.001)
    {
      propStr = "10000000";
    }
    else if (m_properties.bb_cutoff_frequency_MHz > 6.001)
    {
      propStr = "7000000";
    }
    else if (m_properties.bb_cutoff_frequency_MHz > 5.001)
    {
      propStr = "6000000";
    }
    else if (m_properties.bb_cutoff_frequency_MHz > 4.376)
    {
      propStr = "5000000";
    }
    else if (m_properties.bb_cutoff_frequency_MHz > 3.501)
    {
      propStr = "4375000";
    }
    else if (m_properties.bb_cutoff_frequency_MHz > 3.001)
    {
      propStr = "3500000";
    }
    else if (m_properties.bb_cutoff_frequency_MHz > 2.751)
    {
      propStr = "3000000";
    }
    else if (m_properties.bb_cutoff_frequency_MHz > 2.501)
    {
      propStr = "2750000";
    }
    else if (m_properties.bb_cutoff_frequency_MHz > 1.921)
    {
      propStr = "2500000";
    }
    else if (m_properties.bb_cutoff_frequency_MHz > 1.501)
    {
      propStr = "1920000";
    }
    else if (m_properties.bb_cutoff_frequency_MHz > 1.376)
    {
      propStr = "1500000";
    }
    else if (m_properties.bb_cutoff_frequency_MHz > 1.251)
    {
      propStr = "1375000";
    }
    else if (m_properties.bb_cutoff_frequency_MHz > 0.876)
    {
      propStr = "1250000";
    }
    else if (m_properties.bb_cutoff_frequency_MHz > 0.751)
    {
      propStr = "875000";
    }
    else //if (m_properties.bb_cutoff_frequency_MHz > 0.001)
    {
      propStr = "750000";
    }
    /*else
    {
      // set to bypass
      propStr = "0";
    }*/

    app.setProperty("rf_rx_proxy", "lpf_bw_hz", propStr.c_str());

    return RCC_OK;
  }

public:
    Zipper_rxWorker(): isStarting(false) {}; // Empty constructor
};

ZIPPER_RX_START_INFO
// Insert any static info assignments here (memSize, memSizes, portInfo)
// e.g.: info.memSize = sizeof(MyMemoryStruct);
ZIPPER_RX_END_INFO
