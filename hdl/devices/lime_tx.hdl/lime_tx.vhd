-- Lime transmit worker
-- Options based on parameters are:
--   UseTxClkInput: if true, use the transmit clock at the input pin
--   UseRxClkInput: if true, use the receive clock input pin for the transmit clock
--   Future: use some system clock with some divisor

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.types.all, ocpi.util.all;
library bsv;
architecture rtl of lime_tx_worker is
  --Sync FIFO constants
  constant tx_dataWidth : natural := 32;
  constant tx_depth     : natural := 64;  --2^indxWidth
  constant tx_indxWidth : natural := 6;
  constant tx_init      : natural := 0;

  signal my_tx_clk : std_logic;

  --Sync FIFO signals
  signal tx_dEMPTY_N_sig : std_logic;
  signal tx_sFULL_N_sig : std_logic;
  signal tx_sENQ_sig : std_logic;
  signal tx_dDEQ_sig : std_logic;
  signal tx_sD_IN_sig: std_logic_vector(tx_dataWidth - 1 downto 0);
  signal tx_dD_OUT_sig: std_logic_vector(tx_dataWidth - 1 downto 0);
  signal sRst : std_logic;

  --TX signals for output port
  signal tx_iq_sel_sig  : std_logic;
  signal tx_iq_sel_sync : std_logic;
  signal iq_sel_z1      : std_logic;
--  signal iq_sel_z2      : std_logic;
  signal iq_sel_edge    : std_logic;
  signal i_sig          : std_logic_vector(11 downto 0);
  signal q_sig          : std_logic_vector(11 downto 0);
  signal txOut_sig      : std_logic_vector(11 downto 0);

  --Input OCPI interface signals
  signal in_take : std_logic;
  
  signal underrun_r     : bool_t;
begin
  -- Route the raw property signals to the lime_spi
  rawprops_out.present <= '1';
  rawprops_out.reset   <= ctl_in.reset;
  rawprops_out.renable <= props_in.raw_is_read;
  rawprops_out.wenable <= props_in.raw_is_write;
  rawprops_out.addr(props_in.raw_address'left downto 0) <= props_in.raw_address;
  rawprops_out.addr(rawprops_out.addr'left downto props_in.raw_address'left+1)
  <= (others => '0');
  rawprops_out.benable <= props_in.raw_byte_enable;
  rawprops_out.data    <= props_in.raw_data;
  props_out.raw_data    <= rawprops_in.data;
  props_out.underrun    <= underrun_r;
  props_out.present     <= (0 => dev_in.rx_present, 1 => dev_in.tx_present,
                            others => '0');
  ctl_out.done <= rawprops_in.done when
                  props_in.raw_is_read or props_in.raw_is_write
                  else '1';
                            
  -- FIXME: address issue of the quality of this clock
  my_tx_clk <= dev_in.rx_clk_in when clkmode = to_uchar(0) else dev_in.tx_clk_in;
  ----------------------------------------------------------------------------
  -- TX IQ Select
  ----------------------------------------------------------------------------
  tx_iq_select : process(my_tx_clk)
  begin  -- process
    if rising_edge(my_tx_clk) then
      if (ctl_in.reset = '1') then
        underrun_r    <= bfalse;
        tx_iq_sel_sig <= '0';
      else
        if its(props_in.underrun_written) then
          underrun_r <= props_in.underrun;
        end if;
        tx_iq_sel_sig <= not tx_iq_sel_sig;
      end if;
    end if;
  end process;

  tx_iq_sel <= tx_iq_sel_sig;

  ----------------------------------------------------------------------------
  -- Synchronizing Bit for IQ sel between CTL clock and Lime clock
  ----------------------------------------------------------------------------
  tx_syncBit1 : bsv.bsv.SyncBit
    generic map (
      init => tx_init)
    port map (
      --sCLK   => lime_clk,
      sCLK   => my_tx_clk,
      sRST   => sRst,
      sEN    => sRst,
      sD_IN  => tx_iq_sel_sig,
      dCLK   => ctl_in.clk,
      dD_OUT => tx_iq_sel_sync
      );

  ----------------------------------------------------------------------------
  -- IQ Select Edge Detect (in Ctl clock domain)
  ----------------------------------------------------------------------------
  process (ctl_in.clk)
  begin
    if rising_edge(ctl_in.clk) then
      if ctl_in.reset = '1' then
        iq_sel_z1 <= '0';
      else
        iq_sel_z1 <= tx_iq_sel_sync;
      end if;
    end if;
  end process;

  iq_sel_edge <= tx_iq_sel_sync and not iq_sel_z1;

  ----------------------------------------------------------------------------
  -- TX Data
  ----------------------------------------------------------------------------
  i_sig <= std_logic_vector(unsigned(tx_dD_OUT_sig(31 downto 20))+('0' & tx_dD_OUT_sig(19)));
  q_sig <= std_logic_vector(unsigned(tx_dD_OUT_sig(15 downto  4))+('0' & tx_dD_OUT_sig(3)));
  txOut_sig <= i_sig when (tx_iq_sel_sig = '1') else q_sig;
  txd <= txOut_sig when (ctl_in.is_operating='1') else x"000";

  ----------------------------------------------------------------------------
  -- Synchronizing FIFO
  ----------------------------------------------------------------------------
  sRst <= not ctl_in.reset;

  tx_syncFIFO1 : bsv.bsv.SyncFIFO
    generic map (
      dataWidth => tx_dataWidth,
      depth     => tx_depth,
      indxWidth => tx_indxWidth)
    port map (
      sCLK     => ctl_in.clk,
      sRST     => sRst,
      sENQ     => tx_sENQ_sig,
      sD_IN    => tx_sD_IN_sig,
      sFULL_N  => tx_sFULL_N_sig,
      --dCLK     => lime_clk,
      dCLK     => my_tx_clk,
      dDEQ     => tx_dDEQ_sig,
      dEMPTY_N => tx_dEMPTY_N_sig,
      dD_OUT   => tx_dD_OUT_sig
      );

  tx_sD_IN_sig <= in_in.data;

  ----------------------------------------------------------------------------
  -- Enqueue (enqueue when FIFO is not FULL)
  ----------------------------------------------------------------------------
  enqueue : process (ctl_in.clk)
  begin
    if rising_edge(ctl_in.clk) then
      if ctl_in.reset = '1' then
        tx_sENQ_sig <= '0';
      elsif in_take = '1' then
        tx_sENQ_sig <= '1';
      else
        tx_sENQ_sig <= '0';
      end if;
    end if;
  end process enqueue;

  ----------------------------------------------------------------------------
  -- Dequeue (when FIFO is not EMPTY)
  ----------------------------------------------------------------------------
  tx_dDEQ_sig <= '1' when (tx_dEMPTY_N_sig = '1') else '0';

  ----------------------------------------------------------------------------
  -- TX Enable
  ----------------------------------------------------------------------------
  txen <= '1';

  ----------------------------------------------------------------------------
  -- Take (when FIFO is not full and ready)
  ----------------------------------------------------------------------------
  in_out.take <= in_take;

  take : process (ctl_in.clk)
  begin
    if rising_edge(ctl_in.clk) then
      if tx_sFULL_N_sig = '1' and in_in.ready = '1' and iq_sel_edge = '1' then
        in_take <= '1';
      else
        in_take <= '0';
      end if;
    end if;
  end process;

end rtl;
