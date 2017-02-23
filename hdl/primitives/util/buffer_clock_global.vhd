-- generic equivalent of global clock buffer
library IEEE;
use IEEE.std_logic_1164.all, ieee.numeric_std.all;
entity buffer_clock_global is
  port (clk          : in  std_logic;
        clk_buffered : out std_logic);
end entity buffer_clock_global;
architecture rtl of buffer_clock_global is
begin
  clk_buffered <= clk;
end rtl;

