library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.all; use ocpi.types.all; use ocpi.wci.all;

entity decoder is
  generic (
      worker                 : worker_t;
      ocpi_debug             : bool_t;
      endian                 : endian_t;
      properties             : properties_t);
  port (
      ocp_in                 : in in_t;       
      done                   : in bool_t := btrue;
      error                  : in bool_t := bfalse;
      finished               : in bool_t;
      resp                   : out ocp.SResp_t;
      busy                   : out bool_t;
      control_op             : out control_op_t;
      state                  : out state_t;
      is_operating           : out bool_t;  -- just a convenience for state = operating_e
      abort_control_op       : out bool_t;
      is_big_endian          : out bool_t;   -- for runtime dynamic endian
      raw_in                 : in  raw_out_t;
      raw_out                : out raw_in_t;
      barrier                : out bool_t;
      crew                   : out uchar_t;
      rank                   : out uchar_t;
      -- From here down, only for properties
      write_enables          : out bool_array_t(properties'range);
      read_enables           : out bool_array_t(properties'range);
      offsets                : out offset_a_t(properties'range);
      indices                : out offset_a_t(properties'range);
      hi32                   : out bool_t;
      nbytes_1               : out byte_offset_t;
      data_outputs           : out data_a_t(properties'range));
end entity;

architecture rtl of decoder is
  signal my_reset        : Bool_t;       -- positive logic version
  signal my_error        : Bool_t;       -- immediate error detected here (not from worker)
  signal my_config_error : Bool_t;       -- immediate config error
  -- State for decoded accesses
  signal access_in       : access_t;
  signal my_access_r     : access_t;     -- registered access in progress when not immediate
  -- State for control ops
  signal control_op_in   : control_op_t; -- combi input decode
  signal my_control_op_r : control_op_t; -- registered op when not immediately finished
  signal my_state_r      : state_t;
  signal next_op         : std_logic;
  signal ok_op           : std_logic;
  signal state_pos       : natural;
  signal op_pos          : natural;
  signal is_raw_r        : bool_t;
  signal my_done         : bool_t;
  signal my_big_endian   : bool_t;
  signal offset_in       : unsigned(worker.decode_width -1 downto 0);
  signal my_offset_r     : unsigned(worker.decode_width -1 downto 0) := (others => '0');
  -- State for write data
  signal my_data_r       : dword_t;
  signal my_is_read_r    : bool_t;
  signal reading_r       : bool_t;
  signal my_is_write_r   : bool_t;
  --------------------------------------------------------------------------------
  -- From here down, only for non-raw properties
  --------------------------------------------------------------------------------
  -- state for access qualifiers for config read or write
  signal my_nbytes_1     : byte_offset_t;
  signal my_nbytes_1_r   : byte_offset_t;
  signal my_hi32_r       : bool_t;
  signal my_write_enables, my_read_enables : bool_array_t(properties'range);
  signal my_read_enables_r : bool_array_t(properties'range);
  type my_offset_a_t is array(properties'range) of unsigned (worker.decode_width -1 downto 0);
  signal my_offsets      : my_offset_a_t;
  signal high_dw         : bool_t;

  -- convert byte enables to low order address bytes
  function be2offset(byte_en: std_logic_vector(3 downto 0); my_big_endian: bool_t)
    return byte_offset_t is
  begin
    if endian = little_e or (endian = dynamic_e and not its(my_big_endian)) then
      case byte_en is
        when b"0010" =>           return b"01";
        when b"0100" | b"1100" => return b"10";
        when b"1000" =>           return b"11";
        when others =>            return b"00";
      end case;
    else
      case byte_en is
        when b"0100" =>           return b"01";
        when b"0010" | b"0011" => return b"10";
        when b"0001" =>           return b"11";
        when others =>            return b"00";
      end case;
    end if;
  end be2offset;
  function num_bytes_1(input : in_t) return byte_offset_t is
    variable byte_en : std_logic_vector(input.MByteEn'range) := input.MByteEn; -- avoid pedantic error
  begin
    case byte_en is
      when b"0001" | b"0010" | b"0100" | b"1000" => return b"00";
      when b"1100" | b"0011" =>                     return b"01";
      when b"1111" =>                               return b"11";                               
      when others =>                                return b"11";                               
    end case;
  end num_bytes_1;
  -- FIXME check that this synthesizes properly - may have to revert to logic...
  --  function any_true(bools : bool_array_t) return boolean is
  --    variable result: boolean := false;
  --  begin
  --    for i in bools'range loop
  --      if its(bools(i)) then result := true; end if;
  --    end loop;
  --    return result;
  --  end any_true;
begin
  --------------------------------------------------------------------------------
  -- Combi signals and outputs not specific to control ops or properties
  --------------------------------------------------------------------------------
  my_reset       <= not ocp_in.MReset_n;
  access_in      <= write_e when
                    ocp_in.MCmd = ocp.MCmd_WRITE and ocp_in.MAddrSpace(0) = '1' else
                    read_e when
                    ocp_in.MCmd = ocp.MCmd_READ and ocp_in.MAddrSpace(0) = '1' else
                    control_e when 
                    ocp_in.MCmd = ocp.MCmd_READ and ocp_in.MAddrSpace(0) = '0' else
                    none_e when
                    ocp_in.MCmd = ocp.MCmd_IDLE else
                    error_e;
  resp          <= ocp.SResp_ERR when my_access_r /= none_e and my_error else
                   ocp.SResp_DVA when my_access_r /= none_e and my_done and not its(my_error) else
                   ocp.SResp_NULL;
  my_error      <= to_bool(my_access_r = error_e or
                           (my_access_r = control_e and (ok_op = '0' or next_op = '0')) or
                           ((my_access_r = read_e or my_access_r = write_e) and
                            ((its(is_raw_r) and raw_in.error) or
                             (not its(is_raw_r) and error) or
                             my_config_error)));
  my_done       <= raw_in.done when its(is_raw_r) else done and not reading_r;
  busy          <= to_bool(access_in /= none_e or my_access_r /= none_e);
  my_big_endian <= to_bool(endian = big_e or (endian = dynamic_e and ocp_in.MFlag(1) = '1'));
  is_big_endian <= my_big_endian;
  barrier       <= ocp_in.MFlag(2);
  rank          <= to_uchar(ocp_in.MFlag(10 downto 3));
  crew          <= to_uchar(ocp_in.MFlag(18 downto 11));
  --------------------------------------------------------------------------------
  -- Combi signals and outputs for control operations
  --------------------------------------------------------------------------------
  control_op_in       <= ocpi.wci.to_control_op(ocp_in.MAddr(4 downto 2))
                         when access_in = control_e else no_op_e;
  control_op          <= my_control_op_r;
  state               <= my_state_r;
  is_operating        <= to_bool(my_state_r = operating_e);
  abort_control_op    <= to_bool(ocp_in.MFlag(0) = '1');
  -- for our own error checking (not the worker's)
  state_pos           <= get_state_pos(my_state_r);
  op_pos              <= get_op_pos(my_control_op_r);
  next_op             <= next_ops(state_pos)(op_pos);
  ok_op               <= worker.allowed_ops(op_pos);
  raw_out.byte_enable <= ocp_in.MByteEn;
  raw_out.is_read     <= my_is_read_r and is_raw_r;
  raw_out.is_write    <= my_is_write_r and is_raw_r;
  raw_out.data        <= my_data_r;
  --------------------------------------------------------------------------------
  -- Combi signals and outputs for properties
  --------------------------------------------------------------------------------
  offset_in       <= unsigned(ocp_in.MAddr(worker.decode_width-1 downto 2)) &
                              be2offset(ocp_in.MByteEn, bfalse);
  my_nbytes_1     <= num_bytes_1(ocp_in) when my_access_r = none_e else my_nbytes_1_r;
  high_dw         <= to_bool(ocp_in.MAddr(2) = '1')
                     when endian = little_e or (endian = dynamic_e and not its(my_big_endian))
                     else to_bool(ocp_in.MAddr(2) = '0');
  my_config_error <= bfalse;
--  my_config_error <= to_bool((my_access_r = read_e and done and not its(is_raw) and
--                              not any_true(my_read_enables)) or
--                             (my_access_r = write_e and done and not its(is_raw) and
--                              not any_true(my_write_enables)));
  raw_out.address <= resize(my_offset_r - worker.raw_property_base, raw_out.address'length)
                     when my_offset_r >= worker.raw_property_base and
                          its(my_is_read_r or my_is_write_r) else
                     (others => '0');
  write_enables   <= my_write_enables;
  read_enables    <= my_read_enables_r;
  nbytes_1        <= my_nbytes_1;
  hi32            <= my_hi32_r;

  reg: process(ocp_in.Clk) is
  begin
    -- Since we support combinatorial completion, the clocked processing
    -- deals only with longer lived commands
    if rising_edge(ocp_in.Clk) then
      if its(my_reset) then
        my_access_r     <= None_e;
        is_raw_r        <= bfalse;
        my_offset_r     <= (others => '0');
        my_is_read_r    <= bfalse;
        my_is_write_r   <= bfalse;
        reading_r       <= bfalse;
        my_read_enables_r <= (others => '0');
        if worker.allowed_ops(control_op_t'pos(initialize_e)) = '1' then
          my_state_r <= exists_e;
        else
          my_state_r <= initialized_e;
        end if;
      elsif my_access_r = none_e then -- capture while idlng.  cycle starts when access_in != none
        my_access_r     <= access_in;
        my_is_read_r    <= to_bool(access_in = read_e);
        reading_r       <= to_bool(access_in = read_e);
        my_is_write_r   <= to_bool(access_in = write_e);
        is_raw_r        <= to_bool((access_in = read_e or access_in = write_e) and
                                   resize(offset_in, worker.decode_width+1) >=
                                   worker.raw_property_base);
        -- work hard to not have my_offset_r be garbage since there are pure combinatorial
        -- decoding paths from it.
        if access_in /= none_e then
          my_offset_r     <= offset_in;
        end if;
        my_control_op_r <= control_op_in;
        my_data_r       <= ocp_in.MData;
        my_nbytes_1_r   <= num_bytes_1(ocp_in);
        my_hi32_r       <= high_dw;
        -- Transition to finished state when we're idle and notice worker has asserted it.
        if my_state_r /= unusable_e and finished then
          my_state_r <= finished_e;
        end if;
      elsif its(reading_r) then -- pipeline the decoding for read enables
        reading_r <= bfalse;
        my_read_enables_r <= my_read_enables;
      elsif (its(my_done) or my_error) then
        -- the last cycle of the request when we're done or have an error
        my_read_enables_r <= (others => '0');
        my_access_r     <= none_e;
        my_is_read_r    <= bfalse;
        my_is_write_r   <= bfalse;
        my_control_op_r <= no_op_e;
        is_raw_r        <= bfalse;
        if my_access_r = control_e and (its(done) and not its(my_error)) then
          -- successful control op - advance control state
          case my_control_op_r is
            when INITIALIZE_e =>
              my_state_r <= initialized_e;
            when START_e =>
              if my_state_r /= finished_e then
                my_state_r <= operating_e;
              end if;
            when STOP_e =>
              if my_state_r /= finished_e then
                my_state_r <= suspended_e;
              end if;
            when RELEASE_e =>
              my_state_r <= unusable_e;
            when others => null;                            
          end case;
        end if;
      end if;
    end if; -- rising clock
  end process;

  -- generate property instances for each property
  -- they are all combinatorial by design
  g0: if properties'length > 0 generate
    g1: for i in 0 to properties'right generate
      g2: if ocpi_debug or not properties(i).debug generate
      prop: component ocpi.wci.property_decoder
              generic map (properties(i), worker.decode_width, endian)
              port map(reset         => my_reset,
                       -- inputs describing property access
                       offset_in     => my_offset_r,
                       nbytes_1      => my_nbytes_1,
                       is_write      => my_is_write_r,
                       is_read       => my_is_read_r,
                       data_in       => my_data_r,
                       is_big_endian => my_big_endian,
                       -- outputs from the decoding process
                       write_enable  => my_write_enables(i),
                       read_enable   => my_read_enables(i),
                       offset_out    => my_offsets(i),
                       index_out     => indices(i)(worker.decode_width-1 downto 0),
                       data_out      => data_outputs(i));
            indices(i)(indices(i)'left downto worker.decode_width) <= (others => '0');
            offsets(i) <= resize(my_offsets(i),offsets(i)'length);
      end generate g2;
    end generate g1;
  end generate g0;
end rtl;
