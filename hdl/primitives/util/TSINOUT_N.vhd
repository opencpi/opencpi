library ieee; use ieee.std_logic_1164.all;
entity TSINOUT_N is
  generic (width : natural);
  port    (I  : in    std_logic_vector(width-1 downto 0);  -- OUTPUT to PIN when OE = 1
           OE : in    std_logic;                           -- output enable, 1 = enabled
           O  : out   std_logic_vector(width-1 downto 0);  -- INPUT from pin, all the time
           IO : inout std_logic_vector(width-1 downto 0)); -- pin/pad
end entity TSINOUT_N;
architecture rtl of TSINOUT_N is
begin
  IO <= (others => 'Z') when OE = '0' else I;
  O  <= IO;
end rtl;
