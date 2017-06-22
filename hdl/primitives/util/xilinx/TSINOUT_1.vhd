library ieee; use ieee.std_logic_1164.all;
library unisim; use unisim.vcomponents.all;

entity TSINOUT_1 is
  port    (I  : in    std_logic;  -- OUTPUT to PIN when OE = 1
           OE : in    std_logic;                           -- output enable, 1 = enabled
           O  : out   std_logic;  -- INPUT from pin, all the time
           IO : inout std_logic); -- pin/pad
end entity TSINOUT_1;
architecture rtl of TSINOUT_1 is
signal NOE : std_logic;
begin
NOE <= NOT(OE);
  buf : IOBUF
    port map(
      I => I,
      T => NOE,
      O => O,
      IO => IO
      );
end rtl;
