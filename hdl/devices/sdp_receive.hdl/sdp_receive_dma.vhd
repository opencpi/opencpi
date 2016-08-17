-- The SDP receiver DMA submodule, to take data from the SDP port, and put it in BRAM,
-- where the BRAM width is a parameter.

-- Active flow control mode: (means being DMA master for flow control, not data).
--   SDP writes addressed to this BRAM come into this module which acts as a memory slave
--   The parent module indicates when buffers are consumed, triggering this
--   module to master a flag/doorbell write to the SDP.
--   A doorbell to the parent tells it when the buffer has arrived.
--   Requires being passive/slave, which requires the SDP interconnect to act as a slave,
--     which is not the case currently with Zynq using the AXI_HP
--     which could require using M_AXI_GP1 like BAR1 on PCI if actually required.
--     which would require the sdp2axi to be much more complex
--     which would mean that the sdp2axi could be used on PCI on ml605 using the AXI-PCI bridge
--
-- Active message mode (means being DMA master to read data and write flags).
--   The parent module indicates when doorbells arrive which triggers this
--   module to master read requests (overlapping, disconnected) to read the data.
--   When the last read data has arrived, a "buffer is available" is sent to the parent
--   for reading it out of BRAM.

-- TODO:
--   throttle number of issued reads in case tags can't encode then
--   can we use eop more efficiently than counting?

library IEEE, ocpi, util, bsv, sdp;
use IEEE.std_logic_1164.all, ieee.numeric_std.all;
use ocpi.types.all, ocpi.all, ocpi.util.all, sdp.sdp.all;
use work.sdp_receive_constants.all;
entity sdp_receive_dma is
  generic (ocpi_debug       : boolean;
           sdp_width        : natural;
           memory_depth     : natural;
           max_buffers      : natural);
  port (   operating        : in bool_t;
           reset            : in bool_t;
           -- properties
           buffer_ndws      : in  unsigned(width_for_max(memory_depth - 1)-1 downto 0);
           lcl_buffer_count : in  unsigned(width_for_max(max_buffers)-1 downto 0);
           role             : in  role_t;
           rem_flag_addr    : in  ulonglong_t;
           rem_flag_pitch   : in  ulong_t;
           rem_data_addr    : in  ulonglong_t;
           rem_data_pitch   : in  ulong_t;
           rem_buffer_count : in  uchar_t;
           -- inputs from CTL/WSI side
           length_not_empty : in  bool_t;
           length_out       : in  metalength_dws_t;
           avail_not_full   : in  bool_t;
           buffer_consumed  : in  bool_t;
           -- outputs to CTL/WSI side
           length_deq       : out bool_t;
           avail_enq        : out bool_t;
           faults           : out uchar_t;
           bramb_in         : out dword_array_t(0 to sdp_width-1);
           bramb_write      : out bool_array_t(0 to sdp_width-1);
           bramb_addr       : out unsigned(width_for_max(memory_depth - 1)-1 downto 0);
--           status           : out ulong_t;
           -- inputs from SDP
           sdp_in          : in  m2s_t;
           sdp_in_data     : in  dword_array_t(0 to sdp_width-1);
           -- outputs to SDP
           sdp_out         : out s2m_t;
           sdp_out_data    : out dword_array_t(0 to sdp_width-1));
end entity sdp_receive_dma;
architecture rtl of sdp_receive_dma is
  -- Local worker constants
  constant addr_shift_c    : natural := width_for_max(sdp_width * 4 - 1);
  constant addr_width_c    : natural := width_for_max(memory_depth - 1);
  subtype bram_addr_t is unsigned(addr_width_c-1 downto 0);
  function dw_addr_shift_c return natural is
  begin
    if sdp_width = 1 then
      return 0;
    end if;
    return width_for_max(ocpi.util.max(1, sdp_width - 1));
  end dw_addr_shift_c;
  subtype sdp_addr_t is unsigned(addr_width_c + dw_addr_shift_c - 1 downto 0);
  constant count_width_c   : natural := width_for_max(max_buffers);
  subtype buffer_count_t is unsigned(count_width_c - 1 downto 0);
  signal flags_to_send_r     : buffer_count_t;
  -- For arithmetic about the dws in a transfer, including the value sdp_width (not - 1)
  subtype sdp_xfr_dw_t is unsigned(width_for_max(sdp_width)-1 downto 0);
  constant xfer_width        : sdp_xfr_dw_t := to_unsigned(sdp_width, sdp_xfr_dw_t'length);
  -- given the address, what is the offset in the first sdp xfr
  function sdp_addr_dw_offset(addr : addr_t) return sdp_xfr_dw_t is
  begin
    if sdp_width = 1 then
      return to_unsigned(0, sdp_xfr_dw_t'length);
    else
      return addr(width_for_max(ocpi.util.max(1, sdp_width-1))-1 downto 0);
    end if;
  end sdp_addr_dw_offset;
  signal sending_flag        : bool_t;
  signal can_take            : bool_t;
  signal first_dw            : sdp_xfr_dw_t;
  signal dws_in_first_xfer   : sdp_xfr_dw_t;
  signal dws_in_xfer         : sdp_xfr_dw_t;
  signal dws_in_packet       : pkt_ndw_t;
  signal bram_addr           : bram_addr_t;
  signal sdp_addr            : sdp_addr_t; -- DW addresses into the BRAM, on any DW boundary
  signal sdp_am_addr         : sdp_addr_t;
  signal sdp_afc_addr        : sdp_addr_t;
  signal sdp_addr_r          : sdp_addr_t; -- from remote writer, incremented per xfr
  -- ActiveMessage: for processing read responses from multiple out-of-order read requests
  -- for different buffers.  Athough the response fragments for a given read (with a given XID)
  -- are assumed to arrive in order, fragments for different reads are not ordered.
  type sdp_addrs_t is array (natural range <>) of sdp_addr_t;
  signal sdp_am_addr_r       : sdp_addrs_t(0 to max_buffers-1); -- counting up as data comes
  signal sdp_am_last_r       : sdp_addrs_t(0 to max_buffers-1); -- to know when done
  signal sdp_dws_left        : pkt_ndw_t;
  signal sdp_dws_left_r      : pkt_ndw_t;
  signal sdp_starting_r      : bool_t;
  -- Sending flags and indexing buffers:
  -- Active Flow Control: flag is sent when buffer has become empty/consumed by WSI.
  --                      buffers are consumed in order, using rem_buffer_idx_r.
  -- Active Message: flag is sent when remote buffer is now empty on producer side, based on
  --                 the completion of our remote read.  This happens out of order based on the
  --                 last read completion "fragment" for the remote read, and uses the low bits
  --                 of the xid to indicate which buffer the fragment is for.
  --                 sdp_buffer_idx_r is used to know which buffer is the next one to
  --                 be given to the WSI side.
  signal lcl_buffer_idx_r    : buffer_count_t;
  signal rem_buffer_idx_r    : uchar_t;
  -- This is a count of buffers that are not full, including in progress of filling
--  signal rem_buffers_empty_r : uchar_t;
  signal lcl_buffers_empty_r : buffer_count_t;
  signal lcl_read_idx_r      : buffer_count_t;
  signal rem_read_idx_r      : uchar_t;
  signal rem_flag_addr_r     : whole_addr_t;
  signal rem_read_addr_r     : whole_addr_t;
  signal lcl_response_addr_r : sdp_addr_t;
  signal faults_r            : uchar_t;
  subtype buffer_idx_t is unsigned(count_width_c-2 downto 0);
  signal sdp_am_buffer_idx   : buffer_idx_t;
  signal read_starting       : bool_t; -- next cycle will be a read request
  signal reading_r           : bool_t; -- a read is being issued waiting to be accepted
  signal read_accepted       : bool_t; -- last cycle of a read request
  signal read_complete       : bool_t; -- cycle when the last data for a read has arrived
  signal flag_needed         : bool_t; -- a flag cycle is needed
  signal flagging            : bool_t; -- a flag cycle is in progress
  signal flagging_r          : bool_t; -- a flag write is being issues waiting to be accepted
  signal flag_accepted       : bool_t; -- a flag request is being accepted
  signal taking              : bool_t;
  signal rem_last_buffer_idx : uchar_t;
  signal lcl_last_buffer_idx : buffer_count_t;
  --signal status1_r            : uchar_t;
  --signal status2_r            : uchar_t;
  --signal status3_r            : uchar_t;
  --signal status4_r            : uchar_t;
  procedure incdec(signal s: inout unsigned; inc : bool_t; dec : bool_t) is begin
    if inc and not its(dec) then
      s <= s + 1;
    end if;
    if dec and not its(inc) then
      s <= s - 1;
    end if;
  end incdec;
begin
  --status <= to_ulong(std_logic_vector(sdp_am_addr (11 downto 0)) & -- 12
  --                   std_logic_vector(sdp_am_last_r(to_integer(sdp_am_buffer_idx))(11 downto 0)) & -- 12
  --                   std_logic_vector(sdp_am_buffer_idx) & "0" &  -- 2
  --                   slv(sdp_in.sdp.eop) &
  --                   slv(taking) & slv(read_complete) & slv(sending_flag) & slv(flag_accepted));
  --------------------------------------------------------------------------------
  -- Bookkeeping for buffers
  --------------------------------------------------------------------------------
  rem_last_buffer_idx <= resize(rem_buffer_count - 1, rem_last_buffer_idx'length);
  lcl_last_buffer_idx <= resize(lcl_buffer_count - 1, lcl_last_buffer_idx'length);
  sdp_am_buffer_idx   <= sdp_in.sdp.header.xid(sdp_am_buffer_idx'range);
  --------------------------------------------------------------------------------
  -- Bookkeeping for SDP packets
  --------------------------------------------------------------------------------
  -- For responses the count field is ignored.
  dws_in_packet     <= count_in_dws(sdp_in.sdp.header);
  first_dw          <= sdp_addr_dw_offset(sdp_in.sdp.header.addr) when its(sdp_starting_r)
                       else (others => '0');
  dws_in_first_xfer <= resize(ocpi.util.min(xfer_width -
                                            sdp_addr_dw_offset(sdp_in.sdp.header.addr),
                                            dws_in_packet), sdp_xfr_dw_t'length)
                       when sdp_in.sdp.header.op = write_e else
                       resize(xfer_width - sdp_addr_dw_offset(sdp_in.sdp.header.addr),
                              sdp_xfr_dw_t'length);
  sdp_dws_left      <= dws_in_packet - dws_in_first_xfer when its(sdp_starting_r)
                       else sdp_dws_left_r - dws_in_xfer;
  dws_in_xfer       <= dws_in_first_xfer when its(sdp_starting_r)
                       else resize(ocpi.util.min(sdp_dws_left_r, sdp_width),
                                   sdp_xfr_dw_t'length)
                       when sdp_in.sdp.header.op = write_e else
                       xfer_width;
  --------------------------------------------------------------------------------
  -- Managing the addresses of inbound writes or read responses, into BRAM
  --------------------------------------------------------------------------------
  sdp_afc_addr      <= resize(sdp_in.sdp.header.addr, sdp_afc_addr'length)
                       when its(sdp_starting_r) else sdp_addr_r;
  sdp_am_addr       <= sdp_am_addr_r(to_integer(sdp_am_buffer_idx))
                       when its(sdp_starting_r) else sdp_addr_r;
  sdp_addr          <= sdp_am_addr when role = activemessage_e else sdp_afc_addr;
  --------------------------------------------------------------------------------
  -- Communications with the parent.
  --------------------------------------------------------------------------------
  length_deq        <= read_accepted
                       when role = activemessage_e else
                       length_not_empty;
  avail_enq         <= length_not_empty
                       when role = activeflowcontrol_e else
                       read_complete;
  faults            <= faults_r;
  --------------------------------------------------------------------------------
  -- Mastering the SDP to issue requests - either read requests or flag writes
  --------------------------------------------------------------------------------
  -- Note there will be one cycle between reads to allow the length-dequeue to happen
  -- Note also that the length will be non-zero.  ZLMs are optimized at the upper level
  read_starting <= to_bool(role = activemessage_e and
                           length_not_empty and lcl_buffers_empty_r /= 0 and
                           not its(reading_r) and not its(flagging_r));
  read_accepted <= reading_r and sdp_in.sdp.ready;
  -- Flags are different than reads - they can happen in one cycle, and don't need idles
  -- Reads take precedence over flags
  flag_needed   <= to_bool(flags_to_send_r /= 0 or read_complete);
  flagging      <= to_bool(flagging_r or (flag_needed and not its(read_starting or reading_r)));
  flag_accepted <= flagging and sdp_in.sdp.ready;
  --------------------------------------------------------------------------------
  -- Slave (or read responses) handling
  --------------------------------------------------------------------------------
  -- The current xfer of the current read response is the last in the entire message
  read_complete     <= to_bool(taking and role = activemessage_e and
                               sdp_am_addr = sdp_am_last_r(to_integer(sdp_am_buffer_idx)));
  -- We are taking a xfer from the SDP
  taking            <= sdp_in.sdp.valid and can_take;
  can_take          <= btrue; -- to_bool(sdp_buffers_empty_r /= 0);
  --------------------------------------------------------------------------------
  -- Memory interface for writing BRAM
  --------------------------------------------------------------------------------
  bram_addr         <= sdp_addr(bram_addr'left + dw_addr_shift_c downto dw_addr_shift_c);
  bramb_addr        <= bram_addr;
g1: for i in 0 to sdp_width-1 generate
  bramb_write(i)    <= bfalse when i < first_dw or i > first_dw + dws_in_xfer
                       else sdp_in.sdp.valid;
  bramb_in(i)       <= sdp_in_data(i);
  end generate g1;
  --------------------------------------------------------------------------------
  -- Module output ports on the SDP side
  --------------------------------------------------------------------------------
  -- For data arriving on sdp_in: 
  sdp_out.sdp.ready          <= taking;
  -- For flag writes and data reads leaving on sdp_out:
  sdp_out.sdp.header.op      <= read_e when its(reading_r) else write_e;
  sdp_out.sdp.header.xid     <= lcl_read_idx_r;   -- ignored for writes
  sdp_out.sdp.header.lead    <= (others => '0');  -- we are always aligned on a DW
  sdp_out.sdp.header.trail   <= (others => '0');  -- we always send whole DWs
  sdp_out.sdp.header.count   <= (others => '0') when its(flagging) else
                                resize(length_out - 1, count_t'length);
  sdp_out.sdp.header.node    <= sdp_in.id;
  sdp_out.sdp.header.addr    <= rem_read_addr_r(addr_width-1 downto 0)
                                when its(reading_r) else
                                rem_flag_addr_r(addr_width-1 downto 0);
  sdp_out.sdp.header.extaddr <= rem_read_addr_r(whole_addr_bits_c-1 downto addr_width)
                                when its(reading_r) else
                                rem_flag_addr_r(whole_addr_bits_c-1 downto addr_width);
  sdp_out.sdp.eop            <= btrue;
  sdp_out.sdp.valid          <= flagging or reading_r;
  sdp_out.dropCount          <= (others => '0');
g2: for i in 0 to sdp_width-1 generate
    sdp_out_data(i) <= slvn(1, dword_size) when i = 0 else (others => '0');
  end generate g2;

  --------------------------------------------------------------------------------
  -- The process that takes messages from SDP and:
  --  0. waits for buffer space to be available to avoid overwriting previous messages
  --  1. writes data to BRAM either from inbound writes or responses to our reads
  --  2. if active message (reading/pulling), then when all response data has arrived:
  --     a. Sends a flag to the parent to indicate that the pulling is done
  --     b. Enqueues an "available" token to the WSI side
  --------------------------------------------------------------------------------
  sdp2bram : process(sdp_in.clk)
  begin
    if rising_edge(sdp_in.clk) then
      if its(reset) then -- FIXME this is not really the right reset
        sdp_addr_r          <= (others => '0'); -- SDP type DW address
        rem_buffer_idx_r    <= (others => '0');
        lcl_buffer_idx_r    <= (others => '0');
        sdp_starting_r      <= btrue;
        faults_r            <= (others => '0');
        flags_to_send_r     <= (others => '0');
        rem_read_idx_r      <= (others => '0');
        lcl_response_addr_r <= (others => '0');
        lcl_read_idx_r      <= (others => '0');
        flags_to_send_r     <= (others => '0');
        sdp_am_addr_r       <= (others => (others => '0')); --for cleaner sim
        sdp_am_last_r       <= (others => (others => '0')); --for cleaner sim
        flagging_r          <= bfalse;
        reading_r           <= bfalse;
        --status1_r           <= (others => '0');
        --status2_r           <= (others => '0');
        --status3_r           <= (others => '0');
        --status4_r           <= (others => '0');
      elsif not operating then
        -- reset state that depends on properties
--        rem_buffers_empty_r <= resize(rem_buffer_count, rem_buffers_empty_r'length);
        lcl_buffers_empty_r <= resize(lcl_buffer_count, lcl_buffers_empty_r'length);
        rem_flag_addr_r     <= rem_flag_addr(rem_flag_addr_r'left+2 downto 2);
        rem_read_addr_r     <= rem_data_addr(rem_read_addr_r'left+2 downto 2);
      else
        if  bram_addr >= memory_depth then
          faults_r(6) <= btrue;
        end if;
        if length_not_empty and not its(avail_not_full) then
          faults_r(7) <= btrue;
        end if;
        -- Active Message processing for reads
        if its(read_starting) then
          reading_r <= btrue;
          sdp_am_addr_r(to_integer(lcl_read_idx_r)) <= lcl_response_addr_r;
          sdp_am_last_r(to_integer(lcl_read_idx_r)) <=
            resize(lcl_response_addr_r + length_out - 1, sdp_addr_t'length);
        end if;
        if its(read_accepted) then
          reading_r <= bfalse; -- yes, there will be a dead cycle between reads
          -- Local side bookkeepping
          if lcl_read_idx_r = lcl_last_buffer_idx then
            lcl_read_idx_r      <= (others => '0');
            lcl_response_addr_r <= (others => '0');
          else
            lcl_read_idx_r      <= lcl_read_idx_r + 1;
            lcl_response_addr_r <= lcl_response_addr_r + buffer_ndws;
          end if;
          -- Remote side bookkeepping
          if rem_read_idx_r = rem_last_buffer_idx then
            rem_read_idx_r      <= (others => '0');
            rem_read_addr_r     <= rem_data_addr(rem_read_addr_r'left+2 downto 2);
          else
            rem_read_idx_r      <= rem_read_idx_r + 1;
            rem_read_addr_r     <= rem_read_addr_r + rem_data_pitch(31 downto 2);
          end if;
        end if;
        -- Maintain remote flag addressing
        -- Note remote flags are role-dependent:
        --   when active message we are writing to flags per remote buffer
        --   when active flow control we are writing to flags per local buffer
        if its(flag_accepted) then
          flagging_r <= bfalse;
--          status1_r <= status1_r + 1;
          if (role = activeflowcontrol_e and rem_buffer_idx_r = lcl_last_buffer_idx) or
             (role = activemessage_e and rem_buffer_idx_r = rem_last_buffer_idx) then
            rem_buffer_idx_r <= (others => '0');
            rem_flag_addr_r  <= rem_flag_addr(rem_flag_addr_r'left+2 downto 2);
          else
            rem_buffer_idx_r <= rem_buffer_idx_r + 1;
            rem_flag_addr_r  <= rem_flag_addr_r + rem_flag_pitch(31 downto 2);
          end if;
        elsif its(flagging) then
          flagging_r <= btrue;
        end if;
        -- Maintain buffer empty count and queued consumption events (AFC only)
        case role is
          when activeflowcontrol_e|passive_e =>                            
            incdec(flags_to_send_r, buffer_consumed, flag_accepted);
            incdec(lcl_buffers_empty_r, buffer_consumed, length_not_empty);
            --if its(buffer_consumed) then
            --  if not flag_accepted then
            --    flags_to_send_r <= flags_to_send_r + 1;
            --  end if;
            --  if not length_not_empty then
            --    lcl_buffers_empty_r <= lcl_buffers_empty_r + 1;
            --  end if;
            --else
            --  if its(flag_accepted) then
            --    flags_to_send_r <= flags_to_send_r - 1;
            --  end if;
            --  if its(length_not_empty) then
            --    lcl_buffers_empty_r <= lcl_buffers_empty_r - 1;
            --  end if;
            --end if;
          when activemessage_e =>
            incdec(lcl_buffers_empty_r, buffer_consumed, read_accepted);
            incdec(flags_to_send_r, read_complete, flag_accepted);
            --if buffer_consumed and not its(read_accepted) then
            --    lcl_buffers_empty_r <= lcl_buffers_empty_r + 1;
            --elsif not its(buffer_consumed) and read_accepted then
            --    lcl_buffers_empty_r <= lcl_buffers_empty_r - 1;
            --end if;
            --if flag_accepted and not its(read_complete) and flags_to_send_r /= 0 then
            --  flags_to_send_r <= flags_to_send_r - 1;
            --end if;
        end case;
        -- Deal with incoming writes or read responses
        if its(taking) then
          if its(sdp_starting_r) then
            sdp_dws_left_r <= sdp_dws_left;
          else
            sdp_dws_left_r <= sdp_dws_left_r - dws_in_xfer;
          end if;
          --if its(read_complete) then
          --  if its(sdp_in.sdp.eop) then
          --    status2_r <= status2_r + 1;
          --  else
          --    status3_r <= status3_r + 1;
          --  end if;
          --end if;
          if its(sdp_in.sdp.eop) then
            sdp_starting_r <= btrue;
            if role = activemessage_e then
              if its(read_complete) then -- last read response for a message
                -- If the flag transfer for this read completion is not immediate, queue it up
                if not flag_accepted then
--                  status4_r <= status4_r + 1;
                  flags_to_send_r <= flags_to_send_r + 1;
                end if;
              end if;
              sdp_am_addr_r(to_integer(sdp_am_buffer_idx)) <= sdp_addr + dws_in_xfer;
            end if;
          else
            sdp_starting_r <= bfalse;
            sdp_addr_r     <= sdp_addr + dws_in_xfer;
          end if;
        end if;
      end if; -- if/else reset/operating
    end if; -- if rising edge
  end process;
end architecture rtl;
