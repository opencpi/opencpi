-------------------------------------------------------------------------------
-- Copyright (c) 2016 Xilinx, Inc.
-- All Rights Reserved
-------------------------------------------------------------------------------
--   ____  ____
--  /   /\/   /
-- /___/  \  /    Vendor     : Xilinx
-- \   \   \/     Version    : 14.5
--  \   \         Application: XILINX CORE Generator
--  /   /         Filename   : chipscope_ila_trig_n_data.vhd
-- /___/   /\     Timestamp  : Thu Jan 28 15:10:44 EST 2016
-- \   \  /  \
--  \___\/\___\
--
-- Design Name: VHDL Synthesis Wrapper
-------------------------------------------------------------------------------
-- This wrapper is used to integrate with Project Navigator and PlanAhead

LIBRARY ieee;
USE ieee.std_logic_1164.ALL;
ENTITY chipscope_ila_trig_n_data IS
  port (
    CONTROL: inout std_logic_vector(35 downto 0);
    CLK: in std_logic;
    DATA: in std_logic_vector(111 downto 0);
    TRIG0: in std_logic_vector(47 downto 0));
END chipscope_ila_trig_n_data;

ARCHITECTURE chipscope_ila_trig_n_data_a OF chipscope_ila_trig_n_data IS
BEGIN

END chipscope_ila_trig_n_data_a;
