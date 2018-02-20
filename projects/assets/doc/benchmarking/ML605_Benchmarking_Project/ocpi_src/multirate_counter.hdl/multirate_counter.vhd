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

-- THIS FILE WAS ORIGINALLY GENERATED ON Wed Mar  5 15:20:04 2014 EST
-- BASED ON THE FILE: multirate_counter.xml
-- YOU *ARE* EXPECTED TO EDIT IT
-- This file initially contains the architecture skeleton for worker: multirate_counter

library IEEE; 
  use IEEE.std_logic_1164.all; 
  use ieee.numeric_std.all;
library ocpi; 
  use ocpi.types.all;   
  use ocpi.wci.all;-- remove this to avoid all ocpi name collisions
library bsv;
library prims; 

architecture rtl of multirate_counter_worker is

--  constant OUT_MSG_SIZE         : integer := 2048;
  signal msg_cnt                : unsigned(15 downto 0);
  signal fifo_data_in           : std_logic_vector(31 downto 0);
  
  signal clk_div_ratio          : integer :=1;
  signal clk_div_out            : std_logic;
  signal slow_clk               : std_logic;
  signal multirate_clk          : std_logic;
  signal count                  : unsigned(7 downto 0);    
  signal enable_din_mrclk       : std_logic;
  signal enable_dout_cclk       : std_logic; 
  signal enable_dout_mrclk      : std_logic; 
  signal en_dout_cclk_delayed   : std_logic;
  signal is_operating_delayed   : std_logic;
  signal cnt_mrclk_raw          : unsigned(31 downto 0);
  signal cnt_cclk_raw           : unsigned(31 downto 0);
  signal cnt_mrclk_dat, cnt_mrclk_dat_q : unsigned(31 downto 0);
  signal cnt_cclk_dat           : unsigned(31 downto 0);
  signal cnt_fifo_full          : unsigned(31 downto 0);
  signal cnt_fifo_empty         : unsigned(31 downto 0);
  signal out_out_give_false     : unsigned(31 downto 0);
  signal out_in_ready_false     : unsigned(31 downto 0);
  signal messageSize            : unsigned(31 downto 0);  
  signal fifo_full_n            : std_logic;
  signal fifo_empty_n, fifo_empty_n_q  : std_logic;
  signal fifo_overflow          : std_logic;
  signal fifo_underflow         : std_logic;
  signal fifo_data_out          : std_logic_vector(31 downto 0);
  signal data_q                 : unsigned(31 downto 0) := x"00000000";
  signal wordsWritten           : unsigned(31 downto 0) := x"00000000";
  signal cnt_fault              : unsigned(63 downto 0) := x"0000000000000000";
  signal out_out_give           : std_logic;
  signal out_out_valid          : std_logic;
  signal out_out_data           : std_logic_vector(31 downto 0);
  signal out_out_som            : std_logic;
  signal out_out_eom            : std_logic;
begin
  -----------------------------------------------------------------------------
  -- Reset props before is_operating
  process(ctl_in.clk)
  begin
    if rising_edge(ctl_in.clk) then
      if (ctl_in.reset = '1') then
        props_out.cnt_mrclk_raw <= (others => '0');
        props_out.cnt_cclk_raw <= (others => '0');
        props_out.cnt_mrclk_dat <= (others => '0');
        props_out.cnt_cclk_dat <= (others => '0');
        props_out.cnt_fifo_full <= (others => '0');
        props_out.cnt_fifo_empty <= (others => '0');
        props_out.cnt_fault <= (others => '0');
        props_out.wordsWritten <= (others => '0');
        props_out.messageSize <= (others => '0');
        props_out.out_out_give_false <= (others => '0');
        props_out.out_in_ready_false <= (others => '0');
      elsif (ctl_in.is_operating='1') then
        props_out.cnt_mrclk_raw <= cnt_mrclk_raw;
        props_out.cnt_cclk_raw <= cnt_cclk_raw;    
        props_out.cnt_mrclk_dat <= cnt_mrclk_dat;
        props_out.cnt_cclk_dat <= cnt_cclk_dat;  
        props_out.cnt_fifo_full <= cnt_fifo_full;
        props_out.cnt_fifo_empty <= cnt_fifo_empty;
        props_out.out_out_give_false <= out_out_give_false;
        props_out.out_in_ready_false <= out_in_ready_false;
        props_out.messageSize <= props_in.messageSize;
        props_out.cnt_fault <= to_ulonglong(std_logic_vector(cnt_fault));
        props_out.wordsWritten <= to_ulong(std_logic_vector(wordsWritten));
      end if;
    end if;
end process;

metrics : process(ctl_in.clk)
begin
    if rising_edge(ctl_in.clk) then
      if (ctl_in.reset = '1') then
    	out_out_give_false <= (others => '0');
    	out_in_ready_false <= (others => '0');
    else
	if ctl_in.is_operating and out_out_give = '0' then  
    	  out_out_give_false <= out_out_give_false + 1;
    	end if;    		
	if ctl_in.is_operating and out_in.ready = bfalse then 
    	  out_in_ready_false <= out_in_ready_false + 1;
    	end if;
    	end if;
    end if;	
	
end process;

process(ctl_in.clk)
  begin
    if rising_edge(ctl_in.clk) then
    	if (ctl_in.reset = '1') then
    		cnt_mrclk_dat_q <= cnt_mrclk_dat;
    	else
    		cnt_mrclk_dat_q <= (others => '0');
    	end if;
    end if;
end process;

  -----------------------------------------------------------------------------
  clk_div_ratio <= 1 when to_integer(props_in.clk_div_ratio) = 0 else to_integer(props_in.clk_div_ratio);
  
  clk_gen_din : prims.benchmark.clk_div 
  port map(
    N           => clk_div_ratio,
    RST         => ctl_in.reset,
    CLK_IN      => ctl_in.clk,
    CLK_OUT     => clk_div_out);
  
  multirate_clk <=  ctl_in.clk when (props_in.bypass_clk_div=btrue) else clk_div_out;
  -----------------------------------------------------------------------------
--  reg_en_in_mrclk_domain : process(multirate_clk)
--  begin
--    if rising_edge(multirate_clk) then
--      if (ctl_in.reset = '1') then
--        enable_din_mrclk <= '0';
--      else          
--        enable_din_mrclk <= ctl_in.is_operating and out_in.ready;
--      end if;
--    end if;
--  end process;
  enable_din_mrclk <= ctl_in.is_operating and out_in.ready; 
  enable_dout_cclk <= ctl_in.is_operating and out_in.ready; 
  -----------------------------------------------------------------------------  
  counter_mrclk_raw : prims.benchmark.counter
  generic map(
    bit_length => 32)
  port map(
    RST_IN    => ctl_in.reset,
    CLK_IN    => multirate_clk,
    EN_IN     => ctl_in.is_operating,
    EN_OUT    => is_operating_delayed,
    COUNT_OUT => cnt_mrclk_raw);

  counter_cclk_raw : prims.benchmark.counter
  generic map(
    bit_length => 32)
  port map(
    RST_IN    => ctl_in.reset,
    CLK_IN    => ctl_in.clk,
    EN_IN     => ctl_in.is_operating,
    EN_OUT    => open,
    COUNT_OUT => cnt_cclk_raw);

  counter_din_mrclk : prims.benchmark.counter
  generic map(
    bit_length => 32)
  port map(
    RST_IN    => ctl_in.reset,
    CLK_IN    => multirate_clk,
    EN_IN     => enable_din_mrclk and fifo_full_n,
    EN_OUT    => enable_dout_mrclk,
    COUNT_OUT => cnt_mrclk_dat);

  counter_dout_cclk : prims.benchmark.counter
  generic map(
    bit_length => 32)
  port map(
    RST_IN    => ctl_in.reset,
    CLK_IN    => ctl_in.clk,
    EN_IN     => enable_dout_cclk,
    EN_OUT    => en_dout_cclk_delayed,
    COUNT_OUT => cnt_cclk_dat);
  -----------------------------------------------------------------------------
  fifo_1 : bsv.bsv.SyncFIFO
  generic map (
    dataWidth => 32,
    depth     => 16,
    indxWidth => 4)
  port map (
    sCLK     => multirate_clk,
    sRST     => not ctl_in.reset,
    sENQ     => fifo_full_n and is_operating_delayed, -- out_in_ready and enable_dout_mrclk,
    sD_IN    => fifo_data_in, --std_logic_vector(cnt_mrclk_dat),
    sFULL_N  => fifo_full_n,
    dCLK     => ctl_in.clk,
    dDEQ     => enable_dout_cclk and fifo_empty_n,
    dEMPTY_N => fifo_empty_n,
    dD_OUT   => fifo_data_out
    );
   
  fifo_data_in <= std_logic_vector(cnt_mrclk_dat); 
    
  delay_fifo_empty : process(multirate_clk) 
  begin
  if ctl_in.reset = '1' then
  	fifo_empty_n_q <= '0';
  elsif rising_edge(multirate_clk) then
    fifo_empty_n_q <= fifo_empty_n;
  end if;
  end process; 
  -----------------------------------------------------------------------------
  counter_fifo_full : prims.benchmark.counter
  generic map(
    bit_length => 32)
  port map(
    RST_IN    => ctl_in.reset,
    CLK_IN    => multirate_clk,
    EN_IN     => enable_din_mrclk and not fifo_full_n,
    COUNT_OUT => cnt_fifo_full);

  counter_fifo_empty : prims.benchmark.counter
  generic map(
    bit_length => 32)
  port map(
    RST_IN    => ctl_in.reset,
    CLK_IN    => ctl_in.clk,
    EN_IN     => enable_dout_cclk and not fifo_empty_n,
    COUNT_OUT => cnt_fifo_empty);
  -----------------------------------------------------------------------------
  -- Checks FIFO output data for skips in the counter,
  -- prior to sending to the data plane.
  count_faults : process(ctl_in.clk)
  begin
    if rising_edge(ctl_in.clk) then
      if (ctl_in.reset = '1') then
        data_q <= (others => '0');
        cnt_fault <= (others => '0');
        wordsWritten <= (others => '0');
      elsif (enable_dout_cclk='1' and fifo_empty_n) then 
--      if (fifo_empty_n = '1') then -- if word is read from fifo same clock as it's written, this fault count would be inaccurate
        data_q <= unsigned(fifo_data_out);
        wordsWritten <= wordsWritten + 1;
        if (unsigned(fifo_data_out) /= (data_q+1)) then
          cnt_fault <= cnt_fault + 1;
        end if;          
      end if;
--      if (enable_dout_cclk='1' and fifo_empty_n_q) then 
--          if (unsigned(fifo_data_out) /= (data_q+1)) then
--            cnt_fault <= cnt_fault + 1;
--          end if;
--      end if;
    end if;
  end process;
  -------------------------------------------------------------------------------
  output_message_counter: process (ctl_in.clk)
  begin  
    if rising_edge(ctl_in.clk) then
      if (ctl_in.reset = '1') then
        msg_cnt <= (others => '0');
      elsif (enable_dout_cclk='1') then
        if (props_in.bypass_data_sel /= x"00") then
          -- Bypass mode - controlled by Output Ready...ignores FIFO status
          if (msg_cnt = props_in.messageSize-1) then        
            msg_cnt <= (others => '0');
          else
            msg_cnt <= msg_cnt + 1;
          end if;          
        elsif (fifo_empty_n='1') then
          -- Normal mode - controlled by Output Ready & FIFO status
          if (msg_cnt = props_in.messageSize-1) then        
            msg_cnt <= (others => '0');
          else
            msg_cnt <= msg_cnt + 1;
          end if;
        end if;
      end if;
    end if;
  end process;

output_message_signals : process (enable_dout_cclk, props_in.bypass_data_sel,fifo_empty_n,
                                  fifo_data_out, cnt_cclk_raw, cnt_cclk_dat,msg_cnt)
  begin  
    if (enable_dout_cclk='1') then -- enable_dout_clk  <= ctl_in.is_operating and out_in.ready; 
      -- Give & Valid active when Output Ready
      if (props_in.bypass_data_sel = x"00") then
        -- Normal mode - Data from FIFO @ Clk Divider Rate
        if (fifo_empty_n = '1') then
          out_out_give <= '1';
          out_out_valid <= '1';
          out_out_data <= fifo_data_out;            
        else
          out_out_give <= '0';
          out_out_valid <= '0';
        end if;
      elsif (props_in.bypass_data_sel = x"01") then
        -- Bypass Mode - Data @ Raw Control Clk Rate (Will result in cntr discontinuity)
        out_out_give <= '1';
        out_out_valid <= '1';
        out_out_data <= std_logic_vector(cnt_cclk_raw);
      elsif (props_in.bypass_data_sel = x"02") then
        -- Bypass Mode - Data @ Enabled Control Clk Rate
        out_out_give <= '1';
        out_out_valid <= '1';
        out_out_data <= std_logic_vector(cnt_cclk_dat);          
      else
        out_out_give <= '0';
        out_out_valid <= '0';
        out_out_data <= x"55DE_AD55";
      end if;
      
      --SOM & EOM at beginning and end of msg_cnt & ...
      if (props_in.bypass_data_sel = x"00") then
        -- Normal Mode - controlled by Output Ready & FIFO status
        if (fifo_empty_n = '1') then
          if msg_cnt=0 then
            out_out_som <= '1';
          else
            out_out_som <= '0';
          end if;
          if msg_cnt=(props_in.messageSize-1) then
            out_out_eom <= '1';
          else
            out_out_eom <= '0';          
          end if;
        else
          out_out_som <= '0';
          out_out_eom <= '0';
        end if;
      else
        -- Bypass Mode - controlled by Output Ready...ignores FIFO status
        if msg_cnt=0 then
          out_out_som <= '1';
        else
          out_out_som <= '0';
        end if;
        if msg_cnt=(props_in.messageSize-1) then
          out_out_eom <= '1';
        else
          out_out_eom <= '0';          
        end if;
      end if;         
    else
      out_out_data <= x"DEAD_BEEF";
      out_out_give <= '0';
      out_out_valid <= '0';
      out_out_som <= '0';
      out_out_eom <= '0';
    end if;
  end process;
      out_out.data <= out_out_data;
      out_out.give <= out_out_give;
      out_out.valid <= out_out_valid;
      out_out.som <= out_out_som;
      out_out.eom <= out_out_eom;
-------------------------------------------------------------------------------

end rtl;
