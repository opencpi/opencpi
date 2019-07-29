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

-- Adapt width when in and out are non-zero width
entity adapt_width is
  generic (
    width_in  : natural;
    width_out : natural);
  port (
    clk, reset      : std_logic;
    in_ready        : in bool_t;
    in_som, in_valid, in_eom, in_eof : in bool_t;
    in_data         : in std_logic_vector(width_in-1 downto 0);
    in_byte_enable  : in std_logic_vector(width_in/8-1 downto 0);
    in_opcode       : in std_logic_vector(7 downto 0);
    in_take         : out bool_t;

    out_ready       : in bool_t;
    out_give        : out bool_t;
    out_som, out_valid, out_eom, out_eof : out bool_t;
    out_data        : out std_logic_vector(width_out-1 downto 0);
    out_byte_enable : out std_logic_vector(width_out/8-1 downto 0);
    out_opcode      : out std_logic_vector(7 downto 0));
end entity adapt_width;
architecture rtl of adapt_width is
  constant bytes_in  : natural := width_in/8;
  constant bytes_out : natural := width_out/8;
begin
  out_eof <= in_eof;
  -----------------------------------------------------------------------------------------
  -- This first generate is for expanding the width.  Below is for contraction.
  -----------------------------------------------------------------------------------------
  small2big: if width_in < width_out generate
    constant last_c : natural := width_out/width_in - 1;
    type buf_t is array (0 to last_c) of std_logic_vector(width_in-1 downto 0);
    type be_t is array (0 to last_c) of std_logic_vector(bytes_in-1 downto 0);
    signal could_take   : bool_t;
    signal take_now     : bool_t;
    signal give_now     : bool_t;
    signal out_my_valid : bool_t;
    signal last_in_now  : bool_t; -- input is last in output word, not qualified by in_ready
    -- State
    signal data_r       : buf_t := (others=> (others=>'0')); --initialization required for simulation test
    signal opcode_r     : std_logic_vector(in_opcode'range);
    signal be_r         : be_t;
    signal full_r       : bool_t; -- we are holding a complete output word
    signal index_r      : unsigned(width_for_max(last_c) downto 0);
    signal som_r        : bool_t; -- is the buffered data at SOM? (must be first)
    signal valid_r      : bool_t; -- does the buffered data have data? (first must be valid)
    signal eom_r        : bool_t; -- is the buffered data at EOM? (must be last)
  begin
    assert (width_out rem width_in) = 0 report "width_out must be a multiple of width_in";
    could_take     <= out_ready or not full_r when not its(last_in_now) else out_ready and not full_r;
    take_now       <= could_take and in_ready;
    -- is the current input word the last in an output word?
    last_in_now    <= to_bool((in_valid and in_byte_enable /= slv1(in_byte_enable'length)) or
                              not its(in_valid) or               -- standalone SOM?
                              in_eom or                     -- eom with or without data
                              index_r = last_c);               -- last data in output word
    give_now       <= to_bool(out_ready and (full_r or (its(in_ready) and last_in_now)));
    in_take        <= take_now;
    out_eom        <= eom_r when its(full_r) else in_eom;
    out_som        <= som_r when full_r or index_r /= 0 else in_som;
    out_my_valid   <= valid_r when its(full_r) else
                      valid_r or in_valid when index_r /= 0 else
                      in_valid;
    out_valid      <= out_my_valid when its(give_now) else bfalse;
    out_give       <= give_now;
    out_opcode     <= in_opcode when not its(full_r) and last_in_now else opcode_r;
    g0: for i in 0 to last_c generate
      out_data(width_in * i + width_in - 1 downto width_in * i) <= 
        in_data        when not its(full_r) and last_in_now and index_r = i else
        data_r(i);
      out_byte_enable(bytes_in * i + bytes_in - 1 downto bytes_in * i) <=
        in_byte_enable when not its(full_r) and last_in_now and index_r = i else
        (others => '0') when last_in_now and i > index_r else
        be_r(i);
    end generate g0;

    p: process(clk) is
    begin
      if rising_edge(clk) then
        if its(reset) then
          full_r  <= bfalse;
          index_r <= (others => '0');
        elsif its(take_now) then
          -- capture into buffer
          data_r(to_integer(index_r)) <= in_data;
          opcode_r                    <= in_opcode;
          be_r(to_integer(index_r))   <= in_byte_enable;
          if index_r = 0 then
            som_r                     <= in_som;
            valid_r                   <= in_valid;
          end if;
          eom_r                       <= in_eom; -- will force last_in_now
          if its(last_in_now) then
            index_r                   <= (others => '0');
            full_r                    <= full_r or not out_ready;
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
  big2small: if width_in > width_out generate
    constant last_c        : natural := width_in/width_out - 1;
    constant idx_width_c   : natural := ocpi.util.width_for_max(last_c);
    -- A count that is large enough for byte offsets in an inword
    subtype ibcount_t is unsigned(ocpi.util.width_for_max(bytes_in-1)-1 downto 0);
    -- A count that is large enough for outword offsets in an inword
    subtype owcount_t is unsigned(ocpi.util.width_for_max(last_c)-1 downto 0);
    type buf_t is array (0 to last_c) of std_logic_vector(width_out-1 downto 0);
    signal word_data      : buf_t;   -- reorg of input data into words
    signal take_now     : bool_t;  -- we are taking in this cycle
    signal give_now     : bool_t;  -- we are giving in this cycle
    signal out_my_valid : bool_t;
    signal could_take   : bool_t;  -- we could take if there was data offered
    signal last_word_in : owcount_t; -- the index of the last word of the incoming record
    signal last_be_in   : std_logic_vector(bytes_out-1 downto 0); -- BE for the last OW in IW
    signal on_last_word : bool_t;
    -- State
    signal have_data_r  : bool_t;  -- we are holding on to some data
    signal index_r      : owcount_t; -- which outword is being presented from the inword now.
    signal last_r       : owcount_t; -- which outword is the last one in this saved inword
    signal som_r        : bool_t;  -- we buffered a SOM
    signal eom_r        : bool_t;  -- we buffered an EOM
    signal data_r       : buf_t;   -- Buffer of inword
    signal opcode_r     : std_logic_vector(in_opcode'range);
    signal last_be_r    : std_logic_vector(bytes_out-1 downto 0); -- BE for last outword
    -- Compute the byte offset of the last outword in the inword.
    procedure find_extent(in_be : std_logic_vector(bytes_in-1 downto 0);
                          in_valid : bool_t;
                          signal out_be : out std_logic_vector(bytes_out-1 downto 0);
                          signal out_word : out owcount_t) is
      variable bit : natural := 0;
      variable ow : owcount_t := (others => '0');
    begin
      if its(in_valid) then -- if no data is valid last byte is first byte
        for i in 0 to last_c-1 loop
          exit when in_be(bit+bytes_out-1 downto bit) /= ocpi.util.slv1(bytes_out); -- LE
          bit := bit + bytes_out;
          ow  := ow + 1;
        end loop;
        out_be   <= in_be(bit+bytes_out-1 downto bit);
      else
        out_be   <= (others => '0');
      end if;
      out_word <= ow;
    end find_extent;
  begin
    assert (width_in rem width_out) = 0 report "width_in must be a multiple of width_out";
    -- Reorganize input data as an array of output words
    g0: for i in 0 to last_c generate
      word_data(i) <= in_data(width_out*i + width_out-1 downto width_out*i);
    end generate g0;
    -- Find out how many outwords we have and the BE of the last one.
    find_extent(in_byte_enable, in_valid, last_be_in, last_word_in);
    -- Are we about to output the last word of data we have?
    on_last_word    <= to_bool(have_data_r and index_r = last_r);
    -- We can take if we can buffer the data or have buffered data and it will be given
    could_take      <= to_bool(not its(have_data_r) or (its(on_last_word) and out_ready));
    take_now        <= to_bool(its(in_ready) and could_take);
    in_take         <= take_now;
    give_now        <= to_bool(out_ready and (its(have_data_r) or in_ready));
    out_give        <= give_now;
    out_eom         <= eom_r when its(on_last_word) else
                       in_eom when not its(have_data_r) and last_word_in = 0 else
                       bfalse;
    out_som         <= som_r when its(have_data_r) and index_r = 0 else
                       in_som when not have_data_r else
                       bfalse;
    out_my_valid    <= to_bool(last_be_r /= slv0(bytes_out)) when its(on_last_word) else
                       btrue when its(have_data_r) else
                       in_valid;
    out_valid       <= out_my_valid when its(give_now) else bfalse;
    out_data        <= data_r(to_integer(index_r)) when its(have_data_r) else word_data(0);
    out_opcode      <= opcode_r when (its(on_last_word) or its(eom_r)) else in_opcode;
    out_byte_enable <= last_be_r when its(on_last_word) else
                       slv1(bytes_out) when its(have_data_r) else
                       in_byte_enable(bytes_out-1 downto 0);

    p: process(clk) is
    begin
      if rising_edge(clk) then
        if reset = '1' then
          have_data_r <= bfalse;
          index_r     <= (others => '0');
          last_r      <= (others => '0');
          som_r       <= bfalse;
          eom_r       <= bfalse;
          opcode_r    <= (others => '0');
        else
          -- Capture everything when we can, whether its there or not
          if its(could_take) then
            data_r    <= word_data;
            last_r    <= last_word_in; -- remember the last word in the input
            last_be_r <= last_be_in;
            som_r     <= in_som and in_ready;
            eom_r     <= in_eom and in_ready;
            opcode_r  <= in_opcode;
            -- If we are actually buffering any data (not passing through), set the indicator
            if take_now and (have_data_r or
                             ((last_word_in /= 0 or not its(out_ready)))) then
              have_data_r <= btrue;
            end if;
            if out_ready and not its(have_data_r) then
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
  same_size: if width_in = width_out generate
    in_take         <= to_bool(its(in_ready) and out_ready);
    out_give        <= to_bool(its(in_ready) and out_ready);
    out_data        <= in_data;
    out_byte_enable <= in_byte_enable;
    out_valid       <= in_valid and in_ready;
    out_som         <= in_som;
    out_eom         <= in_eom;
    out_opcode      <= in_opcode;
  end generate same_size;

end rtl;
