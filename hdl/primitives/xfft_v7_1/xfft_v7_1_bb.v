// Extracted empty module definition from the sim-only verilog file produced by coregen
(* box_type="user_black_box" *)
module xfft_v7_1 (
  input            clk,

  input            fwd_inv,
  input            fwd_inv_we,
  input  [11 : 0]  scale_sch,
  input            scale_sch_we,

  input            start,
  output           rfd,
  output           busy,
  input  [15 : 0]  xn_re,
  input  [15 : 0]  xn_im,
  output [11 : 0]  xn_index,

  output           done,
  output           edone,
  output           dv,
  output [15 : 0]  xk_re,
  output [15 : 0]  xk_im,
  output [11 : 0]  xk_index );
endmodule
