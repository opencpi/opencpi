-- Adapt the axi_gp master from the PS to a CP master
-- The clock and reset are injected to be supplied to both sides
library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library platform; use platform.all;
library bsv;
library zed; use zed.all;
entity axi2cp is
  port(
    clk     : in std_logic;
    reset   : in std_logic;
    axi_in  : in  zynq_pkg.axi_gp_out_t;
    axi_out : out zynq_pkg.axi_gp_in_t;
    cp_in   : in  platform_pkg.occp_out_t;
    cp_out  : out platform_pkg.occp_in_t
    );
end entity axi2cp;
architecture rtl of axi2cp is
  signal axi_reset_sync_n : std_logic; -- the PS's AXI reset, sync'd to our clock
  signal read_done        : std_logic;
  signal write_done       : std_logic;
  signal in_read_r        : std_logic;
  signal in_write_r       : std_logic;
  function read_byte_en(low2addr   : std_logic_vector(1 downto 0);
                        log2nbytes : std_logic_vector(2 downto 0))
    return std_logic_vector is
  begin
   case log2nbytes & low2addr is
     when "00000" => return "0001";
     when "00001" => return "0010";
     when "00010" => return "0100";
     when "00011" => return "1000";
     when "00100" => return "0011";
     when "00110" => return "1100";
     when "01000" => return "1111";
     when others   => return "0000";
   end case;
  end read_byte_en;
begin
  -- Our reset logic:
  -- We have the reset associated with our clock, and we also
  -- have the reset from the PS for our AXI GP port.
  -- Thus the reset given to the control plane is the OR of then.
  -- But the PS's reset is async to us, so we furst synchronize it.
  sr : bsv.bsv.SyncResetA
    generic map(RSTDELAY => 17)
    port map(IN_RST  => axi_in.ARESETN,
             CLK     => clk,
             OUT_RST => axi_reset_sync_n);

  -- AXI GP signals we drive from the PL into the PS, ordered per AXI Chapter 2
  -- Global signals
  axi_out.ACLK    <= clk;        -- we drive the control clock as the AXI GP CLK
  -- Write Address Channel
  axi_out.AWREADY <= write_done; -- write_done depends on AWVALID and WVALID
  -- Write Data Channel
  axi_out.WREADY  <= write_done;
  -- Write Response Channel
  axi_out.BID     <= axi_in.AWID; -- we only do one at a time
  axi_out.BRESP   <= zynq_pkg.Resp_OKAY;
  axi_out.BVALID  <= write_done;  -- write done is only cleared upon BREADY
  -- Read Address Channel
  axi_out.ARREADY <= read_done;
  -- Read Data Channel
  axi_out.RID     <= axi_in.ARID;
  axi_out.RDATA   <= cp_in.data;
  axi_out.RRESP   <= zynq_pkg.Resp_OKAY;
  axi_out.RLAST   <= read_done; -- WHAT ABOUT A 64 BIT LOAD?  IS THAT A BURST?
  axi_out.RVALID  <= read_done;

  -- CP Master output signals we drive
  cp_out.clk      <= clk;
  cp_out.reset    <= reset or not axi_reset_sync_n;
  cp_out.valid    <= in_read_r or in_write_r; -- latency is one.
  cp_out.is_read  <= in_read_r;
  cp_out.address  <= axi_in.AWADDR(cp_out.address'range)
                     when in_write_r = '1' else axi_in.ARADDR(cp_out.address'range);
  cp_out.byte_en  <= axi_in.WSTRB when in_write_r = '1' else
                       read_byte_en(axi_in.ARADDR(1 downto 0),
                                    axi_in.ARSIZE);
  cp_out.data     <= axi_in.WDATA;
  cp_out.take     <= read_done and axi_in.RREADY;
  -- Internal signals
  -- Write is done when occp takes the write request (and posts it).
  write_done      <= in_write_r and cp_in.take;
  -- Read is done when occp provides a valid response
  read_done       <= in_read_r and cp_in.valid;
  work : process(clk)
  begin
    if rising_edge(clk) then
      if reset = '1' then
        in_read_r  <= '0';
        in_write_r <= '0';
      elsif (in_read_r = '0' and in_write_r = '0') or
            read_done = '1' or write_done = '1' then
        -- nothing in progress or something is finishing
        -- test write before read 
        if axi_in.AWVALID = '1' and axi_in.WVALID = '1' then
          in_write_r <= '1';
        elsif axi_in.ARVALID = '1' then
          in_read_r <= '1';
        end if;
      elsif in_read_r = '1' and read_done = '1' and axi_in.RREADY = '1' then
        in_read_r <= '0';
      elsif in_write_r = '1' and write_done = '1' then
        in_write_r <= '0';
      end if;
    end if;
  end process;
end rtl;
    
