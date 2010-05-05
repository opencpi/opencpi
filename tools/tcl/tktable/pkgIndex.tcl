# based on the ideas on http://wiki.tcl.tk/9427
if {[catch {package require Tcl 8.2}]} return

# return a platform designator, including both OS and machine
proc platform {} {
    global tcl_platform
    set plat [lindex $tcl_platform(os) 0]
    set mach $tcl_platform(machine)
    switch -glob -- $mach {
        intel -
        i*86* { set mach x86 }
	x*86* { set mach x86 }
        "Power Macintosh" { set mach ppc }
    }
    return "$plat-$mach"
}

proc loadlib {dir package version} {
    global tcl_platform
    set lib $package[info sharedlibextension]
    return [file join $dir [platform] $lib]
}

package ifneeded Tktable 2.9 [list load [loadlib $dir Tktable 2.9]]
