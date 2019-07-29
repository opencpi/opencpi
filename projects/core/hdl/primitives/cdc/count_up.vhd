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

library IEEE; use IEEE.std_logic_1164.all, IEEE.numeric_std.all;
entity count_up is
  generic (N         : natural   := 2;
           WIDTH     : positive  := 1);
  port    (src_clk : in  std_logic;
           src_rst : in  std_logic;
           src_in  : in  std_logic;
           dst_clk : in  std_logic;
           dst_rst : in  std_logic;           -- optional; if not required, tie '0'
           dst_out : out unsigned(WIDTH-1 downto 0));
end count_up;
architecture rtl of count_up is
  signal counter_r : unsigned(width-1 downto 0);
  signal increment : std_logic;
begin
  dst_out <= counter_r;
  counting : process(dst_clk)
  begin
    if rising_edge(dst_clk) then
      if dst_rst = '1' then
        counter_r <= (others => '0');
      elsif increment = '1' then
        counter_r <= counter_r + 1;
      end if;
    end if;
  end process;
  pulse_i : component work.cdc.pulse
    generic map(N       => N)
    port map   (src_clk => src_clk,
                src_rst => src_rst,
                src_in  => src_in,
                dst_clk => dst_clk,
                dst_rst => dst_rst,
                dst_out => increment);
end architecture rtl;
