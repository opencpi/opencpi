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

-- The SDP Sender, to take data from a WSI port, and put it in memory, making it available
-- for someone to read it out and acknowledge that reading.

library IEEE, ocpi, util, bsv, sdp, cdc;
use IEEE.std_logic_1164.all, ieee.numeric_std.all;
use ocpi.types.all, ocpi.util.all, sdp.all, sdp.sdp.all;
architecture rtl of worker is
  -- Local worker constants
  constant sdp_width_c    : natural := to_integer(sdp_width);
  constant memory_depth_c : natural := to_integer(memory_bytes) / (sdp_width_c * 4);
  constant addr_width_c   : natural := width_for_max(memory_depth_c - 1);
  constant max_buffers_c  : natural := to_integer(max_buffers);
  constant addr_shift_c   : natural := width_for_max(sdp_width_c * 4 - 1);
  constant count_width_c  : natural := width_for_max(max_buffers_c);
  constant nbytes_width_c : natural := width_for_max(sdp_width_c * 4); -- nbytes in frame
  constant max_remotes_c  : natural := to_integer(max_remotes);
  constant max_seg_dws_c  : natural := 32;

  subtype remote_idx_t is unsigned(width_for_max(ocpi.util.max(1,max_remotes_c - 1)) - 1 downto 0);

  -- Flag/doorbell Sync FIFO definitions
  signal flag_enq       : bool_t;
  signal flag_in_remote : remote_idx_t;
  signal flag_not_full  : std_logic;
  signal flag_deq       : bool_t;
  signal flag_out_slv   : std_logic_vector(remote_idx_t'range);
  signal flag_out       : natural;
  signal flag_not_empty : std_logic;

  -- Metadata definitions for the SDP side
  constant sdp_meta_ndws_c : natural := 2;

  -- Convenience data types
  subtype buffer_count_t is unsigned(count_width_c - 1 downto 0);
  subtype bram_addr_t is unsigned(addr_width_c-1 downto 0);

  -- Dynamic state of a remote destination (static state is in properties)
  type remote_t is record
    index     : buffer_count_t; -- which remote buffer are we sending to?
    empty     : buffer_count_t; -- how many remote buffers are empty?
    data_addr : whole_addr_t;
    meta_addr : whole_addr_t;
    flag_addr : whole_addr_t;
  end record remote_t;
  type remote_array_t is array(0 to max_remotes_c-1) of remote_t;
  signal sdp_remotes : remote_array_t;

  -- Combinatorial signals
  signal can_take          : bool_t;
  signal will_write        : bool_t;
  signal max_offset        : bram_addr_t;
  signal next_buffer_addr  : bram_addr_t;
  signal bram_addr         : bram_addr_t;
  signal bram_addr_actual  : bram_addr_t;
  signal nbytes            : unsigned(nbytes_width_c-1 downto 0);
  -- Registered signals
  ---- WSI buffer filling pointers
  signal buffer_offset_r   : bram_addr_t;    -- offset in current buffer
  signal buffer_maxed_r    : bool_t;         -- last buffer offset addressed last word in buffer
  signal buffer_index_r    : buffer_count_t;
  signal buffer_addr_r     : bram_addr_t;    -- base of current buffer
  signal buffer_avail_r    : buffer_count_t; -- how local many buffers are empty?
  signal buffer_consumed   : bool_t;         -- pulse for buffer consumption from sdp side
  ---- Metadata management
  signal md_in             : metadata_t;
  signal md_out            : metadata_t;
  signal md_out_slv        : std_logic_vector(metawidth_c-1 downto 0);
  signal md_enq            : bool_t;
  signal md_deq            : std_logic;
  signal md_not_empty      : std_logic;
  signal md_not_full       : std_logic;
  -- ndws is rounded up so needs ONE fewer bits, not TWO
  signal md_out_ndws       : unsigned(meta_length_width_c-2 downto 0);
  signal eof_sent_r        : bool_t; -- input eof indication conveyed/enqueued

  -- SDP back side --
  signal sdp_reset              : std_logic;
  signal ctl2sdp_reset          : std_logic;
  signal bramb_addr             : bram_addr_t;
  signal bramb_addr_r           : bram_addr_t;
  signal bramb_out              : std_logic_vector(sdp_width_c*32-1 downto 0);
  signal sdp_out_r              : std_logic_vector(sdp_width_c*32-1 downto 0);
  signal sdp_out_valid_r        : bool_t;
  signal sdp_segment_count_r    : sdp.sdp.count_t;
  signal sdp_last_in_segment    : bool_t;
  -- State that changes for each msg
  signal sdp_msg_idx_r          : buffer_count_t;
  signal sdp_next_msg_addr      : bram_addr_t;
  signal sdp_msg_addr_r         : bram_addr_t;
  -- State that changes for each remote
  type sdp_phase_t is (idle_e, wait_e, data_e, meta_e, flag_e, between_remotes_e);
  signal sdp_remote_phase_r     : sdp_phase_t;
  subtype meta_dw_count_t is unsigned(meta_length_width_c-2 downto 0);
  signal sdp_msg_dws_left_r     : meta_dw_count_t;
  signal sdp_remote_idx_r       : remote_idx_t;
  signal sdp_last_remote        : remote_idx_t;
  -- State that changed for each segment
  signal sdp_segment_addr_r     : whole_addr_t;
  signal sdp_whole_addr         : whole_addr_t;
  signal sdp_segment_dws_left_r : meta_dw_count_t;
  -- sdp clock domain versions of control signals
  signal sdp_is_operating       : bool_t;

  ---- Global state
  signal buffer_size_fault_r : bool_t;
  signal doorbell_fault_r    : bool_t;
  signal truncation_fault_r  : bool_t;
  signal operating_r         : bool_t; -- were we operating in the last cycle?
  signal messageCount        : ulong_t;
  signal truncatedMessage_r  : ulong_t; -- first one
  signal truncatedData       : bool_t; -- first one
  signal ctl_reset_n         : std_logic;
  signal sdp_reset_n         : std_logic;
  signal taking              : bool_t;
  function be2bytes (be : std_logic_vector) return unsigned is
  begin
    for i in 0 to be'length-1 loop
      if be(i) = '0' then
        return to_unsigned(i, nbytes_width_c);
      end if;
    end loop;
    return to_unsigned(be'length, nbytes_width_c);
  end be2bytes;
begin
  ctl_reset_n        <= not ctl_in.reset;
  bram_addr          <= buffer_addr_r + buffer_offset_r;
  bram_addr_actual   <= bram_addr(addr_width_c-1 downto 0);
  next_buffer_addr   <= buffer_addr_r +
                        props_in.buffer_size(bram_addr_t'left + addr_shift_c
                                             downto addr_shift_c);
  can_take           <= to_bool(operating_r and its(not buffer_size_fault_r) and
                                its(not doorbell_fault_r) and md_not_full and
                                buffer_avail_r /= 0);
  will_write         <= can_take and in_in.ready and in_in.valid and not buffer_maxed_r;
  max_offset         <= props_in.buffer_size(bram_addr_t'left + 2 downto 2) - 1;
  -- Take even if bad write to send the truncation error in the metadata
  taking             <= can_take and in_in.ready;
  in_out.take        <= taking;
  in_out.clk         <= sdp_in.clk;
  ctl_out.finished   <= buffer_size_fault_r or doorbell_fault_r;
  props_out.faults(props_out.faults'length-1 downto 3) <= (others => '0');
  props_out.sdp_id   <= resize(sdp_in.id, props_out.sdp_id'length);
  nbytes             <= be2bytes(in_in.byte_enable) when its(in_in.valid) else (others => '0');
  md_in.length       <= (resize(buffer_offset_r, meta_length_width_c) sll addr_shift_c) + nbytes;
  md_in.eof          <= in_in.eof;
  md_in.truncate     <= in_in.valid and buffer_maxed_r;
  md_in.opcode       <= in_in.opcode;
  md_enq             <= to_bool(its(can_take) and ((in_in.ready and its(in_in.eom)) or
                                                   (in_in.eof and not its(eof_sent_r))));
  -- cross these bits from sdp clock domain to ctl clock domain
  -- these are levels that do not relate to each other so the simplest multi-bit cdc is ok here
  -- no input register is needed since these are already registered
  fault_bits : component cdc.cdc.bits
      generic map(
        width => 3)
      port map(
        src_clk           => sdp_in.clk,
        src_rst           => sdp_in.reset,
        src_in(0)         => truncation_fault_r,
        src_in(1)         => doorbell_fault_r,
        src_in(2)         => buffer_size_fault_r,
        dst_clk           => ctl_in.clk,
        dst_rst           => ctl_in.reset,
        unsigned(dst_out) => props_out.faults(2 downto 0));

  truncatedData <= taking and in_in.valid and buffer_maxed_r;
  -- cross from sdp clock domain to ctl clock domain
  -- increment for each cycle where the input is high
  trunc_data : component cdc.cdc.count_up
    generic map(width => props_out.truncatedData'length)
    port map   (src_clk           => sdp_in.clk,
                src_rst           => sdp_in.reset,
                src_in            => truncatedData, -- a count-up pulse
                dst_clk           => ctl_in.clk,
                dst_rst           => ctl_in.reset,
                dst_out           => props_out.truncatedData);
  -- This crosses clock domain (sdp->ctl), but it is an end-of-run debug thing so
  -- proper CDC is unnecessary
  props_out.truncatedMessage <= truncatedMessage_r;
  -- trunc_mesg : component cdc.cdc.bits
  --   generic map(width => props_out.truncatedMessage'length)
  --   port map   (src_clk           => sdp_in.clk,
  --               src_rst           => sdp_in.reset,
  --               src_in            => std_logic_vector(truncatedMessage_r),
  --               dst_clk           => ctl_in.clk,
  --               dst_rst           => ctl_in.reset,
  --               unsigned(dst_out) => props_out.truncatedMessage);

  -- Instance the message data BRAM
  -- Since the BRAM is single cycle, there is no handshake.
  -- note this has different clocks on both ports which we do not need
  bram : component util.util.BRAM2
    generic map(PIPELINED  => 0,
                ADDR_WIDTH => addr_width_c,
                DATA_WIDTH => sdp_width_c * 32,
                MEMSIZE    => memory_depth_c)
    port map   (CLKA       => sdp_in.clk,
                ENA        => '1',
                WEA        => will_write,
                ADDRA      => std_logic_vector(bram_addr_actual),
                DIA        => in_in.data,
                DOA        => open,
                CLKB       => sdp_in.clk,
                ENB        => '1',
                WEB        => '0',
                ADDRB      => std_logic_vector(bramb_addr),
                DIB        => slv0(sdp_width_c*32),
                DOB        => bramb_out);

  -- wsi to sdp, telling SDP to send next buffer with this metadata, same clock domain
  metafifo : component bsv.bsv.SizedFifo
   generic map(p1Width      => metawidth_c,
               p2depth      => roundup_2_power_of_2(max_buffers_c),
               p3cntr_width => width_for_max(roundup_2_power_of_2(max_buffers_c)-1))
   port map   (CLK     => sdp_in.clk,
               RST     => sdp_reset_n,
               ENQ     => std_logic(md_enq),
               D_IN    => meta2slv(md_in),
               FULL_N  => md_not_full,
               DEQ     => md_deq,
               D_OUT   => md_out_slv,
               EMPTY_N => md_not_empty,
               CLR     => '0');

  -- A sync fifo to carry doorbells to the SDP clock domain
  -- telling SDP that a destination buffer has become empty/available
  -- control clock domain to sdp clock domain
  flagfifo : component cdc.cdc.fifo
   generic map(width       => remote_idx_t'length,
               depth       => 2) -- must be power of 2
   port map   (src_CLK     => ctl_in.clk, -- maybe syncfifo later
               src_RST     => ctl_in.reset,
               dst_CLK     => sdp_in.clk,
               src_ENQ     => std_logic(flag_enq),
               src_IN      => std_logic_vector(flag_in_remote),
               src_FULL_N  => flag_not_full,
               dst_DEQ     => flag_deq,
               dst_OUT     => flag_out_slv,
               dst_EMPTY_N => flag_not_empty);

  -- A sync pulse to carry buffer consumption events
  -- The sdp telling WSI that a local buffer has become empty
--  cpulse: component bsv.bsv.SyncPulse
--    port map  (sCLK         => sdp_in.clk,
--               sRST         => sdp_reset_n,
--               dCLK         => ctl_in.clk,
--               sEN          => md_deq,
--               dPulse       => buffer_consumed);
  buffer_consumed <= md_deq;
  -- source side
  flag_enq       <= props_in.remote_doorbell_any_written;
--  flag_in_remote <= props_in.raw.address(flag_in_remote'length-1+2 downto 2); -- ulongs/dws
  flag_in_remote <= (others => '0'); -- props_in.remote_doorbell(0);
  -- destination side
  flag_out       <= to_integer(remote_idx_t(flag_out_slv));
  flag_deq       <= flag_not_empty; -- output of FIFO always processed immediately

  is_op : component cdc.cdc.bit
    port map   (src_clk           => ctl_in.clk,
                src_rst           => ctl_in.reset,
                src_in            => ctl_in.is_operating,
                src_en            => '1',
                dst_clk           => sdp_in.clk,
                dst_rst           => sdp_in.reset,
                dst_out           => sdp_is_operating);
  ctl2sdp_rst : component cdc.cdc.reset
    port map   (src_rst           => ctl_in.reset,
                dst_clk           => sdp_in.clk,
                dst_rst           => ctl2sdp_reset);
  sdp_reset <= sdp_in.reset or ctl2sdp_reset;
  -- the process going from wsi into the data bram and metadata fifo
  wsi2bram : process(sdp_in.clk)
  begin
    if rising_edge(sdp_in.clk) then
      if sdp_reset = '1' then
        buffer_offset_r     <= (others => '0');
        buffer_maxed_r      <= bfalse;
        buffer_index_r      <= (others => '0');
        buffer_addr_r       <= (others => '0');
        operating_r         <= bfalse;
        buffer_size_fault_r <= bfalse;
        doorbell_fault_r    <= bfalse;
        truncation_fault_r  <= bfalse;
        truncatedMessage_r  <= (others => '0');
        messageCount        <= (others => '0');
        eof_sent_r          <= bfalse;
      elsif not operating_r then
        -- initialization on first transition to operating.  poor man's "start".
        if its(sdp_is_operating) then
          operating_r   <= btrue;
          if props_in.buffer_size > memory_bytes then
            buffer_size_fault_r <= btrue;
          end if;
        end if;
        buffer_avail_r <= resize(props_in.buffer_count, buffer_avail_r'length);
      elsif not sdp_is_operating then
        operating_r <= bfalse;
      else
        if its(md_enq) then
          if its(in_in.eof) then
            eof_sent_r <= btrue;
          else
            messageCount <= messageCount + 1;
          end if;
        end if;
        if md_enq and not its(buffer_consumed) then
          buffer_avail_r <= buffer_avail_r - 1;
        end if;
        if buffer_consumed and not its(md_enq) then
          buffer_avail_r <= buffer_avail_r + 1;
        end if;
        if props_in.remote_doorbell_any_written and not flag_not_full = '1' then
          doorbell_fault_r <= '1'; -- this is really a debug thing.  Should not happen.
        end if;
        if its(taking) then
          if its(in_in.valid) then
            if its(buffer_maxed_r) then
              if not truncation_fault_r then
                truncatedMessage_r <= messageCount;
              end if;
              truncation_fault_r <= '1';
            elsif buffer_offset_r = max_offset and not its(in_in.eom) then
              buffer_maxed_r <= btrue;
            else
              buffer_offset_r <= buffer_offset_r + 1;
            end if;
          end if; -- valid
          if its(in_in.eom) then
            buffer_offset_r   <= (others => '0');
            buffer_maxed_r    <= bfalse;
            if buffer_index_r = props_in.buffer_count - 1 then
              buffer_index_r  <= (others => '0');
              buffer_addr_r   <= (others => '0');
            else
              buffer_index_r  <= buffer_index_r + 1;
              buffer_addr_r   <= next_buffer_addr;
            end if;
          end if; -- eom
        end if; -- cantake and ready
      end if; -- operating
    end if; -- rising edge
  end process;

  --------------------------------------------------------------------------------
  -- BRAM to SDP signals and process, in the clock domain from the SDP.
  sdp_reset_n         <= not sdp_in.reset;
  sdp_last_remote     <= resize(props_in.remote_count -1, remote_idx_t'length);
  md_out              <= slv2meta(md_out_slv);
  md_out_ndws         <= resize((md_out.length + dword_bytes - 1) srl 2, md_out_ndws'length);
  md_deq              <= operating_r and md_not_empty and sdp_in.sdp.ready and
                         to_bool(sdp_remote_phase_r = flag_e) and
                         to_bool(sdp_remote_idx_r = sdp_last_remote);
  sdp_last_in_segment <= to_bool(sdp_segment_dws_left_r = 0);
  -- What is the address of the next message in BRAM
  sdp_next_msg_addr   <= sdp_msg_addr_r  when sdp_remote_idx_r /= sdp_last_remote else
                         (others => '0') when sdp_msg_idx_r = props_in.buffer_count - 1 else
                         sdp_msg_addr_r + props_in.buffer_size(bram_addr_t'left + addr_shift_c
                                                               downto addr_shift_c);
  -- The BRAM address must be pipelined (early).
  bramb_addr          <= sdp_next_msg_addr when sdp_remote_phase_r /= data_e else
--                       sdp_msg_addr_r when sdp_last_in_segment and sdp_msg_dws_left_r = 0 else
                         bramb_addr_r;
  -- Drive SDP outputs
  sdp_out.sdp.header.op    <= sdp.sdp.write_e;          -- we are only writing
  sdp_out.sdp.header.xid   <= (others => '0');  -- since we are writing, no xid necessary
  sdp_out.sdp.header.lead  <= (others => '0');  -- we are always aligned on a DW
  sdp_out.sdp.header.trail <= (others => '0');  -- we always send whole DWs
  sdp_out.sdp.header.node  <= (others => '0');
  sdp_out.sdp.header.count <= sdp_segment_count_r;
  with sdp_remote_phase_r select sdp_whole_addr <=
    sdp_segment_addr_r                                  when data_e,
    sdp_remotes(to_integer(sdp_remote_idx_r)).meta_addr when meta_e,
    sdp_remotes(to_integer(sdp_remote_idx_r)).flag_addr when flag_e | idle_e | between_remotes_e | wait_e;

  sdp_out.sdp.header.addr    <= sdp_whole_addr(sdp.sdp.addr_width-1 downto 0);
  sdp_out.sdp.header.extaddr <= sdp_whole_addr(whole_addr_bits_c-1 downto sdp.sdp.addr_width);
  sdp_out.sdp.eop            <= sdp_last_in_segment;
  sdp_out.sdp.valid          <= to_bool(sdp_remote_phase_r /= idle_e and sdp_remote_phase_r /= wait_e);
  sdp_out.sdp.ready          <= bfalse;   -- we are write-only, never accepting an inbound frame
  sdp_out.dropCount          <= (others => '0');
g0: for i in 0 to sdp_width_c-1 generate
    sdp_out_data(i) <= sdp_out_r((i+1)*dword_size-1 downto i*dword_size)
                       when its(sdp_out_valid_r) else
                       bramb_out((i+1)*dword_size-1 downto i*dword_size);
  end generate g0;

  --props_out.rem_idx   <= resize(sdp_remote_idx_r, uchar_t'length);
  --props_out.rem_bidx  <= resize(sdp_remotes(0).index, uchar_t'length);
  --props_out.rem_phase <= to_unsigned(sdp_phase_t'pos(sdp_remote_phase_r),uchar_t'length);
  --props_out.rem_addr  <= resize(sdp_remotes(0).data_addr, ulonglong_t'length);
  --props_out.rem_seg   <= resize(sdp_segment_addr_r, ulonglong_t'length);
  -- The process that takes messages from BRAM and pushes them to the SDP,
  ---- trying to avoid any dead cycles except one at the start of the message
  ---- sending data(except for ZLM), metadata, flag transfers
  ---- breaking up the data messages into SDP-required segments
  bram2sdp : process(sdp_in.clk)
    variable r : natural;
    variable started_remote : boolean;
    impure function remote_is_ready(r : natural) return boolean is
    begin
      return sdp_remotes(r).empty /= 0 or (flag_not_empty = '1' and flag_out = r);
    end remote_is_ready;
    -- Start a segment, given how many DWs are left in the message,
    -- and set the left-in-segment, left-in-message, and segment header count
    procedure begin_segment(ndws_left : meta_dw_count_t) is
      variable ndws : unsigned(ndws_left'range);
    begin
      if ndws_left > max_seg_dws_c then
        ndws :=  to_unsigned(max_seg_dws_c, ndws'length);
      else
        ndws :=  ndws_left;
      end if;
      if ndws <= sdp_width then
        sdp_segment_dws_left_r <= (others => '0');
      else
        sdp_segment_dws_left_r <= ndws - sdp_width;
      end if;
      sdp_msg_dws_left_r     <= ndws_left - ndws; -- this does no harm in non-data phases
      sdp_segment_count_r    <= resize(ndws - 1, sdp_segment_count_r'length);
    end procedure begin_segment;
    -- Enter the flag phase
    procedure begin_flag(flag : dword_t) is
    begin
      sdp_segment_count_r   <= (others => '0');
      sdp_remote_phase_r    <= flag_e;
      sdp_out_r(flag'range) <= flag;
      -- Advance the buffer pointer here (not in flag_e) to pipeline the bram address
      if r = sdp_last_remote then
        if sdp_msg_idx_r = props_in.buffer_count - 1 then
          sdp_msg_idx_r    <= (others => '0');
        else
          sdp_msg_idx_r    <= sdp_msg_idx_r + 1;
        end if;
      end if;
    end procedure begin_flag;
    procedure begin_meta is
      variable meta :std_logic_vector(55 downto 0)
        := slv(slv(md_out.truncate),8) & slv(slv(md_out.eof),8) & md_out.opcode &
        std_logic_vector(resize(md_out.length, 32));
      variable metaflag : metadata_t;
    begin
      if props_in.readsAllowed(0) = '1' then
        begin_flag(meta2slv(md_out));
      else
        sdp_out_r <= std_logic_vector(meta(sdp_out_r'range));
        sdp_remote_phase_r <= meta_e;
        begin_segment(to_unsigned(sdp_meta_ndws_c, meta_dw_count_t'length));
      end if;
      sdp_out_valid_r    <= btrue;
    end procedure begin_meta;
    -- initialization for sending the current message to the indicated remote
    procedure begin_remote is
      variable data_addr : whole_addr_t;
      variable meta_addr : whole_addr_t;
      variable flag_addr : whole_addr_t;
    begin
      started_remote := true;
      sdp_remote_idx_r   <= to_unsigned(r, sdp_remote_idx_r'length);
      if sdp_remotes(r).index = 0 then
        data_addr := props_in.remote_data_addr(r)(data_addr'left+2 downto 2);
        meta_addr := props_in.remote_meta_addr(r)(meta_addr'left+2 downto 2);
        flag_addr := props_in.remote_flag_addr(r)(flag_addr'left+2 downto 2);
      else
        data_addr := sdp_remotes(r).data_addr +
                     resize(props_in.remote_data_pitch(r)(ulong_t'left downto 2),
                            whole_addr_t'length);
        meta_addr := sdp_remotes(r).meta_addr +
                     resize(props_in.remote_meta_pitch(r)(ulong_t'left downto 2),
                            whole_addr_t'length);
        flag_addr := sdp_remotes(r).flag_addr +
                     resize(props_in.remote_flag_pitch(r)(ulong_t'left downto 2),
                            whole_addr_t'length);
      end if;
      sdp_remotes(r).data_addr <= data_addr;
      sdp_remotes(r).meta_addr <= meta_addr;
      sdp_remotes(r).flag_addr <= flag_addr;
      if sdp_remotes(r).index = props_in.remote_buffer_count(r) - 1 then
        sdp_remotes(r).index <= (others => '0');
      else
        sdp_remotes(r).index <= sdp_remotes(r).index + 1;
      end if;
      if md_out_ndws = 0 then
        begin_meta;
        sdp_segment_addr_r <= meta_addr;
      else
        sdp_remote_phase_r <= data_e;
        sdp_segment_addr_r <= data_addr;
        sdp_out_r          <= bramb_out;
        sdp_out_valid_r    <= btrue;
        begin_segment(md_out_ndws);
      end if;
    end procedure begin_remote;
  begin
    if rising_edge(sdp_in.clk) then
      r              := to_integer(sdp_remote_idx_r);
      started_remote := false;
      -- FIXME deal properly with the potential of a separate CDP clk
      -- THe control clock should reset this state, but it might not be the same as the SDP clk.
      if its(ctl_in.reset) then
        -- Reset state for remotes
        for rr in 0 to max_remotes_c - 1 loop
          sdp_remotes(rr).index <= (others => '0');
        end loop;
        sdp_remote_phase_r <= idle_e;
        sdp_remote_idx_r   <= (others => '0');
        sdp_msg_addr_r     <= (others => '0');
        bramb_addr_r       <= (others => '0');
        sdp_out_valid_r    <= bfalse;
      elsif not operating_r then
        -- reset state that depends on properties
        for r in 0 to max_remotes_c - 1 loop
          sdp_remotes(r).empty <= resize(props_in.remote_buffer_count(r),buffer_count_t'length);
        end loop;
        sdp_msg_idx_r <= resize(props_in.buffer_count - 1, sdp_msg_idx_r'length);
      else
        if md_not_empty = '1' then
          if sdp_remote_phase_r = idle_e and md_out.length <= sdp_width_c*4 then
            sdp_remote_phase_r <= wait_e; -- give a cycle for initial data to get through the BRAM
          elsif sdp_remote_phase_r = idle_e or sdp_remote_phase_r = wait_e then
            if remote_is_ready(0) then
              -- We are starting to send a message to all remotes.  bram addr was already valid
              sdp_msg_addr_r <= sdp_next_msg_addr;
              bramb_addr_r   <= sdp_next_msg_addr + 1;
              begin_remote;
            end if;
          elsif sdp_remote_phase_r = between_remotes_e then
            if remote_is_ready(r) then
              begin_remote;
            end if;
          elsif its(sdp_in.sdp.ready) then
            if sdp_segment_dws_left_r /= 0 then
              -- continue the segment
              case sdp_remote_phase_r is
                when data_e =>
                  bramb_addr_r    <= bramb_addr + 1;
                  sdp_out_valid_r <= bfalse;
                when meta_e => -- must be single word width
                  sdp_out_r <= "00000000" & slv(slv(md_out.truncate), 8) &
                               slv(slv(md_out.eof), 8) & md_out.opcode;
                  sdp_out_valid_r <= btrue;
                when others => null;
              end case;
              sdp_segment_dws_left_r <= sdp_segment_dws_left_r - sdp_width_c;
            else
              -- end the segment
              case sdp_remote_phase_r is
                when data_e =>
                  if sdp_msg_dws_left_r /= 0 then
                    sdp_segment_addr_r <= sdp_segment_addr_r + max_seg_dws_c;
                    bramb_addr_r       <= bramb_addr + 1;
                    sdp_out_valid_r    <= bfalse;
                    begin_segment(sdp_msg_dws_left_r);
                  else
                    begin_meta;
                  end if;
                when meta_e =>
                  begin_flag(slv1(dword_t'length));
                when flag_e =>
                  -- end of transferring a message to a remote
                  if r = sdp_last_remote then
                    sdp_remote_phase_r <= idle_e; -- our one idle cycle per message
                    sdp_remote_idx_r   <= (others => '0');
                  else
                    -- move on to the next remote with the same message
                    r := r + 1;
                    bramb_addr_r <= sdp_msg_addr_r + 1;
                    if remote_is_ready(r) then
                      begin_remote;
                    else
                      sdp_remote_idx_r   <= to_unsigned(r, sdp_remote_idx_r'length);
                      sdp_remote_phase_r <= between_remotes_e;
                    end if;
                  end if;
                when others => null;
              end case;
            end if; -- if/else end of segment
          elsif sdp_remote_phase_r = data_e then -- we're reading from pipelined memory
            if not sdp_out_valid_r then
              sdp_out_valid_r <= btrue;
              sdp_out_r       <= bramb_out;
--              bramb_addr_r <= bramb_addr_r + 1;
            end if;
          end if; -- if/else idle
        end if; -- message available in fifo 
        -- Process doorbells
        if flag_not_empty = '1' and not (started_remote and flag_out = r) then
          sdp_remotes(flag_out).empty <= sdp_remotes(flag_out).empty + 1;
        end if;
        if started_remote and not (flag_not_empty = '1' and flag_out = r) then
          sdp_remotes(r).empty <= sdp_remotes(r).empty - 1;
        end if;
      end if; -- if/else reset/operating
    end if; -- if rising edge
  end process;
end architecture rtl;
