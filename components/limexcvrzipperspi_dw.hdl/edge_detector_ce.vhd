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
-- File        : $Id: edge_detector_ce.vhd,v 1.2 2013/03/01 20:25:49 julien.roy Exp $
--------------------------------------------------------------------------------
-- Description : Edge detector module                           
--                 - Rising => 1 : Rising edge detector 
--                 - Rising => 0 : Falling edge detector
--------------------------------------------------------------------------------
-- Copyright (c) 2012 Nutaq inc.
--------------------------------------------------------------------------------
-- $Log: edge_detector_ce.vhd,v $
-- Revision 1.2  2013/03/01 20:25:49  julien.roy
-- Add ADP 6 release vhd source
--
-- Revision 1.2  2013/01/18 19:03:45  julien.roy
-- Merge changes from ZedBoard reference design to Perseus
--
-- Revision 1.1  2012/09/28 19:31:26  khalid.bensadek
-- First commit of a stable AXI version. Xilinx 13.4
--
-- Revision 1.1  2011/06/15 21:12:51  jeffrey.johnson
-- Changed core name.
--
-- Revision 1.1  2011/05/27 13:37:57  patrick.gilbert
-- first commit: revA
--
--------------------------------------------------------------------------------

library IEEE;
use IEEE.STD_LOGIC_1164.ALL;

entity edge_detector_ce is

    Port (
      D         : in std_logic;
      Rising    : in std_logic;
      Clk       : in std_logic;
      Ce        : in std_logic;
      Q         : out std_logic
    );

end edge_detector_ce;



architecture Behavioral of edge_detector_ce is

    signal Q1   : std_logic;
    signal Q2   : std_logic;

begin

    process (Clk)

    begin

        if rising_edge(Clk) then
          if (Ce = '1') then
              Q1 <= D;
              Q2 <= Q1;
          end if;
        end if;

    end process;

    Q <= (Q1 and not(Q2)) when (Rising = '1') else (not(Q1) and Q2);


end Behavioral;
