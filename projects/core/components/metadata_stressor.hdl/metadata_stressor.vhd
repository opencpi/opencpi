-- THIS FILE WAS ORIGINALLY GENERATED ON Thu Jul 19 12:51:57 2018 EDT
-- BASED ON THE FILE: metadata_stressor.xml
-- YOU *ARE* EXPECTED TO EDIT IT
-- This file initially contains the architecture skeleton for worker: metadata_stressor

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

-- This worker provides a series of valid metadata combinations a worker could be
-- expected to accept without failure, though some are unlikely to be encountered.
-- It takes the messages passed to it, and will change the metadata, insert no-ops
-- in the data, or both. It can also add zero length messages between messages.

-- this worker is primarily controlled by the mode property
-- mode = bypass_e, worker is bypassed entirely
-- mode = data_e, metadata is passed through unchanged but data will have no-ops
--           inserted based on enable_take_lsfr or take_duty
-- mode = metadata_e, metadata will cycle through all possible valid combinations,
--           but data will pass through unchanged (ex. it receives a no-op, it passes a no-op)
-- mode = full_e, both metadata and data will be manipulated

-- There are other that control output are:
-- enable_give_lsfr = True: The most significant bit of the lfsr-15 drives output
-- enable_give_lsfr = False: give_duty controls the duty cycle of the output.
-- enable_take_lsfr = True: The seventh bit of the lfsr-15 drives output
-- enable_take_lsfr = False: take_duty controls the duty cycle of the output.
-- give_duty - controls length of give duty cycle if not driven by the LFSR
-- take_duty - controls length of take duty cycle if not driven by the LFSR
-- seed - seed value for lfsr
-- allow_zlms - controls whether zlms are inserted between some messages
-- insert_nop - inserts no ops between messages

-- The primary operation of this worker is the output_select_proc and the FSM
-- contained within. The FSM cycles through the valid metadata patterns, while
-- observing the incoming message boundaries. The patterns are:
-- early SOM, data, late EOM
-- early SOM, data, EOM with data
-- SOM with data, data, late EOM
-- SOM with data, data, EOM with data, (single word message if that is what is received)
-- zero length message (if allow_zlms is true)
-- It repeats those patterns so long as there is data.


library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all; use ieee.math_real.all;
library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions
library util; use util.util.all;
architecture rtl of worker is

  constant DATA_WIDTH_c         : integer := to_integer(unsigned(DATA_WIDTH_p));
  constant BYTE_EN_WIDTH_c      : integer := ocpi_port_in_MByteEn_width;
  constant OPCODE_WIDTH_c       : integer := 8; -- derived from max number of opcodes

  signal enable        : std_logic; -- up/downstream Workers ready and operating
  signal ready_for_in_port_data : std_logic; -- mandatory input port logic
  signal data_ready_for_out_port : std_logic; -- mandatory output port logic
  signal take_en       : std_logic; -- used to throttle taking data in some modes
  signal val_take      : std_logic; -- this worker is prepared to take data
  signal give_en       : std_logic; -- used to throttle giving data in some modes
  signal uut_ready     : std_logic; -- next worker is ready and give is enabled and input is ready
  signal out_ready     : std_logic; -- next worker is ready and give is enabled
  signal in_ready      : std_logic; -- upstream worker is ready
  signal deferred_take : std_logic; -- wanted to take (val_take) but couldn't
  signal zlm_detected  : std_logic; -- zlm detected on input
  signal zlm_queued    : std_logic; -- buffer zlm after detection
  signal swm_detected  : std_logic; -- swm detected on input and not being given
  signal swm_seen      : std_logic; -- swm detected on input
  signal giving_swm_valid : std_logic; -- the valid part of a swm is being given
  signal in_som        : std_logic; -- used to maintain message boundaries
  signal in_valid      : std_logic; -- used to maintain message boundaries
  signal in_eom        : std_logic; -- used to maintain message boundaries
  signal split_swm     : std_logic; -- split swm detected on input
  signal swm_take      : std_logic := '0'; -- used to take data in swm edge cases
  signal EOF           : std_logic; -- used to detect zlms with opcode zero for legacy reasons
  signal EOF_flush     : std_logic; -- used to detect zlms with opcode zero for legacy reasons
  signal trailing_eom  : std_logic; -- detect trailing eoms in order to handle them appropriately
  signal lone_som      : std_logic; -- detect early soms in order to handle them appropriately
  signal swm_live      : std_logic; -- detect swms as they occur

-- key for how states are named:
--
-- es: early start of message
-- le: late end of message
-- sv: start of message with data
-- ve: end of message with data

-- start: beginning of the crafted message
-- end: end of a crafted message
  type StateType is (init_s, start_es_a, es_val_le_a, end_le_a, start_es_b, val_ve_b, end_b_zlm, end_b_swm,
                     start_sv_c, sv_val_le_c, end_le_c, start_sv_d, val_ve_d, end_d,
                     swm_e, nil_s, zlm_z, split_zlm_end);

-- state is the state variable for the fsm in output_select_proc
-- nil_state_store stores the state going into nil_s so that nil_s can insert a
-- delay between messages and the usual flow can be restored, since many states
-- will need to enter nil_s.
  signal state, nil_state_store : StateType;

-- output_state determines what metadata is delivered on the output port
-- Note on valid vs valid_buf, valid_buf is to flush data before late eoms
  type OutStateType is (early_som, valid, late_eom, val_eom, som_val, swm, zlm, prop_nil,
                        nil, valid_buf, som_val_swm, split_zlm_s, split_zlm_e, zlm_eof, init);
  signal output_state : OutStateType;

  signal out_som       : std_logic; -- this worker's choice of som
  signal out_eom       : std_logic; -- this worker's choice of eom
  signal out_valid     : std_logic; -- this worker's choice of valid
  signal out_data      : std_logic_vector(DATA_WIDTH_c-1 downto 0) := (others => '0'); -- buffer for data
  signal out_be        : std_logic_vector(BYTE_EN_WIDTH_c-1 downto 0) := (others => '0'); -- buffer for byte enable
  signal out_op        : std_logic_vector(OPCODE_WIDTH_c-1 downto 0) := (others => '0'); -- buffer for opcode
  signal out_eof       : bool_t; -- this worker's driving signal for v2 out_out.eof
  signal op_zero       : std_logic_vector(OPCODE_WIDTH_c-1 downto 0) := (others => '0'); -- for comparing against opcodes
  signal be_zero       : std_logic_vector(BYTE_EN_WIDTH_c-1 downto 0) := (others => '0'); -- for comparing against opcodes

-- counter for take duty cycle
  signal count_t       : unsigned(props_in.take_duty'range) := (others => '0');

-- counter for give duty cycle
  signal count_g       : unsigned(props_in.give_duty'range) := (others => '0');

-- linear-feedback shift register
  signal lfsr          : unsigned(props_in.seed'range) := (others => '0');


begin

  -- Primitive detects zlms, split or not
    zlm_detect : util.util.zlm_detector
      port map (
       CLK   => ctl_in.clk,
       RESET => ctl_in.reset,
       SOM   => in_in.som,
       VALID => in_in.valid,
       EOM   => in_in.eom,
       READY => in_in.ready,
       TAKE  => ready_for_in_port_data,
       EOZLM_PULSE => open,
       EOZLM   => zlm_detected);

  -- This process detects zero length messages and ends of message
  -- and sets signals used to exert backpressure on the previous worker
  -- or clamp output to the next worker accordingly.
  buffer_inputs : process (ctl_in.clk)
  begin
    if rising_edge(ctl_in.clk) then
      if (ctl_in.reset = '1') then
        in_som <= '0';
        in_valid <= '0';
        in_eom <= '0';
        split_swm <= '0';
        swm_seen <= '0';
        EOF_flush <= '0';
        zlm_queued <= '0';
        out_eof <= bfalse;
        deferred_take <= '0';
      else

  -- Buffer the data and opcode while the new metadata is being generated
  -- to ensure that it is correct
        if ready_for_in_port_data = '1' then
         out_data <= in_in.data;
         out_op   <= in_in.opcode;
        end if;

  -- Byte enables have to be buffered differently because of the possibility of trying to
  -- generate a trailing eom after having received a trailing eom
  -- blindly buffering will cause loss of data in that case
        if (ready_for_in_port_data = '1') then
          out_be   <= in_in.byte_enable;
        end if;
-- This logic handles zlms in the general case
        if (zlm_detected = '1' and output_state /= prop_nil) then
          zlm_queued <= '1';
        elsif (zlm_queued = '1' and (out_op /= op_zero or ocpi_version > 1) and
        (output_state = zlm or (output_state = early_som and data_ready_for_out_port = '1') or
                             output_state = split_zlm_s)) then
          zlm_queued <= '0';
        end if;

-- This worker will usually operate with file_read providing data, and file_read sends zlms with opcode = 0
-- to signify the end of data, which needs to be detected so the worker can flush data.
-- This has to be held until the next worker has been given the zlm
         if (ocpi_version < 2 and zlm_detected = '1' and in_in.opcode = op_zero and output_state /= prop_nil) then
           EOF_flush <= '1';
         elsif (EOF_flush = '1' and data_ready_for_out_port = '1' and (output_state = zlm or
                              output_state = split_zlm_e)) then
           EOF_flush <= '0';
         end if;
        if (ocpi_version > 1 and in_in.eof and out_in.ready and
            (props_in.mode = data_e or props_in.mode = bypass_e or
             (output_state = prop_nil and swm_detected = '0' and zlm_queued = '0') or
             (output_state = nil and zlm_queued = '0'))) then
          out_eof <= btrue;
        end if;

  -- Save SOM and valid until the next time the worker reads its own inputs in order to detect
  -- split single word messages
          if ready_for_in_port_data = '1' then
            in_som   <= in_in.som;
            in_valid <= in_in.valid;
            in_eom <= in_in.eom;
          end if;


  -- Detect single word messages (swm) in order to make decisions in the output_select_proc FSM.
  -- Flag can't be cleared until the swm is sent, or data might be lost
          split_swm <= in_in.eom and not(in_in.valid) and in_in.ready and in_som and in_valid and not(zlm_detected);
          if ((in_in.som = '1' and in_in.eom = '1' and in_in.valid = '1') or split_swm = '1') then
            swm_seen <= '1';
          elsif (swm_seen = '1' and giving_swm_valid) then
            swm_seen <= '0';
          end if;
        if (val_take and in_ready = '0') then
          deferred_take <= '1';
        end if;
        if deferred_take and ready_for_in_port_data then
          deferred_take <= '0';
        end if;
      end if;
    end if;
  end process buffer_inputs;

  -- flags for dealing with single word messages
  giving_swm_valid <=  out_ready and to_bool(output_state = valid or output_state = valid_buf or
                                             output_state = swm or output_state = val_eom or
                                             output_state = som_val_swm or output_state = som_val);
  swm_detected <= swm_seen and not giving_swm_valid;

  -- This FSM controls what kind of message is passed to the unit under test.
  output_select_proc : process (ctl_in.clk)
  begin
   if rising_edge(ctl_in.clk) then
     if (ctl_in.reset = '1') then
       output_state <= nil;
       state <= init_s;
       nil_state_store <= init_s;
     else

  -- It is intentional that the output_state remain the same until the following
  -- state machine deliberately changes it.
       case(state) is
  -- initial state
       when init_s =>
         if (in_in.ready = '1') then
           output_state <= init;
           state <= start_es_a;
         else
           output_state <= nil;
         end if;
  -- early start of message preceding late end of message
       when start_es_a =>
         if ((out_ready = '1' and zlm_queued = '1') or
            (out_in.ready = '1' and EOF = '1')) then
            state <= end_le_a;
            output_state <= early_som;
         elsif (swm_detected = '1' and out_ready = '1') then
           state <= es_val_le_a;
           output_state <= early_som;
         elsif (take_en = '1' and uut_ready = '1') then
           state <= es_val_le_a;
           output_state <= early_som;
         elsif (out_ready = '1') then
           output_state <= nil;
         end if;
  -- send data and/or no-ops between early start of message and late end of message
       when es_val_le_a =>
         if (out_in.ready = '1' and EOF = '1' and in_valid = '0') or
             (out_ready = '1' and in_valid = '0' and zlm_queued = '1') then
           output_state <= late_eom;
           state <= start_es_b;
        elsif ((in_in.eom = '1' and  output_state = valid and uut_ready = '1' and
             output_state /= nil and swm_detected = '0')) or
             ((swm_detected = '1' or swm_live = '1') and out_ready = '1') then
           output_state <= valid_buf;
           state <= end_le_a;
        elsif (take_en = '1' and uut_ready = '1') then
           output_state <= valid;
        elsif (out_ready = '1') then
           output_state <= nil;
         end if;
  -- send late end of message after early start of message
       when end_le_a =>
         if (out_ready = '1' or (out_in.ready = '1' and EOF = '1')) then
           if (EOF = '1') then
             state <= zlm_z;
           elsif (props_in.insert_nop = '1') then
             state <= nil_s;
             nil_state_store <= start_es_b;
           else
             state <= start_es_b;
           end if;
           output_state <= late_eom;
         end if;
  -- early start of message preceding end of message with data
       when start_es_b =>
        if ((out_ready = '1' and zlm_queued = '1') or
           (out_in.ready = '1' and EOF = '1')) then
          state <= end_b_zlm;
          output_state <= early_som;
        elsif (swm_detected = '1' and out_ready = '1') then
          state <= end_b_swm;
          output_state <= early_som;
        elsif (take_en = '1' and uut_ready = '1' and in_in.valid = '0') then
           state <= end_b_zlm;
           output_state <= early_som;
        elsif (take_en = '1' and uut_ready = '1') then
           state <= val_ve_b;
           output_state <= early_som;
        elsif (out_ready = '1') then
           output_state <= nil;
         end if;
  -- send data and/or no-ops and output end of message with data after early start of message
       when val_ve_b =>
         -- we enter this state with taken data and early_som
         if out_ready = '1' then
           if ((output_state = early_som and in_eom = '1') or
               (output_state = valid and in_ready and in_in.eom)) then
             output_state <= val_eom;
             if (props_in.insert_nop = '1') then
               state <= nil_s;
               nil_state_store <= start_sv_c;
             elsif EOF = '1' then
               state <= zlm_z;
             else
               state <= start_sv_c;
             end if;
           elsif output_state = early_som or (in_ready = '1' and take_en) then
             output_state <= valid;
           else
             output_state <= nil;
           end if;
         end if;
-- handle zlms detected in start_es_b
      when end_b_zlm =>
        if (out_ready = '1' or (out_in.ready = '1' and EOF = '1')) then
          output_state <= late_eom;
          if (EOF = '1') then
            state <= zlm_z;
          elsif (props_in.insert_nop = '1') then
            state <= nil_s;
            if (swm_detected = '1' ) then
              nil_state_store <= start_sv_d;
            else
              nil_state_store <= start_sv_c;
            end if;
          else
            if (swm_detected = '1' ) then
              state <= start_sv_d;
            else
              state <= start_sv_c;
            end if;
          end if;
        end if;
-- send data and/or no-ops and output end of message with data after early start of message in the case of swm
      when end_b_swm =>
        if (out_ready = '1' or EOF_flush = '1') then
          if (EOF = '1') then
            state <= zlm_z;
          elsif (props_in.insert_nop = '1') then
            state <= nil_s;
            nil_state_store <= start_es_a; -- the swm - eom with data case fails when
          else                             -- there's a trailing eom swm on the input because the
            state <= start_es_a;           -- swm_take used to flush the trailing eom from the input
          end if;                          -- coincides with the take the fsm needs in
          output_state <= val_eom;         -- essentially there needs to be a second take, when val_take and swm_take coincide
        end if;                            -- or you can just skip it, because it's only a problem for
                                           -- this worker, some of the time, when it's behind itself
  -- send start of message with data preceding late end of message
       when start_sv_c =>
        if (((zlm_queued = '1' or zlm_detected = '1') and out_ready = '1') or
           (out_in.ready = '1' and EOF = '1')) then
            state <= end_le_c;
            output_state <= early_som;
        elsif (take_en = '1' and uut_ready = '1') then
          if (swm_detected = '1' and swm_take = '0') or
             (in_in.eom = '1' and in_in.valid = '1' and ready_for_in_port_data = '1') then
            output_state <= som_val_swm;
            state <= end_le_c;
          else
            state <= sv_val_le_c;
            output_state <= som_val;
          end if;
        elsif (out_ready = '1') then
          if (give_en = '1' and swm_detected = '1') then
            state <= end_le_c;
            output_state <= som_val_swm;
          else
            output_state <= nil;
          end if;
        end if;
  -- send data and/or no-ops between start of message with data and late end of message
       when sv_val_le_c =>
         if (in_in.eom = '1' and uut_ready = '1' and swm_detected = '0' and data_ready_for_out_port and
             (val_take or output_state /= nil)) then
           output_state <= valid_buf;
           state <= end_le_c;
         elsif (swm_detected = '1' and out_ready = '1') then
           output_state <= late_eom;
           state <= start_sv_d;
         elsif (take_en = '1' and uut_ready = '1' and swm_take = '0') then
           output_state <= valid;
         elsif (out_ready = '1') then
           output_state <= nil;
         end if;
  -- send late end of message after start of message with data
       when end_le_c =>
         if (out_ready = '1' or (out_in.ready = '1' and EOF = '1')) then
           if (EOF = '1') then
             state <= zlm_z;
           elsif (props_in.insert_nop = '1') then
             state <= nil_s;
             nil_state_store <= start_sv_d;
           else
             state <= start_sv_d;
           end if;
           output_state <= late_eom;
         end if;
  -- send start of message with data preceding end of message with data
       when start_sv_d =>
         if ((zlm_queued = '1' and out_ready = '1') or
            (out_in.ready = '1' and EOF = '1')) then
           state <= end_d;
           output_state <= early_som;
         elsif (uut_ready = '1' ) then
           if (swm_detected = '1') then
             if (props_in.insert_nop = '1') then
               state <= nil_s;
               nil_state_store <= start_es_a;
             else
               state <= start_es_a;
             end if;
             output_state <= swm;
           elsif (ready_for_in_port_data = '1' and in_in.valid = '0') then
             state <= end_d;
             output_state <= early_som;
           else
             state <= val_ve_d;
             output_state <= som_val;
           end if;
         elsif (out_ready = '1') then
           state <= start_es_a;
           if (swm_detected = '1' and data_ready_for_out_port = '0') then
             output_state <= swm;
           else
             output_state <= nil;
           end if;
         end if;
  -- send data and/or no-ops and output end of message with data after start of message with data
       when val_ve_d =>
         if (in_in.eom = '1' and uut_ready = '1' and output_state /= nil) then
             if (props_in.allow_zlms = '1') then
               if (props_in.insert_nop = '1') then
                 state <= nil_s;
                 nil_state_store <= zlm_z;
               else
                 state <= zlm_z;
               end if;
             else
               if (props_in.insert_nop = '1') then
                 state <= nil_s;
                 nil_state_store <= start_es_a;
               else
                 state <= start_es_a;
               end if;
             end if;
           output_state <= val_eom;
         elsif (take_en = '1' and uut_ready = '1') then
           output_state <= valid;
         elsif (out_ready = '1') then
           output_state <= nil;
         end if;
  -- handle zlms detected in start_sv_d
      when end_d =>
        if (out_ready = '1' or (out_in.ready = '1' and EOF = '1')) then
          if (props_in.insert_nop = '1') then
            state <= nil_s;
            nil_state_store <= start_es_a;
          else
            state <= start_es_a;
          end if;
          output_state <= late_eom;
        end if;
  -- if allow_zlms = true, randomly chose single message zlm or split zlm
  -- or if a zlm is detected on input, pass it through
      when zlm_z =>
        if output_state = zlm_eof and out_in.ready = '1' then
          state <= init_s;
          output_state <= nil;
        elsif EOF_flush = '1' or (EOF = '1' and out_in.ready = '1') then
          output_state <= zlm;
          state <= start_es_a;
        elsif (take_en = '1' and  uut_ready = '1' and lfsr(1) = '0') then
          output_state <= split_zlm_s;
          state <= split_zlm_end;
        end if;
  -- send the second half of a split zlm
      when split_zlm_end =>
       if (take_en = '1' and  uut_ready = '1') then
         output_state <= split_zlm_e;
         state <= start_es_a;
       end if;
  -- send no-op
      when nil_s =>
       if ((out_in.ready = '1' and (EOF = '1' or (EOF_flush = '1' and out_eom and data_ready_for_out_port)))) then
         state <= zlm_z;
         output_state <= zlm_eof;
       elsif (out_ready = '1') then
         state <= nil_state_store;
         output_state <= prop_nil;
       end if;
  -- not expected, but covers the possibility that the state variable is in an unknown state
      when others =>
        state <= init_s;
        output_state <= nil;
       end case;
     end if;
   end if;
  end process output_select_proc;

  -- controls the duty cycle or randomness of taking dating and giving data
  stutter : process (ctl_in.clk)
    variable give_v : std_logic;
    variable take_v : std_logic;
  begin
    if rising_edge(ctl_in.clk) then
      if (ctl_in.reset = '1') then
        give_en <= '1';
        take_en <= '1';
        lfsr <= props_in.seed;
      else

  -- don't starve data in metadata mode
        if (props_in.mode = metadata_e) then
          give_en <= '1';
  -- using MSB of lfsr to provide randomness
        elsif (props_in.enable_give_lsfr = '1') then
          give_en <= lfsr(15);
        else
          give_en <= give_v;
        end if;

  -- don't starve data in metadata mode
        if (props_in.mode = metadata_e) then
          take_en <= '1';
  -- using 7th bit of lfsr to provide randomness (that isn't the same as give_en)
        elsif (props_in.enable_take_lsfr = '1') then
          take_en <= lfsr(7);
        else
          take_en <= take_v;
        end if;

        if (out_in.ready = '1') then
  -- polynomial for linear-feedback shift register is x^16 + x^14 + x^13 + x^11 + 1
          lfsr <= lfsr(14 downto 0) & (lfsr(15) xor lfsr(13) xor lfsr(12) xor lfsr (10));
        end if;

        if (count_g >= props_in.give_duty-1) then
          count_g  <= (others => '0');
          give_v := '1';
        elsif (out_in.ready = '1') then
          count_g  <= count_g + 1;
          give_v := '0';
        end if;

   -- in case the user wants to use mode 1 but hasn't set a duty cycle for take
   -- setting the duty cycle to 5 so this doesn't accidentally mimic bypass
        if ((props_in.mode = data_e) and (props_in.take_duty < 2)) then
          if (count_t >= 5) then
            count_t  <= (others => '0');
            take_v := '1';
          elsif (out_in.ready = '1') then
            count_t  <= count_t + 1;
            take_v := '0';
          end if;
        else
          if (count_t >= props_in.take_duty-1) then
            count_t  <= (others => '0');
            take_v := '1';
          elsif (out_in.ready = '1') then
            count_t  <= count_t + 1;
            take_v := '0';
          end if;
        end if;
      end if;
    end if;
  end process stutter;

  -- these are used in the main FSM
  EOF <= EOF_flush and (not in_valid or in_eom);
  swm_take <= in_in.eom and not(in_in.valid) and split_swm and not(zlm_detected);
  lone_som <=  in_in.som and not(in_in.valid) and not(in_in.eom);
  trailing_eom <= '1';
  swm_live <= in_in.eom and not(in_in.valid) and in_som and in_valid and in_in.ready;

  -- out_som, out_valid, and out_eom determine the metadata pattern in full or
  -- metadata mode, and are driven depending on output_state.
  out_som <= '1' when (output_state = early_som or output_state = som_val or
                       output_state = swm or output_state = som_val_swm or
                       output_state = zlm or output_state = split_zlm_s or output_state = zlm_eof) else '0';
  out_valid <= '1' when (output_state = valid or output_state = valid_buf or
                         output_state = val_eom or output_state = som_val or
                         output_state = som_val_swm or output_state = swm) else '0';
  out_eom <= '1' when (output_state = late_eom or output_state = val_eom or
                       output_state = swm or output_state = zlm or output_state = zlm_eof or
                       output_state = split_zlm_e) else '0';

  -- In the following output states, this worker will take data (assuming other
  -- conditions are also true).
  val_take <= '1' when (output_state = valid or output_state = late_eom or
                        output_state = val_eom or output_state = som_val or
                        output_state = swm or output_state = init or
                        output_state = zlm_eof) else '0';

  out_out.som   <= out_som when (props_in.mode = full_e or props_in.mode = metadata_e) else in_in.som;
  out_out.valid <= to_bool(out_valid and (ocpi_version < 2 or data_ready_for_out_port))
                   when (props_in.mode = full_e or props_in.mode = metadata_e) else
                   in_in.valid and to_bool(ocpi_version < 2 or data_ready_for_out_port) and
                   (take_en or in_in.eom)
                   when (props_in.mode = data_e) else in_in.valid;
  out_out.eom   <= out_eom when (props_in.mode = full_e or props_in.mode = metadata_e) else in_in.eom;

  -- buffered data only needed when this worker is manipulating metadata, not in data or bypass mode
  out_out.data  <= out_data when (props_in.mode = full_e or props_in.mode = metadata_e) else in_in.data;
  out_out.byte_enable <= out_be when (props_in.mode = full_e or props_in.mode = metadata_e) else in_in.byte_enable;
  out_out.opcode <= out_op when (props_in.mode = full_e or props_in.mode = metadata_e) else in_in.opcode;

  -- these signals manage throughput
  out_ready <= out_in.ready and give_en and ctl_in.is_operating;
  in_ready  <= in_in.ready;
  uut_ready <= out_ready and in_ready;
  enable <= ctl_in.is_operating and out_in.ready and in_in.ready;
  ready_for_in_port_data <= ((enable and (deferred_take or val_take or swm_take or lone_som) and give_en))
                 when (props_in.mode = full_e or props_in.mode = metadata_e) else
                 (enable and take_en and give_en)
                 when (props_in.mode = data_e) else enable;
  in_out.take <= ctl_in.is_operating and ready_for_in_port_data;

  -- Don't assert give if metadata is all zeros
  -- Do assert give if sending a zlm
  -- '... or zlm_queued' included to send data at end of file after receiving a split swm
  data_ready_for_out_port <= ((out_ready and trailing_eom and (out_som or out_eom or out_valid))
              or  (out_ready and ((out_som or out_eom) and not(out_valid)) and (zlm_queued))
              or (out_in.ready and (out_som or out_eom or out_valid) and EOF))
                  when (props_in.mode = full_e or props_in.mode = metadata_e) else
                  ((enable and take_en and give_en) or
                  (out_in.ready and take_en and (EOF)))
                  when (props_in.mode = data_e) else enable;
  out_out.give <= ctl_in.is_operating and data_ready_for_out_port;

  gen0: if ocpi_version > 1 generate
    out_out.eof <= out_eof;
  end generate gen0;
end rtl;
