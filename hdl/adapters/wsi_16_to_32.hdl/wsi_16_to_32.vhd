library IEEE, ocpi;
use IEEE.std_logic_1164.all, ieee.numeric_std.all, ocpi.types.all;

architecture rtl of wsi_16_to_32_worker is
  signal data_r        : std_logic_vector(15 downto 0) := (others => '0');
  signal byte_enable_r : std_logic_vector(1 downto 0) := "00";
  signal som_r, eom_r  : bool_t := bfalse;
  signal valid_r       : bool_t := bfalse;
  signal word_ready_r  : bool_t := bfalse; 
begin
  in_out.take         <= in_in.ready and out_in.ready;
  out_out.give        <= in_in.ready and out_in.ready and word_ready_r;
  out_out.data        <= in_in.data & data_r;
  out_out.byte_enable <= in_in.byte_enable & byte_enable_r;
  out_out.som         <= in_in.som or som_r;
  out_out.eom         <= in_in.eom or eom_r;
  out_out.valid       <= in_in.valid or valid_r;

  process (wci_clk) is
  begin
    if rising_edge(wci_clk) then
      if wci_reset or in_in.reset or out_in.reset then
        word_ready_r  <= bfalse;
      elsif in_in.ready and out_in.ready then
        -- toggle which word we are on.
        word_ready_r  <= not word_ready;
        -- capture what happened in the previous cycle
        ready_r       <= in_in.ready;
        data_r        <= in_in.data;
        byte_enable_r <= in_in.byte_enable;
        som_r         <= in_in.som;
        eom_r         <= in_in.eom;
        valid_r       <= in_in.valid;
      end if;
    end if;
  end process;
end rtl;
