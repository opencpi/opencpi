
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


// Depth 2 FIFO  Data width 0
module FIFO20(CLK, 
              RST_N, 
              ENQ, 
              FULL_N, 
              DEQ, 
              EMPTY_N, 
              CLR
              );
   parameter guarded = 1;

   input  RST_N;
   input  CLK;
   input  ENQ;
   input  CLR;
   input  DEQ;

   output FULL_N;
   output EMPTY_N;

   reg    empty_reg;
   reg    full_reg;

   assign FULL_N  = full_reg ;
   assign EMPTY_N = empty_reg ;
   
`ifdef BSV_NO_INITIAL_BLOCKS
`else // not BSV_NO_INITIAL_BLOCKS
   // synopsys translate_off
   initial 
     begin
        empty_reg = 1'b0 ;
        full_reg  = 1'b1 ;
     end // initial begin
   // synopsys translate_on  
`endif // BSV_NO_INITIAL_BLOCKS

   always@(posedge CLK /* or negedge RST_N */)
      begin
         if (!RST_N) 
           begin
              empty_reg <= `BSV_ASSIGNMENT_DELAY 1'b0;
              full_reg  <= `BSV_ASSIGNMENT_DELAY 1'b1;
           end // if (RST_N == 0)
         else
           begin
              if (CLR) 
                begin
                   empty_reg <= `BSV_ASSIGNMENT_DELAY 1'b0;
                   full_reg  <= `BSV_ASSIGNMENT_DELAY 1'b1;
                end 
              else if (ENQ && !DEQ) 
                begin
                   empty_reg <= `BSV_ASSIGNMENT_DELAY 1'b1;
                   full_reg  <= `BSV_ASSIGNMENT_DELAY ! empty_reg;
                end // if (ENQ && !DEQ)              
              else if (!ENQ && DEQ) 
                begin
                   full_reg  <= `BSV_ASSIGNMENT_DELAY 1'b1;
                   empty_reg <= `BSV_ASSIGNMENT_DELAY ! full_reg;
                end // if (!ENQ && DEQ)
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
                  $display( "Warning: FIFO20: %m -- Dequeuing from empty fifo" ) ;
               end
             if ( ! full_reg && ENQ && (!DEQ || guarded) )
               begin
                  enqerror =  1 ;             
                  $display( "Warning: FIFO20: %m -- Enqueuing to a full fifo" ) ;
               end
          end // if ( RST_N )        
     end
   // synopsys translate_on
   
endmodule
