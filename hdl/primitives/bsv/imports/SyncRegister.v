
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

// A register synchronization module across clock domains.
// Uses a Handshake Pulse protocol to trigger the load on
// destination side registers
// Transfer takes 3 dCLK for destination side to see data,
// sRDY recovers takes 3 dCLK + 3 sCLK
module SyncRegister(
                    sCLK,
                    sRST_N,
                    dCLK,
                    sEN,
                    sRDY,
                    sD_IN,
                    dD_OUT
                    );
   parameter             width = 1 ;
   parameter             init = { width {1'b0 }} ;
   
   // Source clock domain ports
   input                 sCLK ;
   input                 sRST_N ;
   input                 sEN ;
   input [width -1 : 0]  sD_IN ;
   output                sRDY ;
      
   // Destination clock domain ports
   input                 dCLK ;
   output [width -1 : 0] dD_OUT ;

   wire                  dPulse ;
   reg [width -1 : 0]    sDataSyncIn ;
   reg [width -1 : 0]    dD_OUT ;

   // instantiate a Handshake Sync
   SyncHandshake sync( .sCLK(sCLK), .sRST_N(sRST_N),
                       .dCLK(dCLK),
                       .sEN(sEN), .sRDY(sRDY),
                       .dPulse(dPulse) ) ;

   always @(posedge sCLK or negedge sRST_N)
      begin
         if (sRST_N == 0)
           begin
              sDataSyncIn <= `BSV_ASSIGNMENT_DELAY init ;
           end // if (sRST_N == 0)
         else
           begin
              if ( sEN )
                begin
                   sDataSyncIn <= `BSV_ASSIGNMENT_DELAY sD_IN ;
                end // if ( sEN )
           end // else: !if(sRST_N == 0)
      end // always @ (posedge sCLK or negedge sRST_N)
   

   // Transfer the data to destination domain when dPulsed is asserted.
   // Setup and hold time are assured since at least 2 dClks occured since
   // sDataSyncIn have been written.
   always @(posedge dCLK or negedge sRST_N)
      begin
         if (sRST_N == 0)
           begin
              dD_OUT <= `BSV_ASSIGNMENT_DELAY init ;
           end // if (sRST_N == 0)
         else
            begin
               if ( dPulse )
                 begin
                    dD_OUT <= `BSV_ASSIGNMENT_DELAY sDataSyncIn ;// clock domain crossing
                 end // if ( dPulse )
            end // else: !if(sRST_N == 0)
      end // always @ (posedge dCLK or negedge sRST_N)


`ifdef BSV_NO_INITIAL_BLOCKS
`else // not BSV_NO_INITIAL_BLOCKS
   // synopsys translate_off  
   initial
      begin
         sDataSyncIn = {((width + 1)/2){2'b10}} ;
         dD_OUT      = {((width + 1)/2){2'b10}} ; 
      end // initial begin
   // synopsys translate_on
`endif // BSV_NO_INITIAL_BLOCKS


endmodule // RegisterSync

               
         
`ifdef testBluespec
module testSyncRegister() ;
   parameter dsize = 8;
   
   wire      sCLK, sRST_N, dCLK ;
   wire      sEN ;
   wire      sRDY ;
      
   reg [dsize -1:0]  sCNT ;
   wire [dsize -1:0] sDIN, dDOUT ;
   
   ClockGen#(20,9,10)  sc( sCLK );
   ClockGen#(11,12,26)  dc( dCLK );

   initial
     begin
        sCNT = 0;
        
        $dumpfile("SyncRegister.dump");
        $dumpvars(5) ;
        $dumpon ;
        #100000 $finish ;
     end
   
   SyncRegister #(dsize)
     dut( sCLK, sRST_N, dCLK, 
          sEN, sRDY, sDIN,
          dDOUT ) ;
   
   
   assign sDIN = sCNT ;
   assign sEN = sRDY ;
   
   always @(posedge sCLK)
     begin
        if (sRDY )
          begin
             sCNT <= `BSV_ASSIGNMENT_DELAY sCNT + 1;
          end
      end // always @ (posedge sCLK)

   
   
endmodule // testSyncFIFO
`endif
         
         
