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

-------------------------------------------------------------------------------
-- Test Jig for Synchronizers
-------------------------------------------------------------------------------
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use ieee.math_real.all;

entity UUT_jig is

  generic (
    SRC_FNAME     : string;
    VER_CHK_FNAME : string;
    UUT_NAME      : string;
    TEST_NUM      : integer;
    DATA_WIDTH    : positive;
    CLK_PER_1     : time;
    CLK_PER_2     : time);

end UUT_jig;

architecture rtl of UUT_jig is

  constant VER_ACT_FNAME : string := UUT_NAME & "_tb" & integer'image(TEST_NUM) & "_act.txt";
  constant VER_LOG_FNAME : string := UUT_NAME & "_tb" & integer'image(TEST_NUM) & "_log.txt";
  constant VER_TAG       : string := UUT_NAME & " Test # " & integer'image(TEST_NUM);
  --------------------------------------------------------------------------------
  -- UUT component declaration
  --------------------------------------------------------------------------------

  component FIFO20
    port (
      CLK     : in  std_logic;
      RST     : in  std_logic;
      ENQ     : in  std_logic;
      DEQ     : in  std_logic;
      FULL_N  : out std_logic;
      EMPTY_N : out std_logic;
      CLR     : in  std_logic);
  end component;

  --------------------------------------------------------------------------------
  -- Test Jig declarations
  --------------------------------------------------------------------------------

  component sim_source
    generic (
      FILE_NAME  : string;
      DATA_WIDTH : positive);
    port (
      CLK        : in  std_logic;
      RST        : in  std_logic;
      EN         : in  std_logic;
      DOUT_TDATA : out std_logic_vector(DATA_WIDTH-1 downto 0);
      DOUT_TRDY  : in  std_logic);
  end component;

  component sim_verify
    generic (
      CHK_FNAME  : string;
      ACT_FNAME  : string;
      LOG_FNAME  : string;
      TAG        : string;
      TOLERANCE  : integer;
      DATA_WIDTH : positive);
    port (
      CLK       : in  std_logic;
      RST       : in  std_logic;
      EN        : in  std_logic;
      DONE      : out std_logic;
      DIN_TDATA : in  std_logic_vector(DATA_WIDTH-1 downto 0));
  end component;

  signal clk1                 : std_logic;
  signal clk2                 : std_logic;
  signal rst                  : std_logic;
  signal din                  : std_logic_vector(DATA_WIDTH-1 downto 0);
  signal din_trdy             : std_logic                               := '1';
  signal dout                 : std_logic_vector(DATA_WIDTH-1 downto 0) := (others => '0');
  signal test_en              : std_logic;
  signal test_done            : std_logic;
  signal FFn_vhdl, EFn_vhdl   : std_logic                               := '0';
  signal FFn_vlog, EFn_vlog   : std_logic                               := '0';
  signal FFn_error, EFn_error : std_logic;
  --------------------------------------------------------------------------------

begin

  ----------------------------------------------------------------------------
  -- Clockgen 1
  ----------------------------------------------------------------------------

  clkgen1 : process
  begin
    clk1 <= '0';
    wait for CLK_PER_1/2;
    clk1 <= '1';
    wait for CLK_PER_1/2;
  end process;

  ----------------------------------------------------------------------------
  -- Clockgen 2
  ----------------------------------------------------------------------------

  clkgen2 : process
  begin
    clk2 <= '0';
    wait for CLK_PER_2/2;
    clk2 <= '1';
    wait for CLK_PER_2/2;
  end process;

  ----------------------------------------------------------------------------
  -- Data Source
  ----------------------------------------------------------------------------

  src : sim_source
    generic map (
      FILE_NAME  => SRC_FNAME,
      DATA_WIDTH => DATA_WIDTH)
    port map (
      CLK        => clk1,               --in
      RST        => rst,                --in
      EN         => test_en,            --in
      DOUT_TDATA => din,                --out
      DOUT_TRDY  => din_trdy);          --in

  ----------------------------------------------------------------------------
  -- UUT
  ----------------------------------------------------------------------------

  uut : entity work.sync_fifo_2x0
    port map (
      CLK     => clk1,                  --in
      RST     => rst,                   --in
      ENQ     => din(15),               --in
      DEQ     => din(7),                --in
      CLR     => '0',                   --in
      FULL_N  => FFn_vhdl,              --out
      EMPTY_N => EFn_vhdl);             --out

  dout(15) <= din(15);
  dout(7)  <= not FFn_vhdl;
  dout(0)  <= EFn_vhdl;

  -- TODO:
  -- BSV VERILOG INSTANTIATED FOR INITIAL TEST/VERIFICATION OF VHDL IMPLEMENTATION
  -- THIS MODULE IS TO BE REMOVED PRIOR TO THE 1.5 RELEASE
  uut_verilog : FIFO20
    port map (
      CLK     => clk1,                  --in
      RST     => not rst,               --in
      CLR     => '0',                   --in
      ENQ     => din(15),               --in
      DEQ     => din(7),                --in
      FULL_N  => FFn_vlog,              --out
      EMPTY_N => EFn_vlog);             --out

  FFn_error <= FFn_vhdl xor FFn_vlog;
  EFn_error <= EFn_vhdl xor EFn_vlog;

  ----------------------------------------------------------------------------
  -- Data Verify
  ----------------------------------------------------------------------------

  verify : sim_verify
    generic map (
      CHK_FNAME  => VER_CHK_FNAME,
      ACT_FNAME  => VER_ACT_FNAME,
      LOG_FNAME  => VER_LOG_FNAME,
      TAG        => VER_TAG,
      TOLERANCE  => 0,  -- 0 because using 'Golden' files for verification
      DATA_WIDTH => DATA_WIDTH)
    port map (
      CLK       => clk1,
      RST       => rst,
      EN        => test_en,
      DONE      => test_done,
      DIN_TDATA => dout);

  ----------------------------------------------------------------------------
  -- Stimulus
  ----------------------------------------------------------------------------

  stimulus : process
  begin
    test_en <= '0';
    rst     <= '1';
    wait for 100 ns; wait until rising_edge(clk1);
    rst     <= '0';
    wait for 100 ns; wait until rising_edge(clk1);
    test_en <= '1';
    wait until test_done = '1';
    wait for 100 ns; wait until rising_edge(clk1);
    test_en <= '0';
    wait;
  end process;

end rtl;
