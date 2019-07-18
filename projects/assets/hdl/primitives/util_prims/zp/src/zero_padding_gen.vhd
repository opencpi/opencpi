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

-- DEPRECATION NOTICE: This worker is deprecated and will be removed in OpenCPI 2.0. Use the Zero Pad component for new designs.
-------------------------------------------------------------------------------
-- Zero Padding Generic
-------------------------------------------------------------------------------
--
-- Description:
--
-- Each bit of the input data is padded with a configurable # of zeros.
--
-- Requires a data valid input vector, with each bit indicating the validity of
-- the corresponding bit in the input data vector.
--
-- Generics allow configuration of the Max # of Zeros and I/O data widths.
--
-------------------------------------------------------------------------------
-- Revision Log:
-------------------------------------------------------------------------------
-- 10/22/14: File Created
-------------------------------------------------------------------------------
library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use ieee.math_real.all;

entity zero_padding_gen is
  generic (
    MAX_NUM_ZEROS : positive := 128;
    DIN_WIDTH     : positive := 32;
    DOUT_WIDTH    : positive := 32
    );
  port (
    RST_IN    : in  std_logic;
    CLK_IN    : in  std_logic;
    NUM_ZEROS : in  integer;
    ENABLE    : in  std_logic;
    BUSY_N    : out std_logic;
    DIN       : in  std_logic_vector(DIN_WIDTH-1 downto 0);
    DIN_VLD   : in  std_logic_vector(DIN_WIDTH-1 downto 0);
    REN       : in  std_logic;
    DOUT      : out std_logic_vector(DOUT_WIDTH-1 downto 0);
    DOUT_VLD  : out std_logic);
end zero_padding_gen;

architecture rtl of zero_padding_gen is

  constant ZPCNT_WIDTH_c : integer := integer(ceil(log(real(MAX_NUM_ZEROS))/log(2.0)));

  type state_t is (IDLE_s,SEND_DATA_s,ZERO_PAD_s);
  signal current_state,next_state : state_t;
  signal zp_cnt : unsigned(ZPCNT_WIDTH_c-1 downto 0);
  signal idata, idata_r : std_logic_vector(DIN_WIDTH-1 downto 0);
  signal idata_vld, idata_vld_r : std_logic_vector(DIN_WIDTH-1 downto 0);

begin

  -----------------------------------------------------------------------------
  -- Mealy State Machine
  -----------------------------------------------------------------------------

  fsm_comb : process (current_state, NUM_ZEROS, ENABLE, REN, DIN, DIN_VLD, zp_cnt, idata_r, idata_vld_r)
  begin
    BUSY_N    <= '0';
    idata     <= idata_r;
    idata_vld <= idata_vld_r;
    DOUT      <= (others => '0');
    DOUT_VLD  <= '0';

    case current_state is
      when IDLE_s =>
        BUSY_N <= '1';                  -- flag to indicate 'not busy'
        if (ENABLE = '1') then
          idata      <= DIN;
          idata_vld  <= DIN_VLD;
          next_state <= SEND_DATA_s;
        else
          next_state <= IDLE_s;
        end if;

      when SEND_DATA_s =>
        -- send if output ready and data valid
        if (REN = '1' and idata_vld_r(DIN_WIDTH-1) = '1') then
          DOUT_VLD <= '1';
          if idata_r(DIN_WIDTH-1) = '1' then  -- max positive
            DOUT(DOUT_WIDTH-1)           <= '0';
            DOUT(DOUT_WIDTH-2  downto 0) <= (others => '1');
          else                          -- max negative
            DOUT(DOUT_WIDTH-1)           <= '1';
            DOUT(DOUT_WIDTH-2  downto 0) <= (others => '0');
          end if;
          next_state <= ZERO_PAD_s;
        else
          next_state <= SEND_DATA_s;
        end if;
        -- shift to next data and data valid pairing
        if (REN = '1') then
          idata(DIN_WIDTH-1 downto 0)     <= idata_r(DIN_WIDTH-2 downto 0) & '0';
          idata_vld(DIN_WIDTH-1 downto 0) <= idata_vld_r(DIN_WIDTH-2 downto 0) & '0';
        end if;

      when ZERO_PAD_s =>
        if (REN = '1' and zp_cnt < NUM_ZEROS) then
          DOUT_VLD <= '1';
          DOUT     <= (others => '0');
        end if;

        if (zp_cnt < NUM_ZEROS) then
          next_state <= ZERO_PAD_s;
        elsif (unsigned(idata_vld_r) = 0) then
          -- all valid data has been sent
          next_state <= IDLE_s;
        else
          next_state <= SEND_DATA_s;
        end if;

      when others => 
    end case;
  end process;

  fsm_seq : process (CLK_IN, RST_IN)
  begin
    if (RST_IN = '1') then              -- async reset
      current_state <= IDLE_s;
    elsif (rising_edge(CLK_IN)) then
      current_state <= next_state;
    end if;
  end process;

  -----------------------------------------------------------------------------
  -- Zero Padding Counter
  -----------------------------------------------------------------------------

  zp_counter : process (CLK_IN)
  begin
    if (rising_edge(CLK_IN)) then
      if (RST_IN = '1') then
        zp_cnt <= (others => '0');
      elsif (REN = '1' and current_state = ZERO_PAD_s) then
        zp_cnt <= zp_cnt + 1;
      elsif (current_state = SEND_DATA_s) then
        zp_cnt <= (others => '0');
      end if;
    end if;
  end process;

  -----------------------------------------------------------------------------
  -- Register shifted data and data valid
  -----------------------------------------------------------------------------

  process (CLK_IN)
  begin
    if (rising_edge(CLK_IN)) then
      if (RST_IN = '1') then
        idata_r     <= (others => '0');
        idata_vld_r <= (others => '0');
      else
        idata_r     <= idata;
        idata_vld_r <= idata_vld;
      end if;
    end if;
  end process;

end rtl;
