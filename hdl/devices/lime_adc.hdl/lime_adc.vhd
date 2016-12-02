-- Lime ADC worker
library IEEE, ocpi;
use IEEE.std_logic_1164.all, ieee.numeric_std.all, ocpi.types.all;
library util; use util.util.all;
architecture rtl of lime_adc_worker is
  -- FIFO parameters
  constant fifo_width : natural := 24; -- the fifo is just wide enough to feed lime DAC
  constant fifo_depth : natural := 64; -- why? how about 16 for SRL?
  -- CTL/WSI clock domain
  signal wsi_data     : std_logic_vector(23 downto 0);
  signal div_ctl_clk  : std_logic;     -- the divided control clock
  -- ADC clock domain
  signal adc_clk      : std_logic;
  signal not_adc_clk  : std_logic;
  signal adc_clk_buf  : std_logic;
  signal adc_data     : std_logic_vector(23 downto 0);
  signal adc_give     : bool_t;
  signal rxd_r        : std_logic_vector(11 downto 0);
  signal hold_i_r     : std_logic_vector(11 downto 0); -- hold I
  signal hold_q_r     : std_logic_vector(11 downto 0); -- hold Q
  signal rx_iq_sel_r  : std_logic;
begin
  -- Take data out of the FIFO and sign extend each I/Q part
  out_out.data             <= (31 downto 28 => wsi_data(23)) & wsi_data(23 downto 12) &
                              (15 downto 12 => wsi_data(11)) & wsi_data(11 downto 0);
  out_out.byte_enable <= (others => '1');
  --------------------------------------------------------------------------------
  -- ADC Sample Clock choices
  --------------------------------------------------------------------------------
  -- 1. We are wired to the RX_CLK_OUT source-synchronous clock from the lime, and use it.
  -- 2. We get a copy of what is wired to the lime's RX_CLK input, and use that.
  -- 3. We get a clock that *we* are supposed to drive to the lime RX_CLK, and use that.
  -- 4. We divide-down the control clock, use it, and drive RX_CLK
  -- 5. We use a container clock, use that, and drive RX_CLK
  adc_clk <= rx_clk_out when its(USE_CLK_OUT_p) else
             rx_clk_in when its(USE_CLK_IN_p) else
             ctl_in.clk when its(USE_CTL_CLK_p) else
             sample_clk;
  not_adc_clk <= not adc_clk;
  rx_clk <= adc_clk when its(DRIVE_CLK_p) else '0';

  --Place ADC clock on global buffer for use in processing
  buf_adc : buffer_clock_global
    port map(
      clk => not_adc_clk,
      clk_buffered => adc_clk_buf
      );

  --Register data and IQ select
  adc_regs : process(adc_clk_buf)
  begin
    if rising_edge(adc_clk_buf) then
      if ctl_in.reset = '1' then
        rxd_r <= (others => '0');
        rx_iq_sel_r <= '1';
      else
        rxd_r <= rxd;
        rx_iq_sel_r <= rx_iq_sel;
      end if;
    end if;
  end process;
  
  -- ADC Clocked process: we just need to hold the I value until the corresponding Q value
  -- is available to clock them together into the FIFO.
  input : process(adc_clk_buf)
  begin
    if rising_edge(adc_clk_buf) then
      if rx_iq_sel_r = '1' then
        hold_i_r <= rxd_r;
      else
        hold_q_r <= rxd_r;
      end if;
    end if;
  end process;

  -- Temp signals necessary only because of older VHDL restrictions on "actuals".
  adc_give             <= to_bool(rx_iq_sel_r = '1');
  adc_data             <= hold_q_r & hold_i_r;
  fifo : entity work.adc_fifo
    generic map(width       => 24,
                depth       => 8096)
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
                adc_clk     => adc_clk_buf,
                adc_give    => adc_give,
                adc_data    => adc_data);
end rtl;
