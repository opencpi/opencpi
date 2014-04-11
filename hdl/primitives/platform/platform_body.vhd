-- VHDL component declarations for platform modules
library IEEE; use IEEE.std_logic_1164.all, IEEE.numeric_std.all;
library ocpi; use ocpi.all, ocpi.types.all, ocpi.util.all;
package body platform_pkg is

function to_slv(d : unoc_data_t) return std_logic_vector is
  variable r : std_logic_vector(unoc_ndw_c*32-1 downto 0);
  variable top : natural;
begin
  for i in 0 to unoc_ndw_c - 1 loop
    top := (unoc_ndw_c - i) * 32;
    r(top - 1 downto top - 32) := d.payload(i);
  end loop;
  return (0=>d.sof) & (0=>d.eof) & d.hit & d.be & r;
end to_slv;

function to_unoc(v : std_logic_vector) return unoc_data_t is
  variable d   : unoc_data_t;
  variable top : natural;
  variable n   : natural;
begin
  for i in 0 to unoc_ndw_c - 1 loop
    top := (unoc_ndw_c - i) * 32;
    d.payload(i) := v(top - 1 downto top - 32);
  end loop;
  n := v'left;
  d.sof := v(n);
  n := n - 1;
  d.eof := v(n);
  n := n - 1;
  d.hit := v(n downto n - d.hit'length + 1);
  n := n - d.hit'length;
  d.be := v(n downto n - d.be'length + 1);
  return d;
end to_unoc;

function to_slv(d : unoc_header_t) return std_logic_vector is
begin
  return
    "0" &
    (0=>d.has_data) &
    (0=>d.is_4dw) &
    d.pkt_type &
    "0" &
    d.class &
    "0000" &
    (0=>d.has_digest) &
    (0=>d.poisoned) &
    (0=>d.ordering) &
    (0=>d.no_snoop) &
    "00" &
    d.dw_length;
end to_slv;

function to_unoc_header(v : std_logic_vector) return unoc_header_t is
  variable d   : unoc_header_t;
  variable n   : natural := 31;
begin
  d.mbz0       := '0';
  n            := n - 1;
  d.has_data   := v(n);
  n            := n - 1;
  d.is_4dw     := v(n);
  n            := n - 1;
  d.pkt_type   := v(n downto n - d.pkt_type'length + 1);
  n            := n - d.pkt_type'length;
  d.mbz1       := '0';
  n            := n - 1;
  d.class      := v(n downto n - d.class'length + 1);
  n            := n - d.class'length;
  d.mbz2       := (others => '0');
  n            := n - d.mbz2'length;
  d.has_digest := v(n);
  n            := n - 1;
  d.poisoned   := v(n);
  n            := n - 1;
  d.ordering   := v(n);
  n            := n - 1;
  d.no_snoop   := v(n);
  n            := n - 1;
  d.mbz3       := (others => '0');
  n            := n - d.mbz3'length;
  d.dw_length  := v(n downto 0);
  return d;
end to_unoc_header;
function from_unoc_header(h : unoc_header_t) return std_logic_vector is
begin
  return
    slv(h.mbz0) & h.has_data & slv(h.is_4dw) & h.pkt_type & slv(h.mbz1) & h.class &
    h.mbz2 & slv(h.has_digest) & slv(h.poisoned) & slv(h.ordering) &
    slv(h.no_snoop) & h.mbz3 & h.dw_length;
end from_unoc_header;

function unoc_make_header(is_write      : bool_t;
                          is_completion : bool_t;
                          is_4dw        : bool_t;
                          dw_length     : unsigned(unoc_dw_length_bits_c downto 0))
  return unoc_header_t is
  variable d   : unoc_header_t;
begin
  d.mbz0       := '0';
  d.has_data   := is_write;
  d.is_4dw     := is_4dw;
  if its(is_completion) then
    d.pkt_type := std_logic_vector(to_unsigned(10, d.pkt_type'length));
  else
    d.pkt_type := std_logic_vector(to_unsigned(0, d.pkt_type'length));
  end if;
  d.mbz1       := '0';
  d.class      := (others => '0');
  d.mbz2       := (others => '0');
  d.has_digest := '0';
  d.poisoned   := '0';
  d.ordering   := '0'; -- strict ordering
  d.no_snoop   := '1'; -- no snooping
  d.mbz3       := (others => '0');
  if dw_length = to_unsigned(1024, dw_length'length) then
    d.dw_length  := (others => '0');
  else
    d.dw_length  := std_logic_vector(resize(dw_length, d.dw_length'length));
  end if;
  return d;
end unoc_make_header;

function to_unoc_request(v : std_logic_vector) return unoc_request_t is
  variable d   : unoc_request_t;
  variable n   : natural := 31;
begin
  d.sender_id  := v(n downto n - d.sender_id'length + 1);
  n            := n - d.sender_id'length;
  d.tag        := v(n downto n - d.tag'length + 1);
  n            := n - d.tag'length;
  d.last_be    := v(n downto n - d.last_be'length + 1);
  n            := n - d.last_be'length;
  d.first_be   := v(n downto n - d.first_be'length + 1);
  return d;
end to_unoc_request;

function unoc_make_response(dw_length : unsigned(unoc_dw_length_bits_c downto 0))
  return unoc_header_t is
begin
  return unoc_make_header(bfalse, btrue, bfalse, dw_length);
end unoc_make_response;
function from_unoc_completion1(h : unoc_completion1_t) return std_logic_vector is
begin
  return h.completer_id & h.status & slv(h.bcm) & h.byte_count;
end from_unoc_completion1;

function unoc_make_completion1(dw_length : unsigned(unoc_dw_length_bits_c downto 0))
  return unoc_completion1_t is
  variable d   : unoc_completion1_t;
begin
  d.completer_id := (others => '0'); -- we don't care
  d.status       := (others => '0'); -- success
  d.bcm          := '0';
  if dw_length = to_unsigned(1024, dw_length'length) then
    d.byte_count  := (others => '0');
  else
    d.byte_count  := std_logic_vector(dw_length(dw_length'left-1 downto 0)) & "00";
  end if;
  return d;
end unoc_make_completion1;
function from_unoc_completion2(h : unoc_completion2_t) return std_logic_vector is
begin
  return h.request_id & h.tag & slv(h.mbz0) & h.low_addr;
end from_unoc_completion2;
function unoc_make_completion2(requester_id : std_logic_vector(15 downto 0);
                               tag : std_logic_vector(7 downto 0);
                               addr : std_logic_vector(31 downto 0))
  return unoc_completion2_t is
  variable d   : unoc_completion2_t;
begin
  d.request_id := requester_id;
  d.tag        := tag;
  d.mbz0       := '0';
  d.low_addr   := addr(d.low_addr'left downto 0);
  return d;
end unoc_make_completion2;

end platform_pkg;
