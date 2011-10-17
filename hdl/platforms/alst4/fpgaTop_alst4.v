// fpgaTop_alst4.v - the top-level Verilog for the Altera 4SGX230N board
// Copyright (c) 2011 Atomic Rules LLC - ALL RIGHTS RESERVED
//

module fpgaTop (
  input  wire         sys0_clk,       // 100 MHz free-running
  input  wire         sys0_rstn,      // External active-low reset 
  input  wire         pcie_clk,       // PCIe Clock
  input  wire         pcie_rstn,      // PCIe Reset
  input  wire [ 3:0]  pcie_rx,        // PCIe lanes...
  output wire [ 3:0]  pcie_tx,
  input  wire [ 7:0]  usr_sw,         // dip-switches
  output wire [15:0]  led             // leds
);

// Instance and connect mkFTop...
 mkFTop_alst4 ftop (
  .sys0_clk          (sys0_clk),
  .sys0_rstn         (sys0_rstn),
  .pcie_clk          (pcie_clk),
  .pcie_rstn         (pcie_rstn),
  .pcie_rx_i         (pcie_rx),
  .pcie_tx           (pcie_tx),
  .usr_sw_i          (usr_sw),
  .led               (led),
  .p125clk           (),
  .CLK_GATE_p125clk  (),
  .p125rst           ()
);

endmodule
