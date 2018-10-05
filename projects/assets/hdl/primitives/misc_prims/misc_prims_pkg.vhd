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

library ieee;
use ieee.std_logic_1164.all;
USE IEEE.NUMERIC_STD.ALL;
USE IEEE.MATH_REAL.ALL;
USE IEEE.MATH_COMPLEX.ALL;

package misc_prims is

component round_conv
  generic (
    DIN_WIDTH  : positive;
    DOUT_WIDTH : positive);
  port (
    CLK      : in  std_logic;
    RST      : in  std_logic;
    DIN      : in  std_logic_vector(DIN_WIDTH-1 downto 0);
    DIN_VLD  : in  std_logic;
    DOUT     : out std_logic_vector(DOUT_WIDTH-1 downto 0);
    DOUT_VLD : out std_logic);
end component;

component lfsr
  generic (
    POLYNOMIAL : std_logic_vector;
    SEED       : std_logic_vector); -- must never be all zeros
  port (
    CLK      : in std_logic; -- rising edge clock
    RST      : in std_logic; -- synchronous, active high
    EN       : in std_logic; -- synchronous, active high
    REG      : out std_logic_vector(POLYNOMIAL'length-1 downto 0));
end component;

component event_in_to_txen
  port (
    EVENT_IN_CLK           : in  std_logic;
    EVENT_IN_RESET         : in  std_logic;
    CTL_IN_IS_OPERATING    : in  std_logic;
    EVENT_IN_IN_RESET      : in  std_logic;
    EVENT_IN_IN_SOM        : in  std_logic;
    EVENT_IN_IN_VALID      : in  std_logic;
    EVENT_IN_IN_EOM        : in  std_logic;
    EVENT_IN_IN_READY      : in  std_logic;
    EVENT_IN_OUT_TAKE      : in  std_logic;
    -- '1'/'0' corresponds to on opcode/off opcode
    EVENT_IN_OPCODE_ON_OFF : in  std_logic;
    -- use case 1: use tx enable to directly drive pin
    TXEN                   : out std_logic;
    -- use case 2: use intermediate signals to drive other logic which
    -- drives pin (useful when there are multiple channels/event ports)
    TXON_PULSE             : out std_logic;
    TXOFF_PULSE            : out std_logic;
    EVENT_IN_CONNECTED     : out std_logic;
    IS_OPERATING           : out std_logic);
end component;

end package misc_prims;
