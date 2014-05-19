// ROM module, initialized from file, inferred on Altera

module ROM(CLK, ADDR, DO);
  parameter  WIDTH          = 1;
  parameter  SIZE           = 1;
  parameter  INITFILE       = "";

  input                     CLK;
  input  [$clog2(SIZE)-1:0] ADDR;
  output [WIDTH-1:0]        DO;

  reg [WIDTH-1:0]           ROM[0:SIZE-1];
  reg [WIDTH-1:0]           DO_R;
  reg [$clog2(SIZE)-1:0]    ADDR_R;

  initial
  begin : init_block
    $readmemh(INITFILE, ROM);
  end

  always @(posedge CLK) begin
    ADDR_R <= ADDR;
    DO_R   <= ROM[ADDR_R];
  end

  assign DO = DO_R;

endmodule // ROM
