// Simple module to get plusargs
module plusarg_internal(val);
parameter name = "";
parameter length = 200;
output reg[0:length*8-1] val;
initial
begin
  if ($value$plusargs({name,"=%s"}, val))
    $display("Received ",name," plusarg value: %s", val);
end
endmodule  

