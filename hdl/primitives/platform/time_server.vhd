-- A time server that is easy to use in platform workers,
-- i.e. you just feed in the property signals
library IEEE; use IEEE.std_logic_1164.all, IEEE.numeric_std.all;
library ocpi; use ocpi.all, ocpi.types.all;
library work; use work.platform_pkg.all;
entity time_server is
  port (
    CLK, RST_N          : in  std_logic;
    timeCLK, timeRST_N  : in  std_logic;
    ppsIn               : in  std_logic;  -- ASYNC
    -- Property interface
    timeControl         : in  ulong_t;
    timeControl_written : in  bool_t;
    timeStatus          : out  ulong_t;
    timeNowIn           : in  ulonglong_t;
    timeNow_written     : in  bool_t;
    timeNowOut          : out ulonglong_t;
    timeDeltaIn         : in  ulonglong_t;
    timeDelta_written   : in  bool_t;
    timeDeltaOut        : out ulonglong_t;
    ticksPerSecond      : out ulong_t;
    -- Outputs in tiock clock domain
    ppsOut              : out std_logic;
    time_service        : out time_service_t  -- time service clock domain
    );
end entity time_server;
architecture rtl of time_server is
  signal dataIn       : ulonglong_t;
  signal doClear      : std_logic;
  signal ticks        : std_logic_vector(27 downto 0);
-- this is for the verilog wrapped version
  component timeService is
    port(
      CLK, RST_N                   : in std_logic;
      CLK_time_clk, RST_N_time_rst : in std_logic;
      -- Interface to set the time, with permission
      timeIn                       : in std_logic_vector(63 downto 0);
      doSetTime                    : in std_logic;
      doDeltaTime                  : in std_logic;
      -- Interface to set the controls, without permission
      -- "clear" also clears some status bits at the same time
      doSetControl                 : in std_logic;
      doClear                      : in std_logic;

      -- Status outputs
      statusOut                    : out std_logic_vector(31 downto 0);
      controlOut                   : out std_logic_vector(4 downto 0);
      ticksPerPPSOut               : out std_logic_vector(27 downto 0);
      nowCC                        : out std_logic_vector(63 downto 0);
      deltaTime                    : out std_logic_vector(63 downto 0);
      nowTC                        : out std_logic_vector(63 downto 0);
      
      -- PPS interface
      ppsSyncIn                    : in std_logic;
      ppsSyncOut                   : out std_logic
      );
  end component TimeService;
begin
  dataIn               <= timeNowIn when its(timeNow_written) else
                          timeDeltaIn when its(timeDelta_written) else
                          to_ulonglong(timeControl);
  doClear              <= '1' when timeControl_written and dataIn(31) = '1' else '0';
  ticksPerSecond       <= ulong_t(resize(unsigned(ticks), ulong_t'length));
  time_service.clk     <= timeCLK;
  time_service.reset_n <= timeRST_N;
  ts : TimeService
    port map(
      CLK                 => CLK,
      RST_N               => RST_N,
      CLK_time_clk        => timeClk,
      RST_N_time_rst      => timeRST_N,
      ppsSyncIn           => ppsIn,     -- async
      -- Property interface inputs
      timeIn              => std_logic_vector(dataIn),
      doSetTime           => timeNow_written,
      doDeltaTime         => timeDelta_written,
      doSetControl        => timeControl_written,
      doClear             => doClear,
      -- Stable outputs in control-clock domain
      ulong_t(statusOut)  => timeStatus,
      controlOut          => open,
      ticksPerPPSOut => ticks,
      ulonglong_t(nowCC)      => timeNowOut,
      ulonglong_t(deltaTime)  => timeDeltaOut,
      -- Outputs in time-clock domain
      to_ulonglong(nowTC) => time_service.now,
      ppsSyncOut          => ppsOut);
  end;
