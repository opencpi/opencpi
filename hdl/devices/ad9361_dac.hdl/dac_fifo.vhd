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

-- A synchronizing FIFO with underrun detection, driven by WSI.
-- Underrun is reported (and cleared) in the WSI clock domain
-- We completely ignore message boundaries here.
-- FIXME: enable SRL FIFOs
library IEEE, ocpi, bsv;
use IEEE.std_logic_1164.all, ieee.numeric_std.all, ocpi.types.all, ocpi.util.all;
entity dac_fifo is
  generic (width : positive := 32;
           depth : positive := 64);
  port (-- WSI side signals
        clk       : std_logic;
        reset     : in bool_t;
        operating : in bool_t;
        wsi_ready : in bool_t;
        wsi_valid : in bool_t;
        wsi_data  : in std_logic_vector(width-1 downto 0);
        clear     : in bool_t;  -- pulse to clean underrun
        wsi_take  : out bool_t;
        underrun  : out bool_t; -- sticky underrun indication
        -- DAC side signals
        dac_clk   : in std_logic;
        dac_take  : in bool_t;
        dac_ready : out bool_t;
        dac_data  : out std_logic_vector(width-1 downto 0));
end entity dac_fifo;
architecture rtl of dac_fifo is
  signal seen_data_r      : bool_t;    -- data seen since operating
  signal underrun_event   : std_logic; -- momentary determination of underrun on DAC side
  signal dac_reset_n      : std_logic; -- inverted reset in dac clk domain
  signal fifo_s_reset_n   : std_logic; -- inverted reset in wsi clock domain
  signal fifo_s_enq       : std_logic;
  signal fifo_s_not_full  : std_logic;
  signal fifo_d_not_empty : std_logic;
begin
  -- Underrun detection: the fifo is empty after is_operation and it has been not empty.
  dac_ready <= fifo_d_not_empty;

  -- Processing in the WSI clock domain.  Track whether ANY data has arrived, set underrun
  wsi_take <= operating and wsi_ready and (not wsi_valid or fifo_s_not_full);
  
  -- Synchronizing FIFO from the WSI input clock domain to the DAC clock domain
  fifo_s_reset_n <= not reset;
  fifo_s_enq     <= wsi_ready and operating and wsi_valid and fifo_s_not_full;
  fifo : bsv.bsv.SyncFIFO
    generic map (dataWidth => width,
                 depth     => depth,
                 indxWidth => width_for_max(depth-1))
    port map (   sCLK      => clk,
                 sRST      => fifo_s_reset_n,
                 sENQ      => fifo_s_enq,
                 sD_IN     => wsi_data,
                 sFULL_N   => fifo_s_not_full,
                 dCLK      => dac_clk,
                 dDEQ      => dac_take,
                 dEMPTY_N  => fifo_d_not_empty,
                 dD_OUT    => dac_data);

  underrun_event <= not fifo_d_not_empty;
  status : entity work.sync_status
    port map   (clk         => clk,
                reset       => reset,
                operating   => operating,
                start       => fifo_s_enq,
                clear       => clear,
                status      => underrun,
                other_clk   => dac_clk,
                other_reset => open, -- dac_reset,
                event       => underrun_event);

end rtl;
