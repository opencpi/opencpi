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

library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;

entity out_port_data_width_32_fsm is
  port(
    clk            : in  std_logic;
    reset          : in  std_logic;
    is_operating   : in  std_logic;
    out_ready      : in  std_logic;
    fifo_give      : in  std_logic;
    fifo_som       : in  std_logic;
    fifo_eom       : in  std_logic;
    fifo_valid     : in  std_logic;
    time_valid     : in  std_logic;
    overrun_event  : in  std_logic;
    data_q         : in  signed(15 downto 0);
    data_i         : in  signed(15 downto 0);
    time_sec       : in  std_logic_vector(31 downto 0);
    time_fract_sec : in  std_logic_vector(31 downto 0);
    opcode         : out std_logic_vector(2 downto 0); -- 000 : samples,
                                                       -- 001 : time,
                                                       -- 010 : interval,
                                                       -- 011 : flush,
                                                       -- 1xx : sync
    som            : out std_logic;
    eom            : out std_logic;
    valid          : out std_logic;
    data           : out std_logic_vector(31 downto 0);
    data_ready     : out std_logic;
    fifo_ready     : out std_logic);
end entity out_port_data_width_32_fsm;
architecture rtl of out_port_data_width_32_fsm is
  constant SAMPLES_OPCODE  : std_logic_vector(opcode'range) := "000";
  constant TIME_OPCODE     : std_logic_vector(opcode'range) := "001";
  constant INTERVAL_OPCODE : std_logic_vector(opcode'range) := "010";
  constant FLUSH_OPCODE    : std_logic_vector(opcode'range) := "011";
  constant SYNC_OPCODE     : std_logic_vector(opcode'range) := "100";
  type state_t is (SAMPLE, TIME_FRACT, TIME_INT, SYNC, IDLE);
  signal state : state_t := IDLE;
  signal recent_time    : std_logic_vector(time_sec'length +
                              time_fract_sec'length-1 downto 0) :=
                              (others => '1');
  signal state_is_sync                  : std_logic := '0';
  signal pending_sync                   : std_logic := '0';
  signal opcode_sent_over_multiple_clks : std_logic := '0';

  -- overrun may cause a samples message to be split into two or more messages
  signal force_samples_som : std_logic := '0';
  signal force_samples_eom : std_logic := '0';

  signal data_q_r     : signed(15 downto 0) := (others => '0');
  signal data_i_r     : signed(15 downto 0) := (others => '0');
  signal fifo_give_r  : std_logic := '0';
  signal fifo_som_r   : std_logic := '0';
  signal fifo_eom_r   : std_logic := '0';
  signal fifo_valid_r : std_logic := '0';

  signal data_ready_s : std_logic := '0';
  signal fifo_ready_s : std_logic := '0';
begin

  data_ready <= data_ready_s;
  fifo_ready <= fifo_ready_s;

  state_is_sync <= '1' when state = SYNC else '0';

  opcode_sent_over_multiple_clks <= '1' when state = TIME_INT else '0';

  state_and_other_metadata_gen : process(clk)
    variable pending_sync_force_samples_som : std_logic := '0';
  begin
    if rising_edge(clk) then
      if reset = '1' then
        state <= IDLE;
        pending_sync <= '0';
        pending_sync_force_samples_som := '0';
      elsif is_operating = '1' then
        -- when in idle state, no data is pending being sent, so out_in.ready
        -- is ignored, when not in idle state, data is pending being sent, so
        -- out_in.ready is required to advance state
        if (state = IDLE) or (state /= IDLE and out_ready = '1' and
            data_ready_s = '1') then

          if state = SAMPLE and overrun_event = '1' and
              opcode_sent_over_multiple_clks = '0' then

            -- enforces requirement: During overrun events, the ADC should produce
            -- a sync operation.
            state <= SYNC;

            pending_sync <= '1';
            pending_sync_force_samples_som := '1';
          elsif state = SYNC then
            pending_sync <= '0';
            -- because overruns will invalidate time+N*interval, time is always
            -- re-sent timestamp after overrun/SYNC
            state <= TIME_FRACT;
          elsif state = TIME_FRACT then
            state <= TIME_INT;
          elsif state = TIME_INT then
            state <= SAMPLE;
            if pending_sync_force_samples_som = '1' then
              force_samples_som <= '1';
              pending_sync_force_samples_som := '0';
            end if;
          -- start sending time message as soon as samples message is done, i.e.
          -- as soon as fifo_eom_r = '1'
          elsif (((state = SAMPLE) and (fifo_eom_r = '1')) or (state = IDLE))
              and (time_valid = '1') then
            state <= TIME_FRACT;
          else
            state <= SAMPLE;
            force_samples_som <= '0';
          end if;

        end if;
      end if;
    end if;
  end process state_and_other_metadata_gen;

  force_samples_eom <= overrun_event;

  metadata_gen : process(state, fifo_som_r, fifo_eom_r, fifo_valid_r,
                         force_samples_som, force_samples_eom)
  begin
    case state is
      -- note that, for a given operation, the first argument (or struct
      -- argument member) is in least significant bits (see
      -- https://opencpi.github.io/OpenCPI_HDL_Development.pdf section on
      -- message payloads vs. physical data width on data interfaces).
      when SAMPLE =>
        opcode <= SAMPLES_OPCODE;
        som    <= fifo_som_r or force_samples_som;
        eom    <= fifo_eom_r or force_samples_eom;
        valid  <= fifo_valid_r;
      when TIME_FRACT =>
        opcode <= TIME_OPCODE;
        som    <= '1';
        eom    <= '0';
        valid  <= '1';
      when TIME_INT =>
        opcode <= TIME_OPCODE;
        som    <= '0';
        eom    <= '1';
        valid  <= '1';
      when SYNC =>
        opcode <= SYNC_OPCODE;
        som    <= '1';
        eom    <= '1';
        valid  <= '0';
      when IDLE =>
        opcode <= SAMPLES_OPCODE;
        som    <= '0';
        eom    <= '0';
        valid  <= '0';
    end case;
  end process metadata_gen;

  -- registers necessary in order to align data one clock after
  -- overrun_event (a requirement generated by FSM architecture)
  samples_regs : process(clk)
  begin
    if rising_edge(clk) then
      if reset = '1' then
        data_q_r <= (others => '0');
        data_i_r <= (others => '0');
        fifo_give_r  <= '0';
        fifo_som_r   <= '0';
        fifo_eom_r   <= '0';
        fifo_valid_r <= '0';
      elsif fifo_ready_s = '1' then
        data_q_r    <= data_q;
        data_i_r    <= data_i;
        fifo_give_r  <= fifo_give;
        fifo_som_r   <= fifo_som;
        fifo_eom_r   <= fifo_eom;
        fifo_valid_r <= fifo_valid;
      end if;
    end if;
  end process samples_regs;

  data_and_flow_ctrl_gen : process(state, data_q_r, data_i_r, out_ready,
                                   pending_sync, fifo_give_r, time_sec,
                                   time_fract_sec)
  begin
    case state is
      -- note that, for a given operation, the first argument (or struct
      -- argument member) is in least significant bits (see
      -- https://opencpi.github.io/OpenCPI_HDL_Development.pdf section on
      -- message payloads vs. physical data width on data interfaces).
      when SAMPLE =>
        data       <= std_logic_vector(data_q_r) & std_logic_vector(data_i_r);

        -- beyond normal port flow control, wait to dequeue fifo until after
        -- overrun_event is handled by sending a SYNC
        fifo_ready_s <= out_ready and (not pending_sync);

        data_ready_s <= fifo_give_r;
      when TIME_FRACT =>
        data <= std_logic_vector(time_fract_sec);
        -- half of time_corrected must be registered and sent later due to
        -- data width of 32
        recent_time <= time_sec & time_fract_sec;

        -- wait until SAMPLE state to pop ADC sample off of the FIFO
        fifo_ready_s <= '0';

        data_ready_s <= '1';
      when TIME_INT =>
        data <= recent_time(recent_time'left downto recent_time'left-time_sec'length+1);

        -- wait until SAMPLE state to pop ADC sample off of the FIFO
        fifo_ready_s <= '0';

        data_ready_s <= '1';
      when SYNC =>
        data <= (others => '0');
        fifo_ready_s <= '0';
        data_ready_s <= '1';
      when IDLE =>
        data <= (others => '0');
        fifo_ready_s <= '0';
        data_ready_s <= '0';
    end case;
  end process data_and_flow_ctrl_gen;

end rtl;
