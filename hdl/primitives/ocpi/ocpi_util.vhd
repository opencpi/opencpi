library ieee; use ieee.std_logic_1164.all;
package util is
function width_for_max (n : natural) return natural;
component FIFO2
  generic (width   : natural := 1; \guarded\ : natural := 1);
  port(    CLK     : in  std_logic;
           RST     : in  std_logic;
           D_IN    : in  std_logic_vector(width - 1 downto 0);
           ENQ     : in  std_logic;
           DEQ     : in  std_logic;
           CLR     : in  std_logic;
           FULL_N  : out std_logic;
           EMPTY_N : out std_logic;
           D_OUT   : out std_logic_vector(width - 1 downto 0));
end component FIFO2;
end package util;
