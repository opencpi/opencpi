library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.all; use ocpi.types.all; use ocpi.wci.all;

entity decoder is
  generic (
      worker                 : worker_t;
      properties             : properties_t;
      ocpi_debug             : bool_t;
      endian                 : endian_t);
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
      data_outputs           : out data_a_t(properties'range);
      is_read                : out bool_t;
      is_write               : out bool_t;
      raw_offset             : out unsigned (worker.decode_width -1 downto 0)
      );
end entity;

architecture rtl of decoder is
  signal my_reset        : Bool_t;      -- positive logic version
  signal my_error        : Bool_t;      -- immediate error detected here (not from worker)
  signal my_config_error : Bool_t;      -- immediate config error
  -- State for decoded accesses
  signal access_in       : access_t;
  signal my_access       : access_t;     -- combi or register as appropriate
  signal my_access_r     : access_t;     -- registered access in progress when not immediate
  -- State for control ops
  signal control_op_in   : control_op_t; -- combi input decode
  signal my_control_op   : control_op_t; -- combi or register as appropriate
  signal my_control_op_r : control_op_t; -- registered op when not immediately finished
  signal my_state_r      : state_t;
  signal next_op         : std_logic;
  signal ok_op           : std_logic;
  signal state_pos       : natural;
  signal op_pos          : natural;
  -- convert state enum value to state number
  -- because at least isim is broken and does not implement the "pos" function
  function get_state_pos(input: state_t) return natural is
  begin
    case input is
      when exists_e => return 0;
      when initialized_e => return 1;
      when operating_e => return 2;
      when suspended_e => return 3;
      when finished_e => return 4;
      when unusable_e => return 5;
    end case;
  end get_state_pos;
  -- convert control op enum value to a number
  -- because at least isim is broken and does not implement the "pos" function
  function get_op_pos(input: control_op_t) return natural is
  begin
    case input is
      when initialize_e   => return 0;
      when start_e        => return 1;
      when stop_e         => return 2;
      when release_e      => return 3;
      when before_query_e => return 4;
      when after_config_e => return 5;
      when test_e         => return 6;
      when no_op_e        => return 7;
    end case;
  end get_op_pos;

  --------------------------------------------------------------------------------
  -- From here down, only for properties
  --------------------------------------------------------------------------------
  -- State for write data
  signal my_data         : dword_t;       -- combi or register as appropriate
  signal my_data_r       : dword_t;       -- registered data when delayed
  -- state for access qualifiers for config read or write
  signal my_is_read      : bool_t;
  signal my_is_write     : bool_t;
  signal my_nbytes_1     : byte_offset_t;
  signal my_nbytes_1_r   : byte_offset_t;
  signal offset_in       : unsigned (worker.decode_width -1 downto 0);
  signal my_offset       : unsigned (worker.decode_width -1 downto 0);
  signal my_offset_r     : unsigned (worker.decode_width -1 downto 0);
  signal my_hi32         : bool_t;
  signal my_hi32_r       : bool_t;

  signal my_write_enables, my_read_enables : bool_array_t(properties'range);
  type my_offset_a_t is array(properties'range) of unsigned (worker.decode_width -1 downto 0);
  signal my_offsets      : my_offset_a_t;
  signal my_big_endian   : bool_t;
  signal high_dw         : bool_t;
  -- convert byte enables to low order address bytes
  impure function be2offset(input: in_t) return byte_offset_t is
    variable byte_en : std_logic_vector(input.MByteEn'range) := input.MByteEn; -- avoid pedantic error
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
  function any_true(bools : bool_array_t) return boolean is
    variable result: boolean := false;
  begin
    for i in bools'range loop
      if its(bools(i)) then result := true; end if;
    end loop;
    return result;
  end any_true;
begin
  --------------------------------------------------------------------------------
  -- Combi signals and outputs not specific to control ops or properties
  --------------------------------------------------------------------------------
  my_reset <= not ocp_in.MReset_n;
  access_in <= decode_access(ocp_in);
  -- ****** For now the OCCP cannot tolerate immediate responses, so we actually
  -- ****** ALWAYS delay for at least one clock.  
  --  my_access   <= access_in when my_access_r = none_e else my_access_r;
  my_access   <= my_access_r;
  -- The response output is combinatorial if done or error is set.
  resp <= ocp.SResp_ERR when my_access /= none_e and (its(error) or my_error) else
          ocp.SResp_DVA when my_access /= none_e and done and not its(my_error) and not its(error) else
          ocp.SResp_NULL;
  my_error  <= to_bool(my_access = error_e or
                       (my_access = control_e and (ok_op = '0' or next_op = '0')) or
                       my_config_error);
  -- The busy output is combinatorial, and my_access might be only registered
  busy <= to_bool(access_in /= none_e or my_access /= none_e);
  -- This should be a constant in the whole synthesized worker unit
  my_big_endian    <= to_bool(endian = big_e or (endian = dynamic_e and ocp_in.MFlag(1) = '1'));
  is_big_endian    <= my_big_endian;
  barrier          <= ocp_in.MFlag(2);
  rank             <= to_uchar(ocp_in.MFlag(10 downto 3));
  crew             <= to_uchar(ocp_in.MFlag(18 downto 11));
  --------------------------------------------------------------------------------
  -- Combi signals and outputs for control operations
  --------------------------------------------------------------------------------
--  control_op_in    <= ocpi.wci.to_control_op(ocp_in.MAddr(4 downto 2));
  control_op_in    <= ocpi.wci.to_control_op(ocp_in.MAddr(4 downto 2)) when access_in = control_e else no_op_e;
  -- the control op to the worker is either combinatorial or registered depending
  -- on whether it is delayed by the worker
-- FIXME: OCCP can't tolerate immediate ops, so we don't do it here
--  my_control_op    <= no_op_e when my_access /= control_e else
--                      control_op_in when my_control_op_r = no_op_e else
--                      my_control_op_r;
  my_control_op    <= my_control_op_r;
  control_op       <= my_control_op;
  state            <= my_state_r;
  is_operating     <= to_bool(my_state_r = operating_e);
  abort_control_op <= to_bool(ocp_in.MFlag(0) = '1');
  -- for our own error checking (not the worker's)
  state_pos <= get_state_pos(my_state_r);
  op_pos    <= get_op_pos(my_control_op);
  next_op   <= next_ops(state_pos)(op_pos);
  ok_op     <= worker.allowed_ops(op_pos);
  --------------------------------------------------------------------------------
  -- Combi signals and outputs for properties
  --------------------------------------------------------------------------------
  offset_in       <= unsigned(ocp_in.MAddr(worker.decode_width-1 downto 2)) & be2offset(ocp_in);
  my_offset       <= offset_in when my_reset or my_access_r = none_e else my_offset_r;
  my_data         <= ocp_in.MData        when my_access_r = none_e else my_data_r;
  my_nbytes_1     <= num_bytes_1(ocp_in) when my_access_r = none_e else my_nbytes_1_r;
  high_dw         <= to_bool(ocp_in.MAddr(2) = '1')
                     when endian = little_e or (endian = dynamic_e and not its(my_big_endian))
                     else to_bool(ocp_in.MAddr(2) = '0');
  my_hi32         <= high_dw when my_access_r = none_e else my_hi32_r;
  my_is_read      <= to_bool(my_access = read_e);
  my_is_write     <= to_bool(my_access = write_e);
  my_config_error <= to_bool((my_access = read_e and done and not any_true(my_read_enables)) or
                               (my_access = write_e and done and not any_true(my_write_enables)));
  -- output ports, based on internally generated signals that are also used internally
  -- force it to always be valid so that workers (even in sim) can assume a valid value.
  raw_offset      <= my_offset - worker.raw_property_base
                     when my_offset >= worker.raw_property_base and
                          its(my_is_read or my_is_write) else
                     (others => '0');
  write_enables   <= my_write_enables;
  read_enables    <= my_read_enables;
  nbytes_1        <= my_nbytes_1;
  hi32            <= my_hi32;
  is_read         <= to_bool(my_is_read and my_offset >= worker.raw_property_base);
  is_write        <= to_bool(my_is_write and my_offset >= worker.raw_property_base);

  -- clocked processing is for delayed completion and capturing requests
  reg: process(ocp_in.Clk) is
  begin
    -- Since we support combinatorial completion, the clocked processing
    -- deals only with longer lived commands
    if rising_edge(ocp_in.Clk) then
      if its(my_reset) then
        my_access_r     <= None_e;
        if worker.allowed_ops(control_op_t'pos(initialize_e)) = '1' then
          my_state_r <= exists_e;
        else
          my_state_r <= initialized_e;
        end if;
      elsif access_in /= none_e then
        -- the first cycle of the request, capture it all per OCP
        my_access_r     <= access_in;  -- delayed version until occp is fixed
        my_control_op_r <= control_op_in;
        -- From here down, only for properties
        my_data_r       <= ocp_in.MData;
        my_nbytes_1_r   <= num_bytes_1(ocp_in);
        my_offset_r     <= offset_in;
        my_hi32_r       <= high_dw; -- to_bool(ocp_in.MAddr(2) = '1');
      elsif its(done) or error or my_error then
        -- the last cycle of the request
        my_access_r     <= none_e;
        my_control_op_r <= no_op_e;
        if my_access = control_e and its(done) and not its(my_error) then
          -- successful control op - advance control state
          case my_control_op is
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
      elsif my_state_r /= unusable_e and finished then
        my_state_r <= finished_e;
      end if;
    end if; -- rising clock
  end process;

  -- generate property instances for each property
  -- they are all combinatorial by design
  g0: if properties'length > 0 generate
    g1: for i in 0 to properties'right generate -- properties'left to 0 generate
      g2: if its(ocpi_debug) or not properties(i).debug generate
      prop: component ocpi.wci.property_decoder
              generic map (properties(i), worker.decode_width, endian)
              port map(reset         => my_reset,
                       -- inputs describing property access
                       offset_in     => my_offset,
                       nbytes_1      => my_nbytes_1,
                       is_write      => my_is_write,
                       is_read       => my_is_read,
                       data_in       => my_data,
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

