module IDELAYCTRL_GRP#(
  parameter IODELAY_GRP = "IODELAY_XXX")
( input    REFCLK,  
  input    RST,
  output   RDY
);

(* IODELAY_GROUP = IODELAY_GRP *)
  IDELAYCTRL idc(.REFCLK(REFCLK), .RST(RST), .RDY(RDY));

endmodule
