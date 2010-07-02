module DCM_BUFG (input I, input RST_N, output O);

   wire RST_P;
   assign RST_P = !RST_N;

   BUFG CLK0_BUFG_INST (.I(I), .O(O));

endmodule
