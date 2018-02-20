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

-- This module adapts between the uNOC and the AXI interface
-- The initial target AXI configuration is a 64 bit version of the AXI_HP ports,
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
library work; use work.axi_pkg.all;

entity unoc2axi is
  generic(
    ocpi_debug : boolean
    );
  port(
    clk       : in std_logic;
    reset     : in bool_t;
    unoc_in   : in  unoc_master_in_t;
    unoc_out  : out unoc_master_out_t;
    axi_in    : in  s_axi_hp_out_t;
    axi_out   : out s_axi_hp_in_t;
    axi_error : out bool_t;
    dbg_state : out ulonglong_t;
    dbg_state1: out ulonglong_t;
    dbg_state2: out ulonglong_t
    );
end entity unoc2axi;
architecture rtl of unoc2axi is
  --------------------------------------------------------------------------------
  -- Terminology:
  -- xf     the beat of a data path - a "transfer"
  -- uxf    the "beat" of the unoc data path when valid (128 bits wide here)
  -- pkt    the 1-or-more uxf unoc message w/ request/address and (if a write) write data
  -- axf    the "beat" of the axi data path (64 bits wide here)
  -- burst  the (up to 16 axf) burst of data over AXI at a single requested address
  -- dw:    32 bit word (Microsoft's and PCI's anachronistic DWORD)
  -- qw:    64 bit word
  -- uw:    unoc word (payload of unoc transfer)
  -- aw:    axi word (currently 64b)
  --------------------------------------------------------------------------------
  constant dw_bits_c       : natural := 32;
  constant qw_bits_c       : natural := 64;
  constant aw_bits_c       : natural := 64;
  constant uw_bits_c       : natural := dw_bits_c * unoc_ndw_c;
  constant uw_bytes_c      : natural := uw_bits_c/8;
  constant aw_bytes_c      : natural := aw_bits_c/8;
  constant axi_max_burst_c : natural := 16;
  constant axf_per_uxf_c   : natural := uw_bytes_c/aw_bytes_c;
  constant axf_per_uxf_width_c   : natural := width_for_max(axf_per_uxf_c)-1;
  -- UNOC request decoding and internal outputs
  signal unoc_hdr               : unoc_header_t;
  signal unoc_addr              : dword_t;
  signal unoc_ndw               : unoc_ndw_t;
  signal unoc_writing           : bool_t;
  signal unoc_take              : bool_t;
  -- Packet calculations during startup
  signal pkt_starting           : bool_t;
  signal pkt_naxf               : unoc_ndw_t;
  -- State of the unoc pkt currently being processed
  signal pkt_active_r           : bool_t; -- addressing machine is active
  signal pkt_writing_r          : bool_t; -- writing state for the ADDRESSING machine
  signal pkt_naxf_left          : unsigned(unoc_hdr.dw_length'left + 1 downto 0);
  signal pkt_naxf_left_r        : unsigned(unoc_hdr.dw_length'left + 1 downto 0);
  -- AXI addressing calculations and state (read or write)
  signal axi_addr64_r           : unsigned(axi_out.AW.ADDR'left - 3 downto 0);
  signal axi_accepting_addr     : bool_t;
  signal axi_burst_len          : unsigned(width_for_max(axi_max_burst_c)-1 downto 0);
  signal axi_burst_len_r        : unsigned(width_for_max(axi_max_burst_c)-1 downto 0);
  signal axi_error_r            : bool_t; -- stick error for internal use
  signal axi_len                : std_logic_vector(axi_out.AW.LEN'range);
  -- Write data output indication
  signal wr_uxf_accepted        : bool_t;
begin
  --------------------------------------------------------------------------------
  -- UNOC computations *FROM* unoc signals
  --------------------------------------------------------------------------------
  unoc_hdr     <= to_unoc_header(unoc_in.data.payload(0));
  unoc_ndw     <= unoc_get_ndw(unoc_hdr);
  unoc_addr    <= unoc_in.data.payload(2);
  unoc_writing <= unoc_hdr.has_data;
  --------------------------------------------------------------------------------
  -- UNOC computations *TO* unoc signals
  --------------------------------------------------------------------------------
  -- dequeue unoc xfer when we start reading (since everything is captured on sof)
  -- or when we have a write data uxf and the write data machine has accepted
  -- all the data in the uxf
  unoc_take <= to_bool((pkt_starting and not its(unoc_writing)) or wr_uxf_accepted);
  --------------------------------------------------------------------------------
  -- PKT processing
  -- There is one cycle of latency from when the unoc request packet is available
  -- and the first axi transaction/burst for that packet can start
  --------------------------------------------------------------------------------
  -- We can start processing a packet if we're not active
  -- We will start on a valid unoc SOF, except for a sticky AXI error
  pkt_starting  <= to_bool(unoc_in.valid and its(unoc_in.data.sof) and
                           not its(pkt_active_r) and not its(axi_error_r));
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
                                ((its(pkt_writing_r) and axi_in.AW.READY = '1') or
                                 (not its(pkt_writing_r) and axi_in.AR.READY = '1')));
  -- Encode the burst length for AXI
  axi_len            <= std_logic_vector(resize(axi_burst_len_r - 1, axi_len'length));
  axi_error          <= axi_error_r;

  --------------------------------------------------------------------------------
  -- Clocked State processing 
  --------------------------------------------------------------------------------
  doclk : process(clk)
  begin
    if rising_edge(clk) then
      if reset = '1' then
        -- We'll list all registers, but comment out those that don't really need init
        pkt_active_r           <= '0';
        pkt_writing_r          <= '0';
        pkt_naxf_left_r        <= (others => '0');

        -- axi_addr64_r           <= (others => '0');
        -- axi_burst_len_r        <= (others => '0');
        axi_error_r            <= '0';
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
          pkt_writing_r   <= unoc_writing;
          -- Amoung left *AFTER* the current burst
          pkt_naxf_left_r <= pkt_naxf - axi_burst_len;
          axi_addr64_r    <= resize(unsigned(unoc_addr(unoc_addr'left downto 3)),
                                    axi_addr64_r'length);
          axi_burst_len_r <= axi_burst_len;
        elsif its(pkt_active_r) then
          ------------------------------------------------------------------------------
          -- Address/burst processing when not starting
          -- The current burst request is being enqueued/accepted, we can advance it.
          ------------------------------------------------------------------------------
          if its(axi_accepting_addr) then
            if pkt_naxf_left_r = 0 then
              pkt_active_r    <= bfalse;
              pkt_writing_r   <= bfalse;
            else
              axi_addr64_r    <= axi_addr64_r + axi_max_burst_c;
              axi_burst_len_r <= axi_burst_len;
              pkt_naxf_left_r <= pkt_naxf_left_r - axi_burst_len;
            end if;
          end if;
          -- Advance the write response channel by just checking for errors
          if axi_in.B.VALID = '1' and axi_in.B.RESP /= Resp_OKAY then
            axi_error_r <= '1';
          end if;
        end if; -- pkt_active_r
      end if;   -- not reset
    end if;     -- rising edge
  end process;

  ----------------------------------------------
  -- Instantiate the write data channel module
  wd : entity work.unoc2axi_wd
    generic map (
      ocpi_debug       => ocpi_debug,
      axf_per_uxf      => axf_per_uxf_c
      )
    port map (
      clk              => clk,
      reset            => reset,
      pkt_starting     => pkt_starting,
      pkt_naxf         => pkt_naxf,
      unoc_take        => unoc_take,
      unoc_in          => unoc_in,
      axi_in           => axi_in.w,
      axi_out          => axi_out.w,
      wr_uxf_accepted  => wr_uxf_accepted,
      debug            => dbg_state1
      );

  ----------------------------------------------
  -- Instantiate the read data channel module
  rd : entity work.unoc2axi_rd
    generic map (
      ocpi_debug       => ocpi_debug,
      axf_per_uxf      => axf_per_uxf_c
      )
    port map (
      clk          => clk,
      reset        => reset,
      pkt_starting => pkt_starting,
      unoc_take    => unoc_take,
      unoc_in      => unoc_in,
      unoc_out     => unoc_out,
      axi_in       => axi_in.r,
      axi_out      => axi_out.r,
      debug        => dbg_state2
      );

  ----------------------------------------------
  -- Interface outputs to the S_AXI_HP interface
  ----------------------------------------------
  -- axi_hp outputs
  axi_out.ACLK                <= clk;
  -- Write address channel
  axi_out.AW.ID                <= (others => '0');  -- spec says same id means in-order
  axi_out.AW.ADDR              <= std_logic_vector(axi_addr64_r) & "000";
  axi_out.AW.LEN               <= axi_len;
  axi_out.AW.SIZE              <= "011";        -- we are always 64 bits wide
  axi_out.AW.BURST             <= "01";         -- we are always doing incrementing bursts
  axi_out.AW.LOCK              <= "00";         -- normal access, no locking or exclusion
  axi_out.AW.CACHE             <= (others => '0');
  axi_out.AW.PROT              <= (others => '0');
  axi_out.AW.VALID             <= pkt_writing_r and pkt_active_r;

  -- Write data channel
  -- wired directly to unoc2axi_wd

  -- Write response channel
  axi_out.B.READY              <= '1';              -- we are always ready for responses

  -- Read address channel
  axi_out.AR.ID                <= (others => '0');
  axi_out.AR.ADDR              <= std_logic_vector(axi_addr64_r) & "000";
  axi_out.AR.LEN               <= axi_len;
  axi_out.AR.SIZE              <= "011"; -- we are always 64 bits wide
  axi_out.AR.BURST             <= "01";  -- we are always doing incrementing bursts
  axi_out.AR.LOCK              <= "00";  -- normal access, no locking or exclusion
  axi_out.AR.CACHE             <= (others => '0');
  axi_out.AR.PROT              <= (others => '0');
  axi_out.AR.VALID             <= pkt_active_r and not pkt_writing_r;

  -- These are not AMBA/AXI
  axi_out.AR.QOS               <= (others => '0');
  axi_out.AW.QOS               <= (others => '0');
  axi_out.AR.ISSUECAP1_EN      <= '0';
  axi_out.AW.ISSUECAP1_EN      <= '0';

  dbg_state <= to_ulonglong(
    std_logic_vector(axi_addr64_r) & "000"
    & -- 32
    slv(pkt_active_r) & -- 31
    "0" & --30
    slv(pkt_writing_r) & --29
    "0" & --28
    "0" & --27
    "00000000000" &
    "0" & -- 16
    std_logic_vector(pkt_naxf_left_r) & -- 15 to 4
    "0000"); -- 28
    
end rtl;
