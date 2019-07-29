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

-- These WSI master and slave modules have all possible signals that may be configured
-- It is expected that the generated code on both sides will be deal with the
-- external optionality (on the outside in plumbing to OCP, and on the inside, when plumbing
-- to the worker.

-- The only place the lack of signal optionality carries a cost is in the input/slave side
-- FIFO.  The "optional" signals are at least one bit wide.  The signals that could be zero
-- bits are "data", "datainfo", "burstlength", reqinfo", and "byteen", so in the worst case
-- the FIFO is 5 bits wider than it would be with custom generated code.
--------------------------------------------
-- A WSI slave can deal with either imprecise or precise, but it needs to know
-- whether it needs to convey precise to the worker.
-- so there are really just two modes: insist/assume precise, or deal with everything
-- All potential input ports are present here,
-- and widths of unneeded fields are 1
-- We assume MDataValid/MDataLast.
library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.all; use ocpi.types.all; use ocpi.wsi.all; use ocpi.util.all;

entity slave is
  generic (precise          : boolean; -- are we precise-only?
           mdata_width      : natural; -- width of ocp mdata path
           mdata_info_width : natural; -- width of ocp mdata info path, always includes
                                       -- abort here
           burst_width      : natural; -- burst count field width
           n_bytes          : natural; -- number of bytes
           byte_width       : natural; -- byte_width
           opcode_width     : natural; -- width of opcode, which may be zero
           own_clock        : boolean; -- does the port have a clock different thanthe wci?
           hdl_version      : natural; -- hdl interface version
           early_request    : boolean -- are datavalid and datalast used?
           );
  port (
    -- Exterior OCP input/master signals
    --- this is the same as wci_clock unless metadata says it isn't
    Clk              : in  std_logic;
    --- only used if burst are precise
    MBurstLength     : in  std_logic_vector(burst_width - 1 downto 0);
    --- only used if bytesize < data width or zlm
    MByteEn          : in  std_logic_vector(n_bytes - 1 downto 0);
    MCmd             : in  ocpi.ocp.MCmd_t;
    MData            : in  std_logic_vector(mdata_width-1 downto 0);
    --- only used for aborts or bytesize not 8 and less than datawidth
    MDataInfo        : in  std_logic_vector(mdata_info_width-1 downto 0);
    --- only used if the "early_request" option is selected.
    MDataLast        : in  std_logic;
    MDataValid       : in  std_logic;
    --- LSB is EOF, MSBs if present is opcode
    MReqInfo         : in  std_logic_vector(opcode_width-1 downto 0);
    MReqLast         : in  std_logic;
    MReset_n         : in  std_logic;
    -- Exterior OCP output/slave signals
    SReset_n         : out std_logic;
    SThreadBusy      : out std_logic_vector(0 downto 0);
    -- Signals connected from the worker's WCI to this module;
    wci_clk          : in  std_logic;
    wci_reset        : in  Bool_t;
    wci_is_operating : in  Bool_t;
    -- Non-worker internal signals
    first_take       : out Bool_t; -- the first datum after is_operating taken, a pulse
    -- Interior signals used by worker logic
    take             : in  Bool_t; -- the worker is taking data
    reset            : out Bool_t; -- this port is being reset from outside/peer
    ready            : out Bool_t; -- data can be taken
    -- user visible metadata
    som, eom, valid, eof, abort : out Bool_t;
    data             : out  std_logic_vector(n_bytes*max(1,byte_width)-1 downto 0);
    -- only used if bytes are required (zlm or byte size < data width)
    byte_enable      : buffer std_logic_vector(n_bytes-1 downto 0);
    -- only used if precise is required
    burst_length     : out std_logic_vector(burst_width-1 downto 0);
    -- only used if number of opcodes > 1
    opcode           : out std_logic_vector(max(opcode_width,1)-1 downto 0)
    );
end entity;

library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.all; use ocpi.types.all; use ocpi.wsi.all; use ocpi.util.all;
library bsv; use bsv.bsv.all;
architecture rtl of slave is
  signal reset_i         : Bool_t; -- internal version of output to worker
  -- FIXME: a opcode field is per message, could be optimized
  constant data_width : natural := n_bytes * byte_width;
  constant fifo_width : natural := data_width +         -- actual data bits
                                   n_bytes +            -- byte enables, always at least one
                                   1 +                  -- last in message
                                   1 +                  -- abort (or eof)
                                   burst_width +        -- burstlength field (per OCP)
                                   max(1,opcode_width); -- opcode for message, always at least one

  -- The bundle of signals that goes through the FIFO.
  -- Packing and unpacking records isn't worth it.
  constant data_bits   : natural := fifo_width - 1;
  constant enable_bits : natural := data_bits - data_width;
  constant last_bit    : natural := enable_bits - n_bytes;
  constant abort_bit   : natural := last_bit -1;
  constant burst_bits  : natural := abort_bit - 1;
  constant opcode_bits : natural := burst_bits - burst_width;
  function pack(data   : std_logic_vector(max(data_width,1) - 1 downto 0);
                enable : std_logic_vector(n_bytes - 1 downto 0);
                last   : std_logic;
                abort  : std_logic;
                burst  : std_logic_vector(burst_width - 1 downto 0);
                opcode : std_logic_vector(max(1,opcode_width)-1 downto 0))
    return std_logic_vector is
  begin
    if data_width /= 0 then
      return data & enable & last & abort & burst & opcode;
    else
      return enable & last & abort & burst & opcode;
    end if;
  end pack;

  -- state of message processing
  signal seen_any_r     : bool_t; -- have we seen any messages at all?
  signal at_end_r       : bool_t; -- was the last thing taken an eom, eof, abort?
  signal v1_eof_r       : bool_t; -- a v1 synthesized zlm_eof was taken
  signal eom_added_r    : bool_t; -- an EOM was added to the last worker deque
  -- signals derived from OCP fed into the fifo (other than raw OCP signals)
  signal ocp_last       : std_logic;
  signal ocp_data       : std_logic_vector(max(1,data_width) - 1 downto 0);
  signal ocp_busy_r     : bool_t; -- busy output to OCP
  -- input signals to the fifo;
  signal fifo_in        : std_logic_vector(fifo_width - 1 downto 0);
  signal fifo_enq, fifo_deq : bool_t;
  -- output signals from the fifo
  signal fifo_out       : std_logic_vector(fifo_width - 1 downto 0);
  signal fifo_count_r   : unsigned(2 downto 0); -- 0, 1, 2, 3 4, all valid
  -- derived from fifo outputs
  signal fifo_ready   : bool_t; -- fifo output is ready for worker
  signal fifo_valid   : bool_t; -- the fifo output is presenting valid
  signal fifo_eom     : bool_t; -- the fifo output is presenting EOM
  signal fifo_eof     : bool_t; -- the fifo is presenting EOF
  -- internal combinatorials
  signal zlm_eof      : bool_t; -- a v1 zlm0 for eof is needed
  signal trailing_eom : bool_t; -- a trailing eom is enqueued *behind* the current non-eom output
  signal teom_deq     : bool_t; -- dequeue and discard a trailing eom now
  signal eom_i        : bool_t; -- internal eom: original or added from trailing  or v1 zlm0 for EOF
  signal ready_i      : bool_t; -- internal ready, prequalified by wci_is_operating
  -- FIFO internal state - our customized 3 deep FIFO
  signal fifo_in_r, fifo_middle_in_r, fifo_middle_out_r, fifo_out_r : std_logic_vector(fifo_width-1 downto 0);
begin
  -- Combi resets:
  --   We get wci reset and wsi peer reset (from master)
  --   We produce wsi peer reset (from slave)
  -- Worker sees reset if wci is doing it or we're not started yet or peer is reset
  reset_i  <= wci_reset or not MReset_n; -- FIXME WHEN own_clock
  reset    <= reset_i; -- in wci clock domain for now
  -- Peer sees reset if wci is doing it or we're not started
  SReset_n <= not wci_reset; -- FIXME WHEN OWN CLOCK
  ----------------------------------
  -- fifo inputs - keep decoding minimumal on input side since that is perhaps a synthesis/routing
  -- boundary.  We still enqueue trailing eoms to avoid conditional enqueue
  -- If there are parts of bytes in data_info_width, combine them nicely for the worker
  -- This also covers the case of zero-width data
  gen0: if mdata_info_width > 1 generate
    gen1: for i in 0 to n_bytes-1 generate
      ocp_data(i*byte_width + 7 downto i*byte_width) <= MData(i*8+7 downto i*8);
      ocp_data(i*byte_width + byte_width-1 downto i*byte_width + byte_width - (byte_width - 8)) <=
        MDataInfo(i*(byte_width-8) + (byte_width-8)-1 downto i*(byte_width-8));
    end generate gen1;
  end generate gen0;
  -- If there are no partial bytes in datainfo, the worker's data is just MData.
  gen2: if mdata_info_width <= 1 and data_width > 0 generate
    ocp_data <= MData;
  end generate gen2;
  ocp_last   <= MDataLast when early_request else MReqLast;
  fifo_in    <= pack(ocp_data,
                     MByteEn,
                     ocp_last,
                     MDataInfo(MDataInfo'left), -- abort or eof
                     MBurstLength,              -- per OCP
                     MReqInfo);                 -- opcode which may be 1 bit wide even w/ no opcode
  fifo_enq   <= MDataValid when early_request else to_bool(MCmd = ocpi.ocp.MCmd_WRITE);
  ----------------------------------
  -- fifo outputs and dequeing etc.
  fifo_eof     <= fifo_out(abort_bit) and at_end_r;
  gen0w: if data_width /= 0 generate
    data       <= fifo_out(data_bits downto enable_bits+1);
  end generate gen0w;
  byte_enable  <= fifo_out(enable_bits downto last_bit+1);
  fifo_eom    <= fifo_out(last_bit);
  burst_length <= fifo_out(burst_bits downto opcode_bits+1);
  opcode       <= fifo_out(opcode_bits downto 0) when not zlm_eof else (others => '0');
  fifo_valid   <= to_bool(byte_enable /= slv0(n_bytes) and not its(fifo_eof));
  -- is a trailing EOM queued up *behind* the fifo's output which is not EOM (ignoring fifo status)
  trailing_eom <= to_bool(not its(fifo_eom) and fifo_middle_out_r(last_bit) = '1' and
                          fifo_middle_out_r(enable_bits downto last_bit+1) = slv0(n_bytes));
  -- deque/discard a trailing eom now?
  teom_deq     <= to_bool(fifo_count_r /= 0 and eom_added_r);
  fifo_deq     <= to_bool(not its(fifo_eof) and       -- EOFs are sticky
                          (teom_deq or                -- discard trailing EOMs after valid
                           (take and its(ready_i)))); -- dequeue from worker
  ----------------------------------
  -- signals based on state and fifo outputs
  -- fifo_ready holds non-EOM valid data from being taken until it knows what's next
  -- it also masks trailing EOMs coming out of the FIFO
  -- it answers the questino: are the fifo outputs currently something the worker should look at?
  fifo_ready   <= to_bool(not its(teom_deq) and
                          (fifo_count_r > 1 or
                           (fifo_count_r = 1 and ((fifo_eom and its(fifo_valid or at_end_r)) or zlm_eof))));
  zlm_eof      <= to_bool(fifo_eof and its(at_end_r) and hdl_version < 2);
  -- final filtering of "ready" for the user based on eof and isoperating
  ready_i      <= wci_is_operating and fifo_ready and (not fifo_eof or to_bool(hdl_version < 2));
  ----------------------------------
  -- Signals to worker
  ready        <= ready_i and not v1_eof_r; -- suppress ready after v1 eof
  som          <= at_end_r or zlm_eof;
  valid        <= ready_i and fifo_valid;  -- prequalified so it can be used directly by workers
  eom_i        <= fifo_eom or zlm_eof or trailing_eom;
  eom          <= eom_i;
  abort        <= fifo_eof and not at_end_r;
  eof          <= to_bool(fifo_count_r /= 0 and fifo_eof and
                          its(fifo_eom or at_end_r));
  -- Signals to OCP - cannot be based on fifo_deq
  -- SThreadBusy(0) <= fifo_busy;
  -- ocp is busy if it cannot enqueue in the next cycle
  SThreadBusy(0) <= to_bool((fifo_count_r = 3 and fifo_enq) or fifo_count_r = 4);
  --SThreadBusy(0) <= ocp_busy_r;
  -- True once
  first_take <= take and ready_i and not seen_any_r;
  -- keep track of message state as it comes out the back of the fifo
  reg: process(Clk) is
  begin
    if rising_edge(clk) then
      if its(reset_i) then
        seen_any_r     <= bfalse;
        at_end_r       <= btrue;
        v1_eof_r       <= bfalse;
        eom_added_r    <= bfalse;
      elsif its(fifo_eof) and ready_i and hdl_version >=2 then
        report "FIFOEOF and READY" severity failure;
      else
        if ready_i and take then
          eom_added_r <= trailing_eom;
          at_end_r    <= eom_i;
          seen_any_r  <= btrue;
          if its(zlm_eof) then
            v1_eof_r  <= btrue;
          end if;
        elsif its(fifo_deq) then
          eom_added_r <= bfalse;
        end if;
        -- ocp_busy_r    <= (to_bool(fifo_count_r = 3) and
        --                   not (fifo_deq and not fifo_enq and ocp_busy_r)) or
        --                  (to_bool(fifo_count_r = 2) and
        --                   not ((fifo_enq and fifo_deq and ocp_busy_r) or
        --                        (not (fifo_enq) and fifo_deq) or
        --                        (not fifo_enq and not fifo_deq and ocp_busy_r))) or
        --                  (to_bool(fifo_count_r = 1) and
        --                   (fifo_enq) and not (fifo_deq) and not (ocp_busy_r));
      end if;
    end if;
  end process;
  -- The 4 deep very specific FIFO that helps with routing, OCP pipelining and trailing EOM handling
  -- The input side cannot depend on what is happening on the output side
  -- We need a fifo that will tell us when there are two slots left.  It would be nice to find
  -- one and not use this one.
  fifo_out  <= fifo_out_r;
  fifo: process(Clk) is
  begin
    if rising_edge(clk) then
      if its(reset_i) then
        fifo_count_r <= (others => '0');
      else
        case fifo_count_r is
          when "000" =>
            assert not its(fifo_deq) severity failure;
            if its(fifo_enq) then
              fifo_out_r   <= fifo_in;
              fifo_count_r <= "001";
            end if;
          when "001" =>
            if its(fifo_enq) then
              if its(fifo_deq) then
                fifo_out_r    <= fifo_in;
              else
                fifo_middle_out_r <= fifo_in;
                fifo_count_r  <= "010";
              end if;
            elsif its(fifo_deq) then
              fifo_count_r    <= "000";
            end if;
          when "010" =>
            if its(fifo_enq) then
              if its(fifo_deq) then
                fifo_middle_out_r <= fifo_in;
                fifo_out_r        <= fifo_middle_out_r;
              else
                fifo_middle_in_r  <= fifo_in;
                fifo_count_r      <= "011";
              end if;
            elsif its(fifo_deq) then
              fifo_out_r      <= fifo_middle_out_r;
              fifo_count_r    <= "001";
            end if;
          when "011" =>
            if its(fifo_enq) then
              if its(fifo_deq) then
                fifo_middle_in_r  <= fifo_in;
                fifo_middle_out_r <= fifo_middle_in_r;
                fifo_out_r        <= fifo_middle_out_r;
              else
                fifo_in_r         <= fifo_in;
                fifo_count_r      <= "100";
              end if;
            elsif its(fifo_deq) then
              fifo_out_r        <= fifo_middle_out_r;
              fifo_middle_out_r <= fifo_middle_in_r;
              fifo_count_r      <= "010";
            end if;
          when "100" =>
            if its(fifo_enq) then
              assert its(fifo_deq) severity failure;
              fifo_in_r         <= fifo_in;
              fifo_middle_in_r  <= fifo_in_r;
              fifo_middle_out_r <= fifo_middle_in_r;
              fifo_out_r        <= fifo_middle_out_r;
            elsif its(fifo_deq) then
              fifo_out_r        <= fifo_middle_out_r;
              fifo_middle_out_r <= fifo_middle_in_r;
              fifo_middle_in_r  <= fifo_in_r;
              fifo_count_r      <= "011";
            end if;
          when others => null;
        end case;
      end if;
    end if;
  end process;
end architecture rtl;
