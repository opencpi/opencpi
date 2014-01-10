--
-- The wci decoder when there are no properties at all
-- FIXME:  we could probably share some of this with above

library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.all; use ocpi.types.all; use ocpi.wci.all;
entity control_decoder is
  generic (
      worker                 : worker_t);
  port (
      ocp_in                 : in in_t;       
      done                   : in bool_t := btrue;
      error                  : in bool_t := bfalse;
      finished               : in bool_t;
      resp                   : out ocp.SResp_t;
      busy                   : out bool_t;
      control_op             : out control_op_t;
      state                  : out state_t;
      is_operating           : out bool_t;  -- just a convenience for state = operating_e
      abort_control_op       : out bool_t;
      is_big_endian          : out bool_t   -- for runtime dynamic endian
      );
end entity;

architecture rtl of control_decoder is
  signal my_reset        : Bool_t;      -- positive logic version
  signal my_error        : Bool_t;      -- immediate error detected here (not from worker)
  signal my_config_error : Bool_t;      -- immediate config error
  -- State for decoded accesses
  signal access_in       : access_t;
  signal my_access       : access_t;     -- combi or register as appropriate
  signal my_access_r     : access_t;     -- registered access in progress when not immediate
  -- State for control ops
  signal control_op_in   : control_op_t; -- combi input decode
  signal my_control_op   : control_op_t; -- combi or register as appropriate
  signal my_control_op_r : control_op_t; -- registered op when not immediately finished
  signal my_state_r      : state_t;
  signal next_op         : std_logic;
  signal ok_op           : std_logic;
  signal state_pos       : natural;
  signal op_pos          : natural;
  -- convert state enum value to state number
  -- because at least isim is broken and does not implement the "pos" function
  function get_state_pos(input: state_t) return natural is
  begin
    case input is
      when exists_e => return 0;
      when initialized_e => return 1;
      when operating_e => return 2;
      when suspended_e => return 3;
      when finished_e => return 4;
      when unusable_e => return 5;
    end case;
  end get_state_pos;
  -- convert control op enum value to a number
  -- because at least isim is broken and does not implement the "pos" function
  function get_op_pos(input: control_op_t) return natural is
  begin
    case input is
      when initialize_e   => return 0;
      when start_e        => return 1;
      when stop_e         => return 2;
      when release_e      => return 3;
      when before_query_e => return 4;
      when after_config_e => return 5;
      when test_e         => return 6;
      when no_op_e        => return 7;
    end case;
  end get_op_pos;
begin
  --------------------------------------------------------------------------------
  -- Combi signals and outputs not specific to control ops or properties
  --------------------------------------------------------------------------------
  my_reset <= not ocp_in.MReset_n;
  access_in <= decode_access(ocp_in);
  -- ****** For now the OCCP cannot tolerate immediate responses, so we actually
  -- ****** ALWAYS delay for at least one clock.  
  --  my_access   <= access_in when my_access_r = none_e else my_access_r;
  my_access   <= my_access_r;
  -- The response output is combinatorial if done or error is set.
  resp <= ocp.SResp_ERR when my_access /= none_e and (its(error) or my_error) else
          ocp.SResp_DVA when my_access /= none_e and done and not its(my_error) and not its(error) else
          ocp.SResp_NULL;
  my_error  <= to_bool(my_access = error_e or
                       (my_access = control_e and (ok_op = '0' or next_op = '0')) or
                       my_config_error);
  -- The busy output is combinatorial, and my_access might be only registered
  busy <= to_bool(access_in /= none_e or my_access /= none_e);
  is_big_endian    <= to_bool(ocp_in.MFlag(1) = '1');
  --------------------------------------------------------------------------------
  -- Combi signals and outputs for control operations
  --------------------------------------------------------------------------------
  control_op_in    <= ocpi.wci.to_control_op(ocp_in.MAddr(4 downto 2));
  -- the control op to the worker is either combinatorial or registered depending
  -- on whether it is delayed by the worker
-- FIXME: OCCP can't tolerate immediate ops, so we don't do it here
--  my_control_op    <= no_op_e when my_access /= control_e else
--                      control_op_in when my_control_op_r = no_op_e else
--                      my_control_op_r;
  my_control_op    <= my_control_op_r;
  control_op       <= my_control_op;
  state            <= my_state_r;
  is_operating     <= to_bool(my_state_r = operating_e);
  abort_control_op <= to_bool(ocp_in.MFlag(0) = '1');
  -- for our own error checking (not the worker's)
  state_pos <= get_state_pos(my_state_r);
  op_pos    <= get_op_pos(my_control_op);
  next_op   <= next_ops(state_pos)(op_pos);
  ok_op     <= worker.allowed_ops(op_pos);
  -- clocked processing is for delayed completion and capturing requests
  reg: process(ocp_in.Clk) is
  begin
    -- Since we support combinatorial completion, the clocked processing
    -- deals only with longer lived commands
    if rising_edge(ocp_in.Clk) then
      if its(my_reset) then
        my_access_r     <= None_e;
        if worker.allowed_ops(control_op_t'pos(initialize_e)) = '1' then
          my_state_r <= exists_e;
        else
          my_state_r <= initialized_e;
        end if;
      elsif access_in /= none_e then
        -- the first cycle of the request, capture it all per OCP
        my_access_r     <= access_in;  -- delayed version until occp is fixed
        my_control_op_r <= control_op_in;
      elsif its(done) or error or my_error then
        -- the last cycle of the request
        my_access_r <= none_e;
        if my_access = control_e and its(done) and not its(my_error) then
          -- successful control op - advance control state
          case my_control_op is
            when INITIALIZE_e =>
              my_state_r <= initialized_e;
            when START_e =>
              if my_state_r /= finished_e then
                my_state_r <= operating_e;
              end if;
            when STOP_e =>
              if my_state_r /= finished_e then
                my_state_r <= suspended_e;
              end if;
            when RELEASE_e =>
              my_state_r <= unusable_e;
            when others => null;                            
          end case;
        end if;
      elsif my_state_r /= unusable_e and finished then
        my_state_r <= finished_e;
      end if;
    end if; -- rising clock
  end process;
end rtl;
