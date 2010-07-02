
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

// 
// Transfer takes 2 dCLK to see data,
// sRDY recovers takes 2 dCLK + 2 sCLK
module SyncHandshake(
                     sCLK,
                     sRST_N,
                     dCLK,
                     sEN,
                     sRDY,
                     dPulse
                     );
   parameter init = 1'b0;
   
   // Source clock port signal
   input     sCLK ;
   input     sRST_N ;   
   input     sEN ;
   output    sRDY ;
   
   // Destination clock port signal
   input     dCLK ;
   output    dPulse ;
   
   // Flops to hold data
   reg       dToggleReg ;
   reg       dSyncReg1, dSyncReg2 ;
   reg       dLastState ;
   reg       sToggleReg ;
   reg       sSyncReg1, sSyncReg2 ;
   reg       sLastState ;
   reg       sRDY_reg ;

   wire      pulseSignal ;
    
   // Output signal
   assign    dPulse = pulseSignal ;
   assign    pulseSignal = dSyncReg2 != dLastState ;
   assign    sRDY = sRDY_reg ;
   
   always @(posedge sCLK or negedge sRST_N)
     begin
        if (sRST_N == 0)
           begin
              sRDY_reg   <= `BSV_ASSIGNMENT_DELAY 1'b1 ;
              sSyncReg1  <= `BSV_ASSIGNMENT_DELAY init ;
              sSyncReg2  <= `BSV_ASSIGNMENT_DELAY init ;
              sLastState <= `BSV_ASSIGNMENT_DELAY init ;
              sToggleReg <= `BSV_ASSIGNMENT_DELAY init ;
           end
        else
           begin

              // hadshake return synchronizer
              sSyncReg1 <= `BSV_ASSIGNMENT_DELAY dToggleReg ;// clock domain crossing
              sSyncReg2 <= `BSV_ASSIGNMENT_DELAY sSyncReg1 ;
              sLastState <= `BSV_ASSIGNMENT_DELAY sSyncReg2 ;

              // Pulse send
              if ( sEN )
                begin
                   sToggleReg <= `BSV_ASSIGNMENT_DELAY ! sToggleReg ;
                end // if ( sEN )

              // Ready signal generation
              if ( sEN )
                begin
                   sRDY_reg <= `BSV_ASSIGNMENT_DELAY 1'b0 ;
                end
              else if ( sSyncReg2 != sLastState )
                begin
                   sRDY_reg <= `BSV_ASSIGNMENT_DELAY 1'b1 ;
                end
           end
     end // always @ (posedge sCLK or negedge sRST_N)

   always @(posedge dCLK or negedge sRST_N)
     begin
        if (sRST_N == 0)
          begin
             dSyncReg1  <= `BSV_ASSIGNMENT_DELAY init;
             dSyncReg2  <= `BSV_ASSIGNMENT_DELAY init;
             dLastState <= `BSV_ASSIGNMENT_DELAY init ;
             dToggleReg <= `BSV_ASSIGNMENT_DELAY init ;
          end
        else
           begin
              dSyncReg1 <= `BSV_ASSIGNMENT_DELAY sToggleReg ;// domain crossing
              dSyncReg2 <= `BSV_ASSIGNMENT_DELAY dSyncReg1 ;
              dLastState <= `BSV_ASSIGNMENT_DELAY dSyncReg2 ;

              if ( pulseSignal )
                begin
                   dToggleReg <= `BSV_ASSIGNMENT_DELAY ! dToggleReg ;
                end
           end
     end // always @ (posedge dCLK or negedge sRST_N)  

`ifdef BSV_NO_INITIAL_BLOCKS
`else // not BSV_NO_INITIAL_BLOCKS
   // synopsys translate_off  
   initial
      begin
         dSyncReg1 = init ;
         dSyncReg2 = init ;
         dLastState = init ;
         dToggleReg = init ;
         
         sToggleReg = init ;
         sSyncReg1 = init ;
         sSyncReg2 = init ;
         sLastState = init ;

         sRDY_reg = 1'b1 ;
      end // initial begin
   // synopsys translate_on
`endif // BSV_NO_INITIAL_BLOCKS
   
endmodule // HandshakeSync
