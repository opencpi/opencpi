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

-- A synchronizing FIFO with overrun, feeding a WSI - e.g. from an ADC
-- Overrun is reported (and cleared) in the WSI clock domain
-- Should be used in all ADC workers
-- FIXME: enable SRL FIFOs
library IEEE, ocpi, bsv, util;
use IEEE.std_logic_1164.all, ieee.numeric_std.all, ocpi.types.all, ocpi.util.all;
entity adc_fifo is
  generic (width : positive := 32;
           depth : positive := 16);
  port (-- WSI side signals for WSI input, in its clk domain
        clk         : in  std_logic;
        reset       : in  bool_t;
        operating   : in  bool_t;
        wsi_ready   : in  bool_t;
        wsi_give    : out bool_t;
        wsi_valid   : out bool_t;
        wsi_som     : out bool_t;
        wsi_eom     : out bool_t;
        wsi_data    : out std_logic_vector(width-1 downto 0);
        clear       : in  bool_t;  -- pulse to clean underrun
        overrun     : out bool_t;  -- sticky underrun indication
        messageSize : in  ulong_t;
        -- ADC side signals in the adc_clock domain
        adc_clk     : in  std_logic;
        adc_give    : in  std_logic; -- qualifier to take adc data into FIFO
        adc_data    : in  std_logic_vector(width-1 downto 0));
end entity adc_fifo;
architecture rtl of adc_fifo is
  -- Signals in WSI clock domain
  signal fifo_d_not_empty   : std_logic;
  signal fifo_d_deq         : std_logic;
  signal samplesInMessage_r : ulong_t;
  -- Signals in ADC clock domain
  signal adc_reset          : bool_t;
  signal fifo_s_reset_n     : std_logic;
  signal overrun_event      : bool_t; -- momentary determination of overrun on ADC side
  signal fifo_s_enq         : std_logic;
  signal fifo_s_not_full    : std_logic;
begin
  -- when FIFO has data to give, ALWAYS dequeue (unless we're operating and wsi is not ready),
  -- note we will dequeue even when not operating (to alleviate forward pressure from ADC),
  --fifo_d_deq <= wsi_ready and operating and fifo_d_not_empty;
  fifo_d_deq <= (not (operating and not wsi_ready)) and fifo_d_not_empty;

  -- sample was dequeued last clock and in operating state this clock
  --wsi_valid  <= fifo_d_not_empty;
  wsi_valid  <= fifo_d_deq and operating;

 -- sample was dequeued last clock and operating this clock and output port will accept it this clock
  --wsi_give   <= fifo_d_deq;
  wsi_give   <= fifo_d_deq and operating and wsi_ready;

  wsi_som    <= to_bool(samplesInMessage_r = 1);
  wsi_eom    <= to_bool(samplesInMessage_r = (messageSize srl 2));

  count: process(clk)
  begin
    if rising_edge(clk) then
      if reset or (samplesInMessage_r = (messageSize srl 2) and fifo_d_deq) then
        samplesInMessage_r <= (0 => '1', others => '0');
      elsif (fifo_d_deq = '1') and (operating = '1') and (wsi_ready = '1') then
        samplesInMessage_r <= samplesInMessage_r + 1;
      end if;
    end if;
  end process;


  -- Synchronizing FIFO from the WSI input clock domain to the ADC clock domain
  fifo_s_reset_n <= not adc_reset;

  -- when data is available ALWAYS enqueue (unless FIFO is full)
  --fifo_s_enq     <= fifo_s_not_full and adc_give and operating;
  fifo_s_enq     <= fifo_s_not_full and adc_give;

  fifo : bsv.bsv.SyncFIFO
    generic map (dataWidth => width,
                 depth     => depth,
                 indxWidth => width_for_max(depth-1))
    port map (   sCLK      => adc_clk,
                 sRST      => fifo_s_reset_n,
                 sENQ      => fifo_s_enq,
                 sD_IN     => adc_data,
                 sFULL_N   => fifo_s_not_full,
                 dCLK      => clk,
                 dDEQ      => fifo_d_deq,
                 dEMPTY_N  => fifo_d_not_empty,
                 dD_OUT    => wsi_data);

  overrun_event <= not fifo_s_not_full;
  status : util.util.sync_status
    port map   (clk         => clk,
                reset       => reset,
                operating   => operating,
                start       => fifo_d_deq,
                clear       => clear,
                status      => overrun,
                other_clk   => adc_clk,
                other_reset => adc_reset,
                event       => overrun_event);
end rtl;
