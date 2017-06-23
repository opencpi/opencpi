puts "Generating wrapper verilog file for processing_system7 version: [lindex $argv 0]"
create_project managed_ip_project managed_ip_project -part xc7z020-clg484-1 -ip -force

create_ip -name processing_system7 -vendor xilinx.com -library ip -version [lindex $argv 0] -module_name processing_system7_0

generate_target all [get_files  managed_ip_project/managed_ip_project.srcs/sources_1/ip/processing_system7_0/processing_system7_0.xci]
