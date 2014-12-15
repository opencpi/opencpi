module cwd_internal(cwd);
parameter length = 100;
output reg[0:length*8-1] cwd;
initial
begin
  if ($value$plusargs("cwd=%s", cwd))
    $display("Received CWD: %s", cwd);
end
endmodule  

