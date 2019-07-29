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

-- see the comments on sdp2cp.vhd
-- this version adds a CDC fifo on the CP side so that it will accept a control cloc and reset
-- that is different from SDP.
-- I.e. platforms that simply want to inject a different clock than the SDP clock as control clock
-- can use this to do that.

library IEEE; use IEEE.std_logic_1164.all, IEEE.numeric_std.all;
library ocpi; use ocpi.all, ocpi.types.all;
library platform, cdc;
entity sdp2cp_clk_rv is
  generic(
    sdp_width    : in   uchar_t := to_uchar(1));
  port(
    wci_Clk               : in   std_logic;
    wci_Reset_n           : in   std_logic;
    cp_in   : in  platform.platform_pkg.occp_out_t;
    cp_out  : out platform.platform_pkg.occp_in_t;
    sdp_in  : in  work.sdp.m2s_t;
    sdp_out : out work.sdp.s2m_t;
    sdp_in_data : in  dword_array_t(0 to to_integer(sdp_width)-1);
    sdp_out_data : out dword_array_t(0 to to_integer(sdp_width)-1));
end entity sdp2cp_clk_rv;
library IEEE, ocpi, sdp, platform;
use IEEE.std_logic_1164.all, ieee.numeric_std.all, ieee.math_real.all, ocpi.types.all,
  sdp.sdp.all, ocpi.util.all, platform.platform_pkg.all;
architecture rtl of sdp2cp_clk_rv is
  constant addr_shift   : natural := natural(log2(real(datum_bytes/4)));
  constant hi_dw        : natural := to_integer(sdp_width) - 1;
  type address_state_t is (a_idle_e,   -- nothing is happening
                           a_first_e,  -- first address (of two) is being offered to cp
                           a_last_wanted_e,
                           a_last_e);  -- last address is offered to cp
  type read_state_t    is (r_idle_e,         -- nothing is happening
                           r_first_valid_e,  -- first data is offered, not accepted
                           r_last_wanted_e,  -- waiting for last response
                           r_last_valid_e);  -- last is offered, not accepted
  signal a_state_r         : address_state_t;
  signal r_state_r         : read_state_t;
  signal c2s_dword_r       : dword_t;
  signal s2c_dwords_r      : dword_array_t(1 downto 0);
  signal in_second_dw_r    : bool_t;
  signal in_read_r         : bool_t;
  signal addr_r            : occp_address_t;
  signal be_r              : occp_byte_en_t;
  signal tag_r             : occp_tag_t;
  signal in_64_r           : bool_t;
  signal hdr               : header_t;
  subtype start_t is unsigned(width_for_max(to_integer(sdp_width))-1 downto 0);
  -- signals to cp in the sdp clock domain
  signal cp_out_data       : std_logic_vector(cp_out.data'range);
  signal cp_out_valid      : std_logic;
  signal cp_out_take       : std_logic;
  signal cp_in_valid       : std_logic;
  signal cp_in_take        : std_logic;
  signal cp_in_tag         : std_logic_vector(cp_in.tag'range);
  signal cp_in_data        : std_logic_vector(cp_in.data'range);

  -- We give the CP a tag that we will get back in read responses.
  -- It tells us enough when we get it back from the CP to form the proper SDP response and
  -- means we don't have to remember anything about a request when we issue it.
  function make_tag(xid : xid_t; first_of_2 : bool_t; startdw : start_t) return occp_tag_t is
    variable tag : occp_tag_t := (others => '0');
  begin
    tag(xid'left downto 0)                               := std_logic_vector(xid);
    tag(xid'length)                                      := std_logic(first_of_2);
    tag(tag'left downto tag'left - (startdw'length - 1)) := std_logic_vector(startdw);
    return tag;
  end make_tag;
  function tag_xid(tag : occp_tag_t) return xid_t is
  begin
    return xid_t(tag(xid_t'left downto 0));
  end tag_xid;
  function tag_startdw(tag : occp_tag_t) return start_t is
  begin
    return start_t(tag(occp_tag_t'left downto occp_tag_t'left - (start_t'length - 1)));
  end tag_startdw;
  function tag_first_of_2(tag : occp_tag_t) return bool_t is
  begin
    return to_bool(tag(xid_t'length));
  end tag_first_of_2;
begin
  hdr            <= sdp_in.sdp.header;  
-- cp_out.clk     <= sdp_in.clk;
-- cp_out.reset   <= sdp_in.reset;
  cp_out_valid   <= to_bool(a_state_r /= a_idle_e and a_state_r /= a_last_wanted_e);
--  cp_out.is_read <= in_read_r;
--  cp_out.address <= addr_r;
--  cp_out.byte_en <= be_r;
  cp_out_data    <= slv(tag_r, dword_size) when its(in_read_r) else
                    s2c_dwords_r(hi_dw) when a_state_r = a_last_e and in_64_r else
                    s2c_dwords_r(0);
  -- take read data from the CP if it will be accepted by the SDP. We don't pipeline
  cp_out_take    <= to_bool(cp_in_valid and
                            ((r_state_r = r_idle_e and
                              tag_first_of_2(cp_in_tag) and sdp_width /= 1) or
                             sdp_in.sdp.ready));
  sdp_out.sdp.header.op    <= response_e;
  sdp_out.sdp.header.count(count_width-1 downto 1) <= (others => '0');
  sdp_out.sdp.header.count(0) <= to_bool(its(tag_first_of_2(cp_in_tag)) or
                                         r_state_r = r_last_wanted_e or
                                         r_state_r = r_last_valid_e);
  sdp_out.sdp.header.xid      <= tag_xid(cp_in_tag);
  sdp_out.sdp.header.lead     <= (others => '0');
  sdp_out.sdp.header.trail    <= (others => '0');
  sdp_out.sdp.header.node     <= (others => '0'); -- CP is always node zero
  sdp_out.sdp.header.addr     <= (others => '0');
  sdp_out.sdp.header.extaddr  <= (others => '0');
  sdp_out.sdp.valid           <= to_bool(
-- no zero-latency reads...             (r_state_r = r_idle_e and its(cp_in.valid) and
--                                       not (tag_first_of_2(cp_in.tag) and sdp_width /= 1)) or
                                      r_state_r = r_first_valid_e or
                                      (r_state_r = r_last_wanted_e and its(cp_in_valid)) or
                                      r_state_r = r_last_valid_e);
  sdp_out.sdp.eop    <= not tag_first_of_2(cp_in_tag);
                        -- r_state_r = r_last_wanted_e or r_state_r = r_last_valid_e;
  sdp_out.sdp.ready  <= to_bool(sdp_in.sdp.valid and
                                (a_state_r = a_idle_e or
                                 (a_state_r = a_first_e and cp_in_take) or
                                 a_state_r = a_last_wanted_e));
  sdp_out.dropCount  <= (others => '0');
g0: for i in 0 to to_integer(sdp_width)-1 generate
  sdp_out_data(i) <= cp_in_data when tag_startdw(cp_in_tag) = i else (others => '0');
  end generate g0;

  -- Our state machines, separate for address and read-data
  work : process(sdp_in.clk)
  begin
    if rising_edge(sdp_in.clk) then
      if sdp_in.reset = '1' then
        in_second_dw_r <= bfalse;
        in_read_r      <= bfalse;
        a_state_r      <= a_idle_e;
        r_state_r      <= r_idle_e;
        c2s_dword_r    <= (others => '0');
        addr_r         <= (others => '0');
      else
        case a_state_r is
          when a_idle_e =>
            -- Capture the request immediately to free up the SDP
            if its(sdp_in.sdp.valid) then
              addr_r          <= occp_address_t(hdr.addr(addr_r'left downto 0));
              be_r            <= header2be(hdr, to_unsigned(0,1));
              in_read_r       <= to_bool(hdr.op = read_e);
              in_64_r         <= to_bool(count_in_dws(hdr) = 2);
              s2c_dwords_r(0) <= sdp_in_data(start_dw(hdr, sdp_width));
              a_state_r       <= a_last_e;
              tag_r           <= make_tag(hdr.xid,
                                          to_bool(count_in_dws(hdr) = 2),
                                          to_unsigned(start_dw(hdr, sdp_width), start_t'length));
              if count_in_dws(hdr) = 2 then
                if sdp_width /= 1 then
                  s2c_dwords_r(1) <= sdp_in_data(start_dw(hdr, sdp_width) + 1);
                end if;
                a_state_r <= a_first_e;
              end if;
            end if;
          when a_first_e => -- First of two is being read or written
            if its(cp_in_take) then
              addr_r(0) <= '1';
              a_state_r <= a_last_e;
              tag_r <= make_tag(tag_xid(tag_r), bfalse,
                                tag_startdw(tag_r) + (1 - bit2unsigned(sdp_width(0))));
              if sdp_width = 1 and not its(in_read_r) then
                if its(sdp_in.sdp.valid) then -- this must be the second word
                  s2c_dwords_r(0) <= sdp_in_data(0);
                else
                  a_state_r <= a_last_wanted_e;
                end if;
              end if;
            end if;
          when a_last_wanted_e =>
            if its(sdp_in.sdp.valid) then
              s2c_dwords_r(0) <= sdp_in_data(0);
              a_state_r <= a_last_e;
            end if;
          when a_last_e =>
            if its(cp_in_take) then
              a_state_r <= a_idle_e;
            end if;
        end case;
        -- The read side state machine.
        -- We put the burden on the CP to feed us back a tag so we don't need
        -- to remember anything about the transaction.
        case r_state_r is
          when r_idle_e =>
            if its(cp_in_valid) then
              if its(tag_first_of_2(cp_in_tag)) then
                if sdp_width /= 1 then
                  -- just buffering it
                  r_state_r   <= r_last_wanted_e;
                  c2s_dword_r <= cp_in_data;
                elsif its(sdp_in.sdp.ready) then
                  r_state_r <= r_last_wanted_e;
                else
                  r_state_r <= r_first_valid_e;
                end if;
              elsif not its(sdp_in.sdp.ready) then
                r_state_r <= r_first_valid_e;
              end if;
            end if;
          when r_first_valid_e => -- waiting for sdp to be ready
            if its(sdp_in.sdp.ready) then -- saved first dword is taken
              if its(tag_first_of_2(cp_in_tag)) then
                r_state_r <= r_last_wanted_e;
              else
                r_state_r <= r_idle_e;
              end if;
            end if;
          when r_last_wanted_e =>
            if its(cp_in_valid) then
              if its(sdp_in.sdp.ready) then
                r_state_r <= r_idle_e;
              else
                r_state_r <= r_last_valid_e;
              end if;
            end if;
          when r_last_valid_e =>
            if its(sdp_in.sdp.ready) then
              r_state_r <= r_idle_e;
            end if;
        end case;
      end if;
    end if;
  end process;
-- A pair of FIFOs on the CP side
  cdc_clock: block is
    constant width2cp_c      : positive :=
      cp_out.address'length + cp_out.data'length + cp_out.byte_en'length + 1; -- +1 for is_read
    constant width2sdp_c     : positive := cp_in.tag'length + cp_in.data'length;
    constant address_lsb_c   : positive := width2cp_c - cp_out.address'length;
    constant data_lsb_c      : positive := address_lsb_c - cp_out.data'length;
    constant byte_en_lsb_c   : positive := data_lsb_c - cp_out.byte_en'length;
    signal fifo2cp_in, fifo2cp_out : std_logic_vector(width2cp_c-1 downto 0);
    signal fifo2cp_enq, fifo2cp_deq, fifo2cp_not_full, fifo2cp_not_empty : std_logic;
    signal fifo2sdp_in, fifo2sdp_out : std_logic_vector(width2sdp_c-1 downto 0);
    signal fifo2sdp_enq, fifo2sdp_deq, fifo2sdp_not_full, fifo2sdp_not_empty : std_logic;
    alias ctl_clk is wci_clk;
    signal ctl_reset : std_logic;
  begin
    ctl_reset <= not wci_reset_n;
    ------ CDC SDP -> CP
    fifo2cp_in     <= addr_r & cp_out_data & be_r & in_read_r;
    fifo2cp_enq    <= fifo2cp_not_full and cp_out_valid;
    cp_in_take     <= fifo2cp_enq; -- we are taking from cp_out
    cp_out.address <= fifo2cp_out(address_lsb_c + cp_out.address'length-1 downto address_lsb_c);
    cp_out.data    <= fifo2cp_out(data_lsb_c + cp_out.data'length-1 downto data_lsb_c);
    cp_out.byte_en <= fifo2cp_out(byte_en_lsb_c + cp_out.byte_en'length-1 downto byte_en_lsb_c);
    cp_out.is_read <= fifo2cp_out(0);
    cp_out.valid   <= fifo2cp_not_empty;
    cp_out.clk     <= ctl_clk; -- delta
    cp_out.reset   <= ctl_reset;
    fifo2cp_deq    <= cp_in.take and fifo2cp_not_empty;
    fifo2cp: cdc.cdc.fifo
      generic map (width       => width2cp_c,
                   depth       => 2)
      port map (   src_CLK     => sdp_in.clk,
                   src_RST     => sdp_in.reset,
                   src_ENQ     => fifo2cp_enq,
                   src_in      => fifo2cp_in,
                   src_FULL_N  => fifo2cp_not_full,
                   dst_CLK     => ctl_clk,
                   dst_DEQ     => fifo2cp_deq,
                   dst_EMPTY_N => fifo2cp_not_empty,
                   dst_out     => fifo2cp_out);
    ------ CDC CP -> SDP
    fifo2sdp_in    <= cp_in.tag & cp_in.data;
    fifo2sdp_enq   <= fifo2sdp_not_full and cp_in.valid;
    cp_out.take    <= fifo2sdp_enq; -- we are taking from cp_out
    cp_in_tag      <= fifo2sdp_out(width2sdp_c-1 downto cp_in.data'length);
    cp_in_data     <= fifo2sdp_out(cp_in.data'range);
    cp_in_valid    <= fifo2sdp_not_empty;
    fifo2sdp_deq   <= cp_out_take and fifo2sdp_not_empty;
    fifo2sdp: cdc.cdc.fifo
      generic map (width       => width2sdp_c,
                   depth       => 2)
      port map (   src_CLK     => ctl_clk,
                   src_RST     => ctl_reset,
                   src_ENQ     => fifo2sdp_enq,
                   src_in      => fifo2sdp_in,
                   src_FULL_N  => fifo2sdp_not_full,
                   dst_CLK     => sdp_in.clk,
                   dst_DEQ     => fifo2sdp_deq,
                   dst_EMPTY_N => fifo2sdp_not_empty,
                   dst_out     => fifo2sdp_out);
end block;
end rtl;
