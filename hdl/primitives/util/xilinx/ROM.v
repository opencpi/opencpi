// Simplest ROM for Xilinx inferred BRAM
module ROM(CLK, ADDR, DO);
  parameter  WIDTH      = 1;
  parameter  SIZE       = 1;
  parameter  INITFILE   = "";

  input                     CLK;
  input  [$clog2(SIZE)-1:0] ADDR;
  output [WIDTH-1:0]        DO;

  reg [WIDTH-1:0]           ROM[0:SIZE-1];
  reg [WIDTH-1:0]           DO_R;

  initial
  begin : init_block
    $readmemh(INITFILE, ROM);
  end

  // having a clocked output register infers BRAM
  always @(posedge CLK) begin
    DO_R <= ROM[ADDR];
  end

  assign DO = DO_R;

endmodule // ROM
