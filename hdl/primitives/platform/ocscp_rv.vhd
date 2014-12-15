-- This module is the control plane infrastructure module for controlling workers
-- via their WCIs.  Thus this module is the WCI master.
-- FIXME: capture in the status register the last successful control op
-- andor whether there was a timeout or other error so we can accurately
-- know what state the worker is actually in!!!
-- FIXME: Reset should in fact reset more of the status bits...
-- FIXME: no way at all to reset "last op valid" etc.?

library IEEE; use IEEE.std_logic_1164.all, ieee.numeric_std.all;
library ocpi; use ocpi.all, ocpi.types.all, ocpi.util.all;
use work.platform_pkg.all;
entity ocscp_rv is
  generic(
    ocpi_debug  : bool_t := bfalse;
    ocpi_endian : endian_t := little_e;
    nWorkers    : ulong_t);
  port(
    wci_in  : in  wci.wci_s2m_array_t(0 to to_integer(nWorkers)-1);
    wci_out : out wci.wci_m2s_array_t(0 to to_integer(nWorkers)-1);
    cp_in   : in  occp_in_t;
    cp_out  : out occp_out_t);
end entity ocscp_rv;
architecture rtl of ocscp_rv is
  -- Types
  constant nWkrs : natural := to_integer(nWorkers);
  type worker_out_array_t is array (nWkrs-1 downto 0) of worker_out_t;
  -- Constants for the parameterized 
  constant id_width            : natural := width_for_max(nWkrs); -- to allow for sentinel
  constant worker_max_id       : unsigned(id_width-1 downto 0) := (others => '1');
  constant OCCP_ERROR_RESULT   : dword_t := X"c0de4202";
  constant OCCP_TIMEOUT_RESULT : dword_t := X"c0de4203";
  constant OCCP_RESET_RESULT   : dword_t := X"c0de4204";
  constant OCCP_SUCCESS_RESULT : dword_t := X"c0de4201";
  constant OCCP_FATAL_RESULT   : dword_t := X"c0de4205";
  constant OCCP_MAGIC_0        : dword_t := swap(from_string(to_string("Open",4),0,false));
  constant OCCP_MAGIC_1        : dword_t := swap(from_string(to_string("CPI",3),0,false));
  -- Combi values
  signal   is_admin            : boolean;
  signal   is_control          : boolean;
  signal   is_config           : boolean;
  signal   id                  : unsigned(id_width-1 downto 0);
  signal   id_bits             : unsigned(id_width-1 downto 0);
  signal   id_r                : unsigned(id_width-1 downto 0);
  signal   workers_out         : worker_in_t;
  signal   workers_in          : worker_out_array_t;
  signal   worker_in           : worker_out_t;
  signal   worker_in_timeout   : worker_timeout_t;
  signal   worker_timeout      : worker_timeout_t;
  signal   admin_data          : std_logic_vector(dword_t'range);
  signal   worker_data         : dword_t;
  signal   present             : word64_t;
  signal   attention           : word64_t;
  signal   admin_control       : dword_t;
  -- Our state
  signal   active_r            : bool_t; -- pipelined "cp_in.valid"
  signal   reading_r           : bool_t;
  signal   timeout_r           : unsigned(2**worker_timeout_t'length - 1 downto 0);
  signal   scratch20_r         : dword_t;
  signal   scratch24_r         : dword_t;
  signal   reset_r             : bool_t; -- master reset
  signal   big_endian_r        : bool_t; -- master endian
  signal   lowbits             : std_logic_vector(1 downto 0);
  signal   address             : unsigned(cp_in.address'range);
  signal   byte_en             : std_logic_vector(cp_in.byte_en'range);
  signal   admin_address       : std_logic_vector(7 downto 0);
  signal   magic0              : dword_t;
  signal   magic1              : dword_t;
  constant default_timeout_c   : natural := 4; -- log2 value
begin
  magic0 <= OCCP_MAGIC_0
            when ocpi_endian = little_e or (ocpi_endian = dynamic_e and not its(big_endian_r))
            else OCCP_MAGIC_1;
  magic1 <= OCCP_MAGIC_1
            when ocpi_endian = little_e or (ocpi_endian = dynamic_e and not its(big_endian_r))
            else OCCP_MAGIC_0;
  -- Ancient VHDL simplifications
  address       <= unsigned(cp_in.address);
  byte_en       <= cp_in.byte_en;
  admin_address <= cp_in.address(5 downto 0) & "00";
  -- Address decode
  is_admin   <= address(address'left downto worker_control_bits) = "0";
  -- Control includes control operations and control/status register access
  is_control <= not is_admin and
                address(address'left downto worker_config_bits) = "0";
  is_config  <= not is_admin and not is_control and
                (address(cp_in.address'left downto worker_config_bits) <= nWorkers);
  -- Low order address bit determination
  with byte_en select
    lowbits <= "01" when "0010",
               "10" when "0100" | "1100",
               "11" when "1000",
               "00" when others;
  -- Operation decode
  workers_out.operation <=
    control_op_e when is_control and cp_in.is_read and
                      (address(worker_control_bits-1 downto 0) < worker_ncontrol_ops) else
    control_read_e when is_control and cp_in.is_read else
    control_write_e when is_control else
    config_read_e when is_config and cp_in.is_read else
    config_write_e when is_config else
    none_e;
  id_bits <= unsigned(cp_in.address(worker_control_bits + id_width - 1 downto
                                    worker_control_bits))
             when is_control else
             unsigned(cp_in.address(worker_config_bits + id_width - 1 downto
                                    worker_config_bits));
  -- Decode worker id, carefully to make it easy for workers to be enabled by it
  id                    <= id_bits - 1 when is_control or is_config else worker_max_id;        
  admin_control         <= slv0(32-4) &
                           slvn(endian_t'pos(ocpi_endian), 2) &
                           slv(big_endian_r) &
                           slv(reset_r);
  worker_in             <= workers_in(0) when id_r = worker_max_id
                           else workers_in(to_integer(id_r));
  worker_in_timeout     <= worker_timeout_t(worker_in.data(worker_timeout_t'left downto 0));
  worker_timeout        <= worker_in_timeout
                           when worker_in_timeout /= 0 else
                           to_unsigned(default_timeout_c, worker_timeout'length);
  workers_out.clk       <= cp_in.clk;
  workers_out.reset     <= cp_in.reset or reset_r;
  workers_out.id        <= resize(id_r, workers_out.id'length);
  -- This is not per OCP protocol - it just the value the WCI master should use
  -- at the right time for this request, decoded here, once.
  workers_out.cmd       <= ocpi.ocp.MCmd_IDLE when cp_in.valid = '0' else
                           ocpi.ocp.MCmd_READ when cp_in.is_read = '1' else
                           ocpi.ocp.MCmd_WRITE;
  workers_out.address   <= cp_in.address(worker_config_bits-1 downto 0) & lowbits;
  workers_out.is_config <= to_bool(is_config);
  workers_out.byte_en   <= cp_in.byte_en;
  workers_out.data      <= cp_in.data;
  workers_out.is_big_endian <= big_endian_r;
  -- We don't allow the log timeout value to be 0, so 1 is always the timedout value
  workers_out.timedout  <= to_bool(timeout_r = 1); --to_unsigned(1, timeout_r'length));
  cp_out.data           <= admin_data when is_admin else worker_data;
  with admin_address select admin_data <=
    magic0                                   when x"00",
    magic1                                   when x"04",
    slvn(1,32)                               when x"08", -- version 1
    slv0(32)                                 when x"0C", -- old birthday - in metadata now
    present(31 downto 0)                     when x"10", -- old present bits - see later
    slv0(32)                                 when x"14", -- PCI device - pf worker now
    attention(31 downto 0)                   when x"18", -- old attention bits - see later
    slv0(32)                                 when x"1C", -- was tlp drops - see pf worker
    scratch20_r                              when x"20",
    scratch24_r                              when x"24",
    admin_control                            when x"28", -- was unused, now is master reset
    std_logic_vector(nWorkers)               when x"2C", -- now many workers configured?
    slvn(1,32)                               when x"7C",
    slv0(32)                                 when x"80",
    slv0(32)                                 when x"84",
    slv0(32)                                 when x"88",
    slv0(32)                                 when x"8C",
    -- new ones for now
    attention(31 downto 0)                   when x"50",
    attention(63 downto 32)                  when x"54",
    slv0(32)                                 when others;

  worker_data <=   
    OCCP_ERROR_RESULT   when worker_in.response = error_e else
    OCCP_TIMEOUT_RESULT when worker_in.response = timedout_e else
    OCCP_RESET_RESULT   when worker_in.response = reset_e else
    OCCP_SUCCESS_RESULT when worker_in.response = ok_e and
                             workers_out.operation = control_op_e else
    worker_in.data;
  cp_out.take  <= to_bool(active_r and (is_admin or worker_in.response /= none_e));
  cp_out.valid <= to_bool(reading_r and (is_admin or worker_in.response /= none_e));
  cp_out.tag   <= cp_in.data(cp_out.tag'left downto 0);

  gen0: for i in 0 to nWkrs - 1 generate
    present(i)   <= workers_in(i).present;
    attention(i) <= workers_in(i).attention;
    wm : component wci_master
      generic map(ocpi_debug => ocpi_debug,
                  id_width   => id_width,
                  id         => i)
      port map(
        wci_in     => wci_in(i),
        wci_out    => wci_out(i),
        worker_in  => workers_out,
        worker_out => workers_in(i)

        );
  end generate gen0;
  gen1: for i in nWkrs to worker_max_nworkers - 1 generate
   present(i) <= '0';
   attention(i) <= '1';
  end generate gen1;
  work : process(cp_in.clk)
  begin
    if rising_edge(cp_in.clk) then
      if its(cp_in.reset) then
        -- Core state
        active_r      <= '0';
        reading_r     <= '0';
        timeout_r     <= (others => '0');
        reset_r       <= '0'; -- master reset for all workers defaults OFF
        big_endian_r  <= '0';
        scratch20_r   <= (others => '0');
        scratch24_r   <= (others => '0');
        -- Debug state
      elsif not its(active_r) and cp_in.valid = '1' then
        id_r      <= id;
        active_r  <= '1';
        reading_r <= cp_in.is_read;
      elsif its(active_r) then
        if timeout_r = 0 then
          timeout_r <= to_unsigned(1, timeout_r'length) sll
                       to_integer(worker_timeout);
        elsif timeout_r /= 1 then
          timeout_r <= timeout_r - 1;
        end if;
        if is_admin and not its(reading_r) then
          -- Writable admin registers
          case admin_address is
            when x"20" => scratch20_r <= cp_in.data;
            when x"24" => scratch24_r <= cp_in.data;
            when x"28" =>
              reset_r      <= cp_in.data(0);
              big_endian_r <= cp_in.data(1);
            when others => null;
          end case;
        end if;
        if (is_admin or worker_in.response /= none_e) and
           (not its(reading_r) or cp_in.take) then
          active_r  <= '0';
          reading_r <= '0';
          timeout_r <= (others => '0');
          id_r      <= worker_max_id;
        end if;
      end if;
    end if;
  end process;
end rtl;
