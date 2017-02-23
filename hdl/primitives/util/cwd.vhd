library ieee; use ieee.std_logic_1164.all, ieee.numeric_std.all;
library ocpi; use ocpi.types.all;
entity cwd is
   generic(length     : natural := work.util.cwd_length);
   port   (cwd        : out ocpi.types.string_t(0 to length));
end entity cwd;
architecture rtl of cwd is
  signal cwd_internal : std_logic_vector(0 to length*8-1);
  signal length_err   : std_logic;
  function slv2string(input : std_logic_vector) return string_t is
    variable result: string_t(0 to length); -- room for null
    variable n: natural := 0;
    variable i: natural := 0;
  begin
    -- skip null chars at the "left" side of the plusarg slv
    while n < length loop
      if unsigned(input(n*8 to n*8+7)) /= 0 then
        exit;
      end if;
      n := n + 1;
    end loop;
    -- copy remaining chars that are right justified to result
    while n < length loop
      result(i) := signed(input(n*8 to n*8+7));
      n := n + 1;
      i := i + 1;
    end loop;
    -- fill the rest of the result with null chars
    while i <= length loop          -- <= because result has extra room for the null
      result(i) := (others => '0');
      i := i + 1;
    end loop;
    return result;
  end slv2string;
begin
  cwd      <= slv2string(cwd_internal); -- convert from right-justified plusarg format
  cwd_i: component work.util.cwd_internal -- instance the verilog that grabs the plusarg
    generic map(length => length)
    port    map(cwd    => cwd_internal,
                len_err    => length_err);

  -- if length_err goes high, the CWD_MAX_LENGTH param is too small - fail.
  process (length_err) is begin
    if length_err = '1' then
     report "CWD buffer length too small in cwd_internal.v - increase CWD_MAX_LENGTH" severity failure;
    end if;
  end process;

end rtl;
