-- This file is protected by Copyright. Please refer to the COPYRIGHT file
-- distributed with this source distribution.
--
-- This file is part of OpenCPI <http://www.opencpi.org>
--
-- OpenCPI is free software: you can redistribute it and/or modify it under the
-- terms of the GNU Lesser General Public License as published by the Free
-- Software Foundation, either version 3 of the License, or (at your option) any
-- later version.
--
-- OpenCPI is distributed in the hope that it will be useful, but WITHOUT ANY
-- WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
-- A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
-- details.
--
-- You should have received a copy of the GNU Lesser General Public License
-- along with this program. If not, see <http://www.gnu.org/licenses/>.

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
component FIFO1
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
end component FIFO1;
component SyncResetA
  generic(RSTDELAY : natural := 1);
  port(   IN_RST   : in  std_logic;
          CLK      : in  std_logic;
          OUT_RST  : out std_logic);
end component SyncResetA;     
component SizedFIFO
   generic(p1width      : natural := 1; -- data width
           p2depth      : natural := 3;
           p3cntr_width : natural := 1; -- log(p2depth-1)
           -- The -1 is allowed since this model has a fast output register
           \guarded\      : natural := 1);
   port(   CLK          : in  std_logic;
           RST          : in  std_logic;
           D_IN         : in  std_logic_vector(p1width-1 downto 0);
           ENQ          : in  std_logic;
           FULL_N       : out std_logic;
           D_OUT        : out std_logic_vector(p1width-1 downto 0);
           DEQ          : in  std_logic;
           EMPTY_N      : out std_logic;
           CLR          : in  std_logic);
end component SizedFIFO;
component BRAM1Load
     generic(
       FILENAME   : string := "";
       PIPELINED  : natural := 0;
       ADDR_WIDTH : natural := 1;
       DATA_WIDTH : natural := 1;
       MEMSIZE    : natural := 1;
       BINARY     : natural := 0
       );
     port(
       CLK  : in std_logic;
       EN   : in std_logic;
       WE   : in std_logic;
       ADDR : in std_logic_vector(ADDR_WIDTH-1 downto 0);
       DI   : in std_logic_vector(DATA_WIDTH-1 downto 0);
       DO   : out std_logic_vector(DATA_WIDTH-1 downto 0)
       );
end component BRAM1Load;
component SyncRegister
  generic(
    width : natural := 1;
    init  : natural := 0 -- std_logic_vector(width-1 downto 0) := (others => '0')
    );
  port(
    sCLK   : in  std_logic;
    sRST   : in  std_logic;
    dCLK   : in  std_logic;
    sEN    : in  std_logic;
    sRDY   : out std_logic;
    sD_IN  : in  std_logic_vector(width-1 downto 0);
    dD_OUT : out std_logic_vector(width-1 downto 0)
    );
end component SyncRegister;
component SyncFIFO
  generic (dataWidth : natural := 1;
           depth     : natural := 2;
           indxWidth : natural   := 1);
  port(    sCLK      : in  std_logic;
           sRST      : in  std_logic;
           dCLK      : in  std_logic;
           sENQ      : in  std_logic;
           sD_IN     : in  std_logic_vector(dataWidth - 1 downto 0);
           sFULL_N   : out std_logic;
           dDEQ      : in  std_logic;
           dD_OUT    : out std_logic_vector(dataWidth - 1 downto 0);
           dEMPTY_N  : out std_logic);
end component SyncFIFO;
component SyncBit
  generic (init      : natural := 0);
  port(    sCLK      : in  std_logic;
           sRST      : in  std_logic;
           dCLK      : in  std_logic;
           sEN       : in  std_logic;
           sD_IN     : in  std_logic;
           dD_OUT    : out std_logic);
end component SyncBIT;
component SyncHandshake
  generic (init      : natural := 0);
  port(    sCLK      : in  std_logic;
           sRST      : in  std_logic;
           dCLK      : in  std_logic;
           sEN       : in  std_logic;
           sRDY      : out std_logic;
           dPulse    : out std_logic);
end component SyncHandshake;
component SyncPulse
  port(    sCLK      : in  std_logic;
           sRST      : in  std_logic;
           dCLK      : in  std_logic;
           sEN       : in  std_logic;
           dPulse    : out std_logic);
end component SyncPulse;
component TriState
  generic (width : natural := 1);
  port    (O     : out   std_logic_vector(width-1 downto 0);
           IO    : inout std_logic_vector(width-1 downto 0);
           OE    : in    std_logic;
           I     : in    std_logic_vector(width-1 downto 0));
end component TriState;
end package bsv;
