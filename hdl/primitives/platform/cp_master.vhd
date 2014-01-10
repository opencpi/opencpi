-- This module is the per-worker infrastructure module for controlling a worker
-- via its WCI.  Thus this module is the WCI master.

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.types.all, ocpi.util.all;
use work.platform_pkg.all;
entity cp_master is
  generic(
    debug      : boolean;
    nWCIs      : natural);
  port(
    wci_in     : in  wci_s2m_array_t(nWCIs downto 0);
    wci_out    : out wci_m2s_array_t(nWCIs downto 0);
    master_in  : in  occp_in_t;
    master_out : out occp_out_t);
end entity  cp_master;
architecture rtl of cp_master is
  -- Types
  type worker_out_array_t is array (nWCIs-1 downto 0) of worker_out_t;
  -- Constants
  constant max_id              : natural := word64_t'length - 1;
  constant id_width            : natural := width_for_max(nWCIs);
  constant worker_max_id       : unsigned(id_width-1 downto 0) := (others => '1');
  constant OCCP_ERROR_RESULT   : word_t := X"c0de4202";
  constant OCCP_TIMEOUT_RESULT : word_t := X"c0de4203";
  constant OCCP_RESET_RESULT   : word_t := X"c0de4204";
  constant OCCP_SUCCESS_RESULT : word_t := X"c0de4201";
  constant OCCP_FATAL_RESULT   : word_t := X"c0de4205";
  constant OCCP_MAGIC_0        : word_t := from_string(to_string("Open",4));
  constant OCCP_MAGIC_1        : word_t := from_string(to_string("CPI",4));
  -- Combi values
  signal   is_admin            : boolean;
  signal   is_control          : boolean;
  signal   is_config           : boolean;
  signal   id                  : unsigned(id_width-1 downto 0);
  signal   workers_out         : worker_in_t;
  signal   workers_in          : worker_out_array_t;
  signal   worker_in           : worker_out_t;
  signal   admin_data          : std_logic_vector(word_t'range);
  signal   worker_data         : word_t;
  signal   present             : word64_t;
  signal   attention           : word64_t;
  signal   control             : word_t;
  -- Our state
  signal   active_r            : bool_t; -- pipelined "master_in.valid"
  signal   timeout_r           : unsigned(2**worker_timeout_t'length - 1 downto 0);
  signal   scratch_r           : word64_t;
  signal   reset_r             : bool_t; -- master reset
  -- Error reporting - perhaps debug only
begin
  -- Address decode
  is_admin   <= master_in.address(master_in.address'left downto worker_control_bits) =
                slv0(master_in.address'length - worker_control_bits);
  -- Control includes control operations and control/status register access
  is_control <= master_in.address(master_in.address'left downto worker_config_bits) =
                slv0(master_in.address'length - worker_config_bits);
  is_config  <= unsigned(master_in.address(master_in.address'left downto
                                           worker_config_bits)) <=
                to_unsigned(nWCIs, master_in.address'length - worker_config_bits);
  -- Ensure the worker id
  id <= worker_max_id when master_in.valid = '0' or is_admin else
        unsigned(master_in.address(worker_control_bits + id_width - 1 downto
                                   worker_control_bits))
        when is_control else
        unsigned(master_in.address(worker_config_bits + id_width - 1 downto
                                   worker_config_bits))
        when is_config else worker_max_id;

  control               <= (0 => reset_r, others => '0');
  worker_in             <= workers_in(0) when id = worker_max_id
                           else workers_in(to_integer(id));
  workers_out.clk       <= master_in.clk;
  workers_out.reset     <= master_in.reset or reset_r;
  workers_out.id        <= id;
  -- This is not per OCP protocol - it just the value the WCI master should use
  -- at the right time for this request, decoded here, once.
  workers_out.cmd       <= ocpi.ocp.MCmd_IDLE when master_in.valid = '0' else
                           ocpi.ocp.MCmd_READ when master_in.is_read = '1' else
                           ocpi.ocp.MCmd_WRITE;
  workers_out.address   <= master_in.address(worker_config_bits-1 downto 0);
  workers_out.is_config <= to_bool(is_config);
  workers_out.byte_en   <= master_in.byte_en;
  workers_out.data      <= master_in.data;
  workers_out.operation <= none_e;
  workers_out.timedout  <= to_bool(timeout_r = to_unsigned(0, timeout_r'length));
  master_out.data       <= admin_data when is_admin else worker_data;
  with to_integer(unsigned(master_in.address(5 downto 2))) select admin_data <=
    OCCP_MAGIC_0                             when 0,
    OCCP_MAGIC_1                             when 1,
    slv0(32)                                 when 2, -- version
    std_logic_vector(to_unsigned(nWCIs, 32)) when 3,
    control                                  when 4,
    slv0(32)                                 when 5, -- pad to align others to 64 bits
    present(31 downto 0)                     when 6,
    present(63 downto 32)                    when 7,
    attention(31 downto 0)                   when 8,
    attention(63 downto 32)                  when 9,
    scratch_r(31 downto 0)                   when 10,
    scratch_r(63 downto 32)                  when 11,
    slv0(32)                                 when others;

  with worker_in.response select worker_data <=   
    OCCP_ERROR_RESULT   when error_e,
    OCCP_TIMEOUT_RESULT when timedout_e,
    OCCP_RESET_RESULT   when reset_e,
    OCCP_SUCCESS_RESULT when ok_e,
    worker_in.data      when others;
  master_out.take  <= '1' when worker_in.response /= none_e else '0';
  master_out.valid <= '1' when worker_in.response /= none_e and master_in.is_read else '0';
  master_out.tag   <= master_in.data(master_out.tag'left downto 0);

  gen0: for i in 0 to nWCIs - 1 generate
    present(i)   <= workers_in(i).present;
    attention(i) <= workers_in(i).attention;
    wm : entity work.wci_master
      generic map(debug => debug,
                  id_width => id_width,
                  id      => i)
      port map(
        wci_in     => wci_in(i),
        wci_out    => wci_out(i),
        worker_in  => workers_out,
        worker_out => workers_in(i)

        );
  end generate gen0;
  gen1: for i in nWCIs to max_id - 1 generate
   present(i) <= '0';
   attention(i) <= '1';
  end generate gen1;
  work : process(master_in.clk)
  begin
    if rising_edge(master_in.clk) then
      if master_in.reset = '1' then
        -- Core state
        active_r  <= '0';
        timeout_r <= (others => '0');
        reset_r   <= '0'; -- master reset for all workers defaults OFF
        -- Debug state
      elsif not its(active_r) and master_in.valid = '1' then
        -- capture WCI master's timeout: value is: 1 << timeout_value
        timeout_r <= to_unsigned(1, timeout_r'length) sll
                     to_integer(unsigned(worker_in.data(worker_timeout_t'left downto 0)));
        active_r <= '1';
      elsif its(active_r) then
        if timeout_r /= to_unsigned(0, timeout_r'length) then
          timeout_r <= timeout_r - 1;
        end if;
        if worker_in.response /= none_e then
          active_r <= '0';
        end if;
      end if;
    end if;
  end process;
end rtl;
