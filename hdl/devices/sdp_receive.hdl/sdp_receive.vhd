-- The SDP receiver, to take data from the SDP port, and put it out on a WSI port,
-- buffering messages in BRAM.
-- This should/will be split into two smaller modules...

library IEEE, ocpi, util, bsv, sdp;
use IEEE.std_logic_1164.all, ieee.numeric_std.all;
use ocpi.types.all, ocpi.all, ocpi.util.all, sdp.sdp.all;
architecture rtl of sdp_receive_worker is
  -- Local worker constants
  constant sdp_width_c     : natural := to_integer(sdp_width);
  constant memory_depth_c  : natural := to_integer(memory_bytes) / (sdp_width_c * 4);
  constant addr_width_c    : natural := width_for_max(memory_depth_c - 1);
  constant max_buffers_c   : natural := to_integer(max_buffers);
  constant addr_shift_c    : natural := width_for_max(sdp_width_c * 4 - 1);
  constant dw_addr_shift_c : natural := width_for_max(sdp_width_c-1);
  constant count_width_c   : natural := width_for_max(max_buffers_c);
  constant nbytes_width_c  : natural := width_for_max(sdp_width_c * 4); -- nbytes in frame
  constant max_remotes_c   : natural := to_integer(max_remotes);
  constant sdp_flag_addr_c : natural := (max_addressable_kbytes * 1024 - 4*1024) / 4;
  constant sdp_meta_addr_c : natural := (max_addressable_kbytes * 1024 - 8*1024) / 4;

  --------------------------------------------------------------------------------
  -- Signals and definitions for communication between the WSI side and the SDP side
  --------------------------------------------------------------------------------

  -- Metadata (internal) FIFO definitions
  constant meta_length_c  : natural := 0;
  constant meta_length_width_c : natural := 23;
  constant meta_eof_c     : natural := meta_length_c + meta_length_width_c;
  constant meta_opcode_c  : natural := meta_eof_c + 1;
  constant meta_opcode_width_c  : natural := 8;
  type metadata_t is record
    length : unsigned(meta_length_width_c-1 downto 0); -- bytes
    eof    : bool_t;
    opcode : std_logic_vector(meta_opcode_width_c-1 downto 0);
  end record metadata_t;
  constant metawidth_c : integer := meta_length_width_c + 1 + meta_opcode_width_c;
  constant meta_ndws_c : integer := (metawidth_c + dword_size - 1) / dword_size;
  function meta2slv(meta : metadata_t) return std_logic_vector is
  begin
    return meta.opcode & slv(meta.eof) & slv(meta.length);
  end meta2slv;
  function slv2meta(s : std_logic_vector(metawidth_c-1 downto 0)) return metadata_t is
    variable m : metadata_t;
  begin
    m.length := unsigned(s(meta_length_width_c-1 downto 0));
    m.eof    := s(meta_length_width_c);
    m.opcode := s(s'left downto meta_eof_c + 1);
    return m;
  end slv2meta;
  signal md_in             : metadata_t;
  signal md_out            : metadata_t;
  signal md_out_slv        : std_logic_vector(metawidth_c-1 downto 0);
  signal md_enq            : bool_t;
  signal md_deq            : std_logic;
  signal md_not_empty      : std_logic;
  signal md_not_full       : std_logic;
  subtype meta_dw_count_t is unsigned(meta_length_width_c-2 downto 0);
  -- ndws is rounded up so needs ONE fewer bits, not TWO
  signal md_out_ndws       : meta_dw_count_t;

  -- Convenience data types
  subtype buffer_count_t is unsigned(count_width_c - 1 downto 0);
  subtype bram_addr_t is unsigned(addr_width_c-1 downto 0);

  --------------------------------------------------------------------------------
  -- Signals and definitions for the WSI side
  --------------------------------------------------------------------------------
  -- Combinatorial signals
  signal ctl_reset_n          : std_logic;
  signal will_give            : bool_t;
  signal last_give            : bool_t;     -- last give in message
  signal wsi_next_buffer_addr : bram_addr_t;
  signal brama_addr           : bram_addr_t;
  -- state
  signal brama_addr_r         : bram_addr_t;
  signal overflow_r           : bool_t; -- tried to write past the end of the buffer
  signal operating_r          : bool_t; -- were we operating in the last cycle?
  signal wsi_starting_r       : bool_t;
--  signal wsi_buffer_offset_r  : bram_addr_t;    -- offset in current buffer
  signal wsi_buffer_index_r   : buffer_count_t;
  signal wsi_buffer_addr_r    : bram_addr_t;    -- base of current buffer
  signal wsi_dws_left         : meta_dw_count_t;
  signal wsi_dws_left_r       : meta_dw_count_t;

  --------------------------------------------------------------------------------
  -- Signals and definitions for the SDP side
  --------------------------------------------------------------------------------

  -- Definitions for remote accesses
  constant sdp_whole_addr_bits_c : integer := addr_width + node_width;
  subtype sdp_whole_addr_t is unsigned(sdp_whole_addr_bits_c-1 downto 0);

  -- Metadata definitions for the SDP side
  constant sdp_meta_ndws_c : natural := 2;

  signal max_offset         : bram_addr_t;
  signal bad_write          : bool_t;
  signal nbytes             : unsigned(nbytes_width_c-1 downto 0);
  signal buffer_consumed    : bool_t;         -- pulse for buffer consumption from wsi side
  signal buffers_consumed_r : buffer_count_t;
  -- For arithmetic about the dws of a segment, including the total number (hdr field+1)
  -- Currently the SDP protocol header allows large (16KB/4KDW) segments
  subtype sdp_seg_dw_t is unsigned(count_width downto 0);
  -- For arithmetic about the dws in a transfer, including sdp_width.
  subtype sdp_xfr_dw_t is unsigned(width_for_max(sdp_width_c)-1 downto 0);
  constant xfer_width        : sdp_xfr_dw_t := to_unsigned(sdp_width_c, sdp_xfr_dw_t'length);
  signal flag_write          : bool_t;
  signal meta_write          : bool_t;
  signal flag_out            : bool_t;
  signal can_take            : bool_t;
  signal first_dw            : sdp_xfr_dw_t;
  signal dws_in_first_xfer   : sdp_xfr_dw_t;
  signal dws_in_xfer         : sdp_xfr_dw_t;
  signal dws_in_segment      : sdp_seg_dw_t;
  signal bramb_addr          : bram_addr_t;
  signal bramb_addr_r        : bram_addr_t;
  signal bramb_in            : dword_array_t(0 to sdp_width_c-1);
  signal bramb_write         : bool_array_t(0 to sdp_width_c-1);
  signal sdp_md_r            : metadata_t;
  signal sdp_out_r           : std_logic_vector(sdp_width_c*32-1 downto 0);
  signal sdp_out_valid_r     : bool_t;
  signal sdp_addr            : addr_t;
  signal sdp_addr_r          : addr_t;
  signal sdp_dws_left        : sdp_seg_dw_t;
  signal sdp_dws_left_r      : sdp_seg_dw_t;
  signal sdp_starting_r      : bool_t;
  signal sdp_buffer_idx_r    : buffer_count_t;
  signal sdp_buffer_offset_r : bram_addr_t;    -- offset in current buffer
  signal sdp_buffers_avail_r : buffer_count_t;
  signal sdp_whole_addr_r    : sdp_whole_addr_t;

  ---- Global state
  signal sdp_reset_n       : std_logic;
  function be2bytes (be    : std_logic_vector) return unsigned is
  begin
    for i in 0 to be'length-1 loop
      if be(i) = '0' then
        return to_unsigned(i, nbytes_width_c);
      end if;
    end loop;
    return to_unsigned(be'length, nbytes_width_c);
  end be2bytes;
begin
  --------------------------------------------------------------------------------
  -- modules instantiated for synchronization between the SDP and the WSI
  --------------------------------------------------------------------------------

  -- BRAM A side is for WSI (reading), B side is for SDP (writing)
  -- We use multiple BRAMS - one for each DW of width.
  -- This supports the SDP writing arbitrary DWs.
g0: for i in 0 to sdp_width_c-1 generate
  bram : component util.util.BRAM2
    generic map(PIPELINED  => 0,
                ADDR_WIDTH => addr_width_c,
                DATA_WIDTH => dword_size,
                MEMSIZE    => memory_depth_c)
    port map   (CLKA       => ctl_in.clk,
                ENA        => '1',
                WEA        => '0',
                ADDRA      => std_logic_vector(brama_addr),
                DIA        => slv0(dword_size),
                DOA        => out_out.data(i*dword_size+dword_size-1 downto i*dword_size),
                CLKB       => sdp_in.clk,
                ENB        => '1',
                WEB        => std_logic(bramb_write(i)),
                ADDRB      => std_logic_vector(bramb_addr),
                DIB        => bramb_in(i),
                DOB        => open);
    bramb_in(i) <= sdp_in_data(i);
  end generate g0;
  -- Metadata fifo enqueued from SDP, dequeued to WSI
  metafifo : component bsv.bsv.SyncFIFO
   generic map(dataWidth    => metawidth_c,
               depth        => roundup_2_power_of_2(max_buffers_c), -- must be power of 2
               indxWidth    => width_for_max(roundup_2_power_of_2(max_buffers_c)-1))
   port map   (sCLK         => sdp_in.clk, -- maybe syncfifo later
               sRST         => sdp_reset_n,
               dCLK         => ctl_in.clk,
               sENQ         => std_logic(md_enq),
               sD_IN        => meta2slv(md_in),
               sFULL_N      => md_not_full,
               dDEQ         => md_deq,
               dD_OUT       => md_out_slv,
               dEMPTY_N     => md_not_empty);
  md_out      <= slv2meta(md_out_slv);
  md_out_ndws <= resize((md_out.length + dword_bytes - 1) srl 2, md_out_ndws'length);

  -- A sync fifo to carry doorbells to the SDP clock domain
  -- Someday perhaps the doorbells will come in via the SDP..
  -- or be eliminated altogether
  --flagfifo : component bsv.bsv.SyncFIFO
  -- generic map(dataWidth    => remote_idx_t'length,
  --             depth        => 2, -- must be power of 2
  --             indxWidth    => width_for_max(2-1))
  -- port map   (sCLK         => ctl_in.clk, -- maybe syncfifo later
  --             sRST         => ctl_reset_n,
  --             dCLK         => sdp_in.clk,
  --             sENQ         => std_logic(flag_enq),
  --             sD_IN        => std_logic_vector(flag_in_remote),
  --             sFULL_N      => flag_not_full,
  --             dDEQ         => flag_deq,
  --             dD_OUT       => flag_out_slv,
  --             dEMPTY_N     => flag_not_empty);

  -- A sync pulse to carry buffer consumption events
  -- The WSI side is telling the SDP side that it can reuse a buffer
  -- and also tell the other side that they can write into the next buffer.
  cpulse: component bsv.bsv.SyncPulse
    port map  (sCLK         => ctl_in.clk,
               sRST         => ctl_reset_n,
               dCLK         => ctl_in.clk,
               sEN          => md_deq,
               dPulse       => buffer_consumed);

  --------------------------------------------------------------------------------
  -- Combinatorial signals on the WSI side
  --------------------------------------------------------------------------------
  ctl_reset_n          <= not ctl_in.reset;
  wsi_next_buffer_addr <= (others => '0')
                          when wsi_buffer_index_r = props_in.buffer_count - 1 else
                          wsi_buffer_addr_r +
                          props_in.buffer_size(bram_addr_t'left + addr_shift_c
                                               downto addr_shift_c);
  brama_addr           <= wsi_next_buffer_addr when its(last_give)
                          else brama_addr_r + 1 when its(will_give) 
                          else brama_addr_r;
  will_give            <= to_bool(operating_r and not its(overflow_r) and md_not_empty and
                                  out_in.ready);
  last_give            <= to_bool(wsi_dws_left = 0);
  md_deq               <= will_give and last_give;
  wsi_dws_left         <= md_out_ndws - ocpi.util.min(md_out_ndws,
                                                      resize(sdp_width, meta_dw_count_t'length))
                          when its(wsi_starting_r) else wsi_dws_left_r;
  --------------------------------------------------------------------------------
  -- Module output ports on the WSI side
  --------------------------------------------------------------------------------
  ctl_out.finished    <= overflow_r;
  props_out.overflow  <= overflow_r;
  props_out.sdp_id    <= resize(sdp_in.id, props_out.sdp_id'length);
  out_out.give        <= will_give;
  out_out.som         <= wsi_starting_r and not md_out.eof;
  out_out.eom         <= last_give and not md_out.eof;
  out_out.valid       <= to_bool(md_not_empty and md_out.length /= 0);
  out_out.opcode      <= md_out.opcode;
  out_out.byte_enable <= (others => '0') when md_out.length = 0 else -- zlm
                         (others => '1') when not its(last_give) or -- full xfer
                                              md_out.length(addr_shift_c-1 downto 0) = 0 else
                         slv(not (unsigned(slv1(sdp_width_c*dword_bytes)) sll
                                  to_integer(md_out.length(addr_shift_c-1 downto 0))));
--    not synthesizable (xst at least)
--                         (sdp_width_c*dword_bytes-1 downto
--                          to_integer(md_out.length(addr_shift_c-1 downto 0)) => '0',
--                          others => '1');
  --------------------------------------------------------------------------------
  -- The process of reading messages from the metadata FIFO and BRAM and sending
  -- then to the WSI port named "out"
  --------------------------------------------------------------------------------
  bram2wsi : process(ctl_in.clk)
  begin
    if rising_edge(ctl_in.clk) then
      if ctl_in.reset = '1' then
--        wsi_buffer_offset_r <= (others => '0');
        brama_addr_r        <= (others => '0');
        wsi_buffer_index_r  <= (others => '0');
        wsi_buffer_addr_r   <= (others => '0');
        operating_r     <= bfalse;
        overflow_r      <= bfalse;
        wsi_starting_r  <= btrue;
      elsif not operating_r then
        -- initialization on first transition to operating.  poor man's "start".
        if its(ctl_in.is_operating) then
          operating_r   <= btrue;
          if props_in.buffer_size > memory_bytes or
             props_in.buffer_size(addr_shift_c-1 downto 0) /= 0 then
            overflow_r <= btrue;
          end if;
        end if;
      elsif not ctl_in.is_operating then
        operating_r <= bfalse;
      else
        if its(will_give) then
          brama_addr_r <= brama_addr;
          if its(last_give) then
            wsi_starting_r       <= btrue;
--            wsi_buffer_offset_r  <= (others => '0');
            wsi_buffer_addr_r    <= wsi_next_buffer_addr;
            if md_out_ndws >= sdp_width_c then
              wsi_dws_left_r     <= md_out_ndws - sdp_width_c;
            else
              wsi_dws_left_r     <= md_out_ndws;
            end if;
            if wsi_buffer_index_r = props_in.buffer_count - 1 then
              wsi_buffer_index_r <= (others => '0');
            else
              wsi_buffer_index_r <= wsi_buffer_index_r + 1;
            end if;
          else
            wsi_dws_left_r       <= wsi_dws_left - sdp_width_c;
--            wsi_buffer_offset_r  <= wsi_buffer_offset_r + 1;
            wsi_starting_r       <= bfalse;
          end if;
        end if; -- will_give
      end if; -- operating
    end if; -- rising edge
  end process;

  --------------------------------------------------------------------------------
  -- Combinatorial signals on the SDP side
  --------------------------------------------------------------------------------
  sdp_reset_n       <= not sdp_in.reset;
  max_offset        <= props_in.buffer_size(bram_addr_t'left + addr_shift_c
                                              downto addr_shift_c) - 1;
  bad_write         <= to_bool(sdp_buffer_offset_r > max_offset or
                               bramb_addr >= memory_depth_c - 1);
  dws_in_segment         <= count_in_dws(sdp_in.sdp.header);
  first_dw               <= sdp_in.sdp.header.addr(sdp_xfr_dw_t'range) when its(sdp_starting_r)
                            else (others => '0');
  dws_in_first_xfer      <= resize(ocpi.util.min(xfer_width -
                                                 sdp_in.sdp.header.addr(sdp_xfr_dw_t'range),
                                                 dws_in_segment), sdp_xfr_dw_t'length);
  dws_in_xfer            <= dws_in_first_xfer when its(sdp_starting_r)
                            else resize(ocpi.util.min(sdp_dws_left_r, sdp_width),
                                        sdp_xfr_dw_t'length);
  sdp_addr               <= sdp_in.sdp.header.addr when its(sdp_starting_r) else sdp_addr_r;
  sdp_dws_left           <= dws_in_segment - dws_in_first_xfer when its(sdp_starting_r)
                            else sdp_dws_left_r - dws_in_xfer;
g1: for i in 0 to sdp_width_c-1 generate
  bramb_write(i)    <= bfalse when i < first_dw or i > first_dw + dws_in_xfer
                       else sdp_in.sdp.valid and not (flag_write or meta_write);
  end generate g1;
  bramb_addr        <= sdp_addr(bramb_addr'left + dw_addr_shift_c downto dw_addr_shift_c);
  md_in             <= sdp_md_r;
  flag_write        <= to_bool(sdp_in.sdp.header.addr = sdp_flag_addr_c);
  md_enq            <= sdp_in.sdp.valid and flag_write;
  meta_write        <= to_bool(sdp_in.sdp.header.addr = sdp_meta_addr_c);
  flag_out          <= to_bool(buffer_consumed or buffers_consumed_r /= 0);
  can_take          <= to_bool(its(flag_write) or meta_write or sdp_buffers_avail_r /= 0);

  --------------------------------------------------------------------------------
  -- Module output ports on the SDP side
  --------------------------------------------------------------------------------
  -- For data arriving on sdp_in: 
  sdp_out.sdp.ready        <= to_bool(sdp_in.sdp.valid and its(can_take));
  -- For flag leaving on sdp_out:
  sdp_out.sdp.header.op    <= write_e;  -- we are only writing flags
  sdp_out.sdp.header.xid   <= (others => '0');  -- since we are writing, no xid necessary
  sdp_out.sdp.header.lead  <= (others => '0');  -- we are always aligned on a DW
  sdp_out.sdp.header.trail <= (others => '0');  -- we always send whole DWs
  sdp_out.sdp.header.count <= (others => '0');  -- we always send exactly one DW
  sdp_out.sdp.header.node  <= sdp_whole_addr_r(sdp_whole_addr_bits_c-1 downto addr_width);
  sdp_out.sdp.header.addr  <= sdp_whole_addr_r(addr_width-1 downto 0);
  sdp_out.sdp.eom          <= btrue;
  sdp_out.sdp.valid        <= flag_out;
  sdp_out_data(0)          <= slvn(1, dword_size);
g2: for i in 1 to sdp_width_c-1 generate
    sdp_out_data(i) <= (others => '0');
  end generate g2;

  -- The process that takes messages from SDP and:
  --  0. waits for buffer space to be available to avoid overwriting previous messages
  --  1. writes data to BRAM or 
  --  2. writes metadata to holding registers or
  --  3. enqueues FIFO with metadata to tell the other side
  sdp2bram : process(sdp_in.clk)
  begin
    if rising_edge(sdp_in.clk) then
      if its(sdp_in.reset) then
        sdp_addr_r            <= (others => '0'); -- SDP type DW address
        sdp_buffer_offset_r   <= (others => '0');
        buffers_consumed_r    <= (others => '0');
        sdp_buffer_idx_r      <= (others => '0');
        sdp_starting_r        <= btrue;
      elsif not operating_r then
        -- reset state that depends on properties
        sdp_buffers_avail_r   <= resize(props_in.buffer_count, sdp_buffers_avail_r'length);
        sdp_whole_addr_r      <= props_in.remote_flag_addr(0)(sdp_whole_addr_r'left+2 downto 2);
      else
        -- Deal with outbound flag processing
        if flag_out and sdp_in.sdp.ready then
          if not buffer_consumed then
            buffers_consumed_r <= buffers_consumed_r - 1;
          end if;
          if sdp_buffer_idx_r = resize(props_in.buffer_count - 1, sdp_buffer_idx_r'length) then
            sdp_buffer_idx_r <= (others => '0');
            sdp_whole_addr_r <= props_in.remote_flag_addr(0)(sdp_whole_addr_r'left+2 downto 2);
          else
            sdp_buffer_idx_r <= sdp_buffer_idx_r + 1;
            sdp_whole_addr_r <= sdp_whole_addr_r + props_in.remote_flag_pitch(0)(31 downto 2);
          end if;
        elsif its(buffer_consumed) then
          buffers_consumed_r <= buffers_consumed_r + 1;
        end if;
        -- Deal with consumption events/pulses from the WSI side.
        -- These affect two different activities:
        -- 1. Initiate/request writing a flag back to the producer (buffer now empty)
        -- 2. Add to local buffers_avail count to accept incoming writes.
        if its(buffer_consumed) then
          if not (its(flag_out) and sdp_in.sdp.ready) then     -- if not decrementing
            buffers_consumed_r <= buffers_consumed_r + 1; -- increase number of flags to be sent
          end if;
          if not (its(sdp_in.sdp.valid) and can_take and flag_write) then
            sdp_buffers_avail_r <= sdp_buffers_avail_r + 1;
          end if;
        end if;
        -- Deal with incoming writes.
        if sdp_in.sdp.valid and can_take then
          sdp_starting_r <= to_bool(sdp_dws_left = 0);
          if its(sdp_starting_r) then
            sdp_dws_left_r <= sdp_dws_left;
          else
            sdp_dws_left_r <= sdp_dws_left_r - dws_in_xfer;
          end if;
          if its(flag_write) then
            -- A flag write that will enqueue the metadata
            if not buffer_consumed then
              sdp_buffers_avail_r <= sdp_buffers_avail_r - 1;
            end if;
          elsif its(meta_write) then -- we assume a 32 bit or 64 bit transfer, aligned
            if its(sdp_starting_r) then
              sdp_md_r.length <= resize(unsigned(sdp_in_data(0)), sdp_md_r.length'length);
              if sdp_width_c > 1 then
                sdp_md_r.opcode <= sdp_in_data(1)(7 downto 0);
                sdp_md_r.eof    <= sdp_in_data(1)(8);
              end if;
            else
              sdp_md_r.opcode <= sdp_in_data(0)(7 downto 0);
              sdp_md_r.eof    <= sdp_in_data(0)(8);
            end if;
          elsif sdp_buffers_avail_r /= 0 or buffer_consumed then
            -- A data write that we can accept
            sdp_addr_r <= sdp_addr + 1;
          end if;
        end if;
      end if; -- if/else reset/operating
    end if; -- if rising edge
  end process;
end architecture rtl;
