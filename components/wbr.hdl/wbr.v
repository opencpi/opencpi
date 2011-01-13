
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

`default_nettype none
`ifdef OLDER
// autocreated by expandPorts.tcl 2.0 and portUtil.tcl 2.0
module wbr (
    dif_Clk,
    dif_m2s_MReset_n,
    control_Clk,
    control_m2s_MReset_n,
    ctl_MCmd,
    ctl_MAddrSpace,
    ctl_MByteEn,
    ctl_MAddr,
    ctl_Mdata,
    ctl_Sresp,
    ctl_Sdata,
    ctl_SThreadBusy,
    ctl_SFlag,
    ctl_MFlag,
    dif_Mcmd,
    dif_MreqLast,
    dif_MburstPrecise,
    dif_MburstLength,
    dif_Mdata,
    dif_MreqInfo,
    dif_SThreadBusy,
    chan_Mcmd,
    chan_MreqLast,
    chan_MburstPrecise,
    chan_MburstLength,
    chan_Mdata,
    chan_MreqInfo,
    chan_SThreadBusy );

  input dif_Clk;
  input dif_m2s_MReset_n;
  input control_Clk;
  input control_m2s_MReset_n;

  // ====================
  // Method = ctl_putreq
  //   input  => ctl_req             53   OCWip::WciReq#(13)
  input  [ 2 : 0 ] ctl_Mcmd;
  input  ctl_MaddrSpace;
  input  [ 3 : 0 ] ctl_MbyteEn;
  input  [ 12 : 0 ] ctl_Maddr;
  input  [ 31 : 0 ] ctl_Mdata;

  // ====================
  // Method = ctl_resp
  //   result => ctl_resp            34   OCWip::WciResp
  output  [ 1 : 0 ] ctl_Sresp;  // ctl_resp[33:32]
  output  [ 31 : 0 ] ctl_Sdata;  // ctl_resp[31:0]

  // ====================
  // Method = ctl_sThreadBusy
  //   result => ctl_SThreadBusy      1   Bool
  output  ctl_SThreadBusy;

  // ====================
  // Method = ctl_sFlag
  //   result => ctl_SFlag            2   Bit#(2)
  output  [ 1 : 0 ] ctl_SFlag;

  // ====================
  // Method = ctl_mFlag
  //   input  => ctl_MFlag            2   Bit#(2)
  input  [ 1 : 0 ] ctl_MFlag;

  // ====================
  // Method = dif_put
  //   input  => dif_req               36   {OCWip::WsiReq#(12, 18, 0, 1, 0)}
  input  [ 2 : 0 ] dif_Mcmd;
  input  dif_MreqLast;
  input  dif_MburstPrecise;
  input  [ 11 : 0 ] dif_MburstLength;
  input  [ 17 : 0 ] dif_Mdata;
  input  dif_MreqInfo;

  // ====================
  // Method = dif_sThreadBusy
  //   result => dif_SThreadBusy        1   Bool
  output  dif_SThreadBusy;

  // ====================
  // Method = chan_get
  //   result => chan_req              50   {OCWip::WsiReq#(12, 32, 0, 1, 0)}
  output  [ 2 : 0 ] chan_Mcmd;  // chan_req[49:47]
  output  chan_MreqLast;  // chan_req[46:46]
  output  chan_MburstPrecise;  // chan_req[45:45]
  output  [ 11 : 0 ] chan_MburstLength;  // chan_req[44:33]
  output  [ 31 : 0 ] chan_Mdata;  // chan_req[32:1]
  output  chan_MreqInfo;  // chan_req[0:0]

  // ====================
  // Method = chan_sThreadBusy
  //   enable => chan_SThreadBusy       1   Bit#(1)
  input  chan_SThreadBusy;


  wire   [ 1 : 0 ] ctl_Sresp;  // ctl_resp[33:32]
  wire   [ 31 : 0 ] ctl_Sdata;  // ctl_resp[31:0]
  wire   ctl_SThreadBusy;
  wire   [ 1 : 0 ] ctl_SFlag;
  wire   dif_SThreadBusy;
  wire   [ 2 : 0 ] chan_Mcmd;  // chan_req[49:47]
  wire   chan_MreqLast;  // chan_req[46:46]
  wire   chan_MburstPrecise;  // chan_req[45:45]
  wire   [ 11 : 0 ] chan_MburstLength;  // chan_req[44:33]
  wire   [ 31 : 0 ] chan_Mdata;  // chan_req[32:1]
  wire   chan_MreqInfo;  // chan_req[0:0]
`else
`define NOT_EMPTY_wbr
`include "wbr_defs.vh"
`endif
  mkRcvrWorker _mkRcvrWorker ( 
   .dif_Clk( dif_Clk ),
   .dif_m2s_MReset_n( dif_MReset_n ),
   .control_Clk( ctl_Clk ),
   .control_m2s_MReset_n( ctl_MReset_n ),
   .wci_s_req( { ctl_MCmd,ctl_MAddrSpace,ctl_MByteEn,ctl_MAddr,ctl_MData } ),
   .wci_s_resp( { ctl_SResp,ctl_SData } ),
   .wci_s_SThreadBusy( ctl_SThreadBusy ),
   .wci_s_SFlag( ctl_SFlag ),
   .wci_s_MFlag( ctl_MFlag ),
    // Note we feed the lower 16 bit value from the input as the high order 16 bits of the
    // underlying 18 bit input path (until we fix the input to cycle through the 16 bit values in
    // each input word
   .dif_req( { dif_MCmd,dif_MReqLast,dif_MBurstPrecise,dif_MBurstLength,dif_MData[15:0],2'b0,dif_MReqInfo } ),
   .dif_SThreadBusy( dif_SThreadBusy ),
   .chan_req( { chan_MCmd,chan_MReqLast,chan_MBurstPrecise,chan_MBurstLength,chan_MData,chan_MReqInfo } ),
   .chan_SThreadBusy( chan_SThreadBusy )
  );

endmodule

