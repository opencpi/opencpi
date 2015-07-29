-- Lime DAC worker
-- Lime side clock is determined by clkSelect parameter
library IEEE, ocpi;
use IEEE.std_logic_1164.all, ieee.numeric_std.all, ocpi.types.all, ocpi.util.all;
architecture rtl of lime_dac_worker is
  -- FIFO parameters
  constant fifo_width : natural := 24; -- the fifo is just wide enough to feed lime DAC
  constant fifo_depth : natural := 64; -- why? how about 16 for SRL?
  -- CTL/WSI Clock domain signals
  signal wsi_data     : std_logic_vector(fifo_width - 1 downto 0);
  signal div_ctl_clk  : std_logic;
  -- Lime TX clock domain signals
  signal dac_clk      : std_logic; -- clock sent to lime tx (if connected)
  signal dac_data     : std_logic_vector(fifo_width - 1 downto 0);
  signal dac_ready    : bool_t;    -- fifo has data for DAC
  signal dac_take     : bool_t;    -- take data from FIFO
  signal sel_iq_r     : bool_t;    -- true for I, false for Q, sent to lime
begin
  --------------------------------------------------------------------------------
  -- DAC Sample Clock choices
  --------------------------------------------------------------------------------
  -- 1. We get a copy of what is wired to the lime's TX_CLK input, and use that.
  -- 2. We get a clock that *we* are supposed to drive to the lime TX_CLK, and use that.
  -- 4. We divide-down the control clock, use it, and drive TX_CLK
  -- 5. We use a container clock, use that, and drive TX_CLK
  dac_clk <= tx_clk_in when its(use_clk_in) else
             div_ctl_clk when its(use_ctl_clk) else
             sample_clk;
  tx_clk <= dac_clk when its(drive_clk) else '0';

  -- Output/lime clock domain processing. 
  -- Mux I and Q onto TXD, consistent with the TX_IQ_SEL output to lime
  tx_iq_sel <= sel_iq_r;
  txd       <= dac_data(23 downto 12) when its(sel_iq_r) else dac_data(11 downto 0);

  output: process(dac_clk)
  begin
    if rising_edge(dac_clk) then
      sel_iq_r <= not sel_iq_r;
    end if;
  end process;     

  -- Temp signal necessary only because of older VHDL
  wsi_data <= slv(unsigned(in_in.data(31 downto 20)) + ('0' & in_in.data(19))) &
              slv(unsigned(in_in.data(15 downto 4)) + ('0' & in_in.data(3)));
  dac_take  <= not sel_iq_r and dac_ready;
  fifo : entity work.dac_fifo
    generic map(width     => fifo_width,
                depth     => fifo_depth)
    port map   (clk       => ctl_in.clk,
                reset     => ctl_in.reset,
                operating => ctl_in.is_operating,
                wsi_ready => in_in.ready,
                wsi_valid => in_in.valid,
                wsi_data  => wsi_data,
                clear     => props_in.underrun_written,
                wsi_take  => in_out.take,
                underrun  => props_out.underrun,
                dac_clk   => dac_clk,
                dac_take  => dac_take,
                dac_ready => dac_ready,
                dac_data  => dac_data);
--  in_out.take <= '1';
--  props_out.underrun <= '1';
--  dac_ready <= '1';
--  dac_data <= (others => '0');
end rtl;
