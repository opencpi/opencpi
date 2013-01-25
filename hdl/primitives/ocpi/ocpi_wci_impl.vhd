-- per-property decoder - purely combinatorial
-- result is write_enable, offset-in-array, and aligned data output
library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.all; use ocpi.types.all; use ocpi.wci.all;

entity property_decoder is
  generic (
        property     : property_t;   -- property type
        decode_width : natural);     -- decoder width in bits
  port (
        reset        : in  bool_t;                            -- active-low WCI worker reset
        offset_in    : in  unsigned(decode_width-1 downto 0); -- offset in Bytes
--        top          : in  bit_offset_t;                      -- High order bit position of datatype
        access_in    : in  access_t;                          -- Enumerated WCI access type
        data_in      : in  std_logic_vector(31 downto 0);     -- WCI slave data
        write_enable : out bool_t;                            -- active-high write pulse
        read_enable  : out bool_t;                            -- active-high read pulse
        offset_out   : out unsigned(decode_width-1 downto 0); -- 
        index_out    : out unsigned(decode_width-1 downto 0); --
        data_out     : out std_logic_vector(data_top(property,"00") downto 0)); --
end entity property_decoder;

architecture rtl of property_decoder is
  subtype decode_t is unsigned (decode_width-1 downto 0);
  signal my_offset : decode_t;
  signal byte_offset : byte_offset_t;
  function element_bytes(property : property_t) return decode_t is
    variable bytes : natural;
  begin
    if property.string_length /= 0 then
      bytes := ((property.string_length + 4)/4) * 4;
    else
      bytes :=  (property.data_width+7)/8;
    end if;
    return to_unsigned(bytes, decode_t'length);
  end element_bytes;
  -- I tried to use offset_in directly, not an argument, and isim generated a wierd warning...
  impure function my_decode (input: unsigned) return boolean is
  begin
    return not its(reset) and 
      input >= property.offset and
      (my_offset = 0 or
       (property.data_width > 32 and my_offset = 4) or
       (property.nitems > 1 and my_offset > 0 and my_offset <= property.bytes_1));
  end my_decode;
begin
  byte_offset  <= offset_in(1 downto 0);
  my_offset    <= offset_in - property.offset;
  write_enable <= to_bool(access_in = write_e and property.writable and my_decode(offset_in));
  read_enable  <= to_bool(access_in = read_e and property.readable and my_decode(offset_in));
  offset_out   <= (others => '0') when property.nitems <= 1 else my_offset;
  l32: if property.data_width = 32 generate
   data_out <= data_in;
  end generate;
  l16: if property.data_width = 16 generate
   data_out <= data_in(15 downto 0) when byte_offset = 0 else data_in(31 downto 16);
  end generate;
  l8: if property.data_width = 8 generate
   data_out <=
     data_in(31 downto 24) when byte_offset = 3 else
     data_in(23 downto 16) when byte_offset = 2 else
     data_in(15 downto  8) when byte_offset = 1 else
     data_in( 7 downto  0);
  end generate;
--  data_out     <= --data_in(data_top(property, byte_offset) downto to_integer(byte_offset*8));

--    data_in(data_top(property, "00")    downto  0) when byte_offset = 0  else
--    data_in(15                        downto  8) when byte_offset = 1  else
--    data_in(data_top(property, "00")+16 downto 16) when byte_offset = 2  else
--    data_in(31 downto 24);
  index_out <=
    to_unsigned(0,index_out'length) when property.nitems <= 1 else
    my_offset/element_bytes(property);  -- this won't be used for strings so no
                                        -- non-power of 2 math needed.
end rtl;

library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.all; use ocpi.types.all; use ocpi.wci.all;

entity decoder is
  generic (
      worker                 : worker_t;
      properties             : properties_t);
  port (
      ocp_in                 : in in_t;       
      done                   : in bool_t := btrue;
      resp                   : out ocp.SResp_t;
      write_enables          : out bool_array_t(properties'range);
      read_enables           : out bool_array_t(properties'range);
      offsets                : out offset_a_t(properties'range);
      indices                : out offset_a_t(properties'range);
      hi32                   : out bool_t;
      nbytes_1               : out byte_offset_t;
      data_outputs           : out data_a_t(properties'range);
      control_op             : out control_op_t;
      state                  : out state_t;
      is_operating           : out bool_t;  -- just a convenience for state = operating_e
      abort_control_op       : out bool_t;
      is_big_endian          : out bool_t   -- for runtime dynamic endian
      );
end entity;

architecture rtl of decoder is
  signal beoffset  : unsigned(1 downto 0);
  signal offset    : unsigned(worker.decode_width-1 downto 0);
  signal my_access : access_t;
  signal control_op_in : control_op_t;
  signal my_write_enables, my_read_enables : bool_array_t(properties'range);
  signal my_control_op : control_op_t;
  type my_offset_a_t is array(properties'range) of unsigned (worker.decode_width -1 downto 0);
  signal my_offsets : my_offset_a_t;
  signal my_state : state_t;
  signal my_reset : Bool_t;
  -- convert byte enables to low order address bytes
  function be2offset(input: in_t) return byte_offset_t is
    variable byte_en : std_logic_vector(input.MByteEn'range) := input.MByteEn; -- avoid pedantic error
  begin
    case byte_en is
      when b"0010" =>           return b"01";
      when b"0100" | b"1100" => return b"10";
      when b"1000" =>           return b"11";
      when others =>            return b"00";
    end case;
  end be2offset;
  --function top_bit(input: in_t) return bit_offset_t is
  --  variable byte_en : std_logic_vector(input.MByteEn'range) := input.MByteEn; -- avoid pedantic error
  --begin
  --  case byte_en is
  --    when b"0001" =>                     return 7;
  --    when b"0010" | b"0011" =>           return 15;
  --    when b"0100" =>                     return 23;
  --    when b"1000" | b"1100" | b"1111" => return 31;
  --    when others =>                      return 31;                                    
  --  end case;
  --end top_bit;
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
begin
  -- combinatorial signals used in various places
  state <= my_state;
  nbytes_1 <= num_bytes_1(ocp_in);
  is_big_endian <= to_bool(ocp_in.MFlag(1) = '1');
  abort_control_op <= to_bool(ocp_in.MFlag(0) = '1');
  beoffset <= be2offset(ocp_in);
  offset <= unsigned(ocp_in.MAddr(worker.decode_width-1 downto 2)) & beoffset;
  my_access <= decode_access(ocp_in);
  control_op_in <= ocpi.wci.to_control_op(ocp_in.MAddr(4 downto 2));
  hi32 <= to_bool(ocp_in.MAddr(2) = '1');
  my_reset <= not ocp_in.MReset_n;
  -- generate property instances for each property
  -- they are all combinatorial by design
  gen: for i in 0 to properties'right generate -- properties'left to 0 generate
    prop: entity ocpi.property_decoder
      generic map (properties(i), worker.decode_width)
      port map(reset        => my_reset,
               offset_in    => offset,
--               top          => top_bit(ocp_in),
               access_in    => my_access,
               data_in      => ocp_in.MData,
               write_enable => my_write_enables(i),
               read_enable  => my_read_enables(i),
               offset_out   => my_offsets(i),
               index_out    => indices(i)(worker.decode_width-1 downto 0),
               data_out     => data_outputs(i)(data_top(properties(i),"00") downto 0));
    indices(i)(indices(i)'left downto worker.decode_width) <= (others => '0');
    offsets(i) <= resize(my_offsets(i),offsets(i)'length); -- resize to 32 bits for VHDL language reasons
  end generate gen;
  
  write_enables <= my_write_enables;
  read_enables <= my_read_enables;
  control_op <= my_control_op;
  -- manage state during control ops and manage the WCI/OCP SResp.
  -- remember that since we have no SCmdAccept signal, any command is only
  -- valid for one clock, but is finished when we assert SResp non-NULL
  reg: process(ocp_in.Clk) is
    -- a mask of allowable operations in the current state
    variable allowed_ops : control_op_mask_t;
    variable next_op : control_op_t;
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
    if rising_edge(ocp_in.Clk) then
      -- default value of the SResp output, which is a register
      resp <= ocp.SResp_NULL;
      if ocp_in.MReset_n = '0' then
        is_operating <= bfalse;
        my_control_op <= NO_OP_e;
        if worker.allowed_ops(control_op_t'pos(initialize_e)) = '1' then
          my_state <= exists_e;
        else
          my_state <= initialized_e;
        end if;
        allowed_ops := next_ops(state_t'pos(my_state));
      elsif my_control_op /= NO_OP_e then
        if its(done) then                   -- FIXME done should also control config i/o
          -- finish the control by setting the state
          is_operating <= bfalse;
          case my_control_op is
            when INITIALIZE_e =>
              my_state <= initialized_e;
            when START_e =>
              my_state <= operating_e;
              is_operating <= btrue;
            when STOP_e =>
              my_state <= suspended_e;
            when RELEASE_e =>
              my_state <= unusable_e;
            when others => null;                            
          end case;
          resp <= ocp.SResp_DVA;
          my_control_op <= NO_OP_e;
        end if;
      else
        case my_access is
          when Error_e =>
            resp <= ocp.SResp_ERR;          -- we don't support read yet
          when Read_e =>
            if any_true(my_read_enables) then
              resp <= ocp.SResp_DVA;        -- assume there is no delay for property capture
            else
              resp <= ocp.SResp_ERR;        -- a write that no property accepted...
            end if;
          when Write_e =>
            if any_true(my_write_enables) then
              resp <= ocp.SResp_DVA;        -- assume there is no delay for property capture
            else
              resp <= ocp.SResp_ERR;        -- a write that no property accepted...
            end if;
          when Control_e =>
            if my_control_op /= no_op_e or                   -- prevented by control plane
               worker.allowed_ops(control_op_t'pos(control_op_in)) = '0' or -- prevented by software
               next_ops(state_t'pos(my_state))(control_op_t'pos(control_op_in)) = '0'
            then    -- prevented by software
              -- This would only happen if the control plane or software is broken
              resp <= ocp.SResp_ERR;
            else
              my_control_op <= control_op_in;
            end if;   
          when None_e => null;
        end case;
      end if;
    end if;
  end process;
end rtl;

--
-- The readback multiplexer for those properties that have readback
--
library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.all; use ocpi.types.all; use ocpi.wci.all;
entity readback is
  generic (properties : properties_t);
  port (
      read_enables : in bool_array_t(properties'range);
      data_inputs  : in data_a_t(properties'range);
      data_output  : out std_logic_vector(31 downto 0)
      );
end entity readback;

architecture rtl of readback is
  subtype index_t is natural range properties'range;
  function first_true(bools : bool_array_t(properties'range)) return index_t is
  begin
    for i in 0 to properties'right loop
      if its(bools(i)) then return i; end if;
    end loop;
    return 0;
  end first_true;
begin
  data_output <= data_inputs(first_true(read_enables));  
end rtl;
