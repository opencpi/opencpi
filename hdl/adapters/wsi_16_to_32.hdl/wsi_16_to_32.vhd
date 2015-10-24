-- THIS FILE WAS ORIGINALLY GENERATED ON Sun Jan 20 14:00:41 2013 EST
-- BASED ON THE FILE: wsi_16_to_32.xml
-- YOU *ARE* EXPECTED TO EDIT IT
-- This file initially contains the architecture skeleton for worker: wsi_32_to_16

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions

--entity wsi_16_to_32_worker is
--  port(
--    -- Clock(s) not associated with one specific port:
--    wci_Clk               : in std_logic;
--    -- Signals for WSI input port named "in".  See record types above.
--    in_in              : in  worker_in_in_t;
--    in_out             : out worker_in_out_t;
--    -- Signals for WSI output port named "out".  See record types above.
--    out_in             : in  worker_out_in_t;
--    out_out            : out worker_out_out_t);
--end entity wsi_16_to_32_worker;

--  type worker_in_in_t is record
--    reset            : Bool_t;           -- this port is being reset from the outside peer
--    ready            : Bool_t;           -- this port is ready for data to be taken
--                                         -- one or more of: som, eom, valid are true
--    data             : std_logic_vector(15 downto 0);
--    byte_enable      : std_logic_vector(1 downto 0);
--    som, eom, valid  : Bool_t;           -- valid means data and byte_enable are present
--  end record worker_in_in_t;
--  type worker_in_out_t is record
--    take             : Bool_t;           -- take data now from this port
--                                         -- can be asserted when ready is true
--  end record worker_in_out_t;
--
--  -- The following record(s) are for the inner/worker interfaces for port "out"
--  type worker_out_in_t is record
--    reset            : Bool_t;           -- this port is being reset from the outside peer
--    ready            : Bool_t;           -- this port is ready for data to be given
--  end record worker_out_in_t;
--  type worker_out_out_t is record
--    give             : Bool_t;           -- give data now to this port
--                                         -- can be asserted when ready is true
--    data             : std_logic_vector(31 downto 0);
--    byte_enable      : std_logic_vector(3 downto 0);
--    som, eom, valid  : Bool_t;            -- one or more must be true when 'give' is asserted
--  end record worker_out_out_t;


architecture rtl of wsi_16_to_32_worker is

  signal data_r        : std_logic_vector(15 downto 0):= (others => '0');
  signal byte_enable_r : std_logic_vector(1 downto 0):= "00";
  signal som_r, eom_r  : bool_t:= bfalse;
  signal valid_r       : bool_t:= bfalse;
  signal word_ready    : bool_t:= bfalse;
  signal ready_r       : bool_t:= bfalse;

begin
--  hi_absent           <= to_bool(in_in.byte_enable(3) = '0' and in_in.byte_enable(2) = '0');
  in_out.take         <= in_in.ready and out_in.ready; -- and
                         --(hi16_r or not in_in.valid or hi_absent);

  out_out.give        <= (in_in.ready or (ready_r and not valid_r)) and out_in.ready and word_ready;
  out_out.data        <= in_in.data & (data_r);
  out_out.byte_enable <= in_in.byte_enable & byte_enable_r;
  out_out.som         <= in_in.som or som_r;
  out_out.eom         <= in_in.eom or eom_r;
  out_out.valid       <= in_in.valid or valid_r;

  process (wci_clk) is
  begin
    if rising_edge(wci_clk) then

      ready_r <= in_in.ready;

      if in_in.reset or out_in.reset then

      elsif in_in.ready and out_in.ready then
        word_ready    <= not word_ready;
        data_r        <= in_in.data;
        byte_enable_r <= in_in.byte_enable;
        som_r         <= in_in.som;
        eom_r         <= in_in.eom;
        valid_r       <= in_in.valid;

      elsif word_ready and ready_r then
        if not valid_r then --zlm
          word_ready <= bfalse;
          som_r      <= bfalse;
          eom_r      <= bfalse;
        end if;

      end if;
    end if;
  end process;
end rtl;
