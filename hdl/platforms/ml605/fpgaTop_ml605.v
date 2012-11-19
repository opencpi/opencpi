// fpgaTop_ml605.v - ssiegel 2009-03-17
// 2012-02-11 ssiegel Added MDIO port

module fpgaTop(
  input  wire        sys0_clkp,      // sys0 Clock +
  input  wire        sys0_clkn,      // sys0 Clock -
  input  wire        sys1_clkp,      // sys1 Clock +
  input  wire        sys1_clkn,      // sys1 Clock -

  input  wire        pci0_clkp,      // PCIe Clock +
  input  wire        pci0_clkn,      // PCIe Clock -
  input  wire        pci0_reset_n,   // PCIe Reset
  output wire [3:0]  pci_exp_txp,    // PCIe lanes...
  output wire [3:0]  pci_exp_txn,
  input  wire [3:0]  pci_exp_rxp,
  input  wire [3:0]  pci_exp_rxn,

  output wire [12:0] led,            // LEDs ml605
  output wire [ 3:0] lcd_db,         // LCD databus
  output wire        lcd_e,          // LCD enable
  output wire        lcd_rs,         // LCD register-select
  output wire        lcd_rw,         // LCD read-not-write
  input  wire        ppsExtIn,       // PPS in
  output wire        ppsOut,         // PPS out

  output wire        gmii_rstn,      // Alaska GMII...
	output wire        gmii_gtx_clk,
	output wire [7:0]  gmii_txd,
	output wire        gmii_tx_en,
	output wire        gmii_tx_er,
	input  wire        gmii_rx_clk,
	input  wire [7:0]  gmii_rxd,
	input  wire        gmii_rx_dv,
	input  wire        gmii_rx_er,
  output wire        gmii_tx_clk,
  input  wire        gmii_COL,
  input  wire        gmii_CRS,
  input  wire        gmii_INT,
  output wire        mdio_mdc,       // Alaska MDIO...
  inout  wire        mdio_mdd,

  output wire [23:0] flash_addr,
  inout  wire [15:0] flash_io_dq,
  input  wire        flash_wait,
  output wire        flash_we_n,
  output wire        flash_oe_n,
  output wire        flash_ce_n,

  inout  wire [63:0] ddr3_dq,        // DDR3 DRAM...
  output wire [12:0] ddr3_addr,
  output wire [2:0]  ddr3_ba,
  output wire        ddr3_ras_n,
  output wire        ddr3_cas_n,
  output wire        ddr3_we_n,
  output wire        ddr3_reset_n,
  output wire [0:0]  ddr3_cs_n,
  output wire [0:0]  ddr3_odt,
  output wire [0:0]  ddr3_cke,
  output wire [7:0]  ddr3_dm,
  inout  wire [7:0]  ddr3_dqs_p,
  inout  wire [7:0]  ddr3_dqs_n,
  output wire [0:0]  ddr3_ck_p,
  output wire [0:0]  ddr3_ck_n,

  output wire        flp_com_sclk,   // FMC150 in LPC Slot...
  output wire        flp_com_sdc2m,
  output wire        flp_cdc_csb,
  output wire        flp_dac_csb,
  input  wire        flp_cdc_sdi,
  input  wire        flp_dac_sdi,
//  input  wire [3:0]  flp_sdi_sdm2c,
//  output wire [3:0]  flp_csb 

  output wire        flp_cdc_rstn,
  output wire        flp_cdc_pdn,

//  output wire        dac0_txena,  //TODO These 7
//  output wire        dac0_dclkp,
//  output wire        dac0_dclkn,
//  output wire        dac0_framep,
//  output wire        dac0_framen,
//  output wire [7:0]  dac0_dap, 
//  output wire [7:0]  dac0_dan, 

  //output wire        flp_mon_rstn,
  //output wire        flp_mon_intn,
  //output wire        flp_adc_rstn,
  input  wire        flp_cdc_clk_p,
  input  wire        flp_cdc_clk_n, 
  //input  wire        flp_cdc_pllstat,
  output wire        flp_cdc_refen
);

//FIXME:
assign flp_cdc_pdn   = 1'b1;
assign flp_cdc_refen = 1'b1;

wire flpCDC_sclkn, flpCDC_sclkgate;
wire flpDAC_sclkn, flpDAC_sclkgate;
assign flp_com_sclk = (flpCDC_sclkn && flpCDC_sclkgate) || (flpDAC_sclkn && flpDAC_sclkgate); 

wire flpCDC_com_sdc2m, flpDAC_com_sdc2m;
assign flp_com_sdc2m = (flpCDC_com_sdc2m && flpCDC_sclkgate) || (flpDAC_com_sdc2m && flpDAC_sclkgate);

// Instance and connect mkFTop...
 mkFTop_ml605 ftop(
  .sys0_clkp         (sys0_clkp),
  .sys0_clkn         (sys0_clkn),
  .sys1_clkp         (sys1_clkp),
  .sys1_clkn         (sys1_clkn),

  .pci0_clkp         (pci0_clkp),
  .pci0_clkn         (pci0_clkn),
  .pci0_rstn         (pci0_reset_n),
  .pcie_rxp_i        (pci_exp_rxp),
  .pcie_rxn_i        (pci_exp_rxn),
  .pcie_txp          (pci_exp_txp),
  .pcie_txn          (pci_exp_txn),

  .led               (led),
  .lcd_db            (lcd_db),
  .lcd_e             (lcd_e),
  .lcd_rs            (lcd_rs),
  .lcd_rw            (lcd_rw),
  .gps_ppsSyncIn_x   (ppsExtIn),
  .gps_ppsSyncOut    (ppsOut),

	.gmii_rstn         (gmii_rstn),
	.gmii_tx_txd       (gmii_txd),
	.gmii_tx_tx_en     (gmii_tx_en),
	.gmii_tx_tx_er     (gmii_tx_er),
	.gmii_rx_rxd_i     (gmii_rxd),
	.gmii_rx_rx_dv_i   (gmii_rx_dv),
	.gmii_rx_rx_er_i   (gmii_rx_er),
	.gmii_tx_tx_clk    (gmii_gtx_clk),
	.gmii_rx_clk       (gmii_rx_clk),
	.gmii_col_i        (gmii_COL),
	.gmii_crs_i        (gmii_CRS),
	.gmii_intr_i       (gmii_INT),
  .mdio_mdc          (mdio_mdc),
  .mdio_mdd          (mdio_mdd),

  .flash_addr        (flash_addr),
  .flash_io_dq       (flash_io_dq),
  .flash_fwait_i     (flash_wait),
  .flash_we_n        (flash_we_n),
  .flash_oe_n        (flash_oe_n),
  .flash_ce_n        (flash_ce_n),

  .dram_io_dq        (ddr3_dq), 
  .dram_addr         (ddr3_addr),
  .dram_ba           (ddr3_ba),
  .dram_ras_n        (ddr3_ras_n),
  .dram_cas_n        (ddr3_cas_n),
  .dram_we_n         (ddr3_we_n),
  .dram_reset_n      (ddr3_reset_n),
  .dram_cs_n         (ddr3_cs_n),
  .dram_odt          (ddr3_odt),
  .dram_cke          (ddr3_cke),
  .dram_dm           (ddr3_dm),
  .dram_io_dqs_p     (ddr3_dqs_p),
  .dram_io_dqs_n     (ddr3_dqs_n),
  .dram_ck_p         (ddr3_ck_p),
  .dram_ck_n         (ddr3_ck_n),

  .flpCDC_sclkn      (flpCDC_sclkn),      // Use the inverted clock for slow-balanced setup/hold
  .flpCDC_sclkgate   (flpCDC_sclkgate), 
  .flpCDC_sdo        (flpCDC_com_sdc2m),
  .flpCDC_csb        (flp_cdc_csb),
  .flpCDC_sdi_arg    (flp_cdc_sdi),

  .flpDAC_sclkn      (flpDAC_sclkn),      // Use the inverted clock for slow-balanced setup/hold
  .flpDAC_sclkgate   (flpDAC_sclkgate), 
  .flpDAC_sdo        (flpDAC_com_sdc2m),
  .flpDAC_csb        (flp_dac_csb),
  .flpDAC_sdi_arg    (flp_dac_sdi),

 //  .dac0_txena       (dac0_txena),  //TODO These 7
 //  .dac0_dclkp       (dac0_dclkp),
 //  .dac0_dclkn       (dac0_dclkn),
 //  .dac0_framep      (dac0_framep),
 //  .dac0_framen      (dac0_framen),
 //  .dac0_dap         (dac0_dap), 
 //  .dac0_dan         (dac0_dan), 

  .flpCDC_srst        (flp_cdc_rstn),      // The srst from SPICore32 active-low
  //.flp_cdc_pdn       (flp_cdc_pdn),
  //.flp_mon_rstn      (flp_mon_rstn),
  //.flp_mon_intn      (flp_mon_intn),
  //.flp_adc_rstn      (flp_adc_rstn),
  .flp_cdc_clk_p     (flp_cdc_clk_p),
  .flp_cdc_clk_n     (flp_cdc_clk_n) 
  //.flp_cdc_pllstat   (flp_cdc_pllstat),
  //.flp_cdc_refen     (flp_cdc_refen)
);

endmodule
