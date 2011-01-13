// ClockInvToBool.v
// Copyright (c) 2010 Atomic Rules LLC, ALL RIGHTS RESERVED
// 2010-02-05 ssiegel Creation
// 2010-02-06 ssiegel Flop added
//
module ClockInvToBool(
  input  CLK_SLOW,
  input  CLK_FAST,
  output CLK_VAL);

FDRSE#(.INIT(1'b0)) FRDSE_inst (.Q(CLK_VAL), .C(CLK_FAST), .CE(1'b1), .D(CLK_SLOW), .R(1'b0), .S(1'b0));

  // Without the flop, invert the slow clock
  //wire      CLK_VAL;
  //assign    CLK_VAL = !CLK_SLOW;
endmodule
