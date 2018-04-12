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
-- Testbench for cordic
-------------------------------------------------------------------------------
--
-- File: cordic_tb.vhd
--
-- Description:
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use ieee.math_real.all;

-- For Signal Spy
--library modelsim_lib;
--use modelsim_lib.util.all;

entity cordic_tb is
end cordic_tb;

architecture rtl of cordic_tb is

   constant CLK_PER : time := 10 ns; -- 100 MHz

   constant DATA_WIDTH        : integer := 20;
   constant STAGES            : integer := 20;
   constant NCO_DATA_WIDTH    : integer := 16;
   constant NCO_PHS_INC_WIDTH : integer := 16;
   constant NCO_PHS_ACC_WIDTH : integer := 20;
   constant NCO_STAGES        : integer := 16;

   component cordic_mult
   generic (
      DATA_WIDTH  : positive;
      STAGES      : positive);
   port (
      CLK      : in std_logic;
      RST      : in std_logic;
      A        : in std_logic_vector(DATA_WIDTH-1 downto 0);
      B        : in std_logic_vector(DATA_WIDTH-1 downto 0);
      VLD_IN   : in std_logic;
      AB       : out std_logic_vector(DATA_WIDTH-1 downto 0);
      VLD_OUT  : out std_logic);
   end component;

   component cordic_divide
   generic (
      DATA_WIDTH  : positive;
      STAGES      : positive);
   port (
      CLK      : in std_logic;
      RST      : in std_logic;
      A        : in std_logic_vector(DATA_WIDTH-1 downto 0);
      B        : in std_logic_vector(DATA_WIDTH-1 downto 0);
      VLD_IN   : in std_logic;
      AOVERB   : out std_logic_vector(DATA_WIDTH-1 downto 0);
      VLD_OUT  : out std_logic);
   end component;

   component cordic_pr
   generic (
      DATA_WIDTH  : positive;
      STAGES      : positive);
   port (
      CLK      : in std_logic;
      RST      : in std_logic;
      MAG      : in std_logic_vector(DATA_WIDTH-1 downto 0);
      PHASE    : in std_logic_vector(DATA_WIDTH-1 downto 0);
      VLD_IN   : in std_logic;
      I        : out std_logic_vector(DATA_WIDTH-1 downto 0);
      Q        : out std_logic_vector(DATA_WIDTH-1 downto 0);
      VLD_OUT  : out std_logic);
   end component;

   component cordic_rp
   generic (
      DATA_WIDTH  : positive;
      STAGES      : positive);
   port (
      CLK      : in std_logic;
      RST      : in std_logic;
      I        : in std_logic_vector(DATA_WIDTH-1 downto 0);
      Q        : in std_logic_vector(DATA_WIDTH-1 downto 0);
      VLD_IN   : in std_logic;
      MAG      : out std_logic_vector(DATA_WIDTH-1 downto 0);
      PHASE    : out std_logic_vector(DATA_WIDTH-1 downto 0);
      VLD_OUT  : out std_logic);
   end component;

   component sim_capture
   generic (
      FILE_NAME   : string;
      DATA_WIDTH  : integer);
   port (
      CLK      : in std_logic;
      RST      : in std_logic;
      DONE     : in std_logic;
      DIN      : in std_logic_vector(DATA_WIDTH-1 downto 0);
      DIN_VLD  : in std_logic);
   end component;

   signal clk           : std_logic;
   signal rst           : std_logic;
   signal a1            : std_logic_vector(DATA_WIDTH-1 downto 0);
   signal b1            : std_logic_vector(DATA_WIDTH-1 downto 0);
   signal a1b1          : std_logic_vector(DATA_WIDTH-1 downto 0);
   signal a2            : std_logic_vector(DATA_WIDTH-1 downto 0);
   signal b2            : std_logic_vector(DATA_WIDTH-1 downto 0);
   signal a2overb2      : std_logic_vector(DATA_WIDTH-1 downto 0);
   signal mag3          : std_logic_vector(DATA_WIDTH-1 downto 0);
   signal pha3          : std_logic_vector(DATA_WIDTH-1 downto 0);
   signal i3            : std_logic_vector(DATA_WIDTH-1 downto 0);
   signal q3            : std_logic_vector(DATA_WIDTH-1 downto 0);
   signal i4            : std_logic_vector(DATA_WIDTH-1 downto 0);
   signal q4            : std_logic_vector(DATA_WIDTH-1 downto 0);
   signal mag4          : std_logic_vector(DATA_WIDTH-1 downto 0);
   signal mag4_16       : std_logic_vector(15 downto 0);
   signal pha4          : std_logic_vector(DATA_WIDTH-1 downto 0);

   signal vld_in        : std_logic;
   signal vld_out1      : std_logic;
   signal vld_out2      : std_logic;
   signal vld_out3      : std_logic;
   signal vld_out4      : std_logic;
   signal sig_gen_en    : std_logic;
   signal sim_done      : std_logic;

begin

   ----------------------------------------------------------------------------
   -- Clockgen
   ----------------------------------------------------------------------------

   clkgen: process
   begin
      clk <= '0';
      wait for CLK_PER/2;
      clk <= '1';
      wait for CLK_PER/2;
   end process;

   ----------------------------------------------------------------------------
   -- UUT
   ----------------------------------------------------------------------------

   uut1: cordic_mult
   generic map (
      DATA_WIDTH  => DATA_WIDTH,
      STAGES      => STAGES)
   port map (
      CLK      => clk,
      RST      => rst,
      A        => a1,
      B        => b1,
      VLD_IN   => vld_in,
      AB       => a1b1,
      VLD_OUT  => vld_out1);

   uut2: cordic_divide
   generic map (
      DATA_WIDTH  => DATA_WIDTH,
      STAGES      => STAGES)
   port map (
      CLK      => clk,
      RST      => rst,
      A        => a2,
      B        => b2,
      VLD_IN   => vld_in,
      AOVERB   => a2overb2,
      VLD_OUT  => vld_out2);

   uut3: cordic_pr
   generic map (
      DATA_WIDTH  => DATA_WIDTH,
      STAGES      => STAGES)
   port map (
      CLK      => clk,
      RST      => rst,
      MAG      => mag3,
      PHASE    => pha3,
      VLD_IN   => vld_in,
      I        => i3,
      Q        => q3,
      VLD_OUT  => vld_out3);

   uut4: cordic_rp
   generic map (
      DATA_WIDTH  => DATA_WIDTH,
      STAGES      => STAGES)
   port map (
      CLK      => clk,
      RST      => rst,
      I        => i3,
      Q        => q3,
      VLD_IN   => vld_out3,
      MAG      => mag4,
      PHASE    => pha4,
      VLD_OUT  => vld_out4);

   mag4_16 <= mag4(DATA_WIDTH-1 downto DATA_WIDTH-16);

   ----------------------------------------------------------------------------
   -- Stimulus
   ----------------------------------------------------------------------------

   stimulus: process
   begin

      -- Signal Spy Init
      -- init_signal_spy("/..._tb/uut/...","<signal name>",0);

      -----------------------------------------------------
      -- Initialization
      -----------------------------------------------------

      rst          <= '1';
      a1           <= X"50000";
      b1           <= X"15555";
      a2           <= X"20000";
      b2           <= X"60000";
      mag3         <= X"00000";
      pha3         <= X"00000";
      i4           <= X"40000";
      q4           <= X"40000";
      vld_in       <= '0';
      sig_gen_en   <= '0';
      sim_done     <= '0';

      wait for 200 ns;

      -----------------------------------------------------
      -- Release External Reset
      -----------------------------------------------------

      rst <= '0';

      wait for 200 ns;

      -----------------------------------------------------
      -- Enable Signal Gen/Capture
      -----------------------------------------------------

      wait until rising_edge(clk);

      vld_in <= '1';
      mag3 <= X"7FFF0";
      pha3 <= (others => '0');

      wait until rising_edge(clk);

      for i in 0 to 20000  loop
         pha3 <= std_logic_vector(signed(pha3) + 800);
         wait until rising_edge(clk);
      end loop;

      pha3 <= (others => '0');
      vld_in <= '0';

      sim_done <= '1';

      wait;

   end process;

   -----------------------------------------------------
   -- Store Outputs in a File
   -----------------------------------------------------

   capture_in: sim_capture
   generic map (
      FILE_NAME   => "data_in.txt",
      DATA_WIDTH  => DATA_WIDTH)
   port map (
      CLK      => clk,
      RST      => rst,
      DONE     => sim_done,
      DIN      => pha3,
      DIN_VLD  => vld_in);

   capture_cos: sim_capture
   generic map (
      FILE_NAME   => "data_out_cos.txt",
      DATA_WIDTH  => DATA_WIDTH)
   port map (
      CLK      => clk,
      RST      => rst,
      DONE     => sim_done,
      DIN      => i3,
      DIN_VLD  => vld_out3);

   capture_sin: sim_capture
   generic map (
      FILE_NAME   => "data_out_sin.txt",
      DATA_WIDTH  => DATA_WIDTH)
   port map (
      CLK      => clk,
      RST      => rst,
      DONE     => sim_done,
      DIN      => q3,
      DIN_VLD  => vld_out3);

   ----------------------------------------------------------------------------

end rtl;
