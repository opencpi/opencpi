
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
// File       : gtx_tx_sync_rate_v6.v
//////////////////////////////////////////////////////////////////////////////

`timescale 1ns / 1ps
`define DLY #1

module GTX_TX_SYNC_RATE_V6
#(
    parameter           C_SIMULATION    = 0 // Set to 1 for simulation
)
(
    output              ENPMAPHASEALIGN,
    output              PMASETPHASE,
    output              SYNC_DONE,
    output              OUT_DIV_RESET,
    output              PCS_RESET,
    output              USER_PHYSTATUS,
    output              RATE_CLK_SEL,
    input               USER_CLK,
    input               RESET,
    input               RATE,
    input               RATEDONE,
    input               GT_PHYSTATUS,
    input               RESETDONE,

    // Test bits / status
    output      [53:0]  DEBUG_STATUS,
    input       [2:0]   ENPMA_STATE_MASK,
    input       [2:0]   OUTDIV_STATE_MASK

);

// synthesis attribute X_CORE_INFO of TX_SYNC is "v6_gtxwizard_v1_0, Coregen v11.1_ip1";

//*******************************Register Declarations************************

    reg           begin_r;
    reg           phase_align_r;
    reg           ready_r;
    reg   [15:0]  sync_counter_r;
    reg   [5:0]   count_32_cycles_r;
    reg           wait_stable_r;
    reg           phase_align_reset_r;
    reg           out_div_reset_r;
    reg           wait_phase_align_reset_r;
    reg           pcs_reset_r;
    reg           wait_reset_done_r;
    reg           gen_phystatus_r;
    reg           rate_r;
    reg           guard_r;
    reg           resetdone_r;
    reg           resetdone_r2;
    reg           ratedone_r;
    reg           ratedone_r2;
    reg           rate_sel_r;

    reg           enpmaphasealign_r;
    reg           reg_out_div_reset_r;
    reg   [24:0]  rate_duration_count_r;


//*******************************Wire Declarations****************************
    
    wire          count_setphase_complete_i;
    wire          count_32_complete_i;
    wire          next_phase_align_c;
    wire          next_ready_c;
    wire          next_wait_stable_c;
    wire          next_phase_align_reset_c;
    wire          next_out_div_reset_c;
    wire          next_wait_phase_align_reset_c;
    wire          next_pcs_reset_c;
    wire          next_wait_reset_done_c;
    wire          next_gen_phystatus_c;
    wire          ratedone_pulse_i;
    
    wire          enpmaphasealign_i;
    wire          out_div_reset_i;

//*******************************Main Body of Code****************************




    //________________________________ State machine __________________________
    // This state machine manages the phase alingment procedure of the GTX.
    // The module is held in reset till the usrclk source is stable.  In the 
    // case of buffer bypass where the refclkout is used to clock the usrclks,
    // the usrclk stable indication is given the pll_locked signal.
    // Once the pll_lock is asserted, state machine goes into the
    // wait_stable_r for 32 clock cycles to allow some time to ensure the pll
    // is stable. After this, it goes into the phase_align_r state where the
    // phase alignment procedure is executed. This involves asserting the
    // ENPMAPHASEALIGN and PMASETPHASE for 32768 clock cycles.
    //
    // If there is a line rate change, the module resets the output divider by
    // asserting the signal for 16 clock cycles and resets the phase alignment
    // block by de-asserting ENPMAPHASEALIGN signal for 16 clock cycles.  The
    // phase alignment procedure as stated above is repeated.  Afterwards, the
    // PCS is reset and a user PHYSTATUS is generated to notify the completion
    // of a line rate change procedure.

    // State registers
    always @(posedge USER_CLK)
        if(RESET)
            {begin_r, wait_stable_r, phase_align_r, ready_r,
             phase_align_reset_r, out_div_reset_r, wait_phase_align_reset_r,
             pcs_reset_r, wait_reset_done_r,
             gen_phystatus_r}  <=  `DLY 10'b10_0000_0000;
        else
        begin
            begin_r                   <=  `DLY  1'b0;
            wait_stable_r             <=  `DLY  next_wait_stable_c;
            phase_align_r             <=  `DLY  next_phase_align_c;
            ready_r                   <=  `DLY  next_ready_c;
            phase_align_reset_r       <=  `DLY  next_phase_align_reset_c;
            out_div_reset_r           <=  `DLY  next_out_div_reset_c;
            wait_phase_align_reset_r  <=  `DLY  next_wait_phase_align_reset_c;
            pcs_reset_r               <=  `DLY  next_pcs_reset_c;
            wait_reset_done_r         <=  `DLY  next_wait_reset_done_c;
            gen_phystatus_r           <=  `DLY  next_gen_phystatus_c;
        end

    // Next state logic
    assign next_ready_c              = (((phase_align_r & count_setphase_complete_i) & !guard_r) | gen_phystatus_r) |
                                      (!ratedone_pulse_i & ready_r);

    assign next_phase_align_reset_c = (ratedone_pulse_i & ready_r) | 
                                      (phase_align_reset_r & !count_32_complete_i);

    assign next_out_div_reset_c     = (phase_align_reset_r & count_32_complete_i)|
                                      (out_div_reset_r & !count_32_complete_i);

    assign next_wait_phase_align_reset_c = (out_div_reset_r & count_32_complete_i) | 
                                           (wait_phase_align_reset_r & !count_32_complete_i);

    assign next_wait_stable_c       = begin_r | (wait_phase_align_reset_r & count_32_complete_i) |
                                      (wait_stable_r & !count_32_complete_i);

    assign next_phase_align_c       = (wait_stable_r & count_32_complete_i) |
                                      (phase_align_r & !count_setphase_complete_i);

    assign next_pcs_reset_c         = ((phase_align_r & count_setphase_complete_i) & guard_r);

    assign next_wait_reset_done_c   = pcs_reset_r |
                                      (!resetdone_r2 & wait_reset_done_r);

    assign next_gen_phystatus_c     = resetdone_r2 & wait_reset_done_r;


        //_________ Counter for to wait for pll to be stable before sync __________
    always @(posedge USER_CLK)
    begin
        if (RESET || count_32_complete_i)
            count_32_cycles_r <= `DLY  6'b000000;
        else if (wait_stable_r || out_div_reset_r || phase_align_reset_r || wait_phase_align_reset_r)
            count_32_cycles_r <= `DLY  count_32_cycles_r + 1'b1;
    end

    assign count_32_complete_i = count_32_cycles_r[5];

    //_______________ Counter for holding SYNC for SYNC_CYCLES ________________
    always @(posedge USER_CLK)
    begin
        if (!phase_align_r)
            sync_counter_r <= `DLY  16'h0000;
        else
            sync_counter_r <= `DLY  sync_counter_r + 1'b1;
    end

    generate
        if (C_SIMULATION) begin: for_simulation
            // Shorten the cycle to 32 clock cycles for simulation
            assign count_setphase_complete_i = sync_counter_r[5];
        end
        else begin: for_hardware
            // For TXPMASETPHASE:
            //  - If RATE[0] = 0, PLL_DIVSEL_OUT = 2 => 16,384 USRCLK2 cycles
            //  - If RATE[0] = 1, PLL_DIVSEL_OUT = 1 => 8,192 USRCLK2 cycles
            assign count_setphase_complete_i = (rate_r) ? sync_counter_r[13] :
                                                          sync_counter_r[14];
        end
    endgenerate


    //_______________ Assign the phase align ports into the GTX _______________

    // Assert the ENPMAPHASEALIGN signal when the reset of this module
    // gets de-asserted and after a reset of the output dividers.  Disabling
    // this signal after reset of the output dividers will reset the phase
    // alignment module.
    //assign ENPMAPHASEALIGN = !(begin_r | phase_align_reset_r | out_div_reset_r | wait_phase_align_reset_r);
    
    // Masking the bits of each state to play around with the pulse of the
    // TXENPMAPHASEALIGN reset (active low signal)
    assign enpmaphasealign_i = ~((begin_r ||
                               (phase_align_reset_r && ENPMA_STATE_MASK[2]) ||
                               (out_div_reset_r && ENPMA_STATE_MASK[1]) || 
                               (wait_phase_align_reset_r && ENPMA_STATE_MASK[0])));

    always @(posedge USER_CLK)
        if (RESET)
            enpmaphasealign_r <= `DLY 1'b0;
        else
            enpmaphasealign_r <= enpmaphasealign_i;

    assign ENPMAPHASEALIGN = enpmaphasealign_r; 

    assign PMASETPHASE     = phase_align_r;


    //_______________________ Assign the sync_done port _______________________

    // Assert the SYNC_DONE signal when the phase alignment procedure is
    // complete after initialization and when line rate change procedure
    // is complete.
    assign SYNC_DONE = ready_r & !guard_r;


    //_______________________ Assign the rest of the ports ____________________
    // Assert the output divider reset for 32 USRCLK2 clock cycles
    //assign OUT_DIV_RESET = out_div_reset_r;

    // Masking the bits of each state to play around with the pulse of the
    // output divider reset
    assign out_div_reset_i= (phase_align_reset_r && OUTDIV_STATE_MASK[2]) ||
                           (out_div_reset_r && OUTDIV_STATE_MASK[1]) || 
                           (wait_phase_align_reset_r && OUTDIV_STATE_MASK[0]);

    always @(posedge USER_CLK)
        if (RESET)
            reg_out_div_reset_r <= `DLY 1'b0;
        else
            reg_out_div_reset_r <= out_div_reset_i;

    assign OUT_DIV_RESET = reg_out_div_reset_r;
                         
    // Assert the PCS reset for 1 USRCLK2 clock cycle
    assign PCS_RESET = pcs_reset_r;


    // Assert user phystatus at the end of the line rate change.  It is also
    // a pass through signal from the GTX when the pulse is not associated
    // with a line rate change (in this module this signal is gated by
    // guard_r signal)
    assign USER_PHYSTATUS = gen_phystatus_r | (GT_PHYSTATUS & !guard_r);


    //////////////////////////////////////////////////////////////////////////
    // Register the RESETDONE input
    //////////////////////////////////////////////////////////////////////////
    always @(posedge USER_CLK)

    begin
        if (RESET | pcs_reset_r)
        begin
            resetdone_r   <= `DLY 1'b0;
            resetdone_r2  <= `DLY 1'b0;
        end
        else
        begin
            resetdone_r   <= `DLY RESETDONE;
            resetdone_r2  <= `DLY resetdone_r;
        end
    end

    //////////////////////////////////////////////////////////////////////////
    // Detect an edge on the RATEDONE signal and generate a pulse from it.
    // The RATEDONE signal by default is initialized to 1'b1.
    //////////////////////////////////////////////////////////////////////////
    always @(posedge USER_CLK)

    begin
        if (RESET)
        begin
            ratedone_r  <= `DLY 1'b0;
            ratedone_r2 <= `DLY 1'b0;
        end
        else
        begin
            ratedone_r  <= `DLY RATEDONE;
            ratedone_r2 <= `DLY ratedone_r;
        end
    end

    assign ratedone_pulse_i = ratedone_r & !ratedone_r2;

    //////////////////////////////////////////////////////////////////////////
    // Detect a line rate change.  Since this is targeted for PCIe, we only
    // need to detect a change on TXRATE[0]/RXRATE[0]:
    // TXRATE[1:0] / RXRATE[1:0] = 10 for output divider /2
    // TXRATE[1:0] / RXRATE[1:0] = 11 for output divider /1
    //////////////////////////////////////////////////////////////////////////
    always @(posedge USER_CLK)
    begin
        rate_r <= `DLY RATE;
    end

    assign rate_change_i = rate_r ^ RATE;


    //////////////////////////////////////////////////////////////////////////
    // Generate an internal "guard" signal to denote that the line rate
    // sequence of operation initiated.  This signal is driven High when the
    // there is a rate change trigger by a change in TXRATE or RXRATE ports.
    //////////////////////////////////////////////////////////////////////////
    always @(posedge USER_CLK)
    begin
        if (RESET | gen_phystatus_r)
            guard_r <= `DLY 1'b0;
        else if (rate_change_i == 1'b1)
            guard_r <= `DLY 1'b1;
    end


    //////////////////////////////////////////////////////////////////////////
    // Generate the BUFGMUX select signal that selects the correct clock to
    // used based on a rate change.  For PCIe:
    //  - RATE[0] = 0 => Use 125 MHz USRCLK2 with RATE_CLK_SEL = 0
    //  - RATE[0] = 1 => Use 250 MHz USRCLK2 with RATE_CLK_SEL = 1
    // The RATE_CLK_SEL changes based on the RATEDONE signal from the GTX.
    // The default of this pin is set to 1'b0.  Someone can change it to grab
    // the value from a parameter if the reset value has to be another value
    // other than 1'b0.
    //////////////////////////////////////////////////////////////////////////
    always @(posedge USER_CLK)
    begin
        if (RESET)
            rate_sel_r <= `DLY 1'b0;
        else if (ratedone_pulse_i == 1'b1)
            rate_sel_r <= `DLY rate_r;
    end

    assign RATE_CLK_SEL = rate_sel_r;

    //////////////////////////////////////////////////////////////////////////
    // Create a counter that starts when guard_r is High.  After
    // guard_r gets de-asserted, the counter stops counting.  The counter gets
    // reset when there is a rate change applied from the user; this rate
    // change pulse occurs one USER_CLK cycle earlier than guard_r.  
    //////////////////////////////////////////////////////////////////////////
    always @(posedge USER_CLK)
    begin
        if (RESET | rate_change_i)
            rate_duration_count_r <= `DLY 25'b0_0000_0000_0000_0000_0000_0000;
        else if (guard_r)
            rate_duration_count_r <= `DLY rate_duration_count_r + 1'b1;
        else
            rate_duration_count_r <= `DLY rate_duration_count_r;
    end


    //Monitoring the signals on ILa
    assign DEBUG_STATUS= {sync_counter_r[15:0],       //[53:38]
                          rate_r,                     //[37]
                          rate_duration_count_r[24:0],//[36:12]
                          begin_r,                    //[11]
                          wait_stable_r,              //[10]
                          phase_align_r,              //[9]
                          ready_r,                    //[8]
                          phase_align_reset_r,        //[7]
                          out_div_reset_r,            //[6]
                          wait_phase_align_reset_r,   //[5]
                          pcs_reset_r,                //[4]
                          wait_reset_done_r,          //[3]
                          gen_phystatus_r,            //[2]
                          guard_r,                    //[1]
                          rate_change_i};             //[0]
endmodule
