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

-------------------------------------------------------------------------------
-- Simulation (Non-Synthesizable) Data Source
-------------------------------------------------------------------------------
--
-- File: sim_source.vhd
--
-- Description:
--
-- Implements a file I/O based signal generator for use with a behavioral
-- testbench. Output generated obeys an AXI-streaming interface protocol.
--
-------------------------------------------------------------------------------
-- Revision Log:
-------------------------------------------------------------------------------
-- 02/26/18:
-- File Created
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use std.textio.all;
use ieee.std_logic_textio.all;

entity sim_source_wide is
generic (
   NUM_PULSES         : in integer;
   DATA_WIDTH         : positive);

port (
   CLK         : in std_logic;
   RST         : in std_logic;
   EN          : in std_logic;
   DOUT_TDATA  : out std_logic_vector(DATA_WIDTH-1 downto 0);
   DOUT_TRDY   : in std_logic);

end entity sim_source_wide;

architecture behavior of sim_source_wide is

   signal out_enable : std_logic;
   signal pulse      : std_logic;
   signal duty_count : unsigned(DATA_WIDTH-1 downto 0) := (others => '0'); --fix this
   signal pulse_count : unsigned(DATA_WIDTH-1 downto 0) := (others => '0'); --fix this
   signal enough_pulses : std_logic;

begin

  -- gen_pulse: process
  -- begin
  --
  --     pulse <= '0';
  --     wait until RST = '1';
  --     wait until RST = '0';
  --     while true loop --find better way to do this
  --       pulse <= '1';
  --       wait for PULSE_WIDTH;
  --       pulse <= '0';
  --       wait for PULSE_WIDTH;
  --     end loop;
  -- end process;

  duty_counter : process (CLK)
  begin
  if rising_edge(CLK) then
    if RST = '1' then
      out_enable <= '0';
      enough_pulses <= '1';
    else
      if (EN = '1' and DOUT_TRDY = '1') then
        if (duty_count = NUM_PULSES-1) then
            duty_count  <= duty_count;
            out_enable <= '1';
        else
            duty_count  <= duty_count + 1;
            out_enable <= '0';
        end if;
      end if;
    end if;
  end if;
  end process;

  DOUT_TDATA <= std_logic_vector(duty_count);

end;
