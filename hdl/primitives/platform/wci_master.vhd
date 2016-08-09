-- This module is the per-worker infrastructure module for controlling a worker
-- via its WCI.  Thus this module is the WCI master.

-- FIXME: ensure that the reset has been asserted for 16 clocks
library IEEE; use IEEE.std_logic_1164.all, ieee.numeric_std.all;
library ocpi; use ocpi.all, ocpi.types.all, ocpi.util.all;
use work.platform_pkg.all;
entity wci_master is
  generic(
    ocpi_debug   : bool_t;
    id_width, id : natural);
  port(
    -- worker-facing side - a WCI
    wci_in     : in  wci.wci_s2m_t;
    wci_out    : out wci.wci_m2s_t;
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
--  signal assert_command       : bool_t;
  signal cmd_asserted_r       : bool_t;
  -- Scalability signals
  signal waiting              : std_logic;                     -- worker is waiting at barrier
  signal barrier_r            : std_logic;                     -- barrier in progress or next
  signal crew_r               : std_logic_vector(7 downto 0);
  signal rank_r               : std_logic_vector(7 downto 0);
begin
  -- worker is only asserted with a valid worker value when there is
  -- a valid operation
  is_master          <= to_bool(worker_in.operation = control_read_e or
                                worker_in.operation = control_write_e);
  starting           <= to_bool(worker_in.id(id_width-1 downto 0) = to_unsigned(id, id_width) and
                                not its(active_r));
                                --(not its(active_r) and (reset_n_r = '0' or is_master)));
                                 --(its(ready_r) or reset_n_r = '0' or is_master)));
  waiting            <= wci_in.SFlag(1);
  -- command is asserted after we go active - i.e. one cycle after we start
  -- FIXME: can we back this up now that occp is gone?
  -- assert_command     <= to_bool(active_r and its(ready_r) and
  -- assert_command     <= to_bool(active_r and not its(is_master) and not its(cmd_asserted_r));
  wci_out.Clk        <= worker_in.clk;
  wci_out.MReset_n   <= reset_n_r;
  wci_out.MCmd       <= worker_in.cmd when its(cmd_asserted_r) else ocpi.ocp.MCmd_IDLE;
  wci_out.MAddr      <= window_r & worker_in.address;
  wci_out.MAddrSpace(0) <= worker_in.is_config;
  wci_out.MByteEn    <= worker_in.byte_en;
  wci_out.MData      <= worker_in.data;
  wci_out.MFlag(0)   <= abort_r;
  wci_out.MFlag(1)   <= worker_in.is_big_endian;
  wci_out.MFlag(2)   <= barrier_r; -- tell worker we're in a barrier
  wci_out.MFlag(10 downto 3)  <= rank_r; -- position in cres
  wci_out.MFlag(18 downto 11) <= crew_r; -- tell worker we're in a barrier
  
  control <= slv(reset_n_r) &            -- 31
             slv(abort_r) &              -- 30
             slv0(29-24+1) &             -- 29:24
             crew_r &                    -- 23:16 size of crew when scaled
             rank_r &                    -- 15:8  rank in crew
             slv0(7-5+1) &               -- 7:5
             std_logic_vector(timeout_r);-- 4:0
  status  <= slv0(31-28+1) &             -- 31:28
             slv(last_write_r) &         -- 27
             last_control_r &            -- 26:24
             last_byte_en_r &            -- 23:20
             slv(last_write_valid_r) &   -- 19
             slv(last_control_valid_r) & -- 18
             slv(last_byte_en_valid_r) & -- 17
             slv(last_addr_valid_r) &    -- 16
             slv(waiting) &              -- 15 - worker is waiting for barrier
             slv(barrier_r) &            -- 14 - barrier in progress
             slv0(13-12+1) &             -- 13:12
             slv(wci_in.SFlag(2)) &      -- 11
             slv(wci_in.SFlag(0)) &      -- 10
             slv(attention_r) &          --  9
             sticky_r;                   --  8:0

  worker_out.data    <=
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
               busy_e     when starting and not its(ready_r) else
               none_e;
  worker_out.response   <= response when its(active_r) else none_e;
  worker_out.attention  <= wci_in.SFlag(0);
  worker_out.present    <= '1';
  worker_out.timeout    <= timeout_r;
  work : process(worker_in.clk)
  begin
    if rising_edge(worker_in.clk) then
      if its(worker_in.reset) then
        -- Core state
        reset_n_r            <= '0';
        window_r             <= (others => '0');
        attention_r          <= '0';
        timeout_r            <= to_unsigned(default_timeout_c, timeout_r'length);
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
        barrier_r            <= '0';
        sticky_r             <= (others => '0');
      else
        ready_r              <= to_bool(wci_in.SThreadBusy(0) = '0'); -- pipelined per spec
        if its(starting) then
          active_r <= '1';
          if not is_master then
            cmd_asserted_r <= btrue;
          end if;
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
          cmd_asserted_r <= bfalse;
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
                crew_r    <= worker_in.data(23 downto 16);
                rank_r    <= worker_in.data(15 downto 8);
              when 11 =>
                if worker_in.data(0) = '1' then
                  barrier_r <= '1';
                end if;
                if worker_in.data(1) = '1' then
                  barrier_r <= '0';
                end if;
              when 12 => -- window register
                window_r <= worker_in.data(window_r'range);
              when others => null;
            end case;
          end if;
          if response /= none_e then
            active_r <= bfalse;
            case response is
              when error_e    => -- error
                case worker_in.operation is
                  when config_write_e => sticky_r(2) <= '1';
                  when config_read_e  => sticky_r(1) <= '1';
                  when control_op_e   => sticky_r(0) <= '1';
                  when others         => null;
                end case;
              when timedout_e => -- worker timed out
                case worker_in.operation is
                  when config_write_e => sticky_r(8) <= '1';
                  when config_read_e  => sticky_r(7) <= '1';
                  when control_op_e   => sticky_r(6) <= '1';
                  when others         => null;
                end case;
              when busy_e     => -- worker was busy
                case worker_in.operation is
                  when config_write_e => sticky_r(5) <= '1';
                  when config_read_e  => sticky_r(4) <= '1';
                  when control_op_e   => sticky_r(3) <= '1';
                  when others         => null;
                end case;
              when others     => null; -- ok/none//data/reset
            end case;
          end if; -- there is a response
        end if; -- active
      end if; -- reset
    end if; -- rising edge
  end process;
end rtl;
