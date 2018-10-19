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

library IEEE; use IEEE.std_logic_1164.all, ieee.numeric_std.all;
library util; use util.util.all;
entity signals is
  port (w_ENABLE : in  std_logic;
        w_TXNRX  : in  std_logic;
        ENABLE   : out std_logic;
        TXNRX    : out std_logic);
end entity signals;
architecture rtl of signals is
begin

  -- device worker signal ENABLE (output)
  ENABLE_buffer : BUFFER_OUT_1
    generic map(DIFFERENTIAL => false)
    port map(I => w_ENABLE,
             O => ENABLE);

  -- device worker signal TXNRX (output)
  TXNRX_buffer : BUFFER_OUT_1
    generic map(DIFFERENTIAL => false)
    port map(I => w_TXNRX,
             O => TXNRX);

end rtl;
