/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Mon Sep 29 22:01:03 2014 EDT
 * BASED ON THE FILE: si5351_proxy.xml
 * YOU *ARE* EXPECTED TO EDIT IT
 *
 * This file contains the implementation skeleton for the si5351_proxy worker in C++
 */

#include "si5351_proxy-worker.hh"
#include <math.h>


using namespace OCPI::RCC; // for easy access to RCC data types and constants
using namespace Si5351_proxyWorkerTypes;

class Si5351_proxyWorker : public Si5351_proxyWorkerBase {
  static const unsigned nPLLs = SI5351_PROXY_NSOURCES;
  static const unsigned nChannels = SI5351_PROXY_NCHANNELS;
  //Global Variables
  struct Si5351_Channel {
    int outputDivider;
    float outputFreqHz;
    float multisynthDivider;
    int pllsource;
    float phaseOffset;
    int powered;
    bool inverted;
    bool int_mode;
  } clks[nChannels];

  struct Si5351_PLL {
    double inputFreqHz;
    double VCO_Hz;
    double feedbackDivider;
    int CLKIN_DIV;
  } plls[nPLLs];

  // Just make sure all the outputs are disabled to start with...
  // Slaves are initialized first before the proxies.
  RCCResult initialize() {
    for (unsigned i = 0; i < nChannels; i++) {
      m_properties.channels[i].state = CHANNELS_STATE_OFF;
      disable(i);
    }
    return RCC_OK;
  }
  RCCResult start() {
    for (unsigned i = 0; i < nChannels; i++)
      if (m_properties.channels[i].state == CHANNELS_STATE_ON ||
	  m_properties.channels[i].state == CHANNELS_STATE_INVERT)
	if (enable(i) != RCC_OK)
	  return RCC_ERROR;
    return RCC_OK;
  }
  RCCResult run(bool /*timedout*/) {
    // This proxy doesn't do anything except respond to property writes.
    return RCC_OK;
  }
  void channels_written() {}

  //Check if clock frequency property is within bounds. If so, set up dividers. 
  //This function sets up the clocks for both Clock 4 and Clock 5 on the Si5351.
  //Clock 4 goes to the Lime, Clock 5 goes to the FPGA. They should be set to the same frequency
  RCCResult enable(unsigned i) {
    float clk_freq = m_properties.channels[i].output_hz;
    if (clk_freq < 500000 || clk_freq  > 80000000)
      return setError("Invalid frequency entered.\n"
		      "Enter Frequency in (Hz) between 500000 Hz and 80000000 Hz\n");
    unsigned source = m_properties.channels[i].source;
    plls[source].inputFreqHz = m_properties.input_hz[source];
    clks[i].powered=1;
    clks[i].inverted=0;
    clks[i].outputFreqHz=clk_freq;

    FindVCO(i);
    slave.set_out_en_ctl(slave.get_out_en_ctl() & ~(1 << i));
    return RCC_OK;
  }
  
  // Disable Clocks
  void disable(unsigned i) {
    slave.set_out_en_ctl(slave.get_out_en_ctl() | (1 << i));
  }

  //Calculate and set the divider registers. Argument is which clocks is being setup.
  //This procedure follows the Si5351 I2C programming procedure found on page 17 of the data sheet
  void FindVCO(int k)
  {
    Si5351_Channel &clk = clks[k];
    unsigned pllN = m_properties.channels[k].source;
    Si5351_PLL &pll = plls[pllN];
    
    double freq,fmin=600000000,fmax=900000000;
    double availablefreqplla[10000],bestvcoa=0,bestvcob=0;
    unsigned int bestscore=0;
    int availableintplla[10000];
    unsigned long it_second[10000];
    int DivA,DivB,DivC,a,b,temp,i,j=0;
    unsigned MSX_P1, MSX_P2,MSX_P3;
    double tmp1;
    int MSNX_P1,MSNX_P2,MSNX_P3;
    uint8_t outputEnableRegister;
    uint8_t tempReg;
	
    //Debug statement
    //printf("Setting clock %x\n",k);

    //Save output enable register
    outputEnableRegister = slave.get_out_en_ctl();

    //Disable outputs
    slave.set_out_en_ctl(0xFF);
    //printf("Reg 3 is %x\n",slave.get_out_en_ctl());

    //Power Down Output Drivers
    for (unsigned n = 0; n < nChannels; n++) {
      slave.set_clk_ctl(n, 0x80);
      // printf("Reg %u is %x\n", n+16, slave.get_clk_ctl(n));
    }
    // printf("Reg 16 is %x\n",slave.get_clk0_ctl());
    // printf("Reg 17 is %x\n",slave.get_clk1_ctl());
    // printf("Reg 18 is %x\n",slave.get_clk2_ctl());
    // printf("Reg 19 is %x\n",slave.get_clk3_ctl());
    // printf("Reg 20 is %x\n",slave.get_clk4_ctl());
    // printf("Reg 21 is %x\n",slave.get_clk5_ctl());
    // printf("Reg 22 is %x\n",slave.get_clk6_ctl());
    // printf("Reg 23 is %x\n",slave.get_clk7_ctl());

    //Setup interrupt mask
    slave.set_int_sts_mask(0xF0);
    //printf("Reg 2 is %x\n",slave.get_int_sts_mask());

    //Set input source to clkin and clkin_div to 00 (divide by 1)
    slave.set_pll_in_src(0x04);
    //printf("Reg 15 is %x\n",slave.get_pll_in_src());

    //Clear it_second array??
    for(i=0;i<60;i++)
      {
	it_second[i] = 0;
      }

    //Reseting parameters
    i=0;
    clk.pllsource = 0;
    clk.int_mode = 0;
    clk.multisynthDivider = 0;
	
    //Set freq based clk.outputFreqHz
    freq = clk.outputFreqHz > fmin ? clk.outputFreqHz :
      (clk.outputFreqHz*((fmin/clk.outputFreqHz) +
			 (((unsigned long)fmin %
			   (unsigned long)round(clk.outputFreqHz))!=0)));
       
    //Setup possible solutions arrays 
    while (freq >= fmin && freq <= fmax)
      {
	availablefreqplla[i]=freq;
	availableintplla[i]=0;
	freq=freq+clk.outputFreqHz;
	i++;
      }
	
    //Clear score variables
    j=i;
    bestscore = 0;
    bestvcoa = 0;

    //Compute score
    for(i=0;i<j;i++)
      {
	if((unsigned long)round(availablefreqplla[i])%(unsigned long)round(clk.outputFreqHz)==0)
	  {
	    it_second[i] = it_second[i]+1;
	  }

	if(it_second[i] >= bestscore)
	  {
	    bestscore = it_second[i];
	    bestvcoa = availablefreqplla[i];
	  }

      }

    pll.VCO_Hz = bestvcoa;
    pll.feedbackDivider = bestvcoa/pll.inputFreqHz;
    clk.multisynthDivider = bestvcoa/clk.outputFreqHz;

    if((unsigned long)round(bestvcoa)%(unsigned long)round(clk.outputFreqHz)==0)
      {
	clk.int_mode =true;
	clk.multisynthDivider =  bestvcoa/clk.outputFreqHz;
      }
    else
      {
	clk.int_mode=false;
	clk.multisynthDivider=bestvcoa/clk.outputFreqHz;
      }

    clk.pllsource=0;

    bestvcob=bestvcoa;

    pll.VCO_Hz=bestvcob;
    pll.feedbackDivider = bestvcob/pll.inputFreqHz;

    clk.multisynthDivider = bestvcob/clk.outputFreqHz;

    if((unsigned long)round(bestvcob)%(unsigned long)round(clk.outputFreqHz) == 0)
      {
	clk.int_mode = true;
	clk.multisynthDivider = bestvcob/clk.outputFreqHz;
      }
    else
      {
	clk.int_mode = false;
	clk.multisynthDivider = bestvcob/clk.outputFreqHz;
      }
    clk.pllsource = 1;

    if(clk.multisynthDivider<8||900<clk.multisynthDivider)
      {
	printf("Error %f\n",clk.multisynthDivider);
      }

    DivA = (int)clk.multisynthDivider;
    DivB = (int)((clk.multisynthDivider-DivA)*1048576+0.5);
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

    if(clk.outputFreqHz <= 150000000)
      {
	tmp1=128 *((float)DivB/DivC);
	MSX_P1 = 128 * DivA + floor(128 *((float)DivB/DivC)) - 512;
	MSX_P2 = 128 * DivB - DivC * floor(128 * DivB/DivC);
	MSX_P3 = DivC;
      }

    DivA = (int)pll.feedbackDivider;
    DivB = (int)((pll.feedbackDivider-DivA)*1048576+0.5);
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
    tempReg = !clk.powered << 7;
    tempReg |=  clk.int_mode <<6;
    tempReg |=  0 << 5;
    tempReg |=  clk.inverted << 4;
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
      
    }
    slave.set_ms_div_params(pllN, 0, slave.get_ms_div_params(pllN, 0) | (MSNX_P3 >> 8));
    slave.set_ms_div_params(pllN, 1, MSNX_P3);
    slave.set_ms_div_params(pllN, 2, MSNX_P1 >> 16);
    slave.set_ms_div_params(pllN, 3, MSNX_P1 >> 8);
    slave.set_ms_div_params(pllN, 4, MSNX_P1);
    slave.set_ms_div_params(pllN, 5, ((MSNX_P2 >> 16) & 0x0F)|((MSNX_P3 >> 16) << 4));
    slave.set_ms_div_params(pllN, 6, MSNX_P2 >> 8);
    slave.set_ms_div_params(pllN, 7, MSNX_P2);

    // for (unsigned n = 0; n < 8; n++)
    //   print("Reg %2u is %x\n", 26+n, slave.get_ms_na_param(n));

    slave.set_pll_reset(0xAC);
    //printf("Reg 177 is %x\n",slave.get_pll_reset());

    slave.set_out_en_ctl(outputEnableRegister & ~(1 << k));
  }   
};

SI5351_PROXY_START_INFO
// Insert any static info assignments here (memSize, memSizes, portInfo)
// e.g.: info.memSize = sizeof(MyMemoryStruct);
SI5351_PROXY_END_INFO
