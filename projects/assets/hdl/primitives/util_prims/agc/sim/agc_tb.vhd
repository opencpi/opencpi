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
-- Testbench for Automatic Gain Control
-------------------------------------------------------------------------------
--
-- File: agc_tb.vhd
--
-- Description:
--
-- This is a test bench to test the AGC implemented in 'agc.vhd'. It requires the
-- following files:
--   'agc.vhd'         - unit under test,
--   'agc_sig_gen.vhd' - getting stimulus data from a file,
--   'agc_capture.vhd' - capturing output data to a file,
--   'agc_tb_in.txt'   - Matlab generated stimulus data,
--   'agc_tb_out.txt'  - Captured data to be display by Matlab
-- In addition, the followings Octave files are also needed:
--   'agc.m' - Matlab/Octave model to setup AGC parameters and to generate stimulus
--             waveform data.
--   'plotting_agc.m' - Matlab/Octave file to plot the output data from simulation.
--
-- Test procedure:
-- 1. Define all parameters in the Octave model 'agc.m' such as:
--      Data bith width (B), sampling freq (Fs), desired output (Ref)
-- 2. Run agc.m and verify the results via the display output, adjust the parameters
--    as needed,
-- 3. Once satisfy with the results, the input waveform is saved in files 'agc_tb_in.txt'
--    and 'agc_tb_in.bin'. Make notes of the print out for values to be used to construct
--    the AGC and the testbench.
-- 4. Modify AGC source code 'agc.vhd' for correct generic parameters,
--    DATA_WIDTH and NAVG.
-- 5. Modify the testbench 'agc_tb.vhd' for correct control values, REF and MU.
-- 6. Setup and simulate the AGC using provided testbench and input data files. The output
--    will be captured in file 'agc_tb_out.txt'
-- 7. To view the results, run 'plotting_agc.m' to display input and output waveform data.
--
-------------------------------------------------------------------------------
-- Revision Log:
-------------------------------------------------------------------------------
-- 03/01/16:
-- File Created
-------------------------------------------------------------------------------

library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;

entity agc_tb is
--  Port ( );
end agc_tb;

architecture Behavioral of agc_tb is
  -- Using the same parameters as agc.m for DATA_WIDTH, NAVG, REF and MU
  constant DATA_WIDTH : integer := 16;         -- input data width
  constant NAVG       : integer := 16;         -- samples in detector's accum,
  constant REF : std_logic_vector(DATA_WIDTH-1 downto 0) := x"1B26"; -- desired output power,
  constant MU  : std_logic_vector(DATA_WIDTH-1 downto 0) := x"144E"; -- feedback error scale factor,

  constant FILE_IN  : string := "agc_tb_in.txt";
  constant FILE_OUT : string := "/home/sngo/FPGA/common/dsp/agc/sim/agc_tb_out.txt";
  constant CLK_PER  : time   := 10 ns;

  constant DOUT_WIDTH : integer := DATA_WIDTH; -- change as needed depends on the signal being probed

  component AGC
--    generic (
--      DATA_WIDTH : integer;
--      NAVG       : integer );
    port (
      CLK     : in  std_logic;
      RST     : in  std_logic;
      REG_WR  : in  std_logic;                               -- program the control values
      REF     : in  std_logic_vector(DATA_WIDTH-1 downto 0); -- desired output
      MU      : in  std_logic_vector(DATA_WIDTH-1 downto 0); -- feedback error scale factor
      DIN_VLD : in  std_logic;                               -- indicates valid input data
      HOLD    : in  std_logic;                               -- maintain the current gain
      DIN     : in  std_logic_vector(DATA_WIDTH-1 downto 0);
      DOUT    : out std_logic_vector(DOUT_WIDTH-1 downto 0) );
  end component;

  component agc_sig_gen
    generic (
      FILE_NAME  : string;
      DATA_WIDTH : integer );
    port (
      CLK      : in  std_logic;
      RST      : in  std_logic;
      GO       : in  std_logic;
      DONE     : out std_logic;
      DOUT     : out std_logic_vector(DATA_WIDTH-1 downto 0);
      DOUT_VLD : out std_logic );
  end component;

  component agc_capture
    generic (
      FILE_NAME  : string;
      DATA_WIDTH : integer );
    port (
      CLK     : in std_logic;
      RST     : in std_logic;
      DONE    : in std_logic;
      DIN     : in std_logic_vector(DATA_WIDTH-1 downto 0) );
  end component;

  signal clk          : std_logic := '0';
  signal rst          : std_logic := '0';
  signal dat_in       : std_logic_vector(DATA_WIDTH-1 downto 0);
  signal dat_out      : std_logic_vector(DOUT_WIDTH-1 downto 0);
  signal regwr        : std_logic := '0';
  signal dvld         : std_logic := '0';
  signal dvld_en      : std_logic := '1';
  signal agc_hold     : std_logic := '0';
  signal sim_start    : std_logic := '0';
  signal sig_gen_dvld : std_logic := '0';
  signal sig_gen_done : std_logic := '0';
  signal sim_done     : std_logic := '0';

begin
  ----------------------------------------------------------------------------
  -- Component instantiation
  --    [ agc_sig_gen ] --> [ AGC ] --> [ agc_capture ]
  ----------------------------------------------------------------------------

  -- Unit Under Test
  uut: AGC
--    generic map (
--      DATA_WIDTH => DATA_WIDTH,
--      NAVG       => NAVG )
    port map (
      CLK     => clk,
      RST     => rst,
      REG_WR  => regwr,
      REF     => REF,
      MU      => MU,
      DIN_VLD => dvld,
      HOLD    => agc_hold,
      DIN     => dat_in,
      DOUT    => dat_out );

  -- File-Based Input Data Stream
  stimulus: agc_sig_gen
    generic map (
      FILE_NAME  => FILE_IN,
      DATA_WIDTH => DATA_WIDTH)
    port map (
      CLK      => clk,
      RST      => rst,
      GO       => sim_start,
      DONE     => sig_gen_done,
      DOUT     => dat_in,
      DOUT_VLD => sig_gen_dvld);

  -- File-Based Output Data Capture
  capture: agc_capture
    generic map (
      FILE_NAME  => FILE_OUT,
      DATA_WIDTH => DOUT_WIDTH)
    port map (
      CLK     => clk,
      RST     => rst,
      DONE    => sim_done,
      DIN     => dat_out);

  ----------------------------------------------------------------------------
  -- Clock Generator
  ----------------------------------------------------------------------------
  clk_gen: process
    begin
      clk <= '0'; wait for CLK_PER/2;
      clk <= '1'; wait for CLK_PER/2;
    end process;

  ----------------------------------------------------------------------------
  -- Stimulus
  ----------------------------------------------------------------------------

  ---- data valid
  dvld <= dvld_en and sig_gen_dvld;
  dvld_sim: process
    begin
      dvld_en <= '1'; wait for 5us;
      dvld_en <= '0'; wait for 1us;
      dvld_en <= '1'; wait;
    end process;

  ---- gain hold
  agchold_sim: process
    begin
      agc_hold <= '0'; wait for 30us;
      agc_hold <= '1'; wait;
    end process;

  ---- main stimulation
  stim: process
    begin
      -- Initial conditions
      rst  <= '1';
      for i in 1 to 10 loop   -- hold for 10 clks
         wait until rising_edge(clk);
      end loop;
      rst <= '0';

      -- Program the control registers
      wait until rising_edge(clk);
      regwr <= '1';

      -- Start the simulation
      wait until rising_edge(clk);
      sim_start <= '1';
      regwr     <= '0';

      -- simulation and capturing in progress ......
      wait until (sig_gen_done='1');

      -- Delay 20 clks then stop the capture
      for i in 1 to 20 loop
         wait until rising_edge(clk);
      end loop;
      sim_done <= '1';

      wait;
    end process;
  ----------------------------------------------------------------------------
end Behavioral;
