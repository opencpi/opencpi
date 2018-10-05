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
 * THIS FILE WAS ORIGINALLY GENERATED ON Fri Jul 24 10:50:49 2015 EDT
 * BASED ON THE FILE: si5338_proxy.xml
 * YOU *ARE* EXPECTED TO EDIT IT
 *
 * This file contains the implementation skeleton for the si5338_proxy worker in C++
 */

#include "si5338_proxy-worker.hh"
#include <math.h>
#include <cstring>
#include <unistd.h>

using namespace OCPI::RCC; // for easy access to RCC data types and constants
using namespace Si5338_proxyWorkerTypes;

class Si5338_proxyWorker : public Si5338_proxyWorkerBase {
  RunCondition m_aRunCondition;
public:
  Si5338_proxyWorker() : m_aRunCondition(RCC_NO_PORTS) {
    //Run function should never be called
    setRunCondition(&m_aRunCondition);
  }
private:
  static const unsigned nPLLs = SI5338_PROXY_NSOURCES;
  static const unsigned nChannels = SI5338_PROXY_NCHANNELS;
  Channels savedChannels[nChannels]; // to see which channels have changed.

  RCCResult start() {
    for (unsigned i = 0; i < nChannels; i++)
      if (config(i) != RCC_OK)
	return RCC_ERROR;
    return RCC_OK;
  }
  // notification that input_hz property will be read
  RCCResult input_hz_read() {
    m_properties.input_hz[0] = slave.get_CLKIN_FREQ_p(); // we call the clkin source 0
    m_properties.input_hz[1] = slave.get_XTAL_FREQ_p();  // we call the xtalin source 1
    return RCC_OK;
  }
  // notification that channels property has been written
  RCCResult channels_written() {
    if (isOperating())
      for (unsigned i = 0; i < nChannels; i++)
	if (memcmp(&m_properties.channels[i], &savedChannels[i], sizeof(Channels)))
	  if (config(i) != RCC_OK)
	    return RCC_ERROR;
    return RCC_OK;
  }
  RCCResult run(bool /*timedout*/) {
    return RCC_DONE;
  }
  RCCResult config(unsigned i) {
    //setDisabledMode(i); // Set the right mode, but don't enable it, separate from counters
    if (m_properties.channels[i].output_hz > .001) {
      if (enable(i) != RCC_OK)
	return RCC_ERROR;
    } else
      //disable(i); // doesn't do anything so commenting out for now
    memcpy(&savedChannels[i], &m_properties.channels[i], sizeof(Channels));
    return RCC_OK;
  }
  // Check if clock frequency property is within bounds. If so, set up dividers. 
  RCCResult enable(unsigned i) {
    float clk_freq = m_properties.channels[i].output_hz;
    if (clk_freq < 200000 || clk_freq  > 80000000)
      return setError("Invalid frequency entered.\n"
		      "Enter Frequency in (Hz) between 200000 Hz and 80000000 Hz\n");
    if (configureChannel(i) != RCC_OK)
      return RCC_ERROR;
    return RCC_OK;
  }
  
  // Disable Clocks
  //void disable(unsigned i) {
    //slave.set_out_en_ctl(slave.get_out_en_ctl() | (1 << i));
  //}

  //This procedure follows the flow chart from Silicon Laboratories 
  //The flow chart can be found in the Si5338 Data Sheet on Page 22
  //Additional information about configuring the Si5338 can be found in
  //the Si5338 Reference Manual: CONFIGURING THE Si5338 WITHOUT CLOCKBUILDER DESKTOP
  RCCResult configureChannel(unsigned i){
    float fOut = m_properties.channels[i].output_hz;
    float R0_float;
    uint8_t R0, divider;

    //Calculate MS0: MS0=fVco/(fOut*R0)
    if(fOut < 5000000){
      //printf("Freq requested < 5MHz\n");
      R0_float = 5000000/fOut;
      //printf("R0_float is %f\n", R0_float);
      if(R0_float > 1 && R0_float <= 2){
	R0 = 1;//2
	divider=2;
      }
      else if(R0_float > 2 && R0_float <= 4){
	R0 = 2;//4
	divider=4;
      }
      else if(R0_float > 4 && R0_float <= 8){
	R0 = 3;//8
	divider=8;
      }
      else if(R0_float > 8 && R0_float <= 16){
	R0 = 4;//16
	divider=16;
      }
      else if(R0_float > 16 && R0_float <= 32){
	R0 = 5;//32
	divider=32;
      }
      else{
	printf("Invalid frequency entered.\n");
	printf("Enter Frequency in (Hz) between 156 kHz and 80 MHz\n");	
      }
    }
    else{
      R0 = 0;
      divider=1;
    }

    fOut = m_properties.channels[i].output_hz * divider;
    //printf("R0 is %x, fOut is %f\n", R0, fOut);

    disableOutputs();
    pauseLossOfLock();
    setRegisterMap(i,R0,fOut);
    validateInputClockStatus();
    configurePllForLocking();
    initiateLockingOfPll();
    restartLossOfLock();
    confirmPllLockStatus();
    copyFcalValues();
    setToUseFcalValues();
    enableOutputs();
    return RCC_OK;
  }

void disableOutputs(){
    //Bit 4 of register 230 (output_en_low) disables all outputs when set to 1
    slave.set_output_en_low(slave.get_output_en_low() | 0x10);
  }

  void pauseLossOfLock(){
    //Bit 7 of register 241 () prevents loss of lock status register from showing loss of lock when set to 1
    slave.set_dis_lol(slave.get_dis_lol() | 0x80);
  }
  
  void setRegisterMap(unsigned i, uint8_t R0, float fOut){
    setMultiSynthRegisters(i,R0,fOut);
    setOutputDriverParameters(i);
    setInputMultiplexors();
    setMiscRegisters();
    setConfigRegisters(i);
  }

  void validateInputClockStatus(){
    //Check bit 2 of register 218 (
    //printf("Check input clock status: %x\n",slave.get_pll_lol());
    while((slave.get_pll_lol() & 0x4) == 0x4){
	  printf("Clock not valid yet. Waiting 1 seconds\n");
	  sleep(1);
    }
  }

  void configurePllForLocking(){
    slave.set_gen_config0(slave.get_gen_config0() & 0x7F);
  }

  void initiateLockingOfPll(){
    /*Per Si5338 Ref Manual:
      "Do not use read-modify-write procedure to perform soft reset.Instead, write reg246=0x02, regardless of the current value of this bit. Reading this bit after a soft reset will return a 1."
    */
    slave.set_soft_reset(0x02);
    //printf("Sleep 25 milliseconds after reset\n");
    usleep(25000);
  }

  void restartLossOfLock(){
    //Clear bit 7
    slave.set_dis_lol(slave.get_dis_lol() & 0x7F);
    //Set reg 241 to 0x65
    slave.set_dis_lol(0x65);
  }

  void confirmPllLockStatus(){
    //Check bits 4 and 0 of register 218 and wait if necessary
    while((slave.get_pll_lol() & 0x11) == 0x11){
	printf("PLL not locked yet. Waiting 1 second\n");
	sleep(1);      
    }
  }

  void copyFcalValues(){
    //Copy 237[1:0] to 47[1:0]
    slave.set_fcal_ovrd(2,(slave.get_fcal_ovrd(2) & 0xFC) | (slave.get_vco_freq_cal(2) & 0x03));
    //Copy 236[7:0] to 46[7:0]
    slave.set_fcal_ovrd(1,slave.get_vco_freq_cal(1));
    //Copy 235[7:0] to 45[70]
    slave.set_fcal_ovrd(0,slave.get_vco_freq_cal(0));
    //Set 47[7:2] to 000101b per data sheet
    slave.set_fcal_ovrd(2,(slave.get_fcal_ovrd(2) & 0x03) | 0x14);
  }
  
  void setToUseFcalValues(){
    slave.set_gen_config0(slave.get_gen_config0() | 0x80);
  }

  void enableOutputs(){
    slave.set_output_en_low(slave.get_output_en_low() & 0xEF);
  }

  void setMultiSynthRegisters(unsigned i, uint8_t R0, float fOut){
    float MS0,MSn,fPfd;
    float p1=1;
    float fVco = 2500000000.0;
    float fIn=m_properties.input_hz[0];
    float MSn_a ,MSn_b, MSn_c;
    long MSn_p1,MSn_p2,MSn_p3;
    uint8_t reg_97,reg_98,reg_99,reg_100,reg_101,reg_102,reg_103,reg_104,reg_105,reg_106;
    double MS0_a ,MS0_b, MS0_c;
    long MS0_p1,MS0_p2,MS0_p3;	
    uint8_t reg_53,reg_54,reg_55,reg_56,reg_57,reg_58,reg_59,reg_60,reg_61,reg_62;
    float pllKphi,K,Q,msCal;
    uint8_t reg_48,reg_49,reg_50,reg_51;
    uint8_t vcoGain, rSel, bwSel, msPec;

    //printf("Frequency Requested: %f Hz\n",fOut);
    //printf("R0 Requested: %x\n",R0);

    //Calculate fPfd
    //p1 is fixed for Matchstiq-Z1 at 1
    //fIn is fixed for Matchstiq-Z1 at 30.72 MHz

    //Check if fIn was initialized
    if(fIn==0){
      input_hz_read();
      fIn=m_properties.input_hz[0];
    }
      
    fPfd=fIn/p1;
    //printf("fPfd used is : %f Hz\n",fPfd);

    //Calculate MSn: MSn = fVco/fPfd
    //fIn is fixed for Matchstiq-Z1 at 30.72 MHz
    MSn=fVco/fPfd;
    //printf("MSn used is : %f Hz\n",MSn);

    //Calculate Parameters for MSn
    //MSn = a + b/c
    //If we're rounding, b=0 and c=1;
    MSn_a = round(MSn);
    MSn_b = 1000000*(MSn-floor(MSn));	
    MSn_c = 1000000;
    //MSn_b = 0;	
    //MSn_c = 1;
    //printf("MSn_a used is : %f\n",MSn_a);
    //printf("MSn_b used is : %f\n",MSn_b);
    //printf("MSn_c used is : %f\n",MSn_c);
	
    MSn_p1 = (long)floor(((MSn_a*MSn_c+MSn_b)*128)/MSn_c-512);
    MSn_p2 = (long)((long)(MSn_b*128)%(long)MSn_c);
    MSn_p3 = (long)MSn_c;
	
    //printf("MSn_p1 used is : 0x%x\n",MSn_p1);
    //printf("MSn_p2 used is : 0x%x\n",MSn_p2);
    //printf("MSn_p3 used is : 0x%x\n",MSn_p3);

    //Calculate register values
    reg_106 = (char)(MSn_p3 >> 24) ^ 0xC0;
    reg_105 = (char)(MSn_p3 >> 16);
    reg_104 = (char)(MSn_p3 >> 8);
    reg_103 = (char)(MSn_p3);
    reg_102 = (char)(MSn_p2 >> 22) & 0xBF;
    reg_101 = (char)(MSn_p2 >> 14);
    reg_100 = (char)(MSn_p2 >> 6);
    reg_99 = (char)(MSn_p1 >> 16) ^ (char)(MSn_p2 << 2);
    reg_98 = (char)(MSn_p1 >> 8);
    reg_97 = (char)(MSn_p1);


    // printf("reg_97 used is : 0x%1x\n",(unsigned)reg_97);
    // printf("reg_98 used is : 0x%1x\n",(unsigned)reg_98);
    // printf("reg_99 used is : 0x%1x\n",(unsigned)reg_99);
    // printf("reg_100 used is : 0x%1x\n",(unsigned)reg_100);	
    // printf("reg_101 used is : 0x%1x\n",(unsigned)reg_101);
    // printf("reg_102 used is : 0x%1x\n",(unsigned)reg_102);
    // printf("reg_103 used is : 0x%1x\n",(unsigned)reg_103);
    // printf("reg_104 used is : 0x%1x\n",(unsigned)reg_104);
    // printf("reg_105 used is : 0x%1x\n",(unsigned)reg_105);
    // printf("reg_106 used is : 0x%1x\n",(unsigned)reg_106);
	
    //Set MSN registers
    slave.set_ms_n_param(0,reg_97);
    slave.set_ms_n_param(1,reg_98);
    slave.set_ms_n_param(2,reg_99);
    slave.set_ms_n_param(3,reg_100);
    slave.set_ms_n_param(4,reg_101);
    slave.set_ms_n_param(5,reg_102);
    slave.set_ms_n_param(6,reg_103);
    slave.set_ms_n_param(7,reg_104);
    slave.set_ms_n_param(8,reg_105);
    slave.set_ms_n_param(9,reg_106);
    
    //Target VCO freq is 2.5 GHz (per datasheet)
    MS0=fVco/(fOut);
    //printf("MS0 used is : %f Hz\n",MS0);

    //Calculate Parameters for MS0
    //MS0 = a + b/c
    //If we're rounding, b=0 and c=1;
    MS0_a = (long)MS0;
    MS0_b = 1000000*(MS0-floor(MS0));	
    MS0_c = 1000000;
    //MS0_b = 0;	
    //MS0_c = 1;
    // printf("MS0_a used is : %f\n",MS0_a);
    // printf("MS0_b used is : 0x%f\n",MS0_b);
    // printf("MS0_c used is : 0x%f\n",MS0_c);

    MS0_p1 = (long)floor(((MS0_a*MS0_c+MS0_b)*128)/MS0_c-512);
    MS0_p2 = (long)((long)(MS0_b*128)%(long)MS0_c);
    MS0_p3 = (long)MS0_c;
	
    // printf("MS0_p1 used is : 0x%x\n",MS0_p1);
    // printf("MS0_p2 used is : 0x%x\n",MS0_p2);
    // printf("MS0_p3 used is : 0x%x\n",MS0_p3);

    //Calculate register values
    reg_62 = (char)(MS0_p3 >> 24) & 0x00;
    reg_61 = (char)(MS0_p3 >> 16);
    reg_60 = (char)(MS0_p3 >> 8);
    reg_59 = (char)(MS0_p3);
    reg_58 = (char)(MS0_p2 >> 22) & 0xBF;
    reg_57 = (char)(MS0_p2 >> 14);
    reg_56 = (char)(MS0_p2 >> 6);
    reg_55 = (char)(MS0_p1 >> 16) ^ (char)(MS0_p2 << 2);;
    reg_54 = (char)(MS0_p1 >> 8);
    reg_53 = (char)(MS0_p1);


    // printf("reg_53 used is : 0x%1x\n",(unsigned)reg_53);
    // printf("reg_54 used is : 0x%1x\n",(unsigned)reg_54);
    // printf("reg_55 used is : 0x%1x\n",(unsigned)reg_55);
    // printf("reg_56 used is : 0x%1x\n",(unsigned)reg_56);	
    // printf("reg_57 used is : 0x%1x\n",(unsigned)reg_57);
    // printf("reg_58 used is : 0x%1x\n",(unsigned)reg_58);
    // printf("reg_59 used is : 0x%1x\n",(unsigned)reg_59);
    // printf("reg_60 used is : 0x%1x\n",(unsigned)reg_60);
    // printf("reg_61 used is : 0x%1x\n",(unsigned)reg_61);
    // printf("reg_62 used is : 0x%1x\n",(unsigned)reg_62);

    //Set MS0 registers
    slave.set_ms_0_4_params(i,1,reg_53);
    slave.set_ms_0_4_params(i,2,reg_54);
    slave.set_ms_0_4_params(i,3,reg_55);
    slave.set_ms_0_4_params(i,4,reg_56);
    slave.set_ms_0_4_params(i,5,reg_57);
    slave.set_ms_0_4_params(i,6,reg_58);
    slave.set_ms_0_4_params(i,7,reg_59);
    slave.set_ms_0_4_params(i,8,reg_60);
    slave.set_ms_0_4_params(i,9,reg_61);
    slave.set_ms_0_4_params(i,10,reg_62);

    //Set R0 divider
    //printf("reg_31 used is : 0x%1x\n", 0xC0 | (R0 << 2));
    slave.set_divout_config(i,0xC0 | (R0 << 2));

    //Calculate PLL parameters
    if(fVco > 2425000000.0){
      Q=3;
      vcoGain = 0;
    }else{
      Q=4;
      vcoGain = 1;
    }

    //Calculate PLL parameters
    if(fPfd >= 15000000){
      K=925;
      rSel=0;
      bwSel=0;
    }else if(fPfd < 15000000 && fPfd >= 8000000){
      K=325;
      rSel=1;
      bwSel=1;
    }else{
      K=185;
      rSel=3;
      bwSel=2;
    }

    pllKphi = round((K/(533*Q)*(fVco/fPfd)*pow((2500/(fVco/1000000)),3)));
    //printf("pllKphi is : %f\n",pllKphi);
	
    msCal=round(-6.67*((fVco/1000000)/1000)+20.67);
    //printf("msCal is : %f\n",msCal);
	
    msPec=0x07;

    reg_48 = (char)(pllKphi) & 0x7F; //Bit7=0 means internal feedback
    reg_49 = ((char)(vcoGain << 4) & (char)(rSel << 2) & (char)(bwSel)) & 0x7F; //Bit 7 set to 0 now. Reset to 1 once FCAL values are set
    reg_50 = (char)(msCal) ^ 0xC0; //Bits 7 and 6 are set to 1
    reg_51 = (char)(msPec) & 0x07;
    // printf("reg_48 used is : 0x%1x\n",(unsigned)reg_48);
    // printf("reg_49 used is : 0x%1x\n",(unsigned)reg_49);
    // printf("reg_50 used is : 0x%1x\n",(unsigned)reg_50);
    // printf("reg_51 used is : 0x%1x\n",(unsigned)reg_51);

    //Set config registers
    slave.set_pfd_config(reg_48);
    slave.set_gen_config0(reg_49);
    slave.set_ms_cal_val(reg_50);
    slave.set_ms_hs_mode(reg_51);
  }

  void setOutputDriverParameters(unsigned i){
    //Output Signal Type and Clock Invert
    slave.set_clk_fmt(i,0x06); //reg36-39

    //Output voltage
    slave.set_vddo_settings(0x00); //reg35

    //Output driver trim
    slave.set_trim_bits(0,0x63); //reg40
    slave.set_trim_bits(1,0x0C); //reg41
    slave.set_trim_bits(2,0x03); //reg42

    //Output Driver Enable/Disable
    slave.set_output_en_low(0x1C); //reg230

    //Output Drive State when Disable
    slave.set_ms_0_4_phs_dis_st(i,3,0x40);

  }
  void setInputMultiplexors(){
    slave.set_divin_config(0,0x8B); //reg28
    slave.set_divin_config(1,0x08); //reg29
    slave.set_divin_config(2,0x00); //reg30
  }
  void setMiscRegisters(){
    //Reg47[7:2] must be written to 000101b per reference manual
    slave.set_fcal_ovrd(2,(slave.get_fcal_ovrd(2) & 0x03) | 0x14);
   
    //Reg106[7] must be 1 per reference manual
    slave.set_ms_n_param(9,slave.get_ms_n_param(9) | 0x80);

    //Reg116[7] must be 1 per reference manual
    slave.set_ms_0_4_phs_dis_st(1,2,slave.get_ms_0_4_phs_dis_st(1,2) | 0x80);

    //Reg42[5] must be 1 per reference manual
    slave.set_trim_bits(1,slave.get_trim_bits(1) | 0x20);

    //Reg6[7:5] must be 000b per reference manual
    slave.set_mask_bits(slave.get_mask_bits() & 0x1F);

    //Reg6[1] must be 0 per reference manual
    slave.set_mask_bits(slave.get_mask_bits() & 0xFD);

    //Reg28[7:6] must be 00b per reference manual
    slave.set_divin_config(0,slave.get_divin_config(0) & 0x3F);
    
  }
  void setConfigRegisters(unsigned i){
    slave.set_ms_0_4_params(i,0,0x10); //reg52
  }

};

SI5338_PROXY_START_INFO
// Insert any static info assignments here (memSize, memSizes, portInfo)
// e.g.: info.memSize = sizeof(MyMemoryStruct);
SI5338_PROXY_END_INFO
