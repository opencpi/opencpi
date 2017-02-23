-- Xilinx equivalent of global clock buffer from Example 4 on page 31 of https://www.altera.com/content/dam/altera-www/global/en_US/pdfs/literature/an/an307.pdf
library IEEE;
use IEEE.std_logic_1164.all, ieee.numeric_std.all;
library unisim; use unisim.vcomponents.all;
entity buffer_clock_global is
  port (clk          : in  std_logic;
        clk_buffered : out std_logic);
end entity buffer_clock_global;
architecture rtl of buffer_clock_global is
begin
  buf : BUFG
    port map(
      I => clk,
      O =>clk_buffered 
      );
end rtl;

