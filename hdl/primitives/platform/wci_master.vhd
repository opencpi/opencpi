-- This module is the per-worker infrastructure module for controlling a worker
-- via its WCI.  Thus this module is the WCI master.

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.types.all; use ocpi.util.all;
use work.platform_pkg.all;
entity wci_master is
  generic(
    ocpi_debug   : bool_t;
    id_width, id : natural);
  port(
    -- worker-facing side - a WCI
    wci_in     : in  wci_s2m_t;
    wci_out    : out wci_m2s_t;
    worker_in  : in  worker_in_t;
    worker_out : out worker_out_t);
end entity  wci_master;
architecture rtl of wci_master is
  -- Combi values
  signal control              : std_logic_vector(31 downto 0); -- control register value
  signal status               : std_logic_vector(31 downto 0); -- status register value
  signal starting             : bool_t; -- We have decoded a request for us
  signal is_master            : bool_t; -- shorthand for operation = control_read|write_e
  signal response             : worker_response_t;               -- our response when active
  -- Our state - minimized for scalability
  signal reset_n_r            : std_logic;                     -- are we reset? (_n)
  signal window_r             : std_logic_vector(11 downto 0); -- high address bits
  signal attention_r          : std_logic;                     -- sticky version of SFlag(0)
  signal timeout_r            : worker_timeout_t;              -- our log2 of timeout ticks
  signal ready_r              : bool_t;                        -- WCI slave !busy (pipelined)
  signal active_r             : bool_t;                        -- We are performing a request
  signal abort_r              : std_logic;                     -- Drive MFlag(0)
  -- Error reporting - perhaps debug only
  signal last_addr_r          : std_logic_vector(worker_in.address'range); -- last config addr
  signal last_write_r         : std_logic;                     -- last op was a config write
  signal last_control_r       : std_logic_vector(2 downto 0);  -- last control op (read)
  signal last_byte_en_r       : std_logic_vector(3 downto 0);  -- last config byte enable
  signal last_write_valid_r   : std_logic;
  signal last_control_valid_r : std_logic;
  signal last_byte_en_valid_r : std_logic;
  signal last_addr_valid_r    : std_logic;
  signal sticky_r             : std_logic_vector(8 downto 0);
  signal assert_command       : bool_t;
  signal cmd_asserted_r       : bool_t;
begin
  -- worker is only asserted with a valid worker value when there is
  -- a valid operation
  is_master          <= to_bool(worker_in.operation = control_read_e or
                                worker_in.operation = control_write_e);
  starting           <= to_bool(worker_in.id(id_width-1 downto 0) = to_unsigned(id, id_width) and
                                (not its(active_r) and
                                 (its(ready_r) or reset_n_r = '0' or is_master)));
  -- the gate for the single cycle command
  assert_command     <= to_bool(active_r and its(ready_r) and
                                not its(is_master) and not its(cmd_asserted_r));
  wci_out.Clk        <= worker_in.clk;
  wci_out.MReset_n   <= reset_n_r;
  wci_out.MCmd       <= worker_in.cmd when its(assert_command) else ocpi.ocp.MCmd_IDLE;
  wci_out.MAddr      <= window_r & worker_in.address;
  wci_out.MAddrSpace(0) <= worker_in.is_config;
  wci_out.MByteEn    <= worker_in.byte_en;
  wci_out.MData      <= worker_in.data;
  wci_out.MFlag(0)   <= abort_r;
  wci_out.MFlag(1)   <= '0'; -- little endian
  control <= slv(reset_n_r) &            -- 31
             slv(abort_r) &              -- 30
             slv0(29-5+1) &              -- 29:5
             std_logic_vector(timeout_r);--  4:0
  status  <= slv0(31-28+1) &             -- 31:28
             slv(last_write_r) &         -- 27
             last_control_r &            -- 26:24
             last_byte_en_r &            -- 23:20
             slv(last_write_valid_r) &   -- 19
             slv(last_control_valid_r) & -- 18
             slv(last_byte_en_valid_r) & -- 17
             slv(last_addr_valid_r) &    -- 16
             slv0(15-11+1) &             -- 15:11
             slv(wci_in.SFlag(0)) &      -- 10
             slv(attention_r) &          --  9
             sticky_r;                   --  8:0

  worker_out.data    <=
    -- Give the master our timeout value when we are idle
    -- so they can grab it when we start something
    std_logic_vector(resize(timeout_r, worker_out.data'length))
                when not active_r else
    status      when is_master and worker_in.address(5 downto 2) = slvn(8,4) else
    control     when is_master and worker_in.address(5 downto 2) = slvn(9,4) else
    slv0(worker_out.data'length - last_addr_r'length) &
    last_addr_r when is_master and worker_in.address(5 downto 2) = slvn(10,4) else
    slv0(worker_out.data'length - window_r'length) &
    window_r    when is_master and worker_in.address(5 downto 2) = slvn(12,4) else
    wci_in.SData;
  -- Note that our response is not qualified by our being requested since
  -- the responses are selected from the proper worker in the master anyway
  response <=  ok_e       when its(is_master) else
               reset_e    when reset_n_r = '0' else
               ok_e       when wci_in.SResp = ocpi.ocp.SResp_DVA else
               error_e    when wci_in.SResp = ocpi.ocp.SResp_ERR or
                               wci_in.SResp = ocpi.ocp.SResp_FAIL else
               timedout_e when worker_in.timedout = '1' else
               none_e;
  worker_out.response   <= response when its(active_r) else none_e;
  worker_out.attention  <= wci_in.SFlag(0);
  worker_out.present    <= '1';
  work : process(worker_in.clk)
  begin
    if rising_edge(worker_in.clk) then
      if worker_in.reset = '1' then
        -- Core state
        reset_n_r            <= '0';
        window_r             <= (others => '0');
        attention_r          <= '0';
        timeout_r            <= (others => '0');
        ready_r              <= '0';
        active_r             <= bfalse;
        abort_r              <= '0';
        cmd_asserted_r       <= bfalse;
        -- Debug state
        last_addr_r          <= (others => '0');
        last_write_r         <= '0';
        last_control_r       <= (others => '0');
        last_byte_en_r       <= (others => '0');
        last_write_valid_r   <= '0';
        last_control_valid_r <= '0';
        last_byte_en_valid_r <= '0';
        last_addr_valid_r    <= '0';
        sticky_r             <= (others => '0');
      else
        ready_r              <= to_bool(wci_in.SThreadBusy(0) = '0'); -- pipelined per spec
        if its(starting) then
          active_r <= '1';
          -- Capture "last request" info upon startup
          case worker_in.operation is
            when config_write_e | config_read_e =>
              last_write_r         <= to_bool(worker_in.operation = config_write_e);
              last_addr_r          <= worker_in.address;
              last_byte_en_r       <= worker_in.byte_en;
              last_write_valid_r   <= '1';
              last_byte_en_valid_r <= '1';
              last_addr_valid_r    <= '1';
            when control_op_e   =>
              last_control_r       <= worker_in.address(4 downto 2);
              last_control_valid_r <= '1';
            when others =>
              null;
          end case;
        elsif its(active_r) then
          if response /= none_e then
            active_r <= bfalse;
            cmd_asserted_r <= bfalse;
          elsif its(assert_command) then
            cmd_asserted_r <= btrue;
          end if;
          if worker_in.operation = control_write_e then
            -- Write to one of 2 control registers
            case to_integer(unsigned(worker_in.address(5 downto 2))) is
              when 9 => -- control register
                reset_n_r <= worker_in.data(31);
                abort_r   <= worker_in.data(30);
                timeout_r <= resize(unsigned(worker_in.data),timeout_r'length);
                if worker_in.data(9) = '1' then
                  attention_r <= '0';
                end if;
                if worker_in.data(8) = '1' then
                  sticky_r <= (others => '0');
                end if;
              when 12 => -- window register
                window_r <= worker_in.data(window_r'range);
              when others => null;
            end case;
          elsif worker_in.operation /= control_read_e and reset_n_r = '1' then
            -- Note that control_read as well as
            -- the "worker-is-reset" case of worker accesses take care of themselves
            -- due to combi muxing on output data and responses (above)
            -- So we just doing config read/write and control op here
            -- 
            -- Capture bad things in sticky bits
            if wci_in.SResp = ocpi.ocp.SResp_FAIL then
              case worker_in.operation is
                when config_write_e => sticky_r(5) <= '1';
                when config_read_e  => sticky_r(4) <= '1';
                when control_op_e   => sticky_r(3) <= '1';
                when others         => null;
              end case;
            elsif wci_in.SResp = ocpi.ocp.SResp_ERR then
              case worker_in.operation is
                when config_write_e => sticky_r(2) <= '1';
                when config_read_e  => sticky_r(1) <= '1';
                when control_op_e   => sticky_r(0) <= '1';
                when others         => null;
              end case;
            end if;
          elsif its(worker_in.timedout) then
            case worker_in.operation is
              when config_write_e => sticky_r(8) <= '1';
              when config_read_e  => sticky_r(7) <= '1';
              when control_op_e   => sticky_r(6) <= '1';
              when others         => null;
            end case;
          end if;
        end if;
      end if;
    end if;
  end process;
end rtl;
