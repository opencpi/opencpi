-- An I2C generic subdevice that takes 2 or more raw property inputs.
-- Normally an I2C subdevice would be specialized for the platform
-- and for the devices it is multiplexing

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.all, ocpi.types.all; -- remove this to avoid all ocpi name collisions
architecture rtl of zipper_i2c_worker is
  constant addr_width_c   : natural := 7;
  constant SLAVE_ADDRESS  : std_logic_vector(addr_width_c-1 downto 0) := "1100000";

  signal din              : std_logic_vector(7 downto 0);
  signal dout             : std_logic_vector(7 downto 0);
  signal start            : std_logic;
  signal stop             : std_logic;
  signal read_sig         : std_logic;
  signal write_sig        : std_logic;
  signal cmd_ack          : std_logic;
  signal ack_out          : std_logic;
  signal i2c_busy         : std_logic;
  signal i2c_not_busy     : std_logic;
  signal i2c_al           : std_logic;
  signal ack_in           : std_logic;
  signal done_sig         : std_logic;
  signal error_sig        : std_logic;
  
     type state_type is (
      IDLE, ERR0, ERR1, ERR2, DONE,
      R0, R1, R2, R3, R4, R5, R6, R7, R8, R9, R10,
      W0, W1, W2, W3, W4, W5, W6, W7, W8);

   signal cstate             : state_type;
   signal nstate             : state_type;

  --OpenCores I2C Module
  component i2c_master_byte_ctrl
    port (
      clk      : in  std_logic;
      rst      : in  std_logic;
      nReset   : in  std_logic;
      ena      : in  std_logic;
      clk_cnt  : in  unsigned(15 downto 0);
      start    : in  std_logic;
      stop     : in  std_logic;
      read     : in  std_logic;
      write    : in  std_logic;
      ack_in   : in  std_logic;
      din      : in  std_logic_vector(7 downto 0);
      cmd_ack  : out std_logic;
      ack_out  : out std_logic;
      i2c_busy : out std_logic;
      i2c_al   : out std_logic;
      dout     : out std_logic_vector(7 downto 0);
      -- i2c lines
      scl_i    : in  std_logic;
      scl_o    : out std_logic;
      scl_oen  : out std_logic;
      sda_i    : in  std_logic;
      sda_o    : out std_logic;
      sda_oen  : out std_logic
      );
  end component;
  signal not_ctl_in_reset : std_logic;
  signal scl_padoen       : std_logic;
  signal sda_padoen       : std_logic;
  -- Internal signals
  signal rdata      : std_logic_vector(7 downto 0);
  signal wdata      : std_logic_vector(7 downto 0);
  signal addr       : unsigned(addr_width_c-1 downto 0);
  -- Convenience
  signal lsb        : natural range 0 to 31;
  signal raw_in     : wci.raw_prop_out_t;
  signal raw_out    : wci.raw_prop_in_t;
begin
  lsb              <= 8 * to_integer(raw_in.raw.address(1 downto 0));
  -- These are temp signals because Xilinx isim can't put them as actuals
  wdata            <= raw_in.raw.data(lsb + 7 downto lsb);
  addr             <= raw_in.raw.address(addr'range);
  raw_out.raw.data <= rdata & rdata & rdata & rdata;
  -- Use the generic raw property arbiter between the nusers inputs
  arb : wci.raw_arb
    generic map(nusers => to_integer(nusers))
    port map(
      clk         => wci_clk,
      reset       => wci_reset,
      from_users  => rprops_in,
      to_users    => rprops_out,
      from_device => raw_out,
      to_device   => raw_in);

  -- Invert reset nad output enables for opencores i2c
  not_ctl_in_reset <= not wci_reset;
  scl_oe <= not scl_padoen;
  sda_oe <= not sda_padoen;

  byte_controller : i2c_master_byte_ctrl
    port map(
      clk      => wci_clk,
      rst      => '0',
      nReset   => not_ctl_in_reset,
      ena      => '1',
      clk_cnt  => x"00C7",              -- This should be a generic
      start    => start,
      stop     => stop,
      read     => read_sig,
      write    => write_sig,
      ack_in   => ack_in,
      din      => din,
      cmd_ack  => cmd_ack,
      ack_out  => ack_out,
      i2c_busy => i2c_busy,
      i2c_al   => i2c_al,
      dout     => rdata,
      -- i2c lines
      scl_i    => scl_i,
      scl_o    => scl_o,
      scl_oen  => scl_padoen,
      sda_i    => sda_i,
      sda_o    => sda_o,
      sda_oen  => sda_padoen
      );

   ----------------------------------------------------------------------------
   -- FSM to direct byte controller:
   ----------------------------------------------------------------------------

   fsm_seq: process(wci_clk, wci_reset)
   begin
      if (wci_clk = '1') then
         cstate <= IDLE;
      elsif rising_edge(wci_clk) then
         cstate <= nstate;
      end if;
   end process;

  fsm_comb : process(cstate, raw_in.raw.is_read, raw_in.raw.is_write, raw_in.raw.address, cmd_ack, ack_out, i2c_busy)
  begin

    -- FSM Output Defaults

    read_sig  <= '0';
    write_sig <= '0';
    start     <= '0';
    stop      <= '0';
    din       <= x"00";
    done_sig  <= '0';
    error_sig <= '0';
    ack_in    <= '0';
    
    -- Next State Logic / Output Assertions

    case (cstate) is

      -- IDLE: Wait for read or write

      when IDLE =>
        if (raw_in.raw.is_write = '1') then
          nstate <= W0;
        elsif (raw_in.raw.is_read = '1') then
          nstate <= R0;
        else
          nstate <= IDLE;
        end if;

        --Wait for cmd_ack to clear
      when ERR0 =>
        if(cmd_ack = '0') then
          nstate    <= ERR1;
        else
          nstate    <= ERR0;
        end if;

        --Issue Stop and wait for acknowledgement
      when ERR1 =>
        stop <= '1';
        if(cmd_ack = '1') then
          nstate    <= ERR2;
        else
          nstate    <= ERR1;
        end if;

        --Wait for I2C busy to clear before asserting error sig
      when ERR2 =>
        if(i2c_busy = '1') then
          nstate <= ERR2;
        else
          error_sig <= '1';
          nstate <= IDLE;
        end if;
        
        -- Done
      when DONE =>
        nstate    <= IDLE;
        
        ----------------------------------------------------------
        -- READ PATH
        ----------------------------------------------------------
        -- R0: Load slave address
      when R0 =>
        din      <= SLAVE_ADDRESS & '0';
        nstate   <= R1;

        -- R1: Generate Start and Write
      when R1 =>
        write_sig <= '1';
        start     <= '1';
        din       <= SLAVE_ADDRESS & '0';
        nstate    <= R2;

        -- R2: Wait for command acknowledgement
      when R2 =>
        start     <= '1';
        din       <= SLAVE_ADDRESS & '0';
        if(cmd_ack = '1') then
          write_sig <= '0';
          if(ack_out = '0') then        --Slave acknowledged
            nstate <= R3;
          else                          --Slave did not acknowledge
            nstate <= ERR0;
          end if;
        else
          write_sig <= '1';
          nstate <= R2;
        end if;

        -- R3: Load Memory Address
      when R3 =>        
        din       <= std_logic_vector(raw_in.raw.address(din'range));
        nstate    <= R4;
        
        -- R4: Generate write
      when R4 =>
        din       <= std_logic_vector(raw_in.raw.address(din'range));
        write_sig <= '1';
        nstate    <= R5;
 
        -- R5: Wait for command acknowledgement
      when R5 =>
        din       <= std_logic_vector(raw_in.raw.address(din'range));
        if(cmd_ack = '1') then
          write_sig <= '0';
          if(ack_out = '0') then        --Slave acknowledged
            nstate <= R6;
          else                          --Slave did not acknowledge
            nstate <= ERR0;
          end if;
        else
          write_sig <= '1';
          nstate <= R5;
        end if;

        -- R6: Load slave address and read bit
      when R6 =>
        din      <= SLAVE_ADDRESS & '1';
        nstate   <= R7;

        -- R7: Generate Start and Write
      when R7 =>
        write_sig <= '1';
        start     <= '1';
        din       <= SLAVE_ADDRESS & '1';
        nstate    <= R8;

        -- R8: Wait for command acknowledgement
      when R8 =>
        start     <= '1';
        din       <= SLAVE_ADDRESS & '1';
        if(cmd_ack = '1') then
          write_sig <= '0';
          if(ack_out = '0') then        --Slave acknowledged
            nstate <= R9;
          else                          --Slave did not acknowledge
            nstate <= ERR0;
          end if;
        else
          write_sig <= '1';
          nstate <= R8;
        end if;

        -- R9: Generate Read and Nack read
      when R9 =>
        read_sig  <= '1';
        ack_in    <= '1';
        nstate    <= R10;

        -- R10: Wait for command acknowledgement
      when R10 =>
        ack_in    <= '1';
        if(cmd_ack = '1') then
          read_sig <= '0';
          if(ack_out = '1') then        --We're expected NACK because this is
                                        --the end of the transfer
            done_sig <= '1';
            nstate <= DONE;
          else                          --If we get an ACK, it's an error
            nstate <= ERR0;
          end if;
        else
          read_sig <= '1';
          nstate <= R10;
        end if;

        ----------------------------------------------------------
        -- WRITE PATH
        ----------------------------------------------------------

        -- W0: Load Slave address
      when W0 =>
        din       <= SLAVE_ADDRESS & '0';
        nstate    <= W1;
        
        -- W1: Generate Start and Write
      when W1 =>
        write_sig <= '1';
        start     <= '1';
        din       <= SLAVE_ADDRESS & '0';
        nstate    <= W2;
        
        -- W2: Wait for command acknowledgement
      when W2 =>
        start     <= '1';
        din       <= SLAVE_ADDRESS & '0';
        if(cmd_ack = '1') then
          write_sig <= '0';
          if(ack_out = '0') then        --Slave acknowledged
            nstate <= W3;
          else                          --Slave did not acknowledge
            nstate <= ERR0;
          end if;
        else
          write_sig <= '1';
          nstate <= W2;
        end if;
        
        -- W3: Load Memory Address
      when W3 =>        
        din       <= std_logic_vector(raw_in.raw.address(din'range));
        nstate    <= W4;
        
        -- W4: Generate write
      when W4 =>
        din       <= std_logic_vector(raw_in.raw.address(din'range));
        write_sig <= '1';
        nstate    <= W5;
 
        -- W5: Wait for command acknowledgement
      when W5 =>
        din       <= std_logic_vector(raw_in.raw.address(din'range));
        if(cmd_ack = '1') then
          write_sig <= '0';
          if(ack_out = '0') then        --Slave acknowledged
            nstate <= W6;
          else                          --Slave did not acknowledge
            nstate <= ERR0;
          end if;
        else
          write_sig <= '1';
          nstate <= W5;
        end if;

        -- W6: Load Data
      when W6 =>        
        din       <= wdata;
        nstate    <= W7;

        -- W7: Generate stop and write
      when W7 =>
        din       <= wdata;
        write_sig <= '1';
        stop      <= '1';
        nstate    <= W8;

        -- W8: Wait for command acknowledgement
      when W8 =>
        din       <= wdata;
        stop <= '1';
        if(cmd_ack = '1') then
          write_sig <= '0';
          if(ack_out = '0') then        --Slave acknowledged
            done_sig <= '1';
            nstate <= DONE;
          else                          --Slave did not acknowledge
            nstate <= ERR0;
          end if;
        else
          write_sig <= '1';
          nstate <= W8;
        end if;

    end case;
   end process;
  
end rtl;
