-- package for letting VHDL access the verilogs in this library
library ieee; use ieee.std_logic_1164.all, ieee.numeric_std.all;
package bsv is
-- this one has a pipelined/early full flag it tell what the fullness will be in the next cycle.
component FIFO2X
  generic (width   : natural := 1; \guarded\ : natural := 1);
  port(    CLK     : in  std_logic;
           RST     : in  std_logic;
           D_IN    : in  std_logic_vector(width - 1 downto 0);
           ENQ     : in  std_logic;
           FULL_N  : out std_logic;
           D_OUT   : out std_logic_vector(width - 1 downto 0);
           DEQ     : in  std_logic;
           EMPTY_N : out std_logic;
           CLR     : in  std_logic);
end component FIFO2X;
component FIFO2
  generic (width   : natural := 1; \guarded\ : natural := 1);
  port(    CLK     : in  std_logic;
           RST     : in  std_logic;
           D_IN    : in  std_logic_vector(width - 1 downto 0);
           ENQ     : in  std_logic;
           FULL_N  : out std_logic;
           D_OUT   : out std_logic_vector(width - 1 downto 0);
           DEQ     : in  std_logic;
           EMPTY_N : out std_logic;
           CLR     : in  std_logic);
end component FIFO2;
component SyncResetA
  generic(RSTDELAY : natural := 1);
  port(   IN_RST   : in  std_logic;
          CLK      : in  std_logic;
          OUT_RST  : out std_logic);
end component SyncResetA;     
end package bsv;
