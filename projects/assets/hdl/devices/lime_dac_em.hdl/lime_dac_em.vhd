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

library IEEE, ocpi;
use IEEE.std_logic_1164.all, IEEE.numeric_std.all, ocpi.types.all, ocpi.wci.all;
library util; use util.util.all;
architecture rtl of lime_dac_em_worker is

  signal adc_give        : std_logic;
  signal som             : std_logic;
  signal eom             : std_logic;
  signal valid           : std_logic;
  signal give            : std_logic;
  signal tx_en_q1        : std_logic := '0';
  signal tx_en_q2        : std_logic := '0';
  signal data            : std_logic_vector(31 downto 0) := (others => '0');
  signal output_count    : unsigned(31 downto 0);
  signal output_count_en : std_logic;
  signal done            : std_logic;
  signal som_zlm         : std_logic;
  signal eom_zlm         : std_logic;
  signal valid_zlm       : std_logic;
  signal give_zlm        : std_logic;

  -- Procedure for clock generation
  procedure clk_gen (signal clk : out std_logic; constant FREQ : real; signal abort : in std_logic) is
    constant PERIOD    : time := 1 sec / FREQ;        -- Full period
    constant HIGH_TIME : time := PERIOD / 2;          -- High time
    constant LOW_TIME  : time := PERIOD - HIGH_TIME;  -- Low time; always >= HIGH_TIME
  begin
    -- Check the arguments
    assert (HIGH_TIME /= 0 fs) report "clk_gen: High time is zero; time resolution too large for frequency" severity FAILURE;
    assert FALSE report "TX_CLK_IN freq = " & real'image(FREQ) severity NOTE;
    -- Time resolution show
    assert FALSE report "Time resolution: " & time'image(time'succ(0 fs)) severity NOTE;
    -- Generate a clock cycle
    loop
      if abort = '1' then return; end if;
      clk <= '1';
      wait for HIGH_TIME;
      clk <= '0';
      wait for LOW_TIME;
    end loop;
  end procedure;

begin

  -----------------------------------------------------------------------------
  -- Generate TX clock
  -- The TX_CLK_IN starts out at the default frequency, and is then updated each
  -- time the tx_clk_rate property is written.
  tx_clk_gen : process
  begin
    TX_CLK_IN <= '0';
    wait until ctl_in.state = INITIALIZED_e;
    clk_gen(TX_CLK_IN, real(to_integer(props_in.tx_clk_rate)), props_in.tx_clk_rate_written);
    loop
      if props_in.tx_clk_rate_written = '1' then
        wait until props_in.tx_clk_rate_written = '0';
        clk_gen(TX_CLK_IN, real(to_integer(props_in.tx_clk_rate)), props_in.tx_clk_rate_written);
      end if;
    end loop;
  end process tx_clk_gen;

  -- this process delays TX_EN by 1 full I/Q period to align with data created
  -- in the following process
  process(TX_CLK)
  begin
    if falling_edge(TX_CLK) then
      tx_en_q1 <= dev_in.TX_EN;
      tx_en_q2 <= tx_en_q1;
    end if;
  end process;

  -- this process transforms 12-bit I/Q data into 16-bit data
  process(TX_CLK)
  begin
    if rising_edge(TX_CLK) then
      if (dev_in.TX_EN = '1') then
        if (TX_IQ_SEL = '1') then
          data(15 downto  0) <= TXD & x"0";
        else
          data(31 downto 16) <= TXD & x"0";
        end if;
      end if;
    end if;
  end process;

  adc_give <= tx_en_q2 and TX_IQ_SEL;

  fifo : util.util.adc_fifo
    generic map(width       => 32,
                depth       => 16)
    port map   (clk         => ctl_in.clk,
                reset       => ctl_in.reset,
                operating   => ctl_in.is_operating,
                wsi_ready   => out_in.ready,
                wsi_give    => give,
                wsi_valid   => valid,
                wsi_som     => som,
                wsi_eom     => eom,
                wsi_data    => out_out.data,
                clear       => props_in.overrun_written,
                overrun     => props_out.overrun,
                messageSize => props_in.messageSize,
                -- In DAC clock domain
                adc_clk     => TX_CLK,
                adc_give    => adc_give,
                adc_data    => data);

  process(ctl_in.clk)
  begin
    if rising_edge(ctl_in.clk) then
      if (ctl_in.reset = '1') then
        output_count    <= (others => '1');
        output_count_en <= '0';
      elsif (props_in.numOutputSamples_written = '1') then
        output_count    <= props_in.numOutputSamples;
        output_count_en <= '1';
      elsif (valid = '1' and give = '1' and output_count_en = '1') then
        if (output_count = 0) then
          output_count <= output_count;
        else
          output_count <= output_count - 1;
        end if;
      end if;
    end if;
  end process;

  process(ctl_in.clk)
  begin
    if rising_edge(ctl_in.clk) then
      if (ctl_in.reset = '1') then
        done      <= '0';
        som_zlm   <= '0';
        eom_zlm   <= '0';
        valid_zlm <= '0';
        give_zlm  <= '0';
      elsif (output_count = 0 and done = '0') then
        done      <= '1';
        som_zlm   <= '1';
        eom_zlm   <= '1';
        valid_zlm <= '0';
        give_zlm  <= '1';
      else
        done      <= done;
        som_zlm   <= '0';
        eom_zlm   <= '0';
        valid_zlm <= '0';
        give_zlm  <= '0';
      end if;
    end if;
  end process;

  out_out.som   <= som when (output_count /= 0) else som_zlm;
  out_out.eom   <= eom when (output_count /= 0) else eom_zlm;
  out_out.valid <= valid when (output_count /= 0) else valid_zlm;
  out_out.give  <= give when (output_count /= 0) else give_zlm;

  out_out.byte_enable <= (others => '1');

end rtl;
