package ifneeded kill 1.0 "
    set killplat [lindex $::tcl_platform(os) 0]
    set killmach $::tcl_platform(machine)
    switch -glob -- \$killmach {
        intel -
        i*86* { set killmach x86 }
        \"Power Macintosh\" { set killmach ppc }
    }
    set killfile \[file join $dir \"kill-\[string tolower \$killplat-\$killmach\][info sharedlibextension]\"\]
    if {\[file exists \$killfile\]} {
      load \$killfile
    } else {
      source \[file join $dir unixkill.tcl\]
    }
"
