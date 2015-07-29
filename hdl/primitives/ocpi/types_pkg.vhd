library ieee; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;  use ieee.math_real;
package types is
--
-- Miscellaneous type declarations not related to OpenCPI data types
--
constant dword_size : natural := 32;
subtype dword_t   is std_logic_vector(31 downto 0);
type    dword_array_t is array (natural range <>) of dword_t;
subtype word8_t  is std_logic_vector(7 downto 0);
subtype word16_t is std_logic_vector(15 downto 0);
subtype word32_t is std_logic_vector(31 downto 0);
subtype word64_t is std_logic_vector(63 downto 0);
subtype byte_offset_t is unsigned(1 downto 0);
subtype bit_offset_t is natural range 0 to 31; -- bit within word
type    endian_t is (little_e, big_e, dynamic_e);



function bit2vec(b : std_logic; n : natural) return std_logic_vector;

-- These types are the mapping of the OpenCPI scalar types to VHDL.
-- We use std_logic vector types and avoid native types.
-- These ranges match the IDL specification
--
-- boolean type, convertible to/from vhdl native boolean
--

-- THESE ARE DEFINITIONS WHEN Bool_t is BOOLEAN
--subtype Bool_t is boolean;

-- THESE ARE DEFINITIONS WHEN Bool_t is std_logic
subtype Bool_t is std_logic;
function "and"  ( l : bool_t; r : bool_t ) return boolean;
function "nand" ( l : bool_t; r : bool_t ) return boolean;
function "or"   ( l : bool_t; r : bool_t ) return boolean;
function "nor"  ( l : bool_t; r : bool_t ) return boolean;
function "xor"  ( l : bool_t; r : bool_t ) return boolean;
function "xnor" ( l : bool_t; r : bool_t ) return boolean;

function "and"  ( l : bool_t; r : boolean ) return boolean;
function "nand" ( l : bool_t; r : boolean ) return boolean;
function "or"   ( l : bool_t; r : boolean ) return boolean;
function "nor"  ( l : bool_t; r : boolean ) return boolean;
function "xor"  ( l : bool_t; r : boolean ) return boolean;
function "xnor" ( l : bool_t; r : boolean ) return boolean;

function "and"  ( l : boolean; r : bool_t ) return boolean;
function "nand" ( l : boolean; r : bool_t ) return boolean;
function "or"   ( l : boolean; r : bool_t ) return boolean;
function "nor"  ( l : boolean; r : bool_t ) return boolean;
function "xor"  ( l : boolean; r : bool_t ) return boolean;
function "xnor" ( l : boolean; r : bool_t ) return boolean;

function "not"  ( l : bool_t             ) return boolean;

-- THESE ARE Bool_t related definitions independent of whether bool_t is boolean or std_logic
type bool_array_t is array (natural range <>) of bool_t;
function To_boolean (b : Bool_t) return boolean;
function To_bool(b : std_logic) return Bool_t;
function To_bool(b : std_logic_vector) return Bool_t;
function To_bool(b : boolean) return Bool_t;
function from_bool(b : bool_t) return std_logic_vector;
function slv(a: bool_array_t) return std_logic_vector;
function to_slv(a: bool_array_t) return std_logic_vector;
function to_bool_array(a: std_logic_vector) return bool_array_t;
function btrue return bool_t;
function bfalse return bool_t;
function its(b : bool_t) return boolean;

--
-- char type, convertible to/from vhdl native character, and integer (due to numeric_std)
--
subtype char_t is signed (7 downto 0);
type char_array_t is array (natural range <>) of char_t;
constant char_min : char_t := to_signed(-128,8);
constant char_max : char_t := to_signed(127,8);
function To_character (c : Char_t) return character;
function To_char (c: Character) return char_t;
function To_char (c: integer) return char_t;
function from_char (c: char_t) return std_logic_vector;
--function slv(a: char_t) return std_logic_vector;
function slv(a: char_array_t) return std_logic_vector;
function to_slv(a: char_array_t) return std_logic_vector;
function to_char_array(a: std_logic_vector) return char_array_t;
--
-- double type
--
subtype double_t is std_logic_vector (63 downto 0);
type double_array_t is array (natural range <>) of double_t;
constant double_min : double_t := x"0010_0000_0000_0000"; -- 2.2250738585072014e-308
constant double_max : double_t := x"7fef_ffff_ffff_ffff"; -- 1.7976931348623157e+308
function to_double(r: real) return double_t;
function from_double(d: double_t) return real;
function from_double(d: double_t) return std_logic_vector;
--function slv(a: double_t) return std_logic_vector;
function slv(a: double_array_t) return std_logic_vector;
function to_slv(a: double_array_t) return std_logic_vector;
function to_double_array(a: std_logic_vector) return double_array_t;
--
-- float type
--
subtype float_t is std_logic_vector (31 downto 0);
type float_array_t is array (natural range <>) of float_t;
constant float_min : float_t := x"0080_0000"; -- 1.17549435e-38
constant float_max : float_t := x"7f7f_ffff"; -- 3.40282347e+38
function to_float(r: real) return float_t;
function from_float(f: float_t) return real;
function from_float(f: float_t) return std_logic_vector;
function from_float(f: float_t) return natural;
--function slv(a: float_t) return std_logic_vector;
function slv(a: float_array_t) return std_logic_vector;
function to_slv(a: float_array_t) return std_logic_vector;
function to_float_array(a: std_logic_vector) return float_array_t;
--
-- short type - convertible to/from vhdl native integer
--
subtype short_t is signed (15 downto 0);
type short_array_t is array (natural range <>) of short_t;
constant short_min : short_t := x"8000";
constant short_max : short_t := x"7fff";
function To_short (c: integer) return short_t;
function from_short (c: short_t) return std_logic_vector;
--function slv(a: short_t) return std_logic_vector;
function slv(a: short_array_t) return std_logic_vector;
function to_slv(a: short_array_t) return std_logic_vector;
function to_short_array(a: std_logic_vector) return short_array_t;
--
-- long type - convertible to/from vhdl native integer
--
subtype long_t is signed (31 downto 0);
type long_array_t is array (natural range <>) of long_t;
constant long_min : long_t := x"8000_0000";
constant long_max : long_t := x"7fff_ffff";
function To_long (c: integer) return long_t;
function from_long (c: long_t) return std_logic_vector;
--function slv(a: long_t) return std_logic_vector;
function slv(a: long_array_t) return std_logic_vector;
function to_slv(a: long_array_t) return std_logic_vector;
function to_long_array(a: std_logic_vector) return long_array_t;
--
-- uchar type - convertible to/from vhdl native natural
--
subtype uchar_t is unsigned (7 downto 0);
type uchar_array_t is array (natural range <>) of uchar_t;
constant uchar_max : uchar_t := to_unsigned(255, 8);
function To_uchar (c: natural) return uchar_t;
function To_uchar (c: std_logic_vector(uchar_t'range)) return uchar_t;
function from_uchar (c: uchar_t) return std_logic_vector;
function slv(a: uchar_array_t) return std_logic_vector;
function to_slv(a: uchar_array_t) return std_logic_vector;
function to_uchar_array(a: std_logic_vector) return uchar_array_t;
--function slv(a: uchar_t) return std_logic_vector;
--
-- ulong type - convertible to/from vhdl native natural
--
subtype ulong_t is unsigned (31 downto 0);
type ulong_array_t is array (natural range <>) of ulong_t;
constant ulong_max : ulong_t := x"ffff_ffff";
function To_ulong (c: natural) return ulong_t;
function To_ulong (c: std_logic_vector(31 downto 0)) return ulong_t;
function from_ulong (c: ulong_t) return std_logic_vector;
--function slv(a: ulong_t) return std_logic_vector;
function slv(a: ulong_array_t) return std_logic_vector;
function to_slv(a: ulong_array_t) return std_logic_vector;
function to_ulong_array(a: std_logic_vector) return ulong_array_t;
--
-- ushort type - convertible to/from vhdl native natural
--
subtype ushort_t is unsigned (15 downto 0);
type ushort_array_t is array (natural range <>) of ushort_t;
constant ushort_max : ushort_t := x"ffff";
function To_ushort (c: natural) return ushort_t;
function from_ushort (c: ushort_t) return std_logic_vector;
--function slv(a: ushort_t) return std_logic_vector;
function slv(a: ushort_array_t) return std_logic_vector;
function to_slv(a: ushort_array_t) return std_logic_vector;
function to_ushort_array(a: std_logic_vector) return ushort_array_t;
--
-- longlong type - convertible to/from vhdl native integer (perhaps)
--
subtype longlong_t is signed (63 downto 0);
type longlong_array_t is array (natural range <>) of longlong_t;
constant longlong_min : longlong_t := x"8000_0000_0000_0000";
constant longlong_max : longlong_t := x"7fff_ffff_ffff_ffff";
-- FIXME: we have no guarantee that VHDL will support the right number of bits...
function To_longlong (c: integer) return longlong_t;
function To_longlong (c: long_t) return longlong_t;
function To_longlong (c: std_logic_vector(longlong_t'range)) return longlong_t;
function from_longlong (c: longlong_t) return std_logic_vector;
--function slv(a: longlong_t) return std_logic_vector;
function slv(a: longlong_array_t) return std_logic_vector;
function to_slv(a: longlong_array_t) return std_logic_vector;
function to_longlong_array(a: std_logic_vector) return longlong_array_t;
--
-- ulonglong type - convertible to/from vhdl native natural
--
subtype ulonglong_t is unsigned (63 downto 0);
type ulonglong_array_t is array (natural range <>) of ulonglong_t;
constant ulonglong_max : ulonglong_t := x"ffff_ffff_ffff_ffff";
function To_ulonglong (c: natural) return ulonglong_t;
function To_ulonglong (c: ulong_t) return ulonglong_t;
function To_ulonglong (c: std_logic_vector(ulonglong_t'range)) return ulonglong_t;
function from_ulonglong (c: ulonglong_t) return std_logic_vector;
--function slv(a: ulonglong_t) return std_logic_vector;
function slv(a: ulonglong_array_t) return std_logic_vector;
function to_slv(a: ulonglong_array_t) return std_logic_vector;
function to_ulonglong_array(a: std_logic_vector) return ulonglong_array_t;
--
-- string type - array of char
--
type string_t is array (natural range <>) of char_t;
type string_array_t is array (natural range <>, natural range <>) of char_t;
subtype wordstring_t is string_t(0 to 3);
-- Convert a VHDL string to our string type.
function to_string(instring : string; length : natural) return string_t;
function to_string_dw(inword : dword_t) return wordstring_t;
function to_string(v : std_logic_vector) return string_t;
function from_string(s : string_t; offset : unsigned; is_big_endian : bool_t) return dword_t;
function from_string(s : string_t; off : natural; is_big_endian : boolean) return dword_t;
function from_string(s : string_t) return std_logic_vector;
function from_string(s : string_t) return string;
function slv(a : string_t) return std_logic_vector;
function to_slv(a: string_array_t) return std_logic_vector;
function to_string_array(a: std_logic_vector; length : natural) return string_array_t;

function from_bool_array(ba : bool_array_t;
                         index, nbytes_1, byte_offset : unsigned;
                         is_big_endian : boolean) return dword_t;

end package types;
