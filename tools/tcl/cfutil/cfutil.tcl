package require Itcl

set cfutildir [file dirname [info script]]

if {[file exists [file join $cfutildir combat orb.tcl]]} {
    lappend auto_path [file join $cfutildir combat]
}

package require combat

source [file join $cfutildir CosNaming.tcl]
source [file join $cfutildir CF.tcl]

source [file join $cfutildir namingContext.tcl]
source [file join $cfutildir loadWorker.tcl]
source [file join $cfutildir executeWorker.tcl]
source [file join $cfutildir uploadZip.tcl]
source [file join $cfutildir fsTools.tcl]
source [file join $cfutildir dmTools.tcl]
source [file join $cfutildir appFactory.tcl]

package provide cfutil 0.1
