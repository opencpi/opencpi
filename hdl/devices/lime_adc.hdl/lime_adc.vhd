-- Lime ADC worker
library IEEE, ocpi;
use IEEE.std_logic_1164.all, ieee.numeric_std.all, ocpi.types.all;
architecture rtl of lime_adc_worker is
  -- FIFO parameters
  constant fifo_width : natural := 24; -- the fifo is just wide enough to feed lime DAC
  constant fifo_depth : natural := 64; -- why? how about 16 for SRL?
  -- CTL/WSI clock domain
  signal wsi_data     : std_logic_vector(23 downto 0);
  signal div_ctl_clk  : std_logic;     -- the divided control clock
  -- ADC clock domain
  signal adc_clk      : std_logic;
  signal adc_data     : std_logic_vector(23 downto 0);
  signal adc_ready    : bool_t;
  signal adc_give     : bool_t;
  signal hold_i_r     : std_logic_vector(11 downto 0); -- hold I waiting for Q
begin
  -- Route the raw property signals to the lime_spi
  rawprops_out.present     <= '1';
  rawprops_out.reset       <= ctl_in.reset;
  rawprops_out.raw         <= props_in.raw;
  props_out.raw            <= rawprops_in.raw;
  props_out.other_present  <= rawprops_in.present(1);
  -- Take data out of the FIFO and sign extend each I/Q part
  out_out.data             <= (31 downto 28 => wsi_data(23)) & wsi_data(23 downto 12) &
                              (15 downto 12 => wsi_data(11)) & wsi_data(11 downto 0);
  --------------------------------------------------------------------------------
  -- ADC Sample Clock choices
  --------------------------------------------------------------------------------
  -- 1. We are wired to the RX_CLK_OUT source-synchronous clock from the lime, and use it.
  -- 2. We get a copy of what is wired to the lime's RX_CLK input, and use that.
  -- 3. We get a clock that *we* are supposed to drive to the lime RX_CLK, and use that.
  -- 4. We divide-down the control clock, use it, and drive RX_CLK
  -- 5. We use a container clock, use that, and drive RX_CLK
  adc_clk <= rx_clk_out when its(use_clk_out) else
             rx_clk_in when its(use_clk_in) else
             div_ctl_clk when its(use_ctl_clk) else
             sample_clk;
  rx_clk <= adc_clk when its(drive_clk) else '0';

  -- ADC Clocked process: we just need to hold the I value until the corresponding Q value
  -- is available to clock them together into the FIFO.
  input : process(adc_clk)
  begin
    if rising_edge(adc_clk) then
      if rx_iq_sel = '0' then
        hold_i_r <= rxd;
      end if;
    end if;
  end process;

  -- Temp signals necessary only because of older VHDL restrictions on "actuals".
  adc_give             <= to_bool(adc_ready and rx_iq_sel = '1');
  adc_data             <= rxd & hold_i_r;
  fifo : entity work.adc_fifo
    generic map(width       => 24,
                depth       => 64)
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
                adc_clk     => adc_clk,
                adc_data    => adc_data);
end rtl;
