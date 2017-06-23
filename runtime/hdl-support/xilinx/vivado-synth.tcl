source $env(OCPI_CDK_DIR)/include/hdl/vivado-util.tcl

# Initialize variables for input arguments
set tcl_imports              "" 
set artifact                 "" 
set hdl_mode                 ""  
set top_mod                  ""  
set synth_part               ""
set synth_opts               ""
set edif_opts                ""

# parse_args parses input arguments of type variableName=value
parse_args $argv

###############################################################################
# Create project. Set part, top-module, settings, mode
##############################################################################
create_project -part $synth_part -force [file rootname [file tail $artifact]] [file dirname $artifact]

set_property top $top_mod [current_fileset]

# Disable the automatic compile ordering. We want to use
# the exact order that the files were added to the project in
set_property source_mgmt_mode None [current_project]
    
# If this is a container, then leave default mode
if {[string equal $hdl_mode container]} {
  set mode default
} else {
# For everything else, we will use out_of_context
  set mode out_of_context
}

# Start timing the stages of this file
set initial_time [clock seconds]

###############################################################################
# Read in checkpoints from cores/workers. DCP files contain netlists and stubs
###############################################################################

# Run the TCL commands for adding the cores and sources for this asset as well
# as any libraries it depends on. This tcl file is generated in vivado.mk and
# exported.
if {[info exists tcl_imports] && [string length $tcl_imports)] > 0} {
  source $tcl_imports
}

report_compile_order -used_in synthesis

set done_load_files_time [clock seconds]

###############################################################################
# Synthesis
###############################################################################
set post_synth "puts \"Synthesis complete.\" ; "
set synth_command ""
# Now add mode-specific options:
switch -regex $hdl_mode {
  core|worker|platform|config|assembly|container {
    # Create a netlist EDIF file
    set post_synth "$post_synth write_edif -force $artifact $edif_opts;"
    set post_synth "$post_synth report_utilization ;"
    set post_synth "$post_synth report_timing ;"
    set post_synth "$post_synth report_design_analysis ;"
  }
  library {
    puts "Adding options specific to primitive libraries"
    # For primitive libraries, we limit this to RTL elaboration only and
    # discard the results.
    if {![info exists hdl_no_elab] || ![string equal $hdl_no_elab true]} {
      # Elaboration in vivado is slow. We want to provide the option NOT to elaborate at all
      if {[info exists module] && [string length $module] > 0} {
      } else {
        set module onewire
        read_verilog $env(OCPI_CDK_DIR)/include/hdl/onewire.v
      }
      set synth_opts "$synth_opts -top $module"
      set post_synth "$post_synth puts \"Not writing checkpoint since doing -rtl only\" ;"
    }
  }
}
set synth_command "synth_design $synth_opts"
puts "Running synthesis command: $synth_command"
set synth_start_time [clock seconds]

eval "$synth_command"

set synth_end_time [clock seconds]

###############################################################################
# Save results and close project
###############################################################################
puts "Running post-synthesis commands: $post_synth"
eval "$post_synth"

close_project

puts "Done with Vivado compilation"

puts "======Timing broken up into subtasks======
-----------------------------------------------------------------
|           Load files from Libraries and Cores: [expr $done_load_files_time - $initial_time] seconds \t|
-----------------------------------------------------------------
|                  Set up the synthesis options: [expr $synth_start_time - $done_load_files_time] seconds \t|
-----------------------------------------------------------------
|                                Synthesis time: [expr $synth_end_time - $synth_start_time] seconds \t|
-----------------------------------------------------------------
|                Post synthesis (write netlist): [expr [clock seconds] - $synth_end_time] seconds \t|
-----------------------------------------------------------------
|                                Total TCL time: [expr [clock seconds] - $initial_time] seconds
-----------------------------------------------------------------"

