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

-- This module adapts between the SDP and the AXI read data channel
-- It is a subsidiary module to the sdp2axi dma adapter.
-- Data arriving at the AXI read data channel is conveyed to the SDP m2s
-- interface as read responses.
-- When SDP is wider than AXI, the AXI data is buffered to build up to a sxf.
--   the use-case for wider is ???
--   when the clocks are different and we want to slow it down?
-- When SDP is narrower than AXI, the data is staged to the narrower path
--   the use case is when internal stuff is slower than the fixed AXI width.
--   PCI bridge is 64 bits
-- Initial profile is same or narrower
--
-- This AXI channel is basically feeding directly into the SDP m2s channel
library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library platform; use platform.platform_pkg.all;
library ocpi; use ocpi.types.all, ocpi.util.all;
library axi; use axi.axi_pkg.all;
library sdp; use sdp.sdp.all;

entity sdp2axi_rd is
  generic(ocpi_debug   : boolean;
          axi_width    : natural;
          sdp_width    : natural);
  port(   clk          : in  std_logic;
          reset        : in  bool_t;
          sdp_take     : in  bool_t;
          sdp_in       : in  s2m_t;
          sdp_out      : out m2s_t;
          sdp_out_data : out dword_array_t(0 to sdp_width-1);
          axi_in       : in  s_axi_hp_out_r_t;  -- read data channel in to here
          axi_out      : out s_axi_hp_in_r_t;   -- read data channel out from here
          debug        : out ulonglong_t);
end entity sdp2axi_rd;
architecture rtl of sdp2axi_rd is
 signal axi_dw_idx      : unsigned(width_for_max(axi_width-1)-1 downto 0);
 signal axi_dw_idx_r    : unsigned(width_for_max(axi_width-1)-1 downto 0);
 signal last_sxf_in_axf : bool_t;
 signal starting_r      : bool_t;
 signal axi_data        : dword_array_t(0 to axi_width-1);
begin
  axi_dw_idx              <= (others => '0') when its(starting_r) else axi_dw_idx_r;
  last_sxf_in_axf         <= to_bool(not its(starting_r) and axi_dw_idx_r = axi_width - 1);
  axi_out.READY           <= to_bool(sdp_in.sdp.ready and
                                     (sdp_width = axi_width or last_sxf_in_axf));
  sdp_out.clk              <= clk;
  sdp_out.reset            <= reset;
  sdp_out.id               <= (others => '0');
  sdp_out.sdp.header.count <= (others => '0');
  sdp_out.sdp.header.op    <= response_e;
  sdp_out.sdp.header.xid   <= unsigned(axi_in.ID(2 downto 0));
  sdp_out.sdp.header.lead  <= (others => '0');
  sdp_out.sdp.header.trail <= (others => '0');
  sdp_out.sdp.header.node  <= resize(unsigned(axi_in.ID(5 downto 3)), node_width);
  -- This is interesting.  Read responses are all assumed aligned, so the
  -- addressing says the data is aligned in the SDP xfer.  When we support
  -- SDP wider than AXI, and alignment less than SDP width, then we will need to track this
  -- and set the low order addr bits so the receiver knows the alignment of the fragment.
  sdp_out.sdp.header.addr  <= (others => '0');
  sdp_out.sdp.eop          <= to_bool(axi_in.LAST = '1' and
                                     (sdp_width = axi_width or last_sxf_in_axf));
  sdp_out.sdp.valid        <= axi_in.VALID;
  sdp_out.sdp.ready        <= sdp_take;
  g0: for i in 0 to axi_width-1 generate
    axi_data(i) <= axi_in.DATA(i*32+31 downto i*32);
  end generate;
  g1: if sdp_width = axi_width generate
    sdp_out_data <= axi_data;
  end generate;
  g2: if sdp_width < axi_width generate
    g3: for i in 0 to sdp_width-1 generate
      sdp_out_data(i) <= axi_data(to_integer(axi_dw_idx) + i);
    end generate;
  end generate;

  --------------------------------------------------------------------------------
  -- Clocked State processing 
  --------------------------------------------------------------------------------
  doclk : process(clk)
  begin
    if rising_edge(clk) then
      if its(reset) then
        -- We'll list all registers, but comment out those that don't really need init
        starting_r <= btrue;
      elsif axi_in.VALID = '1' then
        if its(starting_r) then
          axi_dw_idx_r <= to_unsigned(1, axi_dw_idx_r'length);
          if its(sdp_in.sdp.ready) then
            starting_r <= bfalse;
          end if;
        elsif sdp_width = axi_width then
          if axi_in.LAST = '1' then
            starting_r <= btrue;
          end if;
        else
          if axi_in.LAST = '1' and axi_dw_idx_r = axi_width - sdp_width then
            starting_r <= btrue;
          else
            axi_dw_idx_r <= axi_dw_idx_r + sdp_width;
          end if;
        end if;
      end if;   -- not reset
    end if;     -- rising edge
  end process;
  ----------------------------------------------
  -- Interface output signals to the sdp interface
  -- Since this is a master-only interface, the only outputs will be read completions
  ----------------------------------------------
  debug <= to_ulonglong(0);

end rtl;
