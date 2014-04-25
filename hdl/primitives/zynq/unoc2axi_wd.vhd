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
-- This module adapts between the uNOC and the AXI write data channel
-- It is a subsidiary module to the unoc2axi dma adapter.
library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library platform; use platform.platform_pkg.all;
library ocpi; use ocpi.types.all, ocpi.util.all;
library zynq; use zynq.zynq_pkg.all;

entity unoc2axi_wd is
  generic(
    ocpi_debug       : boolean;
    axf_per_uxf      : natural
    );
  port(
    clk              : in  std_logic;
    reset            : in  bool_t;
    pkt_starting     : in  bool_t;            -- static during a packet
    pkt_naxf         : in  unoc_ndw_t;        -- valid during startup/sof
    unoc_take        : in  bool_t;            -- unoc xfer is being taken now
    unoc_in          : in  unoc_master_in_t;
    axi_in           : in  s_axi_hp_out_w_t; -- write data channel in to here
    axi_out          : out s_axi_hp_in_w_t;  -- write data channel out from here
    wr_uxf_accepted  : out bool_t;           -- indicate that all the unoc data is accepted
    debug            : out ulonglong_t
    );
end entity unoc2axi_wd;
architecture rtl of unoc2axi_wd is
  subtype naxf_in_uxf_t is unsigned(width_for_max(axf_per_uxf)-1 downto 0);
  -- subtype naxf_in_uxf_t is unsigned(1 downto 0);
  subtype ndw_in_uxf_t is unsigned(width_for_max(unoc_ndw_c)-1 downto 0);
  signal unoc_ndw                : unoc_ndw_t;
  signal unoc_aligned            : bool_t;
  signal unoc_hdr                : unoc_header_t;
  signal pkt_writing_data_r      : bool_t;
  signal pkt_aligned_r           : bool_t;
  signal pkt_odd_ndw_r           : bool_t;
  signal wr_first_axf_r          : bool_t;
  signal wr_naxf_in_first_uxf    : naxf_in_uxf_t;
  signal wr_naxf_in_next_uxf     : naxf_in_uxf_t;
  signal wr_naxf_left_for_uxf_r  : naxf_in_uxf_t;                  -- left in current uxf
  signal wr_naxf_left_for_pkt_r  : unoc_ndw_t;                     -- naxf AFTER current uxf
  signal wr_leftover_dw_r        : dword_t;
  signal wr_dwords               : dword_array_t(0 to unoc_ndw_c); -- extra one for leftover
  signal wr_accepting_data       : bool_t;
  signal wr_axi_lo_idx_r         : ndw_in_uxf_t;
  signal wr_axi_hi_idx_r         : ndw_in_uxf_t;
  signal wr_axi_last             : bool_t;
  signal wr_axi_valid            : bool_t;
  signal wr_axi_strobe           : std_logic_vector(axi_out.STRB'range);
begin
  --------------------------------------------------------------------------------
  -- Initial unoc decoding when starting
  unoc_hdr     <= to_unoc_header(unoc_in.data.payload(0));
  unoc_ndw     <= unoc_get_ndw(unoc_hdr);
  unoc_aligned <= to_bool(unoc_in.data.payload(2)(2) = '0');
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
  wr_accepting_data            <= to_bool(axi_in.READY = '1');
  wr_axi_valid    <= to_bool(its(unoc_in.valid) and pkt_writing_data_r and
                             wr_naxf_left_for_uxf_r /= 0);
  wr_axi_last     <= to_bool((wr_naxf_left_for_uxf_r = 1) and (wr_naxf_left_for_pkt_r = 0));
  wr_axi_strobe   <= "00001111"
                     when its(wr_axi_last) and
                          ((its(pkt_aligned_r) and pkt_odd_ndw_r) or
                           (not its(pkt_aligned_r) and not its(pkt_odd_ndw_r))) else
                     "11110000"
                     when wr_first_axf_r and not its(pkt_aligned_r) else
                     "11111111";
  -- We have accepted the current uxf
  wr_uxf_accepted <= to_bool(its(unoc_in.valid) and pkt_writing_data_r and
                             (wr_naxf_left_for_uxf_r = 0 or
                              (wr_naxf_left_for_uxf_r = 1 and wr_accepting_data)));
  --------------------------------------------------------------------------------
  -- Clocked State processing 
  --------------------------------------------------------------------------------
  doclk : process(clk)
  begin
    if rising_edge(clk) then
      if reset = '1' then

        pkt_writing_data_r     <= bfalse;
        wr_first_axf_r         <= bfalse;
        wr_naxf_left_for_uxf_r <= (others => '0');
        wr_naxf_left_for_pkt_r <= (others => '0');
        --wr_leftover_dw_r       <= (others => '0');
        wr_axi_lo_idx_r        <= (others => '0');
        wr_axi_hi_idx_r        <= (others => '0');

      else
        --------------------------------------------------------------------------------
        -- Start processing a unoc packet
        --------------------------------------------------------------------------------
        if pkt_starting and unoc_hdr.has_data then
          --------------------------------------------------------------------------------
          -- Initializations for writing data
          pkt_writing_data_r     <= btrue;
          pkt_aligned_r          <= unoc_aligned;
          pkt_odd_ndw_r          <= unoc_ndw(0);
          wr_first_axf_r         <= btrue;
          wr_naxf_left_for_uxf_r <= wr_naxf_in_first_uxf;
          wr_naxf_left_for_pkt_r <= pkt_naxf - wr_naxf_in_first_uxf;
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
        elsif its(pkt_writing_data_r) then
          --------------------------------------------------------------------------------
          -- Write data flow processing during a packet
          --------------------------------------------------------------------------------
          -- Always grab the last word in case we need it for writing later
          if its(unoc_take) then
            wr_leftover_dw_r <= unoc_in.data.payload(unoc_ndw_c-1);
            if wr_naxf_left_for_uxf_r = 0  then
              -- We are skipping a uxf that is at the start of the packet
              -- It cannot be the end of the packet.
              wr_naxf_left_for_uxf_r <= wr_naxf_in_next_uxf;
              wr_naxf_left_for_pkt_r <= wr_naxf_left_for_pkt_r - wr_naxf_in_next_uxf;
            end if;
          end if;  
          -- If there is a uxf and we need some of it and we can accept it...
          if its(unoc_in.valid) and wr_accepting_data and wr_naxf_left_for_uxf_r /= 0 then
            -- We are moving some data NOW
            wr_first_axf_r         <= bfalse;
            if wr_naxf_left_for_uxf_r = 1 then
              -- We are done with the current UXF - doing the last AXF for it
              if wr_naxf_left_for_pkt_r = 0 then
                -- We're done with the whole packet.  Stop the writing
                wr_naxf_left_for_uxf_r <= (others => '0');
                pkt_writing_data_r <= bfalse;
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
          end if; -- if data is moving
        end if;   -- if pkt_writing_data_r
      end if;     -- if not reset
    end if;       -- if rising edge
  end process;

  -----------------------------------------------------------------
  -- Interface outputs to the S_AXI_HP write data channel interface
  -----------------------------------------------------------------
  -- Write data channel
  axi_out.ID                 <= (others => '0');  -- spec says same id means in-order
  axi_out.DATA(31 downto 0)  <= swap(wr_dwords(to_integer(wr_axi_lo_idx_r)));
  axi_out.DATA(63 downto 32) <= swap(wr_dwords(to_integer(wr_axi_hi_idx_r)));
  axi_out.STRB               <= wr_axi_strobe;
  axi_out.LAST               <= wr_axi_last;
  axi_out.VALID              <= wr_axi_valid;
  -----------------------------------------------------------------
  -- Debug status output - just wires here.
  -----------------------------------------------------------------
  debug <= to_ulonglong(
    "0" &                            -- 63
    slv(wr_accepting_data) &  -- 1      62
    slv(wr_axi_last)       &  -- 1      61
    slv(wr_axi_valid)      &  -- 1 -- 4 60
    "0" &                               -- 59
    std_logic_vector(wr_axi_lo_idx_r)   &  -- 4 56
    "0" &
    std_logic_vector(wr_axi_hi_idx_r)   &  -- 4 52
    "00" &
    std_logic_vector(wr_naxf_in_first_uxf) & -- 4 -- 16 48
    "00" &
    std_logic_vector(wr_naxf_in_next_uxf) &  -- 4 -- 20 44
    "00" &
    std_logic_vector(wr_naxf_left_for_uxf_r) & -- 4 --  40
    std_logic_vector(wr_naxf_left_for_pkt_r(7 downto 0)) & -- 8 -- 32
    wr_leftover_dw_r); -- 0

end rtl;
