-- This file is protected by Copyright. Please refer to the COPYRIGHT file
-- distributed with this source distribution.
--
-- This file is part of OpenCPI <http://www.opencpi.org>
--
-- OpenCPI is free software: you can redistribute it and/or modify it under the
-- terms of the GNU Lesser General Public License as published by the Free
-- Software Foundation, either version 3 of the License, or (at your option) any
-- later version.
--
-- OpenCPI is distributed in the hope that it will be useful, but WITHOUT ANY
-- WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
-- A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
-- details.
--
-- You should have received a copy of the GNU Lesser General Public License
-- along with this program. If not, see <http://www.gnu.org/licenses/>.

--
-- The wci decoder when there are no properties at all
-- FIXME:  we could probably share more code with the "decoder" module of which this is a subset

library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.all; use ocpi.types.all; use ocpi.wci.all;
entity no_props_decoder is
  generic (
      worker                 : worker_t;
      ocpi_debug             : bool_t;
      endian                 : endian_t);
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
      is_big_endian          : out bool_t;   -- for runtime dynamic endian
      raw_in                 : in  raw_out_t
                               := (done => btrue, error => bfalse, data => (others => '0'));
      raw_out                : out raw_in_t;
      barrier                : out bool_t;
      crew                   : out UChar_t;
      rank                   : out UChar_t);
end entity;

architecture rtl of no_props_decoder is
  signal my_reset        : Bool_t;      -- positive logic version
  signal my_error        : Bool_t;      -- immediate error detected here (not from worker)
  signal my_config_error : Bool_t := bfalse;      -- immediate config error
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
  signal is_raw          : bool_t;
  signal my_done         : bool_t;
  signal my_big_endian   : bool_t;
  -- State for write data
  signal my_data         : dword_t;       -- combi or register as appropriate
  signal my_data_r       : dword_t;       -- registered data when delayed
  signal my_is_read      : bool_t;
  signal my_is_write     : bool_t;
begin
  --------------------------------------------------------------------------------
  -- Combi signals and outputs not specific to control ops or properties
  --------------------------------------------------------------------------------
  my_reset <= not ocp_in.MReset_n;
  access_in <= decode_access(ocp_in);
  -- ****** For now the OCCP could not tolerate immediate responses, so we actually
  -- ****** ALWAYS delay for at least one clock.  
  --  my_access   <= access_in when my_access_r = none_e else my_access_r;
  my_access   <= my_access_r;
  -- The response output is combinatorial if done or error is set.
  resp <= ocp.SResp_ERR when my_access /= none_e and my_error else
          ocp.SResp_DVA when my_access /= none_e and my_done and not its(my_error) else
          ocp.SResp_NULL;
  my_error  <= to_bool(my_access = error_e or
                       (my_access = control_e and (ok_op = '0' or next_op = '0')) or
                       (its(is_raw) and raw_in.error) or
                       (not its(is_raw) and error) or
                       my_config_error);
  -- The busy output is combinatorial, and my_access might be only registered
  busy <= to_bool(access_in /= none_e or my_access /= none_e);
  -- This should be a constant in the whole synthesized worker unit
  my_big_endian    <= to_bool(endian = big_e or (endian = dynamic_e and ocp_in.MFlag(1) = '1'));
  is_big_endian    <= my_big_endian;
  barrier          <= ocp_in.MFlag(2);
  rank             <= to_uchar(ocp_in.MFlag(10 downto 3));
  crew             <= to_uchar(ocp_in.MFlag(18 downto 11));
  --------------------------------------------------------------------------------
  -- Combi signals and outputs for control operations
  --------------------------------------------------------------------------------
  control_op_in    <= ocpi.wci.to_control_op(ocp_in.MAddr(4 downto 2)) when access_in = control_e else no_op_e;
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
  state_pos   <= get_state_pos(my_state_r);
  op_pos      <= get_op_pos(my_control_op);
  next_op     <= next_ops(state_pos)(op_pos);
  ok_op       <= worker.allowed_ops(op_pos);
  is_raw      <= my_is_read or my_is_write;
  my_done     <= raw_in.done when its(is_raw) else done;
  my_is_read  <= to_bool(my_access = read_e);
  my_is_write <= to_bool(my_access = write_e);
  raw_out.byte_enable <= ocp_in.MByteEn;
  raw_out.is_read     <= my_is_read and is_raw;
  raw_out.is_write    <= my_is_write and is_raw;
  my_data             <= ocp_in.MData when my_access_r = none_e else my_data_r;
  raw_out.address     <= resize(unsigned(ocp_in.MAddr), raw_out.address'length);
  raw_out.data        <= my_data;
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
        my_data_r       <= ocp_in.MData;
      elsif its(my_done) or my_error then
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
