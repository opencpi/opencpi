library ieee; use ieee.std_logic_1164.all;
entity TSINOUT_1 is
  port    (I  : in    std_logic;  -- OUTPUT to PIN when OE = 1
           OE : in    std_logic;                           -- output enable, 1 = enabled
           O  : out   std_logic;  -- INPUT from pin, all the time
           IO : inout std_logic); -- pin/pad
end entity TSINOUT_1;
architecture rtl of TSINOUT_1 is
begin
  IO <= 'Z' when OE = '0' else I;
  O  <= IO;
end rtl;
