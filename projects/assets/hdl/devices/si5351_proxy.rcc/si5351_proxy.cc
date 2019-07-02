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
 * THIS FILE WAS ORIGINALLY GENERATED ON Mon Sep 29 22:01:03 2014 EDT
 * BASED ON THE FILE: si5351_proxy.xml
 * YOU *ARE* EXPECTED TO EDIT IT
 *
 * This file contains the implementation skeleton for the si5351_proxy worker in C++
 *
 * DISCLAIMER: 
 * The current implementation of this proxy is specific to supporting 
 * the Zipper/MyriadRF (Lime transceiver) daughtercard. As a result, 
 * it does not support the full capabilities of the Si5351 device, such as,
 * clock spreading, configuration of the R dividers in the Output Stage, 
 * or configuring outputs 6 & 7.
 */

#include "si5351_proxy-worker.hh"
#include <math.h>
#include <cstring>


using namespace OCPI::RCC; // for easy access to RCC data types and constants
using namespace Si5351_proxyWorkerTypes;

class Si5351_proxyWorker : public Si5351_proxyWorkerBase {
  RunCondition m_aRunCondition;
public:
  Si5351_proxyWorker() : m_aRunCondition(RCC_NO_PORTS) {
    //Run function should never be called
    setRunCondition(&m_aRunCondition);
  }
private:
  static const unsigned nPLLs = SI5351_PROXY_NSOURCES;
  static const unsigned nChannels = SI5351_PROXY_NCHANNELS;
  Channels savedChannels[nChannels]; // to see which channels have changed.

  // Just make sure all the outputs are disabled to start with...
  // Slaves are initialized first before the proxies.
  RCCResult initialize() {
    return disableAll();
  }

  RCCResult stop() {
    return disableAll();
  }

  RCCResult config(unsigned i) {
    setDisabledMode(i); // Set the right mode, but don't enable it, separate from counters
    if (m_properties.channels[i].output_hz > .001) {
      if (enable(i) != RCC_OK)
	return RCC_ERROR;
    } else
      disable(i);
    memcpy(&savedChannels[i], &m_properties.channels[i], sizeof(Channels));
    return RCC_OK;
  }

  // We know everything is disabled.  Configure all channels
  RCCResult start() {
    input_hz_read();
    for (unsigned i = 0; i < nChannels; i++)
      if (config(i) != RCC_OK)
	return RCC_ERROR;
    return RCC_OK;
  }

  RCCResult run(bool /*timedout*/) {
    // This proxy doesn't do anything except respond to property writes.
    return RCC_DONE;
  }

  // notification that input_hz property will be read
  RCCResult input_hz_read() {
    m_properties.input_hz[0] = slave.get_clkin_freq(); // we call the clkin source 0
    m_properties.input_hz[1] = slave.get_xtal_freq();  // we call the xtalin source 1
    return RCC_OK;
  }

  // Only process the notifications if we are operating - otherwise
  // we enable everything during "start".
  // Configure the changed channels
  RCCResult channels_written() {
    if (isOperating())
      for (unsigned i = 0; i < nChannels; i++)
	if ((m_properties.channels[i].output_hz     != savedChannels[i].output_hz)    ||
	    (m_properties.channels[i].source        != savedChannels[i].source)       ||
	    (m_properties.channels[i].inverted      != savedChannels[i].inverted)     ||
	    (m_properties.channels[i].spread        != savedChannels[i].spread)       ||
	    (m_properties.channels[i].spreadAmount  != savedChannels[i].spreadAmount) ||
	    (m_properties.channels[i].disabled_mode != savedChannels[i].disabled_mode))
	  if (config(i) != RCC_OK)
	    return RCC_ERROR;
    return RCC_OK;
  }

  // Per data sheet, disable and power down all outputs
  RCCResult disableAll() {
    if (slave.get_dev_status() & 0x80)
      return setError("SI5351 has not completed system initialization");
    slave.set_clk30_dis_st(0xAA); // drive Z when disabled
    slave.set_clk74_dis_st(0xAA); // drive Z when disabled
    // Disable output drivers
    slave.set_out_en_ctl(0xFF);
    // Make output enables not controlled by the master OEB pin so we control them
    slave.set_oeb_pin_en(0xFF);
    // Power down outputs
    for (unsigned i = 0; i < nChannels; i++)
      slave.set_clk_ctl(i, 0x80);
    // Mask all the interrupts
    slave.set_int_sts_mask(slave.get_int_sts_mask() | 0xF0);
    return RCC_OK;
  }
  // Configure how the output is driven when disabled
  void setDisabledMode(unsigned i) {
    const Channels &c = m_properties.channels[i];
    uint8_t dis = i < 4 ? slave.get_clk30_dis_st() : slave.get_clk74_dis_st();
    unsigned shift = (i & 3) * 2;
    dis &= ~(3 << shift); // default set to drive low when disabled
    switch (c.disabled_mode) {
    case CHANNELS_DISABLED_MODE_LOW: dis |= 0 << shift; break;
    case CHANNELS_DISABLED_MODE_HIGH: dis |= 1 << shift; break;
    case CHANNELS_DISABLED_MODE_Z: dis |= 2 << shift; break;
    case CHANNELS_DISABLED_MODE_NEVER: dis |= 3 << shift; break; // what does this really mean?
    default:;
    };
    if (i < 4)
      slave.set_clk30_dis_st(dis);
    else
      slave.set_clk74_dis_st(dis);
  }

  // Check if clock frequency property is within bounds. If so, set up dividers. 
  RCCResult enable(unsigned i) {
    float clk_freq = m_properties.channels[i].output_hz;
    //if (clk_freq < 2500 || clk_freq  > 200000000)
    if (clk_freq < 500000 || clk_freq  > 200000000) // R dividers are not implemented
      return setError("Invalid frequency entered.\n"
     //		      "Enter Frequency in (Hz) between 2500 Hz and 200000000 Hz\n");
    		      "Enter Frequency in (Hz) between 500000 Hz and 200000000 Hz\n");
    if (FindVCO(i) != RCC_OK)
      return RCC_ERROR;
    slave.set_clk_ctl(i, slave.get_clk_ctl(i) & ~(1 << 7));   // power up
    slave.set_out_en_ctl(slave.get_out_en_ctl() & ~(1 << i)); // output enabled
    return RCC_OK;
  }
  
  // Disable Clocks
  void disable(unsigned i) {
    slave.set_out_en_ctl(slave.get_out_en_ctl() | (1 << i));
  }

  // Calculate and set the divider registers. Argument is which clocks is being setup.
  // This procedure follows the Si5351 I2C programming procedure found on page 17 of the data sheet
  RCCResult FindVCO(int k)
  {
    float outputFreqHz = m_properties.channels[k].output_hz;
    double inputFreqHz = m_properties.input_hz[m_properties.channels[k].source];
    //double VCO_Hz;
    double feedbackDivider;
    bool int_mode;
    float multisynthDivider;
    unsigned pllN = m_properties.channels[k].source;
    double freq,fmin=600000000,fmax=900000000;
    double availablefreqplla[10000],bestvcoa=0,bestvcob=0;
    unsigned int bestscore=0;
    //int availableintplla[10000];
    unsigned long it_second[10000];
    int DivA,DivB,DivC,a,b,temp,i,j=0;
    unsigned MSX_P1, MSX_P2,MSX_P3;
    //double tmp1;
    int MSNX_P1,MSNX_P2,MSNX_P3;
    uint8_t tempReg;
	
    //Setup interrupt mask
    slave.set_int_sts_mask(0xF0);

    //Set input source to clkin and clkin_div to 00 (divide by 1)
    slave.set_pll_in_src(0x04);

    //Clear it_second array??
    for(i=0;i<60;i++)
      {
	it_second[i] = 0;
      }

    //Reseting parameters
    i=0;
    int_mode = false;
    multisynthDivider = 0;
	
    //Set freq based outputFreqHz
    freq = outputFreqHz > fmin ? outputFreqHz :
      (outputFreqHz*((fmin/outputFreqHz) +
			 (((unsigned long)fmin %
			   (unsigned long)round(outputFreqHz))!=0)));
       
    //Setup possible solutions arrays 
    while (freq >= fmin && freq <= fmax)
      {
	availablefreqplla[i]=freq;
	//availableintplla[i]=0;
	freq=freq+outputFreqHz;
	i++;
      }
	
    //Clear score variables
    j=i;
    bestscore = 0;
    bestvcoa = 0;

    //Compute score
    for(i=0;i<j;i++)
      {
	if((unsigned long)round(availablefreqplla[i])%(unsigned long)round(outputFreqHz)==0)
	  {
	    it_second[i] = it_second[i]+1;
	  }

	if(it_second[i] >= bestscore)
	  {
	    bestscore = it_second[i];
	    bestvcoa = availablefreqplla[i];
	  }

      }

    //VCO_Hz = bestvcoa;
    feedbackDivider = bestvcoa/inputFreqHz;
    multisynthDivider = bestvcoa/outputFreqHz;

    if((unsigned long)round(bestvcoa)%(unsigned long)round(outputFreqHz)==0)
      {
	int_mode = true;
	multisynthDivider =  bestvcoa/outputFreqHz;
      }
    else
      {
	int_mode = false;
	multisynthDivider=bestvcoa/outputFreqHz;
      }


    bestvcob=bestvcoa;

    //VCO_Hz=bestvcob;
    feedbackDivider = bestvcob/inputFreqHz;

    multisynthDivider = bestvcob/outputFreqHz;

    if((unsigned long)round(bestvcob)%(unsigned long)round(outputFreqHz) == 0)
      {
	int_mode = true;
	multisynthDivider = bestvcob/outputFreqHz;
      }
    else
      {
	int_mode = false;
	multisynthDivider = bestvcob/outputFreqHz;
      }

    if(multisynthDivider<8||900<multisynthDivider)
      return setError("Error setting up divider for channel %u source %u: %g",
		      k, m_properties.channels[k].source, multisynthDivider);

    DivA = (int)multisynthDivider;
    DivB = (int)((multisynthDivider-DivA)*1048576+0.5);
    DivC = 1048576;


    a = DivB;
    b = DivC;

    while(b!=0)
      {
	temp = a%b;
	a=b;
	b=temp;
      }
    DivB=DivB/a;
    DivC=DivC/a;

    if(outputFreqHz <= 150000000)
      {
	//tmp1=128 *((float)DivB/DivC);
	MSX_P1 = 128 * DivA + floor(128 *((float)DivB/DivC)) - 512;
	MSX_P2 = 128 * DivB - DivC * floor(128 * DivB/DivC);
	MSX_P3 = DivC;
      }

    DivA = (int)feedbackDivider;
    DivB = (int)((feedbackDivider-DivA)*1048576+0.5);
    DivC = 1048576;

    a = DivB;
    b = DivC;

    while(b!=0)
      {
	temp = a%b;
	a=b;
	b=temp;
      }
    DivB=DivB/a;
    DivC=DivC/a;
    MSNX_P1 = 128 * DivA + floor(128 * ((float)DivB / DivC)) - 512;
    MSNX_P2 = 128 * DivB - DivC * floor(128 * DivB / DivC);
    MSNX_P3 = DivC;

    //printf("MSX_P1 is %x\n",MSX_P1); 
    //printf("MSX_P2 is %x\n",MSX_P2);
    //printf("MSX_P3 is %x\n",MSX_P3);

    //Write clock control register (Regs 18-21)
    // We are enabling, so we must power it on == 0
    tempReg = 0; // !powered << 7;
    tempReg |=  (int_mode ? 1 : 0) <<6;
    tempReg |=  0 << 5;
    tempReg |=  m_properties.channels[k].inverted << 4;
    tempReg |=  3 << 2;
    tempReg |=  3;
    //printf("tempReg is %x\n",tempReg);

    slave.set_clk_ctl(k, tempReg);
    //printf("Reg %2u is %x\n", 16 + k, slave.get_clk_ctl(k));

    if (k < 6) {
      slave.set_ms_0_5_params(k, 0, MSX_P3 >> 8);
      slave.set_ms_0_5_params(k, 1, MSX_P3);
      slave.set_ms_0_5_params(k, 2, (MSX_P1 >> 16 ) & 0x03);
      slave.set_ms_0_5_params(k, 3, MSX_P1 >> 8);
      slave.set_ms_0_5_params(k, 4, MSX_P1);
      slave.set_ms_0_5_params(k, 5, ((MSX_P2 >> 16) & 0x0F) | ((MSX_P3 >> 16) << 4));
      slave.set_ms_0_5_params(k, 6, MSX_P2);
      slave.set_ms_0_5_params(k, 7, MSX_P2 >> 8);
    } else {
      // DOES NOT CURRENTLY SUPPORT CONFIGURATION OF OUTPUTS 6 & 7
    }
    slave.set_ms_div_params(pllN, 0, slave.get_ms_div_params(pllN, 0) | (MSNX_P3 >> 8));
    slave.set_ms_div_params(pllN, 1, MSNX_P3);
    slave.set_ms_div_params(pllN, 2, MSNX_P1 >> 16);
    slave.set_ms_div_params(pllN, 3, MSNX_P1 >> 8);
    slave.set_ms_div_params(pllN, 4, MSNX_P1);
    slave.set_ms_div_params(pllN, 5, ((MSNX_P2 >> 16) & 0x0F)|((MSNX_P3 >> 16) << 4));
    slave.set_ms_div_params(pllN, 6, MSNX_P2 >> 8);
    slave.set_ms_div_params(pllN, 7, MSNX_P2);
    // Data sheet says to write 0xAC to reset both...
    slave.set_pll_reset((slave.get_pll_reset() & ~((1<<7)|(1<<5))) | 1 << (pllN == 0 ? 5 : 7));
    return RCC_OK;
  }
};

SI5351_PROXY_START_INFO
// Insert any static info assignments here (memSize, memSizes, portInfo)
// e.g.: info.memSize = sizeof(MyMemoryStruct);
SI5351_PROXY_END_INFO
