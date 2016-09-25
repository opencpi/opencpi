-- per-property decoder - purely combinatorial
-- result is write_enable, offset-in-array, and aligned data output
-- output data has full 32 bits when sub32 arrays are being written
library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.all, ocpi.types.all, ocpi.wci.all, ocpi.util.all;

entity property_decoder is
  generic (
        property      : property_t;   -- property type
        decode_width  : natural;     -- decoder width in bits
        endian        : endian_t);
  port (
        reset         : in  bool_t;                            -- active-low WCI worker reset
        offset_in     : in  unsigned(decode_width-1 downto 0); -- offset in Bytes
        nbytes_1      : in  byte_offset_t;                     -- how many valid bytes
        is_read       : in  bool_t;
        is_write      : in  bool_t;
        data_in       : in  std_logic_vector(31 downto 0);     -- WCI master data
        is_big_endian : in bool_t;
        write_enable  : out bool_t;                            -- active-high write pulse
        read_enable   : out bool_t;                            -- active-high read pulse
        offset_out    : out unsigned(width_for_max(property.bytes_1)-1 downto 0);
--        index_out     : out unsigned(decode_width-1 downto 0);
        data_out      : out std_logic_vector(31 downto 0));
end entity property_decoder;

architecture rtl of property_decoder is
  subtype decode_t is unsigned (decode_width-1 downto 0);
  signal my_offset : decode_t;
  signal byte_offset : byte_offset_t;
  signal little : boolean;
  signal my_decode : bool_t;
  function element_bytes(property : property_t) return decode_t is
    variable bytes : natural;
  begin
    if property.string_length /= 0 then
      bytes := ((property.string_length + 4)/4) * 4;
    else
      bytes := (property.data_width+7)/8;
    end if;
    return to_unsigned(bytes, decode_t'length);
  end element_bytes;
  begin
  my_decode    <= to_bool(offset_in >= property.offset and
                          offset_in <= property.offset(decode_t'range) + property.bytes_1);
  my_offset    <= offset_in - property.offset(decode_width-1 downto 0);
--                  when its(my_decode) else (others => '0');
  byte_offset  <= offset_in(1 downto 0);
  little       <= endian = little_e or (endian = dynamic_e and not its(is_big_endian));
  l32: if property.data_width >= 32 generate
   data_out <= data_in;
  end generate;
  l16: if property.data_width = 16 generate
    data_out(31 downto 16) <= data_in(15 downto 0) when not little and byte_offset = 2 else
                              data_in(31 downto 16);
                              
    data_out(15 downto 0) <= data_in(31 downto 16) when little and byte_offset = 2 else
                             data_in(15 downto 0);
  end generate;
  l8: if property.data_width = 8 generate
   data_out(31 downto 24) <= data_in(23 downto 16) when not little and byte_offset = 1 else
                             data_in(15 downto  8) when not little and byte_offset = 2 else
                             data_in( 7 downto  0) when not little and byte_offset = 3 else
                             data_in(31 downto 24);
   data_out(23 downto 8) <=  data_in(23 downto 8);
   data_out(7 downto 0)   <= data_in(31 downto 24) when little and byte_offset = 3 else
                             data_in(23 downto 16) when little and byte_offset = 2 else
                             data_in(15 downto  8) when little and byte_offset = 1 else
                             data_in( 7 downto  0);
  end generate;
  -- never on strings so no non-power of 2 math needed.
--  index_out    <= my_offset/element_bytes(property) when its(my_decode) else (others => '0');
--  index_out    <= my_offset srl width_for_max(property.data_width-1);
  offset_out   <= my_offset(offset_out'range);
  write_enable <= to_bool(is_write and property.writable and my_decode);
  read_enable  <= to_bool(is_read and property.readable and my_decode);
end rtl;
