-- package for letting VHDL access the verilogs in this library
library ieee; use ieee.std_logic_1164.all, ieee.numeric_std.all;
library ocpi; use ocpi.types.all;
package util is
component ROM
  generic (WIDTH    : natural;
           SIZE     : natural;
           INITFILE : string);
     port (CLK      : in std_logic;
           ADDR     : in  std_logic_vector(ocpi.util.width_for_max(size-1)-1 downto 0);
           DO       : out std_logic_vector(WIDTH-1 downto 0));
end component ROM;

--- CWD support during simulation
constant cwd_length : natural := 100;

component cwd is
   generic(length     : natural := cwd_length);
   port   (cwd        : out ocpi.types.string_t(0 to length));
end component cwd;

-- The basic verilog function for getting at the "plusarg".
component cwd_internal is
   generic(length : natural := cwd_length);
   port   (cwd    : out std_logic_vector(0 to length*8-1));
end component cwd_internal;

component TSINOUT_1 is
  port    (I  : in    std_logic;  -- OUTPUT to PIN when OE = 1
           OE : in    std_logic;                           -- output enable, 1 = enabled
           O  : out   std_logic;  -- INPUT from pin, all the time
           IO : inout std_logic); -- pin/pad
end component TSINOUT_1;

component TSINOUT_N is
  generic (width : natural);
  port    (I  : in    std_logic_vector(width-1 downto 0);  -- OUTPUT to PIN when OE = 1
           OE : in    std_logic;                           -- output enable, 1 = enabled
           O  : out   std_logic_vector(width-1 downto 0);  -- INPUT from pin, all the time
           IO : inout std_logic_vector(width-1 downto 0)); -- pin/pad
end component TSINOUT_N;

component SyncRegister is
  generic (
      width : positive := 1;
      init  : natural := 0);
    port (
      sCLK   : in std_logic;
      sRST   : in std_logic;
      dCLK   : in std_logic;
      sEN    : in std_logic;
      sRDY   : out std_logic;
      sD_IN  : in std_logic_vector(width-1 downto 0);      
      dD_OUT : out std_logic_vector(width-1 downto 0));
end component SyncRegister;

-- A convenience function to join a CWD to a filename
function cwd_join(cwd : string_t; name : string_t) return string;

type char_file_t is file of character;
procedure open_file(file thefile : char_file_t;
                         cwd     : string_t;
                         name    : string_t;
                         mode    : file_open_kind);

end package util;
