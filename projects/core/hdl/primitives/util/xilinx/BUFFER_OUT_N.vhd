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

-- Xilinx-specific output buffer, supports both single ended and differential
-- iostandards
library IEEE;
use IEEE.std_logic_1164.all, ieee.numeric_std.all;
library util; use util.types.all;
library unisim; use unisim.vcomponents.all;
entity BUFFER_OUT_N is
  generic (width        :   natural;
           IOSTANDARD   :   iostandard_t := UNSPECIFIED;
           DIFFERENTIAL :   boolean); -- only used if IOSTANDARD is UNSPECIFIED
  port (   I          : in  std_logic_vector(width-1 downto 0);
           O          : out std_logic_vector(width-1 downto 0);
           OBAR       : out std_logic_vector(width-1 downto 0) := (others => 'X')); -- only use if relevant to IOSTANDARD
end entity BUFFER_OUT_N;
architecture rtl of BUFFER_OUT_N is
begin

  -- technology: unspecified, supply voltage: unspecified
  UNSPECIFIED_gen : if IOSTANDARD = UNSPECIFIED generate
    single_ended_prim : if DIFFERENTIAL = false generate
      gen_loop : for idx in width-1 downto 0 generate
        buf : OBUF
          port map(
            O  => O(idx),
            I  => I(idx));
      end generate;
    end generate;

    differential_prim : if DIFFERENTIAL = true generate
      gen_loop : for idx in width-1 downto 0 generate
        buf : OBUFDS
          port map(
            O  => O(idx),
            OB => OBAR(idx),
            I  => I(idx));
      end generate;
    end generate;
  end generate;

  -- technology: CMOS, supply voltage: 1.8V
  CMOS18_gen : if IOSTANDARD = CMOS18 generate
    gen_loop : for idx in width-1 downto 0 generate
      buf : OBUF
        generic map(
          IOSTANDARD => "LVCMOS18")
        port map(
          O  => O(idx),
          I  => I(idx));
    end generate;
  end generate;

  -- technology: CMOS, supply voltage: 2.5V
  CMOS25_gen : if IOSTANDARD = CMOS25 generate
    gen_loop : for idx in width-1 downto 0 generate
      buf : OBUF
        generic map(
          IOSTANDARD => "LVCMOS25")
        port map(
          O  => O(idx),
          I  => I(idx));
    end generate;
  end generate;

  -- technology: LVDS (TIA/EIA-644 specification), supply voltage: 2.5V
  LVDS25_gen : if IOSTANDARD = LVDS25 generate
    gen_loop : for idx in width-1 downto 0 generate
      buf : OBUFDS
        generic map(
          IOSTANDARD => "LVDS_25")
        port map(
          O  => O(idx),
          OB => OBAR(idx),
          I  => I(idx));
    end generate;
  end generate;

end rtl;

