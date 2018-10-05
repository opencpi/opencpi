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


-------------------------------------------------------------------------------
-- Capture_v2
-------------------------------------------------------------------------------
--
-- Description:
--
-- The capture_v2 worker takes input port messages and stores their data in a
-- buffer via the data property as 4 byte words. Metadata associated with each
-- message is stored as a record in a metadata buffer via the metadata property.
-- It captures four 4 byte words of metadata.  The first metadata word is the
-- opcode for the message and message size in bytes; opcode 8 MSB and message
-- size 24 LSB. The second word is the fraction time stamp for the EOM. The
-- third word is the fraction time stamp for the SOM. And the fourth word is
-- the seconds timestamp for the SOM. So that the metadata can be read on a
-- little-endian processor, the ordering of the metadata is as follows:
-- opcode (8-bit),
-- message size (24-bit),
-- eom fraction (32-bit),
-- som fraction (32-bit),
-- som seconds (32-bit)
--
-- When the number bytes sent for a message is not a multiple of 4, only the
-- last (number of bytes modulus 4) least significant bytes of the last word
-- in the data buffer represent received data. The (number of bytes modulus 4)
-- most significant bytes will always be zero in the last word in the data
-- buffer. For example, if number of bytes is 5 and the last captured data buffer
-- word is 0x0000005a, the last data received by capture_v2 was 0x5a. If number
-- if bytes was 6, the last data received was 0x005a.
--
-- The capture_v2 worker keeps count of how many metadata records (metadataCount)
-- have been captured and a count of how many data words have been captured (dataCount).
-- It allows for the option to wrap around and continue to capture data and
-- metadata once the buffers are full or to stop capturing data and metadata
-- when the data and metadata buffers are full via the stoponFull property.
--
-- When stopOnFull is true, data and metadata will be captured as long as the
-- data and metadata buffers are not full. If the data buffer is full before the
-- metadata buffer, metadata will still be captured until the metadata buffer is
-- full. If the metadata buffer is full before the data buffer, no more metadata
-- and no more data will be captured. When stopOnFull is false, there will be a
-- wrap around when the data and metadata buffers are full and data metadata will
-- continue to be captured.
--
-- The worker also has properties that keep track of whether or not the
-- metadata and data buffers are full; metaFull and dataFull.
--
-- It has an (optional) output port so that it can placed between two workers.
-- The input message are directly passed to the output port with a latency of
-- the control plane clock cycles.

-------------------------------------------------------------------------------
-- TODO - Add a "trigger" capability that will allow the capture of data based on
-- on a condition, i.e, "start capturing when you a second zlm has occurred or
-- start capturing when an opcode of 5 has been seen"
-------------------------------------------------------------------------------

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.util.all; use ocpi.types.all; -- remove this to avoid all ocpi name collisions
library util; use util.util.all;

architecture rtl of capture_v2_worker is

-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
-- Signals for capture logic
-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
signal stopOnFull       : boolean;
signal metadataCount    : ulong_t := (others => '0');
signal dataCount        : ulong_t := (others => '0');
signal metaFull         : boolean := false;
signal dataFull         : boolean := false;
signal bytes            : integer := 0; -- Number of bytes decoded from byte enable
signal last_bytes       : integer := 0; -- Stores previous value of bytes signal to use for trailing eom logic
signal valid_bytes      : std_logic_vector(31 downto 0) := (others => '0'); -- Valid bytes determined from byte enable
signal msgLen           : unsigned(23 downto 0) := (others => '0'); -- Used to get the number of data stored for each message and then is multiplied by "bytes" to get the message length in bytes
signal last_msgLen      : unsigned(23 downto 0) := (others => '0'); -- Stores previous value of msgLen signal to use for trailing eom logic
signal total_bytes      : std_logic_vector(23 downto 0) := (others => '0'); -- Total number of bytes sent during a message
signal som_seconds      : std_logic_vector(31 downto 0) := (others => '0'); -- Seconds time stamp captured for som
signal som_fraction     : std_logic_vector(31 downto 0) := (others => '0'); -- Fraction time stamp captured for som
signal som_seconds_r    : std_logic_vector(31 downto 0) := (others => '0'); -- Latched value of seconds time stamp captured for som
signal som_fraction_r   : std_logic_vector(31 downto 0) := (others => '0'); -- Latched value of fraction time stamp captured for som
signal finished         : std_logic := '0';   -- Used to drive ctl_out.finished
-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
-- Signals for combinatorial logic
-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
signal dataDisable       : std_logic := '0'; -- When stopOnFull is true, used to disable writing to data BRAM when data is full
signal metadataDisable   : std_logic := '0'; -- When stopOnFull is true, used to disable writing to metadata BRAM and data BRAM when metadata is full
signal eom               : std_logic := '0'; -- Used for counting eoms
signal som               : std_logic := '0'; -- Used for counting soms
signal zlm               : std_logic := '0'; -- Used for counting zlms
signal s_is_read_r       : std_logic := '0'; -- Register to aid in incrementing the BRAM read side addresses
signal s_eom_r           : std_logic := '0'; -- Register to aid in detecting multi clock cycle eom. Used for combinatorial logic to drive eom signal
signal s_som_r           : std_logic := '0'; -- Register to aid in detecting multi clock cycle som. Used for combinatorial logic to drive som signal
signal s_take_r          : std_logic := '0'; -- Register used for combinatorial logic to drive eom and som signals
signal s_in_ready_r      : std_logic := '0'; -- Register used for combinatorial logic to drive capture_vld signal
signal capture_vld       : std_logic := '0'; -- Used to determine if data being captured is valid
signal outNotConnected   : std_logic := '0';
-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
-- Data BRAM constants and signals
-- Writing to the BRAM B side and then reading from the BRAM A side
-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
constant data_memory_depth_c  : natural := to_integer(numDataWords);
constant data_addr_width_c    : natural := width_for_max(data_memory_depth_c - 1);
constant data_bram_dsize_c    : natural := 32; -- Size in bytes of the data for data bram
signal data_bramW_in          : std_logic_vector(31 downto 0) := (others => '0');
signal data_bramR_addr        : unsigned(data_addr_width_c-1 downto 0) := (others => '0');
signal data_bramW_write       : std_logic := '0';
signal data_bramR_out         : std_logic_vector(31 downto 0) := (others => '0');
signal data_bramW_addr        : unsigned(data_addr_width_c-1 downto 0) := (others => '0');
-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
-- Metadata BRAM constants and signals
-- Writing to the BRAM B side and then reading from the BRAM A side
-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
constant metadata_memory_depth_c  : natural := to_integer(numRecords);
constant metadata_addr_width_c    : natural := width_for_max(metadata_memory_depth_c - 1);
constant metadata_bram_dsize_c    : natural := 128; -- Size in bytes of the data for metadata bram
signal metadata_bramW_in          : std_logic_vector(127 downto 0) := (others => '0');
signal metadata_bramR_out         : std_logic_vector(127 downto 0) := (others => '0');
signal metadata_bramW_write       : std_logic := '0';
signal metadata_bramR_addr        : unsigned(metadata_addr_width_c-1 downto 0) := (others => '0');
signal metadata_bramW_addr        : unsigned(metadata_addr_width_c-1 downto 0) := (others => '0');
signal metadata                   : std_logic_vector(31 downto 0) := (others => '0'); -- Used to store metadata words from metadata_bramR_out after doing some decoding
constant counter_size             : natural := width_for_max(3); -- Size in bytes for metadata_bramR_ctr
signal metadata_bramR_ctr         : unsigned(counter_size-1 downto 0) := (others => '0'); -- Used for the data_bramR_addr_counter process and metadata decoding
-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
-- Mandatory input port logic
-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
signal ready_for_in_port_data  : std_logic := '0';
-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
-- Mandatory output port logic
-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
signal data_ready_for_out_port : std_logic := '0';
signal out_meta_is_reserved    : std_logic := '0';
signal out_som                 : std_logic := '0';
signal out_eom                 : std_logic := '0';
signal out_valid               : std_logic := '0';
signal take                    : std_logic := '0';

begin

-- Mandatory input port logic, (note that
-- ready_for_in_port_data MUST have the same clock latency as the latency
-- between in port's data and any associated output port's data)
in_out.take <= take;

take <= ctl_in.is_operating and in_in.ready and
              ready_for_in_port_data; -- this applies backpressure

-- Mandatory output port logic, (note that
-- data_ready_for_out_port MUST be clock-aligned with out_out.data)
-- (note that reserved messages will be DROPPED ON THE FLOOR)
out_out.give <= ctl_in.is_operating and out_in.ready and
              (not out_meta_is_reserved) and data_ready_for_out_port;
out_meta_is_reserved <= (not out_som) and (not out_valid) and (not out_eom);
out_out.som   <= out_som;
out_out.eom   <= out_eom;
out_out.valid <= out_valid;

out_som <= in_in.som;
out_eom <= in_in.eom;
out_valid <= in_in.valid;


ready_for_in_port_data <= out_in.ready or outNotConnected;
data_ready_for_out_port <= in_in.ready;

outNotConnected <= out_in.reset; -- When an optional output port is not connected, out_in.reset will always be '1'

out_out.data <= in_in.data;
out_out.byte_enable <= in_in.byte_enable;
out_out.opcode   <= in_in.opcode;

ctl_out.finished  <= finished;
props_out.metadataCount <= metadataCount;
props_out.dataCount <= dataCount;
props_out.metaFull <= to_bool(metaFull);
props_out.dataFull <= to_bool(dataFull);

capture_vld <= ctl_in.is_operating and in_in.valid and ((in_in.ready and (not s_in_ready_r))
                or (in_in.ready and s_take_r));

stopOnFull <= to_boolean(props_in.stopOnFull);


eom <= (in_in.eom and (not s_eom_r)) or (in_in.eom and s_take_r);
som <= (in_in.som and (not s_som_r)) or (in_in.som and s_take_r);


-- If som and eom occur a the same time, use the non latched seconds timestamp
-- otherwise use the latched version of the timestamp
som_seconds <= std_logic_vector(time_in.seconds) when (its((som and eom))) else
               som_seconds_r;

-- If som and eom occur a the same time, use the non latched fraction timestamp
-- otherwise use the latched version of the timestamp
som_fraction <= std_logic_vector(time_in.fraction) when (its((som and eom))) else
                som_fraction_r;


props_out.raw.done  <= '1';
props_out.raw.error <= '0';

data_bramW_in <= std_logic_vector(in_in.data and valid_bytes);

-- Write to data BRAM when data is valid, when metadata and
-- data not full (when stopOnFull = true), and not finished.
data_bramW_write <= capture_vld and (not dataDisable) and (not metadataDisable) and (not finished);

-- Total bytes sent for a message. Also handles trailing eom.
total_bytes <= std_logic_vector(resize(last_msgLen*last_bytes, 24))
               when (its((not in_in.som) and in_in.eom and (not in_in.valid))) else
              std_logic_vector(resize(msgLen*bytes, 24));

-- Metadata to place into metadata BRAM
metadata_bramW_in <= in_in.opcode  & total_bytes & std_logic_vector(time_in.fraction) & som_fraction & som_seconds
                    when (its(eom)) else
                    (others => '0');

-- Write to metadata BRAM when there is an eom and not finished. And if stopOnFull is true write when metadata not full.
metadata_bramW_write <= eom and (not finished) and (not metadataDisable);


-- Since metadata is the first raw property, assign raw.data to metadata until finished reading in metadata and then assign raw.data to the data
-- also divide the raw.address by 4 since raw.address increments in steps of 4.
-- Mulitplying numRecords by 4 because there are 4 metadata words
props_out.raw.data <= metadata when (to_integer(props_in.raw.address)/4 < 4*numRecords) else data_bramR_out;

-- Decode to get each metadata word
metadata <= metadata_bramR_out(127 downto 96) when (metadata_bramR_ctr = 0) else -- first metadata word
            metadata_bramR_out(95 downto 64) when (metadata_bramR_ctr = 1) else  -- second metadata word
            metadata_bramR_out(63 downto 32) when (metadata_bramR_ctr = 2) else  -- third metadata word
            metadata_bramR_out(31 downto 0) when (metadata_bramR_ctr = 3);       -- fourth metadata word


-- What each byte_enable value means in number of bytes
bytes <= 4 when in_in.byte_enable(3) = '1' else
         3 when in_in.byte_enable(2) = '1' else
         2 when in_in.byte_enable(1) = '1' else
         1 when in_in.byte_enable(0) = '1' else
         0;

valid_bytes(31 downto 24) <= (others => in_in.byte_enable(3));
valid_bytes(23 downto 16) <= (others => in_in.byte_enable(2));
valid_bytes(15 downto  8) <= (others => in_in.byte_enable(1));
valid_bytes( 7 downto  0) <= (others => in_in.byte_enable(0));


-- Use zlm detector primitive to detect zlms and multicycle zlms
zlm_detector : util.util.zlm_detector
  port map (
    clk => ctl_in.clk,
    reset => ctl_in.reset,
    som => in_in.som,
    valid => in_in.valid,
    eom => in_in.eom,
    ready => in_in.ready,
    take => take,
    eozlm_pulse => zlm,
    eozlm => open);


dataBram : component util.util.BRAM2
generic map(PIPELINED  => 0,
            ADDR_WIDTH => data_addr_width_c,
            DATA_WIDTH => data_bram_dsize_c,
            MEMSIZE    => data_memory_depth_c)
  port map   (CLKA       => ctl_in.clk,
              ENA        => '1',
              WEA        => '0',
              ADDRA      => std_logic_vector(data_bramR_addr),
              DIA        => x"00000000",
              DOA        => data_bramR_out,
              CLKB       => ctl_in.clk,
              ENB        => '1',
              WEB        => data_bramW_write,
              ADDRB      => std_logic_vector(data_bramW_addr),
              DIB        => data_bramW_in,
              DOB        => open);

  metadataBram : component util.util.BRAM2
  generic map(PIPELINED  => 0,
              ADDR_WIDTH => metadata_addr_width_c,
              DATA_WIDTH => metadata_bram_dsize_c,
              MEMSIZE    => metadata_memory_depth_c)
    port map   (CLKA       => ctl_in.clk,
                ENA        => '1',
                WEA        => '0',
                ADDRA      => std_logic_vector(metadata_bramR_addr),
                DIA        => x"00000000000000000000000000000000",
                DOA        => metadata_bramR_out,
                CLKB       => ctl_in.clk,
                ENB        => '1',
                WEB        => metadata_bramW_write,
                ADDRB      => std_logic_vector(metadata_bramW_addr),
                DIB        => metadata_bramW_in,
                DOB        => open);

-- Running counter to keep track of data BRAM reading side address
data_bramR_addr_counter : process (ctl_in.clk)
begin
  if rising_edge(ctl_in.clk) then
    if ctl_in.reset = '1' then
      data_bramR_addr  <= (others => '0');
    -- Increment data_bramR_addr only when ready to read in data
    elsif ((to_integer(props_in.raw.address)/4 >= 4*numRecords) and props_in.raw.is_read = '1' and s_is_read_r = '0') then
      if (data_bramR_addr = numDataWords-1) then
        data_bramR_addr <= (others => '0');
      else
        data_bramR_addr <= data_bramR_addr + 1;
      end if;
    end if;
  end if;
end process data_bramR_addr_counter;


-- Counter used to decode metadata and to advance metadata_bramR_addr
metadata_bramR_counter : process (ctl_in.clk)
begin
  if rising_edge(ctl_in.clk) then
    if ctl_in.reset = '1' then
      metadata_bramR_ctr <= (others => '0');
    -- Increment metadata_bramR_ctr only when ready to read in metadata
    elsif ((to_integer(props_in.raw.address)/4 < 4*numRecords) and props_in.raw.is_read = '1' and s_is_read_r = '1') then
          metadata_bramR_ctr <= metadata_bramR_ctr + 1;
    end if;
  end if;
end process metadata_bramR_counter;

-- Running counter to keep track of metadata BRAM reading side address
metadata_bramR_addr_counter : process (ctl_in.clk)
begin
  if rising_edge(ctl_in.clk) then
    if ctl_in.reset = '1' then
      metadata_bramR_addr  <= (others => '0');
    -- Increment metadata_bramR_addr only when ready to read in metadata
    elsif ((to_integer(props_in.raw.address)/4 < 4*numRecords) and props_in.raw.is_read = '1' and s_is_read_r = '1') then
    -- Reset metadata_bramR_addr to 0 when on the last metadata word
        if (metadata_bramR_addr = numRecords-1 and metadata_bramR_ctr = 3) then
          metadata_bramR_addr <= (others => '0');
        elsif (metadata_bramR_ctr = 3) then
          metadata_bramR_addr <= metadata_bramR_addr + 1;
        end if;
    end if;
  end if;
end process metadata_bramR_addr_counter;

-- Process for getting the latched version of the fraction and seconds timestamps when a som occurs
som_timestamp_reg : process(ctl_in.clk)
begin
  if rising_edge(ctl_in.clk) then
    if ctl_in.reset = '1' then
      som_seconds_r <= (others => '0');
      som_fraction_r <= (others => '0');
    elsif (finished = '0' and som = '1') then
      som_seconds_r <= std_logic_vector(time_in.seconds);
      som_fraction_r <= std_logic_vector(time_in.fraction);
    end if;
  end if;
end process som_timestamp_reg;

-- This process sets the  s_is_read_r, s_eom_r, s_som_r s_take_r registers
set_regs : process (ctl_in.clk)
begin
    if rising_edge(ctl_in.clk) then
      if ctl_in.reset = '1' then
        s_is_read_r <= '0';
        s_eom_r <= '0';
        s_som_r <= '0';
        s_take_r <= '0';
        s_in_ready_r <= '0';
      else
        s_is_read_r <= props_in.raw.is_read;
        s_eom_r <= in_in.eom;
        s_som_r <= in_in.som;
        s_take_r <= take;
        s_in_ready_r <= in_in.ready;
      end if;
    end if;
end process set_regs;

-- This process has the logic for incrementing the dataCount, msgLen, data_bramW_addr, and setting dataDisable.
data_capture : process (ctl_in.clk)
begin
    if rising_edge(ctl_in.clk) then
      if ctl_in.reset = '1' then
        dataCount <= (others => '0');
        data_bramW_addr <= (others => '0');
        msgLen <= (0 =>'1', others =>'0');
        dataDisable <= '0';
        last_bytes <= 0;
        last_msgLen <= (others => '0');
      elsif (capture_vld = '1' and finished = '0') then
        -- Take data while buffer is in wrapping mode (stopOnFull=false) or continuous (stopOnFull=true) and data and metadata counts
        -- have not reached their respective maximum.

          -- Increment data count while data is not full and metadata not full
          if (dataCount < numDataWords and metadataCount < numRecords and stopOnFull = true) then
            dataCount <= dataCount + 1;
          end if;

          -- If stopOnFull is true and data will be full set dataDisable to 1 to stop writing to BRAM
          if ((dataCount + 1 = numDataWords) and stopOnFull = true) then
            dataDisable <= '1';
          -- If stopOnFull is true, increment data_bramW_addr and dataCount if data_bramW_addr and metadataCount have not reached their respective maximum
          elsif (stopOnFull = true and data_bramW_addr + 1 < numDataWords and metadataCount < numRecords) then
            data_bramW_addr <= data_bramW_addr + 1;
          -- Wrap around if data will be full and stopOnFull is false
          elsif ((data_bramW_addr + 1 = numDataWords) and stopOnFull = false) then
            data_bramW_addr <= (others => '0');
            dataCount <= dataCount + 1;
          -- If stopOnFull is false increment data_bramW_addr and data count
          elsif stopOnFull = false then
            data_bramW_addr <= data_bramW_addr + 1;
            dataCount <= dataCount + 1;
          end if;
          msgLen <= msgLen + 1;
          last_bytes <= bytes;
          last_msgLen <= msgLen;
      end if;
      -- Reset msgLen after each message
      if eom = '1' then
        msgLen <= (0 =>'1', others =>'0');
      end if;
    end if;
  end process data_capture;

  -- This process has the logic for incrementing the metadataCount, metadata_bramW_addr, and setting metadataDisable.
  metadata_capture : process (ctl_in.clk)
  begin
    if rising_edge(ctl_in.clk) then
      if ctl_in.reset = '1' then
        metadataCount <= (others => '0');
        metadata_bramW_addr <= (others => '0');
        metadataDisable <= '0';
      -- Store metadata after each message or if stopOnFull is true store metadata until data full or metadata full and not finished
      elsif (finished = '0' and -- not finished
        ((stopOnFull = false and eom = '1')  or  -- Case when wrap around is enabled and there is an eom
        (eom = '1' and stopOnFull = true and metadataCount < numRecords))) -- Case when there's an eom, wrap not enabled, and metadata and data not full
         then
          -- If stopOnFull is true and data will be full, set metadataDisable to 1 to stop writing to BRAM
          if ((metadataCount + 1 = numRecords) and stopOnFull = true) then
            metadataDisable <= '1';
          -- If stopOnFull is true, increment metadata_bramW_addr if metadata_bramW_addr has not reached its maximum
          elsif (stopOnFull = true and metadata_bramW_addr + 1 < numRecords) then
            metadata_bramW_addr <= metadata_bramW_addr + 1;
          -- Wrap around if metadata will be full and stopOnFull is false
          elsif ((metadata_bramW_addr + 1 = numRecords) and stopOnFull = false) then
            metadata_bramW_addr <= (others => '0');
          elsif stopOnFull = false then
            metadata_bramW_addr <= metadata_bramW_addr + 1;
          end if;
          metadataCount <= metadataCount + 1;
      end if;
    end if;
  end process metadata_capture;

  -- The process controls setting the dataFull and metafull properties.
  buffer_full : process (ctl_in.clk)
  begin
    if rising_edge(ctl_in.clk) then
      if ctl_in.reset = '1' then
        dataFull <= false;
        metaFull <=false;
      else
        -- Indicates data buffer is full
        if (dataCount = numDataWords) then
          dataFull <= true;
        end if;
        -- Indicates metadata buffer is full
        if (metadataCount = numRecords) then
          metaFull <= true;
        end if;
      end if;
    end if;
  end process buffer_full;

  -- This process controls when the worker is finished
  finish : process (ctl_in.clk)
  begin
      if rising_edge(ctl_in.clk) then
        if ctl_in.reset = '1' then
          finished <= '0';
        -- For stopOnEOF, if there is a zlm, the input opcode is equal to 0, and stopOnEOF is true then set finished to true
        -- For stopOnZLM, if there is a zlm, the input opcode is equal to stopZLMOpcode, and stopOnZLM is true then set finished to true
        elsif (zlm = '1' and ((props_in.stopOnEOF and unsigned(in_in.opcode) = 0) or (to_uchar(in_in.opcode) = props_in.stopZLMOpcode and props_in.stopOnZLM))) then
          finished <= '1';
        end if;
      end if;
 end process finish;

end rtl;
