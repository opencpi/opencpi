module cwd_internal(cwd, len_err);
parameter length = 100;
output reg[0:length*8-1] cwd;
output reg len_err;
initial
begin
  if ($value$plusargs("cwd=%s", cwd)) begin
    $display("Received CWD: %s", cwd);

    // Uncomment below to display CWD in hex for debugging
    //$display("Hex CWD: %h", cwd);

    // if the leftmost byte is not null, buffer may be too small, return error
    if (cwd[0:7] !== 8'h00) begin
      $display("No null characters found in CWD. Increase CWD_MAX_LENGTH");
      len_err = 1;
    end else
      len_err = 0;

  end
end
endmodule  

