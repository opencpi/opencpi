-- THIS FILE WAS ORIGINALLY GENERATED ON Tue Jun  3 18:13:19 2014 EDT
-- BASED ON THE FILE: limexcvrzipperspi_dw.xml
-- YOU *ARE* EXPECTED TO EDIT IT
-- This file initially contains the architecture skeleton for worker: limexcvrzipperspi_dw

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions
use ocpi.wci.all;
architecture rtl of limexcvrzipperspi_dw_worker is
  signal spiDataIn       : std_logic_vector(15 downto 0);
  signal spiDataOut      : std_logic_vector(15 downto 0);
  signal spiBusy         : std_logic;
  signal spiNotBusy      : std_logic;
  signal spiNotBusy_z1   : std_logic;
  signal spiNotBusy_edge : std_logic;
  signal spiStart        : std_logic_vector(0 downto 0);
  signal dataToWrite     : std_logic_vector(7 downto 0);
  signal dataToWriteReg  : std_logic_vector(7 downto 0);
  signal raw_is_read_z1  : std_logic;
  signal readStart       : std_logic;
  signal readEdge        : std_logic;
  signal raw_is_write_z1 : std_logic;
  signal writeStart      : std_logic;
  signal writeEdge       : std_logic;
  signal sdo_sim         : std_logic;
  signal byte_enable_z1  : std_logic_vector(3 downto 0);
  signal read_be_latch   : std_logic_vector(3 downto 0);

begin

  lime_reset <= '1';
  
    LimeReg_l : entity work.spi_register              --50MHz Max
    generic map(
      SPI_NUMBER_CS        => 1,
      SPI_DATA_WIDTH       => 16,
      SPI_CLK_DOWNSAMPLING => 16
      )
    port map (
      -- SPI Interface
      i_clk_p         => ctl_in.clk,
      i_rst_p         => ctl_in.reset,
      iv_spiStart_p   => spiStart,
      i_spiClkPol_p   => '1',
      i_spiMsbf_p     => '1',
      iv_spiDataIn_p  => spiDataIn,
      ov_spiDataOut_p => spiDataOut,
      o_spiBusy_p     => spiBusy,

      o_spiClkOutPin_p => sclk,
      ov_spiCsPin_p(0) => sen,
      o_spiMosiPin_p   => sdio,
      --i_spiMisoPin_p   => sdo_sim
      i_spiMisoPin_p   => sdo
      );

  --Start
  --Double read/write pulses, requires for spi module
  process (ctl_in.clk, ctl_in.reset)
  begin
    if (ctl_in.reset = '1') then
      raw_is_read_z1 <= '0';
    elsif rising_edge(ctl_in.clk) then
      raw_is_read_z1 <= props_in.raw_is_read;
    end if;
  end process;
  readStart <= '1' when (props_in.raw_is_read or raw_is_read_z1='1') else '0';
  readEdge <= (props_in.raw_is_read and not raw_is_read_z1);

  process (ctl_in.clk, ctl_in.reset)
  begin
    if (ctl_in.reset = '1') then
      raw_is_write_z1 <= '0';
    elsif rising_edge(ctl_in.clk) then
      raw_is_write_z1 <= props_in.raw_is_write;
    end if;
  end process;
  writeStart <= '1' when (props_in.raw_is_write or raw_is_write_z1='1') else '0';
  writeEdge <= (props_in.raw_is_write and not raw_is_write_z1);
  
  spiStart(0) <= '1' when (readStart='1' or writeStart='1') else '0';

  --Concatenate Data In vector
  be : process (props_in.raw_byte_enable)
  begin
    case (props_in.raw_byte_enable) is
      when "0001"   => dataToWrite <= props_in.raw_data(7 downto 0);
      when "0010"   => dataToWrite <= props_in.raw_data(15 downto 8);  
      when "0100"   => dataToWrite <= props_in.raw_data(23 downto 16);  
      when "1000"   => dataToWrite <= props_in.raw_data(31 downto 24);  
      when others => dataToWrite <= (others => '0');
    end case;
  end process be;

  --Register dataToWrite
  process (ctl_in.clk, ctl_in.reset)
  begin
    if (ctl_in.reset = '1') then
      dataToWriteReg <= (others => '0');
    elsif rising_edge(ctl_in.clk) then
      dataToWriteReg <= dataToWrite;
    end if;
  end process;

  --Concatenate Data Out vector
  --register byte enable
  process (ctl_in.clk, ctl_in.reset)
  begin
    if (ctl_in.reset = '1') then
       byte_enable_z1 <= (others => '0');
    elsif rising_edge(ctl_in.clk) then
       byte_enable_z1 <= props_in.raw_byte_enable;
    end if;
  end process;
  
  --read byte_enable latch
  process (ctl_in.clk, ctl_in.reset)
  begin
    if (ctl_in.reset = '1') then
      read_be_latch <= (others => '0');
    elsif rising_edge(ctl_in.clk) then
      if(readEdge='1') then
       read_be_latch <= byte_enable_z1;
      end if;
    end if;
  end process;

  
  be_output : process (read_be_latch)
  begin
    case (read_be_latch) is
      when "0001"   => props_out.raw_data <= x"000000"&spiDataOut(7 downto 0);
      when "0010"   => props_out.raw_data <= x"0000"&spiDataOut(7 downto 0)&x"00";
      when "0100"   => props_out.raw_data <= x"00"&spiDataOut(7 downto 0)&x"0000";
      when "1000"   => props_out.raw_data <= spiDataOut(7 downto 0)&x"000000";
      when others => props_out.raw_data <= (others => '0');
    end case;
  end process be_output;

  --Determine Read or Write Bit
  process (ctl_in.clk, ctl_in.reset)
  begin
    if (ctl_in.reset = '1') then
      spiDataIn <= (others => '0');
    elsif rising_edge(ctl_in.clk) then
      if(writeEdge = '1') then
        spiDataIn <= '1' & std_logic_vector(props_in.raw_address) & dataToWriteReg;
      elsif(readEdge = '1') then
        spiDataIn <= '0' & std_logic_vector(props_in.raw_address) & dataToWriteReg;
      end if;
    end if;
  end process;

  --Done
  spiNotBusy   <= not spiBusy;
  
  --Not busy edge detect
  process (ctl_in.clk, ctl_in.reset)
  begin
    if (ctl_in.reset = '1') then
      spiNotBusy_z1 <= '0';
    elsif rising_edge(ctl_in.clk) then
      spiNotBusy_z1 <= spiNotBusy;
    end if;
  end process;

  spiNotBusy_edge <= (spiNotBusy and not spiNotBusy_z1) when (ctl_in.reset='0') else '0';
  --spiNotBusy_edge <= (spiNotBusy and not spiNotBusy_z1) when (ctl_in.control_op=INITIALIZE_e) else '1';
  --spiNotBusy_edge <= (spiNotBusy and not spiNotBusy_z1) when (ctl_in.is_operating='0') else '1';

  ctl_out.done <= '1' when (ctl_in.control_op/=NO_OP_e) else spiNotBusy_edge;

  --Simulation SDO toggles every read
  process (ctl_in.clk, ctl_in.reset)
  begin
    if (ctl_in.reset = '1') then
      sdo_sim <= '0';
    elsif rising_edge(ctl_in.clk) then
      if(readEdge = '1') then 
       sdo_sim <= not sdo_sim;
      end if;
    end if;
  end process;

end rtl;
