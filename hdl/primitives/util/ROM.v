// Simplest ROM
module ROM(CLK, ADDR, DO);
  parameter  WIDTH          = 1;
  parameter  SIZE           = 1;
  parameter  INITFILE       = "";

  input                     CLK;
  input  [$clog2(SIZE)-1:0] ADDR;
  output [WIDTH-1:0]        DO;

  reg [WIDTH-1:0]           ROM[0:SIZE-1];

  initial
  begin : init_block
    $readmemh(INITFILE, ROM);
  end

  assign DO = ROM[ADDR];

endmodule // ROM
