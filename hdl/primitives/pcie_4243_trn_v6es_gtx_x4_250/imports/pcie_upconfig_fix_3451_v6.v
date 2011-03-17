//-----------------------------------------------------------------------------
//
// (c) Copyright 2009 Xilinx, Inc. All rights reserved.
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
// Project    : Virtex-6 Integrated Block for PCI Express
// File       : pcie_upconfig_fix_3451_v6.v
//--
//-- Description: Virtex6 Workaround for Root Port Upconfigurability Bug
//--
//--
//--------------------------------------------------------------------------------

`timescale 1ns/1ns

module pcie_upconfig_fix_3451_v6 # (

  parameter                                     UPSTREAM_FACING = "TRUE",
  parameter                                     PL_FAST_TRAIN = "FALSE",
  parameter                                     LINK_CAP_MAX_LINK_WIDTH = 6'h08

)
(

  input                                         pipe_clk,
  input                                         pl_phy_lnkup_n,

  input  [5:0]                                  pl_ltssm_state,
  input                                         pl_sel_lnk_rate,
  input  [1:0]                                  pl_directed_link_change, 
 
  input  [3:0]                                  cfg_link_status_negotiated_width,

  output                                        filter_pipe

);

  parameter TCQ = 1;

  reg                                           reg_filter_pipe;

  reg  [5:0]                                    reg_prev_pl_ltssm_state;
  wire [5:0]                                    prev_pl_ltssm_state;

  reg  [15:0]                                   reg_tsx_counter;
  wire [15:0]                                   tsx_counter;

  wire [5:0]                                    cap_link_width;

  // Corrupting all Tsx on all lanes as soon as we do R.RC->R.RI transition to allow time for
  // the core to see the TS1s on all the lanes being configured at the same time
  // R.RI has a 2ms timeout.Corrupting tsxs for ~1/4 of that time
  // 225 pipe_clk cycles-sim_fast_train
  // 60000 pipe_clk cycles-without sim_fast_train
  // Not taking any action  when PLDIRECTEDLINKCHANGE is set

  always @ (posedge pipe_clk) begin

    if (pl_phy_lnkup_n) begin

      reg_tsx_counter <= #TCQ 16'h0;
      reg_filter_pipe <= #TCQ 1'b0;

    end else if ((pl_ltssm_state == 6'h20) && 
                 (prev_pl_ltssm_state == 6'h1d) && 
                 (cfg_link_status_negotiated_width != cap_link_width) && 
                 (pl_directed_link_change[1:0] == 2'b00)) begin

      reg_tsx_counter <= #TCQ 16'h0;
      reg_filter_pipe <= #TCQ 1'b1;

    end else if (filter_pipe == 1'b1) begin

      if (tsx_counter < ((PL_FAST_TRAIN == "TRUE") ? 16'd225: pl_sel_lnk_rate ? 16'd30000 : 16'd60000)) begin

        reg_tsx_counter <= #TCQ tsx_counter + 1'b1;
        reg_filter_pipe <= #TCQ 1'b1;

      end else begin 

        reg_tsx_counter <= #TCQ 16'h0;
        reg_filter_pipe <= #TCQ 1'b0;

      end

    end

  end

  assign filter_pipe = (UPSTREAM_FACING == "TRUE") ? 1'b0 : reg_filter_pipe;
  assign tsx_counter = reg_tsx_counter;

  always @(posedge pipe_clk) begin

    if (pl_phy_lnkup_n)
      reg_prev_pl_ltssm_state <= #TCQ 6'h0;
    else
      reg_prev_pl_ltssm_state <= #TCQ pl_ltssm_state;

  end
  assign prev_pl_ltssm_state = reg_prev_pl_ltssm_state;

  assign cap_link_width = LINK_CAP_MAX_LINK_WIDTH;

endmodule
