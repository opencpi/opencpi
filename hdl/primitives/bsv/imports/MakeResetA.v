
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
// $Revision: 20862 $
// $Date: 2010-06-07 18:20:35 +0000 (Mon, 07 Jun 2010) $

`ifdef BSV_ASSIGNMENT_DELAY
`else
`define BSV_ASSIGNMENT_DELAY
`endif


module MakeResetA (
		  CLK,
		  RST_N,
                  ASSERT_IN,
		  ASSERT_OUT,

                  DST_CLK,
                  OUT_RST_N
                  );

   parameter          RSTDELAY = 2  ; // Width of reset shift reg
   parameter          init = 1 ;

   input              CLK ;
   input              RST_N ;
   input              ASSERT_IN ;
   output             ASSERT_OUT ;

   input              DST_CLK ;
   output             OUT_RST_N ;

   reg                rst ;
   wire               OUT_RST_N ;

   assign ASSERT_OUT = !rst ;

   SyncResetA #(RSTDELAY) rstSync (.CLK(DST_CLK),
				   .IN_RST_N(rst),
				   .OUT_RST_N(OUT_RST_N));

   always@(posedge CLK or negedge RST_N) begin
      if (RST_N == 0)
        rst <= `BSV_ASSIGNMENT_DELAY init;
      else
        begin
           if (ASSERT_IN)
             rst <= `BSV_ASSIGNMENT_DELAY 1'b0;
           else // if (rst == 1'b0)
             rst <= `BSV_ASSIGNMENT_DELAY 1'b1;
        end // else: !if(RST_N == 0)
   end // always@ (posedge CLK or negedge RST_N)

`ifdef BSV_NO_INITIAL_BLOCKS
`else // not BSV_NO_INITIAL_BLOCKS
   // synopsys translate_off
   initial begin
      #0 ;
      rst = 1'b1 ;
   end
   // synopsys translate_on
`endif // BSV_NO_INITIAL_BLOCKS

endmodule // MakeResetA
