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
      resp                   : out ocp.SResp_t;
      control_op             : out control_op_t;
      state                  : out state_t;
      is_operating           : out bool_t;  -- just a convenience for state = operating_e
      abort_control_op       : out bool_t;
      is_big_endian          : out bool_t   -- for runtime dynamic endian
      );
end entity;

architecture rtl of control_decoder is
  signal my_access : access_t;
  signal control_op_in : control_op_t;
  signal my_control_op : control_op_t;
  signal my_state : state_t;
  signal my_reset : Bool_t;
begin
  -- combinatorial signals used in various places
  state <= my_state;
  is_big_endian <= to_bool(ocp_in.MFlag(1) = '1');
  abort_control_op <= to_bool(ocp_in.MFlag(0) = '1');
  my_access <= decode_access(ocp_in);
  control_op_in <= ocpi.wci.to_control_op(ocp_in.MAddr(4 downto 2));
  my_reset <= not ocp_in.MReset_n;
  control_op <= my_control_op;
  -- manage state during control ops and manage the WCI/OCP SResp.
  -- remember that since we have no SCmdAccept signal, any command is only
  -- valid for one clock, but is finished when we assert SResp non-NULL
  reg: process(ocp_in.Clk) is
    -- a mask of allowable operations in the current state
    variable allowed_ops : control_op_mask_t;
    variable next_op : control_op_t;
    -- FIXME check that this synthesizes properly - may have to revert to logic...
    --function any_true(bools : bool_array_t) return boolean is
    --   variable result: boolean := false;
    --begin
    --  for i in bools'range loop
    --    if its(bools(i)) then result := true; end if;
    --  end loop;
    --  return result;
    --end any_true;
  begin
    if rising_edge(ocp_in.Clk) then
      -- default value of the SResp output, which is a register
      resp <= ocp.SResp_NULL;
      if ocp_in.MReset_n = '0' then
        is_operating <= bfalse;
        my_control_op <= NO_OP_e;
        if worker.allowed_ops(control_op_t'pos(initialize_e)) = '1' then
          my_state <= exists_e;
        else
          my_state <= initialized_e;
        end if;
        allowed_ops := next_ops(state_t'pos(my_state));
      elsif my_control_op /= NO_OP_e then
        if its(done) then                   -- FIXME done should also control config i/o
          -- finish the control by setting the state
          is_operating <= bfalse;
          case my_control_op is
            when INITIALIZE_e =>
              my_state <= initialized_e;
            when START_e =>
              my_state <= operating_e;
              is_operating <= btrue;
            when STOP_e =>
              my_state <= suspended_e;
            when RELEASE_e =>
              my_state <= unusable_e;
            when others => null;                            
          end case;
          resp <= ocp.SResp_DVA;
          my_control_op <= NO_OP_e;
        end if;
      else
        case my_access is
          when Control_e =>
            if my_control_op /= no_op_e or                   -- prevented by control plane
               worker.allowed_ops(control_op_t'pos(control_op_in)) = '0' or -- prevented by software
               next_ops(state_t'pos(my_state))(control_op_t'pos(control_op_in)) = '0'
            then    -- prevented by software
              -- This would only happen if the control plane or software is broken
              resp <= ocp.SResp_ERR;
            else
              my_control_op <= control_op_in;
            end if;   
          when None_e => null;
          when others =>
            resp <= ocp.SResp_ERR;
        end case;
      end if;
    end if;
  end process;
end rtl;
