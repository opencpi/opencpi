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
-- GPO
-------------------------------------------------------------------------------
--
-- Description:
--
-- The GPO device worker can control the GPIO pins via the property mask_data
-- and its (optional) input port.
--
-- The (optional) input port uses a protocol that has one opcode and contains
-- data and a mask. See gpio_protocol.xml in ocpi.core/specs/gpio_protocol.xml
-- for further details on the protocol.
--
-- The port and the mask_data property set the values of pins using a
-- data and mask. In order for a GPIO pin to take on the value of a data bit,
-- the corresponding mask bit has to be set to 1. Pins that don't have a mask bit
-- set to 1 are left untouched. The MSW of mask_data and the port data, must be
-- the mask and the LSW must be the data.
--
-- The mask_enable property provides a way for enabling or disabling the use of
-- the property "mask_data"; or the input port ”data” and ”mask”; if it desired to
-- enable or disable any one of the ways of controlling the GPIO pins.
-- By default both are enabled.
--
-- There also is a pin_state property that stores the current GPIO pin state.

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions

architecture rtl of worker is

  -----------------------------------------------------------------------------
  -- Constants
  -----------------------------------------------------------------------------
  -- Data port in_in.data has same width as property so these constants can be
  -- used for indexing in_in.data and signals relating to the data port
  constant c_width       : natural := props_in.mask_data'length;
  constant c_half_width  : natural := c_width/2;
  constant c_NUM_OUTPUTS : integer := to_integer(NUM_OUTPUTS);

  -- Data and mask are 16 bits each for mask_data and data port
  signal s_port_mask          : unsigned(c_half_width-1 downto 0) := (others => '0');
  signal s_port_data          : unsigned(c_half_width-1 downto 0) := (others => '0');
  signal s_prop_mask          : unsigned(c_half_width-1 downto 0) := (others => '0');
  signal s_prop_data          : unsigned(c_half_width-1 downto 0) := (others => '0');
  -- The property data and port data OR'ed together. Controls GPIO pins and drive props_out.mask_data
  signal s_mask               : unsigned(c_half_width-1 downto 0) := (others => '0');
  -- The property mask and port mask OR'ed together. Controls GPIO pins and drive props_out.mask_data
  signal s_data               : unsigned(c_half_width-1 downto 0) := (others => '0');
  signal s_gpo_pin            : unsigned(c_NUM_OUTPUTS-1 downto 0) := (others => '0');
  signal s_clr                : std_logic_vector(c_NUM_OUTPUTS-1 downto 0) := (others => '0');

begin

  in_out.take <= in_in.valid;

  -- Mix of Property and Data Port to control GPIO and drive props_out.mask_data
  -- The property and port masks are OR'ed together and the data are OR'ed together

  -- Use s_prop_mask if the mask_enable(0) bit is high
  s_prop_mask <= props_in.mask_data(c_width-1 downto c_half_width) when (its(props_in.mask_enable(0))) else (others => '0');
  s_prop_data <= props_in.mask_data(c_half_width-1 downto 0) and s_prop_mask;

  -- Use s_port_mask if the data is valid and the mask_enable(1) bit is high
  s_port_mask <= unsigned(in_in.data(c_width-1 downto c_half_width)) when (in_in.valid='1' and its(props_in.mask_enable(1))) else (others => '0');
  s_port_data <= unsigned(in_in.data(c_half_width-1 downto 0)) and s_port_mask;


  s_mask <= s_prop_mask or s_port_mask;
  s_data <= s_prop_data or s_port_data;

  -- Using the set_clr primitive to set the GPIO pins and also preserve current GPIO pin state
  output_condition_i : for ii in 0 to c_NUM_OUTPUTS-1 generate
    set_clr : util_prims.util_prims.set_clr
      port map(
        clk => ctl_in.clk,
        rst => ctl_in.reset,
        set => s_data(ii),
        clr => s_clr(ii),
        q   => s_gpo_pin(ii),
        q_r => open);

    s_clr(ii) <= s_mask(ii) and not s_data(ii);
    gpo_pin(ii) <= s_gpo_pin(ii);
    props_out.pin_state(ii) <= s_gpo_pin(ii);
  end generate output_condition_i;

end rtl;
