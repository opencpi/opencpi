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
-- BASED ON THE FILE: ad9361_adc_sub.xml
-- YOU *ARE* EXPECTED TO EDIT IT
-- This file initially contains the architecture skeleton for worker: ad9361_adc_sub

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions
library unisim; use unisim.vcomponents.all;
library util; use util.util.all;
library bsv; use bsv.bsv.all;
architecture rtl of ad9361_adc_sub_worker is
  constant adc_width  : positive := 12; -- must not be > 16 due to WSI width and multiple of 2 due to LVDS generate loop

  -- we could expose these constants as bool parameters with default values
  -- of false, but there's nothing significant to gain from it, so we fix their
  -- values to false here (if someone wanted to change it to true for
  -- whatever reason, it passes this info up to config worker via dev signals,
  -- which in turn passes those dev signals up to the config proxy, which uses
  -- those dev signals to ensure that the corresponding registers are set
  -- properly
  constant data_bus_bits_are_reversed    : bool_t := bfalse;
  constant data_clk_p_is_inverted        : bool_t := bfalse;
  constant rx_frame_is_inverted          : bool_t := bfalse;

  constant rx_frame_usage_is_toggle      : bool_t := btrue;  -- no need for us to support non-toggle ("enable")

  type state_t is (R1_11_6, R1_5_0,
                   R2_11_6, R2_5_0);
  signal state : state_t := R1_11_6;

  -- internal signals
  signal dual_port   : std_logic := '0';
  signal full_duplex : std_logic := '0';
  signal data_rate   : std_logic := '0';
  -- no clock domain / static signals
  signal ch0_worker_present : std_logic := '0';
  signal ch1_worker_present : std_logic := '0';
  signal r1_worker_present  : std_logic := '0';
  signal r2_worker_present  : std_logic := '0';
  -- WSI (control) clock domain signals
  signal wsi_r1_samps_dropped_clear : std_logic := '0';
  signal wsi_r2_samps_dropped_clear : std_logic := '0';
  signal wsi_reset_n                : std_logic := '1';
  signal wsi_channels_are_swapped   : std_logic := '0';
  -- AD9361 RX clock domain signals
  signal adc_clk_buf  : std_logic := '0';
  signal adc_clk      : std_logic := '0';
  signal adc_data0    : std_logic_vector((adc_width*2)-1 downto 0) := (others => '0');
  signal adc_data1    : std_logic_vector((adc_width*2)-1 downto 0) := (others => '0');
  signal adc_rx_frame_p_buf    : std_logic := '0';
  signal adc_rx_frame_p_buf_rr : std_logic := '0';
  signal adc_rx_data_buf_ordered : std_logic_vector((adc_width/2)-1 downto 0) := (others => '0');
  signal adc_ddr_out_rising_rr   : std_logic_vector((adc_width/2)-1 downto 0) := (others => '0');
  signal adc_ddr_out_falling_rr  : std_logic_vector((adc_width/2)-1 downto 0) := (others => '0');
  signal adc_r1_i_h_rrr : std_logic_vector((adc_width/2)-1 downto 0) := (others => '0');
  signal adc_r1_q_h_rrr : std_logic_vector((adc_width/2)-1 downto 0) := (others => '0');
  signal adc_r1_i_rrrr  : std_logic_vector(adc_width-1     downto 0) := (others => '0');
  signal adc_r1_q_rrrr  : std_logic_vector(adc_width-1     downto 0) := (others => '0');
  signal adc_r1_i_rrrrr : std_logic_vector(adc_width-1     downto 0) := (others => '0');
  signal adc_r1_q_rrrrr : std_logic_vector(adc_width-1     downto 0) := (others => '0');
  signal adc_r1_i_rrrrrr: std_logic_vector(adc_width-1     downto 0) := (others => '0');
  signal adc_r1_q_rrrrrr: std_logic_vector(adc_width-1     downto 0) := (others => '0');
  signal adc_r2_i_h_rrr : std_logic_vector((adc_width/2)-1 downto 0) := (others => '0');
  signal adc_r2_q_h_rrr : std_logic_vector((adc_width/2)-1 downto 0) := (others => '0');
  signal adc_r2_i_rrrr  : std_logic_vector(adc_width-1     downto 0) := (others => '0');
  signal adc_r2_q_rrrr  : std_logic_vector(adc_width-1     downto 0) := (others => '0');
  signal adc_r1_give_rrrr   : bool_t := bfalse;
  signal adc_r1_give_rrrrr  : bool_t := bfalse;
  signal adc_r1_give_rrrrrr : bool_t := bfalse;
  signal adc_r2_give_rrrr   : bool_t := bfalse;
  signal adc_r1_samps_dropped : std_logic := '0';
  signal adc_r2_samps_dropped : std_logic := '0';
  signal adc_data : std_logic_vector((adc_width/2)-1 downto 0) := (others => '0');
  signal adc_channels_are_swapped : std_logic := '0';
  -- signals interface (ADC clock domain)
  signal RX_FRAME_P_s : std_logic := '0';
begin

  -- these dev signals are used to (eventually) tell higher level proxy(ies)
  -- about the data port configuration that was enforced when this worker was
  -- compiled, so that said proxy(ies) can set the AD9361 registers accordingly
  dual_port   <= '1' when SINGLE_PORT_p      = bfalse        else '0';
  full_duplex <= '1' when HALF_DUPLEX_p      = bfalse        else '0';
  data_rate   <= '1' when DATA_RATE_CONFIG_p = DDR_e         else '0';
  dev_cfg_data_out.ch0_handler_is_present   <= ch0_worker_present;
  dev_cfg_data_out.ch1_handler_is_present   <= ch1_worker_present;
  dev_cfg_data_out.data_bus_index_direction <= '1' when data_bus_bits_are_reversed = btrue else '0';
  dev_cfg_data_out.data_clk_is_inverted     <= '1' when data_clk_p_is_inverted     = btrue else '0';
  dev_cfg_data_out.islvds                   <= '1' when LVDS_p                     = btrue else '0';
  dev_cfg_data_out.isdualport               <= '1' when LVDS_p                     = btrue else dual_port;
  dev_cfg_data_out.isfullduplex             <= '1' when LVDS_p                     = btrue else full_duplex;
  dev_cfg_data_out.isddr                    <= '1' when LVDS_p                     = btrue else data_rate;
  dev_cfg_data_out.present <= '1';
  dev_cfg_data_rx_out.rx_frame_usage        <= '1' when rx_frame_usage_is_toggle   = '1'   else '0';
  dev_cfg_data_rx_out.rx_frame_is_inverted  <= '1' when rx_frame_is_inverted       = btrue else '0';

  -- ch0_worker_preset should always be '1' since we are it's subdevice
  ch0_worker_present <= dev_data_ch0_out_in.present;
  ch1_worker_present <= dev_data_ch1_out_in.present;

  BUFR_inst : BUFR
  generic map (
     BUFR_DIVIDE => "BYPASS")   -- "BYPASS", "1", "2", "3", "4", "5", "6", "7", "8"
  port map (
     O => adc_clk_buf, -- 1-bit output: Clock output port
     CE => '1',        -- 1-bit input: Active high, clock enable (Divided modes only)
     CLR => '0',       -- 1-bit input: Active high, asynchronous clear (Divided mode only)
     I => dev_data_clk_in.DATA_CLK_P -- 1-bit input: Clock buffer input driven by an IBUFG, MMCM or local interconnect
  );

  buf_lvds : if LVDS_p = btrue generate
    data_bus_bits_are_not_reversed : if data_bus_bits_are_reversed = bfalse generate
      adc_rx_data_buf_ordered <= adc_data((adc_width/2)-1 downto 0);
    end generate;
    data_bus_bits_are_reversed_t : if data_bus_bits_are_reversed = btrue generate
      buf_data : for idx in ((adc_width/2)-1) downto 0 generate
        adc_rx_data_buf_ordered(idx) <= adc_data((adc_width/2)-1-idx);
      end generate;
    end generate;
  end generate;
  -- TODO / FIXME - add data bus direction reversal handling for CMOS IOSTANDARDs

  -- we want adc_clk to be rising edge for worker-internal logic
  gen_adc_clk : if data_clk_p_is_inverted = bfalse generate
    adc_clk <= adc_clk_buf;
  end generate;
  gen_adc_clk_n : if data_clk_p_is_inverted = btrue generate
    adc_clk <= not adc_clk_buf;
  end generate;

  --Register RX FRAME indicator so that it's delayed the same as ddr_out_rising_rr
  --and ddr_out_falling_rr
  gen_rx_frame_inv : if rx_frame_is_inverted = bfalse generate
  begin
    adc_rx_frame_p_buf <= RX_FRAME_P_s;
  end generate;
  gen_rx_frame_not_inv : if rx_frame_is_inverted = btrue  generate
  begin
    adc_rx_frame_p_buf <= not RX_FRAME_P_s;
  end generate;

  data_mode_cmos: if LVDS_p = bfalse generate -- i.e. iostandard is cmos
  begin
    -- don't compile for this!!!
    -- TODO / FIXME fill in this code
    ctl_out.error <= '1'; -- force unsuccessful end of all control operations
    -- TODO / FIXME support runtime dynamic enumeration for CMOS? (if so, we need to check duplex_config = runtime_dynamic)
  end generate;

  -- TODO / FIXME support runtime dynamic enumeration for CMOS? (if so, we need to check duplex_config = runtime_dynamic)
  data_mode_lvds_invalid: if ((LVDS_p = btrue) and
                              ((SINGLE_PORT_p      = btrue) or
                               (HALF_DUPLEX_p      = btrue) or
                               (DATA_RATE_CONFIG_p = SDR_e))) generate
  begin
    -- this OpenCPI build configuration is not supported by AD9361
    -- and should never be used
    ctl_out.error <= '1'; -- force unsuccessful end of all control operations
  end generate;

  data_mode_lvds: if (LVDS_p = btrue) and
                     (SINGLE_PORT_p       = bfalse) and
                     (HALF_DUPLEX_p       = bfalse) and
                     (DATA_RATE_CONFIG_p  = DDR_e) generate
  begin

    adc_data <= dev_data_from_pins_in.data((adc_width/2)-1 downto 0);

    -- allows RX_FRAME_P to have similar skew as P1_D signals, also allows us to
    -- sample RX_FRAME_P at in the middle of the clock period by using the Q2
    -- output
    rx_frame_p_ddr : IDDR
    generic map(
      DDR_CLK_EDGE => "SAME_EDGE_PIPELINED",
      INIT_Q1      => '0',
      INIT_Q2      => '0',
      SRTYPE       => "ASYNC")
    port map(
      Q1 => open,
      Q2 => adc_rx_frame_p_buf_rr,
      C  => adc_clk,
      CE => '1',
      D  => adc_rx_frame_p_buf,
      R  => '0',
      S  => '0'
      );

    buf_data_loop : for idx in ((adc_width/2)-1) downto 0 generate
    begin
      data_ddr : IDDR
      generic map(
        DDR_CLK_EDGE => "SAME_EDGE_PIPELINED",
        INIT_Q1      => '0',
        INIT_Q2      => '0',
        SRTYPE       => "ASYNC")
      port map(
        Q1 => adc_ddr_out_rising_rr(idx),
        Q2 => adc_ddr_out_falling_rr(idx),
        C  => adc_clk,
        CE => '1',
        D  => adc_rx_data_buf_ordered(idx),
        R  => '0',
       S  => '0'
        );
    end generate;

    delay_r1_by_2_to_align_with_r2 : process(adc_clk)
    begin
      if rising_edge(adc_clk) then
        adc_r1_i_rrrrr  <= adc_r1_i_rrrr;
        adc_r1_q_rrrrr  <= adc_r1_q_rrrr;
        adc_r1_i_rrrrrr <= adc_r1_i_rrrrr;
        adc_r1_q_rrrrrr <= adc_r1_q_rrrrr;
        adc_r1_give_rrrrr  <= adc_r1_give_rrrr;
        adc_r1_give_rrrrrr <= adc_r1_give_rrrrr;
      end if;
    end process;

    -- simultaneously handles both possible 1R1T and 2R2T timing diagrams from
    -- AD9361_Reference_Manual_UG-570.pdf Figure 79.
    data_ingest_fsm : process(adc_clk)
    begin
      if rising_edge(adc_clk) then
        case state is
          when R1_11_6 =>
            adc_r1_give_rrrr <= '0';
            adc_r2_give_rrrr <= '0';
            -- this is why we set rx_frame_usage_is_toggle to btrue above
            if(adc_rx_frame_p_buf_rr = '1') then
              adc_r1_i_h_rrr <= adc_ddr_out_rising_rr;
              adc_r1_q_h_rrr <= adc_ddr_out_falling_rr;
              state <= R1_5_0;
            end if;
          when R1_5_0 =>
            adc_r1_i_rrrr <= adc_r1_i_h_rrr & adc_ddr_out_rising_rr;
            adc_r1_q_rrrr <= adc_r1_q_h_rrr & adc_ddr_out_falling_rr;

            adc_r1_give_rrrr <= r1_worker_present;

            -- if valid first channel samples are received but there is no
            -- qadc worker to ingest them, detect the error
            adc_r1_samps_dropped <= not r1_worker_present;

            -- this is why we set rx_frame_usage_is_toggle to btrue above
            if(adc_rx_frame_p_buf_rr = '1') then
              state <= R2_11_6;
            else
              state <= R1_11_6;
            end if;
          when R2_11_6 =>
            adc_r2_i_h_rrr <= adc_ddr_out_rising_rr;
            adc_r2_q_h_rrr <= adc_ddr_out_falling_rr;
            adc_r1_give_rrrr <= '0';
            state <= R2_5_0;
          when R2_5_0 =>
            adc_r2_i_rrrr <= adc_r2_i_h_rrr & adc_ddr_out_rising_rr;
            adc_r2_q_rrrr <= adc_r2_q_h_rrr & adc_ddr_out_falling_rr;

            -- it is possible (when using LVDS and 1R2T mode) for data to be
            -- received in the R2 channel slot that should be ignored -
            -- no-OS (via the ad9361_config_proxy) directs us via the
            -- config_is_two_r signal when to drop the second channel samples
            -- that should be ignored
            adc_r2_give_rrrr <= dev_cfg_data_in.config_is_two_r and
                                r2_worker_present;
            
            -- if valid R2 channel samples are received but there is no
            -- second qadc worker to ingest them, detect the error
            adc_r2_samps_dropped <= dev_cfg_data_in.config_is_two_r and
                                    (not r2_worker_present);

            state <= R1_11_6;
        end case;
      end if;
    end process;

  end generate;

  -- sync (WSI clock domain) -> (ADC clock domain)
  -- note that we don't care if WSI clock is much faster and bits are
  -- dropped - props_in.channels_are_swapped is a configuration bit which is
  -- expected to change very rarely in relation to either clock
  wsi_reset_n <= not ctl_in.reset;
  wsi_channels_are_swapped <= '1' when (props_in.channels_are_swapped = btrue) else '0';
  chan_swap_sync: bsv.bsv.SyncBit
    generic map(
      init   => 0)
    port map(
      sCLK   => ctl_in.clk,
      sRST   => wsi_reset_n, -- apparently sRST is active-low
      dCLK   => adc_clk,
      sEN    => '1',
      sD_IN  => wsi_channels_are_swapped,
      dD_OUT => adc_channels_are_swapped); -- delayed by one WSI clock, two ADC clocks

  r1_worker_present               <= ch0_worker_present when (adc_channels_are_swapped = bfalse) else ch1_worker_present;
  r2_worker_present               <= ch1_worker_present when (adc_channels_are_swapped = bfalse) else ch0_worker_present;

  dev_data_ch0_out_out.adc_clk    <= adc_clk;
  dev_data_ch0_out_out.adc_give   <= adc_r1_give_rrrrrr when (adc_channels_are_swapped = bfalse) else adc_r2_give_rrrr;
  dev_data_ch0_out_out.adc_data_I <= adc_r1_i_rrrrrr    when (adc_channels_are_swapped = bfalse) else adc_r2_i_rrrr;
  dev_data_ch0_out_out.adc_data_Q <= adc_r1_q_rrrrrr    when (adc_channels_are_swapped = bfalse) else adc_r2_q_rrrr;
  dev_data_ch1_out_out.adc_clk    <= adc_clk;
  dev_data_ch1_out_out.adc_give   <= adc_r2_give_rrrr   when (adc_channels_are_swapped = bfalse) else adc_r1_give_rrrrrr;
  dev_data_ch1_out_out.adc_data_I <= adc_r2_i_rrrr      when (adc_channels_are_swapped = bfalse) else adc_r1_i_rrrrrr;
  dev_data_ch1_out_out.adc_data_Q <= adc_r2_q_rrrr      when (adc_channels_are_swapped = bfalse) else adc_r1_q_rrrrrr;

  -- if valid R1 channel samples are received but there is no
  -- qadc worker to ingest them, detect the error
  wsi_r1_samps_dropped_clear <= '1'
                                 when (props_in.r1_samps_dropped_written = '1')
                                      and (props_in.r1_samps_dropped = '0')
                                 else '0';
  r1_samps_dropped_sync : util.util.sync_status
    port map   (clk         => ctl_in.clk,
                reset       => ctl_in.reset,
                operating   => ctl_in.is_operating,
                start       => ctl_in.is_operating,
                clear       => wsi_r1_samps_dropped_clear,
                status      => props_out.r1_samps_dropped,
                other_clk   => adc_clk,
                other_reset => open,
                event       => adc_r1_samps_dropped);

  wsi_r2_samps_dropped_clear <= '1'
                                 when (props_in.r2_samps_dropped_written = '1')
                                      and (props_in.r2_samps_dropped = '0')
                                 else '0';

  -- if valid R2 channel samples are received but there is no
  -- qadc worker to ingest them, detect the error
  r2_samps_dropped_sync : util.util.sync_status
     port map  (clk         => ctl_in.clk,
                reset       => ctl_in.reset,
                operating   => ctl_in.is_operating,
                start       => ctl_in.is_operating,
                clear       => wsi_r2_samps_dropped_clear,
                status      => props_out.r2_samps_dropped,
                other_clk   => adc_clk,
                other_reset => open,
                event       => adc_r2_samps_dropped);
    -- delegate LVDS issues to the data_sub.
    RX_FRAME_P_s <= dev_data_from_pins_in.rx_frame;
     
end rtl;

