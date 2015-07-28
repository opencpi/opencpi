-- An SPD client that talks to the control plane
-- The biggest issue is to map 64 bit transfers into 2 32 bit transactions
-- The only accesses expected are single word 32 bit accesses and aligned 64 bit accesses.
-- We take our clock from the SDP.

-- This module will pipeline request and write data into the CP without regard to
-- serialization, but, for 64 bit reads, it must remember the response information to attach to
-- read data coming out of the CP, and it must coalesce into 64 bit responses

-- Another important issue is to capture requests (and register them here), in order to
-- keep the SDP uncongested.  So requests are "consumed" from the SDP while they
-- are taking place on the control plane.  Write requests are also posted here.
-- So we take the resource hit here to make the SDP work better.
-- Conceivably we could make this behavior configurable on a generic (i.e. enable a more
-- synchronous mode) to save 100 FFs.  For now we tilt toward performance.
library IEEE, ocpi, sdp, platform;
use IEEE.std_logic_1164.all, ieee.numeric_std.all, ieee.math_real.all, ocpi.types.all,
  sdp.sdp.all, ocpi.util.all, platform.platform_pkg.all;
architecture rtl of sdp2cp_rv is
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
  cp_out.clk     <= sdp_in.clk;
  cp_out.reset   <= sdp_in.reset;
  cp_out.valid   <= to_bool(a_state_r /= a_idle_e and a_state_r /= a_last_wanted_e);
  cp_out.is_read <= in_read_r;
  cp_out.address <= addr_r;
  cp_out.byte_en <= be_r;
  cp_out.data    <= slv(tag_r, dword_size) when its(in_read_r) else
                    s2c_dwords_r(hi_dw) when a_state_r = a_last_e and in_64_r else
                    s2c_dwords_r(0);
  -- take read data from the CP if it will be accepted by the SDP. We don't pipeline
  cp_out.take    <= to_bool(cp_in.valid and
                            ((r_state_r = r_idle_e and
                              tag_first_of_2(cp_in.tag) and sdp_width /= 1) or
                             sdp_in.sdp.ready));
  sdp_out.sdp.header.op    <= response_e;
  sdp_out.sdp.header.count(count_width-1 downto 1) <= (others => '0');
  sdp_out.sdp.header.count(0) <= to_bool(its(tag_first_of_2(cp_in.tag)) or
                                         r_state_r = r_last_wanted_e or
                                         r_state_r = r_last_valid_e);
  sdp_out.sdp.header.xid   <= tag_xid(cp_in.tag);
  sdp_out.sdp.header.lead  <= (others => '0');
  sdp_out.sdp.header.trail <= (others => '0');
  sdp_out.sdp.header.node  <= (others => '0'); -- CP is always node zero
  sdp_out.sdp.header.addr  <= (others => '0');
  sdp_out.sdp.valid        <= to_bool((r_state_r = r_idle_e and its(cp_in.valid) and
                                       not (tag_first_of_2(cp_in.tag) and sdp_width /= 1)) or
                                      r_state_r = r_first_valid_e or
                                      (r_state_r = r_last_wanted_e and its(cp_in.valid)) or
                                      r_state_r = r_last_valid_e);
  sdp_out.sdp.eom    <= to_bool(r_state_r = r_last_wanted_e or r_state_r = r_last_valid_e);
  sdp_out.sdp.ready  <= to_bool(sdp_in.sdp.valid and
                                (a_state_r = a_idle_e or
                                 (a_state_r = a_first_e and cp_in.take) or
                                 a_state_r = a_last_wanted_e));
g0: for i in 0 to to_integer(sdp_width)-1 generate
  sdp_out_data(i) <= cp_in.data when tag_startdw(cp_in.tag) = i else (others => '0');
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
            if its(cp_in.take) then
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
            if its(cp_in.take) then
              a_state_r <= a_idle_e;
            end if;
        end case;
        -- The read side state machine.
        -- We put the burden on the CP to feed us back a tag so we don't need
        -- to remember anything about the transaction.
        case r_state_r is
          when r_idle_e =>
            if its(cp_in.valid) then
              if its(tag_first_of_2(cp_in.tag)) then
                if sdp_width /= 1 then
                  -- just buffering it
                  r_state_r   <= r_last_wanted_e;
                  c2s_dword_r <= cp_in.data;
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
              if its(tag_first_of_2(cp_in.tag)) then
                r_state_r <= r_last_wanted_e;
              else
                r_state_r <= r_idle_e;
              end if;
            end if;
          when r_last_wanted_e =>
            if its(cp_in.valid) then
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
end rtl;
