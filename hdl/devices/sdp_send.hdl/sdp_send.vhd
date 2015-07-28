-- The SDP Sender, to take data from a WSI port, and put it in memory, making it available
-- for someone to read it out and acknowledge that reading.

library IEEE, ocpi, util;
use IEEE.std_logic_1164.all, ieee.numeric_std.all, ocpi.types.all, ocpi.util.all;
architecture rtl of sdp_send_worker is
  constant sdp_width_c    : natural := to_integer(sdp_width);
  constant memory_depth_c : natural := to_integer(memory_bytes) / (sdp_width_c * 4);
  constant addr_width_c   : natural := width_for_max(memory_depth_c - 1);
  constant max_buffers_c  : natural := to_integer(max_buffers);
  constant addr_shift_c   : natural := width_for_max(sdp_width_c * 4 - 1);
  constant count_width_c  : natural := width_for_max(max_buffers_c);
  constant nbytes_width_c : natural := width_for_max(sdp_width_c * 4); -- nbytes in frame
  constant meta_length_c  : natural := 0;
  constant meta_length_width_c  : natural := 22;
  constant meta_eof_c     : natural := meta_length_c + meta_length_width_c;
  constant meta_one_c     : natural := meta_eof_c + 1;
  constant meta_opcode_c  : natural := meta_one_c + 1;
  constant meta_opcode_width_c  : natural := 8;
  subtype buffer_count_t is unsigned(count_width_c - 1 downto 0);
  subtype bram_addr_t is unsigned(addr_width_c -1 downto 0);
  -- Combinatorial signals
  signal can_take          : bool_t;
  signal will_write        : bool_t;
  signal finishing_remote  : bool_t;
  signal finishing_local   : bool_t;
  signal prop_ram_addr     : bram_addr_t;
  signal max_offset        : bram_addr_t;
  signal next_buffer_addr  : bram_addr_t;
  signal bram_addr         : bram_addr_t;
  -- Registered signals
  ---- WSI buffer filling pointers
  signal buffer_offset_r   : bram_addr_t; -- offset in current buffer
  signal buffer_index_r    : buffer_count_t;
  signal buffer_addr_r     : bram_addr_t; -- base of current buffer
  ---- Top level availability
  signal remote_avail_r    : buffer_count_t;
  signal local_avail_r     : buffer_count_t;
  signal metadata_r        : ulong_array_t(0 to max_Buffers_c - 1);
  ---- Other state
  signal last_r            : bool_t; -- last buffer word has been written - no more room
  signal overflow_r        : bool_t; -- tried to write past the end of the buffer
  signal operating_r       : bool_t; -- were we operating in the last cycle?
  signal last_nbytes_r     : unsigned(nbytes_width_c-1 downto 0); -- residual in last frame
  signal raw_read_r        : bool_t;
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
  bram_addr        <= buffer_addr_r + buffer_offset_r;
  next_buffer_addr <= buffer_addr_r + props_in.bufferSize(addr_width_c - 1 + 2 downto 2);
  prop_ram_addr    <= props_in.raw_address(addr_width_c - 1 + 2 downto 2);
  can_take         <= to_bool(local_avail_r /= 0 and operating_r);
  will_write       <= can_take and in_in.ready and in_in.valid and not overflow_r;
  max_offset       <= props_in.bufferSize(addr_width_c - 1 + 2 downto 2) - 1;
  finishing_local  <= can_take and in_in.ready and in_in.eom and
                      (not overflow_r or not in_in.valid);
  finishing_remote <= to_bool(props_in.remoteTake_written and remote_avail_r /= 0);

  props_out.remoteAvailable <= resize(remote_avail_r, uchar_t'length);
  props_out.localAvailable  <= resize(local_avail_r, uchar_t'length);
  props_out.remoteBuffer    <= (others => '0');
  props_out.localBuffer     <= resize(buffer_index_r, uchar_t'length);
  props_out.overflow        <= overflow_r;
  props_out.currentRemote   <= (others => '0');
  props_out.metadata        <= metadata_r;
  in_out.take               <= in_in.ready and (will_write or finishing_local or
                                         (in_in.som and not in_in.ready));
  ctl_out.done              <= not props_in.raw_is_read or raw_read_r;

  -- Instance the BRAM and wire it up to the raw interface.
  -- Since the BRAM is single cycle, there is no handshake.
  bram : component util.util.BRAM2
    generic map(PIPELINED  => 0,
                ADDR_WIDTH => addr_width_c,
                DATA_WIDTH => sdp_width_c * 32,
                MEMSIZE    => memory_depth_c)
    port map   (CLKA       => ctl_in.clk,
                ENA        => '1',
                WEA        => props_in.raw_is_write,
                ADDRA      => std_logic_vector(prop_ram_addr),
                DIA        => props_in.raw_data,
                DOA        => props_out.raw_data,
                CLKB       => ctl_in.clk,
                ENB        => '1',
                WEB        => will_write,
                ADDRB      => std_logic_vector(bram_addr),
                DIB        => in_in.data,
                DOB        => open);
  
  -- Our state machines, separate for address and read-data
  work : process(ctl_in.clk)
    variable eom      : boolean;
    variable eof      : bool_t;
    variable nbytes   : unsigned(nbytes_width_c-1 downto 0);
    variable metadata : ulong_t;
  begin
    if rising_edge(ctl_in.clk) then
      if ctl_in.reset = '1' then
        buffer_offset_r <= (others => '0');
        buffer_index_r  <= (others => '0');
        buffer_addr_r   <= (others => '0');
        remote_avail_r  <= (others => '0');
        local_avail_r   <= (others => '0'); -- this is initialized later from properties
        last_r          <= bfalse;
        operating_r     <= bfalse;
        overflow_r      <= bfalse;
        last_nbytes_r   <= (others => '0');
        raw_read_r      <= bfalse;
      elsif not operating_r then
        raw_read_r      <= props_in.raw_is_read;
        -- initialization on first transition to operating.  poor man's "start".
        if its(ctl_in.is_operating) then
          operating_r   <= btrue;
          local_avail_r <= resize(props_in.bufferCount, buffer_count_t'length);
        end if;
      else
        raw_read_r      <= props_in.raw_is_read;
        if its(finishing_local) then
          if its(not finishing_remote) then
            local_avail_r <= local_avail_r - 1;
            remote_avail_r <= remote_avail_r + 1;
          end if;
        elsif its(finishing_remote) then
          if its(not finishing_local) then
            local_avail_r <= local_avail_r + 1;
            remote_avail_r <= remote_avail_r - 1;
          end if;
        end if;
        if ctl_in.is_operating and in_in.ready then
          eom := false;
          eof := bfalse;
          if its(in_in.valid) then
            nbytes        := be2bytes(in_in.byte_enable);
            last_nbytes_r <= nbytes;
            if its(last_r) then
              overflow_r <= btrue; -- this should be sticky
            elsif its(can_take) then    -- Data will be written
              if its(in_in.eom) then
                eom := true;
              elsif buffer_offset_r = max_offset then
                last_r <= btrue;   -- We have written the last word
              else
                buffer_offset_r <= buffer_offset_r + 1;
              end if;
            end if;
          elsif its(can_take) then
            -- ready, but no data
            if its(in_in.eom) then
              eom := true;
              nbytes := last_nbytes_r;
            elsif not in_in.som then
              eom := true;
              eof := btrue;
            end if;
          end if;
          if eom then
            metadata(meta_length_c + meta_length_width_c-1 downto meta_length_c) :=
              (resize(buffer_offset_r, meta_length_width_c) sll addr_shift_c) + nbytes;
            metadata(meta_one_c) := '1';
            metadata(meta_eof_c) := eof;
            metadata(meta_opcode_c + meta_opcode_width_c-1 downto meta_opcode_c) :=
              resize(unsigned(in_in.opcode), meta_opcode_width_c);
            metadata_r(to_integer(buffer_index_r)) <= metadata;
            last_r            <= bfalse;
            last_nbytes_r     <= (others => '0');
            buffer_offset_r   <= (others => '0');
            if buffer_index_r = props_in.bufferCount - 1 then
              buffer_index_r  <= (others => '0');
              buffer_addr_r   <= (others => '0');
            else    
              buffer_index_r  <= buffer_index_r + 1;
              buffer_addr_r   <= next_buffer_addr;
            end if;
          end if; -- eom
        end if; -- operating and ready
      end if; -- operating for one clock and not reset
    end if; -- rising edge
  end process;
end rtl;
