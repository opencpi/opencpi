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

library ieee;
use ieee.std_logic_1164.all;
USE IEEE.NUMERIC_STD.ALL;
USE IEEE.MATH_REAL.ALL;
USE IEEE.MATH_COMPLEX.ALL;

package dsp_prims is

component cic_dec_gen
  generic (
    g_n          : positive;
    g_m          : positive;
    g_r          : positive;
    g_din_width  : positive;
    g_acc_width  : positive;
    g_dout_width : positive);
  port (
    i_clk        : in std_logic;
    i_rst        : in std_logic;
    i_din        : in std_logic_vector(g_din_width-1 downto 0);
    i_din_vld    : in std_logic;
    o_din_rdy    : out std_logic;
    o_dout       : out std_logic_vector(g_dout_width-1 downto 0);
    o_dout_vld   : out std_logic;
    i_dout_rdy   : in std_logic);
end component;

component cic_int_gen
  generic (
    N          : positive;
    M          : positive;
    R          : positive;
    DIN_WIDTH  : positive;
    ACC_WIDTH  : positive;
    DOUT_WIDTH : positive);
  port (
    CLK      : in  std_logic;
    RST      : in  std_logic;
    DIN_VLD  : in  std_logic;
    DIN      : in  std_logic_vector(DIN_WIDTH-1 downto 0);
    DOUT_EN  : in  std_logic;
    DOUT_VLD : out std_logic;
    DOUT     : out std_logic_vector(DOUT_WIDTH-1 downto 0));
end component;

component dc_offset_cancellation
  generic (
    DATA_WIDTH : positive);
  port (
    CLK        : in  std_logic;
    RST        : in  std_logic;
    BYPASS     : in  std_logic;
    UPDATE     : in  std_logic;
    TC         : in  signed(7 downto 0);
    DIN        : in  signed(DATA_WIDTH-1 downto 0);
    DIN_VLD    : in  std_logic;
    DOUT_RDY   : in  std_logic := '1'; -- take the valid output even if no input, like AXI READY
    DOUT       : out signed(DATA_WIDTH-1 downto 0);
    DOUT_VLD   : out std_logic);
end component;

component fir_systolic_sym_even
  generic (
    NUM_SECTIONS  : positive;
    DATA_WIDTH    : positive;
    COEFF_WIDTH   : positive;
    ACC_PREC      : positive;
    DATA_ADDR     : std_logic_vector(15 downto 0);
    PTR_ADDR      : std_logic_vector(15 downto 0);
    USE_COEFF_PTR : boolean := true);
  port (
    CLK      : in  std_logic;
    RST      : in  std_logic;
    DIN      : in  std_logic_vector(DATA_WIDTH-1 downto 0);
    DIN_VLD  : in  std_logic;
    DOUT     : out std_logic_vector(DATA_WIDTH-1 downto 0);
    DOUT_VLD : out std_logic;
    ADDR     : in  std_logic_vector(15 downto 0);
    RDEN     : in  std_logic;
    WREN     : in  std_logic;
    RDATA    : out std_logic_vector(COEFF_WIDTH-1 downto 0);
    WDATA    : in  std_logic_vector(COEFF_WIDTH-1 downto 0));
end component;

component iq_imbalance_corrector
  generic (
    DATA_WIDTH : positive;
    ACC_PREC   : positive);
  port (
    CLK             : in  std_logic;
    RST             : in  std_logic;
    ENABLE          : in  std_logic;
    UPDATE          : in  std_logic;
    LOG2_AVG_LEN    : in  unsigned(4 downto 0);
    NLOG2_LOOP_GAIN : in  unsigned(4 downto 0);
    DIN_I           : in  signed(DATA_WIDTH-1 downto 0);
    DIN_Q           : in  signed(DATA_WIDTH-1 downto 0);
    DIN_VLD         : in  std_logic;
    DOUT_I          : out signed(DATA_WIDTH-1 downto 0);
    DOUT_Q          : out signed(DATA_WIDTH-1 downto 0);
    DOUT_VLD        : out std_logic;
    C_CORR          : out signed(ACC_PREC-1 downto 0);
    D_CORR          : out signed(ACC_PREC-1 downto 0));
end component;

component macc_systolic_sym
  generic (
    DATA_WIDTH  : positive;
    COEFF_WIDTH : positive;
    ACC_PREC    : positive);
  port (
    CLK      : in  std_logic;
    RST      : in  std_logic;
    DIN_VLD  : in  std_logic;
    DLY_IN_F : in  std_logic_vector(DATA_WIDTH-1 downto 0);
    DLY_IN_R : in  std_logic_vector(DATA_WIDTH-1 downto 0);
    DLY_OUT  : out std_logic_vector(DATA_WIDTH-1 downto 0);
    COEFF    : in  std_logic_vector(COEFF_WIDTH-1 downto 0);
    ACC_IN   : in  std_logic_vector(ACC_PREC-1 downto 0);
    ACC_OUT  : out std_logic_vector(ACC_PREC-1 downto 0));
end component;

component cordic_pr
  generic (
    DATA_WIDTH  : positive;
    DATA_EXT    : positive;
    STAGES      : positive);
  port (
    CLK      : in  std_logic;
    RST      : in  std_logic;
    MAG      : in  std_logic_vector(DATA_WIDTH-1 downto 0);
    PHASE    : in  std_logic_vector(DATA_WIDTH-1 downto 0);
    VLD_IN   : in  std_logic;
    I        : out std_logic_vector(DATA_WIDTH-1 downto 0);
    Q        : out std_logic_vector(DATA_WIDTH-1 downto 0);
    VLD_OUT  : out std_logic);
end component;

component cordic_rp
  generic (
    DATA_WIDTH : positive;
    DATA_EXT   : positive;
    STAGES     : positive);
  port (
    CLK      : in  std_logic;
    RST      : in  std_logic;
    I        : in  std_logic_vector(DATA_WIDTH-1 downto 0);
    Q        : in  std_logic_vector(DATA_WIDTH-1 downto 0);
    VLD_IN   : in  std_logic;
    MAG      : out std_logic_vector(DATA_WIDTH-1 downto 0);
    PHASE    : out std_logic_vector(DATA_WIDTH-1 downto 0);
    VLD_OUT  : out std_logic);
end component;

component cordic
  generic (
    DATA_WIDTH : positive;
    STAGES     : positive;
    ANGLE_MODE : string;
    DIR_MODE   : string);
  port (
    CLK     : in  std_logic;
    RST     : in  std_logic;
    X_IN    : in  std_logic_vector(DATA_WIDTH-1 downto 0);
    Y_IN    : in  std_logic_vector(DATA_WIDTH-1 downto 0);
    Z_IN    : in  std_logic_vector(DATA_WIDTH-1 downto 0);
    VLD_IN  : in  std_logic;
    X_OUT   : out std_logic_vector(DATA_WIDTH-1 downto 0);
    Y_OUT   : out std_logic_vector(DATA_WIDTH-1 downto 0);
    Z_OUT   : out std_logic_vector(DATA_WIDTH-1 downto 0);
    VLD_OUT : out std_logic);
end component;

component cordic_stage
  generic (
    DATA_WIDTH : positive;
    ANGLE_MODE : string := "CIRCULAR";
    DIR_MODE   : string := "ROTATION");
  port (
    CLK     : in  std_logic;
    RST     : in  std_logic;
    X_IN    : in  std_logic_vector(DATA_WIDTH-1 downto 0);
    Y_IN    : in  std_logic_vector(DATA_WIDTH-1 downto 0);
    Z_IN    : in  std_logic_vector(DATA_WIDTH-1 downto 0);
    VLD_IN  : in  std_logic;
    SHIFT   : in  integer;
    SIGMA   : in  std_logic_vector(DATA_WIDTH-1 downto 0);
    X_OUT   : out std_logic_vector(DATA_WIDTH-1 downto 0);
    Y_OUT   : out std_logic_vector(DATA_WIDTH-1 downto 0);
    Z_OUT   : out std_logic_vector(DATA_WIDTH-1 downto 0);
    VLD_OUT : out std_logic);
end component;

component nco
  generic (
    DATA_WIDTH    : positive;
    PHS_INC_WIDTH : positive;
    PHS_ACC_WIDTH : positive;
    CORDIC_STAGES : positive);
  port (
    CLK      : in  std_logic;
    RST      : in  std_logic;
    EN       : in  std_logic;
    MAG      : in  std_logic_vector(DATA_WIDTH-1 downto 0);
    PHS_INC  : in  std_logic_vector(PHS_INC_WIDTH-1 downto 0);
    PHS_INIT : in  std_logic_vector(PHS_ACC_WIDTH-1 downto 0);
    I_OUT    : out std_logic_vector(DATA_WIDTH-1 downto 0);
    Q_OUT    : out std_logic_vector(DATA_WIDTH-1 downto 0);
    VLD_OUT  : out std_logic);
end component;
end package dsp_prims;
