--State Machine to control the OpenCores I2C module
library IEEE;
use IEEE.std_logic_1164.all, ieee.numeric_std.all;
entity i2c_opencores_ctrl is
  generic (
    CLK_CNT : unsigned(15 downto 0) := x"00C7");
  port (
    WCI_CLK         : in  std_logic;
    WCI_RESET       : in  std_logic;
    NUMBER_OF_BYTES : in  std_logic_vector(2 downto 0);
    IS_READ         : in  std_logic;
    IS_WRITE        : in  std_logic;
    SLAVE_ADDR      : in  std_logic_vector(6 downto 0);
    ADDR            : in  std_logic_vector(7 downto 0);
    WDATA           : in  std_logic_vector(31 downto 0);
    RDATA           : out std_logic_vector(31 downto 0);
    DONE            : out std_logic;
    ERROR           : out std_logic;
    SCL_I           : in  std_logic;
    SCL_O           : out std_logic;
    SCL_OEN         : out std_logic;
    SDA_I           : in  std_logic;
    SDA_O           : out std_logic;
    SDA_OEN         : out std_logic
    );
end entity i2c_opencores_ctrl;
architecture rtl of i2c_opencores_ctrl is
  signal not_ctl_in_reset  : std_logic := '0';
  signal din               : std_logic_vector(7 downto 0);
  signal dout              : std_logic_vector(7 downto 0);
  signal start             : std_logic;
  signal stop              : std_logic;
  signal read_sig          : std_logic;
  signal write_sig         : std_logic;
  signal cmd_ack           : std_logic;
  signal ack_out           : std_logic;
  signal i2c_busy          : std_logic;
  signal i2c_not_busy      : std_logic;
  signal i2c_al            : std_logic;
  signal ack_in            : std_logic;
  signal start_of_transfer : std_logic;
  signal load_data         : std_logic;
  signal bytes_remaining_r : unsigned(2 downto 0);
  signal lsb               : natural range 0 to 31;

  type state_type is (idle_s, err0_s, err1_s, err2_s, done0_s,
                      rw0_s, rw1_s, rw2_s, rw3_s,
                      r4_s, r5_s, r6_s, r7_s, r8_s,
                      w4_s, w5_s, w6_s, w7_s, w8_s);

  signal current_state : state_type;
  signal next_state    : state_type;
begin
  -- Invert reset for opencores i2c
  not_ctl_in_reset <= not WCI_RESET;

  byte_controller : work.i2c.i2c_master_byte_ctrl
    port map(
      CLK      => WCI_CLK,
      RST      => '0',
      NRESET   => not_ctl_in_reset,
      ENA      => '1',
      CLK_CNT  => std_logic_vector(CLK_CNT),
      START    => start,
      STOP     => stop,
      READ     => read_sig,
      WRITE    => write_sig,
      ACK_IN   => ack_in,
      DIN      => din,
      CMD_ACK  => cmd_ack,
      ACK_OUT  => ack_out,
      I2C_BUSY => i2c_busy,
      I2C_AL   => i2c_al,
      DOUT     => dout,
      SCL_I    => SCL_I,
      SCL_O    => SCL_O,
      SCL_OEN  => SCL_OEN,
      SDA_I    => SDA_I,
      SDA_O    => SDA_O,
      SDA_OEN  => SDA_OEN
      );

   ----------------------------------------------------------------------------
   -- FSM to direct byte controller:
   ----------------------------------------------------------------------------
   fsm_seq: process(WCI_CLK)
   begin
      if rising_edge(WCI_CLK) then
        if (WCI_RESET = '1') then
         current_state <= idle_s;
        else
         current_state <= next_state;
        end if;
      end if;
   end process;

   bytes_remaining_proc: process(WCI_CLK)
   begin
      if rising_edge(WCI_CLK) then
        if (WCI_RESET = '1') then
         bytes_remaining_r <= (others => '0');
         RDATA <= (others => '0');
        else
          if(start_of_transfer='1') then
            bytes_remaining_r <= unsigned(NUMBER_OF_BYTES);
            RDATA <= (others => '0');
          elsif(load_data = '1') then
            bytes_remaining_r <= bytes_remaining_r - 1;
            RDATA(31-lsb downto 31-lsb-7) <= dout;
          else
            bytes_remaining_r <= bytes_remaining_r;
          end if;
        end if;
      end if;
   end process;

  --Calculate lsb
  lsb <= 8*to_integer(bytes_remaining_r)-8;

  fsm_comb : process(current_state, IS_READ, IS_WRITE, ADDR, cmd_ack, ack_out, i2c_busy, bytes_remaining_r, lsb)
  begin
    -- FSM Output Defaults
    read_sig          <= '0';
    write_sig         <= '0';
    start             <= '0';
    stop              <= '0';
    din               <= x"00";
    DONE              <= '0';
    ERROR             <= '0';
    ack_in            <= '0';
    start_of_transfer <= '0';
    load_data         <= '0';

    -- Next State Logic / Output Assertions
    case (current_state) is
      -- idle_s: Wait for read or write
      when idle_s =>
        if (is_write = '1' or is_read = '1') then
          next_state        <= rw0_s;
          start_of_transfer <= '1';
        else
          next_state <= idle_s;
        end if;
        --Wait for cmd_ack to clear
      when err0_s =>
        if(cmd_ack = '0') then
          next_state <= err1_s;
        else
          next_state <= err0_s;
        end if;
        --Issue Stop and wait for acknowledgement
      when err1_s =>
        stop <= '1';
        if(cmd_ack = '1') then
          next_state <= err2_s;
        else
          next_state <= err1_s;
        end if;
        --Wait for I2C busy to clear before asserting error sig
      when err2_s =>
        if(i2c_busy = '1') then
          next_state <= err2_s;
        else
          ERROR  <= '1';
          next_state <= idle_s;
        end if;
        -- DONE
      when done0_s =>
        DONE   <= '1';
        next_state <= idle_s;
        ----------------------------------------------------------
        -- STATES COMMON TO READ AND WRITE PATHS
        ----------------------------------------------------------
        -- rw0_s: Generate Start and Write for Slave Address and not Read bit
      when rw0_s =>
        write_sig <= '1';
        start     <= '1';
        din       <= SLAVE_ADDR & '0';
        next_state    <= rw1_s;
        -- rw1_s: Wait for command acknowledgement
      when rw1_s =>
        din <= SLAVE_ADDR & '0';
        if(cmd_ack = '1') then
          start     <= '0';
          write_sig <= '0';
          if(ack_out = '0') then        --Slave acknowledged
            next_state <= rw2_s;
          else                          --Slave did not acknowledge
            next_state <= err0_s;
          end if;
        else
          start     <= '1';
          write_sig <= '1';
          next_state    <= rw1_s;
        end if;
        -- rw2_s: Generate write for Memory Address
      when rw2_s =>
        din       <= ADDR;
        write_sig <= '1';
        next_state    <= rw3_s;
        -- rw3_s:  Wait for command acknowledgement
      when rw3_s =>
        din <= ADDR;
        if(cmd_ack = '1') then
          write_sig <= '0';
          if(ack_out = '0') then        --Slave acknowledged
            if(is_read='1') then
              next_state <= r4_s;
            else
              next_state <= w4_s;
            end if;
          else                          --Slave did not acknowledge
            next_state <= err0_s;
          end if;
        else
          write_sig <= '1';
          next_state    <= rw3_s;
        end if;
        ----------------------------------------------------------
        -- READ PATH
        ----------------------------------------------------------
        -- r4_s: Generate Start and Write for Slave Address and Read bit
      when r4_s =>
        write_sig <= '1';
        start     <= '1';
        din       <= SLAVE_ADDR & '1';
        next_state    <= r5_s;
        -- r5_s: Wait for command acknowledgement
      when r5_s =>
        din <= SLAVE_ADDR & '1';
        if(cmd_ack = '1') then
          write_sig <= '0';
          start     <= '0';
          if(ack_out = '0') then        --Slave acknowledged
            next_state <= r6_s;
          else                          --Slave did not acknowledge
            next_state <= err0_s;
          end if;
        else
          start     <= '1';
          write_sig <= '1';
          next_state    <= r5_s;
        end if;
        -- r6_s: Generate Read, and Nack if 1 byte read
      when r6_s =>
        read_sig <= '1';
        if(bytes_remaining_r = 1) then
          next_state <= r7_s;
          ack_in <= '1';
        else
          next_state <= r8_s;
          ack_in <= '0';
        end if;
        -- r7_s: Wait for command acknowledgement
      when r7_s =>
        ack_in <= '1';
        if(cmd_ack = '1') then
          load_data <= '1';
          read_sig <= '0';
          if(ack_out = '1') then        --We're expected NACK because this is
                                        --the end of the transfer
            next_state <= done0_s;
          else                          --If we get an ACK, it's an error
            next_state <= err0_s;
          end if;
        else
          read_sig <= '1';
          next_state   <= r7_s;
        end if;
        -- r8_s: Wait for command acknowledgement
      when r8_s =>
        if(cmd_ack = '1') then
          load_data <= '1';
          read_sig <= '0';
          next_state   <= r6_s;
        else
          read_sig <= '1';
          next_state   <= r8_s;
        end if;
        ----------------------------------------------------------
        -- WRITE PATH
        ----------------------------------------------------------
        -- w4_s: Split on num_bytes
      when w4_s =>
        din <= WDATA(lsb+7 downto lsb);
        if(bytes_remaining_r = 1) then
          next_state <= w5_s;
        else
          next_state <= w7_s;
        end if;
        -- w5_s: Generate stop and write
      when w5_s =>
        din       <= WDATA(lsb+7 downto lsb);
        write_sig <= '1';
        stop      <= '1';
        next_state    <= w6_s;
        -- w6_s: Wait for command acknowledgement
      when w6_s =>
        din  <= WDATA(lsb+7 downto lsb);
        stop <= '1';
        if(cmd_ack = '1') then
          write_sig <= '0';
          if(ack_out = '0') then        --Slave acknowledged
            next_state <= done0_s;
          else                          --Slave did not acknowledge
            next_state <= err0_s;
          end if;
        else
          write_sig <= '1';
          next_state    <= w6_s;
        end if;
        -- w7_s: Generate write
      when w7_s =>
        din       <= WDATA(lsb+7 downto lsb);
        write_sig <= '1';
        next_state    <= w8_s;
        -- w8_s: Wait for command acknowledgement
      when w8_s =>
        din <= WDATA(lsb+7 downto lsb);
        if(cmd_ack = '1') then
          load_data <= '1';
          write_sig <= '0';
          if(ack_out = '0') then        --Slave acknowledged
            next_state <= w4_s;
          else                          --Slave did not acknowledge
            next_state <= err0_s;
          end if;
        else
          write_sig <= '1';
          next_state    <= w8_s;
        end if;
    end case;
  end process;
end rtl;
