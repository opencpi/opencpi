
# #####
#
#  Copyright (c) Mercury Federal Systems, Inc., Arlington VA., 2009-2010
#
#    Mercury Federal Systems, Incorporated
#    1901 South Bell Street
#    Suite 402
#    Arlington, Virginia 22202
#    United States of America
#    Telephone 703-413-0781
#    FAX 703-413-0784
#
#  This file is part of OpenCPI (www.opencpi.org).
#     ____                   __________   ____
#    / __ \____  ___  ____  / ____/ __ \ /  _/ ____  _________ _
#   / / / / __ \/ _ \/ __ \/ /   / /_/ / / /  / __ \/ ___/ __ `/
#  / /_/ / /_/ /  __/ / / / /___/ ____/_/ / _/ /_/ / /  / /_/ /
#  \____/ .___/\___/_/ /_/\____/_/    /___/(_)____/_/   \__, /
#      /_/                                             /____/
#
#  OpenCPI is free software: you can redistribute it and/or modify
#  it under the terms of the GNU Lesser General Public License as published
#  by the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  OpenCPI is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU Lesser General Public License for more details.
#
#  You should have received a copy of the GNU Lesser General Public License
#  along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.
#
########################################################################### #

#! /bin/sh
# the next line restarts using expect \
exec tclsh8.5 "$0" ${1+"$@"}

set pdir [file dirname [file dirname [info script]]]

foreach pkg {combat cfutil bwidget} {
    lappend auto_path [file join $pdir $pkg]
}

package require Tk
package require cfutil
package require BWidget

set hostname [info hostname]

catch {
    set testsock [socket -server none -myaddr [info hostname] 0]
    catch {
        set hostname [lindex [fconfigure $testsock -sockname] 0] ;# IP address
    }
    close $testsock
}


set argv [eval corba::init -ORBHostName $hostname $argv]
set ::ncIor ""
set ::appFile ""
set ::sadFile ""
set ::verbose 0
set ::uniqueCounter 0

#
# ----------------------------------------------------------------------
# Application functions.
# ----------------------------------------------------------------------
#

proc MakeAppTop {app} {
    incr ::uniqueCounter
    set appName ".app${::uniqueCounter}"
    set ::applications($appName) $app
    set ::appAutoUpdate($appName) ""

    set props [list]
    $app query props
    set ::appProps($appName) $props

    toplevel $appName -width 600 -height 480
    wm title $appName "[$app cget -name]"

    frame $appName.update
    label $appName.update.l1 -text "Properties:"
    button $appName.update.b -text "Update" -command "Update $appName"
    label $appName.update.l2 -text "or auto-update every"
    entry $appName.update.e -width 4
    label $appName.update.l3 -text "seconds"
    pack $appName.update.l1 $appName.update.b $appName.update.l2 \
        $appName.update.e $appName.update.l3 \
        -side left -padx 1
    pack $appName.update -side top -pady 10
    bind $appName.update.e <Return> "AutoUpdate $appName"

    ScrolledWindow $appName.sw -relief sunken
    ScrollableFrame $appName.sw.sf -constrainedwidth 1
    set propsFrame [$appName.sw.sf getframe]
    $appName.sw setwidget $appName.sw.sf

    set idx 0
    foreach p $props {
        array set prop $p
        set value [lindex $prop(value) 1]
        set wname "l[incr idx]"
        LabelEntry $propsFrame.$wname -label $prop(id) -text $value \
            -labelwidth 20 -width 20 -labelanchor e \
            -command "Configure $appName $prop(id)"
        pack $propsFrame.$wname -side top -fill x
    }

    pack $appName.sw -side top -fill both -expand yes

    Separator $appName.sep2 -orient horizontal
    pack $appName.sep2 -side top -fill x -pady 10

    frame $appName.buts
    button $appName.buts.b1 -width 10 -text "Start" -command "Start $appName"
    button $appName.buts.b2 -width 10 -text "Stop" -command "Stop $appName"
    button $appName.buts.b3 -width 10 -text "Terminate" -command "Terminate $appName"
    pack $appName.buts.b1 $appName.buts.b2 $appName.buts.b3 -side left -pady 10 -padx 20
    pack $appName.buts
}

proc Configure {appName propId} {
    set app $::applications($appName)
    set name [$app cget -name]
    set propsFrame [$appName.sw.sf getframe]
    set props [list]

    foreach p $::appProps($appName) {
        array set prop $p
        set wname "l[incr idx]"

        if {$prop(id) eq $propId} {
            set type [lindex $prop(value) 0]
            set value [$propsFrame.$wname cget -text]
            lappend props [list id $propId value [list $type $value]]
            break
        }
    }

    set ::status "Configuring \"$name\" ..."
    $app configure $props
    set ::status "Configured \"$name\"."
}

proc Update {appName} {
    set app $::applications($appName)

    #
    # The app is likely, but not guaranteed to return properties in the
    # same order always.  So process in the same order that we used when
    # creating the widgets.
    #

    set props [list]
    $app query props

    foreach p $props {
        array set prop $p
        set ap($prop(id)) [lindex $prop(value) 1]
    }

    set propsFrame [$appName.sw.sf getframe]

    foreach p $::appProps($appName) {
        array set prop $p
        set wname "l[incr idx]"
        if {[info exists ap($prop(id))]} {
            $propsFrame.$wname configure -text $ap($prop(id))
        }
    }
}

proc DoAutoUpdate {appName} {
    catch {unset ::appAutoUpdate($appName)}
    Update $appName
    set ::appAutoUpdate($appName) [after $::appAutoInterval "DoAutoUpdate $appName"]
}

proc AutoUpdate {appName} {
    if {[info exists ::appAutoUpdate($appName)]} {
        after cancel $::appAutoUpdate($appName)
        unset ::appAutoUpdate($appName)
    }

    set value [$appName.update.e get]

    if {$value eq "" || $value == 0} {
        return
    }

    set ::appAutoInterval [expr {$value * 1000}]
    DoAutoUpdate $appName
}

proc Start {appName} {
    set app $::applications($appName)
    set name [$app cget -name]
    set ::status "Starting application \"$name\" ..."
    $app start
    set ::status "Started application \"$name\"."
}

proc Stop {appName} {
    set app $::applications($appName)
    set name [$app cget -name]
    set ::status "Stopping application \"$name\" ..."
    $app stop
    set ::status "Stopped application \"$name\"."
}

proc Terminate {appName} {
    set app $::applications($appName)
    set name [$app cget -name]

    set ::status "Terminating application \"$name\" ..."

    if {[info exists ::appAutoUpdate($appName)]} {
        after cancel $::appAutoUpdate($appName)
        unset ::appAutoUpdate($appName)
    }

    unset ::appProps($appName)
    unset ::applications($appName)
    catch {unset ::appAutoUpdate($appName)}
    destroy $appName
    $app releaseObject

    set ::status "Terminated application \"$name\" ..."
}

#
# ----------------------------------------------------------------------
# Application Factory functions.
# ----------------------------------------------------------------------
#

proc Create {afName} {
    set idx 0
    set props [list]
    set propsFrame [$afName.sw.sf getframe]

    foreach p $::afProps($afName) {
        array set prop $p
        set wname "l[incr idx]"
        set type [lindex $prop(value) 0]
        set value [$propsFrame.$wname cget -text]
        if {$value ne ""} {
            lappend props [list id $prop(id) value [list $type $value]]
        }
    }

    set name [$afName.name.e get]
    if {$name eq ""} {error "need name"}
    set ::status "Creating application \"$name\" ..."
    set app [$::appFactories($afName) create $name $props]
    set ::status "Created application \"$name\"."
    MakeAppTop $app
}

proc MakeAppFacTop {af {zipInfo {}}} {
    incr ::uniqueCounter
    set afName ".af${::uniqueCounter}"
    set ::appFactories($afName) $af

    if {[llength $zipInfo]} {
        set ::zipInfo($afName) $zipInfo
    }

    toplevel $afName -width 600 -height 480
    wm title $afName "[$af cget -name] Factory"
    set ::afProps($afName) [$af getConfigurableProperties]

    frame $afName.name
    label $afName.name.l -text "Name"
    entry $afName.name.e -width 40
    pack $afName.name.l -side left -padx 5
    pack $afName.name.e -side left -padx 5 -expand yes -fill x
    pack $afName.name -side top -expand yes -fill x -pady 5 -padx 10

    Separator $afName.sep1 -orient horizontal
    pack $afName.sep1 -side top -fill x -pady 10

    ScrolledWindow $afName.sw -relief sunken
    ScrollableFrame $afName.sw.sf -constrainedwidth 1
    set propsFrame [$afName.sw.sf getframe]
    $afName.sw setwidget $afName.sw.sf

    set idx 0
    foreach p $::afProps($afName) {
        array set prop $p
        set value [lindex $prop(value) 1]
        set wname "l[incr idx]"
        LabelEntry $propsFrame.$wname -label $prop(id) -text $value \
            -labelwidth 20 -width 20 -labelanchor e
        pack $propsFrame.$wname -side top -fill x
    }

    pack $afName.sw -side top -fill both -expand yes

    Separator $afName.sep2 -orient horizontal
    pack $afName.sep2 -side top -fill x -pady 10

    frame $afName.buts
    button $afName.buts.b1 -width 15 -text "Create" -command "Create $afName"
    button $afName.buts.b2 -width 15 -text "Uninstall" -command "Uninstall $afName"
    pack $afName.buts.b1 $afName.buts.b2 -side left -pady 10 -padx 20
    pack $afName.buts
}

proc Uninstall {afName} {
    destroy $afName
    itcl::delete object $::appFactories($afName)
    unset ::appFactories($afName)
    unset ::afProps($afName)
    if {[info exists ::zipInfo($afName)]} {
        eval vfs::zip::Unmount $::zipInfo($afName)
        unset ::zipInfo($afName)
    }
}

#
# ----------------------------------------------------------------------
# Main menu functions.
# ----------------------------------------------------------------------
#

proc ScanNamingContext {} {
    set nc [corba::string_to_object $::ncIor]

    if {[$nc _is_a IDL:CF/DomainManager:1.0]} {
        set eds [cfutil::findExecutableDevicesInDomainManager $nc]
    } else {
        set eds [cfutil::findExecutableDevicesInNamingContext $nc]
    }

    set numEds [llength $eds]

    if {$numEds == 1} {
        set plural ""
    } else {
        set plural "s"
    }

    foreach ed $eds {
        corba::release $ed
    }

    corba::release $nc

    set ::status "${numEds} ExecutableDevice${plural} found."
}

proc UpdateSadCombo {} {
    package require vfs
    package require vfs::zip
    if {[file extension $::appFile] eq ".zip"} {
        set zipFd [vfs::zip::Mount $::appFile /zip]
        set sads [glob -nocomplain -tails -directory /zip {*.[Ss][Aa][Dd]*}]
        vfs::zip::Unmount $zipFd /zip
        .sad.e configure -values $sads
        if {[llength $sads]} {
            set ::sadFile [lindex $sads 0]
        }
    } else {
        .sad.e configure -values [list $::appFile]
        set ::sadFile $::appFile
    }
}

proc BrowseForFile {} {
    set fileTypes {
        {{SAD Files} {.sad.xml}}
        {{ZIP Files} {.zip}}
        {{All Files} *}
    }

    set appFile [tk_getOpenFile -title "Select Application File ..." \
                     -filetypes $fileTypes]

    if {$appFile == ""} {
        continue
    }

    set ::appFile $appFile
    UpdateSadCombo
}

proc Install {} {
    set nc [corba::string_to_object $::ncIor]

    if {[$nc _is_a IDL:CF/DomainManager:1.0]} {
        set eds [cfutil::findExecutableDevicesInDomainManager $nc]
    } else {
        set eds [cfutil::findExecutableDevicesInNamingContext $nc]
    }

    if {[file extension $::appFile] eq ".zip"} {
        package require vfs
        package require vfs::zip
        incr ::uniqueCounter
        set zipMount "/zip${::uniqueCounter}"
        set zipFd [vfs::zip::Mount $::appFile $zipMount]
        set sadFile [file join $zipMount $::sadFile]
        set isZip 1
    } else {
        set isZip 0
        set sadFile $::appFile
    }

    corba::try {
        set af [cfutil::ApplicationFactory \#auto $eds $sadFile $::verbose]
    } catch {... oops} {
        set savedInfo $::errorInfo
        if {$isZip} {
            vfs::zip::Unmount $zipFd $zipMount
        }
        error $oops $savedInfo
    } finally {
        foreach ed $eds {
            corba::release $ed
        }

        corba::release $nc
    }

    if {$isZip} {
        MakeAppFacTop $af [list $zipFd $zipMount]
    } else {
        MakeAppFacTop $af
    }
}

proc Exit {} {
    destroy .
    exit
}

#
# ----------------------------------------------------------------------
# Main menu.
# ----------------------------------------------------------------------
#

wm title . "Application Launchpad"

frame .v
label .v.l -text "Verbosity"
ComboBox .v.v -width 4 -textvariable ::verbose -values {0 1 2 3 4}
pack .v.v -side right -padx 5
pack .v.l -side right -padx 5
pack .v -side top -expand yes -fill x -pady 5 -padx 10

frame .nc
label .nc.l -text "Naming Context" -width 18 -anchor e
entry .nc.e -width 40 -textvariable ::ncIor
button .nc.b -width 10 -text "Scan" -command "ScanNamingContext"
pack .nc.l -side left -padx 5
pack .nc.e -side left -padx 5 -expand yes -fill x
pack .nc.b -side left -padx 5
pack .nc -side top -expand yes -fill x -pady 5 -padx 10

frame .app
label .app.l -text "Application" -width 18 -anchor e
entry .app.e -width 40 -textvariable ::appFile
button .app.b -width 10 -text "Browse" -command "BrowseForFile"
pack .app.l -side left -padx 5
pack .app.e -side left -padx 5 -expand yes -fill x
pack .app.b -side left -padx 5
pack .app -side top -expand yes -fill x -pady 5 -padx 10

frame .sad
label .sad.l -text "SAD File" -width 18 -anchor e
ComboBox .sad.e -width 40 -textvariable ::sadFile
label .sad.d -width 10
pack .sad.l -side left -padx 5
pack .sad.e -side left -padx 5 -expand yes -fill x
pack .sad.d -side left -padx 5
pack .sad -side top -expand yes -fill x -pady 5 -padx 10

Separator .sep1 -orient horizontal
pack .sep1 -side top -fill x -pady 10

frame .buts
button .buts.b1 -width 15 -text "Install" -command "Install"
button .buts.b2 -width 15 -text "Exit" -command "Exit"
pack .buts.b1 .buts.b2 -side left -pady 10 -padx 20
pack .buts

Separator .sep2 -orient horizontal
pack .sep2 -side top -fill x

set ::status ""
label .status -textvariable ::status -anchor w
pack .status -side top -fill x

foreach {::ncIor ::appFile} $argv {}
if {$::appFile ne ""} {UpdateSadCombo}

# for Windows
after 0 "wm deiconify ."
