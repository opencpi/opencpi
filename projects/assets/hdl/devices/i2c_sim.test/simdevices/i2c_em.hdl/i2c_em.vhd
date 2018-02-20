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

-- THIS FILE WAS ORIGINALLY GENERATED ON Mon May 11 19:59:20 2015 EDT
-- BASED ON THE FILE: i2c_em.xml
-- YOU *ARE* EXPECTED TO EDIT IT
-- This file initially contains the architecture skeleton for worker: i2c_em
library ieee, ocpi, util;
use IEEE.std_logic_1164.all, ieee.numeric_std.all, ocpi.types.all;
architecture rtl of i2c_em_worker is

    --Simulation slave model
  component i2c_slave_model
    generic(
      I2C_ADR : integer
      );
    port (
      SCL : in    std_logic;
      SDA : inout std_logic
      );
  end component;

  signal scl : std_logic;
  signal sda : std_logic;
  
begin
  scl <= 'H';
  
  i2c_sda_ts: util.util.TSINOUT_1
    port map(
      I => sda_o,
      IO => sda,
      O => sda_i,
      OE => sda_oe
      );

  i2c_scl_ts: util.util.TSINOUT_1
    port map(
      I => scl_o,
      IO => scl,
      O => scl_i,
      OE => scl_oe
      );
  
  --Simulation slave model
  i2c_sm_inst : i2c_slave_model
    generic map(
      I2C_ADR => 96                    --0x60
      )
    port map (
      SCL => scl,
      SDA => sda
      );
end rtl;
