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

-- THIS FILE WAS ORIGINALLY GENERATED ON Wed Apr 12 12:20:38 2017 EDT
-- BASED ON THE FILE: ad9361_dac_sub.xml
-- YOU *ARE* EXPECTED TO EDIT IT
-- This file initially contains the architecture skeleton for worker: ad9361_dac_sub

-- ***** NOTE: the TX_FRAME_P and TX data bus lines have setup/hold requirements
-- as specified in the ADI UG-570 document. This worker does not include any
-- ODELAY lines. Note also that the AD9361 registers affect setup/hold
-- requirements. Please ensure that the proper constraints are in place (if
-- applicable) and that the timing report for the generated bitstream meets the
-- AD9361 TX data bus specifications.
library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions
use ocpi.util.all;
library bsv; use bsv.bsv.all;
library unisim; use unisim.vcomponents.all;
architecture rtl of ad9361_dac_sub_worker is
  constant dac_width  : positive := 12; -- must not be > 15 due to WSI width, must be multiple of 2 due to LVDS generate loop

  -- we could expose these constants as bool parameters with default values
  -- of false, but there's nothing significant to gain from it, so we fix their
  -- values to false here (if someone wanted to change it to true for
  -- whatever reason, it passes this info up to config worker via dev signals,
  -- which in turn passes those dev signals up to the config proxy, which uses
  -- those dev signals to ensure that the corresponding registers are set
  -- properly)
  constant data_bus_bits_are_reversed    : bool_t := bfalse;

  -- Determine the data_width based on the mode.
  -- While the data-width to the adc/dac will be the same,
  -- the number of pins connected (via the data_sub) will
  -- be different depending on the current mode.
  function data_width (dac_width : positive)
    return positive is
  begin
    if (HALF_DUPLEX_p       = bfalse  and
        DATA_RATE_CONFIG_p  = DDR_e)  and
       ((LVDS_p             = btrue   and
         SINGLE_PORT_p      = bfalse) or
        (LVDS_p             = bfalse  and
         SINGLE_PORT_p      = btrue)) then
      return dac_width/2;
    elsif (LVDS_p             = bfalse   and
           DATA_RATE_CONFIG_p = DDR_e)   and
          ((SINGLE_PORT_p     = btrue    and
            HALF_DUPLEX_p     = btrue)   or
           (SINGLE_PORT_p     = bfalse   and
            HALF_DUPLEX_p     = bfalse)) then
      return dac_width;
    else
      return dac_width*2;
    end if;
  end function data_width;

  constant data_width_from_pins : positive := data_width(dac_width);

  -- no clock domain / static signals
  signal ch0_worker_present : std_logic := '0';
  signal ch1_worker_present : std_logic := '0';
  -- WSI (control) domain signals
  signal wsi_dual_port   : std_logic := '0';
  signal wsi_full_duplex : std_logic := '0';
  signal wsi_data_rate   : std_logic := '0';
  signal wsi_clear       : std_logic := '0';
  signal wsi_reset_n     : std_logic := '1';
  signal wsi_use_two_r_two_t_timing : std_logic := '0';
  signal wsi_txen                   : std_logic := '0';
  -- AD9361 TX clock divided by 2 clock domain signals
  signal dacd2_clk              : std_logic := '0';

  -- manually duplicate these register to aid in timing
  signal dacd2_processing_ch0   : std_logic_vector(dac_width-1 downto 0) := (others => '1');
  signal dacd2_processing_ch0_r : std_logic_vector(dac_width-1 downto 0) := (others => '1');

  signal dacd2_ch0_i_r          : std_logic_vector(dac_width-1 downto 0) := (others => '0');
  signal dacd2_ch0_q_r          : std_logic_vector(dac_width-1 downto 0) := (others => '0');
  signal dacd2_ch1_i_r          : std_logic_vector(dac_width-1 downto 0) := (others => '0');
  signal dacd2_ch1_q_r          : std_logic_vector(dac_width-1 downto 0) := (others => '0');
  signal dacd2_i_r              : std_logic_vector(dac_width-1 downto 0) := (others => '0');
  signal dacd2_q_r              : std_logic_vector(dac_width-1 downto 0) := (others => '0');
  signal dacd2_use_two_r_two_t_timing_rr : std_logic := '0';
  signal dacd2_num_channels_config : std_logic_vector(1 downto 0) := (others => '0'); -- 00 : 1R1T
                                                                                      -- 01 : 1R2T
                                                                                      -- 10 : 2R1T
                                                                                      -- 11 : 2R2T
  signal dacd2_tx_frame_ddr_first_r : std_logic := '0';
  signal dacd2_tx_frame_ddr_second_r : std_logic := '1';
  -- AD9361 TX clock domain signals
  signal dac_clk           : std_logic := '0';
  signal dac_data_ddr_first_r   : std_logic_vector(data_width_from_pins-1 downto 0) := (others => '0');
  signal dac_data_ddr_second_r  : std_logic_vector(data_width_from_pins-1 downto 0) := (others => '0');
  signal dac_start_frame_r : std_logic := '0';
  -- AD9361 TX clock domain x2 signals (i.e. TX clock w/ DDR data)
  signal dacm2_tx_data_r     : std_logic_vector(dac_width-1 downto 0);
  -- signals interface (DAC clock domain)
  signal TX_FRAME_P_s : std_logic := '0';

  attribute equivalent_register_removal : string;
  attribute equivalent_register_removal of dacd2_processing_ch0 : signal is "no";
  attribute equivalent_register_removal of dacd2_processing_ch0_r : signal is "no";
begin

  -- these dev signals are used to (eventually) tell higher level proxy(ies)
  -- about the data port configuration that was enforced when this worker was
  -- compiled, so that said proxy(ies) can set the AD9361 registers accordingly
  wsi_dual_port   <= '1' when SINGLE_PORT_p      = bfalse        else '0';
  wsi_full_duplex <= '1' when HALF_DUPLEX_p      = bfalse        else '0';
  wsi_data_rate   <= '1' when DATA_RATE_CONFIG_p = DDR_e         else '0';
  dev_cfg_data_out.ch0_handler_is_present   <= ch0_worker_present;
  dev_cfg_data_out.ch1_handler_is_present   <= ch1_worker_present;
  dev_cfg_data_out.data_bus_index_direction <= '1' when data_bus_bits_are_reversed = btrue  else '0';
  dev_cfg_data_out.data_clk_is_inverted     <= '0'; -- this worker really doesn't care if DATA_CLK is inverted
  dev_cfg_data_out.islvds                   <= '1' when LVDS_p                     = btrue  else '0';
  dev_cfg_data_out.isdualport               <= '1' when LVDS_p                     = btrue  else wsi_dual_port;
  dev_cfg_data_out.isfullduplex             <= '1' when LVDS_p                     = btrue  else wsi_full_duplex;
  dev_cfg_data_out.isddr                    <= '1' when LVDS_p                     = btrue  else wsi_data_rate;
  dev_cfg_data_out.present <= '1';

  -- ch0_worker_preset should always be '1' since we are it's subdevice
  ch0_worker_present <= dev_data_ch0_in_in.present;
  ch1_worker_present <= dev_data_ch1_in_in.present;

  -- ******* THIS OPENCPI BUILD CONFIGURATION IS NOT SUPPORTED BY *******
  -- ******* AD9361 AND SHOULD NEVER BE USED *******
  data_mode_invalid : if (LVDS_p = btrue) and
                         ((SINGLE_PORT_p      = btrue) or
                          (HALF_DUPLEX_p      = btrue) or
                          (DATA_RATE_CONFIG_p = SDR_e)) generate
  begin
    dacm2_tx_data_r <= (others => '0');

    -- we normally would force unsuccessful end of all control operations here
    -- but we can't since this worker has no control plane
    --ctl_out.error <= '1';

  end generate;

  -- ******* ALL CMOS-SPECIFIC DAC DATA INTERLEAVING HDL CODE IS *******
  -- ******* INTENDED TO EXIST WITHIN THIS IF/GENERATE           *******
  data_mode_cmos : if LVDS_p = bfalse generate
  begin

    -- TODO / FIXME support runtime dynamic enumeration for CMOS? (if so, we need to check duplex_config = runtime_dynamic)

    single_port_fdd_ddr : if(SINGLE_PORT_p = btrue) and
                            (HALF_DUPLEX_p = bfalse) and
                            (DATA_RATE_CONFIG_p = DDR_e) generate
      single_port_fdd_ddr_i : entity work.ad9361_dac_sub_cmos_single_port_fdd_ddr
      generic map(
        data_bus_bits_are_reversed => data_bus_bits_are_reversed = btrue)
      port map(
        wci_clk                      => wci_clk,
        wci_reset                    => wci_reset,
        wci_config_is_two_r          => dev_cfg_data_in.config_is_two_r,
        wci_config_is_two_t          => dev_cfg_data_tx_in.config_is_two_t,
        wci_force_two_r_two_t_timing => dev_cfg_data_tx_in.force_two_r_two_t_timing,
        dac_clk                      => dev_data_clk_in.DATA_CLK_P,
        dac_data_i_ch0               => dev_data_ch0_in_in.dac_data_I,
        dac_data_i_ch1               => dev_data_ch1_in_in.dac_data_I,
        dac_data_q_ch0               => dev_data_ch0_in_in.dac_data_Q,
        dac_data_q_ch1               => dev_data_ch1_in_in.dac_data_Q,
        dac_dev_data_ch0_in_in_ready => dev_data_ch0_in_in.dac_ready,
        dac_dev_data_ch1_in_in_ready => dev_data_ch1_in_in.dac_ready,
        dac_tx_frame                 => TX_FRAME_P_s,
        dac_dev_data_ch0_in_out_clk  => dev_data_ch0_in_out.dac_clk,
        dac_dev_data_ch1_in_out_clk  => dev_data_ch1_in_out.dac_clk,
        dac_dev_data_ch0_in_out_take => dev_data_ch0_in_out.dac_take,
        dac_dev_data_ch1_in_out_take => dev_data_ch1_in_out.dac_take,
        dacm2_dev_data               => dev_data_to_pins_out.data);

    end generate single_port_fdd_ddr;

  end generate data_mode_cmos;

  -- ******* ALL LVDS-SPECIFIC DAC DATA INTERLEAVING HDL CODE EXISTS *******
  -- ******* WITHIN THIS IF/GENERATE                                 *******
  data_mode_lvds : if (LVDS_p = btrue) and
                      (SINGLE_PORT_p       = bfalse) and
                      (HALF_DUPLEX_p       = bfalse) and
                      (DATA_RATE_CONFIG_p  = DDR_e) generate
  begin

    dac_clk <= dev_data_clk_in.DATA_CLK_P;

    BUFR_inst : BUFR
    generic map (
       BUFR_DIVIDE => "2")   -- "BYPASS", "1", "2", "3", "4", "5", "6", "7", "8"
    port map (
       O => dacd2_clk,   -- 1-bit output: Clock output port
       CE => '1',        -- 1-bit input: Active high, clock enable (Divided modes only)
       CLR => '0',       -- 1-bit input: Active high, asynchronous clear (Divided mode only)
       I => dac_clk      -- 1-bit input: Clock buffer input driven by an IBUFG, MMCM or local interconnect
    );

    dev_data_ch0_in_out.dac_clk <= dacd2_clk;
    dev_data_ch1_in_out.dac_clk <= dacd2_clk;

    -- From ADI's UG-570:
    -- "For a system with a 2R1T or a 1R2T configuration, the clock
    -- frequencies, bus transfer rates and sample periods, and data
    -- capture timing are the same as if configured for a 2R2T system.
    -- However, in the path with only a single channel used, the
    -- disabled channel’s I-Q pair in each data group is unused."
    wsi_use_two_r_two_t_timing <= dev_cfg_data_in.config_is_two_r or
                                  dev_cfg_data_tx_in.config_is_two_t or
                                  dev_cfg_data_tx_in.force_two_r_two_t_timing;

    wsi_reset_n <= not wci_reset;
    -- sync (WSI clock domain) -> (DAC clock divided by 2
    -- domain), note that we don't care if WSI clock is much faster and bits are
    -- dropped - wsi_use_two_r_two_t_timing is a configuration bit which is
    -- expected to change very rarely in relation to either clock
    second_chan_enable_sync : bsv.bsv.SyncBit
      generic map(
        init   => 0)
      port map(
        sCLK   => wci_clk,
        sRST   => wsi_reset_n, -- apparently sRST is active-low
        dCLK   => dacd2_clk,
        sEN    => '1',
        sD_IN  => wsi_use_two_r_two_t_timing,
        dD_OUT => dacd2_use_two_r_two_t_timing_rr); -- delayed by one WSI clock, two DACD2 clocks

    -- take is delayed version of ready to ensure that there is always at least
    -- 1 sample in the CDC FIFO (prevents possibility of CDC FIFO underrun upon
    -- startup)
    take_regs : process(dacd2_clk)
    begin
      if rising_edge(dacd2_clk) then
        -- chose dacd2_processing_ch0 idx 0 but we could have done any idx
        dev_data_ch0_in_out.dac_take <= dev_data_ch0_in_in.dac_ready and
                                     dacd2_processing_ch0(0);
        dev_data_ch1_in_out.dac_take <= dev_data_ch1_in_in.dac_ready and
                                     (not dacd2_processing_ch0(0));
      end if;
    end process;

    -- needed loop because we're manually duplicating dacd2_processing_ch0 reg
    -- to aid in timing
    reg_duplication_loop : for idx in (dac_width-1) downto 0 generate
      chan_select_regs : process(dacd2_clk)
      begin
        if rising_edge(dacd2_clk) then
          dacd2_processing_ch0(idx) <= (not dacd2_processing_ch0(idx)) or
                                       (not dacd2_use_two_r_two_t_timing_rr);
          dacd2_processing_ch0_r(idx) <= dacd2_processing_ch0(idx);
        end if;
      end process;

      dacd2_ch0_regs : process(dacd2_clk)
      begin
        if rising_edge(dacd2_clk) then
          if (dev_data_ch0_in_in.dac_ready = '1') and (dacd2_processing_ch0(idx) = '1') then
            dacd2_ch0_i_r(idx) <= dev_data_ch0_in_in.dac_data_I(idx);
            dacd2_ch0_q_r(idx) <= dev_data_ch0_in_in.dac_data_Q(idx);
          end if;
        end if;
      end process;

      dacd2_ch1_regs : process(dacd2_clk)
      begin
        if rising_edge(dacd2_clk) then
          if (dev_data_ch1_in_in.dac_ready = '1') and (dacd2_processing_ch0(idx) = '0') then
            dacd2_ch1_i_r(idx) <= dev_data_ch1_in_in.dac_data_I(idx);
            dacd2_ch1_q_r(idx) <= dev_data_ch1_in_in.dac_data_Q(idx);
          end if;
        end if;
      end process;

      -- channel serialization mux (chan 0 one dacd2_clk, chan 1 next dacd2_clk)
      dacd2_i_r(idx) <= dacd2_ch0_i_r(idx) when
                        (dacd2_processing_ch0_r(idx) = '1') else
                        dacd2_ch1_i_r(idx);
      dacd2_q_r(idx) <= dacd2_ch0_q_r(idx) when
                        (dacd2_processing_ch0_r(idx) = '1') else
                        dacd2_ch1_q_r(idx);
    end generate;

    -- 12-bit word-> 6-bit word serialization registers (high 6 bits one
    -- dac_clk, low 6 bits next dac_clk)
    word_serial_regs : process(dac_clk)
    begin
      if rising_edge(dac_clk) then
        if dacd2_clk = '1' then
          dac_data_ddr_first_r  <= dacd2_i_r(dac_width-1 downto dac_width/2);
          dac_data_ddr_second_r <= dacd2_q_r(dac_width-1 downto dac_width/2);
        else
          dac_data_ddr_first_r  <= dacd2_i_r((dac_width/2)-1 downto 0);
          dac_data_ddr_second_r <= dacd2_q_r((dac_width/2)-1 downto 0);
        end if;
      end if;
    end process;

    tx_frame_toggle_reg : process(dacd2_clk)
    begin
      if rising_edge(dacd2_clk) then
        if dacd2_use_two_r_two_t_timing_rr = '1' then
          -- chose dacd2_processing_ch0 idx 0 but we could have done any idx
          dacd2_tx_frame_ddr_first_r  <= dacd2_processing_ch0(0);
          dacd2_tx_frame_ddr_second_r <= dacd2_processing_ch0(0);
        else
          dacd2_tx_frame_ddr_first_r  <= '1';
          dacd2_tx_frame_ddr_second_r <= '0';
        end if;
      end if;
    end process;

    -- this primitive skews tx frame by a similar amount as the
    -- data, which minimizes overall TX_D/TX_FRAME bus skew, use of this
    -- primitive also allows for tx frame to be driven by
    -- dacd2_tx_frame_high and dacd2_tx_frame_low which are in the slower
    -- clock domain, which aids in timing closure
    tx_frame_ddr : ODDR
    generic map(
      DDR_CLK_EDGE => "SAME_EDGE",
      INIT         => '0',
      SRTYPE       => "ASYNC")
    port map(
      Q  => TX_FRAME_P_s,
      C  => dacd2_clk,
      CE => '1',
      D1  => dacd2_tx_frame_ddr_first_r,
      D2  => dacd2_tx_frame_ddr_second_r,
      R  => '0',
      S  => '0');

    -- I/Q serialization (I rising edge, Q falling edge)
    buf_data_loop : for idx in ((dac_width/2)-1) downto 0 generate
    begin
      data_ddr : ODDR
      generic map(
        DDR_CLK_EDGE => "SAME_EDGE",
        INIT         => '0',
        SRTYPE       => "ASYNC")
      port map(
        Q  => dacm2_tx_data_r(idx),
        C  => dac_clk,
        CE => '1',
        D1  => dac_data_ddr_first_r(idx),
        D2  => dac_data_ddr_second_r(idx),
        R  => '0',
        S  => '0');
    end generate;

    data_bus_bits_are_not_reversed : if data_bus_bits_are_reversed = bfalse generate
      dev_data_to_pins_out.data(dev_data_to_pins_out.data'left downto dac_width/2) <= (others => '0');
      buf_data : for idx in ((dac_width/2)-1) downto 0 generate
        dev_data_to_pins_out.data(idx) <= dacm2_tx_data_r(idx);
      end generate;
    end generate;
    data_bus_bits_are_reversed_t : if data_bus_bits_are_reversed = btrue generate
      dev_data_to_pins_out.data(dev_data_to_pins_out.data'left downto dac_width/2) <= (others => '0');
      buf_data : for idx in ((dac_width/2)-1) downto 0 generate
        dev_data_to_pins_out.data(idx) <= dacm2_tx_data_r((dac_width/2)-1-idx);
      end generate;
    end generate;

  end generate;

  -- delegate to the data_sub.
  dev_data_to_pins_out.tx_frame <= TX_FRAME_P_s;

  event_in_x2_to_txen : entity work.event_in_x2_to_txen
    port map (ctl_in_clk           => wci_clk,
              ctl_in_reset         => wci_reset,
              txon_pulse_0         => dev_tx_event_ch0_in.txon_pulse,
              txoff_pulse_0        => dev_tx_event_ch0_in.txoff_pulse,
              event_in_connected_0 => dev_tx_event_ch0_in.event_in_connected,
              is_operating_0       => dev_tx_event_ch0_in.is_operating,
              txon_pulse_1         => dev_tx_event_ch1_in.txon_pulse,
              txoff_pulse_1        => dev_tx_event_ch1_in.txoff_pulse,
              event_in_connected_1 => dev_tx_event_ch1_in.event_in_connected,
              is_operating_1       => dev_tx_event_ch1_in.is_operating,
              txen                 => wsi_txen);

  dev_txen_out.txen         <= wsi_txen;
end rtl;
