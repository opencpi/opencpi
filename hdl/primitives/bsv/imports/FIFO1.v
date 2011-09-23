
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
// $Revision: 24080 $
// $Date: 2011-05-18 19:32:52 +0000 (Wed, 18 May 2011) $

`ifdef BSV_ASSIGNMENT_DELAY
`else
`define BSV_ASSIGNMENT_DELAY
`endif



// Depth 1 FIFO
module FIFO1(CLK, 
             RST_N, 
             D_IN, 
             ENQ, 
             FULL_N, 
             D_OUT, 
             DEQ, 
             EMPTY_N, 
             CLR
             );

   parameter width = 1;
   parameter guarded = 1;
   input                  CLK;
   input                  RST_N;
   input [width - 1 : 0]  D_IN;
   input                  ENQ;
   input                  DEQ;
   input                  CLR ;

   output                 FULL_N;
   output [width - 1 : 0] D_OUT;
   output                 EMPTY_N;

   reg [width - 1 : 0]    D_OUT;
   reg                    empty_reg ;


   assign                 EMPTY_N = empty_reg ;
   
   
`ifdef BSV_NO_INITIAL_BLOCKS
`else // not BSV_NO_INITIAL_BLOCKS
   // synopsys translate_off
   initial 
     begin
        D_OUT   = {((width + 1)/2) {2'b10}} ;
        empty_reg = 1'b0 ;
     end // initial begin
   // synopsys translate_on
`endif // BSV_NO_INITIAL_BLOCKS

   
   assign FULL_N = !empty_reg;

   always@(posedge CLK /* or negedge RST_N */ )
     begin
        if (!RST_N)
          begin
             empty_reg <= `BSV_ASSIGNMENT_DELAY 1'b0;
          end // if (RST_N == 0)
        else
           begin
              if (CLR)
                begin
                   empty_reg <= `BSV_ASSIGNMENT_DELAY 1'b0;
                end // if (CLR)        
              else if (ENQ)
                begin
                   empty_reg <= `BSV_ASSIGNMENT_DELAY 1'b1;
                end // if (ENQ)
              else if (DEQ)
                begin
                   empty_reg <= `BSV_ASSIGNMENT_DELAY 1'b0;
                end // if (DEQ)
           end // else: !if(RST_N == 0)
     end // always@ (posedge CLK or negedge RST_N)

   always@(posedge CLK /* or negedge RST_N */)
     begin
        // Following section initializes the data registers which
        // may be desired only in some situations.
        // Uncomment to initialize array        
        /*
        if (!RST_N)
          begin
             D_OUT <= `BSV_ASSIGNMENT_DELAY {width {1'b0}} ;
          end
        else
         */
           begin
              if (ENQ)
                D_OUT     <= `BSV_ASSIGNMENT_DELAY D_IN;
           end // else: !if(RST_N == 0)
     end // always@ (posedge CLK or negedge RST_N)

   // synopsys translate_off
   always@(posedge CLK)
     begin: error_checks
        reg deqerror, enqerror ;
        
        deqerror =  0;
        enqerror = 0;
        if ( RST_N )
           begin
              if ( ! empty_reg && DEQ )
                begin
                   deqerror = 1 ;             
                   $display( "Warning: FIFO1: %m -- Dequeuing from empty fifo" ) ;
                end
              if ( ! FULL_N && ENQ && (!DEQ || guarded) )
                begin
                   enqerror =  1 ;             
                   $display( "Warning: FIFO1: %m -- Enqueuing to a full fifo" ) ;
                end
           end // if ( RST_N )        
     end
   // synopsys translate_on

endmodule

