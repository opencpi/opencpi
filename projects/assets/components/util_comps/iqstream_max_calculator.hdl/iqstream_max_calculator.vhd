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

-- THIS FILE WAS ORIGINALLY GENERATED ON Wed Jul 18 16:21:43 2018 EDT
-- BASED ON THE FILE: iqstream_max_calculator.xml
-- YOU *ARE* EXPECTED TO EDIT IT
-- This file initially contains the architecture skeleton for worker: iqstream_max_calculator

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions
architecture rtl of iqstream_max_calculator_worker is

  -- all max values will take this value upon reset
  constant RESET_VAL_c : integer := -32768;

  -- mandatory input port logic
  signal ready_for_in_port_data : std_logic := '0';

  -- mandatory output port logic
  signal data_ready_for_out_port : std_logic := '0';
  signal out_meta_is_reserved    : std_logic := '0';
  signal out_som                 : std_logic := '0';
  signal out_eom                 : std_logic := '0';
  signal out_valid               : std_logic := '0';

  signal out_port_connected_n : std_logic := '0';
  signal in_I    : signed((in_in.data'length/2)-1 downto 0) := (others => '0');
  signal tmp     : std_logic_vector((in_in.data'length/2)-1 downto 0) := (others => '0');
  signal in_Q    : signed((in_in.data'length/2)-1 downto 0) := (others => '0');
  signal max_I   : signed((in_in.data'length/2)-1 downto 0) := to_signed(RESET_VAL_c,in_in.data'length/2);
  signal max_Q   : signed((in_in.data'length/2)-1 downto 0) := to_signed(RESET_VAL_c,in_in.data'length/2);
  signal max_I_r : signed((in_in.data'length/2)-1 downto 0) := to_signed(RESET_VAL_c,in_in.data'length/2);
  signal max_Q_r : signed((in_in.data'length/2)-1 downto 0) := to_signed(RESET_VAL_c,in_in.data'length/2);
  signal max_I_is_valid   : std_logic := '0';
  signal max_Q_is_valid   : std_logic := '0';
  signal max_I_is_valid_r : std_logic := '0';
  signal max_Q_is_valid_r : std_logic := '0';
begin

  ------------------------------------------------------------------------------
  -- in port
  ------------------------------------------------------------------------------

  -- mandatory input port logic, (note that
  -- ready_for_in_port_data MUST have the same clock latency as the latency
  -- between in port's data and any associated output port's data)
  in_out.take <= ctl_in.is_operating and in_in.ready and
                 ready_for_in_port_data; -- this applies backpressure

  -- out port is optional
  ready_for_in_port_data <= out_in.ready or out_port_connected_n;
  out_port_connected_n <= out_in.reset;

  -- iqstream w/ DataWidth=32 formats Q in most significant bits, I in least
  -- significant (see OpenCPI_HDL_Development section on Message Payloads vs.
  -- Physical Data Width on Data Interfaces)
  in_Q <= signed(in_in.data(in_in.data'length-1 downto
                            in_in.data'length - in_Q'length));
  in_I <= signed(in_in.data(in_I'length-1 downto 0));

  ------------------------------------------------------------------------------
  -- out port
  ------------------------------------------------------------------------------

  -- mandatory output port logic, (note that
  -- data_ready_for_out_port MUST be clock-aligned with out_out.data)
  -- (note that reserved messages will be DROPPED ON THE FLOOR)
  out_out.give <= ctl_in.is_operating and out_in.ready and
                  (not out_meta_is_reserved) and data_ready_for_out_port;
  out_meta_is_reserved <= (not out_som) and (not out_valid) and (not out_eom);
  out_out.som   <= out_som;
  out_out.eom   <= out_eom;
  out_out.valid <= out_valid;

  data_ready_for_out_port <= in_in.ready;

  -- iqstream w/ DataWidth=32 formats Q in most significant bits, I in least
  -- significant (see OpenCPI_HDL_Development section on Message Payloads vs.
  -- Physical Data Width on Data Interfaces)
  out_out.data        <= std_logic_vector(in_Q) & std_logic_vector(in_I);
  out_som             <= in_in.som;
  out_eom             <= in_in.eom;
  out_valid           <= in_in.valid;
  out_out.byte_enable <= in_in.byte_enable;

  ------------------------------------------------------------------------------
  -- max_I , max_Q properties
  ------------------------------------------------------------------------------

  max_calc : process(ctl_in.is_operating, in_in.ready,
                     ready_for_in_port_data, in_I, max_I_r, in_Q, max_Q_r)
  begin
    if ctl_in.reset = '1' then
      max_I <= to_signed(RESET_VAL_c, max_I'length);
      max_Q <= to_signed(RESET_VAL_c, max_Q'length);
    elsif (ctl_in.is_operating = '1') and (in_in.ready = '1') and
        (in_in.valid = '1') and (ready_for_in_port_data = '1') then
      if props_in.max_I_read = '1' then
        max_I <= to_signed(RESET_VAL_c, max_I'length);
      elsif in_I > max_I_r then
        max_I <= in_I;
      end if;
      if props_in.max_Q_read = '1' then
        max_Q <= to_signed(RESET_VAL_c, max_Q'length);
      elsif in_Q > max_Q_r then
        max_Q <= in_Q;
      end if;
    end if;
  end process;

  max_regs : process(in_in.ready, ready_for_in_port_data)
  begin
    if rising_edge(ctl_in.clk) then
      if ctl_in.reset = '1' then
        max_I_r <= to_signed(RESET_VAL_c, max_I'length);
        max_Q_r <= to_signed(RESET_VAL_c, max_Q'length);
      elsif ctl_in.is_operating = '1' then
        max_I_r <= max_I;
        max_Q_r <= max_Q;
      end if;
    end if;
  end process;

  props_out.max_I <= max_I;
  props_out.max_Q <= max_Q;

  ------------------------------------------------------------------------------
  -- max_I_is_valid, max_Q_is_valid properties
  ------------------------------------------------------------------------------

  max_I_is_valid <= (max_I_is_valid_r or
                    (in_in.ready and in_in.valid and ready_for_in_port_data))
                    and not (ctl_in.reset or props_in.max_I_read);
  max_Q_is_valid <= (max_Q_is_valid_r or
                    (in_in.ready and in_in.valid and ready_for_in_port_data))
                    and not (ctl_in.reset or props_in.max_Q_read);

  is_valid_regs : process(ctl_in.clk)
  begin
    if rising_edge(ctl_in.clk) then
      if ctl_in.reset = '1' then
        max_I_is_valid_r <= '0';
        max_Q_is_valid_r <= '0';
      elsif ctl_in.is_operating = '1' then
        max_I_is_valid_r <= max_I_is_valid;
        max_Q_is_valid_r <= max_Q_is_valid;
      end if;
    end if;
  end process;

  props_out.max_I_is_valid <= to_bool(max_I_is_valid);
  props_out.max_Q_is_valid <= to_bool(max_Q_is_valid);

end rtl;
