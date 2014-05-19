// Dummy for v5 until we have a proper encapsulation
module DNA_PORT (DOUT, CLK, DIN, READ, SHIFT);

    output DOUT;
    input  CLK, DIN, READ, SHIFT;

  assign DOUT = 0;

endmodule // DNA_PORT
