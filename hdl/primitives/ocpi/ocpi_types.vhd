library ieee; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;  use ieee.math_real;
package types is
--
-- Miscellaneous type declarations not related to OpenCPI data types
--
subtype word_t is std_logic_vector(31 downto 0);
subtype byte_offset_t is unsigned(1 downto 0);
subtype bit_offset_t is natural range 0 to 31; -- bit within word

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

--function "or"   ( l : bool_t; r : boolean ) return bool_t;


function "not"  ( l : bool_t             ) return boolean;

-- THESE ARE Bool_t related definitions independent of whether bool_t is boolean or std_logic
type bool_array_t is array (natural range <>) of bool_t;
function To_boolean (b : Bool_t) return boolean;
function To_bool(b : std_logic) return Bool_t;
function To_bool(b : std_logic_vector) return Bool_t;
function To_bool(b : boolean) return Bool_t;
function from_bool(b : bool_t) return std_logic_vector;
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
--
-- double type - no VHDL conversions defined
--
subtype double_t is std_logic_vector (63 downto 0);
type double_array_t is array (natural range <>) of double_t;
constant double_min : double_t := x"0010_0000_0000_0000"; -- 2.2250738585072014e-308
constant double_max : double_t := x"7fef_ffff_ffff_ffff"; -- 1.7976931348623157e+308
--
-- float type - no VHDL conversions defined
--
subtype float_t is std_logic_vector (31 downto 0);
type float_array_t is array (natural range <>) of float_t;
constant float_min : float_t := x"0080_0000"; -- 1.17549435e-38
constant float_max : float_t := x"7f7f_ffff"; -- 3.40282347e+38
--
-- short type - convertible to/from vhdl native integer
--
subtype short_t is signed (15 downto 0);
type short_array_t is array (natural range <>) of short_t;
constant short_min : short_t := x"8000";
constant short_max : short_t := x"7fff";
function To_short (c: integer) return short_t;
--
-- long type - convertible to/from vhdl native integer
--
subtype long_t is signed (31 downto 0);
type long_array_t is array (natural range <>) of long_t;
constant long_min : long_t := x"8000_0000";
constant long_max : long_t := x"7fff_ffff";
function To_long (c: integer) return long_t;
--
-- uchar type - convertible to/from vhdl native natural
--
subtype uchar_t is unsigned (7 downto 0);
type uchar_array_t is array (natural range <>) of uchar_t;
constant uchar_max : uchar_t := to_unsigned(255, 8);
function To_uchar (c: natural) return uchar_t;
--
-- ulong type - convertible to/from vhdl native natural
--
subtype ulong_t is unsigned (31 downto 0);
type ulong_array_t is array (natural range <>) of ulong_t;
constant ulong_max : ulong_t := x"ffff_ffff";
function To_ulong (c: natural) return ulong_t;
--
-- ushort type - convertible to/from vhdl native natural
--
subtype ushort_t is unsigned (15 downto 0);
type ushort_array_t is array (natural range <>) of ushort_t;
constant ushort_max : ushort_t := x"ffff";
function To_ushort (c: natural) return ushort_t;
--
-- longlong type - convertible to/from vhdl native integer (perhaps)
--
subtype longlong_t is signed (63 downto 0);
type longlong_array_t is array (natural range <>) of longlong_t;
constant longlong_min : longlong_t := x"8000_0000_0000_0000";
constant longlong_max : longlong_t := x"7fff_ffff_ffff_ffff";
--
-- ulong type - convertible to/from vhdl native natural
--
subtype ulonglong_t is unsigned (63 downto 0);
type ulonglong_array_t is array (natural range <>) of ulonglong_t;
constant ulonglong_max : ulonglong_t := x"ffff_ffff_ffff_ffff";
--
-- string type - array of char
--
type string_t is array (natural range <>) of char_t;
type string_array_t is array (natural range <>,natural range <>) of char_t;
subtype wordstring_t is string_t(0 to 3);
function to_string(inword : word_t) return wordstring_t;
function from_string(s : string_t; offset : unsigned) return word_t; --std_logic_vector;


function from_bool_array(ba : bool_array_t; index, nbytes_1, byte_offset : unsigned) return word_t;

end package types;
