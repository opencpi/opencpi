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
component BRAM2
  generic (PIPELINED  : natural;
           ADDR_WIDTH : natural;
           DATA_WIDTH : natural;
           MEMSIZE    : natural);
    port  (CLKA       : in  std_logic;
           ENA        : in  std_logic;
           WEA        : in  std_logic;
           ADDRA      : in  std_logic_vector(ADDR_WIDTH-1 downto 0);
           DIA        : in  std_logic_vector(DATA_WIDTH-1 downto 0);
           DOA        : out std_logic_vector(DATA_WIDTH-1 downto 0);
           CLKB       : in  std_logic;
           ENB        : in  std_logic;
           WEB        : in  std_logic;
           ADDRB      : in  std_logic_vector(ADDR_WIDTH-1 downto 0);
           DIB        : in  std_logic_vector(DATA_WIDTH-1 downto 0);
           DOB        : out std_logic_vector(DATA_WIDTH-1 downto 0));
end component BRAM2;
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
-- A convenience function to join a CWD to a filename
function cwd_join(cwd : string_t; name : string_t) return string;

-- The basic verilog function for getting at the "plusarg".
constant plusarg_length : natural := 200;
component plusarg is
   generic(length     : natural := plusarg_length;
           name       : string);
   port   (val        : out ocpi.types.string_t(0 to length));
end component plusarg;
component plusarg_internal is
   generic(length : natural := plusarg_length;
           name : string);
   port   (val    : out std_logic_vector(0 to length*8-1));
end component plusarg_internal;

type char_file_t is file of character;
procedure open_file(file thefile : char_file_t;
                         cwd     : string_t;
                         name    : string_t;
                         mode    : file_open_kind);
procedure close_file(file thefile : char_file_t; name : string_t);

end package util;
