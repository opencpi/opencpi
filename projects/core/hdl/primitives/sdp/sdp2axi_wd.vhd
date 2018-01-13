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

-- This module adapts between the SDP and the AXI write data channel
-- It is a subsidiary module to the sdp2axi dma adapter.
library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library platform; use platform.platform_pkg.all;
library ocpi; use ocpi.types.all, ocpi.util.all;
library axi; use axi.axi_pkg.all;
library sdp; use sdp.sdp.all;

entity sdp2axi_wd is
  generic(ocpi_debug      : boolean;
          axi_width       : natural;
          sdp_width       : natural);
  port(   clk             : in  std_logic;
          reset           : in  bool_t;
          addressing_done : in  bool_t;           -- addressing is done in/before this cycle
          pipeline        : in  bool_t;           -- our pipeline state is full
          sdp             : in  sdp_t;
          sdp_in_data     : in  dword_array_t(0 to sdp_width-1);
          sdp_p           : in  sdp_t;
          axi_in          : in  s_axi_hp_out_w_t; -- write data channel in to here
          axi_out         : out s_axi_hp_in_w_t;  -- write data channel out from here
          taking_data     : out bool_t;           -- indicate data is being used.
          writing_done    : out bool_t;           -- indicate all data taken.
          debug           : out ulonglong_t);
end entity sdp2axi_wd;
architecture rtl of sdp2axi_wd is
  -- These ensure no truncation during arithmetic...
  constant axi_width_u : unsigned(width_for_max(axi_width)-1 downto 0) :=
    to_unsigned(axi_width, width_for_max(axi_width));
  constant sdp_width_u : unsigned(width_for_max(sdp_width)-1 downto 0) :=
    to_unsigned(sdp_width, width_for_max(sdp_width));
  subtype ndw_in_sxf_t is unsigned(width_for_max(sdp_width)-1 downto 0);
  type write_state_t is (sop_next_e, -- valid is start of packet
                         capture_e,  -- capturing an sxf to build an axf
                         offer_e,    -- offering an axf to write channel)
                         waiting_e); -- waiting for addressing machine to be done with packet
  signal write_state_r            : write_state_t;
  subtype dw_idx_in_axf_t         is unsigned(width_for_max(axi_width-1)-1 downto 0);
  subtype ndws_in_axf_t           is unsigned(width_for_max(axi_width)-1 downto 0);
  subtype pkt_naxf_t              is unsigned(width_for_max(max_pkt_dws/axi_width + 1)-1 downto 0);
  signal pkt_starting             : bool_t;
  signal pkt_ndws_0               : pkt_ndw_t; -- count of dws in an SDP pkt
  signal ndws_this_axf            : ndws_in_axf_t;
  signal pkt_first_axf_dw_offset  : dw_idx_in_axf_t;
  signal pkt_first_axf_dw_offset_0 : dw_idx_in_axf_t;
  signal pkt_first_sxf_dw_offset  : dw_idx_in_axf_t;
  signal pkt_first_axf_sxf_offset : dw_idx_in_axf_t;
  signal pkt_last_axf_dw_offset_0 : dw_idx_in_axf_t;
  signal pkt_first_axf_ndws_0     : ndws_in_axf_t;
  signal pkt_first_sxf_ndws       : ndws_in_axf_t;
  signal pkt_is_first_axf         : bool_t;
  signal pkt_is_first_axf_r       : bool_t;
  signal pkt_ndws_left            : pkt_ndw_t;
  signal pkt_ndws_left_after      : pkt_ndw_t;
  signal pkt_ndws_left_r          : pkt_ndw_t;
  signal pkt_is_last_axf          : bool_t;
  signal axf_ndws                 : ndws_in_axf_t;
  signal axf_first_dw             : dw_idx_in_axf_t;
  signal axf_last_dw              : dw_idx_in_axf_t;
  signal last_sxf_offset_in_axf   : dw_idx_in_axf_t;
  signal axi_data_idx_r           : dw_idx_in_axf_t;
  signal axi_data_r               : dword_array_t(0 to (axi_width - sdp_width) - 1);
  signal axi_data                 : dword_array_t(0 to axi_width - 1);
  signal capturing_first          : bool_t;
  signal capturing_next_axf       : bool_t;
  signal capturing_next_sxf       : bool_t;
  signal can_take_sdp             : bool_t;
  signal axi_valid                : bool_t;
  signal axi_accepting_data       : bool_t;
  signal sdp_valid_write          : bool_t;
  constant axi_width_bytes        : natural := axi_width * dword_bytes;
  subtype axi_byte_t is unsigned(width_for_max(axi_width_bytes-1)-1 downto 0);
  subtype axi_bytes_t is unsigned(width_for_max(axi_width_bytes)-1 downto 0);
  subtype axi_strobe_t is std_logic_vector(axi_width_bytes-1 downto 0);
  signal axi_strobe : axi_strobe_t;
  impure function strobe(first, last : bool_t;
                         first_dw_offset, last_dw_offset : dw_idx_in_axf_t;
                         axf_ndws : ndws_in_axf_t) return axi_strobe_t is
    variable r      : unsigned (axi_strobe_t'length-1 downto 0);
    variable nbytes : axi_bytes_t := shift_left(resize(axf_ndws, axi_bytes_t'length),
                                                dword_shift);
    variable start  : axi_bytes_t := (others => '0');
  begin
    if its(first) then
      start := shift_left(resize(first_dw_offset, axi_bytes_t'length), dword_shift) +
               sdp_p.header.lead;
      nbytes := nbytes - sdp_p.header.lead;
    end if;
    if its(last) then
      nbytes := nbytes - sdp_p.header.trail;
    end if;
    r := (others => '1');
    r := not shift_left(r, to_integer(nbytes));
    r := shift_left(r, to_integer(start));
--    for i in to_integer(start) to to_integer(start + nbytes -1) loop
--      r(i) := '1';
--    end loop;
    return std_logic_vector(r);
  end strobe;
  function fyv(b : std_logic) return std_logic_vector is
  variable v : std_logic_vector(0 downto 0);
  begin
    v(0) := b;
    return v;
  end fyv;
  -- pipeline versions of combinatorial values (sdp_p is fed to us).
  signal sdp_data_p : dword_array_t(0 to sdp_width-1);
  signal pkt_ndws_p : pkt_ndw_t;
  signal pkt_first_axf_ndws_p : ndws_in_axf_t;
  signal pkt_last_axf_dw_offset_p : dw_idx_in_axf_t;
begin
  ---------------------------------------------------------------------------------------
  -- Route the sdp data to the axi data, allowing the last sxf needed to pass through
  ---------------------------------------------------------------------------------------
  sdp_valid_write          <= to_bool(sdp_p.valid and sdp_p.header.op = write_e);
  -- Packet calculations
  pkt_starting             <= to_bool(write_state_r = sop_next_e and sdp_p.valid);
  pkt_ndws_0               <= count_in_dws(sdp.header);
  pkt_first_axf_dw_offset_0 <= sdp.header.addr(width_for_max(axi_width-1)-1 downto 0);
  pkt_first_axf_dw_offset  <= sdp_p.header.addr(width_for_max(axi_width-1)-1 downto 0);
  pkt_first_sxf_dw_offset  <= sdp_p.header.addr(width_for_max(ocpi.util.max(1,sdp_width-1))-1
                                                downto 0);
  pkt_first_axf_sxf_offset <= sdp_p.header.addr(width_for_max(axi_width-1)-1 downto 0) and
                               not to_unsigned(sdp_width - 1, width_for_max(axi_width-1));
  pkt_first_axf_ndws_0     <= resize(ocpi.util.min(axi_width_u - pkt_first_axf_dw_offset_0,
                                                   pkt_ndws_0),
                                     pkt_first_axf_ndws_0'length);
  pkt_first_sxf_ndws       <= resize(ocpi.util.min(sdp_width_u - pkt_first_sxf_dw_offset,
                                                   pkt_ndws_p),
                                     pkt_first_sxf_ndws'length);
  pkt_last_axf_dw_offset_0 <= resize(((pkt_ndws_0 + pkt_first_axf_dw_offset_0 - 1) and
                                      to_unsigned(axi_width-1, pkt_last_axf_dw_offset_0'length)),
                                      pkt_last_axf_dw_offset_0'length);
  pkt_is_first_axf         <= pkt_starting or pkt_is_first_axf_r;
  pkt_ndws_left            <= pkt_ndws_p when its(pkt_is_first_axf) else pkt_ndws_left_r;
  axf_ndws                 <= pkt_first_axf_ndws_p when its(pkt_is_first_axf) else
                              resize(ocpi.util.min(pkt_ndws_left, axi_width), axf_ndws'length);
  pkt_ndws_left_after      <= pkt_ndws_left - axf_ndws;
  pkt_is_last_axf          <= to_bool(pkt_ndws_left <= axi_width);
  -- Current axf calculations
  -- Do we capture the first sxf for a wider axf?
  capturing_first           <= to_bool(axi_width > sdp_width and not its(sdp_p.eop) and
                                       pkt_first_axf_dw_offset < axi_width - sdp_width);
  -- The current axf is being accepted, should the next sxf be captured?
  capturing_next_axf        <= to_bool(axi_width > sdp_width and not its(sdp_p.eop) and
                                       pkt_ndws_left_after > sdp_width);
  -- The current sxf is being captured, should the next sxf also be captured?
  capturing_next_sxf        <= to_bool(axi_width > sdp_width and 
                                       (axi_width_u - (axi_data_idx_r + sdp_width_u)) > sdp_width_u);
  axf_first_dw              <= pkt_first_axf_dw_offset when its(pkt_is_first_axf)
                               else (others => '0');
  axf_last_dw               <= resize(axf_first_dw + axf_ndws - 1, axf_last_dw'length);
  last_sxf_offset_in_axf    <= axf_last_dw and not to_unsigned(sdp_width - 1, axf_last_dw'length);
  g0: if axi_width > sdp_width generate
    g1: for i in 0 to (axi_width-sdp_width)-1 generate -- for all but the last ones
      axi_data(i) <=
        axi_data_r(i)                            -- buffered
        when i < last_sxf_offset_in_axf else
        sdp_data_p(to_integer(i - last_sxf_offset_in_axf)); -- pass through
    end generate;
    axi_data(axi_width - sdp_width to (axi_width - sdp_width) + (sdp_width - 1)) <= sdp_data_p;
  end generate;
  -- When they are the same size, its all pass-through
  g2: if axi_width = sdp_width generate
    axi_data <= sdp_data_p;
  end generate;
  axi_strobe     <= strobe(pkt_is_first_axf, pkt_is_last_axf, pkt_first_axf_dw_offset,
                           pkt_last_axf_dw_offset_p, axf_ndws);
  with write_state_r select
    axi_valid <= bfalse                                    when capture_e | waiting_e,
                 sdp_valid_write and not capturing_first when sop_next_e,
                 btrue                                     when offer_e;
  axi_accepting_data <= axi_in.READY and axi_valid;
  -- when are we ready to take the SDP data if it is available? (if sdp.valid)
  with write_state_r select
    can_take_sdp <= btrue                               when capture_e,
                    capturing_first or axi_in.READY     when sop_next_e,
                    axi_in.READY                        when offer_e,
                    bfalse                              when waiting_e;
  taking_data    <= sdp_valid_write and can_take_sdp;
  --------------------------------------------------------------------------------
  -- Clocked processing, building AXFs and feeding the write data channel.
  -- Since SDP may be narrower than AXI, this process is a combination of
  -- building up the axf, and feeding the write data channel
  --------------------------------------------------------------------------------
  writing_done <= to_bool(write_state_r = waiting_e or its(axi_accepting_data and sdp_p.eop));
  doclk : process(clk)
   procedure check_eop is
   begin
     pkt_is_first_axf_r <= bfalse;
     if its(sdp_p.eop) then
       if its(addressing_done) then
         write_state_r <= sop_next_e;
       else
         write_state_r <= waiting_e;
       end if;
     else
       pkt_ndws_left_r <= pkt_ndws_left - axf_ndws;
       if its(capturing_next_axf) then
         axi_data_idx_r <= (others => '0');
         write_state_r  <= capture_e;
       else
         write_state_r  <= offer_e;
       end if;
     end if;
   end procedure check_eop;
  begin
    if rising_edge(clk) then
      if its(reset) then
        write_state_r      <= sop_next_e;
        pkt_is_first_axf_r <= bfalse;
      elsif write_state_r = waiting_e then
        if its(addressing_done) then
          write_state_r <= sop_next_e;
        end if;
      elsif its(sdp_valid_write) then
        case write_state_r is
          when sop_next_e =>
            pkt_is_first_axf_r <= btrue;
            pkt_ndws_left_r    <= pkt_ndws_p - axf_ndws;
            axi_data_idx_r     <= pkt_first_axf_sxf_offset;
            -- capture when aw > sw and more to come and not high sw in aw
--            axi_data_r(to_integer(pkt_first_axf_sxf_offset) to
--                       to_integer(pkt_first_axf_sxf_offset + (sdp_width - 1)))
            axi_data_r <= sdp_data_p; -- FIXME - the indexing above does not synthesize ?
            if its(capturing_first) then
              if pkt_first_axf_ndws_p - pkt_first_sxf_ndws > sdp_width then
                write_state_r <= capture_e;
              else
                write_state_r <= offer_e;
              end if;
            elsif its(axi_accepting_data) then
              check_eop;
            else
              write_state_r <= offer_e;
            end if;
          when capture_e =>
--            axi_data_r(to_integer(axi_data_idx_r) to
--                     to_integer(axi_data_idx_r + (sdp_width - 1))) <= sdp_in_data;
            axi_data_r <= sdp_data_p;
            axi_data_idx_r <= axi_data_idx_r + sdp_width_u;
            if not capturing_next_sxf then
              write_state_r <= offer_e;
            end if;
          when offer_e =>
            if its(axi_accepting_data) then
              check_eop;
            end if;
          when waiting_e => null;
        end case;
      end if; -- end of valid write
    end if; -- end of rising edge
  end process;

  -----------------------------------------------------------------
  -- Process added for pipelining/fmax
  -----------------------------------------------------------------
  dopipe : process(clk)
  begin
    if rising_edge(clk) then
      if its(reset) then
        pkt_ndws_p                <= (others => '0');
        pkt_first_axf_ndws_p      <= (others => '0');
        pkt_last_axf_dw_offset_p  <= (others => '0');
      elsif its(pipeline) then
        pkt_ndws_p                <= pkt_ndws_0;
        pkt_first_axf_ndws_p      <= pkt_first_axf_ndws_0;
        pkt_last_axf_dw_offset_p  <= pkt_last_axf_dw_offset_0;
        sdp_data_p                <= sdp_in_data;
       end if; 
    end if;
  end process;

  -----------------------------------------------------------------
  -- Interface outputs to the S_AXI_HP write data channel interface
  -----------------------------------------------------------------
  -- Write data channel
  axi_out.ID                 <= (others => '0');  -- spec says same id means in-order
  g3: for i in 0 to axi_width-1 generate -- for all but the last ones
    axi_out.DATA(i*32+31 downto i*32) <= axi_data(i); -- swap(axi_data(i));
--      axi_data_r(i) when i < last_sxf_offset_in_axf else
--      sdp_in_data(i - to_integer(last_sxf_offset_in_axf));
--      swap(axi_data_r(i)) when i < last_sxf_offset_in_axf else
--      swap(sdp_in_data(i - to_integer(last_sxf_offset_in_axf)));
  end generate;
  axi_out.STRB               <= axi_strobe;
  axi_out.LAST               <= pkt_is_last_axf;
  axi_out.VALID              <= axi_valid;
  -----------------------------------------------------------------
  -- Debug status output - just wires here.
  -----------------------------------------------------------------
  debug <= (others => '0');
--  debug <= to_ulonglong(
--  "000" &
--  slv(pkt_ndws_left) &
--  "000" &
--  slv(pkt_ndws) &
---- nibble 7
--  slv(pkt_first_axf_sxf_offset) & -- 1
--  slv(last_sxf_offset_in_axf) & -- 1
--  slv(pkt_first_axf_ndws) & -- 2
---- nibble 6
--slv(axf_ndws) &
--slv(pkt_last_axf_dw_offset) & "0" &
---- nibble 4+5
--slv(axi_strobe) & -- 8
---- nibble 3
--slv(to_unsigned(write_state_t'pos(write_state_r),2)) & --2
--slv(axi_data_idx_r) & --1
--"0" & --1
---- nibble 2
--slv(capturing_next_sxf) & --1
--slv(axf_last_dw) & --1
--slv(axf_ndws) & -- 2
----slv(pkt_first_sxf_dw_offset) & -- 1
----slv(pkt_first_axf_dw_offset) &  -- 1
---- nibble 1
--fyv(sdp_valid_write) & -- 1
--fyv(pkt_starting) & -- 1
--fyv(pkt_is_first_axf) & -- 1
--fyv(pkt_is_last_axf) & -- 1
---- nibble 0
--fyv(capturing_first) & -- 1
--fyv(capturing_next_axf) & -- 1
--slv(pkt_first_axf_sxf_offset) & -- 1
--fyv(axi_accepting_data) -- 1
--);

end rtl;
