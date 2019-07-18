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

-- DEPRECATION NOTICE: This worker is deprecated and will be removed in OpenCPI 2.0. Use the Zero Pad component for new designs.

-------------------------------------------------------------------------------
-- Zero Padding
-------------------------------------------------------------------------------
--
-- Description:
--
-- The Zero Padding worker inputs bits, expands the bits to signed Qm.n samples within
-- the range of +/- 1, and inserts num_zeros zeros between each output sample. The
-- ODATA_WIDTH_p parameter defines the number of bits in each output sample. Valid
-- output sizes are signed 8/16/32/64 bit samples.
--
-- Since there is no internal buffering the output buffer size limits the maximum
-- number of zeros allowed. The output size is ODATA_WIDTH_p * (num_zeros+1) for each
-- input bit.
-------------------------------------------------------------------------------

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
use ieee.math_real.all;
library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions

architecture rtl of zero_padding_worker is

  function max (LEFT, RIGHT: INTEGER) return INTEGER is
  begin
    if (LEFT > RIGHT) then
      return LEFT;
    else
      return RIGHT;
    end if;
  end function max;

  constant MAX_MESSAGE_VALUES_c : natural := to_integer(ocpi_max_bytes_out/IDATA_WIDTH_p/8);

  signal enable         : std_logic;
  signal idata          : std_logic_vector(to_integer(unsigned(IDATA_WIDTH_p))-1 downto 0);
  signal idata_vld      : std_logic_vector(to_integer(unsigned(IDATA_WIDTH_p))-1 downto 0);
  signal busy_n         : std_logic;
  signal odata_vld      : std_logic;
  signal msg_cnt        : unsigned(integer(ceil(log2(real(MAX_MESSAGE_VALUES_c))))-1 downto 0);
  signal max_sample_cnt : unsigned(integer(ceil(log2(real(MAX_MESSAGE_VALUES_c))))-1 downto 0);
  signal take           : std_logic;
  signal force_som      : std_logic;
  signal force_eom      : std_logic;
  type state_t is (INIT_s, WAIT_s, SEND_s);
  signal current_state : state_t;
  -- This temp signal is needed to work around a bug in xsim versions post-2015.4
  signal out_data_temp  : std_logic_vector(to_integer(unsigned(ODATA_WIDTH_p))-1 downto 0);
  signal messageSize    : ulong_t;
begin

  -----------------------------------------------------------------------------
  -- 'enable' zero padding (when up/downstream Workers ready, input data valid)
  -----------------------------------------------------------------------------
  enable <= '1' when (out_in.ready = '1' and in_in.ready = '1' and in_in.valid = '1'
                and ctl_in.is_operating = '1') else '0';

  -----------------------------------------------------------------------------
  -- Byte swap and 'valid' signal alignment, dependent on input data width
  -- Generate necessary due to compiler issues with combinatorial statements
  -----------------------------------------------------------------------------
  in_8byte_enable : if (to_integer(unsigned(IDATA_WIDTH_p)) = 64) generate
    process (in_in.byte_enable, in_in.data)
    begin
      idata <= in_in.data;

      idata_vld <= (others => '0');
      if (in_in.byte_enable(7) = '1') then
        idata_vld(63 downto 56) <= (others => '1');
      end if;
      if (in_in.byte_enable(6) = '1') then
        idata_vld(55 downto 48) <= (others => '1');
      end if;
      if (in_in.byte_enable(5) = '1') then
        idata_vld(47 downto 40) <= (others => '1');
      end if;
      if (in_in.byte_enable(4) = '1') then
        idata_vld(39 downto 32) <= (others => '1');
      end if;
      if (in_in.byte_enable(3) = '1') then
        idata_vld(31 downto 24) <= (others => '1');
      end if;
      if (in_in.byte_enable(2) = '1') then
        idata_vld(23 downto 16) <= (others => '1');
      end if;
      if (in_in.byte_enable(1) = '1') then
        idata_vld(15 downto 8) <= (others => '1');
      end if;
      if (in_in.byte_enable(0) = '1') then
        idata_vld(7 downto 0) <= (others => '1');
      end if;
    end process;
  end generate;

  in_4byte_enable : if (to_integer(unsigned(IDATA_WIDTH_p)) = 32) generate
    process (in_in.byte_enable, in_in.data)
    begin
      idata <= in_in.data;

      idata_vld <= (others => '0');
      if (in_in.byte_enable(3) = '1') then
        idata_vld(31 downto 24) <= (others => '1');
      end if;
      if (in_in.byte_enable(2) = '1') then
        idata_vld(23 downto 16) <= (others => '1');
      end if;
      if (in_in.byte_enable(1) = '1') then
        idata_vld(15 downto 8) <= (others => '1');
      end if;
      if (in_in.byte_enable(0) = '1') then
        idata_vld(7 downto 0) <= (others => '1');
      end if;
    end process;
  end generate;

  in_2byte_enable : if (to_integer(unsigned(IDATA_WIDTH_p)) = 16) generate
    process (in_in.byte_enable, in_in.data)
    begin
      idata <= in_in.data;

      idata_vld <= (others => '0');
      if (in_in.byte_enable(1) = '1') then
        idata_vld(15 downto 8) <= (others => '1');
      end if;
      if (in_in.byte_enable(0) = '1') then
        idata_vld(7 downto 0) <= (others => '1');
      end if;
    end process;
  end generate;

  in_1byte_enable : if (to_integer(unsigned(IDATA_WIDTH_p)) = 8) generate
    process (in_in.byte_enable, in_in.data)
    begin
      idata <= in_in.data;

      idata_vld <= (others => '0');
      if (in_in.byte_enable(0) = '1') then
        idata_vld(7 downto 0) <= (others => '1');
      end if;
    end process;
  end generate;

  -----------------------------------------------------------------------------
  -- Zero Padding primitive
  -----------------------------------------------------------------------------

  zp : util_prims.util_prims.zero_padding_gen
    generic map (
      MAX_NUM_ZEROS => max(to_integer(MAX_NUM_ZEROS_p),1),
      DIN_WIDTH     => to_integer(unsigned(IDATA_WIDTH_p)),
      DOUT_WIDTH    => to_integer(unsigned(ODATA_WIDTH_p)))
    port map (
      RST_IN    => ctl_in.reset,
      CLK_IN    => ctl_in.clk,
      NUM_ZEROS => to_integer(props_in.num_zeros),
      ENABLE    => enable,
      BUSY_N    => busy_n,
      DIN       => idata,
      DIN_VLD   => idata_vld,
      REN       => out_in.ready,
      DOUT      => out_data_temp,
      DOUT_VLD  => odata_vld);

  out_out.data <= out_data_temp;

  -----------------------------------------------------------------------------
  -- Take (when up/downstream Workers ready, input data valid  &
  -- zero_padding_gen.vhd input ready)
  -----------------------------------------------------------------------------

  in_out.take <= '1' when (out_in.ready = '1' and in_in.ready = '1' and take = '1'
                     and ctl_in.is_operating = '1' and busy_n = '1') else '0';

  -----------------------------------------------------------------------------
  -- Give (when downstream Worker ready & zero_padding_gen.vhd output valid)
  -----------------------------------------------------------------------------

  out_out.give <= '1' when (out_in.ready = '1' and ctl_in.is_operating = '1'
                      and (odata_vld = '1' or force_eom = '1')) else '0';

  -----------------------------------------------------------------------------
  -- Valid (when "Give")
  -----------------------------------------------------------------------------

  out_out.valid <= '1' when (out_in.ready = '1' and odata_vld = '1') else '0';

  -----------------------------------------------------------------------------
  -- Zero-Length Message FSM
  -----------------------------------------------------------------------------

  zlm_fsm : process (ctl_in.clk)
  begin
    if rising_edge(ctl_in.clk) then
      if(ctl_in.reset = '1') then
        current_state <= INIT_s;
        take          <= '1';
        force_som     <= '0';
        force_eom     <= '0';
      else
        if props_in.messageSize = to_ulong(0) then
          messageSize <= resize(props_in.ocpi_buffer_size_out, messageSize'length);
        else
          messageSize <= resize(props_in.messageSize, messageSize'length);
        end if;
        -- defaults
        current_state <= current_state;
        take          <= '1';
        force_som     <= '0';
        force_eom     <= '0';

        case current_state is
          when INIT_s =>
            if (in_in.ready = '1' and in_in.som = '1' and in_in.eom = '1' and in_in.valid = '0') then
              current_state <= SEND_s;
            elsif (in_in.som = '1' and in_in.valid = '0') then
              current_state <= WAIT_s;
            end if;
          when WAIT_s =>
            if (in_in.ready = '1' and in_in.valid = '1') then
              current_state <= INIT_s;
            elsif (in_in.ready = '1' and in_in.eom = '1') then
              current_state <= SEND_s;
            end if;
          when SEND_s =>
            if (busy_n = '1') then
              take <= '0';
              if (msg_cnt /= 1 and out_in.ready = '1') then
                force_eom     <= '1';
              elsif (out_in.ready = '1') then
                current_state <= INIT_s;
                force_som     <= '1';
                force_eom     <= '1';
              end if;
            end if;
        end case;

      end if;
    end if;
  end process zlm_fsm;

  -----------------------------------------------------------------------------
  -- SOM/EOM - counter set to message size, increment while giving
  -----------------------------------------------------------------------------

  out_8byte_cntr : if (to_integer(unsigned(ODATA_WIDTH_p)) = 64) generate
    max_sample_cnt <= resize(messageSize srl 3, max_sample_cnt'length);
  end generate;
  out_4byte_cntr : if (to_integer(unsigned(ODATA_WIDTH_p)) = 32) generate
    max_sample_cnt <= resize(messageSize srl 2, max_sample_cnt'length);
  end generate;
  out_2byte_cntr : if (to_integer(unsigned(ODATA_WIDTH_p)) = 16) generate
    max_sample_cnt <= resize(messageSize srl 1, max_sample_cnt'length);
  end generate;
  out_1byte_cntr : if (to_integer(unsigned(ODATA_WIDTH_p)) = 8) generate
    max_sample_cnt <= resize(messageSize, max_sample_cnt'length);
  end generate;

  messageSize_count : process (ctl_in.clk)
  begin
    if rising_edge(ctl_in.clk) then
      if(ctl_in.reset = '1' or force_eom = '1') then
        msg_cnt   <= (0 => '1', others => '0');
      elsif (odata_vld = '1') then
        if(msg_cnt = max_sample_cnt) then
          msg_cnt <= (0 => '1', others => '0');
        else
          msg_cnt <= msg_cnt + 1;
        end if;
      end if;
    end if;
  end process messageSize_count;

  out_out.som <= '1' when ((out_in.ready = '1' and odata_vld = '1' and
                           msg_cnt = 1) or force_som = '1') else '0';
  out_out.eom <= '1' when ((out_in.ready = '1' and odata_vld = '1' and
                           msg_cnt = max_sample_cnt) or
                           force_eom = '1') else '0';

  out_out.byte_enable <= (others => '1');
  out_out.opcode      <= in_in.opcode;

end rtl;
