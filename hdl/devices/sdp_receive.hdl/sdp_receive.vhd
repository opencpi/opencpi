-- The SDP receiver, to take data from the SDP port, and put it out on a WSI port,
-- buffering messages in BRAM.
-- This should/will be split into two smaller modules...

-- The readsAllowed property indicates how many remote reads can be outstanding
-- Thus we can issue reads for all buffers up to this amount.
-- todo
--   split into low modules.
--   allow doorbells over control interface so no slave when active message
--   support active message mode.

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
  --------------------------------------------------------------------------------
  -- Signals and definitions for communication from doorbell to the WSI output
  -- "md" here is the metadata flowing from the doorbell to WSI
  --------------------------------------------------------------------------------
  signal md_in             : metadata_t;
  signal md_out            : metadata_t;
  signal md_out_slv        : std_logic_vector(metawidth_c-1 downto 0);
  signal md_deq            : std_logic;
  signal md_not_empty      : std_logic;
  -- ndws is rounded up so needs ONE fewer bits, not TWO
  signal md_out_ndws       : meta_dw_count_t;

  -- Convenience data types
  subtype bram_addr_t is unsigned(addr_width_c-1 downto 0);
  subtype buffer_count_t is unsigned(width_for_max(max_buffers_c) - 1 downto 0);
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
  signal faults_r              : uchar_t; -- some sticky error bits;
  signal faults                : uchar_t;
  signal dma_faults            : uchar_t;
  signal operating_r          : bool_t;  -- were we operating in the last cycle?
  signal wsi_starting_r       : bool_t;
  signal wsi_buffer_index_r   : buffer_count_t;
  signal wsi_buffer_addr_r    : bram_addr_t;    -- base of current buffer
  signal wsi_dws_left         : meta_dw_count_t;
  signal wsi_dws_left_r       : meta_dw_count_t;

  -- Length Sync FIFO definitions, conveying length from CTL to SDP for PULL
  signal length_enq       : bool_t;
  signal length_not_full  : std_logic;
  signal length_deq       : bool_t;
  signal length_not_empty : std_logic;
  signal length_out_slv   : std_logic_vector(metalength_dws_t'left downto 0);
  signal length_out       : metalength_dws_t;
  signal length_in        : metalength_dws_t;

  -- avail indication to WSI, via FIFO from SDP, enq'd when a pull is completed
  signal avail_enq       : bool_t;
  signal avail_deq       : bool_t;
  signal avail_out_slv   : std_logic_vector(0 downto 0);
  signal avail_out       : natural;
  signal avail_not_empty : std_logic;
  signal avail_not_full  : std_logic;

  -- consume indication from WSI to SDP, to trigger "local buffer is now empty" flag
  signal buffer_consumed    : bool_t;         -- pulse for buffer consumption from wsi side

  --------------------------------------------------------------------------------
  -- Signals and definitions for talking to the SDP side
  --------------------------------------------------------------------------------
  -- to the SDP side
  signal buffer_ndws     : bram_addr_t;
  signal buffer_count    : buffer_count_t;
  signal md_not_full     : std_logic;
  -- from the SDP side
  signal bramb_addr      : bram_addr_t;
  signal bramb_in        : dword_array_t(0 to sdp_width_c-1);
  signal bramb_write     : bool_array_t(0 to sdp_width_c-1);
  signal bad_write       : bool_t;
  signal md_enq          : bool_t;

  ---- Global state
  signal sdp_reset_n       : std_logic;
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
  end generate g0;
  -- Metadata fifo enqueued from doorbell then used on WSI, all in the same clock domain
  -- (until ctl clock is different from wsi clock.)
  -- CTL -> WSI
  metafifo : component bsv.bsv.SizedFIFO
   generic map(p1width      => metawidth_c,
               p2depth      => roundup_2_power_of_2(max_buffers_c), -- must be power of 2
               p3cntr_width => width_for_max(roundup_2_power_of_2(max_buffers_c)-1))
   port map   (CLK          => ctl_in.clk, -- maybe syncfifo later
               RST          => ctl_reset_n,
               D_IN         => slv(props_in.remote_doorbell(0)),
               ENQ          => std_logic(md_enq),
               FULL_N       => md_not_full,
               D_OUT        => md_out_slv,
               DEQ          => md_deq,
               EMPTY_N      => md_not_empty,
               CLR          => '0');
  md_out      <= slv2meta(md_out_slv);
  md_out_ndws <= resize((md_out.length + dword_bytes - 1) srl 2, md_out_ndws'length);
  md_enq      <= props_in.remote_doorbell_any_written;
  md_deq      <= will_give and last_give;
  avail_deq   <= md_deq;
  -- Length fifo enqueued from doorbell for active message/pull mode, dequeued on the SDP side
  -- Telling the SDP (when actively PULLING data) to read this much data
  -- CTL -> SDP
  lengthfifo : component bsv.bsv.SyncFIFO
   generic map(dataWidth    => metalength_dws_t'length,
               depth        => roundup_2_power_of_2(max_buffers_c), -- must be power of 2
               indxWidth    => width_for_max(roundup_2_power_of_2(max_buffers_c)-1))
   port map   (sCLK         => ctl_in.clk, -- maybe syncfifo later
               sRST         => ctl_reset_n,
               dCLK         => sdp_in.clk,
               sENQ         => std_logic(length_enq),
               sD_IN        => std_logic_vector(length_in),
               sFULL_N      => length_not_full,
               dDEQ         => length_deq,
               dD_OUT       => length_out_slv,
               dEMPTY_N     => length_not_empty);
  md_in <= slv2meta(slv(props_in.remote_doorbell(0)));
  length_in  <= resize((md_in.length + dword_bytes-1)/dword_bytes, length_in'length);
  --  FYI:  this expression crashes isim 14.67 with a SIGSEGV
  --  length_in  <= resize((slv2meta(slv(props_in.remote_doorbell(0))).length +
  --                       dword_bytes-1)/dword_bytes, length_in'length);
  length_out <= unsigned(length_out_slv);
  length_enq <= to_bool(props_in.remote_doorbell_any_written and md_in.length /= 0);

  -- A sync fifo to indicate message arrival events from SDP to WSI
  -- I.e. when the SDP side is done PULLING data, it indicates the buffer is full,
  -- and then the WSI processing can send the message, according to the metadata Fifo.
  -- SDP -> WSI
  availfifo : component bsv.bsv.SyncFIFO
   generic map(dataWidth    => 1,
               depth        => roundup_2_power_of_2(max_buffers_c),
               indxWidth    => width_for_max(roundup_2_power_of_2(max_buffers_c)-1))
   port map   (sCLK         => sdp_in.clk,
               sRST         => sdp_reset_n,
               dCLK         => ctl_in.clk,
               sENQ         => avail_enq,
               sD_IN        => "1",
               sFULL_N      => avail_not_full,
               dDEQ         => avail_deq,
               dD_OUT       => avail_out_slv,
               dEMPTY_N     => avail_not_empty);
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
  sdp_reset_n          <= not sdp_in.reset;
  buffer_count         <= resize(props_in.buffer_count, buffer_count_t'length);
  ctl_reset_n          <= not ctl_in.reset;
  wsi_next_buffer_addr <= (others => '0')
                          when wsi_buffer_index_r = props_in.buffer_count - 1 else
                          wsi_buffer_addr_r +
                          props_in.buffer_size(bram_addr_t'left + addr_shift_c
                                               downto addr_shift_c);
  brama_addr           <= wsi_next_buffer_addr when last_give and will_give else
                          brama_addr_r + 1 when its(will_give) else
                          brama_addr_r;
  will_give            <= to_bool(operating_r and faults = 0 and
                                  (its(avail_not_empty and avail_out_slv(0)) or
                                   (md_not_empty and md_out.length = 0)) and out_in.ready);
  last_give            <= to_bool(wsi_dws_left = 0);
  wsi_dws_left         <= md_out_ndws - ocpi.util.min(md_out_ndws,
                                                      resize(sdp_width, meta_dw_count_t'length))
                          when its(wsi_starting_r) else wsi_dws_left_r;
  buffer_ndws          <= props_in.buffer_size(bram_addr_t'left + addr_shift_c
                                               downto addr_shift_c);
  faults                <= faults_r or dma_faults;
  --------------------------------------------------------------------------------
  -- Module output ports on the CTL/WSI side
  --------------------------------------------------------------------------------
  ctl_out.finished    <= to_bool(faults /= 0 );
  props_out.faults     <= faults;
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
        brama_addr_r       <= (others => '0');
        wsi_buffer_index_r <= (others => '0');
        wsi_buffer_addr_r  <= (others => '0');
        operating_r        <= bfalse;
        faults_r            <= (others => '0');
        wsi_starting_r     <= btrue;
      elsif not operating_r then
        -- initialization on first transition to operating.  poor man's "start".
        if its(ctl_in.is_operating) then
          operating_r   <= btrue;
          if props_in.buffer_size > memory_bytes or
             props_in.buffer_size(addr_shift_c-1 downto 0) /= 0 then
            faults_r(0) <= '1';
          end if;
        end if;
      elsif not ctl_in.is_operating then
        operating_r <= bfalse;
      else
        if md_enq and not its(md_not_full) then
          faults_r(2) <= btrue;
        elsif length_enq and not its(length_not_full) then
          faults_r(3) <= btrue;
        elsif its(will_give) then
          brama_addr_r <= brama_addr;
          if its(last_give) then
            wsi_starting_r       <= btrue;
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
            wsi_starting_r       <= bfalse;
          end if;
        end if; -- will_give
      end if; -- operating
    end if; -- rising edge
  end process;

  -- The sub module that deals with the SDP side of the BRAM
  dma : entity work.sdp_receive_dma
    generic map (ocpi_debug       => its(ocpi_debug),
                 sdp_width        => sdp_width_c,
                 memory_depth     => memory_depth_c,
                 max_buffers      => max_buffers_c)
    port map (   reset            => ctl_in.reset,
                 operating        => operating_r,  -- wrong clock domain, but stable enough?
                 -- properties
                 buffer_ndws      => buffer_ndws,
                 lcl_buffer_count => buffer_count,
                 role             => props_in.role,
                 rem_flag_addr    => props_in.remote_flag_addr(0),
                 rem_flag_pitch   => props_in.remote_flag_pitch(0),
                 rem_data_addr    => props_in.remote_data_addr(0),
                 rem_data_pitch   => props_in.remote_data_pitch(0),
                 rem_buffer_count => props_in.remote_buffer_count(0),
                 -- inputs from CTL/WSI side
                 length_not_empty => length_not_empty, -- a length (of next message) is available
                 length_out       => length_out,       -- length of next packet
                 avail_not_full   => avail_not_full,    -- the handshake to WSI is available
                 buffer_consumed  => buffer_consumed,
                 -- outputs to CTL/WSI side
                 length_deq       => length_deq,
                 avail_enq        => avail_enq,
                 faults           => dma_faults,
                 bramb_in         => bramb_in,
                 bramb_write      => bramb_write,
                 bramb_addr       => bramb_addr,
                 -- inputs from SDP
                 sdp_in           => sdp_in,
                 sdp_in_data      => sdp_in_data,
                 -- outputs to SDP
                 sdp_out          => sdp_out,
                 sdp_out_data     => sdp_out_data);

end architecture rtl;
