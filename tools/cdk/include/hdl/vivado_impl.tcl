source $env(OCPI_CDK_DIR)/include/hdl/helper_functions.tcl

set stage               ""
set target_file         ""
set checkpoint          ""
set part                ""
set constraints         ""
parse_args $argv

# Read in platform XDC
if {[info exists constraints] && [string length $constraints] > 0} {
  read_xdc $constraints
}

# Read in an implementation checkpoint
if {[info exists checkpoint] && [string length $checkpoint] > 0} {
  read_checkpoint -part $part $checkpoint
}

# Read in container edif
if {[info exists edif_file] && [string length $edif_file] > 0} {
  read_edif $edif_file
}

# Open the design/link
set mode default
link_design -mode $mode -part $part

# For each stage, allow loading of checkpoint
# Take current stage as parameter, and run the corresponding
# implementation step
set stage_start_time [clock seconds]
puts "Running Implementation stage $stage"
switch $stage {
  opt {
    opt_design
  }
  place {
    place_design
  }
  phys_opt {
    phys_opt_design
  }
  route {
    route_design
  }
  timing {
    report_timing -file $target_file
  }
  bit {
    write_bitstream -force $target_file
  }
}

set stage_end_time [clock seconds]

puts "Writing checkpoint for stage $stage"
if {![string equal $stage bit] && ![string equal $stage timing]} {
  write_checkpoint -force $target_file
}

set final_time [clock seconds]

puts "======Timing broken up into subtasks======
-----------------------------------------------------------------
|  Implementation Stage time: [expr $stage_end_time - $stage_start_time] seconds \t|
-----------------------------------------------------------------
| Post-stage operations time: [expr [clock seconds] - $stage_end_time] seconds \t|
-----------------------------------------------------------------
|                 Total time: [expr [clock seconds] - $stage_start_time] seconds \t|
-----------------------------------------------------------------"

