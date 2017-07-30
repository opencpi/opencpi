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

-- Adapt the input width to the output width.
-- Most of the complexity here is dealing with message sizes that do not divide evenly
-- with the larger width, and to not add any latency (insert idle cycles).
library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.types.all, ocpi.util.all; -- remove this to avoid all ocpi name collisions
architecture rtl of wsi_width_adapter_worker is
  constant win_c       : natural := to_integer(width_in);
  constant wout_c      : natural := to_integer(width_out);
  constant in_bytes_c  : natural := win_c/8;
  constant out_bytes_c : natural := wout_c/8;
begin
  -----------------------------------------------------------------------------------------
  -- This first generate is for expanding the width.  Below is for contraction.
  -----------------------------------------------------------------------------------------
  small2big: if win_c < wout_c generate
    constant last_c : natural := wout_c/win_c - 1;
    type buf_t is array (0 to last_c) of std_logic_vector(win_c-1 downto 0);
    type be_t is array (0 to last_c) of std_logic_vector(in_bytes_c-1 downto 0);
    signal could_take   : bool_t;
    signal take_now     : bool_t;
    signal give_now     : bool_t;
    signal last_in_now  : bool_t; -- input is last in output word, not qualified by in_in.ready
    -- State
    signal data_r       : buf_t;
    signal be_r         : be_t;
    signal full_r       : bool_t; -- we are holding a complete output word
    signal index_r      : unsigned(width_for_max(last_c) downto 0);
    signal som_r        : bool_t; -- is the buffered data at SOM? (must be first)
    signal valid_r      : bool_t; -- does the buffered data have data? (first must be valid)
    signal eom_r        : bool_t; -- is the buffered data at EOM? (must be last)
  begin
    assert (wout_c rem win_c) = 0 report "width_out must be a multiple of width_in";
    could_take     <= out_in.ready or not full_r;
    take_now       <= could_take and in_in.ready;
    -- is the current input word the last in an output word?
    last_in_now    <= to_bool((in_in.valid and in_in.byte_enable /= slv1(in_in.byte_enable'length)) or
                              not its(in_in.valid) or               -- standalone SOM?
                              in_in.eom or                     -- eom with or without data
                              index_r = last_c);               -- last data in output word
    give_now       <= to_bool(out_in.ready and (full_r or (its(in_in.ready) and last_in_now)));
    in_out.take    <= take_now;
    out_out.eom    <= eom_r when its(full_r) else in_in.eom;
    out_out.som    <= som_r when full_r or index_r /= 0 else in_in.som;
    out_out.valid  <= valid_r when its(full_r) else
                      valid_r or in_in.valid when index_r /= 0 else
                      in_in.valid;
    out_out.give   <= give_now;
    out_out.opcode <= in_in.opcode;
    g0: for i in 0 to last_c generate
      out_out.data(win_c * i + win_c - 1 downto win_c * i) <= 
        in_in.data        when not its(full_r) and last_in_now and index_r = i else
        data_r(i);
      out_out.byte_enable(in_bytes_c * i + in_bytes_c - 1 downto in_bytes_c * i) <=
        in_in.byte_enable when not its(full_r) and last_in_now and index_r = i else
        be_r(i);
    end generate g0;

    p: process(wci_clk) is
    begin
      if rising_edge(wci_clk) then
        if its(wci_reset) then
          full_r  <= bfalse;
          index_r <= (others => '0');
        elsif its(take_now) then
          -- capture into buffer
          data_r(to_integer(index_r)) <= in_in.data;
          be_r(to_integer(index_r))   <= in_in.byte_enable;
          if index_r = 0 then
            som_r                     <= in_in.som;
            valid_r                   <= in_in.valid;
          end if;
          eom_r                       <= in_in.eom; -- will force last_in_now
          if its(last_in_now) then
            index_r                   <= (others => '0');
            full_r                    <= full_r or not out_in.ready;
          else
            full_r                    <= bfalse;
            index_r                   <= index_r + 1;
          end if;
        elsif its(give_now) then
          full_r <= bfalse;
        end if;
      end if; -- end of rising edge
    end process;
  end generate small2big;
  -----------------------------------------------------------------------------------------
  -- Earlier generate was for expanding the width.  This one is for the contracting case
  -----------------------------------------------------------------------------------------
  big2small: if win_c > wout_c generate
    constant last_c        : natural := win_c/wout_c - 1;
    constant idx_width_c   : natural := ocpi.util.width_for_max(last_c);
    -- A count that is large enough for byte offsets in an inword
    subtype ibcount_t is unsigned(ocpi.util.width_for_max(in_bytes_c-1)-1 downto 0);
    -- A count that is large enough for outword offsets in an inword
    subtype owcount_t is unsigned(ocpi.util.width_for_max(last_c)-1 downto 0);
    type buf_t is array (0 to last_c) of std_logic_vector(wout_c-1 downto 0);
    signal in_data      : buf_t;   -- reorg of input data into words
    signal take_now     : bool_t;  -- we are taking in this cycle
    signal give_now     : bool_t;  -- we are giving in this cycle
    signal could_take   : bool_t;  -- we could take if there was data offered
    signal last_word_in : owcount_t; -- the index of the last word of the incoming record
    signal last_be_in   : std_logic_vector(out_bytes_c-1 downto 0); -- BE for the last OW in IW
    signal on_last_word : bool_t;
    signal byte_enable  : std_logic_vector(out_bytes_c-1 downto 0);
    -- State
    signal have_data_r  : bool_t;  -- we are holding on to some data
    signal index_r      : owcount_t; -- which outword is being presented from the inword now.
    signal last_r       : owcount_t; -- which outword is the last one in this saved inword
    signal som_r        : bool_t;  -- we buffered a SOM
    signal eom_r        : bool_t;  -- we buffered an EOM
    signal data_r       : buf_t;   -- Buffer of inword
    signal last_be_r    : std_logic_vector(out_bytes_c-1 downto 0); -- BE for last outword
    -- Compute the byte offset of the last outword in the inword.
    procedure find_extent(in_be : std_logic_vector(in_bytes_c-1 downto 0);
                          signal out_be : out std_logic_vector(out_bytes_c-1 downto 0);
                          signal out_word : out owcount_t) is
      variable bit : natural := 0;
      variable ow : owcount_t := (others => '0');
    begin
      for i in 0 to last_c-1 loop
        exit when in_be(bit+out_bytes_c-1 downto bit) /= ocpi.util.slv1(out_bytes_c); -- LE
        bit := bit + out_bytes_c;
        ow  := ow + 1;
      end loop;
      out_be   <= in_be(bit+out_bytes_c-1 downto bit);
      out_word <= ow;
    end find_extent;
  begin
    assert (width_in rem width_out) = 0 report "width_in must be a multiple of width_out";
    -- Reorganize input data as an array of output words
    g0: for i in 0 to last_c generate
      in_data(i) <= in_in.data(wout_c*i + wout_c-1 downto wout_c*i);
    end generate g0;
    -- Find out how many outwords we have and the BE of the last one.
    find_extent(in_in.byte_enable, last_be_in, last_word_in);
    -- Are we about to output the last word of data we have?
    on_last_word   <= to_bool(have_data_r and index_r = last_r);
    -- We can take if we can buffer the data or have buffered data and it will be given
    could_take     <= to_bool(not its(have_data_r) or (its(on_last_word) and out_in.ready));
    take_now       <= to_bool(its(in_in.ready) and could_take);
    in_out.take    <= take_now;
    give_now       <= to_bool(out_in.ready and (its(have_data_r) or in_in.ready));
    out_out.give   <= give_now;
    out_out.eom    <= eom_r when its(on_last_word) else
                      in_in.eom when not its(have_data_r) and last_word_in = 0 else
                      bfalse;
    out_out.som    <= som_r when its(have_data_r) and index_r = 0 else
                      in_in.som when not have_data_r else
                      bfalse;
    out_out.valid  <= to_bool(last_be_r /= slv0(out_bytes_c)) when its(on_last_word) else
                      btrue when its(have_data_r) else
                      in_in.valid;
    out_out.data   <= data_r(to_integer(index_r)) when its(have_data_r) else in_data(0);
    out_out.opcode <= in_in.opcode;
    out_out.byte_enable <= last_be_r when its(on_last_word) else
                           slv1(out_bytes_c) when its(have_data_r) else
                           in_in.byte_enable(out_bytes_c-1 downto 0);

    p: process(wci_clk) is
    begin
      if rising_edge(wci_clk) then
        if wci_reset = '1' then
          have_data_r <= bfalse;
          index_r     <= (others => '0');
          last_r      <= (others => '0');
          som_r       <= bfalse;
          eom_r       <= bfalse;
        else
          -- Capture everything when we can, whether its there or not
          if its(could_take) then
            data_r <= in_data;
            last_r <= last_word_in; -- remember the last word in the input
            last_be_r <= last_be_in;
            som_r  <= in_in.som;
            eom_r  <= in_in.eom;
            -- If we are actually buffering any data (not passing through), set the indicator
            if take_now and (have_data_r or
                             ((last_word_in /= 0 or not its(out_in.ready)))) then
              have_data_r <= btrue;
            end if;
            if out_in.ready and not its(have_data_r) then
              index_r <= to_unsigned(1, index_r'length);
            else
              index_r <= to_unsigned(0, index_r'length);
            end if;
          end if;
          -- Advance the pointer if we are giving from the buffer
          if its(give_now) and have_data_r then
            if index_r /= last_r then
              index_r <= index_r + 1;
            elsif not take_now then
              have_data_r <= bfalse;
            end if;
          end if;
        end if; -- end of not reset
      end if; -- end of rising edge
    end process;
  end generate big2small;
  same_size: if win_c = wout_c generate
    in_out.take         <= to_bool(its(in_in.ready) and out_in.ready);
    out_out.give        <= to_bool(its(in_in.ready) and out_in.ready);
    out_out.data        <= in_in.data;
    out_out.byte_enable <= in_in.byte_enable;
    out_out.valid       <= in_in.valid;
    out_out.som         <= in_in.som;
    out_out.eom         <= in_in.eom;
    out_out.opcode      <= in_in.opcode;
  end generate same_size;

end rtl;
