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

#include <math.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include "lime_shared.h"
namespace OCPI {
  namespace Lime {
    // This function is shared with the limx_tx_proxy
    void calcDividers(double pll_hz, double lo_hz, Divider &div) {
      int freqsel_2downto0;
      unsigned long nintInt, nfracInt;
      unsigned short nint0, nfrac22downto16, nfrac15downto8;
      short x;
      double nint,nfrac;

      //Determine exponent for calculating divider values
      if (lo_hz >= 0.2325e9 && lo_hz <= 0.465e9) {
	freqsel_2downto0 = 7;
      }else if(lo_hz > 0.465e9 && lo_hz <= 0.93e9){
	freqsel_2downto0 = 6;
      }else if(lo_hz > 0.93e9 && lo_hz <= 1.86e9){
	freqsel_2downto0 = 5;
      }else if(lo_hz > 1.86e9 && lo_hz <= 3.72e9){
	freqsel_2downto0 = 4;
      } else
	freqsel_2downto0 = 0;

      //Calculate divider values
      x=pow(2,freqsel_2downto0-3);
      nint=floor((x*lo_hz)/pll_hz); //calculation for the integer part of the divider
      nfrac=((x*lo_hz)/pll_hz)-nint;
      nfrac*=pow(2,23);
      nfrac=floor(nfrac);		//calculation for the fractional part of the divider
      nintInt=(int)nint;
      nfracInt=(int)nfrac;
    
      //Calculate register settings
      div.nint = (nintInt >> 1);
      nint0 = nintInt&0x1;
      nfrac22downto16 = nfracInt >> 16;
      div.nfrac_hi = (nint0 << 7) ^ nfrac22downto16;
      nfrac15downto8 = (nfracInt & 0x0000FF00);
      div.nfrac_mid = nfrac15downto8 >> 8;
      div.nfrac_lo  = (nfracInt & 0x000000FF);
    }

    const char *setVcoCap(uint8_t (*readVtune)(void *arg),
			  void (*writeVcoCap)(void *arg, uint8_t val),
			  void *tx_rx) {
      /*
       * Calculation of VCO capacitor value can be found on page 41
       * of the LM6002D calibration and programming guide
       */
      uint8_t vtune,vcocap,indCL,indCH,vtune_desired;

      //Set VCO Cap to 31
      vcocap=31;
      writeVcoCap(tx_rx,vcocap);
      //Put 20 us sleep to address frequency tuning issues (AV-4083)
      //The reasoning behind this can be found in Lime Microsystems FAQ document
      //http://www.limemicro.com/wp-content/uploads/2015/04/FAQ_v1.0r13.pdf
      //Q: 5.3 What is the receiver and transmitter PLL lock time?
      //A: Maximum PLL lock time is 20us. 
      usleep(20);
      //Read Vtune and use to decide next step
      vtune=readVtune(tx_rx)>>6;
      //printf("Initial Condition: VcoCap is %x, Vtune is %x\n", vcocap, vtune);

      switch(vtune)
	{
	case 0:
	  //Decrement vcocap until vtune = 2
	  while(vtune != 2 and vcocap > 0)
	    {
	      vcocap--;
	      writeVcoCap(tx_rx,vcocap);
	      usleep(20);
	      vtune=readVtune(tx_rx)>>6;
	      //printf("VcoCap is %x, Vtune is %x\n", vcocap, vtune);
	    }
	  indCL = vcocap;
	  //printf("indCL is %x\n", indCL);

	  //Set VCO Cap to 31
	  vcocap=31;
	  writeVcoCap(tx_rx,vcocap);
	  usleep(20);
	  vtune=readVtune(tx_rx)>>6;
	  
	  //Increment vcocap until vtune = 1
	  while(vtune != 1 and vcocap < 63)
	    {
	      vcocap++;
	      writeVcoCap(tx_rx,vcocap);
	      usleep(20);
	      vtune=readVtune(tx_rx)>>6;
	      //printf("VcoCap is %x, Vtune is %x\n", vcocap, vtune);
	    }
	  indCH = vcocap;
	  //printf("indCH is %x\n", indCH);
	  break;
	case 1:
	  //Decrement vcocap until vtune = 0
	  while(vtune != 0)
	    {
	      vcocap--;
	      writeVcoCap(tx_rx,vcocap);
	      usleep(20);
	      vtune=readVtune(tx_rx)>>6;
	      //printf("VcoCap is %x, Vtune is %x\n", vcocap, vtune);
	    }
	  indCL = vcocap;
	  //printf("indCL is %x\n", indCL);

	  //Decrement vcocap until vtune = 2
	  while(vtune != 2 and vcocap > 0)
	    {
	      vcocap--;
	      writeVcoCap(tx_rx,vcocap);
	      usleep(20);
	      vtune=readVtune(tx_rx)>>6;
	      //printf("VcoCap is %x, Vtune is %x\n", vcocap, vtune);
	    }
	  indCH = vcocap;
	  //printf("indCH is %x\n", indCH);
	  break;
	case 2:
	  //Increment vcocap until vtune = 0
	  while(vtune != 0)
	    {
	      vcocap++;
	      writeVcoCap(tx_rx,vcocap);
	      usleep(20);
	      vtune=readVtune(tx_rx)>>6;
	      //printf("VcoCap is %x, Vtune is %x\n", vcocap, vtune);
	    }
	  indCL = vcocap;
	  //printf("indCL is %x\n", indCL);

	  //Increment vcocap until vtune = 1
	  while(vtune != 1 and vcocap < 63)
	    {
	      vcocap++;
	      writeVcoCap(tx_rx,vcocap);
	      usleep(20);
	      vtune=readVtune(tx_rx)>>6;
	      //printf("VcoCap is %x, Vtune is %x\n", vcocap, vtune);
	    }
	  indCH = vcocap;
	  //printf("indCH is %x\n", indCH);
	  break;
	default:
	  return "Invalid VTUNE register value";
	}

      //Final Vco Cap calculation
      vtune_desired=2.0;
      vcocap = round(((vtune_desired-2.5)*(indCH-indCL)/(2.5-0.5))+indCH);
      //printf("Final VcoCap is %x\n", vcocap);
      writeVcoCap(tx_rx,vcocap);
      return NULL;
    }
    const char *getLpfBwValue(float hz, uint8_t &val) {
      float bw_mhz[] =
	{14, 10, 7, 6, 5, 4.375, 3.5, 3, 2.75, 2.5, 1.92, 1.5, 1.375, 1.25, 0.875, 0.75, 0};
      for (unsigned n = 0;  bw_mhz[n] > 0.001; n++)
	if (fabs(hz/1e6 - bw_mhz[n]) < 0.001) {
	  val = n;
	  return NULL;
	}
      return
	"Value in HZ not one of these (Mhz) values are 0.75, 0.875, 1.25, 1.375, "
	"1.5, 1.92, 2.5, 2.75, 3, 3.5, 4.375, 5, 6, 7, 10, 14";
    }
    const char *getFreqValue(float hz, uint8_t &regval) {
      if (hz < 0.2325e9 || hz > 3.72e9)
	return "Invalid center freq. Frequencies must be between 0.2325e9 and 3.72e9 Hz";
      static struct {
	float max_hz;
	uint8_t regval;
      } bands[] = {
	{ 0.285625e9, 0b100111 },
	{ 0.336875e9, 0b101111 },
	{ 0.405e9,    0b110111 },
	{ 0.465e9,    0b111111 },
	{ 0.57125e9,  0b100110 },
	{ 0.67375e9,  0b101110 },
	{ 0.81e9,     0b110110 },
	{ 0.93e9,     0b111110 },
	{ 1.14245e9,  0b100101 },
	{ 1.3475e9,   0b101101 },
	{ 1.62e9,     0b110101 },
	{ 1.86e9,     0b111101 },
	{ 2.285e9,    0b100100 },
	{ 2.695e9,    0b101100 },
	{ 3.24e9,     0b110100 },
	{ 3.72e9,     0b111100 },
	{ 0.0,        0 }
      }, *bp;
      for (bp = bands; bp->regval; bp++)
	if (hz <= bp->max_hz)
	  break;
      assert(bp->regval);
      regval = bp->regval;
      return NULL;
    }
  }
}
