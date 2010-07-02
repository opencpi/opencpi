
// Copyright (c) 2000-2009 Bluespec, Inc.

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// $Revision: 17872 $
// $Date: 2009-09-18 14:32:56 +0000 (Fri, 18 Sep 2009) $

`ifdef BSV_ASSIGNMENT_DELAY
`else
 `define BSV_ASSIGNMENT_DELAY
`endif

// Dual-Ported BRAM
module BRAM2(CLKA,
             ENA,
             WEA,
             ADDRA,
             DIA,
             DOA,
             CLKB,
             ENB,
             WEB,
             ADDRB,
             DIB,
             DOB
             );

   parameter                      PIPELINED  = 0;
   parameter                      ADDR_WIDTH = 1;
   parameter                      DATA_WIDTH = 1;
   parameter                      MEMSIZE    = 1;

   input                          CLKA;
   input                          ENA;
   input                          WEA;
   input [ADDR_WIDTH-1:0]         ADDRA;
   input [DATA_WIDTH-1:0]         DIA;
   output [DATA_WIDTH-1:0]        DOA;

   input                          CLKB;
   input                          ENB;
   input                          WEB;
   input [ADDR_WIDTH-1:0]         ADDRB;
   input [DATA_WIDTH-1:0]         DIB;
   output [DATA_WIDTH-1:0]        DOB;

   reg [DATA_WIDTH-1:0]           RAM[0:MEMSIZE-1] /* synthesis syn_ramstyle="no_rw_check" */ ;
   reg [ADDR_WIDTH-1:0]           ADDRA_R;
   reg [ADDR_WIDTH-1:0]           ADDRB_R;
   reg [DATA_WIDTH-1:0]           DOA_R;
   reg [DATA_WIDTH-1:0]           DOB_R;

   wire [DATA_WIDTH-1:0] 	  DOA_noreg;
   wire [DATA_WIDTH-1:0] 	  DOB_noreg;

   wire [ADDR_WIDTH-1:0] 	  ADDRA_muxed;
   wire [ADDR_WIDTH-1:0] 	  ADDRB_muxed;


`ifdef BSV_NO_INITIAL_BLOCKS
`else
   // synopsys translate_off
   integer                        i;
   initial
   begin : init_block
      for (i = 0; i < MEMSIZE; i = i + 1) begin
         RAM[i] = { ((DATA_WIDTH+1)/2) { 2'b10 } };
      end
      ADDRA_R = { ((ADDR_WIDTH+1)/2) { 2'b10 } };
      ADDRB_R = { ((ADDR_WIDTH+1)/2) { 2'b10 } };
      DOA_R = { ((DATA_WIDTH+1)/2) { 2'b10 } };
      DOB_R = { ((DATA_WIDTH+1)/2) { 2'b10 } };
   end
   // synopsys translate_on
`endif // !`ifdef BSV_NO_INITIAL_BLOCKS


   always @(posedge CLKA) begin
      ADDRA_R <= `BSV_ASSIGNMENT_DELAY ADDRA_muxed;
      if (ENA) begin
         if (WEA)
           RAM[ADDRA_muxed] <= `BSV_ASSIGNMENT_DELAY DIA;
      end
   end

   always @(posedge CLKB) begin
      ADDRB_R <= `BSV_ASSIGNMENT_DELAY ADDRB_muxed;
      if (ENB) begin
         if (WEB)
           RAM[ADDRB_muxed] <= `BSV_ASSIGNMENT_DELAY DIB;
      end
   end


   // ENA workaround for Synplify
   assign ADDRA_muxed = (ENA) ? ADDRA : ADDRA_R;
   assign ADDRB_muxed = (ENB) ? ADDRB : ADDRB_R;

   // Memory read
   assign DOA_noreg = RAM[ADDRA_R];
   assign DOB_noreg = RAM[ADDRB_R];

   // Pipeline
   always @(posedge CLKA)
      DOA_R <= DOA_noreg;

   always @(posedge CLKB)
      DOB_R <= DOB_noreg;

   // Output drivers
   assign DOA = (PIPELINED) ? DOA_R : DOA_noreg;
   assign DOB = (PIPELINED) ? DOB_R : DOB_noreg;

endmodule // BRAM2
