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
-- This module adapts between the uNOC and the AXI read data channel
-- It is a subsidiary module to the unoc2axi dma adapter.
library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library platform; use platform.platform_pkg.all;
library ocpi; use ocpi.types.all, ocpi.util.all;
library zynq; use zynq.zynq_pkg.all;

entity unoc2axi_rd is
  generic(
    ocpi_debug       : boolean;
    axf_per_uxf      : natural
    );
  port(
    clk              : in std_logic;
    reset            : in bool_t;
    pkt_starting     : in  bool_t;            -- static during a packet
    unoc_take        : in  bool_t;            -- unoc xfer is being taken now
    unoc_in          : in  unoc_master_in_t;
    unoc_out         : out unoc_master_out_t;
    axi_in           : in  s_axi_hp_out_r_t;  -- read data channel in to here
    axi_out          : out s_axi_hp_in_r_t;   -- read data channel out from here
    debug            : out ulonglong_t
    );
end entity unoc2axi_rd;
architecture rtl of unoc2axi_rd is
  subtype naxf_in_uxf_t is unsigned(width_for_max(axf_per_uxf)-1 downto 0);
  subtype ndw_in_uxf_t is unsigned(width_for_max(unoc_ndw_c)-1 downto 0);
  subtype ndw_t is unsigned(unoc_dw_length_bits_c downto 0);
  -- UNOC request decoding and internal outputs
  signal unoc_req               : unoc_request_t;
  signal unoc_addr              : dword_t;
  signal unoc_ndw               : unoc_ndw_t;
  -- State of the unoc pkt currently being processed
  signal pkt_aligned_r          : bool_t;
  -- Read data processing state and internal versions of output signals
  signal rd_ndw_left_r          : ndw_t;
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
begin
  --------------------------------------------------------------------------------
  -- UNOC computations *FROM* unoc signals
  --------------------------------------------------------------------------------
  unoc_req     <= to_unoc_request(unoc_in.data.payload(1));
  unoc_ndw     <= unoc_get_ndw(to_unoc_header(unoc_in.data.payload(0)));
  unoc_addr    <= unoc_in.data.payload(2);
  --------------------------------------------------------------------------------
  -- AXI read data channel: connect it to the unoc read data path
  --------------------------------------------------------------------------------
  -- Offer a uxf to the unoc when there are enough dw available for the uxf
  -- Note the case when this condition is satified by a leftover (hi) DW
  -- and no current AXI read data
  rd_unoc_valid    <= to_bool(rd_naxf_left_uxf_r = 1 and axi_in.VALID);
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
  rd_axi_lo        <= swap(axi_in.DATA(31 downto 0));
  rd_axi_hi        <= swap(axi_in.DATA(63 downto 32));
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
  -- Clocked State processing 
  --------------------------------------------------------------------------------
  doclk : process(clk)
  begin
    if rising_edge(clk) then
      if reset = '1' then
        -- We'll list all registers, but comment out those that don't really need init
        pkt_aligned_r          <= '0';
        rd_ndw_left_r          <= (others => '0');
        rd_ndw_for_uxf_r       <= (others => '0');
        rd_naxf_left_uxf_r     <= (others => '0');
        rd_sof_r               <= '0';
        --rd_data_r              <= (others => (others => '0'));
      else
        --------------------------------------------------------------------------------
        -- Start processing a unoc packet
        --------------------------------------------------------------------------------
        if its(pkt_starting) and
           not its(to_unoc_header(unoc_in.data.payload(0)).has_data) then
          pkt_aligned_r      <= to_bool(unoc_addr(2) = '0');
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
        end if;
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
        elsif rd_axi_ready and axi_in.VALID then
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

  -- Read response channel
  axi_out.READY              <= rd_axi_ready;

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
  unoc_out.take         <= unoc_take;
  debug                 <= (others => '0');
end rtl;
