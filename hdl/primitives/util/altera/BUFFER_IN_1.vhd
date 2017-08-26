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

-- Altera-specific input buffer, supports both single ended and differential
library IEEE;
use IEEE.std_logic_1164.all, ieee.numeric_std.all;
library util; use util.types.all;
library altera; use altera.altera_primitives_components.all;
entity BUFFER_IN_1 is
  generic (IOSTANDARD   :     iostandard_t := UNSPECIFIED;
           DIFFERENTIAL :     boolean; -- only used if IOSTANDARD is UNSPECIFIED
           GLOBAL_CLOCK :     boolean       := FALSE);
  port (   I            : in  std_logic             ;
           IBAR         : in  std_logic     := 'X'  ; -- only used if relevant to IOSTANDARD
           O            : out std_logic             );
end entity BUFFER_IN_1;
architecture rtl of BUFFER_IN_1 is
  signal tmp : std_logic;
begin

  -- technology: unspecified, supply voltage: unspecified
  UNSPECIFIED_gen : if IOSTANDARD = UNSPECIFIED generate
    single_ended_prim : if DIFFERENTIAL = false generate
      buf_generic : if GLOBAL_CLOCK = false generate
        buf : ALT_INBUF
          port map(
            I  => I,
            O  => O); -- equivalent to output of Xilinx IBUF
      end generate;

      buf_clk : if GLOBAL_CLOCK = true generate
        buf : ALT_INBUF
          port map(
            I  => I,
            O  => tmp);
        buf_global : GLOBAL
          port map(
            A_IN  => tmp,
            A_OUT => O); -- equivalent to output of Xilinx IBUFG
      end generate;
    end generate;

    differential_prim : if DIFFERENTIAL = true generate
      buf_generic : if GLOBAL_CLOCK = false generate
        buf : ALT_INBUF_DIFF
          port map(
            I    => I,
            IBAR => IBAR,
            O    => O); -- equivalent to output of Xilinx IBUFDS
      end generate;

      buf_clk : if GLOBAL_CLOCK = true generate
        buf : ALT_INBUF_DIFF
          port map(
            I    => I,
            IBAR => IBAR,
            O    => tmp);
        buf_global : GLOBAL
          port map(
            A_IN  => tmp,
            A_OUT => O); -- equivalent to output of Xilinx IBUFGDS
      end generate;
    end generate;
  end generate;

  -- technology: CMOS, supply voltage: 1.8V
  CMOS18_gen : if IOSTANDARD = CMOS18 generate
    buf_generic : if GLOBAL_CLOCK = false generate
      buf : ALT_INBUF
        generic map(
          IO_STANDARD => "1.8 V")
        port map(
          I  => I,
          O  => O); -- equivalent to output of Xilinx IBUF
    end generate;

    buf_clk : if GLOBAL_CLOCK = true generate
      buf : ALT_INBUF
        generic map(
          IO_STANDARD => "1.8 V")
        port map(
          I  => I,
          O  => tmp);
      buf_global : GLOBAL
        port map(
          A_IN  => tmp,
          A_OUT => O); -- equivalent to output of Xilinx IBUFG
    end generate;
  end generate;

  -- technology: CMOS, supply voltage: 2.5V
  CMOS25_gen : if IOSTANDARD = CMOS25 generate
    buf_generic : if GLOBAL_CLOCK = false generate
      buf : ALT_INBUF
        generic map(
          IO_STANDARD => "2.5 V")
        port map(
          I  => I,
          O  => O); -- equivalent to output of Xilinx IBUF
    end generate;

    buf_clk : if GLOBAL_CLOCK = true generate
      buf : ALT_INBUF
        generic map(
          IO_STANDARD => "2.5 V")
        port map(
          I  => I,
          O  => tmp);
      buf_global : GLOBAL
        port map(
          A_IN  => tmp,
          A_OUT => O); -- equivalent to output of Xilinx IBUFG
    end generate;
  end generate;

  -- technology: LVDS (TIA/EIA-644 specification), supply voltage: 2.5V
  LVDS25_gen : if IOSTANDARD = LVDS25 generate
    buf_generic : if GLOBAL_CLOCK = false generate
      buf : ALT_INBUF_DIFF
        generic map(
          IO_STANDARD  => "LVDS")
        port map(
          I    => I,
          IBAR => IBAR,
          O    => O); -- equivalent to output of Xilinx IBUFDS
    end generate;

    buf_clk : if GLOBAL_CLOCK = true generate
      buf : ALT_INBUF_DIFF
        generic map(
          IO_STANDARD  => "LVDS")
        port map(
          I    => I,
          IBAR => IBAR,
          O    => tmp);
      buf_global : GLOBAL
        port map(
          A_IN  => tmp,
          A_OUT => O); -- equivalent to output of Xilinx IBUFGDS
    end generate;
  end generate;

end rtl;

