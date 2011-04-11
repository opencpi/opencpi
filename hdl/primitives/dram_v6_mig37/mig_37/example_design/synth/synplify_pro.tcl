project -new
#project files
add_file -verilog "../rtl/controller/arb_mux.v"
add_file -verilog "../rtl/controller/arb_row_col.v"
add_file -verilog "../rtl/controller/arb_select.v"
add_file -verilog "../rtl/controller/bank_cntrl.v"
add_file -verilog "../rtl/controller/bank_common.v"
add_file -verilog "../rtl/controller/bank_compare.v"
add_file -verilog "../rtl/controller/bank_mach.v"
add_file -verilog "../rtl/controller/bank_queue.v"
add_file -verilog "../rtl/controller/bank_state.v"
add_file -verilog "../rtl/controller/col_mach.v"
add_file -verilog "../rtl/controller/mc.v"
add_file -verilog "../rtl/controller/rank_cntrl.v"
add_file -verilog "../rtl/controller/rank_common.v"
add_file -verilog "../rtl/controller/rank_mach.v"
add_file -verilog "../rtl/controller/round_robin_arb.v"
add_file -verilog "../rtl/ecc/ecc_buf.v"
add_file -verilog "../rtl/ecc/ecc_dec_fix.v"
add_file -verilog "../rtl/ecc/ecc_gen.v"
add_file -verilog "../rtl/ecc/ecc_merge_enc.v"
add_file -verilog "../rtl/ip_top/clk_ibuf.v"
add_file -verilog "../rtl/ip_top/ddr2_ddr3_chipscope.v"
add_file -verilog "../rtl/ip_top/example_top.v"
add_file -verilog "../rtl/ip_top/infrastructure.v"
add_file -verilog "../rtl/ip_top/iodelay_ctrl.v"
add_file -verilog "../rtl/ip_top/mem_intfc.v"
add_file -verilog "../rtl/ip_top/memc_ui_top.v"
add_file -verilog "../rtl/phy/circ_buffer.v"
add_file -verilog "../rtl/phy/phy_ck_iob.v"
add_file -verilog "../rtl/phy/phy_clock_io.v"
add_file -verilog "../rtl/phy/phy_control_io.v"
add_file -verilog "../rtl/phy/phy_data_io.v"
add_file -verilog "../rtl/phy/phy_dly_ctrl.v"
add_file -verilog "../rtl/phy/phy_dm_iob.v"
add_file -verilog "../rtl/phy/phy_dq_iob.v"
add_file -verilog "../rtl/phy/phy_dqs_iob.v"
add_file -verilog "../rtl/phy/phy_init.v"
add_file -verilog "../rtl/phy/phy_pd.v"
add_file -verilog "../rtl/phy/phy_pd_top.v"
add_file -verilog "../rtl/phy/phy_rdclk_gen.v"
add_file -verilog "../rtl/phy/phy_rdctrl_sync.v"
add_file -verilog "../rtl/phy/phy_rddata_sync.v"
add_file -verilog "../rtl/phy/phy_rdlvl.v"
add_file -verilog "../rtl/phy/phy_read.v"
add_file -verilog "../rtl/phy/phy_top.v"
add_file -verilog "../rtl/phy/phy_write.v"
add_file -verilog "../rtl/phy/phy_wrlvl.v"
add_file -verilog "../rtl/phy/rd_bitslip.v"
add_file -verilog "../rtl/traffic_gen/afifo.v"
add_file -verilog "../rtl/traffic_gen/cmd_gen.v"
add_file -verilog "../rtl/traffic_gen/cmd_prbs_gen.v"
add_file -verilog "../rtl/traffic_gen/data_prbs_gen.v"
add_file -verilog "../rtl/traffic_gen/init_mem_pattern_ctr.v"
add_file -verilog "../rtl/traffic_gen/mcb_flow_control.v"
add_file -verilog "../rtl/traffic_gen/mcb_traffic_gen.v"
add_file -verilog "../rtl/traffic_gen/rd_data_gen.v"
add_file -verilog "../rtl/traffic_gen/read_data_path.v"
add_file -verilog "../rtl/traffic_gen/read_posted_fifo.v"
add_file -verilog "../rtl/traffic_gen/sp6_data_gen.v"
add_file -verilog "../rtl/traffic_gen/tg_status.v"
add_file -verilog "../rtl/traffic_gen/v6_data_gen.v"
add_file -verilog "../rtl/traffic_gen/wr_data_gen.v"
add_file -verilog "../rtl/traffic_gen/write_data_path.v"
add_file -verilog "../rtl/ui/ui_cmd.v"
add_file -verilog "../rtl/ui/ui_rd_data.v"
add_file -verilog "../rtl/ui/ui_top.v"
add_file -verilog "../rtl/ui/ui_wr_data.v"

#implementation: "rev_1"
impl -add rev_1 -type fpga

#device options
set_option -technology virtex6
set_option -part xc6vlx240t
set_option -package ff1156
set_option -speed_grade -1
set_option -part_companion ""

#compilation/mapping options
set_option -use_fsm_explorer 0
set_option -top_module "example_top"

# sequential_optimization_options
set_option -symbolic_fsm_compiler 1

# Compiler Options
set_option -compiler_compatible 0
set_option -resource_sharing 1

# mapper_options
set_option -frequency 400
set_option -write_verilog 0
set_option -write_vhdl 0

# Xilinx Virtex2
set_option -run_prop_extract 1
set_option -maxfan 10000
set_option -disable_io_insertion 0
set_option -pipe 1
set_option -update_models_cp 0
set_option -retiming 0
set_option -no_sequential_opt 0
set_option -fixgatedclocks 3
set_option -fixgeneratedclocks 3

# Xilinx Virtex6
set_option -enable_prepacking 1

#VIF options
set_option -write_vif 1

#automatic place and route (vendor) options
set_option -write_apr_constraint 1

#set result format/file last
project -result_file "../synth/rev_1/example_top.edf"

#
#implementation attributes

set_option -vlog_std v2001
set_option -project_relative_includes 1
impl -active "../synth/rev_1"
project -run
project -save
