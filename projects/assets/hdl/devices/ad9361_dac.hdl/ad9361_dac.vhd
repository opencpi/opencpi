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

-- THIS FILE WAS ORIGINALLY GENERATED ON Wed Mar 29 10:45:49 2017 EDT
-- BASED ON THE FILE: ad9361_dac.xml
-- YOU *ARE* EXPECTED TO EDIT IT
-- This file initially contains the architecture skeleton for worker: ad9361_dac

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions
use ocpi.util.all; -- needed for slv()
library util; use util.util.all;
architecture rtl of ad9361_dac_worker is
  constant dac_width : positive := 12; -- must not be > 15 due to WSI width
  -- FIFO parameters
  constant fifo_width : positive := (dac_width*2); -- the fifo is just wide enough to feed AD9361 DAC

  -- CTL/WSI Clock domain signals
  signal in_I       : std_logic_vector(15 downto 0);
  signal in_Q       : std_logic_vector(15 downto 0);
  signal wsi_data_I : std_logic_vector(dac_width- 1 downto 0);
  signal wsi_data_Q : std_logic_vector(dac_width- 1 downto 0);
  signal wsi_data   : std_logic_vector(fifo_width - 1 downto 0);
  -- AD9361 TX clock domain signals
  signal dac_data : std_logic_vector(fifo_width - 1 downto 0);
  signal dac_ready : std_logic := '0';
  signal dac_data_I : std_logic_vector(dac_width - 1 downto 0);
  signal dac_data_Q : std_logic_vector(dac_width - 1 downto 0);
begin

  idata_width_32 : if IDATA_WIDTH_p = 32 generate

    -- iqstream w/ DataWidth=32 formats Q in most significant bits, I in least
    -- significant (see OpenCPI_HDL_Development section on Message Payloads vs.
    -- Physical Data Width on Data Interfaces)
    in_Q <= in_in.data(31 downto 16);
    in_I <= in_in.data(15 downto 0);

  end generate idata_width_32;

  -- transform signed Q0.15 to signed Q0.11, taking most significant 12 bits
  -- (and using the 13th bit to round)
  trunc_round_Q : entity work.trunc_round_15_to_12_signed
    port map(
      DIN  => in_Q,
      DOUT => wsi_data_Q);
  trunc_round_I : entity work.trunc_round_15_to_12_signed
    port map(
      DIN  => in_I,
      DOUT => wsi_data_I);

  wsi_data <= wsi_data_Q & wsi_data_I;

  fifo : util.util.dac_fifo
    generic map(width     => fifo_width,
                depth     => to_integer(fifo_depth))
    port map   (clk       => ctl_in.clk,
                reset     => ctl_in.reset,
                operating => ctl_in.is_operating,
                wsi_ready => in_in.ready,
                wsi_valid => in_in.valid,
                wsi_data  => wsi_data,
                clear     => props_in.underrun_written,
                wsi_take  => in_out.take,
                underrun  => props_out.underrun,
                dac_clk   => dev_dac_in.dac_clk,
                dac_reset => open,
                dac_take  => dev_dac_in.dac_take,
                dac_ready => dac_ready,
                dac_data  => dac_data);

  dev_dac_out.dac_ready <= dac_ready;

  dac_data_Q <= dac_data((dac_width*2)-1 downto dac_width);
  dac_data_I <= dac_data(dac_width-1     downto 0        );

  -- TX zeros when starved for input data
  dev_dac_out.dac_data_Q <= dac_data_Q when dac_ready='1' else (others => '0');
  dev_dac_out.dac_data_I <= dac_data_I when dac_ready='1' else (others => '0');

  dev_dac_out.present <= '1';

end rtl;
