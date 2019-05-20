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
    data             : out  std_logic_vector(n_bytes*byte_width-1 downto 0);
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
  signal reset_i : Bool_t; -- internal version of output to worker
  signal reset_n : Bool_t; -- internal assert-low reset (to avoid silly isim warning).

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
  function pack(data   : std_logic_vector(data_width - 1 downto 0);
                enable : std_logic_vector(n_bytes - 1 downto 0);
                last   : std_logic;
                abort  : std_logic;
                burst  : std_logic_vector(burst_width - 1 downto 0);
                opcode : std_logic_vector(max(1,opcode_width)-1 downto 0))
    return std_logic_vector is
  begin
    return data & enable & last & abort & burst & opcode;
  end pack;

  -- where are we in a message, between them, at the end, or inside the message
  signal seen_any_r : bool_t; -- have we seen any messages at all?
  signal at_end_r   : bool_t; -- was the last thing taken an eom, eof, abort?
  -- output signals from the fifo
  signal fifo_full_n : std_logic;

  -- actual state of the interface - 2 FFs
  signal fifo_in, fifo_out : std_logic_vector(fifo_width - 1 downto 0);
  signal fifo_enq, fifo_last, fifo_deq : std_logic;
  signal fifo_ready : bool_t;
  signal ready_i : bool_t; -- internal ready, prequalified by wci_is_operating
  signal my_data : std_logic_vector(data_width - 1 downto 0);
  signal last, maybe_som : Bool_t; -- internal metadata
  signal fifo_eof, zlm_eof : Bool_t;
begin
  -- Combi resets:
  --   We get wci reset and wsi peer reset (from master)
  --   We produce wsi peer reset (from slave)
  -- Worker sees reset if wci is doing it or we're not started yet or peer is reset
  reset_i  <= wci_reset or not MReset_n; -- FIXME WHEN own_clock
  reset_n  <= not reset_i;
  reset    <= reset_i; -- in wci clock domain for now
  -- Pear sees reset if wci is doing it or we're not started
  SReset_n <= not wci_reset; -- FIXME WHEN OWN CLOCK
  -- If there are parts of bytes in data_info_width, combine them nicely for the worker
  -- This also covers the case of zero-width data
  gen0: if mdata_info_width > 1 generate
    gen1: for i in 0 to n_bytes-1 generate
      my_data(i*byte_width + 7 downto i*byte_width) <= MData(i*8+7 downto i*8);
      my_data(i*byte_width + byte_width-1 downto i*byte_width + byte_width - (byte_width - 8)) <=
        MDataInfo(i*(byte_width-8) + (byte_width-8)-1 downto i*(byte_width-8));
    end generate gen1;
  end generate gen0;
  -- If there are no partial bytes in datainfo, the worker's data is just MData.
  gen2: if mdata_info_width <= 1 generate
    my_data <= MData;
  end generate gen2;
  -- fifo inputs - keep decoding to a minimum on the input side since that is perhaps a synthesis/routing
  -- boundary
  fifo_deq   <= take and ready_i;
  fifo_enq   <= MDataValid when early_request else to_bool(MCmd = ocpi.ocp.MCmd_WRITE);
  fifo_last  <= MDataLast when early_request else MReqLast;
  fifo_in    <= pack(my_data,
                     MByteEn,
                     fifo_last,
                     MDataInfo(MDataInfo'left), -- abort or eof
                     MBurstLength,              -- per OCP
                     MReqInfo);                 -- opcode which may be 1 bit wide even when there no opcode bits
  fifo_eof     <= fifo_out(abort_bit);
  zlm_eof      <= to_bool(fifo_eof and its(maybe_som) and hdl_version < 2);
  -- unpack fifo output and drive signals to worker
  data         <= fifo_out(data_bits downto enable_bits+1);
  byte_enable  <= fifo_out(enable_bits downto last_bit+1);
  last         <= fifo_out(last_bit);
  abort        <= fifo_out(abort_bit) and not maybe_som;
  burst_length <= fifo_out(burst_bits downto opcode_bits+1);
  opcode       <= fifo_out(opcode_bits downto 0);
  maybe_som    <= at_end_r;
  ready_i      <= wci_is_operating and fifo_ready and (not fifo_eof or to_bool(hdl_version < 2));
  --ready_i      <= wci_is_operating and fifo_ready and (not its(fifo_eof) or hdl_version < 2);
  ready        <= ready_i;
  -- These are version sensitive since eof can't turn them off
  som          <= maybe_som or zlm_eof;
  -- Valid is prequalified so it can be used directly by workers
  valid        <= to_bool(ready_i and byte_enable /= slv0(n_bytes) and not its(fifo_eof));
  eom          <= last or zlm_eof;
  abort        <= fifo_eof and last and not maybe_som;
  eof          <= fifo_eof and last and maybe_som;

  -- Instantiate and connect the FIFO
  fifo : FIFO2X
    generic map(width                       => fifo_width)
    port    map(clk                         => Clk,
                rst                         => reset_n,
                d_in                        => fifo_in,
                enq                         => fifo_enq,
                full_n                      => fifo_full_n,
                d_out                       => fifo_out,
                deq                         => fifo_deq,
                empty_n                     => fifo_ready,
                clr                         => '0');
  -- FIXME WHEN OWN CLOCK
  SThreadBusy(0) <= reset_i or not wci_is_operating or not fifo_full_n;
  -- True once
  first_take <= take and ready_i and not seen_any_r;
  -- keep track of message state as it comes out the back of the fifo
  reg: process(Clk) is
  begin
    if rising_edge(clk) then
      if its(reset_i) then
        seen_any_r <= bfalse;
        at_end_r   <= btrue;
      elsif its(fifo_eof) and ready_i and hdl_version >=2 then
        report "FIFOEOF and READY" severity failure;
      elsif ready_i and take then
        at_end_r <= last;
        seen_any_r <= btrue;
      end if;
    end if;
  end process;
end architecture rtl;

