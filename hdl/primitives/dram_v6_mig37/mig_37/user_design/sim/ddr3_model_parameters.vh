/****************************************************************************************
*
*   Disclaimer   This software code and all associated documentation, comments or other
*  of Warranty:  information (collectively "Software") is provided "AS IS" without
*                warranty of any kind. MICRON TECHNOLOGY, INC. ("MTI") EXPRESSLY
*                DISCLAIMS ALL WARRANTIES EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
*                TO, NONINFRINGEMENT OF THIRD PARTY RIGHTS, AND ANY IMPLIED WARRANTIES
*                OF MERCHANTABILITY OR FITNESS FOR ANY PARTICULAR PURPOSE. MTI DOES NOT
*                WARRANT THAT THE SOFTWARE WILL MEET YOUR REQUIREMENTS, OR THAT THE
*                OPERATION OF THE SOFTWARE WILL BE UNINTERRUPTED OR ERROR-FREE.
*                FURTHERMORE, MTI DOES NOT MAKE ANY REPRESENTATIONS REGARDING THE USE OR
*                THE RESULTS OF THE USE OF THE SOFTWARE IN TERMS OF ITS CORRECTNESS,
*                ACCURACY, RELIABILITY, OR OTHERWISE. THE ENTIRE RISK ARISING OUT OF USE
*                OR PERFORMANCE OF THE SOFTWARE REMAINS WITH YOU. IN NO EVENT SHALL MTI,
*                ITS AFFILIATED COMPANIES OR THEIR SUPPLIERS BE LIABLE FOR ANY DIRECT,
*                INDIRECT, CONSEQUENTIAL, INCIDENTAL, OR SPECIAL DAMAGES (INCLUDING,
*                WITHOUT LIMITATION, DAMAGES FOR LOSS OF PROFITS, BUSINESS INTERRUPTION,
*                OR LOSS OF INFORMATION) ARISING OUT OF YOUR USE OF OR INABILITY TO USE
*                THE SOFTWARE, EVEN IF MTI HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH
*                DAMAGES. Because some jurisdictions prohibit the exclusion or
*                limitation of liability for consequential or incidental damages, the
*                above limitation may not apply to you.
*
*                Copyright 2003 Micron Technology, Inc. All rights reserved.
*
****************************************************************************************/

    // Parameters current with 1Gb and 2Gb datasheet rev D

    // Timing parameters based on Speed Grade

                                            // SYMBOL     UNITS DESCRIPTION
                                            // ------     ----- -----------
`ifdef x1Gb  // 1Gb parameters

  `ifdef sg094E                             // sg094E is equivalent to the JEDEC DDR3-2133 (13-13-13) speed bin
      parameter TCK_MIN          =   937.5; // tCK        ps    Minimum Clock Cycle Time
      parameter TJIT_PER         =      50; // tJIT(per)  ps    Period JItter
      parameter TJIT_CC          =     100; // tJIT(cc)   ps    Cycle to Cycle jitter
      parameter TERR_2PER        =      73; // tERR(2per) ps    Accumulated Error (2-cycle)
      parameter TERR_3PER        =      85; // tERR(3per) ps    Accumulated Error (3-cycle)
      parameter TERR_4PER        =      98; // tERR(4per) ps    Accumulated Error (4-cycle)
      parameter TERR_5PER        =     105; // tERR(5per) ps    Accumulated Error (5-cycle)
      parameter TERR_6PER        =     111; // tERR(6per) ps    Accumulated Error (6-cycle)
      parameter TERR_7PER        =     117; // tERR(7per) ps    Accumulated Error (7-cycle)
      parameter TERR_8PER        =     121; // tERR(8per) ps    Accumulated Error (8-cycle)
      parameter TERR_9PER        =     125; // tERR(9per) ps    Accumulated Error (9-cycle)
      parameter TERR_10PER       =     128; // tERR(10per)ps    Accumulated Error (10-cycle)
      parameter TERR_11PER       =     132; // tERR(11per)ps    Accumulated Error (11-cycle)
      parameter TERR_12PER       =     134; // tERR(12per)ps    Accumulated Error (12-cycle)
      parameter TDS              =       5; // tDS        ps    DQ and DM input setup time relative to DQS
      parameter TDH              =      20; // tDH        ps    DQ and DM input hold time relative to DQS
      parameter TDQSQ            =      70; // tDQSQ      ps    DQS-DQ skew, DQS to last DQ valid, per group, per access
      parameter TDQSS            =    0.27; // tDQSS      tCK   Rising clock edge to DQS/DQS# latching transition
      parameter TDSS             =    0.18; // tDSS       tCK   DQS falling edge to CLK rising (setup time)
      parameter TDSH             =    0.18; // tDSH       tCK   DQS falling edge from CLK rising (hold time)
      parameter TDQSCK           =     175; // tDQSCK     ps    DQS output access time from CK/CK#
      parameter TQSH             =    0.40; // tQSH       tCK   DQS Output High Pulse Width
      parameter TQSL             =    0.40; // tQSL       tCK   DQS Output Low Pulse Width
      parameter TDIPW            =     275; // tDIPW      ps    DQ and DM input Pulse Width
      parameter TIPW             =     455; // tIPW       ps    Control and Address input Pulse Width
      parameter TIS              =      35; // tIS        ps    Input Setup Time
      parameter TIH              =      75; // tIH        ps    Input Hold Time
      parameter TRAS_MIN         =   35000; // tRAS       ps    Minimum Active to Precharge command time
      parameter TRC              =   46250; // tRC        ps    Active to Active/Auto Refresh command time
      parameter TRCD             =   12187; // tRCD       ps    Active to Read/Write command time
      parameter TRP              =   12187; // tRP        ps    Precharge command period
      parameter TXP              =    6000; // tXP        ps    Exit power down to a valid command
      parameter TCKE             =    5000; // tCKE       ps    CKE minimum high or low pulse width
      parameter TAON             =     180; // tAON       ps    RTT turn-on from ODTLon reference
      parameter TWLS             =     122; // tWLS       ps    Setup time for tDQS flop
      parameter TWLH             =     122; // tWLH       ps    Hold time of tDQS flop
      parameter TWLO             =    7500; // tWLO       ps    Write levelization output delay
      parameter TAA_MIN          =   12187; // TAA        ps    Internal READ command to first data
      parameter CL_TIME          =   12187; // CL         ps    Minimum CAS Latency
  `else `ifdef sg094                        // sg094  is equivalent to the JEDEC DDR3-2133 (14-14-14) speed bin
      parameter TCK_MIN          =   937.5; // tCK        ps    Minimum Clock Cycle Time
      parameter TJIT_PER         =      50; // tJIT(per)  ps    Period JItter
      parameter TJIT_CC          =     100; // tJIT(cc)   ps    Cycle to Cycle jitter
      parameter TERR_2PER        =      73; // tERR(2per) ps    Accumulated Error (2-cycle)
      parameter TERR_3PER        =      85; // tERR(3per) ps    Accumulated Error (3-cycle)
      parameter TERR_4PER        =      98; // tERR(4per) ps    Accumulated Error (4-cycle)
      parameter TERR_5PER        =     105; // tERR(5per) ps    Accumulated Error (5-cycle)
      parameter TERR_6PER        =     111; // tERR(6per) ps    Accumulated Error (6-cycle)
      parameter TERR_7PER        =     117; // tERR(7per) ps    Accumulated Error (7-cycle)
      parameter TERR_8PER        =     121; // tERR(8per) ps    Accumulated Error (8-cycle)
      parameter TERR_9PER        =     125; // tERR(9per) ps    Accumulated Error (9-cycle)
      parameter TERR_10PER       =     128; // tERR(10per)ps    Accumulated Error (10-cycle)
      parameter TERR_11PER       =     132; // tERR(11per)ps    Accumulated Error (11-cycle)
      parameter TERR_12PER       =     134; // tERR(12per)ps    Accumulated Error (12-cycle)
      parameter TDS              =       5; // tDS        ps    DQ and DM input setup time relative to DQS
      parameter TDH              =      20; // tDH        ps    DQ and DM input hold time relative to DQS
      parameter TDQSQ            =      70; // tDQSQ      ps    DQS-DQ skew, DQS to last DQ valid, per group, per access
      parameter TDQSS            =    0.27; // tDQSS      tCK   Rising clock edge to DQS/DQS# latching transition
      parameter TDSS             =    0.18; // tDSS       tCK   DQS falling edge to CLK rising (setup time)
      parameter TDSH             =    0.18; // tDSH       tCK   DQS falling edge from CLK rising (hold time)
      parameter TDQSCK           =     175; // tDQSCK     ps    DQS output access time from CK/CK#
      parameter TQSH             =    0.40; // tQSH       tCK   DQS Output High Pulse Width
      parameter TQSL             =    0.40; // tQSL       tCK   DQS Output Low Pulse Width
      parameter TDIPW            =     275; // tDIPW      ps    DQ and DM input Pulse Width
      parameter TIPW             =     455; // tIPW       ps    Control and Address input Pulse Width
      parameter TIS              =      35; // tIS        ps    Input Setup Time
      parameter TIH              =      75; // tIH        ps    Input Hold Time
      parameter TRAS_MIN         =   35000; // tRAS       ps    Minimum Active to Precharge command time
      parameter TRC              =   46250; // tRC        ps    Active to Active/Auto Refresh command time
      parameter TRCD             =   13125; // tRCD       ps    Active to Read/Write command time
      parameter TRP              =   13125; // tRP        ps    Precharge command period
      parameter TXP              =    6000; // tXP        ps    Exit power down to a valid command
      parameter TCKE             =    5000; // tCKE       ps    CKE minimum high or low pulse width
      parameter TAON             =     180; // tAON       ps    RTT turn-on from ODTLon reference
      parameter TWLS             =     122; // tWLS       ps    Setup time for tDQS flop
      parameter TWLH             =     122; // tWLH       ps    Hold time of tDQS flop
      parameter TWLO             =    7500; // tWLO       ps    Write levelization output delay
      parameter TAA_MIN          =   13125; // TAA        ps    Internal READ command to first data
      parameter CL_TIME          =   13125; // CL         ps    Minimum CAS Latency
  `else `ifdef sg107F                       // sg107F is equivalent to the JEDEC DDR3-1866 (12-12-12) speed bin
      parameter TCK_MIN          = 15e3/14; // tCK        ps    Minimum Clock Cycle Time
      parameter TJIT_PER         =      60; // tJIT(per)  ps    Period JItter
      parameter TJIT_CC          =     120; // tJIT(cc)   ps    Cycle to Cycle jitter
      parameter TERR_2PER        =      88; // tERR(2per) ps    Accumulated Error (2-cycle)
      parameter TERR_3PER        =     103; // tERR(3per) ps    Accumulated Error (3-cycle)
      parameter TERR_4PER        =     117; // tERR(4per) ps    Accumulated Error (4-cycle)
      parameter TERR_5PER        =     126; // tERR(5per) ps    Accumulated Error (5-cycle)
      parameter TERR_6PER        =     133; // tERR(6per) ps    Accumulated Error (6-cycle)
      parameter TERR_7PER        =     140; // tERR(7per) ps    Accumulated Error (7-cycle)
      parameter TERR_8PER        =     145; // tERR(8per) ps    Accumulated Error (8-cycle)
      parameter TERR_9PER        =     150; // tERR(9per) ps    Accumulated Error (9-cycle)
      parameter TERR_10PER       =     154; // tERR(10per)ps    Accumulated Error (10-cycle)
      parameter TERR_11PER       =     158; // tERR(11per)ps    Accumulated Error (11-cycle)
      parameter TERR_12PER       =     161; // tERR(12per)ps    Accumulated Error (12-cycle)
      parameter TDS              =      10; // tDS        ps    DQ and DM input setup time relative to DQS
      parameter TDH              =      20; // tDH        ps    DQ and DM input hold time relative to DQS
      parameter TDQSQ            =      80; // tDQSQ      ps    DQS-DQ skew, DQS to last DQ valid, per group, per access
      parameter TDQSS            =    0.27; // tDQSS      tCK   Rising clock edge to DQS/DQS# latching transition
      parameter TDSS             =    0.18; // tDSS       tCK   DQS falling edge to CLK rising (setup time)
      parameter TDSH             =    0.18; // tDSH       tCK   DQS falling edge from CLK rising (hold time)
      parameter TDQSCK           =     200; // tDQSCK     ps    DQS output access time from CK/CK#
      parameter TQSH             =    0.40; // tQSH       tCK   DQS Output High Pulse Width
      parameter TQSL             =    0.40; // tQSL       tCK   DQS Output Low Pulse Width
      parameter TDIPW            =     300; // tDIPW      ps    DQ and DM input Pulse Width
      parameter TIPW             =     505; // tIPW       ps    Control and Address input Pulse Width
      parameter TIS              =      50; // tIS        ps    Input Setup Time
      parameter TIH              =     100; // tIH        ps    Input Hold Time
      parameter TRAS_MIN         =   35000; // tRAS       ps    Minimum Active to Precharge command time
      parameter TRC              =   46250; // tRC        ps    Active to Active/Auto Refresh command time
      parameter TRCD             =   12857; // tRCD       ps    Active to Read/Write command time
      parameter TRP              =   12857; // tRP        ps    Precharge command period
      parameter TXP              =    6000; // tXP        ps    Exit power down to a valid command
      parameter TCKE             =    5000; // tCKE       ps    CKE minimum high or low pulse width
      parameter TAON             =     200; // tAON       ps    RTT turn-on from ODTLon reference
      parameter TWLS             =     140; // tWLS       ps    Setup time for tDQS flop
      parameter TWLH             =     140; // tWLH       ps    Hold time of tDQS flop
      parameter TWLO             =    7500; // tWLO       ps    Write levelization output delay
      parameter TAA_MIN          =   12857; // TAA        ps    Internal READ command to first data
      parameter CL_TIME          =   12857; // CL         ps    Minimum CAS Latency
  `else `ifdef sg107E                       // sg107E is equivalent to the JEDEC DDR3-1866 (13-13-13) speed bin
      parameter TCK_MIN          = 15e3/14; // tCK        ps    Minimum Clock Cycle Time
      parameter TJIT_PER         =      60; // tJIT(per)  ps    Period JItter
      parameter TJIT_CC          =     120; // tJIT(cc)   ps    Cycle to Cycle jitter
      parameter TERR_2PER        =      88; // tERR(2per) ps    Accumulated Error (2-cycle)
      parameter TERR_3PER        =     103; // tERR(3per) ps    Accumulated Error (3-cycle)
      parameter TERR_4PER        =     117; // tERR(4per) ps    Accumulated Error (4-cycle)
      parameter TERR_5PER        =     126; // tERR(5per) ps    Accumulated Error (5-cycle)
      parameter TERR_6PER        =     133; // tERR(6per) ps    Accumulated Error (6-cycle)
      parameter TERR_7PER        =     140; // tERR(7per) ps    Accumulated Error (7-cycle)
      parameter TERR_8PER        =     145; // tERR(8per) ps    Accumulated Error (8-cycle)
      parameter TERR_9PER        =     150; // tERR(9per) ps    Accumulated Error (9-cycle)
      parameter TERR_10PER       =     154; // tERR(10per)ps    Accumulated Error (10-cycle)
      parameter TERR_11PER       =     158; // tERR(11per)ps    Accumulated Error (11-cycle)
      parameter TERR_12PER       =     161; // tERR(12per)ps    Accumulated Error (12-cycle)
      parameter TDS              =      10; // tDS        ps    DQ and DM input setup time relative to DQS
      parameter TDH              =      20; // tDH        ps    DQ and DM input hold time relative to DQS
      parameter TDQSQ            =      80; // tDQSQ      ps    DQS-DQ skew, DQS to last DQ valid, per group, per access
      parameter TDQSS            =    0.27; // tDQSS      tCK   Rising clock edge to DQS/DQS# latching transition
      parameter TDSS             =    0.18; // tDSS       tCK   DQS falling edge to CLK rising (setup time)
      parameter TDSH             =    0.18; // tDSH       tCK   DQS falling edge from CLK rising (hold time)
      parameter TDQSCK           =     200; // tDQSCK     ps    DQS output access time from CK/CK#
      parameter TQSH             =    0.40; // tQSH       tCK   DQS Output High Pulse Width
      parameter TQSL             =    0.40; // tQSL       tCK   DQS Output Low Pulse Width
      parameter TDIPW            =     300; // tDIPW      ps    DQ and DM input Pulse Width
      parameter TIPW             =     505; // tIPW       ps    Control and Address input Pulse Width
      parameter TIS              =      50; // tIS        ps    Input Setup Time
      parameter TIH              =     100; // tIH        ps    Input Hold Time
      parameter TRAS_MIN         =   35000; // tRAS       ps    Minimum Active to Precharge command time
      parameter TRC              =   46250; // tRC        ps    Active to Active/Auto Refresh command time
      parameter TRCD             =   13928; // tRCD       ps    Active to Read/Write command time
      parameter TRP              =   13928; // tRP        ps    Precharge command period
      parameter TXP              =    6000; // tXP        ps    Exit power down to a valid command
      parameter TCKE             =    5000; // tCKE       ps    CKE minimum high or low pulse width
      parameter TAON             =     200; // tAON       ps    RTT turn-on from ODTLon reference
      parameter TWLS             =     140; // tWLS       ps    Setup time for tDQS flop
      parameter TWLH             =     140; // tWLH       ps    Hold time of tDQS flop
      parameter TWLO             =    7500; // tWLO       ps    Write levelization output delay
      parameter TAA_MIN          =   13928; // TAA        ps    Internal READ command to first data
      parameter CL_TIME          =   13928; // CL         ps    Minimum CAS Latency
  `else `ifdef sg107                        // sg107  is equivalent to the JEDEC DDR3-1866 (14-14-14) speed bin
      parameter TCK_MIN          = 15e3/14; // tCK        ps    Minimum Clock Cycle Time
      parameter TJIT_PER         =      60; // tJIT(per)  ps    Period JItter
      parameter TJIT_CC          =     120; // tJIT(cc)   ps    Cycle to Cycle jitter
      parameter TERR_2PER        =      88; // tERR(2per) ps    Accumulated Error (2-cycle)
      parameter TERR_3PER        =     103; // tERR(3per) ps    Accumulated Error (3-cycle)
      parameter TERR_4PER        =     117; // tERR(4per) ps    Accumulated Error (4-cycle)
      parameter TERR_5PER        =     126; // tERR(5per) ps    Accumulated Error (5-cycle)
      parameter TERR_6PER        =     133; // tERR(6per) ps    Accumulated Error (6-cycle)
      parameter TERR_7PER        =     140; // tERR(7per) ps    Accumulated Error (7-cycle)
      parameter TERR_8PER        =     145; // tERR(8per) ps    Accumulated Error (8-cycle)
      parameter TERR_9PER        =     150; // tERR(9per) ps    Accumulated Error (9-cycle)
      parameter TERR_10PER       =     154; // tERR(10per)ps    Accumulated Error (10-cycle)
      parameter TERR_11PER       =     158; // tERR(11per)ps    Accumulated Error (11-cycle)
      parameter TERR_12PER       =     161; // tERR(12per)ps    Accumulated Error (12-cycle)
      parameter TDS              =      10; // tDS        ps    DQ and DM input setup time relative to DQS
      parameter TDH              =      20; // tDH        ps    DQ and DM input hold time relative to DQS
      parameter TDQSQ            =      80; // tDQSQ      ps    DQS-DQ skew, DQS to last DQ valid, per group, per access
      parameter TDQSS            =    0.27; // tDQSS      tCK   Rising clock edge to DQS/DQS# latching transition
      parameter TDSS             =    0.18; // tDSS       tCK   DQS falling edge to CLK rising (setup time)
      parameter TDSH             =    0.18; // tDSH       tCK   DQS falling edge from CLK rising (hold time)
      parameter TDQSCK           =     200; // tDQSCK     ps    DQS output access time from CK/CK#
      parameter TQSH             =    0.40; // tQSH       tCK   DQS Output High Pulse Width
      parameter TQSL             =    0.40; // tQSL       tCK   DQS Output Low Pulse Width
      parameter TDIPW            =     300; // tDIPW      ps    DQ and DM input Pulse Width
      parameter TIPW             =     505; // tIPW       ps    Control and Address input Pulse Width
      parameter TIS              =      50; // tIS        ps    Input Setup Time
      parameter TIH              =     100; // tIH        ps    Input Hold Time
      parameter TRAS_MIN         =   35000; // tRAS       ps    Minimum Active to Precharge command time
      parameter TRC              =   46250; // tRC        ps    Active to Active/Auto Refresh command time
      parameter TRCD             =   15000; // tRCD       ps    Active to Read/Write command time
      parameter TRP              =   15000; // tRP        ps    Precharge command period
      parameter TXP              =    6000; // tXP        ps    Exit power down to a valid command
      parameter TCKE             =    5000; // tCKE       ps    CKE minimum high or low pulse width
      parameter TAON             =     200; // tAON       ps    RTT turn-on from ODTLon reference
      parameter TWLS             =     140; // tWLS       ps    Setup time for tDQS flop
      parameter TWLH             =     140; // tWLH       ps    Hold time of tDQS flop
      parameter TWLO             =    7500; // tWLO       ps    Write levelization output delay
      parameter TAA_MIN          =   15000; // TAA        ps    Internal READ command to first data
      parameter CL_TIME          =   15000; // CL         ps    Minimum CAS Latency
  `else `ifdef sg125F                       // sg125F is equivalent to the JEDEC DDR3-1600 (9-9-9) speed bin
      parameter TCK_MIN          =    1250; // tCK        ps    Minimum Clock Cycle Time
      parameter TJIT_PER         =      70; // tJIT(per)  ps    Period JItter
      parameter TJIT_CC          =     140; // tJIT(cc)   ps    Cycle to Cycle jitter
      parameter TERR_2PER        =     103; // tERR(2per) ps    Accumulated Error (2-cycle)
      parameter TERR_3PER        =     122; // tERR(3per) ps    Accumulated Error (3-cycle)
      parameter TERR_4PER        =     136; // tERR(4per) ps    Accumulated Error (4-cycle)
      parameter TERR_5PER        =     147; // tERR(5per) ps    Accumulated Error (5-cycle)
      parameter TERR_6PER        =     155; // tERR(6per) ps    Accumulated Error (6-cycle)
      parameter TERR_7PER        =     163; // tERR(7per) ps    Accumulated Error (7-cycle)
      parameter TERR_8PER        =     169; // tERR(8per) ps    Accumulated Error (8-cycle)
      parameter TERR_9PER        =     175; // tERR(9per) ps    Accumulated Error (9-cycle)
      parameter TERR_10PER       =     180; // tERR(10per)ps    Accumulated Error (10-cycle)
      parameter TERR_11PER       =     184; // tERR(11per)ps    Accumulated Error (11-cycle)
      parameter TERR_12PER       =     188; // tERR(12per)ps    Accumulated Error (12-cycle)
      parameter TDS              =      10; // tDS        ps    DQ and DM input setup time relative to DQS
      parameter TDH              =      45; // tDH        ps    DQ and DM input hold time relative to DQS
      parameter TDQSQ            =     100; // tDQSQ      ps    DQS-DQ skew, DQS to last DQ valid, per group, per access
      parameter TDQSS            =    0.27; // tDQSS      tCK   Rising clock edge to DQS/DQS# latching transition
      parameter TDSS             =    0.18; // tDSS       tCK   DQS falling edge to CLK rising (setup time)
      parameter TDSH             =    0.18; // tDSH       tCK   DQS falling edge from CLK rising (hold time)
      parameter TDQSCK           =     225; // tDQSCK     ps    DQS output access time from CK/CK#
      parameter TQSH             =    0.40; // tQSH       tCK   DQS Output High Pulse Width
      parameter TQSL             =    0.40; // tQSL       tCK   DQS Output Low Pulse Width
      parameter TDIPW            =     360; // tDIPW      ps    DQ and DM input Pulse Width
      parameter TIPW             =     560; // tIPW       ps    Control and Address input Pulse Width
      parameter TIS              =     170; // tIS        ps    Input Setup Time
      parameter TIH              =     120; // tIH        ps    Input Hold Time
      parameter TRAS_MIN         =   35000; // tRAS       ps    Minimum Active to Precharge command time
      parameter TRC              =   46250; // tRC        ps    Active to Active/Auto Refresh command time
      parameter TRCD             =   11250; // tRCD       ps    Active to Read/Write command time
      parameter TRP              =   11250; // tRP        ps    Precharge command period
      parameter TXP              =    6000; // tXP        ps    Exit power down to a valid command
      parameter TCKE             =    5000; // tCKE       ps    CKE minimum high or low pulse width
      parameter TAON             =     250; // tAON       ps    RTT turn-on from ODTLon reference
      parameter TWLS             =     165; // tWLS       ps    Setup time for tDQS flop
      parameter TWLH             =     165; // tWLH       ps    Hold time of tDQS flop
      parameter TWLO             =    7500; // tWLO       ps    Write levelization output delay
      parameter TAA_MIN          =   11250; // TAA        ps    Internal READ command to first data
      parameter CL_TIME          =   11250; // CL         ps    Minimum CAS Latency
  `else `ifdef sg125E                       // sg125E is equivalent to the JEDEC DDR3-1600 (10-10-10) speed bin
      parameter TCK_MIN          =    1250; // tCK        ps    Minimum Clock Cycle Time
      parameter TJIT_PER         =      70; // tJIT(per)  ps    Period JItter
      parameter TJIT_CC          =     140; // tJIT(cc)   ps    Cycle to Cycle jitter
      parameter TERR_2PER        =     103; // tERR(2per) ps    Accumulated Error (2-cycle)
      parameter TERR_3PER        =     122; // tERR(3per) ps    Accumulated Error (3-cycle)
      parameter TERR_4PER        =     136; // tERR(4per) ps    Accumulated Error (4-cycle)
      parameter TERR_5PER        =     147; // tERR(5per) ps    Accumulated Error (5-cycle)
      parameter TERR_6PER        =     155; // tERR(6per) ps    Accumulated Error (6-cycle)
      parameter TERR_7PER        =     163; // tERR(7per) ps    Accumulated Error (7-cycle)
      parameter TERR_8PER        =     169; // tERR(8per) ps    Accumulated Error (8-cycle)
      parameter TERR_9PER        =     175; // tERR(9per) ps    Accumulated Error (9-cycle)
      parameter TERR_10PER       =     180; // tERR(10per)ps    Accumulated Error (10-cycle)
      parameter TERR_11PER       =     184; // tERR(11per)ps    Accumulated Error (11-cycle)
      parameter TERR_12PER       =     188; // tERR(12per)ps    Accumulated Error (12-cycle)
      parameter TDS              =      10; // tDS        ps    DQ and DM input setup time relative to DQS
      parameter TDH              =      45; // tDH        ps    DQ and DM input hold time relative to DQS
      parameter TDQSQ            =     100; // tDQSQ      ps    DQS-DQ skew, DQS to last DQ valid, per group, per access
      parameter TDQSS            =    0.27; // tDQSS      tCK   Rising clock edge to DQS/DQS# latching transition
      parameter TDSS             =    0.18; // tDSS       tCK   DQS falling edge to CLK rising (setup time)
      parameter TDSH             =    0.18; // tDSH       tCK   DQS falling edge from CLK rising (hold time)
      parameter TDQSCK           =     225; // tDQSCK     ps    DQS output access time from CK/CK#
      parameter TQSH             =    0.40; // tQSH       tCK   DQS Output High Pulse Width
      parameter TQSL             =    0.40; // tQSL       tCK   DQS Output Low Pulse Width
      parameter TDIPW            =     360; // tDIPW      ps    DQ and DM input Pulse Width
      parameter TIPW             =     560; // tIPW       ps    Control and Address input Pulse Width
      parameter TIS              =     170; // tIS        ps    Input Setup Time
      parameter TIH              =     120; // tIH        ps    Input Hold Time
      parameter TRAS_MIN         =   35000; // tRAS       ps    Minimum Active to Precharge command time
      parameter TRC              =   47500; // tRC        ps    Active to Active/Auto Refresh command time
      parameter TRCD             =   12500; // tRCD       ps    Active to Read/Write command time
      parameter TRP              =   12500; // tRP        ps    Precharge command period
      parameter TXP              =    6000; // tXP        ps    Exit power down to a valid command
      parameter TCKE             =    5000; // tCKE       ps    CKE minimum high or low pulse width
      parameter TAON             =     250; // tAON       ps    RTT turn-on from ODTLon reference
      parameter TWLS             =     165; // tWLS       ps    Setup time for tDQS flop
      parameter TWLH             =     165; // tWLH       ps    Hold time of tDQS flop
      parameter TWLO             =    7500; // tWLO       ps    Write levelization output delay
      parameter TAA_MIN          =   12500; // TAA        ps    Internal READ command to first data
      parameter CL_TIME          =   12500; // CL         ps    Minimum CAS Latency
  `else `ifdef sg125                        // sg125 is equivalent to the JEDEC DDR3-1600 (11-11-11) speed bin
      parameter TCK_MIN          =    1250; // tCK        ps    Minimum Clock Cycle Time
      parameter TJIT_PER         =      70; // tJIT(per)  ps    Period JItter
      parameter TJIT_CC          =     140; // tJIT(cc)   ps    Cycle to Cycle jitter
      parameter TERR_2PER        =     103; // tERR(2per) ps    Accumulated Error (2-cycle)
      parameter TERR_3PER        =     122; // tERR(3per) ps    Accumulated Error (3-cycle)
      parameter TERR_4PER        =     136; // tERR(4per) ps    Accumulated Error (4-cycle)
      parameter TERR_5PER        =     147; // tERR(5per) ps    Accumulated Error (5-cycle)
      parameter TERR_6PER        =     155; // tERR(6per) ps    Accumulated Error (6-cycle)
      parameter TERR_7PER        =     163; // tERR(7per) ps    Accumulated Error (7-cycle)
      parameter TERR_8PER        =     169; // tERR(8per) ps    Accumulated Error (8-cycle)
      parameter TERR_9PER        =     175; // tERR(9per) ps    Accumulated Error (9-cycle)
      parameter TERR_10PER       =     180; // tERR(10per)ps    Accumulated Error (10-cycle)
      parameter TERR_11PER       =     184; // tERR(11per)ps    Accumulated Error (11-cycle)
      parameter TERR_12PER       =     188; // tERR(12per)ps    Accumulated Error (12-cycle)
      parameter TDS              =      10; // tDS        ps    DQ and DM input setup time relative to DQS
      parameter TDH              =      45; // tDH        ps    DQ and DM input hold time relative to DQS
      parameter TDQSQ            =     100; // tDQSQ      ps    DQS-DQ skew, DQS to last DQ valid, per group, per access
      parameter TDQSS            =    0.27; // tDQSS      tCK   Rising clock edge to DQS/DQS# latching transition
      parameter TDSS             =    0.18; // tDSS       tCK   DQS falling edge to CLK rising (setup time)
      parameter TDSH             =    0.18; // tDSH       tCK   DQS falling edge from CLK rising (hold time)
      parameter TDQSCK           =     225; // tDQSCK     ps    DQS output access time from CK/CK#
      parameter TQSH             =    0.40; // tQSH       tCK   DQS Output High Pulse Width
      parameter TQSL             =    0.40; // tQSL       tCK   DQS Output Low Pulse Width
      parameter TDIPW            =     360; // tDIPW      ps    DQ and DM input Pulse Width
      parameter TIPW             =     560; // tIPW       ps    Control and Address input Pulse Width
      parameter TIS              =     170; // tIS        ps    Input Setup Time
      parameter TIH              =     120; // tIH        ps    Input Hold Time
      parameter TRAS_MIN         =   35000; // tRAS       ps    Minimum Active to Precharge command time
      parameter TRC              =   48750; // tRC        ps    Active to Active/Auto Refresh command time
      parameter TRCD             =   13125; // tRCD       ps    Active to Read/Write command time
      parameter TRP              =   13125; // tRP        ps    Precharge command period
      parameter TXP              =    6000; // tXP        ps    Exit power down to a valid command
      parameter TCKE             =    5000; // tCKE       ps    CKE minimum high or low pulse width
      parameter TAON             =     250; // tAON       ps    RTT turn-on from ODTLon reference
      parameter TWLS             =     165; // tWLS       ps    Setup time for tDQS flop
      parameter TWLH             =     165; // tWLH       ps    Hold time of tDQS flop
      parameter TWLO             =    7500; // tWLO       ps    Write levelization output delay
      parameter TAA_MIN          =   13125; // TAA        ps    Internal READ command to first data
      parameter CL_TIME          =   13125; // CL         ps    Minimum CAS Latency
  `else `ifdef sg15E                        // sg15E is equivalent to the JEDEC DDR3-1333H (9-9-9) speed bin
      parameter TCK_MIN          =    1500; // tCK        ps    Minimum Clock Cycle Time
      parameter TJIT_PER         =      80; // tJIT(per)  ps    Period JItter
      parameter TJIT_CC          =     160; // tJIT(cc)   ps    Cycle to Cycle jitter
      parameter TERR_2PER        =     118; // tERR(2per) ps    Accumulated Error (2-cycle)
      parameter TERR_3PER        =     140; // tERR(3per) ps    Accumulated Error (3-cycle)
      parameter TERR_4PER        =     155; // tERR(4per) ps    Accumulated Error (4-cycle)
      parameter TERR_5PER        =     168; // tERR(5per) ps    Accumulated Error (5-cycle)
      parameter TERR_6PER        =     177; // tERR(6per) ps    Accumulated Error (6-cycle)
      parameter TERR_7PER        =     186; // tERR(7per) ps    Accumulated Error (7-cycle)
      parameter TERR_8PER        =     193; // tERR(8per) ps    Accumulated Error (8-cycle)
      parameter TERR_9PER        =     200; // tERR(9per) ps    Accumulated Error (9-cycle)
      parameter TERR_10PER       =     205; // tERR(10per)ps    Accumulated Error (10-cycle)
      parameter TERR_11PER       =     210; // tERR(11per)ps    Accumulated Error (11-cycle)
      parameter TERR_12PER       =     215; // tERR(12per)ps    Accumulated Error (12-cycle)
      parameter TDS              =      30; // tDS        ps    DQ and DM input setup time relative to DQS
      parameter TDH              =      65; // tDH        ps    DQ and DM input hold time relative to DQS
      parameter TDQSQ            =     125; // tDQSQ      ps    DQS-DQ skew, DQS to last DQ valid, per group, per access
      parameter TDQSS            =    0.25; // tDQSS      tCK   Rising clock edge to DQS/DQS# latching transition
      parameter TDSS             =    0.20; // tDSS       tCK   DQS falling edge to CLK rising (setup time)
      parameter TDSH             =    0.20; // tDSH       tCK   DQS falling edge from CLK rising (hold time)
      parameter TDQSCK           =     255; // tDQSCK     ps    DQS output access time from CK/CK#
      parameter TQSH             =    0.40; // tQSH       tCK   DQS Output High Pulse Width
      parameter TQSL             =    0.40; // tQSL       tCK   DQS Output Low Pulse Width
      parameter TDIPW            =     400; // tDIPW      ps    DQ and DM input Pulse Width
      parameter TIPW             =     620; // tIPW       ps    Control and Address input Pulse Width
      parameter TIS              =     190; // tIS        ps    Input Setup Time
      parameter TIH              =     140; // tIH        ps    Input Hold Time
      parameter TRAS_MIN         =   36000; // tRAS       ps    Minimum Active to Precharge command time
      parameter TRC              =   49500; // tRC        ps    Active to Active/Auto Refresh command time
      parameter TRCD             =   13125; // tRCD       ps    Active to Read/Write command time
      parameter TRP              =   13125; // tRP        ps    Precharge command period
      parameter TXP              =    6000; // tXP        ps    Exit power down to a valid command
      parameter TCKE             =    5625; // tCKE       ps    CKE minimum high or low pulse width
      parameter TAON             =     250; // tAON       ps    RTT turn-on from ODTLon reference
      parameter TWLS             =     195; // tWLS       ps    Setup time for tDQS flop
      parameter TWLH             =     195; // tWLH       ps    Hold time of tDQS flop
      parameter TWLO             =    9000; // tWLO       ps    Write levelization output delay
      parameter TAA_MIN          =   13125; // TAA        ps    Internal READ command to first data
      parameter CL_TIME          =   13125; // CL         ps    Minimum CAS Latency
  `else `ifdef sg15                         // sg15 is equivalent to the JEDEC DDR3-1333J (10-10-10) speed bin
      parameter TCK_MIN          =    1500; // tCK        ps    Minimum Clock Cycle Time
      parameter TJIT_PER         =      80; // tJIT(per)  ps    Period JItter
      parameter TJIT_CC          =     160; // tJIT(cc)   ps    Cycle to Cycle jitter
      parameter TERR_2PER        =     118; // tERR(2per) ps    Accumulated Error (2-cycle)
      parameter TERR_3PER        =     140; // tERR(3per) ps    Accumulated Error (3-cycle)
      parameter TERR_4PER        =     155; // tERR(4per) ps    Accumulated Error (4-cycle)
      parameter TERR_5PER        =     168; // tERR(5per) ps    Accumulated Error (5-cycle)
      parameter TERR_6PER        =     177; // tERR(6per) ps    Accumulated Error (6-cycle)
      parameter TERR_7PER        =     186; // tERR(7per) ps    Accumulated Error (7-cycle)
      parameter TERR_8PER        =     193; // tERR(8per) ps    Accumulated Error (8-cycle)
      parameter TERR_9PER        =     200; // tERR(9per) ps    Accumulated Error (9-cycle)
      parameter TERR_10PER       =     205; // tERR(10per)ps    Accumulated Error (10-cycle)
      parameter TERR_11PER       =     210; // tERR(11per)ps    Accumulated Error (11-cycle)
      parameter TERR_12PER       =     215; // tERR(12per)ps    Accumulated Error (12-cycle)
      parameter TDS              =      30; // tDS        ps    DQ and DM input setup time relative to DQS
      parameter TDH              =      65; // tDH        ps    DQ and DM input hold time relative to DQS
      parameter TDQSQ            =     125; // tDQSQ      ps    DQS-DQ skew, DQS to last DQ valid, per group, per access
      parameter TDQSS            =    0.25; // tDQSS      tCK   Rising clock edge to DQS/DQS# latching transition
      parameter TDSS             =    0.20; // tDSS       tCK   DQS falling edge to CLK rising (setup time)
      parameter TDSH             =    0.20; // tDSH       tCK   DQS falling edge from CLK rising (hold time)
      parameter TDQSCK           =     255; // tDQSCK     ps    DQS output access time from CK/CK#
      parameter TQSH             =    0.40; // tQSH       tCK   DQS Output High Pulse Width
      parameter TQSL             =    0.40; // tQSL       tCK   DQS Output Low Pulse Width
      parameter TDIPW            =     400; // tDIPW      ps    DQ and DM input Pulse Width
      parameter TIPW             =     620; // tIPW       ps    Control and Address input Pulse Width
      parameter TIS              =     190; // tIS        ps    Input Setup Time
      parameter TIH              =     140; // tIH        ps    Input Hold Time
      parameter TRAS_MIN         =   36000; // tRAS       ps    Minimum Active to Precharge command time
      parameter TRC              =   51000; // tRC        ps    Active to Active/Auto Refresh command time
      parameter TRCD             =   15000; // tRCD       ps    Active to Read/Write command time
      parameter TRP              =   15000; // tRP        ps    Precharge command period
      parameter TXP              =    6000; // tXP        ps    Exit power down to a valid command
      parameter TCKE             =    5625; // tCKE       ps    CKE minimum high or low pulse width
      parameter TAON             =     250; // tAON       ps    RTT turn-on from ODTLon reference
      parameter TWLS             =     195; // tWLS       ps    Setup time for tDQS flop
      parameter TWLH             =     195; // tWLH       ps    Hold time of tDQS flop
      parameter TWLO             =    9000; // tWLO       ps    Write levelization output delay
      parameter TAA_MIN          =   15000; // TAA        ps    Internal READ command to first data
      parameter CL_TIME          =   15000; // CL         ps    Minimum CAS Latency
  `else `ifdef sg187E                       // sg187E is equivalent to the JEDEC DDR3-1066F (7-7-7) speed bin
      parameter TCK_MIN          =    1875; // tCK        ps    Minimum Clock Cycle Time
      parameter TJIT_PER         =      90; // tJIT(per)  ps    Period JItter
      parameter TJIT_CC          =     180; // tJIT(cc)   ps    Cycle to Cycle jitter
      parameter TERR_2PER        =     132; // tERR(2per) ps    Accumulated Error (2-cycle)
      parameter TERR_3PER        =     157; // tERR(3per) ps    Accumulated Error (3-cycle)
      parameter TERR_4PER        =     175; // tERR(4per) ps    Accumulated Error (4-cycle)
      parameter TERR_5PER        =     188; // tERR(5per) ps    Accumulated Error (5-cycle)
      parameter TERR_6PER        =     200; // tERR(6per) ps    Accumulated Error (6-cycle)
      parameter TERR_7PER        =     209; // tERR(7per) ps    Accumulated Error (7-cycle)
      parameter TERR_8PER        =     217; // tERR(8per) ps    Accumulated Error (8-cycle)
      parameter TERR_9PER        =     224; // tERR(9per) ps    Accumulated Error (9-cycle)
      parameter TERR_10PER       =     231; // tERR(10per)ps    Accumulated Error (10-cycle)
      parameter TERR_11PER       =     237; // tERR(11per)ps    Accumulated Error (11-cycle)
      parameter TERR_12PER       =     242; // tERR(12per)ps    Accumulated Error (12-cycle)
      parameter TDS              =      75; // tDS        ps    DQ and DM input setup time relative to DQS
      parameter TDH              =     100; // tDH        ps    DQ and DM input hold time relative to DQS
      parameter TDQSQ            =     150; // tDQSQ      ps    DQS-DQ skew, DQS to last DQ valid, per group, per access
      parameter TDQSS            =    0.25; // tDQSS      tCK   Rising clock edge to DQS/DQS# latching transition
      parameter TDSS             =    0.20; // tDSS       tCK   DQS falling edge to CLK rising (setup time)
      parameter TDSH             =    0.20; // tDSH       tCK   DQS falling edge from CLK rising (hold time)
      parameter TDQSCK           =     300; // tDQSCK     ps    DQS output access time from CK/CK#
      parameter TQSH             =    0.38; // tQSH       tCK   DQS Output High Pulse Width
      parameter TQSL             =    0.38; // tQSL       tCK   DQS Output Low Pulse Width
      parameter TDIPW            =     490; // tDIPW      ps    DQ and DM input Pulse Width
      parameter TIPW             =     780; // tIPW       ps    Control and Address input Pulse Width
      parameter TIS              =     275; // tIS        ps    Input Setup Time
      parameter TIH              =     200; // tIH        ps    Input Hold Time
      parameter TRAS_MIN         =   37500; // tRAS       ps    Minimum Active to Precharge command time
      parameter TRC              =   50625; // tRC        ps    Active to Active/Auto Refresh command time
      parameter TRCD             =   13125; // tRCD       ps    Active to Read/Write command time
      parameter TRP              =   13125; // tRP        ps    Precharge command period
      parameter TXP              =    7500; // tXP        ps    Exit power down to a valid command
      parameter TCKE             =    5625; // tCKE       ps    CKE minimum high or low pulse width
      parameter TAON             =     300; // tAON       ps    RTT turn-on from ODTLon reference
      parameter TWLS             =     245; // tWLS       ps    Setup time for tDQS flop
      parameter TWLH             =     245; // tWLH       ps    Hold time of tDQS flop
      parameter TWLO             =    9000; // tWLO       ps    Write levelization output delay
      parameter TAA_MIN          =   13125; // TAA        ps    Internal READ command to first data
      parameter CL_TIME          =   13125; // CL         ps    Minimum CAS Latency
  `else `ifdef sg187                        // sg187  is equivalent to the JEDEC DDR3-1066G (8-8-8) speed bin
      parameter TCK_MIN          =    1875; // tCK        ps    Minimum Clock Cycle Time
      parameter TJIT_PER         =      90; // tJIT(per)  ps    Period JItter
      parameter TJIT_CC          =     180; // tJIT(cc)   ps    Cycle to Cycle jitter
      parameter TERR_2PER        =     132; // tERR(2per) ps    Accumulated Error (2-cycle)
      parameter TERR_3PER        =     157; // tERR(3per) ps    Accumulated Error (3-cycle)
      parameter TERR_4PER        =     175; // tERR(4per) ps    Accumulated Error (4-cycle)
      parameter TERR_5PER        =     188; // tERR(5per) ps    Accumulated Error (5-cycle)
      parameter TERR_6PER        =     200; // tERR(6per) ps    Accumulated Error (6-cycle)
      parameter TERR_7PER        =     209; // tERR(7per) ps    Accumulated Error (7-cycle)
      parameter TERR_8PER        =     217; // tERR(8per) ps    Accumulated Error (8-cycle)
      parameter TERR_9PER        =     224; // tERR(9per) ps    Accumulated Error (9-cycle)
      parameter TERR_10PER       =     231; // tERR(10per)ps    Accumulated Error (10-cycle)
      parameter TERR_11PER       =     237; // tERR(11per)ps    Accumulated Error (11-cycle)
      parameter TERR_12PER       =     242; // tERR(12per)ps    Accumulated Error (12-cycle)
      parameter TDS              =      75; // tDS        ps    DQ and DM input setup time relative to DQS
      parameter TDH              =     100; // tDH        ps    DQ and DM input hold time relative to DQS
      parameter TDQSQ            =     150; // tDQSQ      ps    DQS-DQ skew, DQS to last DQ valid, per group, per access
      parameter TDQSS            =    0.25; // tDQSS      tCK   Rising clock edge to DQS/DQS# latching transition
      parameter TDSS             =    0.20; // tDSS       tCK   DQS falling edge to CLK rising (setup time)
      parameter TDSH             =    0.20; // tDSH       tCK   DQS falling edge from CLK rising (hold time)
      parameter TDQSCK           =     300; // tDQSCK     ps    DQS output access time from CK/CK#
      parameter TQSH             =    0.38; // tQSH       tCK   DQS Output High Pulse Width
      parameter TQSL             =    0.38; // tQSL       tCK   DQS Output Low Pulse Width
      parameter TDIPW            =     490; // tDIPW      ps    DQ and DM input Pulse Width
      parameter TIPW             =     780; // tIPW       ps    Control and Address input Pulse Width
      parameter TIS              =     275; // tIS        ps    Input Setup Time
      parameter TIH              =     200; // tIH        ps    Input Hold Time
      parameter TRAS_MIN         =   37500; // tRAS       ps    Minimum Active to Precharge command time
      parameter TRC              =   52500; // tRC        ps    Active to Active/Auto Refresh command time
      parameter TRCD             =   15000; // tRCD       ps    Active to Read/Write command time
      parameter TRP              =   15000; // tRP        ps    Precharge command period
      parameter TXP              =    7500; // tXP        ps    Exit power down to a valid command
      parameter TCKE             =    5625; // tCKE       ps    CKE minimum high or low pulse width
      parameter TAON             =     300; // tAON       ps    RTT turn-on from ODTLon reference
      parameter TWLS             =     245; // tWLS       ps    Setup time for tDQS flop
      parameter TWLH             =     245; // tWLH       ps    Hold time of tDQS flop
      parameter TWLO             =    9000; // tWLO       ps    Write levelization output delay
      parameter TAA_MIN          =   15000; // TAA        ps    Internal READ command to first data
      parameter CL_TIME          =   15000; // CL         ps    Minimum CAS Latency
  `else `ifdef sg25E                        // sg25E is equivalent to the JEDEC DDR3-800D (5-5-5) speed bin
      parameter TCK_MIN          =    2500; // tCK        ps    Minimum Clock Cycle Time
      parameter TJIT_PER         =     100; // tJIT(per)  ps    Period JItter
      parameter TJIT_CC          =     200; // tJIT(cc)   ps    Cycle to Cycle jitter
      parameter TERR_2PER        =     147; // tERR(2per) ps    Accumulated Error (2-cycle)
      parameter TERR_3PER        =     175; // tERR(3per) ps    Accumulated Error (3-cycle)
      parameter TERR_4PER        =     194; // tERR(4per) ps    Accumulated Error (4-cycle)
      parameter TERR_5PER        =     209; // tERR(5per) ps    Accumulated Error (5-cycle)
      parameter TERR_6PER        =     222; // tERR(6per) ps    Accumulated Error (6-cycle)
      parameter TERR_7PER        =     232; // tERR(7per) ps    Accumulated Error (7-cycle)
      parameter TERR_8PER        =     241; // tERR(8per) ps    Accumulated Error (8-cycle)
      parameter TERR_9PER        =     249; // tERR(9per) ps    Accumulated Error (9-cycle)
      parameter TERR_10PER       =     257; // tERR(10per)ps    Accumulated Error (10-cycle)
      parameter TERR_11PER       =     263; // tERR(11per)ps    Accumulated Error (11-cycle)
      parameter TERR_12PER       =     269; // tERR(12per)ps    Accumulated Error (12-cycle)
      parameter TDS              =     125; // tDS        ps    DQ and DM input setup time relative to DQS
      parameter TDH              =     150; // tDH        ps    DQ and DM input hold time relative to DQS
      parameter TDQSQ            =     200; // tDQSQ      ps    DQS-DQ skew, DQS to last DQ valid, per group, per access
      parameter TDQSS            =    0.25; // tDQSS      tCK   Rising clock edge to DQS/DQS# latching transition
      parameter TDSS             =    0.20; // tDSS       tCK   DQS falling edge to CLK rising (setup time)
      parameter TDSH             =    0.20; // tDSH       tCK   DQS falling edge from CLK rising (hold time)
      parameter TDQSCK           =     400; // tDQSCK     ps    DQS output access time from CK/CK#
      parameter TQSH             =    0.38; // tQSH       tCK   DQS Output High Pulse Width
      parameter TQSL             =    0.38; // tQSL       tCK   DQS Output Low Pulse Width
      parameter TDIPW            =     600; // tDIPW      ps    DQ and DM input Pulse Width
      parameter TIPW             =     900; // tIPW       ps    Control and Address input Pulse Width
      parameter TIS              =     350; // tIS        ps    Input Setup Time
      parameter TIH              =     275; // tIH        ps    Input Hold Time
      parameter TRAS_MIN         =   37500; // tRAS       ps    Minimum Active to Precharge command time
      parameter TRC              =   50000; // tRC        ps    Active to Active/Auto Refresh command time
      parameter TRCD             =   12500; // tRCD       ps    Active to Read/Write command time
      parameter TRP              =   12500; // tRP        ps    Precharge command period
      parameter TXP              =    7500; // tXP        ps    Exit power down to a valid command
      parameter TCKE             =    7500; // tCKE       ps    CKE minimum high or low pulse width
      parameter TAON             =     400; // tAON       ps    RTT turn-on from ODTLon reference
      parameter TWLS             =     325; // tWLS       ps    Setup time for tDQS flop
      parameter TWLH             =     325; // tWLH       ps    Hold time of tDQS flop
      parameter TWLO             =    9000; // tWLO       ps    Write levelization output delay
      parameter TAA_MIN          =   12500; // TAA        ps    Internal READ command to first data
      parameter CL_TIME          =   12500; // CL         ps    Minimum CAS Latency
  `else `define sg25                        // sg25 is equivalent to the JEDEC DDR3-800E (6-6-6) speed bin
      parameter TCK_MIN          =    2500; // tCK        ps    Minimum Clock Cycle Time
      parameter TJIT_PER         =     100; // tJIT(per)  ps    Period JItter
      parameter TJIT_CC          =     200; // tJIT(cc)   ps    Cycle to Cycle jitter
      parameter TERR_2PER        =     147; // tERR(2per) ps    Accumulated Error (2-cycle)
      parameter TERR_3PER        =     175; // tERR(3per) ps    Accumulated Error (3-cycle)
      parameter TERR_4PER        =     194; // tERR(4per) ps    Accumulated Error (4-cycle)
      parameter TERR_5PER        =     209; // tERR(5per) ps    Accumulated Error (5-cycle)
      parameter TERR_6PER        =     222; // tERR(6per) ps    Accumulated Error (6-cycle)
      parameter TERR_7PER        =     232; // tERR(7per) ps    Accumulated Error (7-cycle)
      parameter TERR_8PER        =     241; // tERR(8per) ps    Accumulated Error (8-cycle)
      parameter TERR_9PER        =     249; // tERR(9per) ps    Accumulated Error (9-cycle)
      parameter TERR_10PER       =     257; // tERR(10per)ps    Accumulated Error (10-cycle)
      parameter TERR_11PER       =     263; // tERR(11per)ps    Accumulated Error (11-cycle)
      parameter TERR_12PER       =     269; // tERR(12per)ps    Accumulated Error (12-cycle)
      parameter TDS              =     125; // tDS        ps    DQ and DM input setup time relative to DQS
      parameter TDH              =     150; // tDH        ps    DQ and DM input hold time relative to DQS
      parameter TDQSQ            =     200; // tDQSQ      ps    DQS-DQ skew, DQS to last DQ valid, per group, per access
      parameter TDQSS            =    0.25; // tDQSS      tCK   Rising clock edge to DQS/DQS# latching transition
      parameter TDSS             =    0.20; // tDSS       tCK   DQS falling edge to CLK rising (setup time)
      parameter TDSH             =    0.20; // tDSH       tCK   DQS falling edge from CLK rising (hold time)
      parameter TDQSCK           =     400; // tDQSCK     ps    DQS output access time from CK/CK#
      parameter TQSH             =    0.38; // tQSH       tCK   DQS Output High Pulse Width
      parameter TQSL             =    0.38; // tQSL       tCK   DQS Output Low Pulse Width
      parameter TDIPW            =     600; // tDIPW      ps    DQ and DM input Pulse Width
      parameter TIPW             =     900; // tIPW       ps    Control and Address input Pulse Width
      parameter TIS              =     350; // tIS        ps    Input Setup Time
      parameter TIH              =     275; // tIH        ps    Input Hold Time
      parameter TRAS_MIN         =   37500; // tRAS       ps    Minimum Active to Precharge command time
      parameter TRC              =   52500; // tRC        ps    Active to Active/Auto Refresh command time
      parameter TRCD             =   15000; // tRCD       ps    Active to Read/Write command time
      parameter TRP              =   15000; // tRP        ps    Precharge command period
      parameter TXP              =    7500; // tXP        ps    Exit power down to a valid command
      parameter TCKE             =    7500; // tCKE       ps    CKE minimum high or low pulse width
      parameter TAON             =     400; // tAON       ps    RTT turn-on from ODTLon reference
      parameter TWLS             =     325; // tWLS       ps    Setup time for tDQS flop
      parameter TWLH             =     325; // tWLH       ps    Hold time of tDQS flop
      parameter TWLO             =    9000; // tWLO       ps    Write levelization output delay
      parameter TAA_MIN          =   15000; // TAA        ps    Internal READ command to first data
      parameter CL_TIME          =   15000; // CL         ps    Minimum CAS Latency
  `endif `endif `endif `endif `endif `endif `endif `endif `endif `endif `endif `endif `endif

  `ifdef x16
    `ifdef sg094E
      parameter TRRD             =    7500; // tRRD       ps     (2KB page size) Active bank a to Active bank b command time
      parameter TFAW             =   30000; // tFAW       ps     (2KB page size) Four Bank Activate window
    `else `ifdef sg094
      parameter TRRD             =    7500; // tRRD       ps     (2KB page size) Active bank a to Active bank b command time
      parameter TFAW             =   30000; // tFAW       ps     (2KB page size) Four Bank Activate window
    `else `ifdef sg107F
      parameter TRRD             =    7500; // tRRD       ps     (2KB page size) Active bank a to Active bank b command time
      parameter TFAW             =   35000; // tFAW       ps     (2KB page size) Four Bank Activate window
    `else `ifdef sg107E
      parameter TRRD             =    7500; // tRRD       ps     (2KB page size) Active bank a to Active bank b command time
      parameter TFAW             =   35000; // tFAW       ps     (2KB page size) Four Bank Activate window
    `else `ifdef sg107
      parameter TRRD             =    7500; // tRRD       ps     (2KB page size) Active bank a to Active bank b command time
      parameter TFAW             =   35000; // tFAW       ps     (2KB page size) Four Bank Activate window
    `else `ifdef sg125F
      parameter TRRD             =    7500; // tRRD       ps     (2KB page size) Active bank a to Active bank b command time
      parameter TFAW             =   40000; // tFAW       ps     (2KB page size) Four Bank Activate window
    `else `ifdef sg125E
      parameter TRRD             =    7500; // tRRD       ps     (2KB page size) Active bank a to Active bank b command time
      parameter TFAW             =   40000; // tFAW       ps     (2KB page size) Four Bank Activate window
    `else `ifdef sg125
      parameter TRRD             =    7500; // tRRD       ps     (2KB page size) Active bank a to Active bank b command time
      parameter TFAW             =   40000; // tFAW       ps     (2KB page size) Four Bank Activate window
    `else `ifdef sg15E
      parameter TRRD             =    7500; // tRRD       ps     (2KB page size) Active bank a to Active bank b command time
      parameter TFAW             =   45000; // tFAW       ps     (2KB page size) Four Bank Activate window
    `else `ifdef sg15
      parameter TRRD             =    7500; // tRRD       ps     (2KB page size) Active bank a to Active bank b command time
      parameter TFAW             =   45000; // tFAW       ps     (2KB page size) Four Bank Activate window
    `else // sg187E, sg187, sg25, sg25E
      parameter TRRD             =   10000; // tRRD       ps     (2KB page size) Active bank a to Active bank b command time
      parameter TFAW             =   50000; // tFAW       ps     (2KB page size) Four Bank Activate window
    `endif `endif `endif `endif `endif `endif `endif `endif `endif `endif
  `else // x4, x8
    `ifdef sg094E
      parameter TRRD             =    6000; // tRRD       ps     (1KB page size) Active bank a to Active bank b command time
      parameter TFAW             =   25000; // tFAW       ps     (1KB page size) Four Bank Activate window
    `else `ifdef sg094
      parameter TRRD             =    6000; // tRRD       ps     (1KB page size) Active bank a to Active bank b command time
      parameter TFAW             =   25000; // tFAW       ps     (1KB page size) Four Bank Activate window
    `else `ifdef sg107F
      parameter TRRD             =    6000; // tRRD       ps     (1KB page size) Active bank a to Active bank b command time
      parameter TFAW             =   25000; // tFAW       ps     (1KB page size) Four Bank Activate window
    `else `ifdef sg107E
      parameter TRRD             =    6000; // tRRD       ps     (1KB page size) Active bank a to Active bank b command time
      parameter TFAW             =   25000; // tFAW       ps     (1KB page size) Four Bank Activate window
    `else `ifdef sg107
      parameter TRRD             =    6000; // tRRD       ps     (1KB page size) Active bank a to Active bank b command time
      parameter TFAW             =   25000; // tFAW       ps     (1KB page size) Four Bank Activate window
    `else `ifdef sg125F
      parameter TRRD             =    6000; // tRRD       ps     (1KB page size) Active bank a to Active bank b command time
      parameter TFAW             =   30000; // tFAW       ps     (1KB page size) Four Bank Activate window
    `else `ifdef sg125E
      parameter TRRD             =    6000; // tRRD       ps     (1KB page size) Active bank a to Active bank b command time
      parameter TFAW             =   30000; // tFAW       ps     (1KB page size) Four Bank Activate window
    `else `ifdef sg125
      parameter TRRD             =    6000; // tRRD       ps     (1KB page size) Active bank a to Active bank b command time
      parameter TFAW             =   30000; // tFAW       ps     (1KB page size) Four Bank Activate window
    `else `ifdef sg15E
      parameter TRRD             =    6000; // tRRD       ps     (1KB page size) Active bank a to Active bank b command time
      parameter TFAW             =   30000; // tFAW       ps     (1KB page size) Four Bank Activate window
    `else `ifdef sg15
      parameter TRRD             =    6000; // tRRD       ps     (1KB page size) Active bank a to Active bank b command time
      parameter TFAW             =   30000; // tFAW       ps     (1KB page size) Four Bank Activate window
    `else `ifdef sg187E
      parameter TRRD             =    7500; // tRRD       ps     (1KB page size) Active bank a to Active bank b command time
      parameter TFAW             =   37500; // tFAW       ps     (1KB page size) Four Bank Activate window
    `else `ifdef sg187
      parameter TRRD             =    7500; // tRRD       ps     (1KB page size) Active bank a to Active bank b command time
      parameter TFAW             =   37500; // tFAW       ps     (1KB page size) Four Bank Activate window
    `else // sg25, sg25E
      parameter TRRD             =   10000; // tRRD       ps     (1KB page size) Active bank a to Active bank b command time
      parameter TFAW             =   40000; // tFAW       ps     (1KB page size) Four Bank Activate window
    `endif `endif `endif `endif `endif `endif `endif `endif `endif `endif `endif `endif
`endif

      // Timing Parameters

      // Mode Register
      parameter CL_MIN           =       5; // CL         tCK   Minimum CAS Latency
      parameter CL_MAX           =      14; // CL         tCK   Maximum CAS Latency
      parameter AL_MIN           =       0; // AL         tCK   Minimum Additive Latency
      parameter AL_MAX           =       2; // AL         tCK   Maximum Additive Latency
      parameter WR_MIN           =       5; // WR         tCK   Minimum Write Recovery
      parameter WR_MAX           =      16; // WR         tCK   Maximum Write Recovery
      parameter BL_MIN           =       4; // BL         tCK   Minimum Burst Length
      parameter BL_MAX           =       8; // BL         tCK   Minimum Burst Length
      parameter CWL_MIN          =       5; // CWL        tCK   Minimum CAS Write Latency
      parameter CWL_MAX          =      10; // CWL        tCK   Maximum CAS Write Latency

      // Clock
      parameter TCK_MAX          =    3300; // tCK        ps    Maximum Clock Cycle Time
      parameter TCH_AVG_MIN      =    0.47; // tCH        tCK   Minimum Clock High-Level Pulse Width
      parameter TCL_AVG_MIN      =    0.47; // tCL        tCK   Minimum Clock Low-Level Pulse Width
      parameter TCH_AVG_MAX      =    0.53; // tCH        tCK   Maximum Clock High-Level Pulse Width
      parameter TCL_AVG_MAX      =    0.53; // tCL        tCK   Maximum Clock Low-Level Pulse Width
      parameter TCH_ABS_MIN      =    0.43; // tCH        tCK   Minimum Clock High-Level Pulse Width
      parameter TCL_ABS_MIN      =    0.43; // tCL        tCK   Maximum Clock Low-Level Pulse Width
      parameter TCKE_TCK         =       3; // tCKE       tCK   CKE minimum high or low pulse width
      parameter TAA_MAX          =   20000; // TAA        ps    Internal READ command to first data

      // Data OUT
      parameter TQH              =    0.38; // tQH        ps    DQ output hold time from DQS, DQS#
      // Data Strobe OUT
      parameter TRPRE            =    0.90; // tRPRE      tCK   DQS Read Preamble
      parameter TRPST            =    0.30; // tRPST      tCK   DQS Read Postamble
      // Data Strobe IN
      parameter TDQSH            =    0.45; // tDQSH      tCK   DQS input High Pulse Width
      parameter TDQSL            =    0.45; // tDQSL      tCK   DQS input Low Pulse Width
      parameter TWPRE            =    0.90; // tWPRE      tCK   DQS Write Preamble
      parameter TWPST            =    0.30; // tWPST      tCK   DQS Write Postamble
      // Command and Address
      parameter TZQCS            =      64; // tZQCS      tCK   ZQ Cal (Short) time
      parameter TZQINIT          =     512; // tZQinit    tCK   ZQ Cal (Long) time
      parameter TZQOPER          =     256; // tZQoper    tCK   ZQ Cal (Long) time
      parameter TCCD             =       4; // tCCD       tCK   Cas to Cas command delay
      parameter TCCD_DG          =       2; // tCCD_DG    tCK   Cas to Cas command delay to different group
      parameter TRAS_MAX         =    60e9; // tRAS       ps    Maximum Active to Precharge command time
      parameter TWR              =   15000; // tWR        ps    Write recovery time
      parameter TMRD             =       4; // tMRD       tCK   Load Mode Register command cycle time
      parameter TMOD             =   15000; // tMOD       ps    LOAD MODE to non-LOAD MODE command cycle time
      parameter TMOD_TCK         =      12; // tMOD       tCK   LOAD MODE to non-LOAD MODE command cycle time
      parameter TRRD_TCK         =       4; // tRRD       tCK   Active bank a to Active bank b command time
      parameter TRRD_DG          =    3000; // tRRD_DG    ps     Active bank a to Active bank b command time to different group
      parameter TRRD_DG_TCK      =       2; // tRRD_DG    tCK   Active bank a to Active bank b command time to different group
      parameter TRTP             =    7500; // tRTP       ps    Read to Precharge command delay
      parameter TRTP_TCK         =       4; // tRTP       tCK   Read to Precharge command delay
      parameter TWTR             =    7500; // tWTR       ps    Write to Read command delay
      parameter TWTR_DG          =    3750; // tWTR_DG    ps    Write to Read command delay to different group
      parameter TWTR_TCK         =       4; // tWTR       tCK   Write to Read command delay
      parameter TWTR_DG_TCK      =       2; // tWTR_DG    tCK   Write to Read command delay to different group
      parameter TDLLK            =     512; // tDLLK      tCK   DLL locking time
      // Refresh - 1Gb
      parameter TRFC_MIN         =  110000; // tRFC       ps    Refresh to Refresh Command interval minimum value
      parameter TRFC_MAX         =70312500; // tRFC       ps    Refresh to Refresh Command Interval maximum value
      // Power Down
      parameter TXP_TCK          =       3; // tXP        tCK   Exit power down to a valid command
      parameter TXPDLL           =   24000; // tXPDLL     ps    Exit precharge power down to READ or WRITE command (DLL-off mode)
      parameter TXPDLL_TCK       =      10; // tXPDLL     tCK   Exit precharge power down to READ or WRITE command (DLL-off mode)
      parameter TACTPDEN         =       1; // tACTPDEN   tCK   Timing of last ACT command to power down entry
      parameter TPRPDEN          =       1; // tPREPDEN   tCK   Timing of last PRE command to power down entry
      parameter TREFPDEN         =       1; // tARPDEN    tCK   Timing of last REFRESH command to power down entry
      parameter TCPDED           =       1; // tCPDED     tCK   Command pass disable/enable delay
      parameter TPD_MAX          =TRFC_MAX; // tPD        ps    Power-down entry-to-exit timing
      parameter TXPR             =  120000; // tXPR       ps    Exit Reset from CKE assertion to a valid command
      parameter TXPR_TCK         =       5; // tXPR       tCK   Exit Reset from CKE assertion to a valid command
      // Self Refresh
      parameter TXS              =  120000; // tXS        ps    Exit self refesh to a non-read or write command
      parameter TXS_TCK          =       5; // tXS        tCK   Exit self refesh to a non-read or write command
      parameter TXSDLL           =   TDLLK; // tXSRD      tCK   Exit self refresh to a read or write command
      parameter TISXR            =     TIS; // tISXR      ps    CKE setup time during self refresh exit.
      parameter TCKSRE           =   10000; // tCKSRE     ps    Valid Clock requirement after self refresh entry (SRE)
      parameter TCKSRE_TCK       =       5; // tCKSRE     tCK   Valid Clock requirement after self refresh entry (SRE)
      parameter TCKSRX           =   10000; // tCKSRX     ps    Valid Clock requirement prior to self refresh exit (SRX)
      parameter TCKSRX_TCK       =       5; // tCKSRX     tCK   Valid Clock requirement prior to self refresh exit (SRX)
      parameter TCKESR_TCK       =       4; // tCKESR     tCK   Minimum CKE low width for Self Refresh entry to exit timing
      // ODT
      parameter TAOF             =     0.7; // tAOF       tCK   RTT turn-off from ODTLoff reference
      parameter TAONPD           =    8500; // tAONPD     ps    Asynchronous RTT turn-on delay (Power-Down with DLL frozen)
      parameter TAOFPD           =    8500; // tAONPD     ps    Asynchronous RTT turn-off delay (Power-Down with DLL frozen)
      parameter ODTH4            =       4; // ODTH4      tCK   ODT minimum HIGH time after ODT assertion or write (BL4)
      parameter ODTH8            =       6; // ODTH8      tCK   ODT minimum HIGH time after write (BL8)
      parameter TADC             =     0.7; // tADC       tCK   RTT dynamic change skew
      // Write Levelization
      parameter TWLMRD           =      40; // tWLMRD     tCK   First DQS pulse rising edge after tDQSS margining mode is programmed
      parameter TWLDQSEN         =      25; // tWLDQSEN   tCK   DQS/DQS delay after tDQSS margining mode is programmed
      parameter TWLOE            =    2000; // tWLOE      ps    Write levelization output error

      // Size Parameters based on Part Width

  `ifdef x4
      parameter DM_BITS          =       1; // Set this parameter to control how many Data Mask bits are used
      parameter ADDR_BITS        =      14; // MAX Address Bits
      parameter ROW_BITS         =      14; // Set this parameter to control how many Address bits are used
      parameter COL_BITS         =      11; // Set this parameter to control how many Column bits are used
      parameter DQ_BITS          =       4; // Set this parameter to control how many Data bits are used       **Same as part bit width**
      parameter DQS_BITS         =       1; // Set this parameter to control how many Dqs bits are used
  `else `ifdef x8
      parameter DM_BITS          =       1; // Set this parameter to control how many Data Mask bits are used
      parameter ADDR_BITS        =      14; // MAX Address Bits
      parameter ROW_BITS         =      14; // Set this parameter to control how many Address bits are used
      parameter COL_BITS         =      10; // Set this parameter to control how many Column bits are used
      parameter DQ_BITS          =       8; // Set this parameter to control how many Data bits are used       **Same as part bit width**
      parameter DQS_BITS         =       1; // Set this parameter to control how many Dqs bits are used
  `else `define x16
      parameter DM_BITS          =       2; // Set this parameter to control how many Data Mask bits are used
      parameter ADDR_BITS        =      13; // MAX Address Bits
      parameter ROW_BITS         =      13; // Set this parameter to control how many Address bits are used
      parameter COL_BITS         =      10; // Set this parameter to control how many Column bits are used
      parameter DQ_BITS          =      16; // Set this parameter to control how many Data bits are used       **Same as part bit width**
      parameter DQS_BITS         =       2; // Set this parameter to control how many Dqs bits are used
  `endif `endif

      // Size Parameters
      parameter BA_BITS          =       3; // Set this parmaeter to control how many Bank Address bits are used
      parameter MEM_BITS         =      15; // Set this parameter to control how many write data bursts can be stored in memory.  The default is 2^10=1024.
      parameter AP               =      10; // the address bit that controls auto-precharge and precharge-all
      parameter BC               =      12; // the address bit that controls burst chop
      parameter BL_BITS          =       3; // the number of bits required to count to BL_MAX
      parameter BO_BITS          =       2; // the number of Burst Order Bits

  `ifdef QUAD_RANK
      `define DUAL_RANK // also define DUAL_RANK
      parameter CS_BITS          =       4; // Number of Chip Select Bits
      parameter RANKS            =       4; // Number of Chip Selects
  `else `ifdef DUAL_RANK
      parameter CS_BITS          =       2; // Number of Chip Select Bits
      parameter RANKS            =       2; // Number of Chip Selects
  `else
      parameter CS_BITS          =       2; // Number of Chip Select Bits
      parameter RANKS            =       1; // Number of Chip Selects
  `endif `endif

      // Simulation parameters
      parameter RZQ              =     240; // termination resistance
      parameter PRE_DEF_PAT      =   8'hAA; // value returned during mpr pre-defined pattern readout
      parameter STOP_ON_ERROR    =       1; // If set to 1, the model will halt on command sequence/major errors
      parameter DEBUG            =       1; // Turn on Debug messages
      parameter BUS_DELAY        =       0; // delay in nanoseconds
      parameter RANDOM_OUT_DELAY =       0; // If set to 1, the model will put a random amount of delay on DQ/DQS during reads
      parameter RANDOM_SEED    = 711689044; //seed value for random generator.

      parameter RDQSEN_PRE       =       2; // DQS driving time prior to first read strobe
      parameter RDQSEN_PST       =       1; // DQS driving time after last read strobe
      parameter RDQS_PRE         =       2; // DQS low time prior to first read strobe
      parameter RDQS_PST         =       1; // DQS low time after last read strobe
      parameter RDQEN_PRE        =       0; // DQ/DM driving time prior to first read data
      parameter RDQEN_PST        =       0; // DQ/DM driving time after last read data
      parameter WDQS_PRE         =       2; // DQS half clock periods prior to first write strobe
      parameter WDQS_PST         =       1; // DQS half clock periods after last write strobe

  // check for legal cas latency based on the cas write latency
  function valid_cl;
      input [3:0] cl;
      input [3:0] cwl;

      case ({cwl, cl})
  `ifdef sg094E
          {4'd5, 4'd5 },
          {4'd5, 4'd6 },
          {4'd6, 4'd7 },
          {4'd6, 4'd8 },
          {4'd7, 4'd9 },
          {4'd7, 4'd10},
          {4'd8, 4'd10},
          {4'd8, 4'd11},
          {4'd8, 4'd12},
          {4'd9, 4'd12},
          {4'd9, 4'd13},
          {4'd9, 4'd14},
          {4'd10, 4'd13},
          {4'd10, 4'd14}: valid_cl = 1;
  `else `ifdef sg094
          {4'd5, 4'd6 },
          {4'd6, 4'd7 },
          {4'd6, 4'd8 },
          {4'd7, 4'd10},
          {4'd8, 4'd11},
          {4'd8, 4'd12},
          {4'd9, 4'd13},
          {4'd9, 4'd14},
          {4'd10, 4'd14}: valid_cl = 1;
  `else `ifdef sg107F
          {4'd5, 4'd6 },
          {4'd6, 4'd7 },
          {4'd6, 4'd8 },
          {4'd7, 4'd9 },
          {4'd7, 4'd10},
          {4'd8, 4'd11},
          {4'd8, 4'd12},
          {4'd9, 4'd12},
          {4'd9, 4'd13},
          {4'd9, 4'd14}: valid_cl = 1;
  `else `ifdef sg107E
          {4'd5, 4'd6 },
          {4'd6, 4'd8 },
          {4'd7, 4'd10},
          {4'd8, 4'd12},
          {4'd9, 4'd13},
          {4'd9, 4'd14}: valid_cl = 1;
  `else `ifdef sg107
          {4'd5, 4'd6 },
          {4'd6, 4'd8 },
          {4'd7, 4'd10},
          {4'd8, 4'd12},
          {4'd9, 4'd14}: valid_cl = 1;
  `else `ifdef sg125F
          {4'd5, 4'd5 },
          {4'd5, 4'd6 },
          {4'd6, 4'd7 },
          {4'd6, 4'd8 },
          {4'd7, 4'd8 },
          {4'd7, 4'd9 },
          {4'd7, 4'd10},
          {4'd8, 4'd9 },
          {4'd8, 4'd10},
          {4'd8, 4'd11}: valid_cl = 1;
  `else `ifdef sg125E
          {4'd5, 4'd5 },
          {4'd5, 4'd6 },
          {4'd6, 4'd7 },
          {4'd6, 4'd8 },
          {4'd7, 4'd9 },
          {4'd7, 4'd10},
          {4'd8, 4'd10},
          {4'd8, 4'd11}: valid_cl = 1;
  `else `ifdef sg125
          {4'd5, 4'd6 },
          {4'd6, 4'd7 },
          {4'd6, 4'd8 },
          {4'd7, 4'd9 },
          {4'd7, 4'd10},
          {4'd8, 4'd11}: valid_cl = 1;
  `else `ifdef sg15E
          {4'd5, 4'd6 },
          {4'd6, 4'd7 },
          {4'd6, 4'd8 },
          {4'd7, 4'd9 },
          {4'd7, 4'd10}: valid_cl = 1;
  `else `ifdef sg15
          {4'd5, 4'd6 },
          {4'd6, 4'd8 },
          {4'd7, 4'd10}: valid_cl = 1;
  `else `ifdef sg187E
          {4'd5, 4'd6 },
          {4'd6, 4'd7 },
          {4'd6, 4'd8 }: valid_cl = 1;
  `else `ifdef sg187
          {4'd5, 4'd6 },
          {4'd6, 4'd8 }: valid_cl = 1;
  `else `ifdef sg25E
          {4'd5, 4'd5 },
          {4'd5, 4'd6 }: valid_cl = 1;
  `else `ifdef sg25
          {4'd5, 4'd6 }: valid_cl = 1;
  `endif `endif `endif `endif `endif `endif `endif `endif `endif  `endif `endif `endif `endif `endif
          default : valid_cl = 0;
      endcase
  endfunction

  // find the minimum valid cas write latency
  function [3:0] min_cwl;
      input period;
      real period;
      min_cwl = (period >= 2500.0) ? 5:
                (period >= 1875.0) ? 6:
                (period >= 1500.0) ? 7:
                (period >= 1250.0) ? 8:
                (period >= 15e3/14) ? 9:
                10; // (period >= 937.5)
  endfunction

  // find the minimum valid cas latency
  function [3:0] min_cl;
      input period;
      real period;
      reg [3:0] cwl;
      reg [3:0] cl;
      begin
          cwl = min_cwl(period);
          for (cl=CL_MAX; cl>=CL_MIN; cl=cl-1) begin
              if (valid_cl(cl, cwl)) begin
                  min_cl = cl;
              end
          end
      end
  endfunction

`elsif x2Gb // 2Gb parts
                                            // SYMBOL     UNITS DESCRIPTION
                                            // ------     ----- -----------
  `ifdef sg15E                              // sg15E is equivelant to the JEDEC DDR3-1333H (9-9-9) speed bin
      parameter TCK_MIN          =    1500; // tCK        ps    Minimum Clock Cycle Time
      parameter TJIT_PER         =      80; // tJIT(per)  ps    Period JItter
      parameter TJIT_CC          =     160; // tJIT(cc)   ps    Cycle to Cycle jitter
      parameter TERR_2PER        =     118; // tERR(2per) ps    Accumulated Error (2-cycle)
      parameter TERR_3PER        =     140; // tERR(3per) ps    Accumulated Error (3-cycle)
      parameter TERR_4PER        =     155; // tERR(4per) ps    Accumulated Error (4-cycle)
      parameter TERR_5PER        =     168; // tERR(5per) ps    Accumulated Error (5-cycle)
      parameter TERR_6PER        =     177; // tERR(6per) ps    Accumulated Error (6-cycle)
      parameter TERR_7PER        =     186; // tERR(7per) ps    Accumulated Error (7-cycle)
      parameter TERR_8PER        =     193; // tERR(8per) ps    Accumulated Error (8-cycle)
      parameter TERR_9PER        =     200; // tERR(9per) ps    Accumulated Error (9-cycle)
      parameter TERR_10PER       =     205; // tERR(10per)ps    Accumulated Error (10-cycle)
      parameter TERR_11PER       =     210; // tERR(11per)ps    Accumulated Error (11-cycle)
      parameter TERR_12PER       =     215; // tERR(12per)ps    Accumulated Error (12-cycle)
      parameter TDS              =      30; // tDS        ps    DQ and DM input setup time relative to DQS
      parameter TDH              =      65; // tDH        ps    DQ and DM input hold time relative to DQS
      parameter TDQSQ            =     125; // tDQSQ      ps    DQS-DQ skew, DQS to last DQ valid, per group, per access
      parameter TDQSS            =    0.25; // tDQSS      tCK   Rising clock edge to DQS/DQS# latching transition
      parameter TDSS             =    0.20; // tDSS       tCK   DQS falling edge to CLK rising (setup time)
      parameter TDSH             =    0.20; // tDSH       tCK   DQS falling edge from CLK rising (hold time)
      parameter TDQSCK           =     255; // tDQSCK     ps    DQS output access time from CK/CK#
      parameter TQSH             =    0.40; // tQSH       tCK   DQS Output High Pulse Width
      parameter TQSL             =    0.40; // tQSL       tCK   DQS Output Low Pulse Width
      parameter TDIPW            =     400; // tDIPW      ps    DQ and DM input Pulse Width
      parameter TIPW             =     620; // tIPW       ps    Control and Address input Pulse Width
      parameter TIS              =     190; // tIS        ps    Input Setup Time
      parameter TIH              =     140; // tIH        ps    Input Hold Time
      parameter TRAS_MIN         =   36000; // tRAS       ps    Minimum Active to Precharge command time
      parameter TRC              =   49500; // tRC        ps    Active to Active/Auto Refresh command time
      parameter TRCD             =   13500; // tRCD       ps    Active to Read/Write command time
      parameter TRP              =   13500; // tRP        ps    Precharge command period
      parameter TXP              =    6000; // tXP        ps    Exit power down to a valid command
      parameter TCKE             =    5625; // tCKE       ps    CKE minimum high or low pulse width
      parameter TAON             =     250; // tAON       ps    RTT turn-on from ODTLon reference
      parameter TWLS             =     195; // tWLS       ps    Setup time for tDQS flop
      parameter TWLH             =     195; // tWLH       ps    Hold time of tDQS flop
      parameter TWLO             =    9000; // tWLO       ps    Write levelization output delay
      parameter TAA_MIN          =   13500; // TAA        ps    Internal READ command to first data
      parameter CL_TIME          =   13500; // CL         ps    Minimum CAS Latency
  `else `ifdef sg15                         // sg15 is equivelant to the JEDEC DDR3-1333J (10-10-10) speed bin
      parameter TCK_MIN          =    1500; // tCK        ps    Minimum Clock Cycle Time
      parameter TJIT_PER         =      80; // tJIT(per)  ps    Period JItter
      parameter TJIT_CC          =     160; // tJIT(cc)   ps    Cycle to Cycle jitter
      parameter TERR_2PER        =     118; // tERR(2per) ps    Accumulated Error (2-cycle)
      parameter TERR_3PER        =     140; // tERR(3per) ps    Accumulated Error (3-cycle)
      parameter TERR_4PER        =     155; // tERR(4per) ps    Accumulated Error (4-cycle)
      parameter TERR_5PER        =     168; // tERR(5per) ps    Accumulated Error (5-cycle)
      parameter TERR_6PER        =     177; // tERR(6per) ps    Accumulated Error (6-cycle)
      parameter TERR_7PER        =     186; // tERR(7per) ps    Accumulated Error (7-cycle)
      parameter TERR_8PER        =     193; // tERR(8per) ps    Accumulated Error (8-cycle)
      parameter TERR_9PER        =     200; // tERR(9per) ps    Accumulated Error (9-cycle)
      parameter TERR_10PER       =     205; // tERR(10per)ps    Accumulated Error (10-cycle)
      parameter TERR_11PER       =     210; // tERR(11per)ps    Accumulated Error (11-cycle)
      parameter TERR_12PER       =     215; // tERR(12per)ps    Accumulated Error (12-cycle)
      parameter TDS              =      30; // tDS        ps    DQ and DM input setup time relative to DQS
      parameter TDH              =      65; // tDH        ps    DQ and DM input hold time relative to DQS
      parameter TDQSQ            =     125; // tDQSQ      ps    DQS-DQ skew, DQS to last DQ valid, per group, per access
      parameter TDQSS            =    0.25; // tDQSS      tCK   Rising clock edge to DQS/DQS# latching transition
      parameter TDSS             =    0.20; // tDSS       tCK   DQS falling edge to CLK rising (setup time)
      parameter TDSH             =    0.20; // tDSH       tCK   DQS falling edge from CLK rising (hold time)
      parameter TDQSCK           =     255; // tDQSCK     ps    DQS output access time from CK/CK#
      parameter TQSH             =    0.40; // tQSH       tCK   DQS Output High Pulse Width
      parameter TQSL             =    0.40; // tQSL       tCK   DQS Output Low Pulse Width
      parameter TDIPW            =     400; // tDIPW      ps    DQ and DM input Pulse Width
      parameter TIPW             =     620; // tIPW       ps    Control and Address input Pulse Width
      parameter TIS              =     190; // tIS        ps    Input Setup Time
      parameter TIH              =     140; // tIH        ps    Input Hold Time
      parameter TRAS_MIN         =   36000; // tRAS       ps    Minimum Active to Precharge command time
      parameter TRC              =   51000; // tRC        ps    Active to Active/Auto Refresh command time
      parameter TRCD             =   15000; // tRCD       ps    Active to Read/Write command time
      parameter TRP              =   15000; // tRP        ps    Precharge command period
      parameter TXP              =    6000; // tXP        ps    Exit power down to a valid command
      parameter TCKE             =    5625; // tCKE       ps    CKE minimum high or low pulse width
      parameter TAON             =     250; // tAON       ps    RTT turn-on from ODTLon reference
      parameter TWLS             =     195; // tWLS       ps    Setup time for tDQS flop
      parameter TWLH             =     195; // tWLH       ps    Hold time of tDQS flop
      parameter TWLO             =    9000; // tWLO       ps    Write levelization output delay
      parameter TAA_MIN          =   15000; // TAA        ps    Internal READ command to first data
      parameter CL_TIME          =   15000; // CL         ps    Minimum CAS Latency
  `else `ifdef sg187E                       // sg187E is equivelant to the JEDEC DDR3-1066F (7-7-7) speed bin
      parameter TCK_MIN          =    1875; // tCK        ps    Minimum Clock Cycle Time
      parameter TJIT_PER         =      90; // tJIT(per)  ps    Period JItter
      parameter TJIT_CC          =     180; // tJIT(cc)   ps    Cycle to Cycle jitter
      parameter TERR_2PER        =     132; // tERR(2per) ps    Accumulated Error (2-cycle)
      parameter TERR_3PER        =     157; // tERR(3per) ps    Accumulated Error (3-cycle)
      parameter TERR_4PER        =     175; // tERR(4per) ps    Accumulated Error (4-cycle)
      parameter TERR_5PER        =     188; // tERR(5per) ps    Accumulated Error (5-cycle)
      parameter TERR_6PER        =     200; // tERR(6per) ps    Accumulated Error (6-cycle)
      parameter TERR_7PER        =     209; // tERR(7per) ps    Accumulated Error (7-cycle)
      parameter TERR_8PER        =     217; // tERR(8per) ps    Accumulated Error (8-cycle)
      parameter TERR_9PER        =     224; // tERR(9per) ps    Accumulated Error (9-cycle)
      parameter TERR_10PER       =     231; // tERR(10per)ps    Accumulated Error (10-cycle)
      parameter TERR_11PER       =     237; // tERR(11per)ps    Accumulated Error (11-cycle)
      parameter TERR_12PER       =     242; // tERR(12per)ps    Accumulated Error (12-cycle)
      parameter TDS              =      25; // tDS        ps    DQ and DM input setup time relative to DQS
      parameter TDH              =     100; // tDH        ps    DQ and DM input hold time relative to DQS
      parameter TDQSQ            =     150; // tDQSQ      ps    DQS-DQ skew, DQS to last DQ valid, per group, per access
      parameter TDQSS            =    0.25; // tDQSS      tCK   Rising clock edge to DQS/DQS# latching transition
      parameter TDSS             =    0.20; // tDSS       tCK   DQS falling edge to CLK rising (setup time)
      parameter TDSH             =    0.20; // tDSH       tCK   DQS falling edge from CLK rising (hold time)
      parameter TDQSCK           =     300; // tDQSCK     ps    DQS output access time from CK/CK#
      parameter TQSH             =    0.38; // tQSH       tCK   DQS Output High Pulse Width
      parameter TQSL             =    0.38; // tQSL       tCK   DQS Output Low Pulse Width
      parameter TDIPW            =     490; // tDIPW      ps    DQ and DM input Pulse Width
      parameter TIPW             =     780; // tIPW       ps    Control and Address input Pulse Width
      parameter TIS              =     125; // tIS        ps    Input Setup Time
      parameter TIH              =     200; // tIH        ps    Input Hold Time
      parameter TRAS_MIN         =   37500; // tRAS       ps    Minimum Active to Precharge command time
      parameter TRC              =   50625; // tRC        ps    Active to Active/Auto Refresh command time
      parameter TRCD             =   13125; // tRCD       ps    Active to Read/Write command time
      parameter TRP              =   13125; // tRP        ps    Precharge command period
      parameter TXP              =    7500; // tXP        ps    Exit power down to a valid command
      parameter TCKE             =    5625; // tCKE       ps    CKE minimum high or low pulse width
      parameter TAON             =     300; // tAON       ps    RTT turn-on from ODTLon reference
      parameter TWLS             =     245; // tWLS       ps    Setup time for tDQS flop
      parameter TWLH             =     245; // tWLH       ps    Hold time of tDQS flop
      parameter TWLO             =    9000; // tWLO       ps    Write levelization output delay
      parameter TAA_MIN          =   13125; // TAA        ps    Internal READ command to first data
      parameter CL_TIME          =   13125; // CL         ps    Minimum CAS Latency
  `else `ifdef sg187                        // sg187  is equivelant to the JEDEC DDR3-1066G (8-8-8) speed bin
      parameter TCK_MIN          =    1875; // tCK        ps    Minimum Clock Cycle Time
      parameter TJIT_PER         =      90; // tJIT(per)  ps    Period JItter
      parameter TJIT_CC          =     180; // tJIT(cc)   ps    Cycle to Cycle jitter
      parameter TERR_2PER        =     132; // tERR(2per) ps    Accumulated Error (2-cycle)
      parameter TERR_3PER        =     157; // tERR(3per) ps    Accumulated Error (3-cycle)
      parameter TERR_4PER        =     175; // tERR(4per) ps    Accumulated Error (4-cycle)
      parameter TERR_5PER        =     188; // tERR(5per) ps    Accumulated Error (5-cycle)
      parameter TERR_6PER        =     200; // tERR(6per) ps    Accumulated Error (6-cycle)
      parameter TERR_7PER        =     209; // tERR(7per) ps    Accumulated Error (7-cycle)
      parameter TERR_8PER        =     217; // tERR(8per) ps    Accumulated Error (8-cycle)
      parameter TERR_9PER        =     224; // tERR(9per) ps    Accumulated Error (9-cycle)
      parameter TERR_10PER       =     231; // tERR(10per)ps    Accumulated Error (10-cycle)
      parameter TERR_11PER       =     237; // tERR(11per)ps    Accumulated Error (11-cycle)
      parameter TERR_12PER       =     242; // tERR(12per)ps    Accumulated Error (12-cycle)
      parameter TDS              =      25; // tDS        ps    DQ and DM input setup time relative to DQS
      parameter TDH              =     100; // tDH        ps    DQ and DM input hold time relative to DQS
      parameter TDQSQ            =     150; // tDQSQ      ps    DQS-DQ skew, DQS to last DQ valid, per group, per access
      parameter TDQSS            =    0.25; // tDQSS      tCK   Rising clock edge to DQS/DQS# latching transition
      parameter TDSS             =    0.20; // tDSS       tCK   DQS falling edge to CLK rising (setup time)
      parameter TDSH             =    0.20; // tDSH       tCK   DQS falling edge from CLK rising (hold time)
      parameter TDQSCK           =     300; // tDQSCK     ps    DQS output access time from CK/CK#
      parameter TQSH             =    0.38; // tQSH       tCK   DQS Output High Pulse Width
      parameter TQSL             =    0.38; // tQSL       tCK   DQS Output Low Pulse Width
      parameter TDIPW            =     490; // tDIPW      ps    DQ and DM input Pulse Width
      parameter TIPW             =     780; // tIPW       ps    Control and Address input Pulse Width
      parameter TIS              =     125; // tIS        ps    Input Setup Time
      parameter TIH              =     200; // tIH        ps    Input Hold Time
      parameter TRAS_MIN         =   37500; // tRAS       ps    Minimum Active to Precharge command time
      parameter TRC              =   52500; // tRC        ps    Active to Active/Auto Refresh command time
      parameter TRCD             =   15000; // tRCD       ps    Active to Read/Write command time
      parameter TRP              =   15000; // tRP        ps    Precharge command period
      parameter TXP              =    7500; // tXP        ps    Exit power down to a valid command
      parameter TCKE             =    5625; // tCKE       ps    CKE minimum high or low pulse width
      parameter TAON             =     300; // tAON       ps    RTT turn-on from ODTLon reference
      parameter TWLS             =     245; // tWLS       ps    Setup time for tDQS flop
      parameter TWLH             =     245; // tWLH       ps    Hold time of tDQS flop
      parameter TWLO             =    9000; // tWLO       ps    Write levelization output delay
      parameter TAA_MIN          =   15000; // TAA        ps    Internal READ command to first data
      parameter CL_TIME          =   15000; // CL         ps    Minimum CAS Latency
  `else `ifdef sg25E                        // sg25E is equivelant to the JEDEC DDR3-800D (5-5-5) speed bin
      parameter TCK_MIN          =    2500; // tCK        ps    Minimum Clock Cycle Time
      parameter TJIT_PER         =     100; // tJIT(per)  ps    Period JItter
      parameter TJIT_CC          =     200; // tJIT(cc)   ps    Cycle to Cycle jitter
      parameter TERR_2PER        =     147; // tERR(2per) ps    Accumulated Error (2-cycle)
      parameter TERR_3PER        =     175; // tERR(3per) ps    Accumulated Error (3-cycle)
      parameter TERR_4PER        =     194; // tERR(4per) ps    Accumulated Error (4-cycle)
      parameter TERR_5PER        =     209; // tERR(5per) ps    Accumulated Error (5-cycle)
      parameter TERR_6PER        =     222; // tERR(6per) ps    Accumulated Error (6-cycle)
      parameter TERR_7PER        =     232; // tERR(7per) ps    Accumulated Error (7-cycle)
      parameter TERR_8PER        =     241; // tERR(8per) ps    Accumulated Error (8-cycle)
      parameter TERR_9PER        =     249; // tERR(9per) ps    Accumulated Error (9-cycle)
      parameter TERR_10PER       =     257; // tERR(10per)ps    Accumulated Error (10-cycle)
      parameter TERR_11PER       =     263; // tERR(11per)ps    Accumulated Error (11-cycle)
      parameter TERR_12PER       =     269; // tERR(12per)ps    Accumulated Error (12-cycle)
      parameter TDS              =      75; // tDS        ps    DQ and DM input setup time relative to DQS
      parameter TDH              =     150; // tDH        ps    DQ and DM input hold time relative to DQS
      parameter TDQSQ            =     200; // tDQSQ      ps    DQS-DQ skew, DQS to last DQ valid, per group, per access
      parameter TDQSS            =    0.25; // tDQSS      tCK   Rising clock edge to DQS/DQS# latching transition
      parameter TDSS             =    0.20; // tDSS       tCK   DQS falling edge to CLK rising (setup time)
      parameter TDSH             =    0.20; // tDSH       tCK   DQS falling edge from CLK rising (hold time)
      parameter TDQSCK           =     400; // tDQSCK     ps    DQS output access time from CK/CK#
      parameter TQSH             =    0.38; // tQSH       tCK   DQS Output High Pulse Width
      parameter TQSL             =    0.38; // tQSL       tCK   DQS Output Low Pulse Width
      parameter TDIPW            =     600; // tDIPW      ps    DQ and DM input Pulse Width
      parameter TIPW             =     900; // tIPW       ps    Control and Address input Pulse Width
      parameter TIS              =     200; // tIS        ps    Input Setup Time
      parameter TIH              =     275; // tIH        ps    Input Hold Time
      parameter TRAS_MIN         =   37500; // tRAS       ps    Minimum Active to Precharge command time
      parameter TRC              =   50000; // tRC        ps    Active to Active/Auto Refresh command time
      parameter TRCD             =   12500; // tRCD       ps    Active to Read/Write command time
      parameter TRP              =   12500; // tRP        ps    Precharge command period
      parameter TXP              =    7500; // tXP        ps    Exit power down to a valid command
      parameter TCKE             =    7500; // tCKE       ps    CKE minimum high or low pulse width
      parameter TAON             =     400; // tAON       ps    RTT turn-on from ODTLon reference
      parameter TWLS             =     325; // tWLS       ps    Setup time for tDQS flop
      parameter TWLH             =     325; // tWLH       ps    Hold time of tDQS flop
      parameter TWLO             =    9000; // tWLO       ps    Write levelization output delay
      parameter TAA_MIN          =   12500; // TAA        ps    Internal READ command to first data
      parameter CL_TIME          =   12500; // CL         ps    Minimum CAS Latency
  `else `define sg25                        // sg25 is equivelant to the JEDEC DDR3-800E (6-6-6) speed bin
      parameter TCK_MIN          =    2500; // tCK        ps    Minimum Clock Cycle Time
      parameter TJIT_PER         =     100; // tJIT(per)  ps    Period JItter
      parameter TJIT_CC          =     200; // tJIT(cc)   ps    Cycle to Cycle jitter
      parameter TERR_2PER        =     147; // tERR(2per) ps    Accumulated Error (2-cycle)
      parameter TERR_3PER        =     175; // tERR(3per) ps    Accumulated Error (3-cycle)
      parameter TERR_4PER        =     194; // tERR(4per) ps    Accumulated Error (4-cycle)
      parameter TERR_5PER        =     209; // tERR(5per) ps    Accumulated Error (5-cycle)
      parameter TERR_6PER        =     222; // tERR(6per) ps    Accumulated Error (6-cycle)
      parameter TERR_7PER        =     232; // tERR(7per) ps    Accumulated Error (7-cycle)
      parameter TERR_8PER        =     241; // tERR(8per) ps    Accumulated Error (8-cycle)
      parameter TERR_9PER        =     249; // tERR(9per) ps    Accumulated Error (9-cycle)
      parameter TERR_10PER       =     257; // tERR(10per)ps    Accumulated Error (10-cycle)
      parameter TERR_11PER       =     263; // tERR(11per)ps    Accumulated Error (11-cycle)
      parameter TERR_12PER       =     269; // tERR(12per)ps    Accumulated Error (12-cycle)
      parameter TDS              =      75; // tDS        ps    DQ and DM input setup time relative to DQS
      parameter TDH              =     150; // tDH        ps    DQ and DM input hold time relative to DQS
      parameter TDQSQ            =     200; // tDQSQ      ps    DQS-DQ skew, DQS to last DQ valid, per group, per access
      parameter TDQSS            =    0.25; // tDQSS      tCK   Rising clock edge to DQS/DQS# latching transition
      parameter TDSS             =    0.20; // tDSS       tCK   DQS falling edge to CLK rising (setup time)
      parameter TDSH             =    0.20; // tDSH       tCK   DQS falling edge from CLK rising (hold time)
      parameter TDQSCK           =     400; // tDQSCK     ps    DQS output access time from CK/CK#
      parameter TQSH             =    0.38; // tQSH       tCK   DQS Output High Pulse Width
      parameter TQSL             =    0.38; // tQSL       tCK   DQS Output Low Pulse Width
      parameter TDIPW            =     600; // tDIPW      ps    DQ and DM input Pulse Width
      parameter TIPW             =     900; // tIPW       ps    Control and Address input Pulse Width
      parameter TIS              =     200; // tIS        ps    Input Setup Time
      parameter TIH              =     275; // tIH        ps    Input Hold Time
      parameter TRAS_MIN         =   37500; // tRAS       ps    Minimum Active to Precharge command time
      parameter TRC              =   52500; // tRC        ps    Active to Active/Auto Refresh command time
      parameter TRCD             =   15000; // tRCD       ps    Active to Read/Write command time
      parameter TRP              =   15000; // tRP        ps    Precharge command period
      parameter TXP              =    7500; // tXP        ps    Exit power down to a valid command
      parameter TCKE             =    7500; // tCKE       ps    CKE minimum high or low pulse width
      parameter TAON             =     400; // tAON       ps    RTT turn-on from ODTLon reference
      parameter TWLS             =     325; // tWLS       ps    Setup time for tDQS flop
      parameter TWLH             =     325; // tWLH       ps    Hold time of tDQS flop
      parameter TWLO             =    9000; // tWLO       ps    Write levelization output delay
      parameter TAA_MIN          =   15000; // TAA        ps    Internal READ command to first data
      parameter CL_TIME          =   15000; // CL         ps    Minimum CAS Latency
  `endif `endif `endif `endif `endif

  `ifdef x16
    `ifdef sg15E
      parameter TRRD             =    7500; // tRRD       ps     (2KB page size) Active bank a to Active bank b command time
      parameter TFAW             =   45000; // tFAW       ps     (2KB page size) Four Bank Activate window
    `else `ifdef sg15
      parameter TRRD             =    7500; // tRRD       ps     (2KB page size) Active bank a to Active bank b command time
      parameter TFAW             =   45000; // tFAW       ps     (2KB page size) Four Bank Activate window
    `else // sg187E, sg187, sg25, sg25E
      parameter TRRD             =   10000; // tRRD       ps     (2KB page size) Active bank a to Active bank b command time
      parameter TFAW             =   50000; // tFAW       ps     (2KB page size) Four Bank Activate window
    `endif `endif
  `else // x4, x8
    `ifdef sg15E
      parameter TRRD             =    6000; // tRRD       ps     (1KB page size) Active bank a to Active bank b command time
      parameter TFAW             =   30000; // tFAW       ps     (1KB page size) Four Bank Activate window
    `else `ifdef sg15
      parameter TRRD             =    6000; // tRRD       ps     (1KB page size) Active bank a to Active bank b command time
      parameter TFAW             =   30000; // tFAW       ps     (1KB page size) Four Bank Activate window
    `else `ifdef sg187E
      parameter TRRD             =    7500; // tRRD       ps     (1KB page size) Active bank a to Active bank b command time
      parameter TFAW             =   37500; // tFAW       ps     (1KB page size) Four Bank Activate window
    `else `ifdef sg187
      parameter TRRD             =    7500; // tRRD       ps     (1KB page size) Active bank a to Active bank b command time
      parameter TFAW             =   37500; // tFAW       ps     (1KB page size) Four Bank Activate window
    `else // sg25, sg25E
      parameter TRRD             =   10000; // tRRD       ps     (1KB page size) Active bank a to Active bank b command time
      parameter TFAW             =   40000; // tFAW       ps     (1KB page size) Four Bank Activate window
    `endif `endif `endif `endif
  `endif

      // Timing Parameters

      // Mode Register
      parameter CL_MIN           =       5; // CL         tCK   Minimum CAS Latency
      parameter CL_MAX           =      11; // CL         tCK   Maximum CAS Latency
      parameter AL_MIN           =       0; // AL         tCK   Minimum Additive Latency
      parameter AL_MAX           =       2; // AL         tCK   Maximum Additive Latency
      parameter WR_MIN           =       5; // WR         tCK   Minimum Write Recovery
      parameter WR_MAX           =      12; // WR         tCK   Maximum Write Recovery
      parameter BL_MIN           =       4; // BL         tCK   Minimum Burst Length
      parameter BL_MAX           =       8; // BL         tCK   Minimum Burst Length
      parameter CWL_MIN          =       5; // CWL        tCK   Minimum CAS Write Latency
      parameter CWL_MAX          =       8; // CWL        tCK   Maximum CAS Write Latency

      // Clock
      parameter TCK_MAX          =    3300; // tCK        ps    Maximum Clock Cycle Time
      parameter TCH_AVG_MIN      =    0.47; // tCH        tCK   Minimum Clock High-Level Pulse Width
      parameter TCL_AVG_MIN      =    0.47; // tCL        tCK   Minimum Clock Low-Level Pulse Width
      parameter TCH_AVG_MAX      =    0.53; // tCH        tCK   Maximum Clock High-Level Pulse Width
      parameter TCL_AVG_MAX      =    0.53; // tCL        tCK   Maximum Clock Low-Level Pulse Width
      parameter TCH_ABS_MIN      =    0.43; // tCH        tCK   Minimum Clock High-Level Pulse Width
      parameter TCL_ABS_MIN      =    0.43; // tCL        tCK   Maximum Clock Low-Level Pulse Width
      parameter TCKE_TCK         =       3; // tCKE       tCK   CKE minimum high or low pulse width
      parameter TAA_MAX          =   20000; // TAA        ps    Internal READ command to first data

      // Data OUT
      parameter TQH              =    0.38; // tQH        ps    DQ output hold time from DQS, DQS#
      // Data Strobe OUT
      parameter TRPRE            =    0.90; // tRPRE      tCK   DQS Read Preamble
      parameter TRPST            =    0.30; // tRPST      tCK   DQS Read Postamble
      // Data Strobe IN
      parameter TDQSH            =    0.45; // tDQSH      tCK   DQS input High Pulse Width
      parameter TDQSL            =    0.45; // tDQSL      tCK   DQS input Low Pulse Width
      parameter TWPRE            =    0.90; // tWPRE      tCK   DQS Write Preamble
      parameter TWPST            =    0.30; // tWPST      tCK   DQS Write Postamble
      // Command and Address
      parameter TZQCS            =      64; // tZQCS      tCK   ZQ Cal (Short) time
      parameter TZQINIT          =     512; // tZQinit    tCK   ZQ Cal (Long) time
      parameter TZQOPER          =     256; // tZQoper    tCK   ZQ Cal (Long) time
      parameter TCCD             =       4; // tCCD       tCK   Cas to Cas command delay
      parameter TCCD_DG          =       2; // tCCD_DG    tCK   Cas to Cas command delay to different group
      parameter TRAS_MAX         =70312500; // tRAS       ps    Maximum Active to Precharge command time
      parameter TWR              =   15000; // tWR        ps    Write recovery time
      parameter TMRD             =       4; // tMRD       tCK   Load Mode Register command cycle time
      parameter TMOD             =   15000; // tMOD       ps    LOAD MODE to non-LOAD MODE command cycle time
      parameter TMOD_TCK         =      12; // tMOD       tCK   LOAD MODE to non-LOAD MODE command cycle time
      parameter TRRD_TCK         =       4; // tRRD       tCK   Active bank a to Active bank b command time
      parameter TRRD_DG          =    3000; // tRRD_DG    ps     Active bank a to Active bank b command time to different group
      parameter TRRD_DG_TCK      =       2; // tRRD_DG    tCK   Active bank a to Active bank b command time to different group
      parameter TRTP             =    7500; // tRTP       ps    Read to Precharge command delay
      parameter TRTP_TCK         =       4; // tRTP       tCK   Read to Precharge command delay
      parameter TWTR             =    7500; // tWTR       ps    Write to Read command delay
      parameter TWTR_DG          =    3750; // tWTR_DG    ps    Write to Read command delay to different group
      parameter TWTR_TCK         =       4; // tWTR       tCK   Write to Read command delay
      parameter TWTR_DG_TCK      =       2; // tWTR_DG    tCK   Write to Read command delay to different group
      parameter TDLLK            =     512; // tDLLK      tCK   DLL locking time
      // Refresh - 2Gb
      parameter TRFC_MIN         =  160000; // tRFC       ps    Refresh to Refresh Command interval minimum value
      parameter TRFC_MAX         =70312500; // tRFC       ps    Refresh to Refresh Command Interval maximum value
      // Power Down
      parameter TXP_TCK          =       3; // tXP        tCK   Exit power down to a valid command
      parameter TXPDLL           =   24000; // tXPDLL     ps    Exit precharge power down to READ or WRITE command (DLL-off mode)
      parameter TXPDLL_TCK       =      10; // tXPDLL     tCK   Exit precharge power down to READ or WRITE command (DLL-off mode)
      parameter TACTPDEN         =       1; // tACTPDEN   tCK   Timing of last ACT command to power down entry
      parameter TPRPDEN          =       1; // tPREPDEN   tCK   Timing of last PRE command to power down entry
      parameter TREFPDEN         =       1; // tARPDEN    tCK   Timing of last REFRESH command to power down entry
      parameter TCPDED           =       1; // tCPDED     tCK   Command pass disable/enable delay
      parameter TPD_MAX          =TRFC_MAX; // tPD        ps    Power-down entry-to-exit timing
      parameter TXPR             =  170000; // tXPR       ps    Exit Reset from CKE assertion to a valid command
      parameter TXPR_TCK         =       5; // tXPR       tCK   Exit Reset from CKE assertion to a valid command
      // Self Refresh
      parameter TXS              =  170000; // tXS        ps    Exit self refesh to a non-read or write command
      parameter TXS_TCK          =       5; // tXS        tCK   Exit self refesh to a non-read or write command
      parameter TXSDLL           =   TDLLK; // tXSRD      tCK   Exit self refresh to a read or write command
      parameter TISXR            =     TIS; // tISXR      ps    CKE setup time during self refresh exit.
      parameter TCKSRE           =   10000; // tCKSRE     ps    Valid Clock requirement after self refresh entry (SRE)
      parameter TCKSRE_TCK       =       5; // tCKSRE     tCK   Valid Clock requirement after self refresh entry (SRE)
      parameter TCKSRX           =   10000; // tCKSRX     ps    Valid Clock requirement prior to self refresh exit (SRX)
      parameter TCKSRX_TCK       =       5; // tCKSRX     tCK   Valid Clock requirement prior to self refresh exit (SRX)
      parameter TCKESR_TCK       =       4; // tCKESR     tCK   Minimum CKE low width for Self Refresh entry to exit timing
      // ODT
      parameter TAOF             =     0.7; // tAOF       tCK   RTT turn-off from ODTLoff reference
      parameter TAONPD           =    9000; // tAONPD     ps    Asynchronous RTT turn-on delay (Power-Down with DLL frozen)
      parameter TAOFPD           =    9000; // tAONPD     ps    Asynchronous RTT turn-off delay (Power-Down with DLL frozen)
      parameter ODTH4            =       4; // ODTH4      tCK   ODT minimum HIGH time after ODT assertion or write (BL4)
      parameter ODTH8            =       6; // ODTH8      tCK   ODT minimum HIGH time after write (BL8)
      parameter TADC             =     0.7; // tADC       tCK   RTT dynamic change skew
      // Write Levelization
      parameter TWLMRD           =      40; // tWLMRD     tCK   First DQS pulse rising edge after tDQSS margining mode is programmed
      parameter TWLDQSEN         =      25; // tWLDQSEN   tCK   DQS/DQS delay after tDQSS margining mode is programmed
      parameter TWLOE            =    2000; // tWLOE      ps    Write levelization output error

      // Size Parameters based on Part Width

  `ifdef x4
      parameter DM_BITS          =       1; // Set this parameter to control how many Data Mask bits are used
      parameter ADDR_BITS        =      15; // MAX Address Bits
      parameter ROW_BITS         =      15; // Set this parameter to control how many Address bits are used
      parameter COL_BITS         =      11; // Set this parameter to control how many Column bits are used
      parameter DQ_BITS          =       4; // Set this parameter to control how many Data bits are used       **Same as part bit width**
      parameter DQS_BITS         =       1; // Set this parameter to control how many Dqs bits are used
  `else `ifdef x8
      parameter DM_BITS          =       1; // Set this parameter to control how many Data Mask bits are used
      parameter ADDR_BITS        =      15; // MAX Address Bits
      parameter ROW_BITS         =      15; // Set this parameter to control how many Address bits are used
      parameter COL_BITS         =      10; // Set this parameter to control how many Column bits are used
      parameter DQ_BITS          =       8; // Set this parameter to control how many Data bits are used       **Same as part bit width**
      parameter DQS_BITS         =       1; // Set this parameter to control how many Dqs bits are used
  `else `define x16
      parameter DM_BITS          =       2; // Set this parameter to control how many Data Mask bits are used
      parameter ADDR_BITS        =      14; // MAX Address Bits
      parameter ROW_BITS         =      14; // Set this parameter to control how many Address bits are used
      parameter COL_BITS         =      10; // Set this parameter to control how many Column bits are used
      parameter DQ_BITS          =      16; // Set this parameter to control how many Data bits are used       **Same as part bit width**
      parameter DQS_BITS         =       2; // Set this parameter to control how many Dqs bits are used
  `endif `endif

      // Size Parameters
      parameter BA_BITS          =       3; // Set this parmaeter to control how many Bank Address bits are used
      parameter MEM_BITS         =      15; // Set this parameter to control how many write data bursts can be stored in memory.  The default is 2^10=1024.
      parameter AP               =      10; // the address bit that controls auto-precharge and precharge-all
      parameter BC               =      12; // the address bit that controls burst chop
      parameter BL_BITS          =       3; // the number of bits required to count to BL_MAX
      parameter BO_BITS          =       2; // the number of Burst Order Bits

  `ifdef QUAD_RANK
      `define DUAL_RANK // also define DUAL_RANK
      parameter CS_BITS          =       4; // Number of Chip Select Bits
      parameter RANKS            =       4; // Number of Chip Selects
  `else `ifdef DUAL_RANK
      parameter CS_BITS          =       2; // Number of Chip Select Bits
      parameter RANKS            =       2; // Number of Chip Selects
  `else
      parameter CS_BITS          =       2; // Number of Chip Select Bits
      parameter RANKS            =       1; // Number of Chip Selects
  `endif `endif

      // Simulation parameters
      parameter RZQ              =     240; // termination resistance
      parameter PRE_DEF_PAT      =   8'hAA; // value returned during mpr pre-defined pattern readout
      parameter STOP_ON_ERROR    =       1; // If set to 1, the model will halt on command sequence/major errors
      parameter DEBUG            =       1; // Turn on Debug messages
      parameter BUS_DELAY        =       0; // delay in nanoseconds
      parameter RANDOM_OUT_DELAY =       0; // If set to 1, the model will put a random amount of delay on DQ/DQS during reads
      parameter RANDOM_SEED    = 711689044; //seed value for random generator.

      parameter RDQSEN_PRE       =       2; // DQS driving time prior to first read strobe
      parameter RDQSEN_PST       =       1; // DQS driving time after last read strobe
      parameter RDQS_PRE         =       2; // DQS low time prior to first read strobe
      parameter RDQS_PST         =       1; // DQS low time after last read strobe
      parameter RDQEN_PRE        =       0; // DQ/DM driving time prior to first read data
      parameter RDQEN_PST        =       0; // DQ/DM driving time after last read data
      parameter WDQS_PRE         =       2; // DQS half clock periods prior to first write strobe
      parameter WDQS_PST         =       1; // DQS half clock periods after last write strobe

  // check for legal cas latency based on the cas write latency
  function valid_cl;
      input [3:0] cl;
      input [3:0] cwl;

      case ({cwl, cl})
  `ifdef sg15E
          {4'd5, 4'd6 },
          {4'd6, 4'd8 },
          {4'd7, 4'd9 },
          {4'd7, 4'd10}: valid_cl = 1;
  `else `ifdef sg15
          {4'd5, 4'd6 },
          {4'd6, 4'd8 },
          {4'd7, 4'd10}: valid_cl = 1;
  `else `ifdef sg187E
          {4'd5, 4'd6 },
          {4'd6, 4'd7 },
          {4'd6, 4'd8 }: valid_cl = 1;
  `else `ifdef sg187
          {4'd5, 4'd6 },
          {4'd6, 4'd8 }: valid_cl = 1;
  `else `ifdef sg25E
          {4'd5, 4'd5 },
          {4'd5, 4'd6 }: valid_cl = 1;
  `else `ifdef sg25
          {4'd5, 4'd6 }: valid_cl = 1;
  `endif `endif `endif `endif `endif `endif
          default : valid_cl = 0;
      endcase
  endfunction

  // find the minimum valid cas write latency
  function [3:0] min_cwl;
      input period;
      real period;
      min_cwl = (period >= 2500.0) ? 5:
                (period >= 1875.0) ? 6:
                (period >= 1500.0) ? 7:
                8; //(period >= 1250.0)
  endfunction

  // find the minimum valid cas latency
  function [3:0] min_cl;
      input period;
      real period;
      reg [3:0] cwl;
      reg [3:0] cl;
      begin
          cwl = min_cwl(period);
          for (cl=CL_MAX; cl>=CL_MIN; cl=cl-1) begin
              if (valid_cl(cl, cwl)) begin
                  min_cl = cl;
              end
          end
      end
  endfunction

`endif
