-- Copyright 1986-2017 Xilinx, Inc. All Rights Reserved.
-- --------------------------------------------------------------------------------
-- Tool Version: Vivado v.2017.1 (lin64) Build 1846317 Fri Apr 14 18:54:47 MDT 2017
-- Date        : Thu Jun  1 10:51:06 2017
-- Host        : buildhost running 64-bit CentOS Linux release 7.2.1511 (Core)
-- Command     : write_vhdl -force -mode synth_stub
--               /data/Documents/workspace/vivado_ila/project_1/project_1.srcs/sources_1/ip/vivado_ila/vivado_ila_stub.vhdl
-- Design      : vivado_ila
-- Purpose     : Stub declaration of top-level module interface
-- Device      : xc7z020clg484-1
-- --------------------------------------------------------------------------------
library IEEE;
use IEEE.STD_LOGIC_1164.ALL;

entity vivado_ila is
  Port (
    clk : in STD_LOGIC;
    probe0 : in STD_LOGIC_VECTOR ( 111 downto 0 );
    probe1 : in STD_LOGIC_VECTOR ( 47 downto 0 )
  );

end vivado_ila;

architecture stub of vivado_ila is
attribute syn_black_box : boolean;
attribute black_box_pad_pin : string;
attribute syn_black_box of stub : architecture is true;
attribute black_box_pad_pin of stub : architecture is "clk,probe0[111:0],probe1[47:0]";
attribute X_CORE_INFO : string;
attribute X_CORE_INFO of stub : architecture is "ila,Vivado 2017.1";
begin
end;
