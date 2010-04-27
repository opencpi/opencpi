#
# ----------------------------------------------------------------------
# Various File System tools
# ----------------------------------------------------------------------
#

namespace eval cfutil {}
namespace eval cfutil::fs {}

#
# ----------------------------------------------------------------------
# Find file(s) matching a pattern.
# ----------------------------------------------------------------------
#

proc cfutil::fs::find {fs origDir pattern {recursive 1} {first 0}} {
    set found [list]
    set dirs [list $origDir]

    if {$origDir != "/"} {
	set odl [string length $origDir]
    } else {
	set odl 0
    }

    while {[llength $dirs]} {
	set dir [lindex $dirs 0]
	set dirs [lrange $dirs 1 end]
	set dl [string length $dir]

	if {$recursive} {
	    if {$dir != "/"} {
		set pat "$dir/*"
	    } else {
		set pat "/*"
	    }
	} else {
	    if {$dir != "/"} {
		set pat "$dir/$pattern"
	    } else {
		set pat "/$pattern"
	    }
	}

	foreach file [$fs list $pat] {
	    set fn [lindex $file 1]

	    if {[string range $fn end end] == "/"} {
		set fn [string range $fn 0 end-1]
	    }

	    if {[set lsi [string first "/" $fn]] != -1} {
		set fn [string range $fn [expr {$lsi+1}] end]
	    }

	    if {$dir != "/"} {
		set absName "$dir/$fn"
	    } else {
		set absName "/$fn"
	    }

	    if {$odl && [string range $absName 0 [expr {$odl-1}]] != $origDir} {
		# Looks like we got off track
		continue
	    }

	    # The Harris Core Framework lists "." and "..".  We don't want them.

	    if {$fn == "." || $fn == ".."} {
		continue
	    }

	    set type [lindex $file 3]

	    if {$recursive && ($type == "DIRECTORY" || $type == "FILE_SYSTEM")} {
		lappend dirs $absName
	    } elseif {$type == "PLAIN"} {
		if {[string match $pattern $fn]} {
		    if {$first} {
			return $absName
		    }

		    lappend found $absName
		}
	    }
	}
    }

    if {$first} {
	error "no files found matching pattern $pattern"
    }

    return $found
}

#
# ----------------------------------------------------------------------
# rm -rf
# ----------------------------------------------------------------------
#

proc cfutil::fs::rmrf {fs files} {
    foreach file $files {
	#
	# The only way to test if a name is a directory is to list it.
	#

	if {[string range $file end end] == "/"} {
	    set file [string range $file 0 end-1]
	}

	set l [$fs list $file]

	if {[llength $l] != 1} {
	    continue
	}

	set ff [lindex $l 0]
	set fn [lindex $ff 1]
	set type [lindex $ff 3]

	if {[string range $fn end end] == "/"} {
	    set fn [string range $fn 0 end-1]
	}

	if {[set lsi [string first "/" $fn]] != -1} {
	    set fn [string range $fn [expr {$lsi+1}] end]
	}

	if {$type == "DIRECTORY"} {
	    set pat "$file/*"
	    set contents [list]

	    foreach df [$fs list $pat] {
		set dfn [lindex $df 1]

		if {[string range $dfn end end] == "/"} {
		    set dfn [string range $dfn 0 end-1]
		}

		if {[set lsi [string first "/" $dfn]] != -1} {
		    set dfn [string range $dfn [expr {$lsi+1}] end]
		}

		if {$dfn == "." || $dfn == ".."} {
		    continue
		}

		lappend contents "$file/$dfn"
	    }

	    rmrf $fs $contents
	    $fs rmdir $file
	} elseif {$type == "PLAIN"} {
	    $fs remove $file
	}
    }
}

