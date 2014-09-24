--------------------------------------------------------------------------------
--
--    ****                              *
--   ******                            ***
--   *******                           ****
--   ********    ****  ****     **** *********    ******* ****    ***********
--   *********   ****  ****     **** *********  **************  *************
--   **** *****  ****  ****     ****   ****    *****    ****** *****     ****
--   ****  ***** ****  ****     ****   ****   *****      ****  ****      ****
--  ****    *********  ****     ****   ****   ****       ****  ****      ****
--  ****     ********  ****    *****  ****    *****     *****  ****      ****
--  ****      ******   ***** ******   *****    ****** *******  ****** *******
--  ****        ****   ************    ******   *************   *************
--  ****         ***     ****  ****     ****      *****  ****     *****  ****
--                                                                       ****
--          I N N O V A T I O N  T O D A Y  F O R  T O M M O R O W       ****
--                                                                        ***
--
--------------------------------------------------------------------------------
-- File        : $Id: spi_register.vhd,v 1.2 2013/03/01 20:25:49 julien.roy Exp $
--------------------------------------------------------------------------------
-- Description : Generic SPI core
--------------------------------------------------------------------------------
-- Copyright (c) 2012 Nutaq inc.
--------------------------------------------------------------------------------
-- $Log: spi_register.vhd,v $
-- Revision 1.2  2013/03/01 20:25:49  julien.roy
-- Add ADP 6 release vhd source
--
-- Revision 1.2  2013/01/18 19:03:46  julien.roy
-- Merge changes from ZedBoard reference design to Perseus
--
-- Revision 1.1  2012/09/28 19:31:26  khalid.bensadek
-- First commit of a stable AXI version. Xilinx 13.4
--
-- Revision 1.2  2011/06/16 18:54:55  jeffrey.johnson
-- Added CVS tags.
--
--
--------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_arith.all;
use ieee.std_logic_unsigned.all;

entity spi_register is
  generic(
    SPI_DATA_WIDTH  : integer := 16;
    SPI_NUMBER_CS : integer := 1;
    SPI_CLK_DOWNSAMPLING : integer := 4   --max 254
  );
  port  (
    -- SPI Interface
    i_clk_p            : in     std_logic;
    i_rst_p            : in     std_logic;
    iv_spiStart_p      : in     std_logic_vector(SPI_NUMBER_CS-1 downto 0);
    i_spiClkPol_p      : in     std_logic;
    i_spiMsbf_p        : in     std_logic;
    iv_spiDataIn_p     : in     std_logic_vector((SPI_DATA_WIDTH - 1) downto 0);
    ov_spiDataOut_p    : out    std_logic_vector((SPI_DATA_WIDTH - 1) downto 0);
    o_spiBusy_p        : out    std_logic;

    o_spiClkOutPin_p   : out    std_logic;
    ov_spiCsPin_p      : out    std_logic_vector(SPI_NUMBER_CS-1 downto 0);
    o_spiMosiPin_p     : out    std_logic;
    i_spiMisoPin_p     : in    std_logic
  );
end entity spi_register;


architecture rtl of spi_register is

  ----------------------------------------
  -- Function declaration
  ----------------------------------------

  function OR_REDUCE(ARG: std_logic_vector) return std_logic is
    variable result: std_logic;
    begin
      result := '0';
      for i in ARG'range loop
        result := result or ARG(i);
      end loop;
      return result;
    end;

  ----------------------------------------
  -- Component declaration
  ----------------------------------------
  
  component Edge_detector_ce is
    Port (
      D       : in std_logic;
      Rising  : in std_logic;
      Clk     : in std_logic;
      Ce      : in std_logic;
      Q       : out std_logic
  );
  end component;

  ----------------------------------------
  -- Type declaration
  ---------------------------------------- 

  --Declaration for control state machine
  type state_type is (st1_idle, st1a_startClockDelay, st2_load_tx, st3_transmit, st4_error, st5_delay1);

  ----------------------------------------
  -- Signal declaration
  ---------------------------------------- 
  
  signal state_sig, next_state_sig : state_type;

  signal spi_tx_enable_s    :std_logic;
  signal spiEnable_s        :std_logic;

  signal spiClkPin_s        : std_logic;
  signal csLatch_s          : std_logic_vector(SPI_NUMBER_CS-1 downto 0);
  signal countTxBit_s       : integer range 0 to (SPI_DATA_WIDTH-1);
  signal orStart_s          : std_logic;
  signal startEdge_s        : std_logic;
  signal startEdgeDetect_s  : std_logic;
  signal v_spi_lfsr_s       : std_logic_vector(SPI_DATA_WIDTH-1  downto 0);

  signal spiCe_s            : std_logic;
  signal spiClkCount_s      : unsigned(7 downto 0);

  signal countClock_s       : std_logic_vector(2 downto 0);
  signal countEn_s          : std_logic;

begin

  ----------------------------------------
  -- Clock decimatation process
  ----------------------------------------
  p_ClkDecim_l: process(i_clk_p)
  begin
    if (rising_edge(i_clk_p)) then
        if (i_rst_p = '1') then
          spiClkCount_s <= (others => '0');
          spiClkPin_s <= '0';
          spiCe_s <= '0';
        else
          spiCe_s <= '0';
          spiClkCount_s <= spiClkCount_s + 1;
          if (spiClkCount_s = SPI_CLK_DOWNSAMPLING/2-1) then
            spiClkPin_s <= '0';
          end if;
          if (spiClkCount_s = SPI_CLK_DOWNSAMPLING-1) then
            spiClkCount_s <= (others => '0');
            spiClkPin_s <= '1';
            spiCe_s <= '1';
          end if;
        end if;
    end if;
  end process;

  ----------------------------------------
  -- SPI clock assignment and disable on non activity
  ----------------------------------------
  regSpiClk_l: process(i_clk_p)
  begin
    if rising_edge(i_clk_p) then
      if (i_spiClkPol_p = '0') then
        if (state_sig = st1_idle) then
          o_spiClkOutPin_p <= '0';
        else
          o_spiClkOutPin_p <= spiClkPin_s ;
        end if;
      else
        if (state_sig = st1_idle) then
          o_spiClkOutPin_p <= '0';
        else
          o_spiClkOutPin_p <= not(spiClkPin_s);
        end if;
      end if;
   end if;
 end process;

  ----------------------------------------
  --Detect register rising edge
  ----------------------------------------
  orStart_s <= OR_REDUCE(iv_spiStart_p);
  
  U1_ed1 : Edge_detector_ce
    port map (
      D         => orStart_s,
      Ce        => '1',
      Rising    => '1',
      Clk       => i_clk_p,
      Q         => startEdgeDetect_s
    );
    
  ----------------------------------------
  -- Wait detection due to clock decimation
  ----------------------------------------
  Latch_process_l : process (i_clk_p)
  begin
    if rising_edge(i_clk_p) then
      if (i_rst_p = '1' or spiEnable_s = '1' ) then
        startEdge_s   <= '0';
      -- Synchronous latching of status
      elsif (startEdgeDetect_s = '1') then
        csLatch_s <= iv_spiStart_p;
        startEdge_s   <= '1';
      end if;
    end if;
  end process;

  ----------------------------------------
  -- Serialize/Deserialize SPI data
  ----------------------------------------
  ps_UserSPIO: process(i_clk_p)
   begin
      if rising_edge(i_clk_p) then
        if (i_rst_p = '1') then
          v_spi_lfsr_s <= (others=>'0');
        elsif (spiCe_s ='1' and spiEnable_s ='1') then
          if ( spi_tx_enable_s = '1' ) then
            if (i_spiMsbf_p = '1') then
              for i in (SPI_DATA_WIDTH - 1) downto 1 loop
                v_spi_lfsr_s(i) <= v_spi_lfsr_s(i-1);
              end loop;
              v_spi_lfsr_s(0) <= i_spiMisoPin_p;
              
            else
              for i in 0 to (SPI_DATA_WIDTH - 2) loop
                v_spi_lfsr_s(i) <= v_spi_lfsr_s(i+1);
              end loop;
              v_spi_lfsr_s(SPI_DATA_WIDTH - 1) <= i_spiMisoPin_p;
              
            end if;
            
          else
          
            v_spi_lfsr_s <= iv_spiDataIn_p;
            
          end if;
        end if;
     end if;
  end process ;

  ----------------------------------------
  -- Create a clock counter
  ----------------------------------------
  ps_countclock: process (i_clk_p)
    begin
      if rising_edge(i_clk_p) then
        if (i_rst_p = '1') then
          countClock_s <= (others=>'0');
        elsif (countEn_s = '1') then
          if (spiCe_s ='1') then
            countClock_s <= countClock_s + 1 ;
          end if;
        else
          countClock_s <= (others=>'0');
        end if;
     end if;
  end process;

  ----------------------------------------
  -- Create a TX bit counter
  ----------------------------------------
  ps_countbit: process (i_clk_p)
    begin
      if rising_edge(i_clk_p) then
        if (i_rst_p = '1') then
          countTxBit_s <= 0;
        elsif (spiCe_s ='1' and spi_tx_enable_s = '1') then
          if (countTxBit_s = SPI_DATA_WIDTH -1) then
            countTxBit_s <= 0;
          else
            countTxBit_s <= countTxBit_s + 1 ;
          end if;
        end if;
     end if;
  end process;


  ----------------------------------------
  -- State machine
  ----------------------------------------
  SYNC_PROC: process (i_clk_p, i_rst_p)
  begin
    if rising_edge(i_clk_p) then
      if (i_rst_p='1') then
        state_sig <= st1_idle;
      elsif (spiCe_s ='1') then
        state_sig <= next_state_sig;
      end if;
    end if;
  end process;

  -- Moore State Machine - Outputs based on state only
  OUTPUT_DECODE: process (state_sig,spiEnable_s,spi_tx_enable_s,countEn_s)
  begin
    case (state_sig) is
        when st1_idle =>
            spi_tx_enable_s <= '0';
            spiEnable_s <= '0';
            countEn_s <= '0';
        when st1a_startClockDelay=>
            spi_tx_enable_s <= '0';
            spiEnable_s <= '0';
            countEn_s <= '1';
        when st2_load_tx =>
            spi_tx_enable_s <= '0';
            spiEnable_s <= '1';
            countEn_s <= '0';
        when st3_transmit =>
            spi_tx_enable_s <= '1';
            spiEnable_s <= '1';
            countEn_s <= '0';
        when st5_delay1 =>
            spi_tx_enable_s <= '0';
            spiEnable_s <= '0';
            countEn_s <= '1';
        when others =>
            spi_tx_enable_s <= '0';
            spiEnable_s <= '0';
            countEn_s <= '0';
    end case;
  end process;

  -- Next state decoder
  NEXT_STATE_DECODE: process (state_sig, startEdge_s,countTxBit_s,countClock_s)
  begin
  
    -- Declare default state for next_state to avoid latches
    next_state_sig <= state_sig;  -- Default is to stay in current state

    case (state_sig) is
      when st1_idle =>
        if startEdge_s = '1' then
          next_state_sig <= st1a_startClockDelay;
        end if;
     when st1a_startClockDelay =>
        if countClock_s = "111" then
          next_state_sig <= st2_load_tx;
        end if;
     when st2_load_tx =>
        next_state_sig <= st3_transmit;
     when st3_transmit =>
        if (countTxBit_s = SPI_DATA_WIDTH -1) then
            next_state_sig <= st5_delay1;
        end if;
     when st5_delay1 =>
        if countClock_s = "111" then
           next_state_sig <= st1_idle;
        end if;
     when others =>
        next_state_sig <= st4_error;
    end case;
  end process;
         
  ----------------------------------------
  -- Output assignment
  ----------------------------------------
  o_spiBusy_p  <= spiEnable_s or countEn_s;
  ov_spiCsPin_p <= not csLatch_s when (spi_tx_enable_s = '1') else (others=>'1');
  o_spiMosiPin_p <= v_spi_lfsr_s(SPI_DATA_WIDTH-1) when (i_spiMsbf_p = '1') else v_spi_lfsr_s(0);
  ov_spiDataOut_p <= v_spi_lfsr_s;

end architecture rtl;
