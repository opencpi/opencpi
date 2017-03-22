
# This function looks for arguments of syntax "A=B"
#   It creates the variable A and assigns it the value B
proc parse_args {arguments} {
  puts  "Args: $arguments"
  foreach i $arguments {
    puts "I: $i"
    set assignment [split $i =]
    if {[llength $assignment] != 2} {
      puts "Invalid assignment \"$assignment\". tclargs must be in the following format: variablename=value"
      exit 2
    }
    set var_name [lindex $assignment 0]
    upvar 1 $var_name var
    set var_value [lindex $assignment 1]
    puts "Assignment: $var_name = $var_value"
    set var $var_value
  }
}

