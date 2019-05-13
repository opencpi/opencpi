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
-- Matchstiq-Z1 GP Out
-------------------------------------------------------------------------------
--
-- Description:
--
-- The Matchstiq-Z1 GP Out device worker can control the GPIO pins via the
-- property mask_data, it’s (optional) input port, and the devsignal, dev_gp.
-- The devsignal only controls FPGA_GPIO1, while the property and the (optional)
-- port control all 3 pins.
--
-- The (optional) input port uses a protocol that has one opcode and contains
-- data and a mask. See gpio_protocol.xml in ocpi.core/specs/gpio_protocol.xml
-- for further details on the protocol.
--
-- The devsignal dev_gp is controlled by the lime_tx device worker through the
-- lime_tx’s dev_txen_dac_in.txen devsignal. This provides a way to control a
-- device when transmit is enabled. By default FPGA_GPIO1 is controlled by
-- lime_tx. When the Matchstiq-Z1 GP Out device worker is in the reset state,
-- this signal is still controlled by lime_tx.
--
-- All three ways of controlling the GPIO pins set the values of pins using a
-- data and mask. In order for a GPIO pin to take on the value of a set data bit,
-- the corresponding mask bit has to be set. The MSW of mask data and the data
-- port data, must be the mask and LSW must be the data and the 3 LSB of the mask
-- and data correspond to the three GPIO pins of the Matchstiq-Z1.
--
-- The input_mask property provides a way for enabling or disabling the use of
-- the property "mask_data"; in port ”data” and ”mask”; or the devsignal ”data”
-- and ”mask”, if it desired to enable or disable any one of these ways of
-- controlling the GPIO pins. By default all three are enabled.


library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions
architecture rtl of matchstiq_z1_gp_out_worker is

  -----------------------------------------------------------------------------
  -- Constants
  -----------------------------------------------------------------------------
  -- Data port in_in.data has same width as property so these constants can be
  -- used for indexing in_in.data and signals relating to the data port
  constant c_width           : natural := props_out.mask_data'length;
  constant c_half_width      : natural := c_width/2;
  -----------------------------------------------------------------------------
  -- Signals for Matchstiq_z1 GP Out
  -----------------------------------------------------------------------------
  signal s_enable                 : std_logic := '0';
  signal s_idata_valid            : std_logic := '0';
  -- Data and mask are 16 bits each for mask_data and data port
  signal s_port_mask              : unsigned(c_half_width-1 downto 0) := (others => '0');
  signal s_port_data              : unsigned(c_half_width-1 downto 0) := (others => '0');
  signal s_prop_mask              : unsigned(c_half_width-1 downto 0) := (others => '0');
  signal s_prop_data              : unsigned(c_half_width-1 downto 0) := (others => '0');
  signal s_mask                   : unsigned(c_half_width-1 downto 0) := (others => '0'); -- The property data and port data OR'ed together. Controls gpio pins and drive props_out.mask_data
  signal s_data                   : unsigned(c_half_width-1 downto 0) := (others => '0'); -- The property mask and port mask OR'ed together. Controls gpio pins and drive props_out.mask_data
  signal s_dev_gp_data            : std_logic := '0';
  signal s_dev_gp_mask            : std_logic := '0';
  signal gp_em_port_enable        : std_logic := '0';
  signal gp_em_prop_enable        : std_logic := '0';


begin


  -----------------------------------------------------------------------------
  -- 's_enable' when Control State is_operating and upstream Worker ready
  -----------------------------------------------------------------------------
  s_enable <= ctl_in.is_operating and in_in.ready;

  -----------------------------------------------------------------------------
  -- 's_idata_valid' when enabled and input valid
  -----------------------------------------------------------------------------
  s_idata_valid <= s_enable and in_in.valid;

  -----------------------------------------------------------------------------
  -- Take (when upstream Worker ready and operating)
  -----------------------------------------------------------------------------

  in_out.take <= s_enable;


  -- Mix of Property, Data Port and DevSignals to control GP outputs and drive props_out.mask_data
  -- The property, port, and devsignal masks are OR'ed together and the data are
  -- OR'ed together

  -- Use s_prop_mask if the input_mask(0) bit is high
  s_prop_mask <= props_in.mask_data(c_width-1 downto c_half_width) when (its(props_in.input_mask(0))) else (others => '0');
  s_prop_data <= props_in.mask_data(c_half_width-1 downto 0) and s_prop_mask;

  -- Use s_port_mask if the data is valid and the input_mask(1) bit is high
  s_port_mask <= unsigned(in_in.data(c_width-1 downto c_half_width)) when (its(s_idata_valid and props_in.input_mask(1))) else (others => '0');
  s_port_data <=  unsigned(in_in.data(c_half_width-1 downto 0)) and s_port_mask;

  s_mask <= s_prop_mask or s_port_mask ;
  s_data <= s_prop_data or s_port_data;

  -- Use s_dev_gp_mask if the input_mask(2) bit is high
  s_dev_gp_mask <= dev_gp_in.mask and props_in.input_mask(2);
  s_dev_gp_data <= dev_gp_in.data and s_dev_gp_mask;

  -- OR'ing s_mask and s_data with s_dev_gp_mask and s_dev_gp_mask and concatenating
  -- everything together to form mask_data to drive props_out.mask_data
  props_out.mask_data <= s_mask(s_mask'high downto 1) & (s_mask(0) or s_dev_gp_mask) &
                      s_data(s_data'high downto 1) & (s_data(0) or s_dev_gp_data);

  -- Control gpio pins with 3 LSB of s_data
  -- The devsignal from lime_tx, dev_gp_in.data is routed to gpio1, OR'ed with s_data.
  -- It should be noted that even when the Matchstiq-Z1 GP Out device worker is
  -- in the reset state, gpio1 is still controlled by lime_tx. The devsignal interface
  -- is not impacted by the control plane reset.
  gpio1 <= s_data(0) or s_dev_gp_data;
  gpio2 <= s_data(1);
  gpio3 <= s_data(2);

  -- Logic for controlling dev_gp_em_out.enable dev signal for emulator

  gen_em_out:   if its(simulation_p)  generate
    -- Set gp_em_port_enable to control dev_gp_em_out.enable
    -- When data valid and input_mask(1) high, gp_em_port_enable should go high
    gp_em_port_enable <= '1' when (its(s_idata_valid and props_in.input_mask(1))) else '0';

    -- Set gp_em_prop_enable to control dev_gp_em_out.enable
    -- Since different mask_data values are set in -test.xml, want gp_em_prop_enable to go high
    -- only when the property is written and when is_operating and with a valid props_in.input_mask(0)
    -- otherwise it should go low.
    gp_em_prop_enable <= '1' when (its(ctl_in.is_operating and props_in.mask_data_written and props_in.input_mask(0))) else '0';
    dev_gp_em_out.enable <= gp_em_port_enable or gp_em_prop_enable;
  end generate;



end rtl;
