-------------------------------------------------------------------------------
--  This file is part of OpenCPI (www.opencpi.org).
--     ____                   __________   ____
--    / __ \____  ___  ____  / ____/ __ \ /  _/ ____  _________ _
--   / / / / __ \/ _ \/ __ \/ /   / /_/ / / /  / __ \/ ___/ __ `/
--  / /_/ / /_/ /  __/ / / / /___/ ____/_/ / _/ /_/ / /  / /_/ /
--  \____/ .___/\___/_/ /_/\____/_/    /___/(_)____/_/   \__, /
--      /_/                                             /____/
--
--  OpenCPI is free software: you can redistribute it and/or modify
--  it under the terms of the GNU Lesser General Public License as published
--  by the Free Software Foundation, either version 3 of the License, or
--  (at your option) any later version.
--
--  OpenCPI is distributed in the hope that it will be useful,
--  but WITHOUT ANY WARRANTY; without even the implied warranty of
--  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
--  GNU Lesser General Public License for more details.
--
--  You should have received a copy of the GNU Lesser General Public License
--  along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.
--
--------------------------------------------------------------------------------
-- This module adapts between the SDP and an AXI4 slave interface
-- The initial target AXI configuration is a 64 bit version of the AXI_HP ports on zynq,
---- This is a hardware interface on Zynq
-- Another target is the AXI PCIE bridge, 
---- This is a VHDL bridge to the underlying PCIE AXI4-lite TLP interface (mostly HW)
---- The tax (performace and gates) for using this IP core is unknown
---- Can be wide and can have long bursts
---- But this "AXU-master-only" mode does not work well for peer-to-peer,
---- So a different module that is both master and slace is necessary for good peer to peer.
-- For now we do not parameterize the AXI interface - it is what the zynq 64 hardware is.
-- This adapter is between the connected SDP acting as sdp "slave" and this adapter
-- We are acting as SDP master and an axi master.  (An SDP slave can stll issue requests).
-- For the purposes of the OpenCPI data plane, this adapter will only support the
-- "active-message" mode since it does not have a slave to receive data written
-- by the "other side" (yet, if ever).  This keeps this adapter smaller.
-- In order to fully implement the passive mode on zynq, or the active flow control mode,
-- we would have to also enable the M_AXI_GP1 interface and use it to allow
-- software to write into the BRAMs.  There would be a small latency benefit
-- for PS->PL data transfers, but everything else would suffer (throughput, gates, etc.)

-- The clock and reset are injected to be supplied to both sides

library IEEE; use IEEE.std_logic_1164.all, ieee.numeric_std.all;
library platform; use platform.platform_pkg.all;
library ocpi; use ocpi.types.all, ocpi.util.all;
library axi; use axi.axi_pkg.all;
library sdp; use sdp.sdp.all;

entity sdp2axi is
  generic(
    ocpi_debug   : boolean;
    sdp_width    : natural;
    axi_width    : natural);
  port(
    clk          : in  std_logic;
    reset        : in  bool_t;
    sdp_in       : in  s2m_t;
    sdp_in_data  : in  dword_array_t(0 to sdp_width-1);
    sdp_out      : out m2s_t;
    sdp_out_data : out dword_array_t(0 to sdp_width-1);
    axi_in       : in  s_axi_hp_out_t;
    axi_out      : out s_axi_hp_in_t;
    axi_error    : out bool_t;
    dbg_state    : out ulonglong_t;
    dbg_state1   : out ulonglong_t;
    dbg_state2   : out ulonglong_t
    );
end entity sdp2axi;
architecture rtl of sdp2axi is
  --------------------------------------------------------------------------------
  -- Terminology:
  -- xf     the beat of a data path - a "transfer"
  -- sxf    the "beat" of the sdp data path when valid
  -- pkt    the 1-or-more sxf sdp packet w/ request/address and (if a write) write data
  -- axf    the "beat" of the axi data path (fixed at 64 bits wide here)
  -- burst  the burst (of axfs) of data over AXI to a single requested address
  -- dw:    32 bit word (Microsoft's and PCI's anachronistic DWORD)
  -- sw:    sdp word (payload of sdp transfer sdp_width*32)
  -- aw:    axi word (currently 64b)
  --------------------------------------------------------------------------------
  constant dw_bytes_c          : natural := 32/8;
  constant aw_bytes_c          : natural := axi_width * dw_bytes_c;
  constant axi_max_burst_c     : natural := 16;
  subtype pkt_naxf_t           is unsigned(width_for_max(max_pkt_dws/axi_width)-1 downto 0);
  -- SDP request decoding and internal outputs
  signal pkt_dw_addr           : whole_addr_t;
  signal pkt_ndws              : pkt_ndw_t; -- count of dws in an SDP pkt
  signal pkt_writing           : bool_t;
  signal sdp_take              : bool_t;    -- we are taking from SDP in this cycle

  -- States of the address FSM
  type address_state_t is (sop_next_e, -- next sdp_in.sdp.valid is start of packet
                           in_pkt_e,   -- generating burst requests for packet
                           waiting_e); -- waiting for eop from write channel
  -- The address of the axi transfer in units of the axi width
  subtype axi_xfr_addr_t is unsigned(axi_out.AW.ADDR'left - width_for_max(aw_bytes_c-1)
                                    downto 0);
  signal addr_state_r          : address_state_t;
  signal axf_initial_dw_offset : unsigned(width_for_max(axi_width-1)-1 downto 0);
  signal axi_addr              : axi_xfr_addr_t;
  signal axi_addr_r            : axi_xfr_addr_t;
  signal axi_error_r           : bool_t; -- sticky error for internal use
  signal axi_len               : unsigned(width_for_max(axi_max_burst_c-1)-1 downto 0);
  signal pkt_naxf              : pkt_naxf_t;
  signal pkt_naxf_left         : pkt_naxf_t;
  signal pkt_naxf_left_r       : pkt_naxf_t;
  signal axi_requesting_addr   : bool_t;
  signal axi_accepting_addr    : bool_t;
  -- Write data output indication
  signal taking_data           : bool_t; -- write channel is taking data
  signal addressing_done       : bool_t;
  signal writing_done          : bool_t;
  signal accepting_last        : bool_t;
begin
  pkt_dw_addr           <= sdp_in.sdp.header.extaddr & sdp_in.sdp.header.addr;
  pkt_ndws              <= count_in_dws(sdp_in.sdp.header);
  pkt_naxf              <= resize((pkt_ndws + axf_initial_dw_offset + axi_width - 1) / axi_width,
                                  pkt_naxf'length);
  pkt_naxf_left         <= pkt_naxf when addr_state_r = sop_next_e else pkt_naxf_left_r;
  pkt_writing           <= to_bool(sdp_in.sdp.header.op = write_e);
  axf_initial_dw_offset <= pkt_dw_addr(width_for_max(axi_width-1)-1 downto 0);
  axi_len               <= resize(ocpi.util.min(pkt_naxf_left, axi_max_burst_c) - 1, axi_len'length);
  axi_accepting_addr    <= to_bool(((its(pkt_writing) and axi_in.AW.READY = '1') or
                                 (not its(pkt_writing) and axi_in.AR.READY = '1')));
  axi_requesting_addr   <= to_bool(sdp_in.sdp.valid and addr_state_r /= waiting_e);
  axi_addr              <= resize(pkt_dw_addr(pkt_dw_addr'left downto
                                              width_for_max(axi_width-1)), axi_addr'length)
                           when addr_state_r = sop_next_e else
                           axi_addr_r;
  axi_error             <= axi_error_r;
  accepting_last        <= to_bool(axi_accepting_addr and
                                   (pkt_naxf <= axi_max_burst_c or
                                    pkt_naxf_left <= axi_max_burst_c));
  -- Tell other modules that the addressing is done previously or is done this cycle
  addressing_done       <= to_bool(accepting_last or addr_state_r = waiting_e);
  -- take from SDP when
  sdp_take <= to_bool(sdp_in.sdp.valid and
                      -- For reads, take when AXI accepts the addr for the last burst in pkt
                      ((not its(pkt_writing) and accepting_last) or
                       -- For writes, take when write data is taken and addr bursts are done
                       (pkt_writing and
                        ((taking_data and (not its(sdp_in.sdp.eop) or addressing_done)) or
                         (its(writing_done) and accepting_last)))));

  --------------------------------------------------------------------------------
  -- Clocked processing for addressing and breaking addressed packets into bursts
  -- Since the SDP header is stable for the life of the packet, we can issue all the
  -- address channel bursts based on that, independent of read and write data channels
  -- States for this FSM:
  --   sop_next_e : between packets.  Any sdp_in.valid starts packet
  --   in_pkt_e   : working on bursts for a pkt
  --   waiting_e  : waiting for EOP after all bursts done.
  --------------------------------------------------------------------------------
  doclk : process(clk)
  begin
    if rising_edge(clk) then
      if its(reset) then
        addr_state_r <= sop_next_e;  -- we are initially waiting for SOP
        axi_error_r  <= bfalse;
      else
        case addr_state_r is
          when sop_next_e =>
            if sdp_in.sdp.valid and not its(axi_error_r) then -- SDP is offering something.  capture address
              if its(axi_accepting_addr) then
                pkt_naxf_left_r    <= pkt_naxf - ocpi.util.min(pkt_naxf, axi_max_burst_c);
                axi_addr_r         <= axi_addr + axi_max_burst_c;
                -- first burst if being accepted.
                if pkt_naxf > axi_max_burst_c then -- more bursts after this
                  addr_state_r <= in_pkt_e;
                elsif not its(sdp_take and sdp_in.sdp.eop) then
                  addr_state_r <= waiting_e;
                end if;
              end if;
            end if;
          when in_pkt_e =>
            if its(axi_accepting_addr) then
              pkt_naxf_left_r    <= pkt_naxf_left_r -
                                    ocpi.util.min(pkt_naxf_left_r, axi_max_burst_c);
              axi_addr_r         <= axi_addr + axi_max_burst_c;
              if pkt_naxf_left_r <= axi_max_burst_c then
                if sdp_in.sdp.eop and sdp_take then
                  addr_state_r <= sop_next_e;
                else
                  addr_state_r <= waiting_e;
                end if;
              end if;
            end if;
          when waiting_e =>
            if sdp_in.sdp.eop and sdp_take then
              addr_state_r <= sop_next_e;
            end if;
        end case;
        -- Capture errors from write responses.  Write data submodule doesn't look
        if axi_in.B.VALID = '1' and axi_in.B.RESP /= Resp_OKAY then
          axi_error_r <= '1';
        end if;
      end if; -- not reset
    end if;     -- rising edge
  end process;

  ----------------------------------------------
  -- Interface outputs to the S_AXI_HP interface
  ----------------------------------------------
  -- axi_hp outputs
  axi_out.ACLK                <= clk;
  -- Write address channel
  axi_out.AW.ID                <= (others => '0');  -- spec says same id means in-order
  axi_out.AW.ADDR              <= std_logic_vector(axi_addr) & "000";
  axi_out.AW.LEN               <= std_logic_vector(axi_len);
  axi_out.AW.SIZE              <= "011";        -- we are always 64 bits wide
  axi_out.AW.BURST             <= "01";         -- we are always doing incrementing bursts
  axi_out.AW.LOCK              <= "00";         -- normal access, no locking or exclusion
  axi_out.AW.CACHE             <= (others => '0');
  axi_out.AW.PROT              <= (others => '0');
  axi_out.AW.VALID             <= axi_requesting_addr and pkt_writing;

  -- Write data channel
  -- wired directly to sdp2axi_wd

  -- Write response channel
  axi_out.B.READY              <= '1';              -- we are always ready for responses

  -- Read address channel
  axi_out.AR.ID                <= std_logic_vector(sdp_in.sdp.header.node(2 downto 0)) &
                                  std_logic_vector(sdp_in.sdp.header.xid);
  axi_out.AR.ADDR              <= std_logic_vector(axi_addr) & "000";
  axi_out.AR.LEN               <= std_logic_vector(axi_len);
  axi_out.AR.SIZE              <= "011"; -- we are always 64 bits wide
  axi_out.AR.BURST             <= "01";  -- we are always doing incrementing bursts
  axi_out.AR.LOCK              <= "00";  -- normal access, no locking or exclusion
  axi_out.AR.CACHE             <= (others => '0');
  axi_out.AR.PROT              <= (others => '0');
  axi_out.AR.VALID             <= axi_requesting_addr and not pkt_writing;

  -- These are not AMBA/AXI
  axi_out.AR.QOS               <= (others => '0');
  axi_out.AW.QOS               <= (others => '0');
  axi_out.AR.ISSUECAP1_EN      <= '0';
  axi_out.AW.ISSUECAP1_EN      <= '0';

  ----------------------------------------------
  -- debug output - very poor man's ILA
  ----------------------------------------------
  dbg_state <= to_ulonglong(
    std_logic_vector(axi_addr_r) & "000" &
    slv(pkt_writing) & --31
    slv(taking_data) & --30
    slv(sdp_in.sdp.eop) & --29
    slv(addressing_done) & --28
    slv(axi_accepting_addr) & --27
    slv(accepting_last) & --26
    slv(sdp_in.sdp.valid) & --25
    "00000000000" & -- 24 - 14
    slv(to_unsigned(address_state_t'pos(addr_state_r),2)) & -- 13-12
    std_logic_vector(pkt_naxf_left) -- 11 to 0
    );
    
  ----------------------------------------------
  -- Instantiate the write data channel module
  wd : entity work.sdp2axi_wd
    generic map (ocpi_debug      => ocpi_debug,
                 axi_width       => axi_width,
                 sdp_width       => sdp_width)
    port map (   clk             => clk,
                 reset           => reset,
                 addressing_done => addressing_done,
                 sdp             => sdp_in.sdp,
                 sdp_in_data     => sdp_in_data,
                 axi_in          => axi_in.w,
                 axi_out         => axi_out.w,
                 taking_data     => taking_data,
                 writing_done    => writing_done,
                 debug           => dbg_state1);

  ----------------------------------------------
  -- Instantiate the read data channel module
  rd : entity work.sdp2axi_rd
    generic map (ocpi_debug   => ocpi_debug,
                 axi_width    => axi_width,
                 sdp_width    => sdp_width)
    port map (   clk          => clk,
                 reset        => reset,
                 sdp_take     => sdp_take,
                 sdp_in       => sdp_in,
                 sdp_out      => sdp_out,
                 sdp_out_data => sdp_out_data,
                 axi_in       => axi_in.r,
                 axi_out      => axi_out.r,
                 debug        => dbg_state2);

end rtl;
