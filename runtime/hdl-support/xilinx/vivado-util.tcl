###############################################################################
# Helper functions
###############################################################################

# This function looks for arguments of syntax "A=B"
#   It creates the variable A and assigns it the value B
proc parse_args {arguments} {
  foreach i $arguments {
    set assignment [split $i =]
    if {[llength $assignment] != 2} {
      puts "Invalid assignment \"$assignment\". tclargs must be in the following format: variablename=value"
      exit 2
    }
    set var_name [lindex $assignment 0]
    upvar 1 $var_name var
    set var_value [lindex $assignment 1]
    set var $var_value
  }
}

# This function adds the files <f> and then associates them with <library>.
# If <f> is empty, nothing is done. 
proc add_files_set_lib {library f} {
  add_files $f -quiet
  set_property LIBRARY $library [get_files $f -quiet] -quiet 
}

# Read in either an edif or dcp (whichever is passed in)
# If reading in a checkpoint, set checkpoints_loaded flag to 1.
#
# Since read_edif can also take in an ngc, this function 
# can actually load dcps, edifs, OR ngcs
proc read_edif_or_dcp {f} {
  if {[string match *.edf $f]} {
    read_edif $f
  } elseif {[string match *.edn $f]} {
    read_edif $f
  } elseif {[string match *.dcp $f]} {
    read_checkpoint $f
  } else {
    puts "File $f is not an EDIF, DCP or XCI file. This may cause problems."
    add_files $f
  }
}
