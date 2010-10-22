
/*
 *  Copyright (c) Atomic Rules LLC, Auburn NH., 2009-2010
 *
 *    Atomic Rules LLC
 *    287 Chester Road
 *    Auburn, NH 03032
 *    United States of America
 *    Telephone 603-483-0994
 *
 *  This file is part of OpenCPI (www.opencpi.org).
 *     ____                   __________   ____
 *    / __ \____  ___  ____  / ____/ __ \ /  _/ ____  _________ _
 *   / / / / __ \/ _ \/ __ \/ /   / /_/ / / /  / __ \/ ___/ __ `/
 *  / /_/ / /_/ /  __/ / / / /___/ ____/_/ / _/ /_/ / /  / /_/ /
 *  \____/ .___/\___/_/ /_/\____/_/    /___/(_)____/_/   \__, /
 *      /_/                                             /____/
 *
 *  OpenCPI is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  OpenCPI is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.
 */

// arSRLFIFO.v
//
// 2009-05-10 ssiegel Creation in VHDL
// 2009-11-01 ssiegel Converted to Verilog from VHDL
// 2010-07-23 ssiegel Logic change for empty and full

module arSRLFIFO (CLK,RST_N,ENQ,DEQ,FULL_N,EMPTY_N,D_IN,D_OUT,CLR);

  parameter width  = 128;
  parameter l2depth = 5;
  parameter depth  = 2**l2depth;

  input             CLK;
  input             RST_N;
  input             CLR;
  input             ENQ;
  input             DEQ;
  output            FULL_N;
  output            EMPTY_N;
  input[width-1:0]  D_IN;
  output[width-1:0] D_OUT;

  reg[l2depth-1:0]  pos;             // head position
  reg[width-1:0]    dat[depth-1:0];  // SRL and output DFFs wed together
  reg               empty, full;

  integer i;

  always@(posedge CLK) begin
    if(!RST_N || CLR) begin
      pos     <= 1'b0;
      empty   <= 1'b1;
      full    <= 1'b0;
    end else begin
      if (!ENQ &&  DEQ) pos <= pos - 1;
      if ( ENQ && !DEQ) pos <= pos + 1;
      if (ENQ) begin
        for(i=depth-1;i>0;i=i-1) dat[i] <= dat[i-1];
        dat[0] <= D_IN;
      end
      //was... (1-cycle "late" in updating empty after ENQ or full after DEQ)
      //empty <= (pos==0 || (pos==1 && (DEQ&&!ENQ)));
      //full  <= (pos==(depth-1) || (pos==(depth-2) && (ENQ&&!DEQ)));
      //is...
      empty <= ((pos==0         && !ENQ) || (pos==1         && (DEQ&&!ENQ)));
      full  <= ((pos==(depth-1) && !DEQ) || (pos==(depth-2) && (ENQ&&!DEQ)));
    end
  end

  assign FULL_N  = !full;
  assign EMPTY_N = !empty;
  assign D_OUT   = dat[pos-1];

endmodule
