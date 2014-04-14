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
-- This module adapts between the uNOC and the AXI interface
-- The initial target AXI configuration is a 64 bit version of the AXI_HP ports,
-- although the AXI aspect should be fairly generic.
-- This adapter is between the connected uNOC acting as unoc "master" and this adapter
-- acting as unoc slave and an axi master.
-- For the purposes of the OpenCPI data plane, this adapter will only support the
-- "active-message" mode since it does not have a slave to receive data written
-- by the "other side" (yet, if ever).  This keeps this adapter smaller.
--
-- In order to fully implement the passive mode, or the active flow control mode,
-- we would have to also enable the M_AXI_GP1 interface and use it to allow
-- software to write into the BRAMs.  There would be a small latency benefit
-- for PS->PL data transfers, but everything else would suffer (throughput, gates, etc.)
--
-- Note we assume 32 bit addressing and do not support the 64 bit addressing unoc mode.
--
-- The clock and reset are injected to be supplied to both sides
library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library platform; use platform.platform_pkg.all;
library ocpi; use ocpi.types.all, ocpi.util.all;
library zynq; use zynq.zynq_pkg.all;

entity unoc2axi is
  port(
    clk       : in std_logic;
    reset     : in bool_t;
    unoc_in   : in  unoc_master_in_t;
    unoc_out  : out unoc_master_out_t;
    axi_in    : in  s_axi_hp_out_t;
    axi_out   : out s_axi_hp_in_t;
    axi_error : out bool_t;
    dbg_state : out ulonglong_t;
    dbg_state1: out ulonglong_t
    );
end entity unoc2axi;
architecture rtl of unoc2axi is
  --------------------------------------------------------------------------------
  -- Terminology:
  -- xf            the beat of a data path - a "transfer"
  -- uxf           the "beat" of the unoc data path when valid (128 bits wide here)
  -- pkt           the 1 or more uxf unoc message that has request/address and (if a write) write data
  -- axf           the "beat" of the axi data path (64 bits wide here)
  -- burst         the (up to 16 axf) burst of data over AXI at a single requested address
  -- dw:           32 bit word (Microsoft's and PCI's anachronistic DWORD)
  -- qw:           64 bit word
  -- uw:           unoc word (payload of unoc transfer)
  -- aw:           axi word (currently 64b)
  --------------------------------------------------------------------------------
  constant dw_bits_c       : natural := 32;
  -- constant dw_bytes      : natural := dw_bits/8;
  constant qw_bits_c       : natural := 64;
  -- constant qw_bytes      : natural := qw_bits/8;
  constant aw_bits_c       : natural := 64;
  constant uw_bits_c       : natural := dw_bits_c * unoc_ndw_c;
  constant uw_bytes_c      : natural := uw_bits_c/8;
  constant aw_bytes_c      : natural := aw_bits_c/8;
  constant axi_max_burst_c : natural := 16;
  constant axf_per_uxf_c   : natural := uw_bytes_c/aw_bytes_c;
  constant axf_per_uxf_width_c   : natural := width_for_max(axf_per_uxf_c)-1;
  -- Size of counters of axfs within a uxf.
  subtype naxf_in_uxf_t is unsigned(1 downto 0); -- width_for_max(axf_per_uxf_c+1)-1 downto 0;
  subtype ndw_in_uxf_t is unsigned(width_for_max(unoc_ndw_c)-1 downto 0);
  -- UNOC request decoding and internal outputs
  signal unoc_hdr               : unoc_header_t;
  signal unoc_req               : unoc_request_t;
  signal unoc_addr              : dword_t;
  signal unoc_ndw               : unsigned(unoc_hdr.dw_length'left + 1 downto 0);
  signal unoc_take_req          : bool_t;
  signal unoc_aligned           : bool_t; -- unoc address is aligned to 64bits
  -- Packet calculations during startup
  signal pkt_can_start          : bool_t;
  signal pkt_starting           : bool_t;
  signal pkt_naxf               : unsigned(unoc_ndw'range);
  -- State of the unoc pkt currently being processed
  signal pkt_active_r           : bool_t;
  signal pkt_aligned_r          : bool_t;
  signal pkt_writing_r          : bool_t;
  signal pkt_odd_ndw_r          : bool_t;
  signal pkt_naxf_left          : unsigned(unoc_hdr.dw_length'left + 1 downto 0);
  signal pkt_naxf_left_r        : unsigned(unoc_hdr.dw_length'left + 1 downto 0);
  -- AXI addressing calculations and state (read or write)
  signal axi_addr64_r           : unsigned(axi_out.AWADDR'left - 3 downto 0);
  signal axi_accepting_addr     : bool_t;
  signal axi_burst_len          : unsigned(width_for_max(axi_max_burst_c)-1 downto 0);
  signal axi_burst_len_r        : unsigned(width_for_max(axi_max_burst_c)-1 downto 0);
  signal axi_error_r            : bool_t; -- stick error for internal use
  signal axi_len                : std_logic_vector(axi_out.AWLEN'range);
  -- Read data processing state and internal versions of output signals
  signal rd_ndw_left_r          : unsigned(unoc_hdr.dw_length'left + 1 downto 0);
  signal rd_ndw_next_uxf        : ndw_in_uxf_t;
  signal rd_ndw_for_uxf_r       : ndw_in_uxf_t;
  signal rd_naxf_next_uxf       : naxf_in_uxf_t;
  signal rd_naxf_left_uxf_r     : naxf_in_uxf_t;
  signal rd_sof_r               : bool_t; -- current uxf is sof
  signal rd_eof                 : bool_t;
  signal rd_unoc_valid          : bool_t; -- internal output
  signal rd_axi_ready           : bool_t; -- internal output
  signal rd_axi_lo              : dword_t; -- low DW from AXI read data
  signal rd_axi_hi              : dword_t; -- hi DW from AXI read data
  signal rd_data_r              : dword_array_t(0 to unoc_ndw_c-2); -- need to buffer these
  signal rd_mask                : std_logic_vector(0 to width_for_max(unoc_ndw_c)-1 + 2);
  -- Write data processing
  signal wr_naxf_in_first_uxf   : naxf_in_uxf_t;
  signal wr_naxf_in_next_uxf    : naxf_in_uxf_t;
  signal wr_naxf_left_for_uxf_r : naxf_in_uxf_t;
  signal wr_naxf_left_for_pkt_r : unsigned(unoc_ndw'range);
  signal wr_leftover_dw_r       : dword_t;
  signal wr_dwords              : dword_array_t(0 to unoc_ndw_c); -- note extra one for leftover
  signal wr_accepting_data      : bool_t;
  signal wr_axi_lo_idx          : ndw_in_uxf_t;
  signal wr_axi_hi_idx          : ndw_in_uxf_t;
  signal wr_axi_lo_idx_r        : ndw_in_uxf_t;
  signal wr_axi_hi_idx_r        : ndw_in_uxf_t;
  signal wr_axi_last            : bool_t;
  signal wr_axi_valid           : bool_t;
  signal wr_axi_strobe          : std_logic_vector(axi_out.WSTRB'range);
  signal wr_first_axf_r         : bool_t;
  function swap(d : dword_t) return dword_t is
  begin
    return dword_t'(d(7 downto 0) & d(15 downto 8) & d(23 downto 16) & d(31 downto 24));
  end swap;
begin
  --------------------------------------------------------------------------------
  -- UNOC computations *FROM* unoc signals
  --------------------------------------------------------------------------------
  unoc_hdr     <= to_unoc_header(unoc_in.data.payload(0));
  unoc_req     <= to_unoc_request(unoc_in.data.payload(1));
  unoc_ndw     <= to_unsigned(1024, unoc_ndw'length)
                  when unoc_hdr.dw_length = slv0(unoc_hdr.dw_length'length) else
                  resize(unsigned(unoc_hdr.dw_length), unoc_ndw'length);
  unoc_addr    <= unoc_in.data.payload(2);
  unoc_aligned <= to_bool(unoc_addr(2) = '0');
  --------------------------------------------------------------------------------
  -- UNOC computations *TO* unoc signals
  --------------------------------------------------------------------------------
  -- dequeue unoc xfer when (read or write) request aspects are done and write data
  -- aspects are done.
  unoc_take_req <=
    to_bool(
      -- 1. The unoc xfer is valid - there is an xfer to be taken
      its(unoc_in.valid) and
      -- 2. If a sof, then the request must be capturable, and its a read request,
      --    or the write data in the SOF is either captured too (as a leftover),
      --    or AXI write channel can take it
      ((its(unoc_in.data.sof) and pkt_can_start and
        (not its(unoc_hdr.has_data) or wr_naxf_in_first_uxf = 0 or
         (wr_naxf_in_first_uxf /= 0 and wr_accepting_data))) or
      -- 3. If not a sof, then its a write, and we just want to know that 
      --    the last axf is happening now.
       (not its(unoc_in.data.sof) and wr_naxf_left_for_uxf_r = 1 and wr_accepting_data)));

  unoc_out.take <= unoc_take_req;
  --------------------------------------------------------------------------------
  -- PKT processing
  -- There is one cycle of latency from when the unoc request packet is available
  -- and the first axi transaction/burst for that packet can start
  --------------------------------------------------------------------------------
  -- We can start processing a packet if we're not active or if we are are
  -- in the cycle where the last burst request will be accepted
  pkt_can_start <= to_bool(not its(pkt_active_r) or
                           (axi_accepting_addr and pkt_naxf_left_r = 0));
  -- We will start on a valid unoc SOF, except for a sticky AXI error
  pkt_starting  <= to_bool(unoc_in.valid and its(unoc_in.data.sof) and
                           its(pkt_can_start) and not its(axi_error_r));
  -- How many axi xfers will be in the currently starting unoc packet?
  pkt_naxf <= (unoc_ndw + 1 + unsigned(slv(unoc_addr(2)))) / 2;
  --------------------------------------------------------------------------------
  -- AXI address channel: deal w/axi 64b alignment and axi burst limits
  --    The addressing/bursting done for reads or writes, the only difference being
  --    which address channel to use.
  --------------------------------------------------------------------------------
  -- How many axi xfers are left to do in this packet (for request/burst accounting)
  pkt_naxf_left      <= pkt_naxf when its(pkt_starting) else pkt_naxf_left_r;
  -- What is the length (number of xfers) of the next AXI burst
  axi_burst_len      <=
    resize(pkt_naxf_left, axi_burst_len'length)
    when pkt_naxf_left < axi_max_burst_c
    else to_unsigned(axi_max_burst_c, axi_burst_len'length);
  -- Is AXI taking our (burst) address?
  axi_accepting_addr <= to_bool(pkt_active_r and
                                ((its(pkt_writing_r) and axi_in.AWREADY = '1') or
                                 (not its(pkt_writing_r) and axi_in.ARREADY = '1')));
  -- Encode the burst length for AXI
  axi_len            <= std_logic_vector(resize(axi_burst_len_r - 1, axi_len'length));
  axi_error          <= axi_error_r;
  --------------------------------------------------------------------------------
  -- AXI read data channel: connect it to the unoc read data path
  --------------------------------------------------------------------------------
  -- Offer a uxf to the unoc when there are enough dw available for the uxf
  -- Note the case when this condition is satified by a leftover (hi) DW
  -- and no current AXI read data
  rd_unoc_valid    <= to_bool(rd_naxf_left_uxf_r = 1 and axi_in.RVALID);
  rd_eof           <= to_bool(rd_ndw_next_uxf = 0);
  rd_axi_ready     <= to_bool(rd_naxf_left_uxf_r > 1 or
                              (rd_naxf_left_uxf_r = 1 and unoc_in.take));
  -- How many DWs for the NEXT uxf after the current one
  -- And the NEXT uxf can't be the SOF with the header, so its simpler
  rd_ndw_next_uxf  <= resize(rd_ndw_left_r,  rd_ndw_next_uxf'length)
                      when rd_ndw_left_r < unoc_ndw_c else
                      to_unsigned(unoc_ndw_c, rd_ndw_next_uxf'length);
  rd_naxf_next_uxf <= "10"
                      when rd_ndw_next_uxf = 4 or (rd_ndw_next_uxf = 3 and not its(pkt_aligned_r)) else
                      "00"
                      when rd_ndw_next_uxf = 0 or (rd_ndw_next_uxf = 1 and pkt_aligned_r) else
                      "01";
  rd_axi_lo        <= swap(axi_in.RDATA(31 downto 0));
  rd_axi_hi        <= swap(axi_in.RDATA(63 downto 32));
  -- Read data routing to uxf DWs - pretty ugly..
--  rd_max_uxf       <= to_unsigned(4, rd_max_uxf'length)
--                      when rd_ndw_for_uxf_r >= 4 else
--                      rd_ndw_for_uxf_r;
  rd_mask          <= slv(rd_sof_r) & slv(pkt_aligned_r) & std_logic_vector(rd_ndw_for_uxf_r);
  unoc_out.data.payload(0) <=
    rd_axi_lo    when rd_mask = "00001" or rd_mask = "00010" else
    rd_data_r(0);
  unoc_out.data.payload(1) <=
    rd_axi_hi    when rd_mask = "00010" else
    rd_axi_lo    when rd_mask = "01010" or rd_mask = "01011" else
    rd_data_r(1);
  unoc_out.data.payload(2) <=
    rd_axi_hi    when rd_mask = "01011" else
    rd_axi_lo    when rd_mask = "00011" or rd_mask = "00100" else
    rd_data_r(2);
  unoc_out.data.payload(3) <= rd_axi_lo when its(pkt_aligned_r) else rd_axi_hi;

  --------------------------------------------------------------------------------
  -- AXI write data channel computations
  --------------------------------------------------------------------------------
  -- How many axfs for the *FIRST* uxf
  wr_naxf_in_first_uxf <= "01"
                          when unoc_ndw = 1 or not its(unoc_aligned) else
                          "00";
  -- How many axfs for the *NEXT* uxf, knowing there is one.  1, 2 or 3
  -- Not used for the first uxf
  wr_naxf_in_next_uxf <= 
    resize(wr_naxf_left_for_pkt_r, wr_naxf_in_next_uxf'length)
    when wr_naxf_left_for_pkt_r < 3 else
    "11" when wr_naxf_left_for_pkt_r = 3 and pkt_aligned_r and pkt_odd_ndw_r else
    "10";
  -- Write data sources to axi - either the unoc itself or a "leftover" register
  wr_dwords(0 to unoc_ndw_c-1) <= unoc_in.data.payload;
  wr_dwords(unoc_ndw_c)        <= wr_leftover_dw_r;
  wr_accepting_data            <= to_bool(axi_in.WREADY = '1');
  wr_axi_lo_idx                <= to_unsigned(3, wr_axi_lo_idx'length)
                                  when its(pkt_starting) else
                                  wr_axi_lo_idx_r;
  wr_axi_hi_idx                <= to_unsigned(3, wr_axi_hi_idx'length)
                                  when its(pkt_starting) else
                                  wr_axi_hi_idx_r;
  -- AXI write data is valid for axi
  wr_axi_valid    <= to_bool(unoc_in.valid and
                                ((its(unoc_in.data.sof) and wr_naxf_in_first_uxf = 1) or
                                 (not its(unoc_in.data.sof) and wr_naxf_left_for_uxf_r /= 0)));
  wr_axi_last     <= to_bool(((its(unoc_in.data.sof) and wr_naxf_in_first_uxf = pkt_naxf) or
                                 (not its(unoc_in.data.sof) and wr_naxf_left_for_uxf_r /= 0)));
  wr_axi_strobe   <= "00001111"
                     when (its(unoc_in.data.sof) and its(unoc_aligned) and wr_naxf_in_first_uxf = 1) or
                          (not its(unoc_in.data.sof) and its(wr_axi_last) and
                           ((its(pkt_aligned_r) and pkt_odd_ndw_r) or
                            (not its(pkt_aligned_r) and not its(pkt_odd_ndw_r)))) else
                     "11110000"
                     when (its(unoc_in.data.sof) and not its(unoc_aligned)) or
                          (not its(unoc_in.data.sof) and wr_axi_last and
                           ((pkt_aligned_r and not its(pkt_odd_ndw_r)) or
                            (not its(pkt_aligned_r) and pkt_odd_ndw_r))) else
                     "11111111";
  --------------------------------------------------------------------------------
  -- Clocked State processing 
  --------------------------------------------------------------------------------
  doclk : process(clk)
  begin
    if rising_edge(clk) then
      if reset = '1' then
        -- We'll list all registers, but comment out those that don't really need init

        pkt_active_r           <= '0';
        pkt_aligned_r          <= '0';
        pkt_writing_r          <= '0'; -- implies active
        pkt_odd_ndw_r          <= '0';
        pkt_naxf_left_r        <= (others => '0');

        axi_addr64_r           <= (others => '0');
        axi_burst_len_r        <= (others => '0');
        axi_error_r            <= '0';

        rd_ndw_left_r          <= (others => '0');
        rd_ndw_for_uxf_r       <= (others => '0');
        rd_naxf_left_uxf_r     <= (others => '0');
        rd_sof_r               <= '0';
        rd_data_r              <= (others => (others => '0'));

        wr_naxf_left_for_uxf_r <= (others => '0');
        wr_naxf_left_for_pkt_r <= (others => '0');
        wr_leftover_dw_r       <= (others => '0');
        wr_axi_lo_idx_r        <= (others => '0');
        wr_axi_hi_idx_r        <= (others => '0');
        wr_first_axf_r         <= '0';

      else
        --------------------------------------------------------------------------------
        -- Start processing a unoc packet
        --------------------------------------------------------------------------------
        if its(pkt_starting) then
          --------------------------------------------------------------------------------
          -- Initializations for this unoc packet for addressing (read or write)
          -- we are starting a unoc packet and capturing info from first unoc xfer.
          -- it will be presented to axi in the next cycle.  This is setting up both
          -- the addressing machinery (bursts)
          pkt_active_r    <= btrue;
          pkt_aligned_r   <= unoc_aligned;
          pkt_odd_ndw_r   <= unoc_ndw(0);
          pkt_writing_r   <= unoc_hdr.has_data;
          -- Amoung left *AFTER* the current burst
          pkt_naxf_left_r <= pkt_naxf - axi_burst_len;
          axi_addr64_r    <= resize(unsigned(unoc_addr(unoc_addr'left downto 3)),
                                    axi_addr64_r'length);
          axi_burst_len_r <= axi_burst_len;
          --------------------------------------------------------------------------------
          -- Initializations for reading data
          -- How many DW left after the first uxf
          rd_ndw_left_r      <= unoc_ndw - 1;
          -- The first uxf has a 3 word response header and 1 dw of data
          rd_ndw_for_uxf_r   <= to_unsigned(1, rd_ndw_for_uxf_r'length);
          rd_naxf_left_uxf_r <= to_unsigned(1, rd_naxf_left_uxf_r'length);
          rd_sof_r           <= btrue;
          rd_data_r(0)       <= from_unoc_header(unoc_make_response(unoc_ndw));
          rd_data_r(1)       <= from_unoc_completion1(unoc_make_completion1(unoc_ndw));
          rd_data_r(2)       <= from_unoc_completion2(unoc_make_completion2(unoc_req.sender_id,
                                                                            unoc_req.tag,
                                                                            unoc_addr));
          --------------------------------------------------------------------------------
          -- Initializations for writing data
          wr_first_axf_r <= btrue;
          -- This is naxf left AFTER the current UXF
          wr_naxf_left_for_pkt_r <= pkt_naxf - wr_naxf_in_first_uxf;
          -- This naxf is for the current uxf and is counted down
          wr_naxf_left_for_uxf_r <= wr_naxf_in_first_uxf;
          -- Where should the first low and high AXI write dwords for first axf come from?
          if unoc_ndw = 1 or not its(unoc_aligned) then
            -- single word or unaligned means any data comes from unoc dw 3
            wr_axi_lo_idx_r <= to_unsigned(3, wr_axi_lo_idx_r'length);
            wr_axi_hi_idx_r <= to_unsigned(3, wr_axi_hi_idx_r'length);
          else
            -- aligned, multiword means use leftover for low, dw 0 for high
            wr_axi_lo_idx_r <= to_unsigned(4, wr_axi_lo_idx_r'length);
            wr_axi_hi_idx_r <= to_unsigned(0, wr_axi_lo_idx_r'length);
          end if;
        elsif its(pkt_active_r) then
          ------------------------------------------------------------------------------
          -- Address/burst processing when not starting
          -- The current burst request is being enqueued/accepted, we can advance it.
          ------------------------------------------------------------------------------
          if its(axi_accepting_addr) then
            if pkt_naxf_left_r = 0 then
              pkt_active_r    <= bfalse;
            else
              axi_addr64_r    <= axi_addr64_r + axi_max_burst_c;
              axi_burst_len_r <= axi_burst_len;
              pkt_naxf_left_r <= pkt_naxf_left_r - axi_burst_len;
            end if;
          end if;
          --------------------------------------------------------------------------------
          -- Write data flow processing (not starting)
          --------------------------------------------------------------------------------
          -- Always grab the last word in case we need it for writing later
          if its(unoc_take_req) then
            wr_leftover_dw_r <= unoc_in.data.payload(unoc_ndw_c-1);
          end if;
          if wr_naxf_left_for_uxf_r /= 0 and unoc_in.valid and wr_accepting_data then
            -- We are moving some data NOW
            wr_first_axf_r <= bfalse;
            if wr_naxf_left_for_uxf_r = 1 then
              -- We are done with the current valid UXF - doing the last AXF for it
              if wr_naxf_left_for_pkt_r = 0 then
                -- We're done with the whole packet.  Stop the writing
                wr_naxf_left_for_uxf_r <= (others => '0');
              else
                -- We're not done with the pkt, so prepare for next uxf
                wr_naxf_left_for_uxf_r <= wr_naxf_in_next_uxf;
                wr_naxf_left_for_pkt_r <= wr_naxf_left_for_pkt_r - wr_naxf_in_next_uxf;
              end if;
            else
              -- We're just making progress on the current uxf, not done with it.
              wr_naxf_left_for_uxf_r <= wr_naxf_left_for_uxf_r - 1;
            end if;
            -- We need to advance the data routing to the axi low and high dwords
            case to_integer(wr_axi_lo_idx_r) is
              when 0      => wr_axi_lo_idx_r <= O"2";
              when 1      => wr_axi_lo_idx_r <= O"4";
              when 2      => wr_axi_lo_idx_r <= O"0";
              when others => wr_axi_lo_idx_r <= O"1";
            end case;
            case to_integer(wr_axi_hi_idx_r) is
              when 0      => wr_axi_hi_idx_r <= O"2";
              when 1      => wr_axi_hi_idx_r <= O"3";
              when 2      => wr_axi_hi_idx_r <= O"0";
              when others => wr_axi_hi_idx_r <= O"1";
            end case;
          end if;
          -- Advance the write response channel by just checking for errors
          if axi_in.BVALID = '1' and axi_in.BRESP /= Resp_OKAY then
            axi_error_r <= '1';
          end if;
        end if; -- pkt_active_r
        --------------------------------------------------------------------------------
        -- Read data flow processing.  Driven by incoming read channel data.
        --------------------------------------------------------------------------------
        -- The read data state machine.  Note that read data can be flowing into the
        -- unoc while new/later unoc requests are being conveyed to other AXI channels
        if rd_unoc_valid and unoc_in.take then
          -- unoc is taking a uxf from us right now.
          -- compute new NAXF in NEXT uxf which might be zero
          rd_naxf_left_uxf_r <= rd_naxf_next_uxf;
          -- compute new NDWs in NEXT uxf which might be zero
          rd_ndw_for_uxf_r <= rd_ndw_next_uxf;
          -- subtract (possibly to zero) the DWs left after the next uxf
          rd_ndw_left_r   <= rd_ndw_left_r - rd_ndw_next_uxf;
          rd_sof_r        <= bfalse;
          rd_data_r(0)    <= rd_axi_hi; -- left over high dw when aligned.
        elsif rd_axi_ready and axi_in.RVALID then
          -- We're taking an axi xfr from the read channel for parts of a uxf
          -- when its not the last axi xfer for this uxf
          rd_naxf_left_uxf_r <= rd_naxf_left_uxf_r - 1;
          if its(pkt_aligned_r) then
            rd_data_r(0)    <= rd_axi_hi; -- leftover
            rd_data_r(1)    <= rd_axi_lo;
          else
            rd_data_r(0)    <= rd_axi_lo;
            rd_data_r(1)    <= rd_axi_hi;
          end if;
          rd_data_r(2)    <= rd_axi_hi;
        end if;
      end if;   -- not reset
    end if;     -- rising edge
  end process;

  ----------------------------------------------
  -- Interface outputs to the S_AXI_HP interface
  ----------------------------------------------
  -- axi_hp outputs
  axi_out.ACLK                <= clk;
  -- Write address channel
  axi_out.AWID                <= (others => '0');  -- spec says same id means in-order
  axi_out.AWADDR              <= std_logic_vector(axi_addr64_r) & "000";
  axi_out.AWLEN               <= axi_len;
  axi_out.AWSIZE              <= "011";        -- we are always 64 bits wide
  axi_out.AWBURST             <= "01";         -- we are always doing incrementing bursts
  axi_out.AWLOCK              <= "00";         -- normal access, no locking or exclusion
  axi_out.AWCACHE             <= (others => '0');
  axi_out.AWPROT              <= (others => '0');
  axi_out.AWVALID             <= pkt_writing_r and pkt_active_r;

  -- Write data channel
  axi_out.WID                 <= (others => '0');  -- spec says same id means in-order
  axi_out.WDATA(31 downto 0)  <= swap(wr_dwords(to_integer(wr_axi_lo_idx)));
  axi_out.WDATA(63 downto 32) <= swap(wr_dwords(to_integer(wr_axi_hi_idx)));
  axi_out.WSTRB               <= wr_axi_strobe;
  axi_out.WLAST               <= wr_axi_last;
  axi_out.WVALID              <= wr_axi_valid;

  -- Write response channel
  axi_out.BREADY              <= '1';              -- we are always ready for responses

  -- Read address channel
  axi_out.ARID                <= (others => '0');
  axi_out.ARADDR              <= std_logic_vector(axi_addr64_r) & "000";
  axi_out.ARLEN               <= axi_len;
  axi_out.ARSIZE              <= "011"; -- we are always 64 bits wide
  axi_out.ARBURST             <= "01";  -- we are always doing incrementing bursts
  axi_out.ARLOCK              <= "00";  -- normal access, no locking or exclusion
  axi_out.ARCACHE             <= (others => '0');
  axi_out.ARPROT              <= (others => '0');
  axi_out.ARVALID             <= pkt_active_r and not pkt_writing_r;

  -- Read response channel
  axi_out.RREADY              <= rd_axi_ready;

  -- These are not AMBA/AXI
  axi_out.ARQOS               <= (others => '0');
  axi_out.AWQOS               <= (others => '0');
  axi_out.RDISSUECAP1_EN      <= '0';
  axi_out.WRISSUECAP1_EN      <= '0';

  ----------------------------------------------
  -- Interface output signals to the unoc interface
  -- Since this is a master-only interface, the only outputs will be read completions
  ----------------------------------------------
  unoc_out.clk          <= clk;
  unoc_out.reset_n      <= not reset;
  unoc_out.id           <= (others => '0'); -- only needed by PCI
  unoc_out.data.sof     <= rd_sof_r;
  unoc_out.data.eof     <= rd_eof;
  unoc_out.data.hit     <= (others => '0');
  unoc_out.data.be      <= (others => '0');
  unoc_out.valid        <= rd_unoc_valid;
  dbg_state <= to_ulonglong(
    std_logic_vector(axi_addr64_r) & "000"
    & -- 32
    slv(pkt_active_r) & -- 31
    slv(pkt_aligned_r) & --30
    slv(pkt_writing_r) & --29
    slv(pkt_odd_ndw_r) & --28
    "0" & --27
    std_logic_vector(rd_ndw_left_r) & --26 to 16
    "0" & -- 16
    std_logic_vector(pkt_naxf_left_r) & -- 15 to 4
    "0000"); -- 28

  dbg_state1 <= to_ulonglong(
    "000" & std_logic_vector(axi_burst_len_r) &  -- 8 (63 to 56)
    "00" & std_logic_vector(wr_naxf_left_for_uxf_r) & -- 55 downto 52
    "0" & std_logic_vector(rd_ndw_for_uxf_r) & --8
    slv(rd_sof_r) & -- 1
    slv(unoc_in.valid) &
    slv(unoc_in.data.sof) & -- 2
    "0000100100011" &
    "0000000000000000000000000000" & slv(pkt_starting) & std_logic_vector(rd_ndw_for_uxf_r));
    
end rtl;
