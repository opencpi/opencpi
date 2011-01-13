(* box_type="user_black_box" *)
module duc_ddc_compiler_v1_0 (
  sreg_presetn, sreg_psel, int_errpacket, sreg_pslverr, mdata_valid, sdata_ready, data_resetn, clk, mdata_ready, int_missinput, sdata_valid, 
mdata_clean, int_ducddc, sreg_pwrite, mdata_last, int_lostoutput, sreg_pready, sreg_penable, mdata_i, mdata_q, sdata_r, sreg_prdata, sreg_pwdata, 
sreg_paddr
)/* synthesis syn_black_box syn_noprune=1 */;
  input sreg_presetn;
  input sreg_psel;
  output int_errpacket;
  output sreg_pslverr;
  output mdata_valid;
  output sdata_ready;
  input data_resetn;
  input clk;
  input mdata_ready;
  output int_missinput;
  input sdata_valid;
  output mdata_clean;
  output int_ducddc;
  input sreg_pwrite;
  output mdata_last;
  output int_lostoutput;
  output sreg_pready;
  input sreg_penable;
  output [15 : 0] mdata_i;
  output [15 : 0] mdata_q;
  input [15 : 0] sdata_r;
  output [31 : 0] sreg_prdata;
  input [31 : 0] sreg_pwdata;
  input [11 : 0] sreg_paddr;
endmodule