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

--------------------------------------------------------------------------------
-- This implements AD9361_Reference_Manual_UG-570.pdf Figure 80. Transmit
-- Data Path, LVDS. The implementation is purely combinatorial.
--
--------------------------------------------------------------------------------
library IEEE; use IEEE.std_logic_1164.all, ieee.numeric_std.all;
library util; use util.util.all;
library unisim; use unisim.vcomponents.all; -- ODDR
entity ad936x_tx_data_lvds is
  port (data_clk       : in  std_logic; -- AD9361 DATA_CLK
        two_r_two_t_en : in  std_logic; -- tells use whether 2R2T is
                                        -- enforced by AD9361 config
        -- T1/T2 in AD9361_Reference_Manual_UG-570.pdf Figure 80.
        -- (TX channel 1 and 2)
        data_i_t1      : in  std_logic_vector(11 downto 0);
        data_q_t1      : in  std_logic_vector(11 downto 0);
        data_i_t2      : in  std_logic_vector(11 downto 0);
        data_q_t2      : in  std_logic_vector(11 downto 0);
        ready_t1       : in  std_logic; -- data...t1 are valid and ready
        ready_t2       : in  std_logic; -- data...t2 are valid and ready
        take_t1        : out std_logic;
        take_t2        : out std_logic;
        fb_clk         : out std_logic; -- AD9361 FB_CLK
        tx_frame       : out std_logic; -- AD9361 TX_FRAME
        tx_data        : out std_logic_vector(5 downto 0)); -- AD9361 P0 or P1
end entity ad936x_tx_data_lvds;
architecture rtl of ad936x_tx_data_lvds is
  signal hi_lo_sel       : std_logic := '0'; -- 1 means hi, 0 means lo
  signal ch_toggle       : std_logic := '0';
  signal ch_sel          : std_logic := '0'; -- 1 means T2, 0 means T1
  signal data_i          : std_logic_vector(11 downto 0) := (others => '0');
  signal data_q          : std_logic_vector(11 downto 0) := (others => '0');
  signal data_i_hi       : std_logic_vector(5 downto 0) := (others => '0');
  signal data_q_hi       : std_logic_vector(5 downto 0) := (others => '0');
  signal data_i_lo_r     : std_logic_vector(5 downto 0) := (others => '0');
  signal data_q_lo_r     : std_logic_vector(5 downto 0) := (others => '0');
  signal tx_frame_ddr    : std_logic := '0';
  signal tx_data_ddr_ris : std_logic_vector(5 downto 0) := (others => '0');
  signal tx_data_ddr_fal : std_logic_vector(5 downto 0) := (others => '0');
begin

  hi_lo_sel_reg : process(data_clk)
  begin
    if rising_edge(data_clk) then
      hi_lo_sel <= not hi_lo_sel;
    end if;
  end process;

  ch_toggle_reg : process(data_clk)
  begin
    if rising_edge(data_clk) then
      if hi_lo_sel = '0' then
        ch_toggle <= not ch_toggle;
      end if;
    end if;
  end process;

  ch_sel <= ch_toggle and two_r_two_t_en;

  take_t1 <= ready_t1 and (not ch_sel) when hi_lo_sel = '0' else '0';
  take_t2 <= ready_t2 and (    ch_sel) when hi_lo_sel = '0' else '0';

  data_i <= data_i_t2 when ch_sel = '1' else data_i_t1;
  data_q <= data_q_t2 when ch_sel = '1' else data_q_t1;

  data_i_hi <= data_i(11 downto 6);
  data_q_hi <= data_q(11 downto 6);

  -- because of hi/lo serialization, we delay lo to ensure that, by the time lo
  -- is serialized, it corresponds to the same previously serialized hi
  data_lo_delay_regs : process(data_clk)
  begin
    if rising_edge(data_clk) then
      if hi_lo_sel = '1' then
        data_i_lo_r <= data_i(5 downto 0);
        data_q_lo_r <= data_q(5 downto 0);
      end if;
    end if;
  end process;

  tx_frame_ddr <= not ch_sel when two_r_two_t_en = '1' else hi_lo_sel;

  tx_data_ddr_ris <= data_i_hi when hi_lo_sel = '1' else data_i_lo_r;
  tx_data_ddr_fal <= data_q_hi when hi_lo_sel = '1' else data_q_lo_r;

  -- D1/D2 of 0/1 causes fb_clk to be inverted version of data_clk
  -- (necessary for framing alignment)
  data_clk_forward : ODDR
    generic map(
      DDR_CLK_EDGE => "SAME_EDGE",
      INIT         => '0',
      SRTYPE       => "ASYNC")
    port map(
      Q  => fb_clk,
      C  => data_clk,
      CE => '1',
      D1 => '0',
      D2 => '1',
      R  => '0',
      S  => '0');

  -- an ODDR primitive used here minimizes slack, which is necessary to meet
  -- timing on Xilinx FPGAs at LVDS DATA_CLK rates
  tx_frame_oddr : ODDR
    generic map(
      DDR_CLK_EDGE => "SAME_EDGE",
      INIT         => '0',
      SRTYPE       => "ASYNC")
    port map(
      Q  => tx_frame,
      C  => data_clk,
      CE => '1',
      D1 => tx_frame_ddr,
      D2 => tx_frame_ddr,
      R  => '0',
      S  => '0');

  tx_data_oddr_loop : for idx in tx_data'length-1 downto 0 generate
  begin
    tx_data_oddr : ODDR
      generic map(
        DDR_CLK_EDGE => "SAME_EDGE",
        INIT         => '0',
        SRTYPE       => "ASYNC")
      port map(
        Q  => tx_data(idx),
        C  => data_clk,
        CE => '1',
        D1  => tx_data_ddr_ris(idx),
        D2  => tx_data_ddr_fal(idx),
        R  => '0',
        S  => '0');
  end generate;

end rtl;
