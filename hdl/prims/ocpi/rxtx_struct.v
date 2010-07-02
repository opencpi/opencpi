// rxtx.v

module rxtx( 
   input   wire            A2D_CLKn, 
   input   wire            A2D_CLKp, 
   input   wire            ADC_SYNC_INn, 
   input   wire            ADC_SYNC_INp, 
   input   wire            CH1_A2D_CLKn, 
   input   wire            CH1_A2D_CLKp, 
   input   wire    [6:0]   CH1_A2D_DATAn, 
   input   wire    [6:0]   CH1_A2D_DATAp, 
   input   wire            CH1_HV_OF, 
   input   wire            CH1_LV_OF, 
   input   wire            CH2_A2D_CLKn, 
   input   wire            CH2_A2D_CLKp, 
   input   wire    [6:0]   CH2_A2D_DATAn, 
   input   wire    [6:0]   CH2_A2D_DATAp, 
   input   wire            CH2_HV_OF, 
   input   wire            CH2_LV_OF, 
   input   wire            DAC_GDATACLKn, 
   input   wire            DAC_GDATACLKp, 
   input   wire            DAC_SYNC_INn, 
   input   wire            DAC_SYNC_INp, 
   input   wire            EXT_CLK_SDI, 
   input   wire            EXT_CLK_STATUS, 
   input   wire            LVCLK_200n, 
   input   wire            LVCLK_200p, 
   input   wire            MGT_REFCLK_n, 
   input   wire            MGT_REFCLK_p, 
   input   wire            PCIE_REFCLK_n, 
   input   wire            PCIE_REFCLK_p, 
   input   wire            PCIE_RESETn, 
   input   wire            TEMP_ALERTn, 
   input   wire            TEMP_THERMn, 
   output  wire            ADC_SYNC_DISABLEn, 
   output  wire            ADC_SYNC_DISABLEp, 
   output  wire            ADC_SYNC_OUTn, 
   output  wire            ADC_SYNC_OUTp, 
   output  wire            CH1_A2D_OE, 
   output  wire            CH1_A2D_RESET, 
   output  wire            CH1_A2D_SCLK, 
   output  wire            CH1_A2D_SDATA, 
   output  wire            CH1_A2D_SEN, 
   output  wire            CH2_A2D_OE, 
   output  wire            CH2_A2D_RESET, 
   output  wire            CH2_A2D_SCLK, 
   output  wire            CH2_A2D_SDATA, 
   output  wire            CH2_A2D_SEN, 
   output  wire            CPLD_CSn, 
   output  wire            CPLD_RSTn, 
   output  wire            DAC_CAL, 
   output  wire            DAC_CLKDIV, 
   output  wire    [11:0]  DAC_DAn, 
   output  wire    [11:0]  DAC_DAp, 
   output  wire    [11:0]  DAC_DBn, 
   output  wire    [11:0]  DAC_DBp, 
   output  wire    [11:0]  DAC_DCn, 
   output  wire    [11:0]  DAC_DCp, 
   output  wire    [11:0]  DAC_DDn, 
   output  wire    [11:0]  DAC_DDp, 
   output  wire            DAC_DELAY, 
   output  wire            DAC_RF, 
   output  wire            DAC_RZ, 
   output  wire            DAC_SYNC_DISABLEn, 
   output  wire            DAC_SYNC_DISABLEp, 
   output  wire            DAC_SYNC_OUTn, 
   output  wire            DAC_SYNC_OUTp, 
   output  wire            EXT_CLK_CSn, 
   output  wire            EXT_CLK_FUNC, 
   output  wire            EXT_CLK_SCLK, 
   output  wire            EXT_CLK_SDO, 
   output  wire            I2C_BUS_SELECT, 
   inout   wire            I2C_SCLK, 
   inout   wire            I2C_SDATA 
);




supply0           GND;
wire              RX_FIFO_ALMOST_EMPTY;
wire              RX_FIFO_EMPTY;
wire              TX_FIFO_ALMOST_FULL;
wire              TX_FIFO_FULL;
wire              a2dCh1FifoAlmostEmpty;
wire     [15:0]   a2dCh1FifoDataOut;
wire              a2dCh1LcbAck;
wire     [31:0]   a2dCh1LcbData;
wire              a2dCh2FifoAlmostEmpty;
wire     [15:0]   a2dCh2FifoDataOut;
wire              a2dCh2LcbAck;
wire     [31:0]   a2dCh2LcbData;
wire     [1:0]    a2dChLcbRead;
wire     [1:0]    a2dChLcbSelect;
wire     [1:0]    a2dChLcbWrite;
wire              a2dClk;
wire              a2dDcmReset;
wire     [3:0]    a2dFifoRdEn;
wire     [3:0]    a2dFifoSyncWrEn;
wire              a2dFifoWrEnable;
wire              a2d_clk_locked;
wire              a2d_clk_reset;
wire     [31:0]   absTs1pps;
wire     [31:0]   absTsSample;
wire              adcISync;
wire              adcLsSyncCtrlLcbAck;
wire     [31:0]   adcLsSyncCtrlLcbData;
wire              adcSyncDisable;
wire              adcSyncIn;
wire              adcSyncOut;
wire     [127:0]  c2sFifoDin;
wire              c2sFifoWrite;
wire     [7 : 0]  cfg_bus_number;
wire     [3 : 0]  cfg_byte_en_n;
wire     [15 : 0] cfg_command;
wire     [15 : 0] cfg_dcommand;
wire     [4 : 0]  cfg_device_number;
wire     [31 : 0] cfg_di;
wire     [31 : 0] cfg_do;
wire     [63:0]   cfg_dsn_n;
wire     [15 : 0] cfg_dstatus;
wire     [9 : 0]  cfg_dwaddr;
wire              cfg_err_cor_n;
wire              cfg_err_cpl_abort_n;
wire              cfg_err_cpl_rdy_n;
wire              cfg_err_cpl_timeout_n;
wire              cfg_err_cpl_unexpect_n;
wire              cfg_err_ecrc_n;
wire              cfg_err_posted_n;
wire     [47 : 0] cfg_err_tlp_cpl_header;
wire              cfg_err_ur_n;
wire     [2 : 0]  cfg_function_number;
wire              cfg_interrupt_assert_n;
wire     [7 : 0]  cfg_interrupt_di;
wire     [7 : 0]  cfg_interrupt_do;
wire     [2 : 0]  cfg_interrupt_mmenable;
wire              cfg_interrupt_msienable;
wire              cfg_interrupt_n;
wire              cfg_interrupt_rdy_n;
wire     [15 : 0] cfg_lcommand;
wire     [15 : 0] cfg_lstatus;
wire     [2 : 0]  cfg_pcie_link_state_n;
wire              cfg_pm_wake_n;
wire              cfg_rd_en_n;
wire              cfg_rd_wr_done_n;
wire     [15 : 0] cfg_status;
wire              cfg_to_turnoff_n;
wire              cfg_trn_pending_n;
wire              cfg_wr_en_n;
wire              clk150Mhz;
wire              clk200Mhz;
wire              clk390KHz;
wire              dacClk;
wire              dacClkLowFreqSelect;
wire              dacDcmReset;
wire              dacISync;
wire              dacLcbAck;
wire     [31:0]   dacLcbData;
wire              dacLcbRead;
wire              dacLcbSelect;
wire              dacLcbWrite;
wire              dacLsSyncCtrlLcbAck;
wire     [31:0]   dacLsSyncCtrlLcbData;
wire              dacSyncDisable;
wire              dacSyncIn;
wire              dacSyncOut;
wire              dacTrigIn;
wire     [5:0]    dacTxFifoAFullThresh;
wire              dacTxFifoAlmostFull;
wire     [255:0]  dacTxFifoData;
wire              dacTxFifoFull;
wire              dacTxFifoWrite;
wire              dac_clk_div2;
wire              dac_clk_locked;
wire              dac_reset;
wire              fast_train_simulation_only;
wire              lcbAckOr;
wire              lcbClk;
wire              lcb_clk_locked;
wire              lcb_reset;
wire              lcb_resetn;
wire     [1:0]    lsSyncCtrlLcbRead;
wire     [1:0]    lsSyncCtrlLcbSelect;
wire     [1:0]    lsSyncCtrlLcbWrite;
wire              lsiAck;
wire     [31:0]   lsiAddress;
wire              lsiCs;
wire     [31:0]   lsiDataIn;
wire     [31:0]   lsiDataOut;
wire              lsiRead;
wire              lsiWrite;
wire              mgtRefClk;
wire     [127:0]  packetDataCh0;
wire     [127:0]  packetDataCh1;
wire     [127:0]  packetDataCh2;
wire     [127:0]  packetDataCh3;
wire     [3:0]    packetWriteEnable;
wire     [7:0]    pcieC2sBusGrant;
wire     [7:0]    pcieC2sBusRequest;
wire              pcieC2sFifoAlmostFull;
wire              pcieC2sFifoFull;
wire     [7:0]    pcieC2sWriteEnable;
wire              pcieLcbAck;
wire     [31:0]   pcieLcbData;
wire              pcieLcbRead;
wire              pcieLcbSelect;
wire              pcieLcbWrite;
wire              pcie_ref_clk;
wire     [3:0]    pgBusRequest;
wire              pgLcbAck;
wire     [31:0]   pgLcbData;
wire              pgLcbRead;
wire              pgLcbSelect;
wire              pgLcbWrite;
wire              pll_reset;
wire              rcvrCh1LcbAck;
wire     [31:0]   rcvrCh1LcbData;
wire              rcvrCh2LcbAck;
wire     [31:0]   rcvrCh2LcbData;
wire     [1:0]    rcvrChLcbRead;
wire     [1:0]    rcvrChLcbSelect;
wire     [1:0]    rcvrChLcbWrite;
wire              ref_clk_out;
wire     [31:0]   relTs;
wire              reset;
wire              resetn;
wire              smBusLcbAck;
wire     [31:0]   smBusLcbData;
wire              smBusLcbRead;
wire              smBusLcbSelect;
wire              smBusLcbWrite;
wire              spiLcbAck;
wire     [31:0]   spiLcbData;
wire              spiLcbRead;
wire              spiLcbSelect;
wire              spiLcbWrite;
wire     [15:0]   testCount;
wire     [15:0]   testSignal;
wire              testStimulusLcbAck;
wire     [31:0]   testStimulusLcbData;
wire              testStimulusLcbRead;
wire              testStimulusLcbSelect;
wire              testStimulusLcbWrite;
wire              timeStampLcbAck;
wire     [31:0]   timeStampLcbData;
wire              timeStampLcbRead;
wire              timeStampLcbSelect;
wire              timeStampLcbWrite;
wire              trn_lnk_up_n;
wire     [6 : 0]  trn_rbar_hit_n;
wire              trn_rcpl_streaming_n;
wire     [63 : 0] trn_rd;
wire              trn_rdst_rdy_n;
wire              trn_reof_n;
wire              trn_rerrfwd_n;
wire              trn_reset_n;
wire              trn_rnp_ok_n;
wire     [7 : 0]  trn_rrem_n;
wire              trn_rsof_n;
wire              trn_rsrc_dsc_n;
wire              trn_rsrc_rdy_n;
wire     [3 : 0]  trn_tbuf_av;
wire     [63 : 0] trn_td;
wire              trn_tdst_dsc_n;
wire              trn_tdst_rdy_n;
wire              trn_teof_n;
wire              trn_terrfwd_n;
wire     [7 : 0]  trn_trem_n;
wire              trn_tsof_n;
wire              trn_tsrc_dsc_n;
wire              trn_tsrc_rdy_n;
wire              user_clk;
wire              user_clk_div2;
wire              user_clk_locked;
wire              user_rst_div2_n;
wire              user_rst_n;
wire     [31:0]   waveGenData;
wire              waveGenEnable;
wire              waveGenLcbAck;
wire     [10:0]   waveGenLoopAddress;
wire              waveGenRead;
wire              waveGenSelect;
wire              waveGenTrigOut;
wire              waveGenTrigPulseOut;
wire              waveGenTrigger;
wire              waveGenWrite;
wire     [7:0]    wbCh0BusGrant;
wire     [7:0]    wbCh0BusRequest;
wire              wbCh0CollFifoAlmostEmpty;
wire     [127:0]  wbCh0CollFifoDataOut;
wire              wbCh0CollFifoEmpty;
wire              wbCh0CollFifoReadEnable;
wire     [7:0]    wbCh0DestFifoAlmostFull;
wire     [7:0]    wbCh0DestFifoFull;
wire     [7:0]    wbCh0DestWrite;
wire              wbCh0DropFrameRouteCode;
wire     [31:0]   wbCh0FrameSizeBytesRouter;
wire     [3:0]    wbCh0RouteCode;
wire     [127:0]  wbCh0RouteDataOut;
wire     [7:0]    wbCh1BusGrant;
wire     [7:0]    wbCh1BusRequest;
wire              wbCh1CollFifoAlmostEmpty;
wire     [127:0]  wbCh1CollFifoDataOut;
wire              wbCh1CollFifoEmpty;
wire              wbCh1CollFifoReadEnable;
wire     [7:0]    wbCh1DestFifoAlmostFull;
wire     [7:0]    wbCh1DestFifoFull;
wire     [7:0]    wbCh1DestWrite;
wire              wbCh1DropFrameRouteCode;
wire     [31:0]   wbCh1FrameSizeBytesRouter;
wire     [3:0]    wbCh1RouteCode;
wire     [127:0]  wbCh1RouteDataOut;


// Instances 
epg128BitFrameRouter ch1_rcvr_fr( 
   .clk                (clk200Mhz), 
   .inFifoAlmostEmpty  (wbCh0CollFifoAlmostEmpty), 
   .inFifoEmpty        (wbCh0CollFifoEmpty), 
   .frameDataIn        (wbCh0CollFifoDataOut), 
   .frameSizeBytes     (wbCh0FrameSizeBytesRouter), 
   .resetn             (resetn), 
   .destBusGrant       (wbCh0BusGrant), 
   .destFifoAlmostFull (wbCh0DestFifoAlmostFull), 
   .destFifoFull       (wbCh0DestFifoFull), 
   .routeCode          (wbCh0RouteCode[2:0]), 
   .dropFrameRouteCode (wbCh0DropFrameRouteCode), 
   .frameDataRead      (wbCh0CollFifoReadEnable), 
   .routeDataOut       (wbCh0RouteDataOut), 
   .destWrite          (wbCh0DestWrite), 
   .destBusReq         (wbCh0BusRequest), 
   .frameInt           ()
); 

epg128BitFrameRouter ch2_rcvr_fr( 
   .clk                (clk200Mhz), 
   .inFifoAlmostEmpty  (wbCh1CollFifoAlmostEmpty), 
   .inFifoEmpty        (wbCh1CollFifoEmpty), 
   .frameDataIn        (wbCh1CollFifoDataOut), 
   .frameSizeBytes     (wbCh1FrameSizeBytesRouter), 
   .resetn             (resetn), 
   .destBusGrant       (wbCh1BusGrant), 
   .destFifoAlmostFull (wbCh1DestFifoAlmostFull), 
   .destFifoFull       (wbCh1DestFifoFull), 
   .routeCode          (wbCh1RouteCode[2:0]), 
   .dropFrameRouteCode (wbCh1DropFrameRouteCode), 
   .frameDataRead      (wbCh1CollFifoReadEnable), 
   .routeDataOut       (wbCh1RouteDataOut), 
   .destWrite          (wbCh1DestWrite), 
   .destBusReq         (wbCh1BusRequest), 
   .frameInt           ()
); 

epg_a2d_ads6149_interface a2dInfCh1( 
   .a2dClkn            (CH1_A2D_CLKn), 
   .a2dClkp            (CH1_A2D_CLKp), 
   .a2dDatan           (CH1_A2D_DATAn), 
   .a2dDatap           (CH1_A2D_DATAp), 
   .a2dFifoSyncWrEn    (a2dFifoSyncWrEn[0:0]), 
   .a2dOvrRngSpiDataIn (CH1_LV_OF), 
   .a2dRdClk           (a2dClk), 
   .a2dRdEn            (a2dFifoRdEn[0:0]), 
   .idelayRefClk       (clk200Mhz), 
   .lcbAddress         (lsiAddress[8:0]), 
   .lcbClk             (lcbClk), 
   .lcbCoreSelect      (a2dChLcbSelect[0:0]), 
   .lcbDataIn          (lsiDataOut), 
   .lcbRead            (a2dChLcbRead[0:0]), 
   .lcbWrite           (a2dChLcbWrite[0:0]), 
   .resetn             (resetn), 
   .serdesClk          (lcbClk), 
   .a2dFifoAlmostEmpty (a2dCh1FifoAlmostEmpty), 
   .a2dFifoAlmostFull  (), 
   .a2dFifoDataOut     (a2dCh1FifoDataOut), 
   .a2dFifoEmpty       (), 
   .a2dFifoFull        (), 
   .a2dOE              (CH1_A2D_OE), 
   .a2dReset           (CH1_A2D_RESET), 
   .lcbAck             (a2dCh1LcbAck), 
   .lcbDataOut         (a2dCh1LcbData), 
   .spiChipSelectn     (CH1_A2D_SEN), 
   .spiClk             (CH1_A2D_SCLK), 
   .spiDataOut         (CH1_A2D_SDATA), 
   .spiWriteEnable     ()
); 

epg_a2d_ads6149_interface a2dInfCh2( 
   .a2dClkn            (CH2_A2D_CLKn), 
   .a2dClkp            (CH2_A2D_CLKp), 
   .a2dDatan           (CH2_A2D_DATAn), 
   .a2dDatap           (CH2_A2D_DATAp), 
   .a2dFifoSyncWrEn    (a2dFifoSyncWrEn[1:1]), 
   .a2dOvrRngSpiDataIn (CH2_LV_OF), 
   .a2dRdClk           (a2dClk), 
   .a2dRdEn            (a2dFifoRdEn[1:1]), 
   .idelayRefClk       (clk200Mhz), 
   .lcbAddress         (lsiAddress[8:0]), 
   .lcbClk             (lcbClk), 
   .lcbCoreSelect      (a2dChLcbSelect[1:1]), 
   .lcbDataIn          (lsiDataOut), 
   .lcbRead            (a2dChLcbRead[1:1]), 
   .lcbWrite           (a2dChLcbWrite[1:1]), 
   .resetn             (resetn), 
   .serdesClk          (lcbClk), 
   .a2dFifoAlmostEmpty (a2dCh2FifoAlmostEmpty), 
   .a2dFifoAlmostFull  (), 
   .a2dFifoDataOut     (a2dCh2FifoDataOut), 
   .a2dFifoEmpty       (), 
   .a2dFifoFull        (), 
   .a2dOE              (CH2_A2D_OE), 
   .a2dReset           (CH2_A2D_RESET), 
   .lcbAck             (a2dCh2LcbAck), 
   .lcbDataOut         (a2dCh2LcbData), 
   .spiChipSelectn     (CH2_A2D_SEN), 
   .spiClk             (CH2_A2D_SCLK), 
   .spiDataOut         (CH2_A2D_SDATA), 
   .spiWriteEnable     ()
); 

epgA2DFifoCtrlInterface a2dFifoCtrl( 
   .a2dFifoAE0      (a2dCh1FifoAlmostEmpty), 
   .a2dFifoAE1      (a2dCh2FifoAlmostEmpty), 
   .a2dFifoAE2      (GND), 
   .a2dFifoAE3      (GND), 
   .a2dFifoWrEnable (a2dFifoWrEnable), 
   .collClk         (a2dClk), 
   .resetn          (resetn), 
   .a2dFifoRdEn     (a2dFifoRdEn), 
   .a2dFifoSyncWrEn (a2dFifoSyncWrEn)
); 

epg_dac_interface_max19692_top dac1( 
   .lcbAddress        (lsiAddress[6:0]), 
   .lcbClk            (lcbClk), 
   .lcbCoreSelect     (dacLcbSelect), 
   .lcbDataIn         (lsiDataOut), 
   .lcbRead           (dacLcbRead), 
   .lcbWrite          (dacLcbWrite), 
   .resetn            (lcb_resetn), 
   .sampleClk         (dacClk), 
   .trigIn            (dacTrigIn), 
   .txClk             (clk150Mhz), 
   .txData            (dacTxFifoData), 
   .txFifoAFullThresh (dacTxFifoAFullThresh), 
   .txFifoWrite       (dacTxFifoWrite), 
   .Cal               (DAC_CAL), 
   .ClkDiv            (DAC_CLKDIV), 
   .DAn               (DAC_DAn), 
   .DAp               (DAC_DAp), 
   .DBn               (DAC_DBn), 
   .DBp               (DAC_DBp), 
   .DCn               (DAC_DCn), 
   .DCp               (DAC_DCp), 
   .DDn               (DAC_DDn), 
   .DDp               (DAC_DDp), 
   .Delay             (DAC_DELAY), 
   .Rf                (DAC_RF), 
   .Rz                (DAC_RZ), 
   .lcbAck            (dacLcbAck), 
   .lcbDataOut        (dacLcbData), 
   .txFifoAlmostFull  (dacTxFifoAlmostFull), 
   .txFifoFull        (dacTxFifoFull)
); 

arbMux8Ch pcie_c2s_mux( 
   .clk            (clk200Mhz), 
   .resetn         (resetn), 
   .request        (pcieC2sBusRequest), 
   .ch0Data        (wbCh0RouteDataOut), 
   .ch1Data        (wbCh1RouteDataOut), 
   .ch2Data        (0), //mod
   .ch3Data        (0), //mod 
   .ch4Data        (packetDataCh0), 
   .ch5Data        (packetDataCh1), 
   .ch6Data        (packetDataCh2), 
   .ch7Data        (packetDataCh3), 
   .writeEnableIn  (pcieC2sWriteEnable), 
   .grant          (pcieC2sBusGrant), 
   .muxData        (c2sFifoDin), 
   .writeEnableOut (c2sFifoWrite)
); 

pulseCapture dac_trig1( 
   .clk      (dacClk), 
   .resetn   (resetn), 
   .pulseIn  (waveGenTrigOut), 
   .pulseOut (waveGenTrigPulseOut)
); 

pulseCapture wg_trig1( 
   .clk      (clk150Mhz), 
   .resetn   (resetn), 
   .pulseIn  (dacISync), 
   .pulseOut (waveGenTrigger)
); 

ls_sync_controller adc_sync( 
   .lcbAddress     (lsiAddress[7:0]), 
   .lcbClk         (lcbClk), 
   .lcbCoreSelect  (lsSyncCtrlLcbSelect[0:0]), 
   .lcbDataIn      (lsiDataOut), 
   .lcbRead        (lsSyncCtrlLcbRead[0:0]), 
   .lcbWrite       (lsSyncCtrlLcbWrite[0:0]), 
   .resetn         (lcb_resetn), 
   .syncClk        (a2dClk), 
   .syncIn         (adcSyncIn), 
   .extSyncDisable (adcSyncDisable), 
   .iSync          (adcISync), 
   .lcbAck         (adcLsSyncCtrlLcbAck), 
   .lcbDataOut     (adcLsSyncCtrlLcbData), 
   .syncOut        (adcSyncOut)
); 

ls_sync_controller dac_sync( 
   .lcbAddress     (lsiAddress[7:0]), 
   .lcbClk         (lcbClk), 
   .lcbCoreSelect  (lsSyncCtrlLcbSelect[1:1]), 
   .lcbDataIn      (lsiDataOut), 
   .lcbRead        (lsSyncCtrlLcbRead[1:1]), 
   .lcbWrite       (lsSyncCtrlLcbWrite[1:1]), 
   .resetn         (lcb_resetn), 
   .syncClk        (dacClk), 
   .syncIn         (dacSyncIn), 
   .extSyncDisable (dacSyncDisable), 
   .iSync          (dacISync), 
   .lcbAck         (dacLsSyncCtrlLcbAck), 
   .lcbDataOut     (dacLsSyncCtrlLcbData), 
   .syncOut        (dacSyncOut)
); 

packetGenerator4xTop packetGen1( 
   .busGrant          (pcieC2sBusGrant[7:4]), 
   .fifoAlmostFull    (pcieC2sFifoAlmostFull), 
   .fifoFull          (pcieC2sFifoFull), 
   .lcbAddress        (lsiAddress[7:0]), 
   .lcbClk            (lcbClk), 
   .lcbCoreSelect     (pgLcbSelect), 
   .lcbDataIn         (lsiDataOut), 
   .lcbRead           (pgLcbRead), 
   .lcbWrite          (pgLcbWrite), 
   .packetClk         (clk200Mhz), 
   .resetn            (lcb_resetn), 
   .trigIn            (adcISync), 
   .busRequest        (pgBusRequest), 
   .lcbAck            (pgLcbAck), 
   .lcbDataOut        (pgLcbData), 
   .packetDataCh1     (packetDataCh1), 
   .packetDataCh2     (packetDataCh2), 
   .packetDataCh3     (packetDataCh3), 
   .packetWriteEnable (packetWriteEnable), 
   .packetDataCh0     (packetDataCh0)
); 

rcvr_channel ch1_rcvr( 
   .a2dCh1               (a2dCh1FifoDataOut), 
   .a2dCh2               (a2dCh2FifoDataOut), 
   .a2dCh3               (0), //mod
   .a2dCh4               (0), //mod
   .a2dClk               (a2dClk), 
   .abs1ppsTimeStamp     (absTs1pps), 
   .absSampleTimeStamp   (absTsSample), 
   .collFifoReadClk      (clk200Mhz), 
   .collFifoReadEnable   (wbCh0CollFifoReadEnable), 
   .lcbAddress           (lsiAddress[12:0]), 
   .lcbClk               (lcbClk), 
   .lcbCoreSelect        (rcvrChLcbSelect[0:0]), 
   .lcbDataIn            (lsiDataOut), 
   .lcbRead              (rcvrChLcbRead[0:0]), 
   .lcbWrite             (rcvrChLcbWrite[0:0]), 
   .relTimeStamp         (relTs), 
   .resetn               (lcb_resetn), 
   .syncClk              (a2dClk), 
   .syncIn               (adcISync), 
   .testCount            (testCount), 
   .testSignal           (testSignal), 
   .collFifoAlmostEmpty  (wbCh0CollFifoAlmostEmpty), 
   .collFifoDataOut      (wbCh0CollFifoDataOut), 
   .collFifoEmpty        (wbCh0CollFifoEmpty), 
   .dropFrameRouteCode   (wbCh0DropFrameRouteCode), 
   .frameSizeBytesRouter (wbCh0FrameSizeBytesRouter), 
   .lcbAck               (rcvrCh1LcbAck), 
   .lcbDataOut           (rcvrCh1LcbData), 
   .routeCode            (wbCh0RouteCode)
); 

rcvr_channel ch2_rcvr( 
   .a2dCh1               (a2dCh1FifoDataOut), 
   .a2dCh2               (a2dCh2FifoDataOut), 
   .a2dCh3               (0), //mod
   .a2dCh4               (0), //mod
   .a2dClk               (a2dClk), 
   .abs1ppsTimeStamp     (absTs1pps), 
   .absSampleTimeStamp   (absTsSample), 
   .collFifoReadClk      (clk200Mhz), 
   .collFifoReadEnable   (wbCh1CollFifoReadEnable), 
   .lcbAddress           (lsiAddress[12:0]), 
   .lcbClk               (lcbClk), 
   .lcbCoreSelect        (rcvrChLcbSelect[1:1]), 
   .lcbDataIn            (lsiDataOut), 
   .lcbRead              (rcvrChLcbRead[1:1]), 
   .lcbWrite             (rcvrChLcbWrite[1:1]), 
   .relTimeStamp         (relTs), 
   .resetn               (lcb_resetn), 
   .syncClk              (a2dClk), 
   .syncIn               (adcISync), 
   .testCount            (testCount), 
   .testSignal           (testSignal), 
   .collFifoAlmostEmpty  (wbCh1CollFifoAlmostEmpty), 
   .collFifoDataOut      (wbCh1CollFifoDataOut), 
   .collFifoEmpty        (wbCh1CollFifoEmpty), 
   .dropFrameRouteCode   (wbCh1DropFrameRouteCode), 
   .frameSizeBytesRouter (wbCh1FrameSizeBytesRouter), 
   .lcbAck               (rcvrCh2LcbAck), 
   .lcbDataOut           (rcvrCh2LcbData), 
   .routeCode            (wbCh1RouteCode)
); 

epgSmBusMaster sm_bus1( 
   .lcbAddress    (lsiAddress[3:0]), 
   .lcbClk        (lcbClk), 
   .lcbCoreSelect (smBusLcbSelect), 
   .lcbDataIn     (lsiDataOut), 
   .lcbRead       (smBusLcbRead), 
   .lcbWrite      (smBusLcbWrite), 
   .resetn        (lcb_resetn), 
   .serdesClk     (clk390KHz), 
   .lcbAck        (smBusLcbAck), 
   .lcbDataOut    (smBusLcbData), 
   .smbClk        (I2C_SCLK), 
   .smbData       (I2C_SDATA)
); 

epgSpiControllerG2 ext_clk_spi( 
   .lcbAddress     (lsiAddress[7:0]), 
   .lcbClk         (lcbClk), 
   .lcbCoreSelect  (spiLcbSelect), 
   .lcbDataIn      (lsiDataOut), 
   .lcbRead        (spiLcbRead), 
   .lcbWrite       (spiLcbWrite), 
   .resetn         (lcb_resetn), 
   .serdesClk      (lcbClk), 
   .spiDataIn      (EXT_CLK_SDI), 
   .lcbAck         (spiLcbAck), 
   .lcbDataOut     (spiLcbData), 
   .spiChipSelectn (EXT_CLK_CSn), 
   .spiClk         (EXT_CLK_SCLK), 
   .spiDataOut     (EXT_CLK_SDO), 
   .spiWriteEnable ()
); 

test_stimulus test_stim( 
   .lcbAddress    (lsiAddress[7:0]), 
   .lcbClk        (lcbClk), 
   .lcbCoreSelect (testStimulusLcbSelect), 
   .lcbDataIn     (lsiDataOut), 
   .lcbRead       (testStimulusLcbRead), 
   .lcbWrite      (testStimulusLcbWrite), 
   .resetn        (lcb_resetn), 
   .syncClk       (a2dClk), 
   .syncIn        (adcISync), 
   .lcbAck        (testStimulusLcbAck), 
   .lcbDataOut    (testStimulusLcbData), 
   .testCount     (testCount), 
   .testSignal    (testSignal)
); 

time_stamp_counters_g2 time_stamp_counter( 
   .lcbAddress    (lsiAddress[7:0]), 
   .lcbClk        (lcbClk), 
   .lcbCoreSelect (timeStampLcbSelect), 
   .lcbDataIn     (lsiDataOut), 
   .lcbRead       (timeStampLcbRead), 
   .lcbWrite      (timeStampLcbWrite), 
   .pulse1pps     (GND), 
   .resetn        (lcb_resetn), 
   .syncPulse     (adcISync), 
   .tsClk         (a2dClk), 
   .absTs1pps     (absTs1pps), 
   .absTsSample   (absTsSample), 
   .lcbAck        (timeStampLcbAck), 
   .lcbDataOut    (timeStampLcbData), 
   .relTs         (relTs)
); 

clockControl clockControl1( 
   .A2D_CLKn            (A2D_CLKn), 
   .A2D_CLKp            (A2D_CLKp), 
   .DAC_GDATACLKn       (DAC_GDATACLKn), 
   .DAC_GDATACLKp       (DAC_GDATACLKp), 
   .LVCLK_200n          (LVCLK_200n), 
   .LVCLK_200p          (LVCLK_200p), 
   .MGT_REFCLK_n        (MGT_REFCLK_n), 
   .MGT_REFCLK_p        (MGT_REFCLK_p), 
   .PCIE_REFCLK_n       (PCIE_REFCLK_n), 
   .PCIE_REFCLK_p       (PCIE_REFCLK_p), 
   .a2d_clk_reset       (a2d_clk_reset), 
   .dacClkLowFreqSelect (dacClkLowFreqSelect), 
   .dac_reset           (dac_reset), 
   .pll_reset           (pll_reset), 
   .user_clk            (user_clk), 
   .a2dClk              (a2dClk), 
   .a2d_clk_locked      (a2d_clk_locked), 
   .clk150Mhz           (clk150Mhz), 
   .clk200Mhz           (clk200Mhz), 
   .clk390KHz           (clk390KHz), 
   .dacClk              (dacClk), 
   .dac_clk_div2        (dac_clk_div2), 
   .dac_clk_locked      (dac_clk_locked), 
   .lcbClk              (lcbClk), 
   .lcb_clk_locked      (lcb_clk_locked), 
   .mgtRefClk           (mgtRefClk), 
   .pcie_ref_clk        (pcie_ref_clk), 
   .user_clk_div2       (user_clk_div2), 
   .user_clk_locked     (user_clk_locked)
); 

rxTxFpgaRegs regs1( 
   .lcbClk              (lcbClk), 
   .lcbAddress          (lsiAddress[15:0]), 
   .resetn              (lcb_resetn), 
   .lcbWrite            (lsiWrite), 
   .lcbRead             (lsiRead), 
   .lcbCoreSelect       (lsiCs), 
   .lcbDataIn           (lsiDataOut), 
   .portAck             (lcbAckOr), 
   .waveGenData         (waveGenData), 
   .auroraData          (32'h00000000), 
   .pgData              (pgLcbData), 
   .flashData           (32'h00000000), 
   .timeStampData       (timeStampLcbData), 
   .a2dCh1Data          (a2dCh1LcbData), 
   .a2dCh2Data          (a2dCh2LcbData), 
   .adcLsSyncCtrlData   (adcLsSyncCtrlLcbData), 
   .dacLsSyncCtrlData   (dacLsSyncCtrlLcbData), 
   .testStimulusData    (testStimulusLcbData), 
   .sdramData           (32'h00000000), 
   .dacData             (dacLcbData), 
   .spiData             (spiLcbData), 
   .smBusData           (smBusLcbData), 
   .rcvrCh1Data         (rcvrCh1LcbData), 
   .rcvrCh2Data         (rcvrCh2LcbData), 
   .pcieData            (pcieLcbData), 
   .lcbClkLocked        (lcb_clk_locked), 
   .userClkLocked       (user_clk_locked), 
   .sdramClkLocked      (1'b1), 
   .a2dClkLocked        (a2d_clk_locked), 
   .dacClkLocked        (dac_clk_locked), 
   .extClkStatus        (EXT_CLK_STATUS), 
   .tempAlertn          (TEMP_ALERTn), 
   .tempThermn          (TEMP_THERMn), 
   .lcbDataOut          (lsiDataIn), 
   .lcbAck              (lsiAck), 
   .a2dDcmReset         (a2dDcmReset), 
   .dacDcmReset         (dacDcmReset), 
   .sdramDcmReset       (sdramDcmReset), 
   .extClkFunction      (EXT_CLK_FUNC), 
   .a2dFifoWriteEnable  (a2dFifoWrEnable), 
   .dacClkLowFreqSelect (dacClkLowFreqSelect), 
   .i2cBusSelect        (I2C_BUS_SELECT), 
   .waveGenEnable       (waveGenEnable), 
   .waveGenLoopAddress  (waveGenLoopAddress), 
   .waveGenSelect       (waveGenSelect), 
   .waveGenWrite        (waveGenWrite), 
   .waveGenRead         (waveGenRead), 
   .auroraSelect        (), 
   .auroraWrite         (), 
   .auroraRead          (), 
   .pgSelect            (pgLcbSelect), 
   .pgWrite             (pgLcbWrite), 
   .pgRead              (pgLcbRead), 
   .flashSelect         (), 
   .flashWrite          (), 
   .flashRead           (), 
   .timeStampSelect     (timeStampLcbSelect), 
   .timeStampWrite      (timeStampLcbWrite), 
   .timeStampRead       (timeStampLcbRead), 
   .a2dChSelect         (a2dChLcbSelect), 
   .a2dChWrite          (a2dChLcbWrite), 
   .a2dChRead           (a2dChLcbRead), 
   .lsSyncCtrlSelect    (lsSyncCtrlLcbSelect), 
   .lsSyncCtrlWrite     (lsSyncCtrlLcbWrite), 
   .lsSyncCtrlRead      (lsSyncCtrlLcbRead), 
   .testStimulusSelect  (testStimulusLcbSelect), 
   .testStimulusWrite   (testStimulusLcbWrite), 
   .testStimulusRead    (testStimulusLcbRead), 
   .sdramSelect         (), 
   .sdramWrite          (), 
   .sdramRead           (), 
   .dacSelect           (dacLcbSelect), 
   .dacWrite            (dacLcbWrite), 
   .dacRead             (dacLcbRead), 
   .spiSelect           (spiLcbSelect), 
   .spiWrite            (spiLcbWrite), 
   .spiRead             (spiLcbRead), 
   .smBusSelect         (smBusLcbSelect), 
   .smBusWrite          (smBusLcbWrite), 
   .smBusRead           (smBusLcbRead), 
   .rcvrChSelect        (rcvrChLcbSelect), 
   .rcvrChWrite         (rcvrChLcbWrite), 
   .rcvrChRead          (rcvrChLcbRead), 
   .pcieSelect          (pcieLcbSelect), 
   .pcieWrite           (pcieLcbWrite), 
   .pcieRead            (pcieLcbRead)
); 

waveGen wavegen1( 
   .clk              (clk150Mhz), 
   .enable           (waveGenEnable), 
   .lcbAddress       (lsiAddress[13:0]), 
   .lcbClk           (lcbClk), 
   .lcbCoreSelect    (waveGenSelect), 
   .lcbDataIn        (lsiDataOut), 
   .lcbRead          (waveGenRead), 
   .lcbWrite         (waveGenWrite), 
   .ramLoopAddress   (waveGenLoopAddress), 
   .resetn           (lcb_resetn), 
   .trigIn           (waveGenTrigger), 
   .txFifoAlmostFull (dacTxFifoAlmostFull), 
   .txFifoFull       (dacTxFifoFull), 
   .lcbAck           (waveGenLcbAck), 
   .lcbDataOut       (waveGenData), 
   .trigOut          (waveGenTrigOut), 
   .txFifoData       (dacTxFifoData), 
   .txFifoWrite      (dacTxFifoWrite)
); 

IBUFDS_LVDS_25 adc_sync_ibuf( .O  (adcSyncIn), .I  (ADC_SYNC_INp), .IB (ADC_SYNC_INn)); 
IBUFDS_LVDS_25 dac_sync_ibuf( .O  (dacSyncIn), .I  (DAC_SYNC_INp), .IB (DAC_SYNC_INn)); 
OBUFDS_LVDS_25 adc_sync_dis_obuf( .O  (ADC_SYNC_DISABLEp), .OB (ADC_SYNC_DISABLEn), .I  (adcSyncDisable)); 
OBUFDS_LVDS_25 adc_sync_obuf( .O  (ADC_SYNC_OUTp), .OB (ADC_SYNC_OUTn), .I  (adcSyncOut)); 
OBUFDS_LVDS_25 dac_sync_dis_obuf( .O  (DAC_SYNC_DISABLEp), .OB (DAC_SYNC_DISABLEn), .I  (dacSyncDisable)); 
OBUFDS_LVDS_25 dac_sync_obuf( .O  (DAC_SYNC_OUTp), .OB (DAC_SYNC_OUTn), .I  (dacSyncOut)); 

assign lcbAckOr = waveGenLcbAck | pcieLcbAck | pgLcbAck | a2dCh1LcbAck | a2dCh2LcbAck 
  | timeStampLcbAck | testStimulusLcbAck | adcLsSyncCtrlLcbAck | dacLcbAck 
  | dacLsSyncCtrlLcbAck | spiLcbAck | smBusLcbAck;


wire perst_n;
assign perst_n = PCIE_RESETn;

assign pll_reset = ~(perst_n & trn_reset_n);
assign reset     = ~(perst_n & trn_reset_n & ~trn_lnk_up_n & user_clk_locked);
assign resetn    = ~reset;

reg d2_user_rst;
reg d1_user_rst;
reg user_rst;
reg d2_user_rst_div2;
reg d1_user_rst_div2;
reg user_rst_div2;
reg d2_lcb_rst;
reg d1_lcb_rst;
reg lcb_rst;

wire user_rst_bufg;
wire user_rst_div2_bufg;
wire lcb_rst_bufg;

// Synchronize reset to user_clk
always @(posedge user_clk or posedge reset)
begin
    if (reset == 1'b1)
    begin
        d2_user_rst <= 1'b1;
        d1_user_rst <= 1'b1;
        user_rst    <= 1'b1;
    end
    else
    begin
        d2_user_rst <= 1'b0;
        d1_user_rst <= d2_user_rst;
        user_rst    <= d1_user_rst;
    end
end

assign user_rst_n = ~user_rst;

// Synchronize reset to user_clk_div2
always @(posedge user_clk_div2 or posedge reset)
begin
    if (reset == 1'b1)
    begin
        d2_user_rst_div2 <= 1'b1;
        d1_user_rst_div2 <= 1'b1;
        user_rst_div2    <= 1'b1;
    end
    else
    begin
        d2_user_rst_div2 <= 1'b0;
        d1_user_rst_div2 <= d2_user_rst_div2;
        user_rst_div2    <= d1_user_rst_div2;
    end
end

assign user_rst_div2_n = ~user_rst_div2;

// Synchronize reset to lcbClk
always @(posedge lcbClk or posedge reset)
begin
    if (reset == 1'b1)
    begin
        d2_lcb_rst <= 1'b1;
        d1_lcb_rst <= 1'b1;
        lcb_rst    <= 1'b1;
    end
    else
    begin
        d2_lcb_rst <= 1'b0;
        d1_lcb_rst <= d2_lcb_rst;
        lcb_rst    <= d1_lcb_rst;
    end
end
assign lcb_reset = lcb_rst;
assign lcb_resetn = ~lcb_rst;

assign a2d_clk_reset = reset | a2dDcmReset;
assign dac_reset = reset | dacDcmReset;

assign fast_train_simulation_only = 1'b0;


assign wbCh0BusGrant = {7'h0,pcieC2sBusGrant[0]};
assign wbCh0DestFifoAlmostFull = {7'h0,pcieC2sFifoAlmostFull};
assign wbCh0DestFifoFull = {7'h0,pcieC2sFifoFull};
assign wbCh1BusGrant = {7'h0,pcieC2sBusGrant[1]};
assign wbCh1DestFifoAlmostFull = {7'h0,pcieC2sFifoAlmostFull};
assign wbCh1DestFifoFull = {7'h0,pcieC2sFifoFull};
assign pcieC2sBusRequest = {pgBusRequest[3:0],2'h0,wbCh1BusRequest[0],wbCh0BusRequest[0]};
assign pcieC2sWriteEnable[7:0] = {packetWriteEnable[3:0],2'b0,wbCh1DestWrite[0],wbCh0DestWrite[0]};
assign dacTxFifoAFullThresh[5:0] = 6'h30;
assign dacTrigIn = waveGenEnable ? waveGenTrigPulseOut : dacISync;


endmodule
