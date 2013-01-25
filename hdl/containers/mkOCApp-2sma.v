// The container RTL for an app with 3 workers and no adc or memory connections
module mkOCApp4B(RST_N_rst_0,
	       RST_N_rst_1,
	       RST_N_rst_2,
	       RST_N_rst_3,
	       RST_N_rst_4,
	       RST_N_rst_5,
	       RST_N_rst_6,
	       RST_N_rst_7,
	       CLK,
	       RST_N,
	       wci_s_0_MCmd,
	       wci_s_0_MAddrSpace,
	       wci_s_0_MByteEn,
	       wci_s_0_MAddr,
	       wci_s_0_MData,
	       wci_s_0_SResp,
	       wci_s_0_SData,
	       wci_s_0_SThreadBusy,
	       wci_s_0_SFlag,
	       wci_s_0_MFlag,
	       wci_s_1_MCmd,
	       wci_s_1_MAddrSpace,
	       wci_s_1_MByteEn,
	       wci_s_1_MAddr,
	       wci_s_1_MData,
	       wci_s_1_SResp,
	       wci_s_1_SData,
	       wci_s_1_SThreadBusy,
	       wci_s_1_SFlag,
	       wci_s_1_MFlag,
	       wci_s_2_MCmd,
	       wci_s_2_MAddrSpace,
	       wci_s_2_MByteEn,
	       wci_s_2_MAddr,
	       wci_s_2_MData,
	       wci_s_2_SResp,
	       wci_s_2_SData,
	       wci_s_2_SThreadBusy,
	       wci_s_2_SFlag,
	       wci_s_2_MFlag,
	       wci_s_3_MCmd,
	       wci_s_3_MAddrSpace,
	       wci_s_3_MByteEn,
	       wci_s_3_MAddr,
	       wci_s_3_MData,
	       wci_s_3_SResp,
	       wci_s_3_SData,
	       wci_s_3_SThreadBusy,
	       wci_s_3_SFlag,
	       wci_s_3_MFlag,
	       wci_s_4_MCmd,
	       wci_s_4_MAddrSpace,
	       wci_s_4_MByteEn,
	       wci_s_4_MAddr,
	       wci_s_4_MData,
	       wci_s_4_SResp,
	       wci_s_4_SData,
	       wci_s_4_SThreadBusy,
	       wci_s_4_SFlag,
	       wci_s_4_MFlag,
	       wci_s_5_MCmd,
	       wci_s_5_MAddrSpace,
	       wci_s_5_MByteEn,
	       wci_s_5_MAddr,
	       wci_s_5_MData,
	       wci_s_5_SResp,
	       wci_s_5_SData,
	       wci_s_5_SThreadBusy,
	       wci_s_5_SFlag,
	       wci_s_5_MFlag,
	       wci_s_6_MCmd,
	       wci_s_6_MAddrSpace,
	       wci_s_6_MByteEn,
	       wci_s_6_MAddr,
	       wci_s_6_MData,
	       wci_s_6_SResp,
	       wci_s_6_SData,
	       wci_s_6_SThreadBusy,
	       wci_s_6_SFlag,
	       wci_s_6_MFlag,
	       wci_s_7_MCmd,
	       wci_s_7_MAddrSpace,
	       wci_s_7_MByteEn,
	       wci_s_7_MAddr,
	       wci_s_7_MData,
	       wci_s_7_SResp,
	       wci_s_7_SData,
	       wci_s_7_SThreadBusy,
	       wci_s_7_SFlag,
	       wci_s_7_MFlag,
		 wti_s_0_MCmd,
		 wti_s_0_MData,
		 wti_s_0_SThreadBusy,
		 wti_s_0_SReset_n,
		 wti_s_1_MCmd,
		 wti_s_1_MData,
		 wti_s_1_SThreadBusy,
		 wti_s_1_SReset_n,
		 wti_s_2_MCmd,
		 wti_s_2_MData,
		 wti_s_2_SThreadBusy,
		 wti_s_2_SReset_n,
	       wmiM0_MCmd,
	       wmiM0_MReqLast,
	       wmiM0_MReqInfo,
	       wmiM0_MAddrSpace,
	       wmiM0_MAddr,
	       wmiM0_MBurstLength,
	       wmiM0_MDataValid,
	       wmiM0_MDataLast,
	       wmiM0_MData,
	       wmiM0_MDataByteEn,
	       wmiM0_SResp,
	       wmiM0_SData,
	       wmiM0_SThreadBusy,
	       wmiM0_SDataThreadBusy,
	       wmiM0_SRespLast,
	       wmiM0_SFlag,
	       wmiM0_MFlag,
	       wmiM0_MReset_n,
	       wmiM0_SReset_n,
	       wmiM1_MCmd,
	       wmiM1_MReqLast,
	       wmiM1_MReqInfo,
	       wmiM1_MAddrSpace,
	       wmiM1_MAddr,
	       wmiM1_MBurstLength,
	       wmiM1_MDataValid,
	       wmiM1_MDataLast,
	       wmiM1_MData,
	       wmiM1_MDataByteEn,
	       wmiM1_SResp,
	       wmiM1_SData,
	       wmiM1_SThreadBusy,
	       wmiM1_SDataThreadBusy,
	       wmiM1_SRespLast,
	       wmiM1_SFlag,
	       wmiM1_MFlag,
	       wmiM1_MReset_n,
	       wmiM1_SReset_n,
	       wmemiM0_MCmd,
	       wmemiM0_MReqLast,
	       wmemiM0_MAddr,
	       wmemiM0_MBurstLength,
	       wmemiM0_MDataValid,
	       wmemiM0_MDataLast,
	       wmemiM0_MData,
	       wmemiM0_MDataByteEn,
	       wmemiM0_SResp,
	       wmemiM0_SRespLast,
	       wmemiM0_SData,
	       wmemiM0_SCmdAccept,
	       wmemiM0_SDataAccept,
	       wmemiM0_MReset_n,
	       wsi_s_adc_MCmd,
	       wsi_s_adc_MReqLast,
	       wsi_s_adc_MBurstPrecise,
	       wsi_s_adc_MBurstLength,
	       wsi_s_adc_MData,
	       wsi_s_adc_MByteEn,
	       wsi_s_adc_MReqInfo,
	       wsi_s_adc_SThreadBusy,
	       wsi_s_adc_SReset_n,
	       wsi_s_adc_MReset_n,
	       wsi_m_dac_MCmd,
	       wsi_m_dac_MReqLast,
	       wsi_m_dac_MBurstPrecise,
	       wsi_m_dac_MBurstLength,
	       wsi_m_dac_MData,
	       wsi_m_dac_MByteEn,
	       wsi_m_dac_MReqInfo,
	       wsi_m_dac_SThreadBusy,
	       wsi_m_dac_MReset_n,
	       wsi_m_dac_SReset_n,
	       uuid,
	       rom_en,
	       rom_addr,
	       rom_data);
  parameter [0 : 0] hasDebugLogic = 1'b0;
  input  RST_N_rst_0;
  input  RST_N_rst_1;
  input  RST_N_rst_2;
  input  RST_N_rst_3;
  input  RST_N_rst_4;
  input  RST_N_rst_5;
  input  RST_N_rst_6;
  input  RST_N_rst_7;
  input  CLK;
  input  RST_N;

  // action method wci_s_0_mCmd
  input  [2 : 0] wci_s_0_MCmd;

  // action method wci_s_0_mAddrSpace
  input  wci_s_0_MAddrSpace;

  // action method wci_s_0_mByteEn
  input  [3 : 0] wci_s_0_MByteEn;

  // action method wci_s_0_mAddr
  input  [31 : 0] wci_s_0_MAddr;

  // action method wci_s_0_mData
  input  [31 : 0] wci_s_0_MData;

  // value method wci_s_0_sResp
  output [1 : 0] wci_s_0_SResp;

  // value method wci_s_0_sData
  output [31 : 0] wci_s_0_SData;

  // value method wci_s_0_sThreadBusy
  output wci_s_0_SThreadBusy;

  // value method wci_s_0_sFlag
  output [1 : 0] wci_s_0_SFlag;

  // action method wci_s_0_mFlag
  input  [1 : 0] wci_s_0_MFlag;

  // action method wci_s_1_mCmd
  input  [2 : 0] wci_s_1_MCmd;

  // action method wci_s_1_mAddrSpace
  input  wci_s_1_MAddrSpace;

  // action method wci_s_1_mByteEn
  input  [3 : 0] wci_s_1_MByteEn;

  // action method wci_s_1_mAddr
  input  [31 : 0] wci_s_1_MAddr;

  // action method wci_s_1_mData
  input  [31 : 0] wci_s_1_MData;

  // value method wci_s_1_sResp
  output [1 : 0] wci_s_1_SResp;

  // value method wci_s_1_sData
  output [31 : 0] wci_s_1_SData;

  // value method wci_s_1_sThreadBusy
  output wci_s_1_SThreadBusy;

  // value method wci_s_1_sFlag
  output [1 : 0] wci_s_1_SFlag;

  // action method wci_s_1_mFlag
  input  [1 : 0] wci_s_1_MFlag;

  // action method wci_s_2_mCmd
  input  [2 : 0] wci_s_2_MCmd;

  // action method wci_s_2_mAddrSpace
  input  wci_s_2_MAddrSpace;

  // action method wci_s_2_mByteEn
  input  [3 : 0] wci_s_2_MByteEn;

  // action method wci_s_2_mAddr
  input  [31 : 0] wci_s_2_MAddr;

  // action method wci_s_2_mData
  input  [31 : 0] wci_s_2_MData;

  // value method wci_s_2_sResp
  output [1 : 0] wci_s_2_SResp;

  // value method wci_s_2_sData
  output [31 : 0] wci_s_2_SData;

  // value method wci_s_2_sThreadBusy
  output wci_s_2_SThreadBusy;

  // value method wci_s_2_sFlag
  output [1 : 0] wci_s_2_SFlag;

  // action method wci_s_2_mFlag
  input  [1 : 0] wci_s_2_MFlag;

  // action method wci_s_3_mCmd
  input  [2 : 0] wci_s_3_MCmd;

  // action method wci_s_3_mAddrSpace
  input  wci_s_3_MAddrSpace;

  // action method wci_s_3_mByteEn
  input  [3 : 0] wci_s_3_MByteEn;

  // action method wci_s_3_mAddr
  input  [31 : 0] wci_s_3_MAddr;

  // action method wci_s_3_mData
  input  [31 : 0] wci_s_3_MData;

  // value method wci_s_3_sResp
  output [1 : 0] wci_s_3_SResp;

  // value method wci_s_3_sData
  output [31 : 0] wci_s_3_SData;

  // value method wci_s_3_sThreadBusy
  output wci_s_3_SThreadBusy;

  // value method wci_s_3_sFlag
  output [1 : 0] wci_s_3_SFlag;

  // action method wci_s_3_mFlag
  input  [1 : 0] wci_s_3_MFlag;

  // action method wci_s_4_mCmd
  input  [2 : 0] wci_s_4_MCmd;

  // action method wci_s_4_mAddrSpace
  input  wci_s_4_MAddrSpace;

  // action method wci_s_4_mByteEn
  input  [3 : 0] wci_s_4_MByteEn;

  // action method wci_s_4_mAddr
  input  [31 : 0] wci_s_4_MAddr;

  // action method wci_s_4_mData
  input  [31 : 0] wci_s_4_MData;

  // value method wci_s_4_sResp
  output [1 : 0] wci_s_4_SResp;

  // value method wci_s_4_sData
  output [31 : 0] wci_s_4_SData;

  // value method wci_s_4_sThreadBusy
  output wci_s_4_SThreadBusy;

  // value method wci_s_4_sFlag
  output [1 : 0] wci_s_4_SFlag;

  // action method wci_s_4_mFlag
  input  [1 : 0] wci_s_4_MFlag;

  // action method wci_s_5_mCmd
  input  [2 : 0] wci_s_5_MCmd;

  // action method wci_s_5_mAddrSpace
  input  wci_s_5_MAddrSpace;

  // action method wci_s_5_mByteEn
  input  [3 : 0] wci_s_5_MByteEn;

  // action method wci_s_5_mAddr
  input  [31 : 0] wci_s_5_MAddr;

  // action method wci_s_5_mData
  input  [31 : 0] wci_s_5_MData;

  // value method wci_s_5_sResp
  output [1 : 0] wci_s_5_SResp;

  // value method wci_s_5_sData
  output [31 : 0] wci_s_5_SData;

  // value method wci_s_5_sThreadBusy
  output wci_s_5_SThreadBusy;

  // value method wci_s_5_sFlag
  output [1 : 0] wci_s_5_SFlag;

  // action method wci_s_5_mFlag
  input  [1 : 0] wci_s_5_MFlag;

  // action method wci_s_6_mCmd
  input  [2 : 0] wci_s_6_MCmd;

  // action method wci_s_6_mAddrSpace
  input  wci_s_6_MAddrSpace;

  // action method wci_s_6_mByteEn
  input  [3 : 0] wci_s_6_MByteEn;

  // action method wci_s_6_mAddr
  input  [31 : 0] wci_s_6_MAddr;

  // action method wci_s_6_mData
  input  [31 : 0] wci_s_6_MData;

  // value method wci_s_6_sResp
  output [1 : 0] wci_s_6_SResp;

  // value method wci_s_6_sData
  output [31 : 0] wci_s_6_SData;

  // value method wci_s_6_sThreadBusy
  output wci_s_6_SThreadBusy;

  // value method wci_s_6_sFlag
  output [1 : 0] wci_s_6_SFlag;

  // action method wci_s_6_mFlag
  input  [1 : 0] wci_s_6_MFlag;

  // action method wci_s_7_mCmd
  input  [2 : 0] wci_s_7_MCmd;

  // action method wci_s_7_mAddrSpace
  input  wci_s_7_MAddrSpace;

  // action method wci_s_7_mByteEn
  input  [3 : 0] wci_s_7_MByteEn;

  // action method wci_s_7_mAddr
  input  [31 : 0] wci_s_7_MAddr;

  // action method wci_s_7_mData
  input  [31 : 0] wci_s_7_MData;

  // value method wci_s_7_sResp
  output [1 : 0] wci_s_7_SResp;

  // value method wci_s_7_sData
  output [31 : 0] wci_s_7_SData;

  // value method wci_s_7_sThreadBusy
  output wci_s_7_SThreadBusy;

  // value method wci_s_7_sFlag
  output [1 : 0] wci_s_7_SFlag;

  // action method wci_s_7_mFlag
  input  [1 : 0] wci_s_7_MFlag;

  // action method wti_s_0_mCmd
  input  [2 : 0] wti_s_0_MCmd;

  // action method wti_s_0_mData
  input  [63 : 0] wti_s_0_MData;

  // value method wti_s_0_sThreadBusy
  output wti_s_0_SThreadBusy;

  // value method wti_s_0_sReset_n
  output wti_s_0_SReset_n;

  // action method wti_s_1_mCmd
  input  [2 : 0] wti_s_1_MCmd;

  // action method wti_s_1_mData
  input  [63 : 0] wti_s_1_MData;

  // value method wti_s_1_sThreadBusy
  output wti_s_1_SThreadBusy;

  // value method wti_s_1_sReset_n
  output wti_s_1_SReset_n;

  // action method wti_s_2_mCmd
  input  [2 : 0] wti_s_2_MCmd;

  // action method wti_s_2_mData
  input  [63 : 0] wti_s_2_MData;

  // value method wti_s_2_sThreadBusy
  output wti_s_2_SThreadBusy;

  // value method wti_s_2_sReset_n
  output wti_s_2_SReset_n;

  // value method wmiM0_mCmd
  output [2 : 0] wmiM0_MCmd;

  // value method wmiM0_mReqLast
  output wmiM0_MReqLast;

  // value method wmiM0_mReqInfo
  output wmiM0_MReqInfo;

  // value method wmiM0_mAddrSpace
  output wmiM0_MAddrSpace;

  // value method wmiM0_mAddr
  output [13 : 0] wmiM0_MAddr;

  // value method wmiM0_mBurstLength
  output [11 : 0] wmiM0_MBurstLength;

  // value method wmiM0_mDataValid
  output wmiM0_MDataValid;

  // value method wmiM0_mDataLast
  output wmiM0_MDataLast;

  // value method wmiM0_mData
  output [31 : 0] wmiM0_MData;

  // value method wmiM0_mDataInfo

  // value method wmiM0_mDataByteEn
  output [3 : 0] wmiM0_MDataByteEn;

  // action method wmiM0_sResp
  input  [1 : 0] wmiM0_SResp;

  // action method wmiM0_sData
  input  [31 : 0] wmiM0_SData;

  // action method wmiM0_sThreadBusy
  input  wmiM0_SThreadBusy;

  // action method wmiM0_sDataThreadBusy
  input  wmiM0_SDataThreadBusy;

  // action method wmiM0_sRespLast
  input  wmiM0_SRespLast;

  // action method wmiM0_sFlag
  input  [31 : 0] wmiM0_SFlag;

  // value method wmiM0_mFlag
  output [31 : 0] wmiM0_MFlag;

  // value method wmiM0_mReset_n
  output wmiM0_MReset_n;

  // action method wmiM0_sReset_n
  input  wmiM0_SReset_n;

  // value method wmiM1_mCmd
  output [2 : 0] wmiM1_MCmd;

  // value method wmiM1_mReqLast
  output wmiM1_MReqLast;

  // value method wmiM1_mReqInfo
  output wmiM1_MReqInfo;

  // value method wmiM1_mAddrSpace
  output wmiM1_MAddrSpace;

  // value method wmiM1_mAddr
  output [13 : 0] wmiM1_MAddr;

  // value method wmiM1_mBurstLength
  output [11 : 0] wmiM1_MBurstLength;

  // value method wmiM1_mDataValid
  output wmiM1_MDataValid;

  // value method wmiM1_mDataLast
  output wmiM1_MDataLast;

  // value method wmiM1_mData
  output [31 : 0] wmiM1_MData;

  // value method wmiM1_mDataInfo

  // value method wmiM1_mDataByteEn
  output [3 : 0] wmiM1_MDataByteEn;

  // action method wmiM1_sResp
  input  [1 : 0] wmiM1_SResp;

  // action method wmiM1_sData
  input  [31 : 0] wmiM1_SData;

  // action method wmiM1_sThreadBusy
  input  wmiM1_SThreadBusy;

  // action method wmiM1_sDataThreadBusy
  input  wmiM1_SDataThreadBusy;

  // action method wmiM1_sRespLast
  input  wmiM1_SRespLast;

  // action method wmiM1_sFlag
  input  [31 : 0] wmiM1_SFlag;

  // value method wmiM1_mFlag
  output [31 : 0] wmiM1_MFlag;

  // value method wmiM1_mReset_n
  output wmiM1_MReset_n;

  // action method wmiM1_sReset_n
  input  wmiM1_SReset_n;

  // value method wmemiM0_mCmd
  output [2 : 0] wmemiM0_MCmd;

  // value method wmemiM0_mReqLast
  output wmemiM0_MReqLast;

  // value method wmemiM0_mAddr
  output [35 : 0] wmemiM0_MAddr;

  // value method wmemiM0_mBurstLength
  output [11 : 0] wmemiM0_MBurstLength;

  // value method wmemiM0_mDataValid
  output wmemiM0_MDataValid;

  // value method wmemiM0_mDataLast
  output wmemiM0_MDataLast;

  // value method wmemiM0_mData
  output [127 : 0] wmemiM0_MData;

  // value method wmemiM0_mDataByteEn
  output [15 : 0] wmemiM0_MDataByteEn;

  // action method wmemiM0_sResp
  input  [1 : 0] wmemiM0_SResp;

  // action method wmemiM0_sRespLast
  input  wmemiM0_SRespLast;

  // action method wmemiM0_sData
  input  [127 : 0] wmemiM0_SData;

  // action method wmemiM0_sCmdAccept
  input  wmemiM0_SCmdAccept;

  // action method wmemiM0_sDataAccept
  input  wmemiM0_SDataAccept;

  // value method wmemiM0_mReset_n
  output wmemiM0_MReset_n;

  // action method wsi_s_adc_mCmd
  input  [2 : 0] wsi_s_adc_MCmd;

  // action method wsi_s_adc_mReqLast
  input  wsi_s_adc_MReqLast;

  // action method wsi_s_adc_mBurstPrecise
  input  wsi_s_adc_MBurstPrecise;

  // action method wsi_s_adc_mBurstLength
  input  [11 : 0] wsi_s_adc_MBurstLength;

  // action method wsi_s_adc_mData
  input  [31 : 0] wsi_s_adc_MData;

  // action method wsi_s_adc_mByteEn
  input  [3 : 0] wsi_s_adc_MByteEn;

  // action method wsi_s_adc_mReqInfo
  input  [7 : 0] wsi_s_adc_MReqInfo;

  // action method wsi_s_adc_mDataInfo

  // value method wsi_s_adc_sThreadBusy
  output wsi_s_adc_SThreadBusy;

  // value method wsi_s_adc_sReset_n
  output wsi_s_adc_SReset_n;

  // action method wsi_s_adc_mReset_n
  input  wsi_s_adc_MReset_n;

  // value method wsi_m_dac_mCmd
  output [2 : 0] wsi_m_dac_MCmd;

  // value method wsi_m_dac_mReqLast
  output wsi_m_dac_MReqLast;

  // value method wsi_m_dac_mBurstPrecise
  output wsi_m_dac_MBurstPrecise;

  // value method wsi_m_dac_mBurstLength
  output [11 : 0] wsi_m_dac_MBurstLength;

  // value method wsi_m_dac_mData
  output [31 : 0] wsi_m_dac_MData;

  // value method wsi_m_dac_mByteEn
  output [3 : 0] wsi_m_dac_MByteEn;

  // value method wsi_m_dac_mReqInfo
  output [7 : 0] wsi_m_dac_MReqInfo;

  // value method wsi_m_dac_mDataInfo

  // action method wsi_m_dac_sThreadBusy
  input  wsi_m_dac_SThreadBusy;

  // value method wsi_m_dac_mReset_n
  output wsi_m_dac_MReset_n;

  // action method wsi_m_dac_sReset_n
  input  wsi_m_dac_SReset_n;

  output [511 : 0] uuid;

  input 	   rom_en;
  input  [9:0] 	   rom_addr;
  output [31:0]    rom_data;

  wire [511 : 0]   myUUID;
  wire [511 : 0]   uuid = myUUID;
  
  // signals for module outputs
  wire [127 : 0] wmemiM0_MData;
  wire [35 : 0] wmemiM0_MAddr;
  wire [31 : 0] wci_s_0_SData,
		wci_s_1_SData,
		wci_s_2_SData,
		wci_s_3_SData,
		wci_s_4_SData,
		wci_s_5_SData,
		wci_s_6_SData,
		wci_s_7_SData,
		wmiM0_MData,
		wmiM0_MFlag,
		wmiM1_MData,
		wmiM1_MFlag,
		wsi_m_dac_MData;
  wire [15 : 0] wmemiM0_MDataByteEn;
  wire [13 : 0] wmiM0_MAddr, wmiM1_MAddr;
  wire [11 : 0] wmemiM0_MBurstLength,
		wmiM0_MBurstLength,
		wmiM1_MBurstLength,
		wsi_m_dac_MBurstLength;
  wire [7 : 0] wsi_m_dac_MReqInfo;
  wire [3 : 0] wmiM0_MDataByteEn, wmiM1_MDataByteEn, wsi_m_dac_MByteEn;
  wire [2 : 0] wmemiM0_MCmd, wmiM0_MCmd, wmiM1_MCmd, wsi_m_dac_MCmd;
  wire [1 : 0] wci_s_0_SFlag,
	       wci_s_0_SResp,
	       wci_s_1_SFlag,
	       wci_s_1_SResp,
	       wci_s_2_SFlag,
	       wci_s_2_SResp,
	       wci_s_3_SFlag,
	       wci_s_3_SResp,
	       wci_s_4_SFlag,
	       wci_s_4_SResp,
	       wci_s_5_SFlag,
	       wci_s_5_SResp,
	       wci_s_6_SFlag,
	       wci_s_6_SResp,
	       wci_s_7_SFlag,
	       wci_s_7_SResp;
  wire wci_s_0_SThreadBusy,
       wci_s_1_SThreadBusy,
       wci_s_2_SThreadBusy,
       wci_s_3_SThreadBusy,
       wci_s_4_SThreadBusy,
       wci_s_5_SThreadBusy,
       wci_s_6_SThreadBusy,
       wci_s_7_SThreadBusy,
       wmemiM0_MDataLast,
       wmemiM0_MDataValid,
       wmemiM0_MReqLast,
       wmemiM0_MReset_n,
       wmiM0_MAddrSpace,
       wmiM0_MDataLast,
       wmiM0_MDataValid,
       wmiM0_MReqInfo,
       wmiM0_MReqLast,
       wmiM0_MReset_n,
       wmiM1_MAddrSpace,
       wmiM1_MDataLast,
       wmiM1_MDataValid,
       wmiM1_MReqInfo,
       wmiM1_MReqLast,
       wmiM1_MReset_n,
       wsi_m_dac_MBurstPrecise,
       wsi_m_dac_MReqLast,
       wsi_m_dac_MReset_n,
       wsi_s_adc_SReset_n,
       wsi_s_adc_SThreadBusy;
  mkUUID id(.uuid(myUUID));
wire [11: 0] sma0_out_2_app_in_MBurstLength;
wire [ 3: 0] sma0_out_2_app_in_MByteEn;
wire [ 2: 0] sma0_out_2_app_in_MCmd;
wire [31: 0] sma0_out_2_app_in_MData;
wire         sma0_out_2_app_in_MBurstPrecise;
wire [ 7: 0] sma0_out_2_app_in_MReqInfo;
wire         sma0_out_2_app_in_MReqLast;
wire         sma0_out_2_app_in_MReset_n;
wire         app_in_2_sma0_out_SReset_n;
wire [ 0: 0] app_in_2_sma0_out_SThreadBusy;
wire [11: 0] app_out_2_sma1_in_MBurstLength;
wire [ 3: 0] app_out_2_sma1_in_MByteEn;
wire [ 2: 0] app_out_2_sma1_in_MCmd;
wire [31: 0] app_out_2_sma1_in_MData;
wire         app_out_2_sma1_in_MBurstPrecise;
wire [ 7: 0] app_out_2_sma1_in_MReqInfo;
wire         app_out_2_sma1_in_MReqLast;
wire         app_out_2_sma1_in_MReset_n;
wire         sma1_in_2_app_out_SReset_n;
wire [ 0: 0] sma1_in_2_app_out_SThreadBusy;
sma sma0 (
  .wciS0_Clk(CLK),
  .wciS0_MAddr(wci_s_2_MAddr),
  .wciS0_MAddrSpace(wci_s_2_MAddrSpace),
  .wciS0_MByteEn(wci_s_2_MByteEn),
  .wciS0_MCmd(wci_s_2_MCmd),
  .wciS0_MData(wci_s_2_MData),
  .wciS0_MFlag(wci_s_2_MFlag),
  .wciS0_MReset_n(RST_N_rst_2),
  .wciS0_SData(wci_s_2_SData),
  .wciS0_SFlag(wci_s_2_SFlag),
  .wciS0_SResp(wci_s_2_SResp),
  .wciS0_SThreadBusy(wci_s_2_SThreadBusy),
  .wsiS0_MBurstLength(12'b0),
  .wsiS0_MByteEn(4'b0),
  .wsiS0_MCmd(3'b0),
  .wsiS0_MData(32'b0),
  .wsiS0_MBurstPrecise(1'b0),
  .wsiS0_MReqInfo(8'b0),
  .wsiS0_MReqLast(1'b0),
  .wsiS0_MReset_n(1'b0),
  .wsiS0_SReset_n(),
  .wsiS0_SThreadBusy(),
  .wsiM0_MBurstLength(sma0_out_2_app_in_MBurstLength),
  .wsiM0_MByteEn(sma0_out_2_app_in_MByteEn),
  .wsiM0_MCmd(sma0_out_2_app_in_MCmd),
  .wsiM0_MData(sma0_out_2_app_in_MData),
  .wsiM0_MBurstPrecise(sma0_out_2_app_in_MBurstPrecise),
  .wsiM0_MReqInfo(sma0_out_2_app_in_MReqInfo),
  .wsiM0_MReqLast(sma0_out_2_app_in_MReqLast),
  .wsiM0_MReset_n(sma0_out_2_app_in_MReset_n),
  .wsiM0_SReset_n(app_in_2_sma0_out_SReset_n),
  .wsiM0_SThreadBusy(app_in_2_sma0_out_SThreadBusy),
  .wmiM0_MAddr(wmiM0_MAddr),
  .wmiM0_MAddrSpace(wmiM0_MAddrSpace),
  .wmiM0_MBurstLength(wmiM0_MBurstLength),
  .wmiM0_MCmd(wmiM0_MCmd),
  .wmiM0_MData(wmiM0_MData),
  .wmiM0_MDataByteEn(wmiM0_MDataByteEn),
  .wmiM0_MDataLast(wmiM0_MDataLast),
  .wmiM0_MDataValid(wmiM0_MDataValid),
  .wmiM0_MFlag(wmiM0_MFlag),
  .wmiM0_MReqInfo(wmiM0_MReqInfo),
  .wmiM0_MReqLast(wmiM0_MReqLast),
  .wmiM0_MReset_n(wmiM0_MReset_n),
  .wmiM0_SData(wmiM0_SData),
  .wmiM0_SDataThreadBusy(wmiM0_SDataThreadBusy),
  .wmiM0_SFlag(wmiM0_SFlag),
  .wmiM0_SResp(wmiM0_SResp),
  .wmiM0_SRespLast(wmiM0_SRespLast),
  .wmiM0_SReset_n(wmiM0_SReset_n),
  .wmiM0_SThreadBusy(wmiM0_SThreadBusy));
sma sma1 (
  .wciS0_Clk(CLK),
  .wciS0_MAddr(wci_s_4_MAddr),
  .wciS0_MAddrSpace(wci_s_4_MAddrSpace),
  .wciS0_MByteEn(wci_s_4_MByteEn),
  .wciS0_MCmd(wci_s_4_MCmd),
  .wciS0_MData(wci_s_4_MData),
  .wciS0_MFlag(wci_s_4_MFlag),
  .wciS0_MReset_n(RST_N_rst_4),
  .wciS0_SData(wci_s_4_SData),
  .wciS0_SFlag(wci_s_4_SFlag),
  .wciS0_SResp(wci_s_4_SResp),
  .wciS0_SThreadBusy(wci_s_4_SThreadBusy),
  .wsiS0_MBurstLength(app_out_2_sma1_in_MBurstLength),
  .wsiS0_MByteEn(app_out_2_sma1_in_MByteEn),
  .wsiS0_MCmd(app_out_2_sma1_in_MCmd),
  .wsiS0_MData(app_out_2_sma1_in_MData),
  .wsiS0_MBurstPrecise(app_out_2_sma1_in_MBurstPrecise),
  .wsiS0_MReqInfo(app_out_2_sma1_in_MReqInfo),
  .wsiS0_MReqLast(app_out_2_sma1_in_MReqLast),
  .wsiS0_MReset_n(app_out_2_sma1_in_MReset_n),
  .wsiS0_SReset_n(sma1_in_2_app_out_SReset_n),
  .wsiS0_SThreadBusy(sma1_in_2_app_out_SThreadBusy),
  .wsiM0_SReset_n(1'b0),
  .wsiM0_SThreadBusy(1'b0),
  .wsiM0_MBurstLength(),
  .wsiM0_MByteEn(),
  .wsiM0_MCmd(),
  .wsiM0_MData(),
  .wsiM0_MBurstPrecise(),
  .wsiM0_MReqInfo(),
  .wsiM0_MReqLast(),
  .wsiM0_MReset_n(),
  .wmiM0_MAddr(wmiM1_MAddr),
  .wmiM0_MAddrSpace(wmiM1_MAddrSpace),
  .wmiM0_MBurstLength(wmiM1_MBurstLength),
  .wmiM0_MCmd(wmiM1_MCmd),
  .wmiM0_MData(wmiM1_MData),
  .wmiM0_MDataByteEn(wmiM1_MDataByteEn),
  .wmiM0_MDataLast(wmiM1_MDataLast),
  .wmiM0_MDataValid(wmiM1_MDataValid),
  .wmiM0_MFlag(wmiM1_MFlag),
  .wmiM0_MReqInfo(wmiM1_MReqInfo),
  .wmiM0_MReqLast(wmiM1_MReqLast),
  .wmiM0_MReset_n(wmiM1_MReset_n),
  .wmiM0_SData(wmiM1_SData),
  .wmiM0_SDataThreadBusy(wmiM1_SDataThreadBusy),
  .wmiM0_SFlag(wmiM1_SFlag),
  .wmiM0_SResp(wmiM1_SResp),
  .wmiM0_SRespLast(wmiM1_SRespLast),
  .wmiM0_SReset_n(wmiM1_SReset_n),
  .wmiM0_SThreadBusy(wmiM1_SThreadBusy));

  // Instantiate the wip-compliant app
  ocpi_app app(
  .wci_MReset_n(RST_N_rst_3),
  .wci_Clk(CLK),
  .wci_MAddr(wci_s_3_MAddr),
  .wci_MAddrSpace(wci_s_3_MAddrSpace),
  .wci_MByteEn(wci_s_3_MByteEn),
  .wci_MCmd(wci_s_3_MCmd),
  .wci_MData(wci_s_3_MData),
  .wci_MFlag(wci_s_3_MFlag),
  .wci_SResp(wci_s_3_SResp),
  .wci_SData(wci_s_3_SData),
  .wci_SThreadBusy(wci_s_3_SThreadBusy),
  .wci_SFlag(wci_s_3_SFlag),
  .in_MBurstLength(sma0_out_2_app_in_MBurstLength),
  .in_MByteEn(sma0_out_2_app_in_MByteEn),
  .in_MCmd(sma0_out_2_app_in_MCmd),
  .in_MData(sma0_out_2_app_in_MData),
  .in_MBurstPrecise(sma0_out_2_app_in_MBurstPrecise),
  .in_MReqInfo(sma0_out_2_app_in_MReqInfo),
  .in_MReqLast(sma0_out_2_app_in_MReqLast),
  .in_MReset_n(sma0_out_2_app_in_MReset_n),
  .in_SReset_n(app_in_2_sma0_out_SReset_n),
  .in_SThreadBusy(app_in_2_sma0_out_SThreadBusy),
  .out_MBurstLength(app_out_2_sma1_in_MBurstLength),
  .out_MByteEn(app_out_2_sma1_in_MByteEn),
  .out_MCmd(app_out_2_sma1_in_MCmd),
  .out_MData(app_out_2_sma1_in_MData),
  .out_MBurstPrecise(app_out_2_sma1_in_MBurstPrecise),
  .out_MReqInfo(app_out_2_sma1_in_MReqInfo),
  .out_MReqLast(app_out_2_sma1_in_MReqLast),
  .out_MReset_n(app_out_2_sma1_in_MReset_n),
  .out_SReset_n(sma1_in_2_app_out_SReset_n),
  .out_SThreadBusy(sma1_in_2_app_out_SThreadBusy));

  BRAM1Load #(.FILENAME("metadatarom.data"),
	      .PIPELINED(1'd0),
	      .ADDR_WIDTH(32'd10),
	      .DATA_WIDTH(32'd32),
	      .MEMSIZE(11'd1024),
	      .BINARY(1'd0)) rom_memory(.CLK(CLK),
					.ADDR(rom_addr),
					.DI(32'd0),
					.WE(1'd0),
					.EN(rom_en),
					.DO(rom_data));

// Tieoffs for unused signals
// First, the unused WSI slave signals
assign wci_s_0_SResp = 2'b0;
assign wci_s_0_SThreadBusy = 1'b0;
assign wci_s_0_SFlag = 2'b0;
assign wci_s_0_SData = 32'b0;
assign wci_s_1_SResp = 2'b0;
assign wci_s_1_SThreadBusy = 1'b0;
assign wci_s_1_SFlag = 2'b0;
assign wci_s_1_SData = 32'b0;
assign wci_s_5_SResp = 2'b0;
assign wci_s_5_SThreadBusy = 1'b0;
assign wci_s_5_SFlag = 2'b0;
assign wci_s_5_SData = 32'b0;
assign wci_s_6_SResp = 2'b0;
assign wci_s_6_SThreadBusy = 1'b0;
assign wci_s_6_SFlag = 2'b0;
assign wci_s_6_SData = 32'b0;
assign wci_s_7_SResp = 2'b0;
assign wci_s_7_SThreadBusy = 1'b0;
assign wci_s_7_SFlag = 2'b0;
assign wci_s_7_SData = 32'b0;
// Unused memory master signals
assign wmemiM0_MCmd = 3'b0;
assign wmemiM0_MReqLast = 1'b0;
assign wmemiM0_MAddr = 36'b0;
assign wmemiM0_MBurstLength = 12'b0;
assign wmemiM0_MDataValid = 1'b0;
assign wmemiM0_MDataLast = 1'b0;
assign wmemiM0_MData = 128'b0;
assign wmemiM0_MDataByteEn = 16'b0;
assign wmemiM0_MReset_n = 1'b1;
// Unused adc slave signals
assign wsi_s_adc_SThreadBusy = 1'b1;
assign wsi_s_adc_SReset_n = 1'b0;
// Unusd dac master signals
assign wsi_m_dac_MCmd = 3'b0;
assign wsi_m_dac_MReqLast = 1'b0;
assign wsi_m_dac_MBurstPrecise = 1'b0;
assign wsi_m_dac_MBurstLength = 12'b0;
assign wsi_m_dac_MData = 32'b0;
assign wsi_m_dac_MByteEn = 4'b0;
assign wsi_m_dac_MReqInfo = 8'b0;
assign wsi_m_dac_MReset_n = 1'b0;
// Unused wti slave signals
assign wti_s_0_SThreadBusy = 1'b0;
assign wti_s_0_SReset_n = 1'b1;
assign wti_s_1_SThreadBusy = 1'b0;
assign wti_s_1_SReset_n = 1'b1;
assign wti_s_2_SThreadBusy = 1'b0;
assign wti_s_2_SReset_n = 1'b1;
endmodule  // mkOCApp

