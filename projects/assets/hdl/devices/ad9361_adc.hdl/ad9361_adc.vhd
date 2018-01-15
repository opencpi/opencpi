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

-- THIS FILE WAS ORIGINALLY GENERATED ON Thu Mar 23 18:26:07 2017 EDT
-- BASED ON THE FILE: ad9361_adc.xml
-- YOU *ARE* EXPECTED TO EDIT IT
-- This file initially contains the architecture skeleton for worker: ad9361_adc

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions
library util; use util.util.all;
architecture rtl of ad9361_adc_worker is
  constant adc_width : positive := 12; -- must not be > 16 due to WSI width
  -- FIFO parameters
  constant fifo_width : natural := (adc_width*2); -- the fifo is just wide enough for ADC samples

  -- CTL/WSI clock domain
  signal wsi_data : std_logic_vector((adc_width*2)-1 downto 0);
  -- ADC clock domain
  signal adc_data : std_logic_vector((adc_width*2)-1 downto 0);
begin
  -- Take data out of the FIFO and sign extend each I/Q part
  out_out.data <= (31 downto 16+adc_width => wsi_data((adc_width*2)-1)) & wsi_data((adc_width*2)-1 downto adc_width) &
                  (15 downto adc_width    => wsi_data(adc_width-1))     & wsi_data(adc_width-1 downto 0);
  out_out.byte_enable <= (others => '1');

  -- Temp signals necessary only because of older VHDL restrictions on "actuals".
  adc_data <= dev_adc_in.adc_data_Q & dev_adc_in.adc_data_I;
  fifo : util.util.adc_fifo
    generic map(width       => fifo_width,
                depth       => to_integer(fifo_depth))
    port map   (clk         => ctl_in.clk,
                reset       => ctl_in.reset,
                operating   => ctl_in.is_operating,
                wsi_ready   => out_in.ready,
                wsi_give    => out_out.give,
                wsi_valid   => out_out.valid,
                wsi_som     => out_out.som,
                wsi_eom     => out_out.eom,
                wsi_data    => wsi_data,
                clear       => props_in.overrun_written,
                overrun     => props_out.overrun,
                messageSize => props_in.messageSize,
                -- In ADC clock domain
                adc_clk     => dev_adc_in.adc_clk,
                adc_give    => dev_adc_in.adc_give,
                adc_data    => adc_data);

  dev_adc_out.present <= '1';

end rtl;

