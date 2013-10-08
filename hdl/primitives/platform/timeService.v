// Time server extracted from mkOCCP.v the hard way
// not "mk" since I'm not sure that the interface would be
// This may end up being instanced in the platform worker, so the controls
// would already be registered
module timeService(
`ifdef not
		   input 	 CLK,
		   input 	 RST_N,
		   input 	 CLK_time_clk,
		   input 	 RST_N_time_rst,
		   // Interface to set the time, with permission
		   input [63:0]  timeIn,
		   input 	 doSetTime, // set the time: now = timeIn
		   input 	 doDeltaTime, // set deltaTime = now - timeIn
		   // Interface to set the controls, without permission
		   input 	 doSetControl,
		   input 	 doClear,

		   // Status outputs
		   output [31:0] statusOut,
		   output [4:0]  controlOut,
		   output [27:0] ticksPerPPSOut,
		   output [63:0] nowCC, // now in common/control clock domain
		   output [63:0] deltaTime, // result of last doDeltaTime
		   output [63:0] nowTC, // now in timeservice clock domain, for time clients
		  
		   // PPS interface
		   input 	 ppsSyncIn,
		   output 	 ppsSyncOut
`endif
		   CLK,
		   RST_N,
		   CLK_time_clk,
		   RST_N_time_rst,
		   // Interface to set the time, with permission
		   timeIn,
		   doSetTime, // set the time: now = timeIn
		   doDeltaTime, // set deltaTime = now - timeIn
		   // Interface to set the controls, without permission
		   doSetControl,
		   doClear,
		   // Status outputs
		   statusOut,
		   controlOut,
		   ticksPerPPSOut,
		   nowCC, // now in common/control clock domain
		   deltaTime, // result of last doDeltaTime
		   nowTC, // now in timeservice clock domain, for time clients
		  
		   // PPS interface
		   ppsSyncIn,
		   ppsSyncOut
		  );

  input 			 CLK;
  input 			 RST_N;
  input 			 CLK_time_clk;
  input 			 RST_N_time_rst;
  
  // Interface to set the time, with permission
  input [63:0] 			 timeIn;
  input 			 doSetTime;   // set the time: now = timeIn
  input 			 doDeltaTime; // set deltaTime = now - timeIn
  // Interface to set the controls, without permission
  input 			 doSetControl;
  input 			 doClear;
  // Status outputs
  output [31:0] 		 statusOut;
  output [4:0] 			 controlOut;
  output [27:0] 		 ticksPerPPSOut;
  output [63:0] 		 nowCC;       // now in common/control clock domain
  output [63:0] 		 deltaTime;   // result of last doDeltaTime
  output [63:0] 		 nowTC;        // now in timeservice clock domain, for time clients
  // PPS interface
  input 			 ppsSyncIn;
  output 			 ppsSyncOut;

  // From here, extracted from mkOCCP
  wire [49 : 0] 		timeServ_jamFracVal_1$wget;
  wire
      timeServ_jamFracVal_1$whas,
      timeServ_jamFrac_1$wget,
      timeServ_jamFrac_1$whas;

  // register deltaTime
  reg [63 : 0] deltaTime;
  wire [63 : 0] deltaTime$D_IN;
  wire deltaTime$EN;

  // register timeServ_delSec
  reg [1 : 0] timeServ_delSec;
  wire [1 : 0] timeServ_delSec$D_IN;
  wire timeServ_delSec$EN;

  // register timeServ_delSecond
  reg [49 : 0] timeServ_delSecond;
  wire [49 : 0] timeServ_delSecond$D_IN;
  wire timeServ_delSecond$EN;

  // register timeServ_fracInc
  reg [49 : 0] timeServ_fracInc;
  wire [49 : 0] timeServ_fracInc$D_IN;
  wire timeServ_fracInc$EN;

  // register timeServ_fracSeconds
  reg [49 : 0] timeServ_fracSeconds;
  wire [49 : 0] timeServ_fracSeconds$D_IN;
  wire timeServ_fracSeconds$EN;

  // register timeServ_gpsInSticky
  reg timeServ_gpsInSticky;
  wire timeServ_gpsInSticky$D_IN, timeServ_gpsInSticky$EN;

  // register timeServ_jamFrac
  reg timeServ_jamFrac;
  wire timeServ_jamFrac$D_IN, timeServ_jamFrac$EN;

  // register timeServ_jamFracVal
  reg [49 : 0] timeServ_jamFracVal;
  wire [49 : 0] timeServ_jamFracVal$D_IN;
  wire timeServ_jamFracVal$EN;

  // register timeServ_lastSecond
  reg [49 : 0] timeServ_lastSecond;
  wire [49 : 0] timeServ_lastSecond$D_IN;
  wire timeServ_lastSecond$EN;

  // register timeServ_now
  reg [63 : 0] timeServ_now;
  wire [63 : 0] timeServ_now$D_IN;
  wire timeServ_now$EN;

  // register timeServ_ppsDrive
  reg timeServ_ppsDrive;
  wire timeServ_ppsDrive$D_IN, timeServ_ppsDrive$EN;

  // register timeServ_ppsEdgeCount
  reg [7 : 0] timeServ_ppsEdgeCount;
  wire [7 : 0] timeServ_ppsEdgeCount$D_IN;
  wire timeServ_ppsEdgeCount$EN;

  // register timeServ_ppsExtCapture
//  reg timeServ_ppsExtCapture;
//  wire timeServ_ppsExtCapture$D_IN, timeServ_ppsExtCapture$EN;

  // register timeServ_ppsExtSyncD
  reg timeServ_ppsExtSyncD;
  wire timeServ_ppsExtSyncD$D_IN, timeServ_ppsExtSyncD$EN;

  // register timeServ_ppsExtSync_d1
  reg timeServ_ppsExtSync_d1;
  wire timeServ_ppsExtSync_d1$D_IN, timeServ_ppsExtSync_d1$EN;

  // register timeServ_ppsExtSync_d2
  reg timeServ_ppsExtSync_d2;
  wire timeServ_ppsExtSync_d2$D_IN, timeServ_ppsExtSync_d2$EN;

  // register timeServ_ppsInSticky
  reg timeServ_ppsInSticky;
  wire timeServ_ppsInSticky$D_IN, timeServ_ppsInSticky$EN;

  // register timeServ_ppsLost
  reg timeServ_ppsLost;
  wire timeServ_ppsLost$D_IN, timeServ_ppsLost$EN;

  // register timeServ_ppsLostSticky
  reg timeServ_ppsLostSticky;
  wire timeServ_ppsLostSticky$D_IN, timeServ_ppsLostSticky$EN;

  // register timeServ_ppsOK
  reg timeServ_ppsOK;
  wire timeServ_ppsOK$D_IN, timeServ_ppsOK$EN;

  // register timeServ_refFreeCount
  reg [27 : 0] timeServ_refFreeCount;
  wire [27 : 0] timeServ_refFreeCount$D_IN;
  wire timeServ_refFreeCount$EN;

  // register timeServ_refFreeSamp
  reg [27 : 0] timeServ_refFreeSamp;
  wire [27 : 0] timeServ_refFreeSamp$D_IN;
  wire timeServ_refFreeSamp$EN;

  // register timeServ_refFreeSpan
  reg [27 : 0] timeServ_refFreeSpan;
  wire [27 : 0] timeServ_refFreeSpan$D_IN;
  wire timeServ_refFreeSpan$EN;

  // register timeServ_refFromRise
  reg [27 : 0] timeServ_refFromRise;
  wire [27 : 0] timeServ_refFromRise$D_IN;
  wire timeServ_refFromRise$EN;

  // register timeServ_refPerCount
  reg [27 : 0] timeServ_refPerCount;
  wire [27 : 0] timeServ_refPerCount$D_IN;
  wire timeServ_refPerCount$EN;

  // register timeServ_refSecCount
  reg [31 : 0] timeServ_refSecCount;
  wire [31 : 0] timeServ_refSecCount$D_IN;
  wire timeServ_refSecCount$EN;

  // register timeServ_rplTimeControl
  reg [4 : 0] timeServ_rplTimeControl;
  wire [4 : 0] timeServ_rplTimeControl$D_IN;
  wire timeServ_rplTimeControl$EN;

  // register timeServ_timeSetSticky
  reg timeServ_timeSetSticky;
  wire timeServ_timeSetSticky$D_IN, timeServ_timeSetSticky$EN;

  // register timeServ_xo2
  reg timeServ_xo2;
  wire timeServ_xo2$D_IN, timeServ_xo2$EN;

  // ports of submodule timeServ_disableServo
  wire timeServ_disableServo$dD_OUT,
       timeServ_disableServo$sD_IN,
       timeServ_disableServo$sEN,
       timeServ_disableServo$sRDY;

  // ports of submodule timeServ_nowInCC
  wire [63 : 0] timeServ_nowInCC$dD_OUT, timeServ_nowInCC$sD_IN;
  wire timeServ_nowInCC$sEN, timeServ_nowInCC$sRDY;

  // ports of submodule timeServ_ppsDisablePPS
  wire timeServ_ppsDisablePPS$dD_OUT,
       timeServ_ppsDisablePPS$sD_IN,
       timeServ_ppsDisablePPS$sEN,
       timeServ_ppsDisablePPS$sRDY;

  // ports of submodule timeServ_ppsLostCC
  wire timeServ_ppsLostCC$dD_OUT,
       timeServ_ppsLostCC$sD_IN,
       timeServ_ppsLostCC$sEN,
       timeServ_ppsLostCC$sRDY;

  // ports of submodule timeServ_ppsOKCC
  wire timeServ_ppsOKCC$dD_OUT,
       timeServ_ppsOKCC$sD_IN,
       timeServ_ppsOKCC$sEN,
       timeServ_ppsOKCC$sRDY;

  // ports of submodule timeServ_ppsOutMode
  wire [1 : 0] timeServ_ppsOutMode$dD_OUT, timeServ_ppsOutMode$sD_IN;
  wire timeServ_ppsOutMode$sEN, timeServ_ppsOutMode$sRDY;

  // ports of submodule timeServ_refPerPPS
  wire [27 : 0] timeServ_refPerPPS$dD_OUT, timeServ_refPerPPS$sD_IN;
  wire timeServ_refPerPPS$sEN, timeServ_refPerPPS$sRDY;

  // ports of submodule timeServ_rollingPPSIn
  wire [7 : 0] timeServ_rollingPPSIn$dD_OUT, timeServ_rollingPPSIn$sD_IN;
  wire timeServ_rollingPPSIn$sEN, timeServ_rollingPPSIn$sRDY;

  // ports of submodule timeServ_setRefF
  wire [63 : 0] timeServ_setRefF$dD_OUT, timeServ_setRefF$sD_IN;
  wire timeServ_setRefF$dDEQ,
       timeServ_setRefF$dEMPTY_N,
       timeServ_setRefF$sENQ,
       timeServ_setRefF$sFULL_N;

  // internal state
  wire [49 : 0] _281474976710656_MINUS_timeServ_delSecond__q1,
		x__h3700,
		x__h4421,
		x__h4649;
  wire [31:0]	x__h4715;
  wire [21 : 0] _281474976710656_MINUS_timeServ_delSecond_BITS__ETC__q2;
  wire
      IF_timeServ_ppsOK_7_THEN_timeServ_ppsExtSync_d_ETC___d5465,
      timeServ_ppsExtSync_d2_2_AND_NOT_timeServ_ppsE_ETC___d61,
      timeServ_ppsExtSync_d2_2_AND_NOT_timeServ_ppsE_ETC___d70,
      timeServ_refFromRise_3_ULE_199800000___d5459,
      timeServ_refFromRise_3_ULT_200200000___d5878;
  

  // Wires that don't have "timeserv" in their names...
  wire 
       WILL_FIRE_RL_cpDispatch_T_F_F_F_F_F_F_T,     // set time
       WILL_FIRE_RL_cpDispatch_T_F_F_F_F_T_F,       // set control
       WILL_FIRE_RL_cpDispatch_T_F_F_F_F_T_T,       // clear sticky status
       WILL_FIRE_RL_cpDispatch_T_F_F_F_F_F_F_F_F_T; // set deltatime
  
  reg  gps_ppsSyncOut;

  // Assign inputs:
  assign timeServ_setRefF$sD_IN = timeIn;
  assign WILL_FIRE_RL_cpDispatch_T_F_F_F_F_F_F_T = doSetTime;
  assign timeServ_rplTimeControl$D_IN = timeIn[4:0];
  assign WILL_FIRE_RL_cpDispatch_T_F_F_F_F_T_F = doSetControl;
//  assign WILL_FIRE_RL_cpDispatch_T_F_F_F_F_T_T = doSetControl && timeIn[31];
  assign WILL_FIRE_RL_cpDispatch_T_F_F_F_F_T_T = doClear;
  assign WILL_FIRE_RL_cpDispatch_T_F_F_F_F_F_F_F_F_T = doDeltaTime;
  assign gps_ppsSyncIn_x = ppsSyncIn;
  assign deltaTime$D_IN = timeServ_nowInCC$dD_OUT - timeIn;
  
  // Assign outputs:
  //  assign maySetTime = timeServ_setRefF$sFULL_N;
  assign statusOut = { timeServ_ppsLostSticky,
		       timeServ_gpsInSticky,
		       timeServ_ppsInSticky,
		       timeServ_timeSetSticky,
		       timeServ_ppsOKCC$dD_OUT,
		       timeServ_ppsLostCC$dD_OUT,
		       18'h0,
		       timeServ_rollingPPSIn$dD_OUT };
  assign nowCC = timeServ_nowInCC$dD_OUT;
  assign controlOut = timeServ_rplTimeControl;
  assign ticksPerPPSOut = timeServ_refPerPPS$dD_OUT;
  assign nowCC = timeServ_nowInCC$dD_OUT;
  assign nowTC = timeServ_now;
  assign ppsSyncOut = gps_ppsSyncOut;
  // value method gps_ppsSyncOut
  always@(timeServ_ppsOutMode$dD_OUT or
	  timeServ_xo2 or timeServ_ppsDrive or timeServ_ppsExtSync_d2)
  begin
    case (timeServ_ppsOutMode$dD_OUT)
      2'd0: gps_ppsSyncOut = timeServ_ppsDrive;
      2'd1: gps_ppsSyncOut = timeServ_ppsExtSync_d2;
      default: gps_ppsSyncOut =
		   timeServ_ppsOutMode$dD_OUT == 2'd2 && timeServ_xo2;
    endcase
  end
  // submodule timeServ_disableServo
  SyncRegister #(.width(32'd1), .init(1'd0)) timeServ_disableServo(.sCLK(CLK),
								   .dCLK(CLK_time_clk),
								   .sRST(RST_N),
								   .sD_IN(timeServ_disableServo$sD_IN),
								   .sEN(timeServ_disableServo$sEN),
								   .dD_OUT(timeServ_disableServo$dD_OUT),
								   .sRDY(timeServ_disableServo$sRDY));

  // submodule timeServ_nowInCC
  SyncRegister #(.width(32'd64),
		 .init(64'd0)) timeServ_nowInCC(.sCLK(CLK_time_clk),
						.dCLK(CLK),
						.sRST(RST_N_time_rst),
						.sD_IN(timeServ_nowInCC$sD_IN),
						.sEN(timeServ_nowInCC$sEN),
						.dD_OUT(timeServ_nowInCC$dD_OUT),
						.sRDY(timeServ_nowInCC$sRDY));

  // submodule timeServ_ppsDisablePPS
  SyncRegister #(.width(32'd1),
		 .init(1'd0)) timeServ_ppsDisablePPS(.sCLK(CLK),
						     .dCLK(CLK_time_clk),
						     .sRST(RST_N),
						     .sD_IN(timeServ_ppsDisablePPS$sD_IN),
						     .sEN(timeServ_ppsDisablePPS$sEN),
						     .dD_OUT(timeServ_ppsDisablePPS$dD_OUT),
						     .sRDY(timeServ_ppsDisablePPS$sRDY));

  // submodule timeServ_ppsLostCC
  SyncRegister #(.width(32'd1),
		 .init(1'd0)) timeServ_ppsLostCC(.sCLK(CLK_time_clk),
						 .dCLK(CLK),
						 .sRST(RST_N_time_rst),
						 .sD_IN(timeServ_ppsLostCC$sD_IN),
						 .sEN(timeServ_ppsLostCC$sEN),
						 .dD_OUT(timeServ_ppsLostCC$dD_OUT),
						 .sRDY(timeServ_ppsLostCC$sRDY));

  // submodule timeServ_ppsOKCC
  SyncRegister #(.width(32'd1),
		 .init(1'd0)) timeServ_ppsOKCC(.sCLK(CLK_time_clk),
					       .dCLK(CLK),
					       .sRST(RST_N_time_rst),
					       .sD_IN(timeServ_ppsOKCC$sD_IN),
					       .sEN(timeServ_ppsOKCC$sEN),
					       .dD_OUT(timeServ_ppsOKCC$dD_OUT),
					       .sRDY(timeServ_ppsOKCC$sRDY));

  // submodule timeServ_ppsOutMode
  SyncRegister #(.width(32'd2), .init(2'd0)) timeServ_ppsOutMode(.sCLK(CLK),
								 .dCLK(CLK_time_clk),
								 .sRST(RST_N),
								 .sD_IN(timeServ_ppsOutMode$sD_IN),
								 .sEN(timeServ_ppsOutMode$sEN),
								 .dD_OUT(timeServ_ppsOutMode$dD_OUT),
								 .sRDY(timeServ_ppsOutMode$sRDY));

  // submodule timeServ_refPerPPS
  SyncRegister #(.width(32'd28),
		 .init(28'd0)) timeServ_refPerPPS(.sCLK(CLK_time_clk),
						  .dCLK(CLK),
						  .sRST(RST_N_time_rst),
						  .sD_IN(timeServ_refPerPPS$sD_IN),
						  .sEN(timeServ_refPerPPS$sEN),
						  .dD_OUT(timeServ_refPerPPS$dD_OUT),
						  .sRDY(timeServ_refPerPPS$sRDY));

  // submodule timeServ_rollingPPSIn
  SyncRegister #(.width(32'd8),
		 .init(8'd0)) timeServ_rollingPPSIn(.sCLK(CLK_time_clk),
						    .dCLK(CLK),
						    .sRST(RST_N_time_rst),
						    .sD_IN(timeServ_rollingPPSIn$sD_IN),
						    .sEN(timeServ_rollingPPSIn$sEN),
						    .dD_OUT(timeServ_rollingPPSIn$dD_OUT),
						    .sRDY(timeServ_rollingPPSIn$sRDY));

  // submodule timeServ_setRefF
  SyncFIFO #(.dataWidth(32'd64),
	     .depth(32'd2),
	     .indxWidth(32'd1)) timeServ_setRefF(.sCLK(CLK),
						 .dCLK(CLK_time_clk),
						 .sRST(RST_N),
						 .sD_IN(timeServ_setRefF$sD_IN),
						 .sENQ(timeServ_setRefF$sENQ),
						 .dDEQ(timeServ_setRefF$dDEQ),
						 .dD_OUT(timeServ_setRefF$dD_OUT),
						 .sFULL_N(timeServ_setRefF$sFULL_N),
						 .dEMPTY_N(timeServ_setRefF$dEMPTY_N));

  
  // inlined wires
  assign timeServ_jamFrac_1$wget = 1'd1 ;
  assign timeServ_jamFrac_1$whas =
	     timeServ_setRefF$dEMPTY_N && !timeServ_ppsOK ;
  assign timeServ_jamFracVal_1$wget = x__h3700 ;
  assign timeServ_jamFracVal_1$whas = timeServ_jamFrac_1$whas ;
    // register timeServ_delSec
  assign timeServ_delSec$D_IN = timeServ_fracSeconds[49:48] ;
  assign timeServ_delSec$EN = 1'd1 ;

  // register timeServ_delSecond
  assign timeServ_delSecond$D_IN =
	     timeServ_fracSeconds - timeServ_lastSecond ;
  assign timeServ_delSecond$EN =
	     timeServ_ppsExtSync_d2 && !timeServ_ppsExtSyncD &&
	     !timeServ_refFromRise_3_ULE_199800000___d5459 &&
	     timeServ_refFromRise_3_ULT_200200000___d5878 ;

  // register timeServ_fracInc
  assign timeServ_fracInc$D_IN = timeServ_fracInc + x__h4421 ;
  assign timeServ_fracInc$EN =
	     timeServ_ppsExtSync_d2_2_AND_NOT_timeServ_ppsE_ETC___d70 ;

  // register timeServ_fracSeconds
  assign timeServ_fracSeconds$D_IN =
	     timeServ_jamFrac ? timeServ_jamFracVal : x__h4649 ;
  assign timeServ_fracSeconds$EN = 1'd1 ;

  // register timeServ_gpsInSticky
  assign timeServ_gpsInSticky$D_IN = 1'd0 ;
  assign timeServ_gpsInSticky$EN = WILL_FIRE_RL_cpDispatch_T_F_F_F_F_T_T ;

  // register timeServ_jamFrac
  assign timeServ_jamFrac$D_IN = timeServ_jamFrac_1$whas ;
  assign timeServ_jamFrac$EN = 1'd1 ;

  // register timeServ_jamFracVal
  assign timeServ_jamFracVal$D_IN =
	     timeServ_jamFrac_1$whas ? x__h3700 : 50'd0 ;
  assign timeServ_jamFracVal$EN = 1'd1 ;

  // register timeServ_lastSecond
  assign timeServ_lastSecond$D_IN = timeServ_fracSeconds ;
  assign timeServ_lastSecond$EN =
	     timeServ_ppsExtSync_d2 && !timeServ_ppsExtSyncD &&
	     !timeServ_refFromRise_3_ULE_199800000___d5459 &&
	     timeServ_refFromRise_3_ULT_200200000___d5878 ;

  // register timeServ_now
  assign timeServ_now$D_IN =
	     { timeServ_refSecCount, timeServ_fracSeconds[47:16] } ;
  assign timeServ_now$EN = timeServ_nowInCC$sRDY ;

  // register timeServ_ppsDrive
  assign timeServ_ppsDrive$D_IN = timeServ_refPerCount < 28'd180000000 ;
  assign timeServ_ppsDrive$EN = 1'd1 ;

  // register timeServ_ppsEdgeCount
  assign timeServ_ppsEdgeCount$D_IN = timeServ_ppsEdgeCount + 8'd1 ;
  assign timeServ_ppsEdgeCount$EN =
	     timeServ_ppsExtSync_d2 && !timeServ_ppsExtSyncD ;

  // register timeServ_ppsExtCapture
//  assign timeServ_ppsExtCapture$D_IN = 1'b0 ;
//  assign timeServ_ppsExtCapture$EN = 1'b0 ;

  // register timeServ_ppsExtSyncD
  assign timeServ_ppsExtSyncD$D_IN = timeServ_ppsExtSync_d2 ;
  assign timeServ_ppsExtSyncD$EN = !timeServ_ppsDisablePPS$dD_OUT ;

  // register timeServ_ppsExtSync_d1
  assign timeServ_ppsExtSync_d1$D_IN = gps_ppsSyncIn_x ;
  assign timeServ_ppsExtSync_d1$EN = 1'd1 ;

  // register timeServ_ppsExtSync_d2
  assign timeServ_ppsExtSync_d2$D_IN = timeServ_ppsExtSync_d1 ;
  assign timeServ_ppsExtSync_d2$EN = 1'd1 ;

  // register timeServ_ppsInSticky
  assign timeServ_ppsInSticky$D_IN = timeServ_ppsOKCC$dD_OUT ;
  assign timeServ_ppsInSticky$EN =
	     WILL_FIRE_RL_cpDispatch_T_F_F_F_F_T_T ||
	     timeServ_ppsOKCC$dD_OUT ;

  // register timeServ_ppsLost
  assign timeServ_ppsLost$D_IN =
	     timeServ_ppsOK &&
	     timeServ_ppsExtSync_d2_2_AND_NOT_timeServ_ppsE_ETC___d61 ;
  assign timeServ_ppsLost$EN = 1'd1 ;

  // register timeServ_ppsLostSticky
  assign timeServ_ppsLostSticky$D_IN = timeServ_ppsLostCC$dD_OUT ;
  assign timeServ_ppsLostSticky$EN =
	     WILL_FIRE_RL_cpDispatch_T_F_F_F_F_T_T ||
	     timeServ_ppsLostCC$dD_OUT ;

  // register timeServ_ppsOK
  assign timeServ_ppsOK$D_IN =
	     timeServ_ppsExtSync_d2 && !timeServ_ppsExtSyncD &&
	     !timeServ_refFromRise_3_ULE_199800000___d5459 &&
	     timeServ_refFromRise_3_ULT_200200000___d5878 ||
	     timeServ_ppsOK && !timeServ_ppsLost ;
  assign timeServ_ppsOK$EN = 1'd1 ;

  // register timeServ_refFreeCount
  assign timeServ_refFreeCount$D_IN = timeServ_refFreeCount + 28'd1 ;
  assign timeServ_refFreeCount$EN = 1'd1 ;

  // register timeServ_refFreeSamp
  assign timeServ_refFreeSamp$D_IN = timeServ_refFreeCount ;
  assign timeServ_refFreeSamp$EN =
	     timeServ_ppsExtSync_d2 && !timeServ_ppsExtSyncD &&
	     !timeServ_refFromRise_3_ULE_199800000___d5459 &&
	     timeServ_refFromRise_3_ULT_200200000___d5878 ;

  // register timeServ_refFreeSpan
  assign timeServ_refFreeSpan$D_IN =
	     timeServ_refFreeCount - timeServ_refFreeSamp ;
  assign timeServ_refFreeSpan$EN =
	     timeServ_ppsExtSync_d2 && !timeServ_ppsExtSyncD &&
	     !timeServ_refFromRise_3_ULE_199800000___d5459 &&
	     timeServ_refFromRise_3_ULT_200200000___d5878 ;

  // register timeServ_refFromRise
  assign timeServ_refFromRise$D_IN =
	     (timeServ_ppsExtSync_d2 && !timeServ_ppsExtSyncD) ?
	       28'd0 :
	       timeServ_refFromRise + 28'd1 ;
  assign timeServ_refFromRise$EN = 1'd1 ;

  // register timeServ_refPerCount
  assign timeServ_refPerCount$D_IN =
	     IF_timeServ_ppsOK_7_THEN_timeServ_ppsExtSync_d_ETC___d5465 ?
	       28'd0 :
	       timeServ_refPerCount + 28'd1 ;
  assign timeServ_refPerCount$EN = 1'd1 ;

  // register timeServ_refSecCount
  assign timeServ_refSecCount$D_IN =
	     timeServ_setRefF$dEMPTY_N ?
	       timeServ_setRefF$dD_OUT[63:32] :
	       x__h4715 ;
  assign timeServ_refSecCount$EN =
	     timeServ_setRefF$dEMPTY_N ||
	     IF_timeServ_ppsOK_7_THEN_timeServ_ppsExtSync_d_ETC___d5465 ;

  // register timeServ_rplTimeControl
//  assign timeServ_rplTimeControl$D_IN = cpReq[32:28] ; - replaced with assignment above
  assign timeServ_rplTimeControl$EN =
	     WILL_FIRE_RL_cpDispatch_T_F_F_F_F_T_F ||
	     WILL_FIRE_RL_cpDispatch_T_F_F_F_F_T_T ;

  // register timeServ_timeSetSticky
  assign timeServ_timeSetSticky$D_IN =
	     !WILL_FIRE_RL_cpDispatch_T_F_F_F_F_T_T ;
  assign timeServ_timeSetSticky$EN =
	     WILL_FIRE_RL_cpDispatch_T_F_F_F_F_T_T ||
	     WILL_FIRE_RL_cpDispatch_T_F_F_F_F_F_F_T ;

  // register timeServ_xo2
  assign timeServ_xo2$D_IN = !timeServ_xo2 ;
  assign timeServ_xo2$EN = 1'd1 ;

  // register deltaTime
//  assign deltaTime$D_IN = timeServ_nowInCC$dD_OUT - { td, cpReq[59:28] } ; replace with above
  assign deltaTime$EN = WILL_FIRE_RL_cpDispatch_T_F_F_F_F_F_F_F_F_T ;

  // submodule timeServ_disableServo
  assign timeServ_disableServo$sD_IN = timeServ_rplTimeControl[4] ;
  assign timeServ_disableServo$sEN = timeServ_disableServo$sRDY ;

  // submodule timeServ_nowInCC
  assign timeServ_nowInCC$sD_IN =
	     { timeServ_refSecCount, timeServ_fracSeconds[47:16] } ;
  assign timeServ_nowInCC$sEN = timeServ_nowInCC$sRDY ;

  // submodule timeServ_ppsDisablePPS
  assign timeServ_ppsDisablePPS$sD_IN = timeServ_rplTimeControl[2] ;
  assign timeServ_ppsDisablePPS$sEN = timeServ_ppsDisablePPS$sRDY ;

  // submodule timeServ_ppsLostCC
  assign timeServ_ppsLostCC$sD_IN = timeServ_ppsLost ;
  assign timeServ_ppsLostCC$sEN = timeServ_ppsLostCC$sRDY ;

  // submodule timeServ_ppsOKCC
  assign timeServ_ppsOKCC$sD_IN = timeServ_ppsOK ;
  assign timeServ_ppsOKCC$sEN = timeServ_ppsOKCC$sRDY ;

  // submodule timeServ_ppsOutMode
  assign timeServ_ppsOutMode$sD_IN = timeServ_rplTimeControl[1:0] ;
  assign timeServ_ppsOutMode$sEN = timeServ_ppsOutMode$sRDY ;

  // submodule timeServ_refPerPPS
  assign timeServ_refPerPPS$sD_IN = timeServ_refFreeSpan ;
  assign timeServ_refPerPPS$sEN =
	     timeServ_refPerPPS$sRDY && timeServ_ppsExtSync_d2 &&
	     !timeServ_ppsExtSyncD ;

  // submodule timeServ_rollingPPSIn
  assign timeServ_rollingPPSIn$sD_IN = timeServ_ppsEdgeCount ;
  assign timeServ_rollingPPSIn$sEN = timeServ_rollingPPSIn$sRDY ;

  // submodule timeServ_setRefF
//  assign timeServ_setRefF$sD_IN = { td, cpReq[59:28] } ; replaced by assignment above
  assign timeServ_setRefF$sENQ = WILL_FIRE_RL_cpDispatch_T_F_F_F_F_F_F_T ;
  assign timeServ_setRefF$dDEQ = timeServ_setRefF$dEMPTY_N ;

  assign IF_timeServ_ppsOK_7_THEN_timeServ_ppsExtSync_d_ETC___d5465
    =
     timeServ_ppsOK ?
     timeServ_ppsExtSync_d2 && !timeServ_ppsExtSyncD :
     timeServ_delSec != timeServ_fracSeconds[49:48] ;

  assign _281474976710656_MINUS_timeServ_delSecond_BITS__ETC__q2 =
	     _281474976710656_MINUS_timeServ_delSecond__q1[49:28] ;
  assign _281474976710656_MINUS_timeServ_delSecond__q1 =
	     50'h1000000000000 - timeServ_delSecond ;
  assign timeServ_ppsExtSync_d2_2_AND_NOT_timeServ_ppsE_ETC___d61 =
	     timeServ_ppsExtSync_d2 && !timeServ_ppsExtSyncD &&
	     (timeServ_refFromRise_3_ULE_199800000___d5459 ||
	      !timeServ_refFromRise_3_ULT_200200000___d5878) ||
	     timeServ_refFromRise > 28'd200200000 ;
  assign timeServ_ppsExtSync_d2_2_AND_NOT_timeServ_ppsE_ETC___d70 =
	     timeServ_ppsExtSync_d2 && !timeServ_ppsExtSyncD &&
	     !timeServ_refFromRise_3_ULE_199800000___d5459 &&
	     timeServ_refFromRise_3_ULT_200200000___d5878 &&
	     timeServ_ppsOK &&
	     !timeServ_disableServo$dD_OUT ;
  assign timeServ_refFromRise_3_ULE_199800000___d5459 =
	     timeServ_refFromRise <= 28'd199800000 ;
  assign timeServ_refFromRise_3_ULT_200200000___d5878 =
	     timeServ_refFromRise < 28'd200200000 ;
  assign x__h4421 =
	     { {28{_281474976710656_MINUS_timeServ_delSecond_BITS__ETC__q2[21]}},
	       _281474976710656_MINUS_timeServ_delSecond_BITS__ETC__q2 } ;
  assign x__h4649 = timeServ_fracSeconds + timeServ_fracInc ;
  assign x__h4715 = timeServ_refSecCount + 32'd1 ;
//  assign x_f__h4848 = { timeServ_setRefF$dD_OUT[31:0], 16'h0 } ;

  // handling of inlined registers

  always@(posedge CLK)
  begin
    if (RST_N == `BSV_RESET_VALUE)
      begin
	timeServ_gpsInSticky <= `BSV_ASSIGNMENT_DELAY 1'd0;
	timeServ_ppsInSticky <= `BSV_ASSIGNMENT_DELAY 1'd0;
	timeServ_ppsLostSticky <= `BSV_ASSIGNMENT_DELAY 1'd0;
	timeServ_rplTimeControl <= `BSV_ASSIGNMENT_DELAY 5'd0;
	timeServ_timeSetSticky <= `BSV_ASSIGNMENT_DELAY 1'd0;
      end
    else
      begin
	if (timeServ_gpsInSticky$EN)
	  timeServ_gpsInSticky <= `BSV_ASSIGNMENT_DELAY
	      timeServ_gpsInSticky$D_IN;
	if (timeServ_ppsInSticky$EN)
	  timeServ_ppsInSticky <= `BSV_ASSIGNMENT_DELAY
	      timeServ_ppsInSticky$D_IN;
	if (timeServ_ppsLostSticky$EN)
	  timeServ_ppsLostSticky <= `BSV_ASSIGNMENT_DELAY
	      timeServ_ppsLostSticky$D_IN;
	if (timeServ_rplTimeControl$EN)
	  timeServ_rplTimeControl <= `BSV_ASSIGNMENT_DELAY
	      timeServ_rplTimeControl$D_IN;
	if (timeServ_timeSetSticky$EN)
	  timeServ_timeSetSticky <= `BSV_ASSIGNMENT_DELAY
	      timeServ_timeSetSticky$D_IN;
      end
  end
  always@(posedge CLK_time_clk)
  begin
    if (RST_N_time_rst == `BSV_RESET_VALUE)
      begin
        timeServ_delSec <= `BSV_ASSIGNMENT_DELAY 2'd0;
	timeServ_delSecond <= `BSV_ASSIGNMENT_DELAY 50'h1000000000000;
	timeServ_fracInc <= `BSV_ASSIGNMENT_DELAY 50'd1407374;
	timeServ_fracSeconds <= `BSV_ASSIGNMENT_DELAY 50'd0;
	timeServ_jamFrac <= `BSV_ASSIGNMENT_DELAY 1'd0;
	timeServ_jamFracVal <= `BSV_ASSIGNMENT_DELAY 50'd0;
	timeServ_lastSecond <= `BSV_ASSIGNMENT_DELAY 50'd0;
	timeServ_now <= `BSV_ASSIGNMENT_DELAY 64'd0;
	timeServ_ppsDrive <= `BSV_ASSIGNMENT_DELAY 1'd0;
	timeServ_ppsEdgeCount <= `BSV_ASSIGNMENT_DELAY 8'd0;
//	timeServ_ppsExtCapture <= `BSV_ASSIGNMENT_DELAY 1'd0;
	timeServ_ppsExtSyncD <= `BSV_ASSIGNMENT_DELAY 1'd0;
	timeServ_ppsExtSync_d1 <= `BSV_ASSIGNMENT_DELAY 1'd0;
	timeServ_ppsExtSync_d2 <= `BSV_ASSIGNMENT_DELAY 1'd0;
	timeServ_ppsLost <= `BSV_ASSIGNMENT_DELAY 1'd0;
	timeServ_ppsOK <= `BSV_ASSIGNMENT_DELAY 1'd0;
	timeServ_refFreeCount <= `BSV_ASSIGNMENT_DELAY 28'd0;
	timeServ_refFreeSamp <= `BSV_ASSIGNMENT_DELAY 28'd0;
	timeServ_refFreeSpan <= `BSV_ASSIGNMENT_DELAY 28'd0;
	timeServ_refFromRise <= `BSV_ASSIGNMENT_DELAY 28'd0;
	timeServ_refPerCount <= `BSV_ASSIGNMENT_DELAY 28'd0;
	timeServ_refSecCount <= `BSV_ASSIGNMENT_DELAY 32'd0;
	timeServ_xo2 <= `BSV_ASSIGNMENT_DELAY 1'd0;
      end
    else
      begin
        if (timeServ_delSec$EN)
	  timeServ_delSec <= `BSV_ASSIGNMENT_DELAY timeServ_delSec$D_IN;
	if (timeServ_delSecond$EN)
	  timeServ_delSecond <= `BSV_ASSIGNMENT_DELAY timeServ_delSecond$D_IN;
	if (timeServ_fracInc$EN)
	  timeServ_fracInc <= `BSV_ASSIGNMENT_DELAY timeServ_fracInc$D_IN;
	if (timeServ_fracSeconds$EN)
	  timeServ_fracSeconds <= `BSV_ASSIGNMENT_DELAY
	      timeServ_fracSeconds$D_IN;
	if (timeServ_jamFrac$EN)
	  timeServ_jamFrac <= `BSV_ASSIGNMENT_DELAY timeServ_jamFrac$D_IN;
	if (timeServ_jamFracVal$EN)
	  timeServ_jamFracVal <= `BSV_ASSIGNMENT_DELAY
	      timeServ_jamFracVal$D_IN;
	if (timeServ_lastSecond$EN)
	  timeServ_lastSecond <= `BSV_ASSIGNMENT_DELAY
	      timeServ_lastSecond$D_IN;
	if (timeServ_now$EN)
	  timeServ_now <= `BSV_ASSIGNMENT_DELAY timeServ_now$D_IN;
	if (timeServ_ppsDrive$EN)
	  timeServ_ppsDrive <= `BSV_ASSIGNMENT_DELAY timeServ_ppsDrive$D_IN;
	if (timeServ_ppsEdgeCount$EN)
	  timeServ_ppsEdgeCount <= `BSV_ASSIGNMENT_DELAY
	      timeServ_ppsEdgeCount$D_IN;
//	if (timeServ_ppsExtCapture$EN)
//	  timeServ_ppsExtCapture <= `BSV_ASSIGNMENT_DELAY
//	      timeServ_ppsExtCapture$D_IN;
	if (timeServ_ppsExtSyncD$EN)
	  timeServ_ppsExtSyncD <= `BSV_ASSIGNMENT_DELAY
	      timeServ_ppsExtSyncD$D_IN;
	if (timeServ_ppsExtSync_d1$EN)
	  timeServ_ppsExtSync_d1 <= `BSV_ASSIGNMENT_DELAY
	      timeServ_ppsExtSync_d1$D_IN;
	if (timeServ_ppsExtSync_d2$EN)
	  timeServ_ppsExtSync_d2 <= `BSV_ASSIGNMENT_DELAY
	      timeServ_ppsExtSync_d2$D_IN;
	if (timeServ_ppsLost$EN)
	  timeServ_ppsLost <= `BSV_ASSIGNMENT_DELAY timeServ_ppsLost$D_IN;
	if (timeServ_ppsOK$EN)
	  timeServ_ppsOK <= `BSV_ASSIGNMENT_DELAY timeServ_ppsOK$D_IN;
	if (timeServ_refFreeCount$EN)
	  timeServ_refFreeCount <= `BSV_ASSIGNMENT_DELAY
	      timeServ_refFreeCount$D_IN;
	if (timeServ_refFreeSamp$EN)
	  timeServ_refFreeSamp <= `BSV_ASSIGNMENT_DELAY
	      timeServ_refFreeSamp$D_IN;
	if (timeServ_refFreeSpan$EN)
	  timeServ_refFreeSpan <= `BSV_ASSIGNMENT_DELAY
	      timeServ_refFreeSpan$D_IN;
	if (timeServ_refFromRise$EN)
	  timeServ_refFromRise <= `BSV_ASSIGNMENT_DELAY
	      timeServ_refFromRise$D_IN;
	if (timeServ_refPerCount$EN)
	  timeServ_refPerCount <= `BSV_ASSIGNMENT_DELAY
	      timeServ_refPerCount$D_IN;
	if (timeServ_refSecCount$EN)
	  timeServ_refSecCount <= `BSV_ASSIGNMENT_DELAY
	      timeServ_refSecCount$D_IN;
	if (timeServ_xo2$EN)
	  timeServ_xo2 <= `BSV_ASSIGNMENT_DELAY timeServ_xo2$D_IN;
	if (deltaTime$EN) deltaTime <= `BSV_ASSIGNMENT_DELAY deltaTime$D_IN;
      end
  end
  // synopsys translate_off
  `ifdef BSV_NO_INITIAL_BLOCKS
  `else // not BSV_NO_INITIAL_BLOCKS
  initial
  begin
    timeServ_delSec = 2'h2;
    timeServ_delSecond = 50'h2AAAAAAAAAAAA;
    timeServ_fracInc = 50'h2AAAAAAAAAAAA;
    timeServ_fracSeconds = 50'h2AAAAAAAAAAAA;
    timeServ_gpsInSticky = 1'h0;
    timeServ_jamFrac = 1'h0;
    timeServ_jamFracVal = 50'h2AAAAAAAAAAAA;
    timeServ_lastSecond = 50'h2AAAAAAAAAAAA;
    timeServ_now = 64'hAAAAAAAAAAAAAAAA;
    timeServ_ppsDrive = 1'h0;
    timeServ_ppsEdgeCount = 8'hAA;
//    timeServ_ppsExtCapture = 1'h0;
    timeServ_ppsExtSyncD = 1'h0;
    timeServ_ppsExtSync_d1 = 1'h0;
    timeServ_ppsExtSync_d2 = 1'h0;
    timeServ_ppsInSticky = 1'h0;
    timeServ_ppsLost = 1'h0;
    timeServ_ppsLostSticky = 1'h0;
    timeServ_ppsOK = 1'h0;
    timeServ_refFreeCount = 28'hAAAAAAA;
    timeServ_refFreeSamp = 28'hAAAAAAA;
    timeServ_refFreeSpan = 28'hAAAAAAA;
    timeServ_refFromRise = 28'hAAAAAAA;
    timeServ_refPerCount = 28'hAAAAAAA;
    timeServ_refSecCount = 32'hAAAAAAAA;
    timeServ_rplTimeControl = 5'h0A;
    timeServ_timeSetSticky = 1'h0;
    timeServ_xo2 = 1'h0;
  end
  `endif // BSV_NO_INITIAL_BLOCKS
  // synopsys translate_on
endmodule // TimeService
