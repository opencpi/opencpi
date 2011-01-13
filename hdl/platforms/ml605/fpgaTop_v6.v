// fpgaTop_v6.v - ssiegel 2009-03-17

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
  input  wire        ppsExtIn,       // PPS in
  output wire        ppsOut,         // PPS out

	output wire [7:0]  gmii_txd,       // Alaska GMII...
	output wire        gmii_tx_en,
	output wire        gmii_tx_er,
	input  wire [7:0]  gmii_rxd,
	input  wire        gmii_rx_dv,
	input  wire        gmii_rx_er,
	output wire        gmii_tx_clk,
	input  wire        gmii_rx_clk,

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
  output wire [0:0]  ddr3_ck_n

  //output wire        flp_com_sclk,   // FMC150 in LPC Slot...
  //output wire        flp_com_sdc2m,
  //input  wire        flp_cdc_sdm2c,
  //input  wire        flp_mon_sdm2c,
  //input  wire        flp_adc_sdm2c,
  //input  wire        flp_dac_sdm2c,
  //output wire        flp_cdc_sen_n,
  //output wire        flp_mon_sen_n,
  //output wire        flp_adc_sen_n,
  //output wire        flp_dac_sen_n,
  //output wire        flp_cdc_rstn,
  //output wire        flp_cdc_pdn,
  //output wire        flp_mon_rstn,
  //output wire        flp_mon_intn,
  //output wire        flp_adc_rstn
);

// Instance and connect mkFTop...
 mkFTop ftop(
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
  .gps_ppsSyncIn_x   (ppsExtIn),
  .gps_ppsSyncOut    (ppsOut),

	.gmii_txd          (gmii_txd),
	.gmii_tx_en        (gmii_tx_en),
	.gmii_tx_er        (gmii_tx_er),
	.gmii_rxd_i        (gmii_rxd),
	.gmii_rx_dv_i      (gmii_rx_dv),
	.gmii_rx_er_i      (gmii_rx_er),
	.gmii_tx_clk       (gmii_tx_clk),
	.gmii_rx_clk       (gmii_rx_clk),

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
  .dram_ck_n         (ddr3_ck_n)

  //.flp_com_sclk      (flp_com_sclk),
  //.flp_com_sdc2m     (flp_com_sdc2m),
  //.flp_cdc_sdm2c     (flp_cdc_sdm2c),
  //.flp_mon_sdm2c     (flp_mon_sdm2c),
  //.flp_adc_sdm2c     (flp_adc_sdm2c),
  //.flp_dac_sdm2c     (flp_dac_sdm2c),
  //.flp_cdc_sen_n     (flp_cdc_sen_n),
  //.flp_mon_sen_n     (flp_mon_sen_n),
  //.flp_adc_sen_n     (flp_adc_sen_n),
  //.flp_dac_sen_n     (flp_dac_sen_n),
  //.flp_cdc_rstn      (flp_cdc_rstn),
  //.flp_cdc_pdn       (flp_cdc_pdn),
  //.flp_mon_rstn      (flp_mon_rstn),
  //.flp_mon_intn      (flp_mon_intn),
  //.flp_adc_rstn      (flp_adc_rstn)
);

endmodule
