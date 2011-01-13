
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

// A pulse based clock domain synchronization scheme.
// When a sEN is asserted, a pulse is eventually sent to dPulse in the
// destination clock domain.
// Close and Multiple asserts of sEN may not be seen at the destination side.
// Reset signal is not needed since it a pulse-based, rather than
// level-based protocol
// Delay is 2 dCLK cycle.  
// dPulse is not registered.
module SyncPulse(
                  sCLK,
                  sRST_N,
                  dCLK,
                  sEN,
                  dPulse
                  );

   // source clock ports
   input     sCLK ;
   input     sRST_N ;
   input     sEN ;
   
   // destination clock ports
   input     dCLK ;
   output    dPulse ;
   
   // Flops to hold data
   reg       sSyncReg;
   reg       dSyncReg1, dSyncReg2;
   reg       dSyncPulse;
   
   assign    dPulse = dSyncReg2 != dSyncPulse ;
   
   always @(posedge sCLK or negedge sRST_N)
     begin
        if (sRST_N == 0)
          sSyncReg <= `BSV_ASSIGNMENT_DELAY 1'b0 ;
        else
          begin
             if ( sEN )
               begin
                  sSyncReg <= `BSV_ASSIGNMENT_DELAY ! sSyncReg ;
               end
          end // else: !if(sRST_N == 0)
     end // always @ (posedge sCLK or negedge sRST_N)


   always @(posedge dCLK or negedge sRST_N )
      begin
         if (sRST_N == 0)
            begin
               dSyncReg1 <= `BSV_ASSIGNMENT_DELAY 1'b0 ;               
               dSyncReg2 <= `BSV_ASSIGNMENT_DELAY 1'b0 ;
               dSyncPulse <= `BSV_ASSIGNMENT_DELAY 1'b0 ;
            end // if (sRST_N == 0)
         else
           begin
              dSyncReg1 <= `BSV_ASSIGNMENT_DELAY sSyncReg ;// domain crossing
              dSyncReg2 <= `BSV_ASSIGNMENT_DELAY dSyncReg1 ;
              dSyncPulse <= `BSV_ASSIGNMENT_DELAY dSyncReg2 ;
           end // else: !if(sRST_N == 0)
      end // always @ (posedge dCLK or negedge sRST_N )
   
`ifdef BSV_NO_INITIAL_BLOCKS
`else // not BSV_NO_INITIAL_BLOCKS
   // synopsys translate_off
   initial
      begin
         sSyncReg   = 1'b0 ;
         dSyncReg1  = 1'b0 ;
         dSyncReg2  = 1'b0 ;
         dSyncPulse = 1'b0 ;
      end // initial begin
   // synopsys translate_on
`endif // BSV_NO_INITIAL_BLOCKS

endmodule // PulseSync

