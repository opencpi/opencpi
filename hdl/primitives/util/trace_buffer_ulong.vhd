library ieee; use ieee.std_logic_1164.all, ieee.numeric_std.all;
library ocpi, util; use ocpi.types.all, ocpi.wci.all, ocpi.util.all;
entity trace_buffer_ulong is
  generic (depth : ulong_t); -- expect parameter value
  port  (clk : in std_logic;
         reset : in bool_t;
         input : in std_logic_vector(ulong_t'range);  -- input values
         give  : in bool_t;   -- worker is giving new value this clock cycle
         rawin : in raw_in_t; -- wire to raw interface
         rawout : out raw_out_t); -- wire to raw interface
end entity trace_buffer_ulong;
architecture rtl of trace_buffer_ulong is
  constant depth_c : natural := to_integer(depth);
  constant width_c : natural := ocpi.util.width_for_max(depth_c-1);
  signal bram_addr : unsigned(width_c-1 downto 0);
  signal bram_data : ulong_t;
  signal count_r   : ulong_t;
  signal done_r    : bool_t; -- pipelined read acknowledgement
begin
  bram : component util.util.BRAM2
    generic map(PIPELINED  => 0,
                ADDR_WIDTH => width_c,
                DATA_WIDTH => ulong_t'length,
                MEMSIZE    => depth_c)
    port map   (CLKA       => clk,
                ENA        => '1',
                WEA        => '0',
                ADDRA      => std_logic_vector(bram_addr),
                DIA        => slv0(dword_size),
                to_ulong(DOA) => bram_data,
                CLKB       => clk,
                ENB        => '1',
                WEB        => give,
                ADDRB      => std_logic_vector(count_r(width_c-1 downto 0)),
                DIB        => std_logic_vector(input),
                DOB        => open);
  bram_addr <= resize((rawin.address - 4) srl 2, bram_addr'length);
  rawout.data <= dword_t(count_r) when rawin.address = 0 else dword_t(bram_data);
  rawout.done <= done_r;
  rawout.error <= bfalse;
  process(clk) is
  begin
    if rising_edge(Clk) then
      if its(reset) then
        count_r <= (others => '0');
        done_r  <= bfalse;
      else
        if rawin.is_read and not its(done_r) then
          done_r <= btrue;
        else
          done_r <= bfalse;
        end if;
        if give and count_r < depth - 1 then
          count_r <= count_r + 1;
        end if;
      end if;
    end if;
  end process;  
end rtl;
