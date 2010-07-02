`include "ocpi_ocp_defs.v"
localparam OCPI_WCI_CONFIG =       1'b1;
localparam OCPI_WCI_CONTROL =      1'b0;
localparam OCPI_WCI_EXISTS =       0;
localparam OCPI_WCI_INITIALIZED =  1;
localparam OCPI_WCI_OPERATING =    2;
localparam OCPI_WCI_SUSPENDED =    3;
localparam OCPI_WCI_UNUSABLE =     4;
localparam OCPI_WCI_STATE_WIDTH =  3;
`define OCPI_WCI_STATE_RANGE       [OCPI_WIP_WCI_STATE_WIDTH-1:0]
localparam OCPI_WCI_INITIALIZE =   3'h0;
localparam OCPI_WCI_START =        3'h1;
localparam OCPI_WCI_STOP =         3'h2;
localparam OCPI_WCI_RELEASE =      3'h3;
localparam OCPI_WCI_TEST =         3'h4;
localparam OCPI_WCI_BEFORE_QUERY = 3'h5;
localparam OCPI_WCI_AFTER_CONFIG = 3'h6;
localparam OCPI_WCI_RESERVED =     3'h7;


