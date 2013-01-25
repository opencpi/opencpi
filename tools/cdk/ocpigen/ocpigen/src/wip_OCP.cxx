
/*
 *  Copyright (c) Mercury Federal Systems, Inc., Arlington VA., 2009-2010
 *
 *    Mercury Federal Systems, Incorporated
 *    1901 South Bell Street
 *    Suite 402
 *    Arlington, Virginia 22202
 *    United States of America
 *    Telephone 703-413-0781
 *    FAX 703-413-0784
 *
 *  This file is part of OpenCPI (www.opencpi.org).
 *     ____                   __________   ____
 *    / __ \____  ___  ____  / ____/ __ \ /  _/ ____  _________ _
 *   / / / / __ \/ _ \/ __ \/ /   / /_/ / / /  / __ \/ ___/ __ `/
 *  / /_/ / /_/ /  __/ / / / /___/ ____/_/ / _/ /_/ / /  / /_/ /
 *  \____/ .___/\___/_/ /_/\____/_/    /___/(_)____/_/   \__, /
 *      /_/                                             /____/
 *
 *  OpenCPI is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  OpenCPI is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.
 */

// This file is for ocp interface derivation from the WIP spec
#include <strings.h>
#include <stdlib.h>
#include <stdio.h>
#include "OcpiUtilMisc.h"
#include "wip.h"

namespace OU = OCPI::Util;

#undef OCP_SIGNAL_MT
#define OCP_SIGNAL_MT(n, w) {#n, true, true, w, true, false},
#define OCP_SIGNAL_MTR(n, w) {#n, true, true, w, true, true},
#undef OCP_SIGNAL_ST
#define OCP_SIGNAL_ST(n, w) {#n, true, false, w, true, false},
#define OCP_SIGNAL_MS(n) {#n, false, true, 0, false, false},
#define OCP_SIGNAL_MV(n, w) {#n, true, true, w, false, false},
#define OCP_SIGNAL_MSR(n) {#n, false, true, 0, false, true},
#define OCP_SIGNAL_MVR(n, w) {#n, true, true, w, false, true},
#define OCP_SIGNAL_SS(n) {#n, false, false, 0, false, false},
#define OCP_SIGNAL_SV(n, w) {#n, true, false, w, false, false},
OcpSignalDesc ocpSignals [N_OCP_SIGNALS+1] = {
OCP_SIGNALS
{0,0,0,0,0,0}
};

#undef OCP_SIGNAL_MS
#undef OCP_SIGNAL_MV
#undef OCP_SIGNAL_SS
#undef OCP_SIGNAL_SV

static unsigned myfls(uint64_t n) {
  for (int i = sizeof(n)*8; i > 0; i--)
    if (n & ((uint64_t)1 << (i - 1)))
      return i;
  return 0;
}
// Derive the OCP signal configuration based on the WIP profile
uint64_t ceilLog2(uint64_t n) {
  return n ? myfls(n - 1) : 0;
}
static unsigned floorLog2(uint64_t n) {
  //  ocpiInfo("Floor log2 of %u is %u", n, myfls(n)-1);
  return myfls(n) - 1;
}
#define max(a,b) ((a) > (b) ? (a) : (b))

static void fixOCP(Port *p) {
  OcpSignal *o = p->ocp.signals;
  OcpSignalDesc *osd = ocpSignals;
  unsigned nAlloc = 0;
  for (unsigned i = 0; i < N_OCP_SIGNALS; i++, o++, osd++)
    if (o->value || o->width) {
      if (osd->vector) {
	if (osd->width)
	  o->width = osd->width;
      } else
	o->width = 1;
      nAlloc += o->width;
    }
  p->values = (uint8_t*)calloc(nAlloc, 1);
  uint8_t *v = p->values;
  o = p->ocp.signals;
  osd = ocpSignals;
  for (unsigned i = 0; i < N_OCP_SIGNALS; i++, o++, osd++)
    if (o->value || o->width) {
      o->value = v;
      v += o->width;
    }
}

const char *
deriveOCP(Worker *w) {
  //  printf("4095 %d 4096 %d\n", floorLog2(4095), floorLog2(4096));
  static uint8_t s[1]; // a non-zero string pointer
  for (unsigned i = 0; i < w->ports.size(); i++) {
    Port *p = w->ports[i];
    OcpSignals *ocp = &p->ocp;
    if (p->myClock)
      ocp->Clk.value = s;
    ocp->MCmd.width = 3;
    switch (p->type) {
    case WCIPort:
      // Do the WCI port
      p->master = false;
      if (w->ctl.sizeOfConfigSpace <= 32)
	ocp->MAddr.width = 5;
      else
	ocp->MAddr.width = ceilLog2(w->ctl.sizeOfConfigSpace);
      if (w->ctl.sizeOfConfigSpace != 0)
	ocp->MAddrSpace.value = s;
      if (w->ctl.sub32Bits)
	ocp->MByteEn.width = 4;
      if (w->ctl.writables)
	ocp->MData.width = 32;
      ocp->MFlag.width = 2;
      ocp->MReset_n.value = s;
      if (w->ctl.readables)
	ocp->SData.width = 32;
      ocp->SFlag.width = 2;  //FIXME should be 1
      ocp->SResp.value = s;
      ocp->SThreadBusy.value = s;
      break;
    case WSIPort:
      p->master = p->u.wdi.isProducer ? true : false;
      if (p->preciseBurst) {
	ocp->MBurstLength.width =
	  floorLog2((p->protocol->m_maxMessageValues * p->protocol->m_dataValueWidth  + p->dataWidth - 1)/
		    p->dataWidth) + 1;
	//	ocpiInfo("Burst %u from mmv %u dvw %u dw %u",
	//		  ocp->MBurstLength.width, p->protocol->m_maxMessageValues,
	//		  p->protocol->m_dataValueWidth, p->dataWidth);
	if (ocp->MBurstLength.width < 2)
	  ocp->MBurstLength.width = 2;
	// FIXME: this is not really supported, but was for compatibility
	if (p->impreciseBurst)
	  ocp->MBurstPrecise.value = s;
      } else
	ocp->MBurstLength.width = 2;
      if (p->byteWidth != p->dataWidth || p->protocol->m_zeroLengthMessages) {
	ocp->MByteEn.width = p->dataWidth / p->byteWidth;
	ocp->MByteEn.value = s;
      }
      if (p->dataWidth != 0)
	ocp->MData.width =
	  p->byteWidth != p->dataWidth && p->byteWidth != 8 ?
	  8 * p->dataWidth / p->byteWidth : p->dataWidth;
      if (p->byteWidth != p->dataWidth && p->byteWidth != 8)
	ocp->MDataInfo.width = p->dataWidth - (8 * p->dataWidth / p->byteWidth);
      if (p->u.wsi.earlyRequest) {
	ocp->MDataLast.value = s;
	ocp->MDataValid.value = s;
      }
      if (p->u.wsi.abortable)
	ocp->MFlag.width = 1;
      if (p->u.wdi.nOpcodes > 1)
	ocp->MReqInfo.width = ceilLog2(p->u.wdi.nOpcodes);
      ocp->MReqLast.value = s;
      ocp->MReset_n.value = s;
      ocp->SReset_n.value = s;
      ocp->SThreadBusy.value = s;
      break;
    case WMIPort:
      p->master = true;
      {
	unsigned n = (p->protocol->m_maxMessageValues * p->protocol->m_dataValueWidth +
		      p->dataWidth - 1) / p->dataWidth;
	if (n > 1) {
	  ocp->MAddr.width = ceilLog2(n);
	  unsigned nn = ceilLog2(p->dataWidth);
	  if (nn > 3)
	    ocp->MAddr.width += ceilLog2(p->dataWidth) - 3;
	}
	ocp->MAddrSpace.value = s;
	if (p->preciseBurst) {
	  ocp->MBurstLength.width = n < 4 ? 2 : floorLog2(n-1) + 1;
	  if (p->impreciseBurst)
	    ocp->MBurstPrecise.value = s;
	} else
	  ocp->MBurstLength.width = 2;
      }
      if (p->u.wdi.isProducer || p->u.wmi.talkBack || p->u.wdi.isBidirectional) {
	ocp->MData.width =
	  p->byteWidth != p->dataWidth && p->byteWidth != 8 ?
	  8 * p->dataWidth / p->byteWidth : p->dataWidth;
	if (p->byteWidth != p->dataWidth)
	  ocp->MDataByteEn.width = p->dataWidth / p->byteWidth;
	if (p->byteWidth != p->dataWidth && p->byteWidth != 8)
	  ocp->MDataInfo.width = p->dataWidth - (8 * p->dataWidth / p->byteWidth);
	ocp->MDataLast.value = s;
	ocp->MDataValid.value = s;
      }
      if ((p->u.wdi.isProducer || p->u.wdi.isBidirectional) &&
	  (p->u.wdi.nOpcodes > 1 || p->protocol->m_variableMessageLength))
	ocp->MFlag.width = 8 + ceilLog2(p->protocol->m_maxMessageValues + 1);
      ocp->MReqInfo.width = 1;
      ocp->MReqLast.value = s;
      ocp->MReset_n.value = s;
      if (!p->u.wdi.isProducer || p->u.wmi.talkBack || p->u.wdi.isBidirectional)
	ocp->SData.width = p->dataWidth;
      if (p->u.wdi.isProducer || p->u.wmi.talkBack || p->u.wdi.isBidirectional)
	ocp->SDataThreadBusy.value = s;
      if ((!p->u.wdi.isProducer || p->u.wdi.isBidirectional) &&
	  (p->u.wdi.nOpcodes > 1 || p->protocol->m_variableMessageLength))
	ocp->SFlag.width = 8 + ceilLog2(p->protocol->m_maxMessageValues + 1);
      ocp->SReset_n.value = s;
      if (!p->u.wdi.isProducer || p->u.wmi.talkBack || p->u.wdi.isBidirectional)
	ocp->SResp.value = s;
      if ((p->impreciseBurst || p->preciseBurst) &&
	  (!p->u.wdi.isProducer || p->u.wmi.talkBack || p->u.wdi.isBidirectional))
	ocp->SRespLast.value = s;
      ocp->SThreadBusy.value = s;
      if (p->u.wmi.mflagWidth) {
	ocp->MFlag.width = p->u.wmi.mflagWidth; // FIXME remove when shep kludge unnecessary
	ocp->SFlag.width = p->u.wmi.mflagWidth; // FIXME remove when shep kludge unnecessary
      }
      break;
    case WMemIPort:
      p->master = !p->u.wmemi.isSlave;
      ocp->MAddr.width =
	ceilLog2(p->u.wmemi.memoryWords) + ceilLog2(p->dataWidth/p->byteWidth);
      ocp->MAddr.value = s;
      if (p->preciseBurst)
	ocp->MBurstLength.width = floorLog2(max(2, p->u.wmemi.maxBurstLength)) + 1;
      else if (p->impreciseBurst)
	ocp->MBurstLength.width = 2;
      if (p->preciseBurst && p->impreciseBurst) {
	ocp->MBurstPrecise.value = s;
	ocp->MBurstSingleReq.value = s;
      }
      ocp->MData.width =
	p->byteWidth != p->dataWidth && p->byteWidth != 8 ?
	  8 * p->dataWidth / p->byteWidth : p->dataWidth;
      if (!p->preciseBurst && !p->impreciseBurst && p->byteWidth != p->dataWidth)
	ocp->MByteEn.width = p->dataWidth / p->byteWidth;
      if ((p->preciseBurst || p->impreciseBurst) && p->byteWidth != p->dataWidth)
	ocp->MDataByteEn.width = p->dataWidth / p->byteWidth;
      if (p->byteWidth != p->dataWidth && p->byteWidth != 8)
	ocp->MDataInfo.width = p->dataWidth - (8 * p->dataWidth / p->byteWidth);
      if (p->preciseBurst || p->impreciseBurst) {
	ocp->MDataLast.value = s;
	ocp->MDataValid.value = s;
	ocp->MReqLast.value = s;
	ocp->SRespLast.value = s;
      }
      ocp->MReset_n.value = s;
      if ((p->preciseBurst || p->impreciseBurst) &&
	  p->u.wmemi.readDataFlowControl)
	ocp->MRespAccept.value = s;
      ocp->SCmdAccept.value = s;
      ocp->SData.width =
	p->byteWidth != p->dataWidth && p->byteWidth != 8 ?
	8 * p->dataWidth / p->byteWidth : p->dataWidth;
      if ((p->preciseBurst || p->impreciseBurst) && p->u.wmemi.writeDataFlowControl)
	ocp->SDataAccept.value = s;
      if (p->byteWidth != p->dataWidth && p->byteWidth != 8)
	ocp->SDataInfo.width = p->dataWidth - (8 * p->dataWidth / p->byteWidth);
      ocp->SResp.value = s;
      break;
    case WTIPort:
      ocp->MData.width = p->dataWidth;
      p->master = false;
      ocp->SReset_n.value = s;
      ocp->SThreadBusy.value = s;
      break;
    default:
      return OU::esprintf("No WIP port type specified for interface \"%s\"", p->name);
    }
    fixOCP(p);
  }
  return 0;
}
