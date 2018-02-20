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

library IEEE;
use IEEE.std_logic_1164.all, IEEE.numeric_std.all;
library unisim; use unisim.vcomponents.all;

entity clock_forward is
  generic (
    INVERT_CLOCK : boolean := false;
    SINGLE_ENDED : boolean := true
  );
  port (
    RST       : in  std_logic;
    CLK_IN    : in  std_logic;
    CLK_OUT_P : out std_logic;
    CLK_OUT_N : out std_logic
  );
end entity clock_forward;

architecture rtl of clock_forward is

  signal clk_fwd : std_logic;

begin

  noninverted_clock : if (INVERT_CLOCK = false) generate
    ODDR_inst : ODDR
      generic map (
        DDR_CLK_EDGE => "OPPOSITE_EDGE", -- "OPPOSITE_EDGE" or "SAME_EDGE"
        INIT         => '0',             -- Initial value for Q port ('1' or '0')
        SRTYPE       => "SYNC"           -- Reset Type ("ASYNC" or "SYNC")
      )
      port map (
        Q  => clk_fwd, -- 1-bit DDR output
        C  => CLK_IN,  -- 1-bit clock input
        CE => '1',     -- 1-bit clock enable input
        D1 => '1',     -- 1-bit data input (positive edge)
        D2 => '0',     -- 1-bit data input (negative edge)
        R  => RST,     -- 1-bit reset input
        S  => '0'      -- 1-bit set input
      );
  end generate noninverted_clock;

  inverted_clock : if (INVERT_CLOCK = true) generate
    ODDR_inst : ODDR
      generic map (
        DDR_CLK_EDGE => "OPPOSITE_EDGE", -- "OPPOSITE_EDGE" or "SAME_EDGE"
        INIT         => '0',             -- Initial value for Q port ('1' or '0')
        SRTYPE       => "SYNC"           -- Reset Type ("ASYNC" or "SYNC")
      )
      port map (
        Q  => clk_fwd, -- 1-bit DDR output
        C  => CLK_IN,  -- 1-bit clock input
        CE => '1',     -- 1-bit clock enable input
        D1 => '0',     -- 1-bit data input (positive edge)
        D2 => '1',     -- 1-bit data input (negative edge)
        R  => RST,     -- 1-bit reset input
        S  => '0'      -- 1-bit set input
      );
  end generate inverted_clock;

  diff_clk_out : if (SINGLE_ENDED = false) generate
    OBUFDS_inst : OBUFDS
      generic map (
        IOSTANDARD => "DEFAULT"
      )
      port map (
        O  => CLK_OUT_P,
        OB => CLK_OUT_N,
        I  => clk_fwd
    );
  end generate diff_clk_out;

  single_clk_out : if (SINGLE_ENDED = true) generate
    OBUF_inst : OBUF
      port map (
        I => clk_fwd,
        O => CLK_OUT_P
      );

    CLK_OUT_N <= '0';

  end generate single_clk_out;

end rtl;
