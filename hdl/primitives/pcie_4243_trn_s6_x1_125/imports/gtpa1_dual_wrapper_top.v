//-----------------------------------------------------------------------------
//
// (c) Copyright 2008, 2009 Xilinx, Inc. All rights reserved.
//
// This file contains confidential and proprietary information of Xilinx, Inc.
// and is protected under U.S. and international copyright and other
// intellectual property laws.
//
// DISCLAIMER
//
// This disclaimer is not a license and does not grant any rights to the
// materials distributed herewith. Except as otherwise provided in a valid
// license issued to you by Xilinx, and to the maximum extent permitted by
// applicable law: (1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND WITH ALL
// FAULTS, AND XILINX HEREBY DISCLAIMS ALL WARRANTIES AND CONDITIONS, EXPRESS,
// IMPLIED, OR STATUTORY, INCLUDING BUT NOT LIMITED TO WARRANTIES OF
// MERCHANTABILITY, NON-INFRINGEMENT, OR FITNESS FOR ANY PARTICULAR PURPOSE;
// and (2) Xilinx shall not be liable (whether in contract or tort, including
// negligence, or under any other theory of liability) for any loss or damage
// of any kind or nature related to, arising under or in connection with these
// materials, including for any direct, or any indirect, special, incidental,
// or consequential loss or damage (including loss of data, profits, goodwill,
// or any type of loss or damage suffered as a result of any action brought by
// a third party) even if such damage or loss was reasonably foreseeable or
// Xilinx had been advised of the possibility of the same.
//
// CRITICAL APPLICATIONS
//
// Xilinx products are not designed or intended to be fail-safe, or for use in
// any application requiring fail-safe performance, such as life-support or
// safety devices or systems, Class III medical devices, nuclear facilities,
// applications related to the deployment of airbags, or any other
// applications that could lead to death, personal injury, or severe property
// or environmental damage (individually and collectively, "Critical
// Applications"). Customer assumes the sole risk and liability of any use of
// Xilinx products in Critical Applications, subject only to applicable laws
// and regulations governing limitations on product liability.
//
// THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS PART OF THIS FILE
// AT ALL TIMES.
//
//-----------------------------------------------------------------------------
// Project    : Spartan-6 Integrated Block for PCI Express
// File       : gtpa1_dual_wrapper_top.v
// Description: PCI Express Wrapper for GTPA1_DUAL
//
//-----------------------------------------------------------------------------

`timescale  1 ns / 1 ps

module gtpa1_dual_wrapper_top #(
  parameter SIMULATION   = 0
)
(
  // Clock and reset
  input  wire        sys_rst_n,
  input  wire        sys_clk,
  input  wire        gt_usrclk,
  input  wire        gt_usrclk2x,
  output wire        gt_refclk_out,
  output wire        gt_reset_done,
  input  wire        rxreset,

  // RX and TX path GTP <-> PCIe
  output wire [1:0]  rx_char_is_k,
  output wire [15:0] rx_data,
  output wire        rx_enter_elecidle,
  output wire [2:0]  rx_status,
  input  wire        rx_polarity,
  input  wire [1:0]  tx_char_disp_mode,
  input  wire [1:0]  tx_char_is_k,
  input  wire        tx_rcvr_det,
  input  wire [15:0] tx_data,

  // Status and control path GTP <-> PCIe
  output wire        phystatus,
  output wire        gt_rx_valid,
  output wire        gt_plllkdet_out,
  input  wire        gt_tx_elec_idle,
  input  wire [1:0]  gt_power_down,

  // PCIe serial datapath
  output wire        arp_txp,
  output wire        arp_txn,
  input  wire        arp_rxp,
  input  wire        arp_rxn
);

  wire [1:0] gt_refclk;
  assign gt_refclk_out = gt_refclk[0];

  GTPA1_DUAL_WRAPPER #
  (
    // Simulation attributes
    .WRAPPER_SIM_GTPRESET_SPEEDUP (1),
    .WRAPPER_SIMULATION           (SIMULATION)
  ) GT_i
  (

    //---------------------- Loopback and Powerdown Ports ----------------------
    .TILE0_RXPOWERDOWN0_IN (gt_power_down),
    .TILE0_RXPOWERDOWN1_IN (2'b10),
    .TILE0_TXPOWERDOWN0_IN (gt_power_down),
    .TILE0_TXPOWERDOWN1_IN (2'b10),
    //------------------------------- PLL Ports --------------------------------
    .TILE0_CLK00_IN       (sys_clk),
    .TILE0_CLK01_IN       (1'b0),
    .TILE0_GTPRESET0_IN   (!sys_rst_n),
    .TILE0_GTPRESET1_IN   (1'b1),
    .TILE0_PLLLKDET0_OUT  (gt_plllkdet_out),
    .TILE0_PLLLKDET1_OUT  (),
    .TILE0_RESETDONE0_OUT (gt_reset_done),
    .TILE0_RESETDONE1_OUT (),
    //--------------------- Receive Ports - 8b10b Decoder ----------------------
    .TILE0_RXCHARISK0_OUT    ({rx_char_is_k[0], rx_char_is_k[1]}),
    .TILE0_RXCHARISK1_OUT    (),
    .TILE0_RXDISPERR0_OUT    (),
    .TILE0_RXDISPERR1_OUT    (),
    .TILE0_RXNOTINTABLE0_OUT (),
    .TILE0_RXNOTINTABLE1_OUT (),
    //-------------------- Receive Ports - Clock Correction --------------------
    .TILE0_RXCLKCORCNT0_OUT (),
    .TILE0_RXCLKCORCNT1_OUT (),
    //------------- Receive Ports - Comma Detection and Alignment --------------
    .TILE0_RXENMCOMMAALIGN0_IN (1'b1),
    .TILE0_RXENMCOMMAALIGN1_IN (1'b1),
    .TILE0_RXENPCOMMAALIGN0_IN (1'b1),
    .TILE0_RXENPCOMMAALIGN1_IN (1'b1),
    //----------------- Receive Ports - RX Data Path interface -----------------
    .TILE0_RXDATA0_OUT   ({rx_data[7:0], rx_data[15:8]}),
    .TILE0_RXDATA1_OUT   (),
    .TILE0_RXRESET0_IN   (rxreset),
    .TILE0_RXRESET1_IN   (1'b1),
    .TILE0_RXUSRCLK0_IN  (gt_usrclk2x),
    .TILE0_RXUSRCLK1_IN  (1'b0),
    .TILE0_RXUSRCLK20_IN (gt_usrclk),
    .TILE0_RXUSRCLK21_IN (1'b0),
    //----- Receive Ports - RX Driver,OOB signalling,Coupling and Eq.,CDR ------
    .TILE0_GATERXELECIDLE0_IN (1'b0),
    .TILE0_GATERXELECIDLE1_IN (1'b0),
    .TILE0_IGNORESIGDET0_IN   (1'b0),
    .TILE0_IGNORESIGDET1_IN   (1'b0),
    .TILE0_RXELECIDLE0_OUT    (rx_enter_elecidle),
    .TILE0_RXELECIDLE1_OUT    (),
    .TILE0_RXN0_IN            (arp_rxn),
    .TILE0_RXN1_IN            (1'b0),
    .TILE0_RXP0_IN            (arp_rxp),
    .TILE0_RXP1_IN            (1'b0),
    //--------- Receive Ports - RX Elastic Buffer and Phase Alignment ----------
    .TILE0_RXSTATUS0_OUT (rx_status),
    .TILE0_RXSTATUS1_OUT (),
    //------------ Receive Ports - RX Pipe Control for PCI Express -------------
    .TILE0_PHYSTATUS0_OUT (phystatus),
    .TILE0_PHYSTATUS1_OUT (),
    .TILE0_RXVALID0_OUT   (gt_rx_valid),
    .TILE0_RXVALID1_OUT   (),
    //------------------ Receive Ports - RX Polarity Control -------------------
    .TILE0_RXPOLARITY0_IN (rx_polarity),
    .TILE0_RXPOLARITY1_IN (1'b0),
    //-------------------------- TX/RX Datapath Ports --------------------------
    .TILE0_GTPCLKOUT0_OUT (gt_refclk),
    .TILE0_GTPCLKOUT1_OUT (),
    //----------------- Transmit Ports - 8b10b Encoder Control -----------------
    .TILE0_TXCHARDISPMODE0_IN ({tx_char_disp_mode[0], tx_char_disp_mode[1]}),
    .TILE0_TXCHARDISPMODE1_IN (2'b00),
    .TILE0_TXCHARISK0_IN      ({tx_char_is_k[0], tx_char_is_k[1]}),
    .TILE0_TXCHARISK1_IN      (2'b00),
    //---------------- Transmit Ports - TX Data Path interface -----------------
    .TILE0_TXDATA0_IN    ({tx_data[7:0], tx_data[15:8]}),
    .TILE0_TXDATA1_IN    (16'd0),
    .TILE0_TXUSRCLK0_IN  (gt_usrclk2x),
    .TILE0_TXUSRCLK1_IN  (1'b0),
    .TILE0_TXUSRCLK20_IN (gt_usrclk),
    .TILE0_TXUSRCLK21_IN (1'b0),
    //------------- Transmit Ports - TX Driver and OOB signalling --------------
    .TILE0_TXN0_OUT (arp_txn),
    .TILE0_TXN1_OUT (),
    .TILE0_TXP0_OUT (arp_txp),
    .TILE0_TXP1_OUT (),
    //--------------- Transmit Ports - TX Ports for PCI Express ----------------
    .TILE0_TXDETECTRX0_IN (tx_rcvr_det),
    .TILE0_TXDETECTRX1_IN (1'b0),
    .TILE0_TXELECIDLE0_IN (gt_tx_elec_idle),
    .TILE0_TXELECIDLE1_IN (1'b0)
  );

endmodule

