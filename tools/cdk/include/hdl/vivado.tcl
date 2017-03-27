source $env(OCPI_CDK_DIR)/include/hdl/helper_functions.tcl

# Collect input arguments
set load_cores_function "" 
set prim_libs           "" 
set vhdl_sources        "" 
set v_sources           "" 
set inc_dirs            "" 
set top_module          "" 
set core                "" 
set artifact            "" 
set tgt_dir             ""  
set target_part         ""    
set full_part           ""   
set hdl_mode            ""  
set abs_path            ""   

puts "Arguments: $argv"
parse_args $argv

###############################################################################
# Set the mode, module, and part
###############################################################################

# If this is a container, then leave default mode
if {[string equal $hdl_mode container]} {
  set mode default
} else {
# For everything else, we will use out_of_context
  set mode out_of_context
}
set module $top_module
# This should not be hardcoded... Vivado requires a full part number for synthesis
set part $full_part
#set part xc7z020clg484-1

###############################################################################
# Helper functions
###############################################################################

# This function adds the files <f> and then associates them with <library>.
# If <f> is empty, nothing is done. 
# If <hdlmode> != library, we do associate files with a library.
proc add_files_set_lib {f library hdlmode} {
  if {[string equal [string trim $f] ""]} {
    puts "No files to add"
  } else {
#    add_files $f -quiet
    add_files $f
      #puts "Adding file $f and setting LIBRARY to $library"
#      set_property LIBRARY "$library" [get_files $f -quiet]
      set_property LIBRARY "$library" [get_files $f]
  }
}

set initial_time [clock seconds]
puts "Init time: $initial_time"

###############################################################################
# Extract files and their associated vhdl libraries from previously processed
# ocpi libraries
###############################################################################

# Here we open up the project files for other assets (e.g. a primitive library)
# We extract the project's files and each file's associated library into two lists:
#   files
#   libs
# We then add both of these lists (as a tuple of lists called files_libs) to 
#   lib_lists
# So, lib_lists is a 3d list with dimension 1 for accessing an included library,
#   dimension 2 for accessing either the library's list of files or list of associated
#   vhdl libraries, and dimension 3 for accessing a specific file and associated library
# libinc_names is a list of the "included library" names (corresponding to an opencpi asset)
#   this list is only for print statements and so may be removed
#
# The tmp/ directory is used to store copies of the project files being opened. Copies need
# to be made because if we open the original project file (even as read-only), the edit-date
# will be updated on the project file. Copying it to a tmp location allows the xpr project
# file to actually be a make target whose date can be checked.
if {[string length $prim_libs] > 0} {
  set lib_lists {}
  file mkdir $tgt_dir/tmp
  foreach lib $prim_libs {
    puts "Primitive library being imported: $lib"
    set libname [file tail $lib]
    ## For primitives: Checkpoint is exported to <project>/exports/lib/hdl/<prim>/<target>/<prim>.dcp
    ##read_checkpoint [glob $lib/$target_part/*.dcp]
    file copy $lib/$target_part/$libname.xpr $tgt_dir/tmp/
    open_project -read_only $tgt_dir/tmp/$libname.xpr -quiet
    set f_lib_tuples {}
    set files [get_files]
    set libs [get_property LIBRARY $files]
    set files_libs {}
    lappend files_libs $files
    lappend files_libs $libs
    lappend lib_lists $files_libs
    lappend libinc_names [file tail $lib]
    file delete -force [glob $tgt_dir/tmp/*]
  }
  file delete -force $tgt_dir/tmp
}

set done_read_lib_files_time [clock seconds]

###############################################################################
# Create the project. Add the files from previous libraries and maintain
# previous associations with vhdl libraries
###############################################################################
create_project [file rootname [file tail $artifact]] -part $part -force $tgt_dir
set_property part $full_part [current_project]

if {[string length $prim_libs] > 0} {
  if {[info exists libinc_names]} {
    # Loop through the included libraries (probably primitive libraries)
    #   Loop through the files in each primitive library ("files" list), 
    #     add them to the current project (if not already added), and 
    #     associate them with the vhdl library at the same index in "libs"
    #
    set libinc_index 0
    foreach libinc $libinc_names {
      #puts "Index: $lib_index"
      puts "Reassociating files to libraries for primitive: $libinc"
      set files_libs [lindex $lib_lists $libinc_index]
      #puts "Lib$lib_index: $lib_files"
      set files [lindex $files_libs 0]
      set libs [lindex $files_libs 1]
      set num_files [llength $files]
      for {set f_index 0} {$f_index < $num_files} {incr f_index} {
        set f [lindex $files $f_index]
        set lib [lindex $libs $f_index]
        if {[expr [llength [get_files $f]] > 0]} {
          #puts "This file($f) exists already in project. Skipping."
        } else {
          add_files $f -quiet 
          set_property LIBRARY "$lib" [get_files $f -quiet]
        }
      }
      incr libinc_index
    }
  }
}

set done_associate_lib_files_time [clock seconds]

###############################################################################
# Read in checkpoints from cores/workers. DCP files contain netlists and stubs
###############################################################################

if {[string length $load_cores_function] > 0} {
  eval $load_cores_function
}

set done_dcps_time [clock seconds]

###############################################################################
# Read in the source files for this ocpi asset and set the vhdl library
# to the asset's name. This includes Verilog, VHDL, XDC, include dirs
###############################################################################

set library [file tail $abs_path]

# For each VHDL source, add the file and associate it with the current library
if {[string length $vhdl_sources] > 0} {
  puts "Adding files: $vhdl_sources"
  add_files_set_lib $vhdl_sources $library $hdl_mode
}

if {[string length $inc_dirs] > 0} {
  set inc_dirs_opt "$inc_dirs"
}

# Do not need this with INC_DIRS?
#if {[string length $env(V_INCLUDES)] > 0} {
#  add_files $env(V_INCLUDES)
#}

# For each V source, add the file
if {[string length $v_sources] > 0} {
  add_files -scan_for_includes $v_sources -quiet 
}

set files_added_time [clock seconds]

#report_compile_order

###############################################################################
# Synthesis
###############################################################################
set_property top $module [current_fileset]
# Default options used for any mode: 
#set synth_options "-mode $mode -top $module -part $part -fsm_extraction off -flatten rebuilt -include_dirs $tgt_dir" 
set synth_options ""
set synth_options "$synth_options -top $module"
set synth_options "$synth_options -part $part"
# The target-* dir needs to be added for the *.vh files within
set synth_options "$synth_options -include_dirs \"$tgt_dir\""
set synth_options "$synth_options -flatten rebuilt"
#set synth_options "$synth_options -flatten none"
#set synth_options "$synth_options -directive runtimeoptimized"

# If there are 'include directories' add those
if {[string length $inc_dirs] > 0} {
  set synth_options "$synth_options -include_dirs \"$inc_dirs_opt\""
}

set post_synth "puts \"Synthesis complete.\" ; "
set synth_command ""
# Now add mode-specific options:
switch -regex $hdl_mode {
  core|worker|platform|config|assembly|container {
    # Create the Design Checkpoint only for containers
    #   This is used during the implementation stage
    if {[string equal $hdl_mode container]} {
      set synth_options "$synth_options -mode default"
#      set post_synth "$post_synth write_checkpoint -force [file rootname $artifact].dcp ;"
    } else {
      # We wait until our very last synthesis run to do optimizations
      set synth_options "$synth_options -mode out_of_context"
      set synth_options "$synth_options -quick"
      set synth_options "$synth_options -no_timing_driven"
      set synth_options "$synth_options -fsm_extraction off"
      # Create a netlist EDIF file
    }
    set synth_command "synth_design $synth_options ;"
    set post_synth "$post_synth write_edif -force $artifact ;"
    set post_synth "$post_synth report_utilization ;"
  }
  library {
    puts "Adding options specific to primitive libraries"
    # For primitive libraries, we limit this to RTL elaboration only and
    # discard the results. Our progress has been stored in the XPR project file.
    set hdl_no_elab true
    if {![string equal $hdl_no_elab true]} {
      # Elaboration in vivado is slow. We want to provide the option NOT to elaborate at all
      set synth_options "$synth_options -mode out_of_context"
      set synth_options "$synth_options -quick"
      set synth_options "$synth_options -rtl"
      set synth_options "$synth_options -no_timing_driven"
      set synth_options "$synth_options -fsm_extraction off"
      set post_synth "$post_synth puts \"Not writing checkpoint since doing -rtl only\" ;"
      set synth_command "synth_design $synth_options ;"
    } else {
      set post_synth "$post_synth puts \"Not elaborating library to speed up build.\""
    }
  }
}
puts "Running synthesis command: $synth_command"
set synth_start_time [clock seconds]
puts "Synth start time: $synth_start_time"

eval "$synth_command"

set synth_end_time [clock seconds]
puts "Synth end time: $synth_end_time"

###############################################################################
# Save results and close project
###############################################################################

puts "Running post-synthesis commands: $post_synth"
eval "$post_synth"

close_project

puts "Done with Vivado compilation"

puts "======Timing broken up into subtasks======
-----------------------------------------------------------------
|                     Read library source files: [expr $done_read_lib_files_time - $initial_time] seconds \t|
-----------------------------------------------------------------
| Associate library sources with VHDL libraries: [expr $done_associate_lib_files_time - $done_read_lib_files_time] seconds \t|
-----------------------------------------------------------------
|                     Load DCP files from Cores: [expr $done_dcps_time - $done_associate_lib_files_time] seconds \t|
-----------------------------------------------------------------
|          Add source files for this OCPI asset: [expr $files_added_time - $done_dcps_time] seconds \t|
-----------------------------------------------------------------
|                                Synthesis time: [expr $synth_end_time - $synth_start_time] seconds \t|
-----------------------------------------------------------------
|             Post synthesis (write_checkpoint): [expr [clock seconds] - $synth_end_time] seconds \t|
-----------------------------------------------------------------
|                                Total TCL time: [expr [clock seconds] - $initial_time] seconds
-----------------------------------------------------------------"

