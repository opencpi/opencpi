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

-- Lime DAC worker
library IEEE, ocpi;
use IEEE.std_logic_1164.all, ieee.numeric_std.all, ocpi.types.all, ocpi.util.all;
library util; use util.util.all;
library misc_prims; use misc_prims.misc_prims.all;
architecture rtl of lime_dac_worker is
  constant dac_width : positive := 12; -- must not be > 15 due to WSI width
  -- FIFO parameters
  constant fifo_width : natural := 24; -- the fifo is just wide enough to feed lime DAC
  -- CTL/WSI Clock domain signals
  signal in_I         : std_logic_vector(15 downto 0);
  signal in_Q         : std_logic_vector(15 downto 0);
  signal wsi_data_I   : std_logic_vector(dac_width- 1 downto 0);
  signal wsi_data_Q   : std_logic_vector(dac_width- 1 downto 0);
  signal wsi_data     : std_logic_vector(fifo_width - 1 downto 0);
  signal div_ctl_clk  : std_logic;
  -- Lime DAC (TX) clock domain signals
  signal dac_clk      : std_logic; -- clock sent to lime tx (if connected)
  signal not_dac_clk  : std_logic;
  signal dac_clk_buf  : std_logic;
  signal dac_data     : std_logic_vector(fifo_width - 1 downto 0);
  signal dac_ready    : bool_t;    -- fifo has data for DAC
  signal dac_take     : bool_t;    -- take data from FIFO
  signal sel_iq_r     : bool_t := '0';    -- true for I, false for Q, used locally
  signal event_port_connected         : std_logic := '0';
  signal event_pending                : std_logic := '0';
  signal wsi_event_in_opcode_on_off   : std_logic := '0';
  signal event_in_out_take            : std_logic := '0';
  signal ready_for_event_in_port_data : std_logic := '0';
  signal done_flag                    : std_logic := '0';
  signal event_in_clk                 : std_logic := '0';
  signal event_in_reset               : std_logic := '0';
begin
  --TODO: Replace ctl clock with port clock once AV-65 is resolved 
  event_in_clk <= ctl_in.clk;
  event_in_reset <= ctl_in.reset;
  --------------------------------------------------------------------------------
  -- DAC Sample Clock choices
  --------------------------------------------------------------------------------
  -- 1. We get a copy of what is wired to the lime's TX_CLK input, and use that.
  -- 2. We get a clock that *we* are supposed to drive to the lime TX_CLK, and use that.
  -- 4. We divide-down the control clock, use it, and drive TX_CLK
  -- 5. We use a container clock, use that, and drive TX_CLK
  dac_clk <= TX_CLK_IN when its(USE_CLK_IN_p) else
             ctl_in.clk when its(USE_CTL_CLK_p) else
             sample_clk;

  -- Negate clock in an attempt to better align with TX IQ interface diagram
  -- Figure 10 in LMS6002Dr2-DataSheet-1.2r0.pdf
  not_dac_clk <= not dac_clk;

  TX_CLK <= dac_clk when its(DRIVE_CLK_p) else '0';

  -- Good practice to place on global clock lines
  buf_dac : buffer_clock_global
    port map(
      clk => not_dac_clk,
      clk_buffered => dac_clk_buf
      );

  -- Determined that negation of I/Q Select was necessary for
  -- currently supported platforms (Matchstiq-Z1 and Zipper(Zed))
  -- In the future, it should be managed within the Lime transceiver,
  -- as there is existing circuitry to: swap polarity of I/Q select,
  -- the ordering of I/Q and clock edges. But due to the amount of changes
  -- currently happening to the Apps and Proxies O' Proxies, adding a
  -- negation here had far less impact.
  TX_IQ_SEL <= not sel_iq_r;

  -- Output/lime clock domain processing. 
  -- Mux I and Q onto TXD, consistent with the TX_IQ_SEL output to lime
  -- TXD set to 0s when dac_fifo not ready
  TXD       <= (others => '0')          when not its(dac_ready) else
               dac_data(11 downto 0)    when its(sel_iq_r) else
               dac_data(23 downto 12);

  gen_iq_sel : process(dac_clk_buf)
  begin
    if rising_edge(dac_clk_buf) then
      sel_iq_r <= not sel_iq_r;
    end if;
  end process;     

  -- Take I/Q sample from FIFO when I/Q select 'low' and data is available (i.e. FIFO not Empty)
  dac_take  <= not sel_iq_r and dac_ready;

  idata_width_32 : if IDATA_WIDTH_p = 32 generate
  -- iqstream w/ DataWidth=32 formats Q in most significant bits, I in least
  -- significant (see OpenCPI_HDL_Development section on Message Payloads vs.
  -- Physical Data Width on Data Interfaces)
  in_Q <= in_in.data(31 downto 16);
  in_I <= in_in.data(15 downto 0);

  end generate idata_width_32;

  -- Transform signed Q0.15 to signed Q0.11, taking most significant 12 bits
  -- (and using the 13th bit to round)
  trunc_round_Q : entity work.trunc_round_16_to_12_signed
    port map(
      DIN  => in_Q,
      DOUT => wsi_data_Q);
  trunc_round_I : entity work.trunc_round_16_to_12_signed
    port map(
      DIN  => in_I,
      DOUT => wsi_data_I);

  wsi_data <= wsi_data_Q & wsi_data_I;

  --wsi_data <= slv(signed(in_in.data(31 downto 20)) + signed(in_in.data(19 downto 19))) &
--              slv(signed(in_in.data(15 downto 4))  + signed(in_in.data(3 downto 3)));

  -- CDC (Control to Lime DAC) FIFO
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
                dac_clk   => dac_clk_buf,
                dac_reset => open,
                dac_take  => dac_take,
                dac_ready => dac_ready,
                dac_data  => dac_data);

  -- lime_tx.hdl reports back when it is done processing an event. The done flag
  -- must be registered in the case that it occurs when ready_for_event_in_port_data
  -- is not set yet
  done_flag_proc : process(ctl_in.clk)
  begin
    if rising_edge(ctl_in.clk) then
      if ctl_in.reset = '1' then
        done_flag <= '0';
      else
        if event_in_out_take = '1' then                        --clear on take
          done_flag <= '0';
        elsif dev_tx_event_in.done_processing_event = '1' then --set on done
          done_flag <= '1';
        end if;
      end if;
    end if;
  end process;

  --Event port activity (connected,pending,opcode) must be conveyed to lime_tx.hdl
  dev_tx_event_out.connected <= event_port_connected;
  
  event_pending <= ctl_in.is_operating and event_in_in.ready;
  dev_tx_event_out.event_pending <= event_pending and not done_flag;
  
  wsi_event_in_opcode_on_off <= '1' when (event_in_in.opcode = tx_event_txOn_op_e) else '0';
  dev_tx_event_out.opcode <= wsi_event_in_opcode_on_off;
  
  -- Taking an event requires
  -- 1. Port is connected
  -- 2. Event pending (port ready and operating)
  -- 3. lime_tx.hdl is not busy other non-event SPI transaction (always not
  --    busy for pin control)
  -- 4. Counter to space out TX events signals it is ready
  -- 5. lime_tx.hdl is done processing the previous event (in case of SPI
  --    control, always done for pin_control)
  event_in_out_take <= event_port_connected and
                       event_pending and
                       not dev_tx_event_in.busy and
                       ready_for_event_in_port_data and
                       done_flag;
  event_in_out.take <= event_in_out_take;

  -- this is necessary because we have to synchronize tx events from control
  -- plane clock domain to DAC clock domain (was decided to use two
  -- flip-flop synchronizer, which necessitates ensuring tx events only occur at
  -- most at 1.5X the DAC clock rate
  -- the 
  counter_to_space_out_tx_events : process(ctl_in.clk)
    -- props.in-min_num_cp_clks_per_txen_events is UShort_t
    constant init_val : UShort_t := to_ushort(1);
    variable count : UShort_t := to_ushort(1);
  begin
    if rising_edge(ctl_in.clk) then
      if ctl_in.reset = '1' then
        count := init_val;
        ready_for_event_in_port_data <= '1';
      elsif count < props_in.min_num_cp_clks_per_txen_events then
        if count = init_val then
          if event_in_in.ready = '1' then
            count := count + 1;

            -- applies backpressure to event_in
            ready_for_event_in_port_data <= '0';

          end if;
        else -- count > 1
          count := count + 1;
        end if;
      else -- count >= props_in.min_num_cp_clks_per_txen_events
        -- note this will be always be reached when
        -- props_in.min_num_cp_clks_per_txen_events is 0 or 1
        ready_for_event_in_port_data <= '1';
        --reset counter on take
        if event_in_out_take = '1' then
          count := init_val;
        end if;
      end if;
    end if;
  end process counter_to_space_out_tx_events;

  event_in_to_txen : misc_prims.misc_prims.event_in_to_txen
    port map (event_in_clk           => event_in_clk,
              event_in_reset         => event_in_clk,
              ctl_in_is_operating    => ctl_in.is_operating,
              event_in_in_reset      => event_in_in.reset,
              event_in_in_som        => event_in_in.som,
              event_in_in_valid      => event_in_in.valid,
              event_in_in_eom        => event_in_in.eom,
              event_in_in_ready      => event_in_in.ready,
              event_in_out_take      => event_in_out_take,
              event_in_opcode_on_off => wsi_event_in_opcode_on_off,
              txon_pulse             => open,
              txoff_pulse            => open,
              txen                   => dev_txen_out.txen,
              event_in_connected     => event_port_connected,
              is_operating           => open);

end rtl;
