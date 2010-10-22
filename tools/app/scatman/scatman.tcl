#! /bin/sh

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

# the next line restarts using wish \
exec wish85 "$0" ${1+"$@"}

#
# SCAtMan Version Tag
#

set cvsId {$Id: scatman.tcl,v 1.14 2006/04/10 13:37:46 fpilhofe Exp $}
set cvsRevision {$Revision: 1.14 $}
set cvsDate {$Date: 2006/04/10 13:37:46 $}
set cvsName {$Name:  $}

#
# Load packages.
#

if {[catch {package require Tk}]} {
    puts stderr "Oops: Tk package not found."
    exit 1
}

foreach pd {. .. ../../tcl} {
    set pdp [file join [file dirname [info script]] $pd]

    foreach pkg {bwidget combat} {
        foreach pkd [glob -nocomplain -directory $pdp "${pkg}*"] {
            if {[file exists $pkd] && [file isdirectory $pkd]} {
                lappend auto_path $pkd
            }
        }
    }
}

if {[catch {package require Tcl 8.4}]} {
    wm withdraw .
    tk_messageBox -type ok -icon error -default ok \
        -title "Wrong Tcl/Tk Version" \
        -message "The SDR Control Room requires Tcl/Tk 8.4.\
                This appears to be Tcl/Tk $::tcl_version."
    exit 1
}

if {[catch {package require BWidget}]} {
    wm withdraw .
    tk_messageBox -type ok -icon error -default ok \
        -title "BWidget Required" \
        -message "The SDR Control Room requires the \"BWidget\" package,\
                which does not seem to be available."
    exit 1
}

if {[catch {package require Itcl}]} {
    wm withdraw .
    tk_messageBox -type ok -icon error -default ok \
        -title "\[Incr Tcl\] Required" \
        -message "The SDR Control Room requires the \"\[Incr Tcl\]\" package,\
                which does not seem to be available."
    exit 1
}

if {[catch {package require combat}]} {
    wm withdraw .
    tk_messageBox -type ok -icon error -default ok \
        -title "Combat Required" \
        -message "The SDR Control Room requires the \"Combat\" package,\
                which does not seem to be available."
    exit 1
}

if {![info exists init]} {
    set init 0
}

#
# ----------------------------------------------------------------------
# Splash Screen
# ----------------------------------------------------------------------
#

set ::images(splash) [image create photo -data "
R0lGODlhKgE7APcAAHQCBKx2ZMSylMTOrLSafJQ+NMS6tOTWzJxaTKSenISChNzavNTCrIwiHOTO
vLSmjJROPNzKtKyGbNyynKxqXMSWhOzOvNSinOzizMSKhJQyLOS6pIQSDLx6dLyijJRGPOTGtKRO
TNzOtKxeXLRubOS+tOTezOTCrNTWvMyalOTGvNTWtKyqpIyOjIwqJMSGdNy2rKRyXNSWhOzWxNSq
nIQaFKRGPLx2bLSyrNTOtMSShJQ6NOy6rLSCdHwKBKxiVMzKrMymlNyypOzOxKRWTJSWlJw+PMzC
tPTWzKxaTNzaxNzOxJxWRLRqZPTm3NSejMSGfNSqpOyypLR+bNTOrKymnIyKhJRORNTKxOzi1JQy
NPTGtLRmXPS+tOze1PTGvJQqLNSWjIwaHKRGRNSShMSCdK1aVOzaxNSKfLR2bLy6lMbSrNy+tOva
zNrGrIwiJNLGvMyWjNy6rIQSFJxGROXKtKRSTOTWxLSurJSSlMS2rOy+rIIKDMyulOC2pO/SxJ5C
PLyulLSOdLSihMyelPS2rKRyZKxuZNyelMC+nLSehKyKdOSmnNSKhMx+dKRORMySjLyCfKxWVLRm
ZMSCfPTezOTWvKQ6NNzKrNymlMzCvKympIyKjLRyZHwGBKZ6ZMa+tKiinIWGhOLSvLyqjKRuXOzS
vJRKPNzStLVybObCtOTGrOXKvLSupJSSjIwuJNyunLx6bLy2rNLStJyalOTexN/SxLRuZMWKfNau
pJRSRPTm1JU2NNiajIseHKRKRLBeVI4mJNTKvMyajOS+rIQWFIQODNSilJRCNNSmnMSOhOS+pLx+
dKxiXPTazOy2pPTCtPTKvJQuLOzexMyilHwCBMzOrOzWzNzCrPTOvPTizJwyLJxGPOzGtKxOTOTO
tOzezOzCrNzWvLSqpJSOjJQqJPTWxNyqnIwaFMR2bLyyrMyShJw6NPS6rNTKrNSmlOSypPTOxKxW
TJyWlKQ+POTaxLxqZMyGfJxORPTi1LxmXPTe1NyWjKxGRNyShMyCdCH5BAAAAAAALAAAAAAqATsA
Bwj/AHc5ySeQIMFdCBMqXMiwocOFTnblG1iwYD6DEw1K3Kixo8SMGxF6HFmRIkGTIU2iHKkSI0aJ
FFdyXNnxpcyaNzl+hElyZUueQEle1KkxpsGfOUEeHPqy5sWnUKNKnUqVasiOESfOxHoU69anW4MS
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
dWlMYTqfgAAAOw==
"]

#
# ----------------------------------------------------------------------
# GUI and other Initialization
# ----------------------------------------------------------------------
#

proc Init {} {
    set ::uniquenodeindex 0
    set ::busyMutex 0
}

proc InitImages {} {
    set imgdir [file dirname [info script]]
    set ::images(icon,DomainManager) [image create photo -file [file join $imgdir domainmanager.gif]]
    set ::images(icon,AppFactory) [image create photo -file [file join $imgdir appfactory.gif]]
    set ::images(icon,Application) [image create photo -file [file join $imgdir application.gif]]
    set ::images(icon,DeviceManager) [image create photo -file [file join $imgdir devicemanager.gif]]
    set ::images(icon,Device) [image create photo -file [file join $imgdir device.gif]]
    set ::images(icon,Service) [image create photo -file [file join $imgdir service.gif]]
    set ::images(icon,Resource) [image create photo -file [file join $imgdir component.gif]]
    set ::images(icon,ResFactory) [image create photo -file [file join $imgdir resourcefactory.gif]]
}

proc InitGui {} {
    option add *Button*font {helvetica 10 bold}
    option add *title*font {Arial 16 bold}
    option add *Menu*tearOff 0

    set menudesc {
        "&File" "" "" 0 {
            {command "E&xit" {} "Exit" {Ctrl x} -command "Exit"}
        }
        "&Setup" "" "" 0 {
            {command "&Add Domain Manager from Naming Service" {} "Look up a Domain Manager in a Naming Service" {Ctrl a} -command "AddDomainManager"}
            {command "Add Domain Manager by &URI ..." {} "Add a Domain Manager by its Object URI" {Ctrl u} -command "AddDomainManagerByURI"}
            {separator}
            {command "Reset" {} "Remove all Domain Managers" {} -command "Reset"}
        }
        "&View" "" "" 0 {
            {command "&Refresh" {} "Refresh current page" {f5} -command "RefreshCurrent"}
        }
        "&Help" "" "" 0 {
            {command "&About" {} "About" {f1} -command "About"}
        }
    }

    if {![info exists ::status]} {
        set ::status "Initializing ..."
    }

    wm title . "Mercury SDR ControlRoom"

    set mainframe [MainFrame .mainframe \
                       -textvariable ::status \
                       -menu $menudesc]
    set allframe [$mainframe getframe]

    set pane [PanedWindow $allframe.pane -side top -pad 5]
    set leftpane [$pane add -weight 1]
    set rightpane [$pane add -weight 2]

    # left pane: tree view of domain managers

    set dmsw [ScrolledWindow $leftpane.sw -relief sunken -borderwidth 2]
    set dmtree [Tree $dmsw.tree \
                    -relief flat -borderwidth 0 -highlightthickness 0 \
                    -opencmd "TreeNodeOpen" -closecmd "TreeNodeClose"]
    $dmsw setwidget $dmtree

    $dmtree bindText <ButtonPress-1> "TreeNodeSelect"
    $dmtree bindImage <ButtonPress-1> "TreeNodeSelect"
    $dmtree bindText <ButtonPress-3> "TreeNodePopup"
    $dmtree bindImage <ButtonPress-3> "TreeNodePopup"

    pack $dmsw -side top -expand yes -fill both

    # right pane: item information

    #
    # show splash screen
    #

    frame $rightpane.bg -bg "\#f4e4d5"
    label $rightpane.bg.splash -image $::images(splash) -bg "\#f4e4d5" \
         -width 500 -height 400 -anchor center
    pack $rightpane.bg.splash -fill both -expand yes -anchor center
    pack $rightpane.bg -fill both -expand yes -anchor center

    #
    # finalize
    #

    pack $pane -side top -expand yes -fill both -padx 5 -pady 5
    pack $mainframe -fill both -expand yes

    #
    # remember widgets
    #

    set ::widgets(main) $mainframe
    set ::widgets(tree) $dmtree
    set ::widgets(rightpane) $rightpane

    #
    # bindings
    #

    bind . <Control-L> "Reload"
    bind . <Control-R> "Refresh"
    bind . <Control-C> "ToggleConsole"
    bind . <Control-q> "Exit"
}

#
# ----------------------------------------------------------------------
# Init GUI right away, so that the user has something to look at while
# the rest of the app is loading
# ----------------------------------------------------------------------
#

if {!$init} {
    set ::status "Loading ..."
    InitGui
    update
}

#
# ----------------------------------------------------------------------
# Tree Management: Select a node
# ----------------------------------------------------------------------
#

proc TreeNodeSelect {node} {
    if {$::busyMutex} {
        return
    }

    set ::busyMutex 1

    if {![info exists ::widgets(title)]} {
        #
        # recover from splash screen
        #

        set rightpane $::widgets(rightpane)

        foreach widget [winfo children $rightpane] {
            destroy $widget
        }

        set infotitle [label $rightpane.title]
        set infoframe [frame $rightpane.frame -height 300 -width 500]
        pack $infotitle -side top -fill x -padx 10 -pady 10
        pack $infoframe -side top -expand yes -fill both
        set ::widgets(title) $infotitle
        set ::widgets(info) $infoframe
    }

    set oldSelection [$::widgets(tree) selection get]
    if {[llength $oldSelection]} {
        set oldNode [lindex $oldSelection 0]
        set oldData [$::widgets(tree) itemcget $oldNode -data]
        set oldType [lindex $oldData 0]
    }

    focus $::widgets(tree)
    $::widgets(tree) selection set $node
    $::widgets(tree) see $node

    set data [$::widgets(tree) itemcget $node -data]
    set type [lindex $data 0]
    set ref  [lindex $data 1]

    if {[llength $oldSelection] && $type == $oldType} {
        set reuse 1
    } else {
        set reuse 0

        foreach widget [winfo children $::widgets(info)] {
            destroy $widget
        }
    }

    if {[catch {
        switch -- $type {
            DomainManager {
                DomainManager::ShowInfo $node $ref $reuse
            }
            ApplicationFactory {
                ApplicationFactory::ShowInfo $node $ref $reuse
            }
            Application {
                Application::ShowInfo $node $ref $reuse
            }
            Component {
                Component::ShowInfo $node $ref $reuse
            }
            DeviceManager {
                DeviceManager::ShowInfo $node $ref $reuse
            }
            Device {
                Device::ShowInfo $node $ref $reuse
            }
            Service {
                Service::ShowInfo $node $ref $reuse
            }
            FileManager {
                FileManager::ShowInfo $node $ref $reuse
            }
            FileSystem {
                FileSystem::ShowInfo $node $ref $reuse
            }
            Directory {
                Directory::ShowInfo $node $ref $reuse
            }
        }
    } oops]} {
        set ::status $oops
    }

    set ::busyMutex 0
}

#
# ----------------------------------------------------------------------
# Tree Management: Open/Close a node
# ----------------------------------------------------------------------
#

proc UpdateChildren {node} {
    if {$::busyMutex} {
        return
    }

    set ::busyMutex 1

    set data [$::widgets(tree) itemcget $node -data]
    set type [lindex $data 0]
    set ref  [lindex $data 1]
    
    $::widgets(tree) delete [$::widgets(tree) nodes $node]

    if {[catch {
        switch -- $type {
            DomainManager {
                DomainManager::UpdateChildren $node $ref
            }
            DeviceManager {
                DeviceManager::UpdateChildren $node $ref
            }
            Application {
                Application::UpdateChildren $node $ref
            }
            FileManager -
            FileSystem -
            Directory {
                Directory::UpdateChildren $node $ref
            }
        }
    } oops]} {
        set ::status $oops
    }

    set ::busyMutex 0
}

proc TreeNodeOpen {node} {
    if {$::busyMutex} {
        return
    }

    TreeNodeSelect $node

    if {[$::widgets(tree) itemcget $node -drawcross] != "auto"} {
        $::widgets(tree) itemconfigure $node -drawcross auto
        UpdateChildren $node
    }
}

proc TreeNodeClose {node} {
    if {$::busyMutex} {
        return
    }

    TreeNodeSelect $node
}

#
# ----------------------------------------------------------------------
# Tree Management: show a popup
# ----------------------------------------------------------------------
#

proc TreeNodePopup {node} {
    if {$::busyMutex} {
        return
    }

    set oldSelection [$::widgets(tree) selection get]

    if {[llength $oldSelection] == 0 || [lindex $oldSelection 0] != $node} {
        TreeNodeSelect $node
    }

    set xpos [expr [winfo pointerx .] + 5]
    set ypos [expr [winfo pointery .] + 5]

    set data [$::widgets(tree) itemcget $node -data]
    set type [lindex $data 0]

    switch -- $type {
        DomainManager {
            DomainManager::Popup $xpos $ypos
        }
        ApplicationFactory {
            ApplicationFactory::Popup $xpos $ypos
        }
        Application {
            Application::Popup $xpos $ypos
        }
        DeviceManager {
            DeviceManager::Popup $xpos $ypos
        }
        Device {
            Device::Popup $xpos $ypos
        }
        FileManager -
        FileSystem -
        Directory {
            Directory::Popup $xpos $ypos
        }
    }
}

#
# ----------------------------------------------------------------------
# Refresh current item
# ----------------------------------------------------------------------
#

proc RefreshCurrent {} {
    set selection [$::widgets(tree) selection get]
    if {[llength $selection]} {
        set node [lindex $selection 0]
        TreeNodeSelect $node
        UpdateChildren $node
    }
}

#
# ----------------------------------------------------------------------
# DomainManager
# ----------------------------------------------------------------------
#

namespace eval DomainManager {}

proc DomainManager::UpdateChildren {node dmRef} {
    set name [$::widgets(tree) itemcget $node -text]
    set data [$::widgets(tree) itemcget $node -data]
    set fileMgr [lindex $data 2]

    set ::status "Loading ApplicationFactories for DomainManager $name ..."
    update

    corba::try {
        set factories [$dmRef applicationFactories]
    } catch {... ex} {
        set ::status "Error: cannot talk to DomainManager $name: [lindex $ex 0]"
        return
    }

    foreach factory $factories {
        corba::try {
            set afid [$factory name]
        } catch {...} {
            set afid "Unknown"
        }

        set nodename "node[incr ::uniquenodeindex]"
        $::widgets(tree) insert end $node $nodename \
            -open 0 -drawcross never \
            -image $::images(icon,AppFactory) -text $afid \
            -data [list ApplicationFactory $factory]
    }

    set ::status "Loading applications for DomainManager $name ..."
    update

    corba::try {
        set applications [$dmRef applications]
    } catch {...} {
        set ::status "Error: cannot access applications for DomainManager $name."
        set applications [list]
    }

    foreach application $applications {
        corba::try {
            set aname [$application name]
        } catch {...} {
            set aname "Unknown"
        }
        
        set nodename "node[incr ::uniquenodeindex]"
        $::widgets(tree) insert end $node $nodename \
            -open 0 -drawcross allways \
            -image $::images(icon,Application) -text $aname \
            -data [list Application $application]
    }

    set ::status "Loading DeviceManagers for DomainManager $name ..."
    update

    corba::try {
        set devicemanagers [$dmRef deviceManagers]
    } catch {...} {
        set ::status "Error: cannot access DeviceManagers for DomainManager $name."
        set devicemanagers [list]
    }

    foreach devicemanager $devicemanagers {
        corba::try {
            set dmname [$devicemanager identifier]
        } catch {...} {
            set dmname "Unknown"
        }
        
        set nodename "node[incr ::uniquenodeindex]"
        $::widgets(tree) insert end $node $nodename \
            -open 0 -drawcross allways \
            -image $::images(icon,DeviceManager) -text $dmname \
            -data [list DeviceManager $devicemanager]
    }

    set ::status "Loading File Manager for Domain Manager $name ..."
    update

    if {$fileMgr != 0} {
        set nodename "node[incr ::uniquenodeindex]"
        $::widgets(tree) insert end $node $nodename \
            -open 0 -drawcross allways \
            -image [Bitmap::get folder] -text "File Manager" \
            -data [list FileManager $fileMgr "/"]
    }

    set ::status "Loaded information for Domain Manager $name."
}

proc DomainManager::ShowInfo {node dmRef reuse} {
    set name [$::widgets(tree) itemcget $node -text]
    set data [$::widgets(tree) itemcget $node -data]
    set fileMgr [lindex $data 2]

    set ::status "Updating info for Domain Manager $name ..."
    update

    set info $::widgets(info)
    $::widgets(title) configure -text "Domain Manager"

    if {!$reuse} {
        LabelEntry $info.id -label "Identifier" \
            -labelwidth 16 -width 40 -labelanchor w
        LabelEntry $info.prof -label "Profile File" \
            -labelwidth 16 -width 40 -labelanchor w

        pack $info.id -side top -fill x -pady 5
        pack $info.prof -side top -fill x -pady 5

        set proptitle [TitleFrame $info.props -text "Properties"] 
        pack $proptitle -side top -fill both -expand yes -pady 5
    } else {
        bind $info.prof.e <Double-Button-1> ""
    }

    set propcont [$info.props getframe]

    corba::try {
        set identifier [$dmRef identifier]
        set profile [$dmRef domainManagerProfile]
        set props [list]
        $dmRef query props
    } catch {... ex} {
        set ::status "Error: cannot talk to DomainManager: [lindex $ex 0]"
        return
    }

    $info.id configure -text $identifier

    if {[string range $profile 0 7] == "<profile" && \
            [regexp {filename(?: )*=(?: )*\"([^\"]*)\"} $profile match fileName] && \
            [regexp {type(?: )*=(?: )*\"([^\"]*)\"} $profile match type]} {
        $info.prof configure -label "Profile File" -text $fileName
        if {$fileMgr != 0} {
            bind $info.prof.e <Double-Button-1> "Directory::DisplayFile $fileMgr \"$fileName\""
        }
    } else {
        $info.prof configure -label "Profile" -text $profile
    }

    ShowProperties $dmRef $propcont $props
    set ::status "Information for Domain Manager $name updated."
}

proc DomainManager::Popup {xpos ypos} {
    if {![info exists ::widgets(popup,DomainManager)]} {
        set ::widgets(popup,DomainManager) [menu .popupForDomMan]
        $::widgets(popup,DomainManager) add command \
            -label "Install Application" \
            -command "DomainManager::InstallApplication"
        $::widgets(popup,DomainManager) add command \
            -label "Show Incoming Events" \
            -command "DomainManager::ShowEvents in"
        $::widgets(popup,DomainManager) add command \
            -label "Show Outgoing Events" \
            -command "DomainManager::ShowEvents out"
        $::widgets(popup,DomainManager) add separator
        $::widgets(popup,DomainManager) add command \
            -label "Refresh" \
            -command "DomainManager::Refresh"
        $::widgets(popup,DomainManager) add command \
            -label "Remove" \
            -command "DomainManager::Remove"
    }

    tk_popup $::widgets(popup,DomainManager) $xpos $ypos
}

proc DomainManager::InstallApplication {} {
    ::InstallApplication [lindex [$::widgets(tree) selection get] 0]
}

proc DomainManager::ShowEvents {inout} {
    set node [lindex [$::widgets(tree) selection get] 0]
    set data [$::widgets(tree) itemcget $node -data]
    set type [lindex $data 0]
    set dmRef [lindex $data 1]

    if {![info exists ::evcount]} {
        set ::evcount 0
    } else {
        incr ::evcount
    }

    set guid "scatman-[pid]-$::evcount"
    set poa [corba::resolve_initial_references RootPOA]
    set mgr [$poa the_POAManager]
    $mgr hold_requests 1

    set srv [DMEventConsumer \#auto]
    set ref [$srv _this]

    if {$inout == "in"} {
        set direction "Incoming"
        set channelName "IDM_Channel"
    } else {
        set direction "Outgoing"
        set channelName "ODM_Channel"
    }


    set ::status "Connecting to $direction Domain Management event channel ..."

    corba::try {
        set identifier [$dmRef identifier]
        $dmRef registerWithEventChannel $ref $guid $channelName
    } catch {... ex} {
        set ::status "Error: cannot register with event channel: [lindex $ex 0]"
        set oid [$poa servant_to_id $srv]
        $poa deactivate_object $oid
        delete object $srv
        $mgr activate
    }

    set top .events$::evcount
    toplevel $top
    wm title $top "$direction Events for DomainManager $identifier"

    frame $top.buts
    set but1 [button $top.buts.b1 -width 15 -text "Close"]
    pack $but1 -side left -pady 10 -padx 20
    pack $top.buts -side bottom

    set eventsw [ScrolledWindow $top.sw -relief sunken -auto vertical -borderwidth 1]
    set eventtxt [text $eventsw.text -font {Courier 8} -wrap none]
    $eventsw setwidget $eventtxt
    pack $eventsw -fill both -expand yes

    $but1 configure -command "[namespace current]::$srv destroy"

    $srv configure -channelName $channelName \
        -guid $guid -domainManager $dmRef \
        -toplevel $top -textWidget $eventtxt

    $mgr activate

    set ::status "Connected to $direction Domain Management event channel."
}

catch {
    itcl::delete class DMEventConsumer
}

itcl::class DMEventConsumer {
    inherit PortableServer::ServantBase

    public variable guid
    public variable domainManager
    public variable toplevel
    public variable textWidget
    public variable channelName

    constructor {} {
        set domainManager 0
    }

    public method _Interface {} {
        return "IDL:omg.org/CosEventComm/PushConsumer:1.0"
    }

    public method push {data} {
        set time [clock format [clock seconds] -format "%Y.%m.%d %H:%M:%S"]
        set eventtc [lindex $data 0]
        set eventval [lindex $data 1]

        if {[lindex $eventtc 0] == "struct"} {
            set repid [lindex [lindex $eventtc 1] 0]
            array set values $eventval

            switch -exact -- $repid {
                IDL:StandardEvent/StateChangeEventType:1.0 {
                    $textWidget insert end "\n*** $time *** State Change Event ***\n\n"
                    $textWidget insert end " Producer: $values(producerId)\n"
                    $textWidget insert end "   Source: $values(sourceId)\n"
                    $textWidget insert end " Category: $values(stateChangeCategory)\n"
                    $textWidget insert end "Old State: $values(stateChangeFrom)\n"
                    $textWidget insert end "New State: $values(stateChangeTo)\n"
                }
                IDL:StandardEvent/DomainManagementObjectRemovedEventType:1.0 {
                    $textWidget insert end "\n*** $time *** Domain Management Object Removed ***\n\n"
                    $textWidget insert end " Producer: $values(producerId)\n"
                    $textWidget insert end "   Source: $values(sourceId)\n"
                    $textWidget insert end "     Name: $values(sourceName)\n"
                    $textWidget insert end " Category: $values(sourceCategory)\n"
                }
                IDL:StandardEvent/DomainManagementObjectAddedEventType:1.0 {
                    $textWidget insert end "\n*** $time *** Domain Management Object Added ***\n\n"
                    $textWidget insert end " Producer: $values(producerId)\n"
                    $textWidget insert end "   Source: $values(sourceId)\n"
                    $textWidget insert end "     Name: $values(sourceName)\n"
                    $textWidget insert end " Category: $values(sourceCategory)\n"
                }
                default {
                    $textWidget insert end "\n*** $time *** Event ***\n\n"
                    $textWidget insert end "     Type: $repid\n"

                    foreach field [array names values] {
                        $textWidget insert end "$field: $values($field)\n"
                    }
                }
            }

            array unset values
        } else {
            $textWidget insert end "\n*** $time *** Event ***\n\n"
            $textWidget insert end "     Type: $eventtc"
            $textWidget insert end "    Value: $eventval"
        }
    }

    public method disconnect_push_consumer {} {
        set domainManager 0
        set time [clock format [clock seconds] -format "%Y.%m.%d %H:%M:%S"]
        $textWidget insert end "\n*** $time *** Disconnected ***\n"
    }

    public method destroy {} {
        if {$domainManager != 0} {
            corba::try {
                $domainManager unregisterFromEventChannel $guid $channelName
            }
        }

        set poa [corba::resolve_initial_references RootPOA]
        set oid [$poa servant_to_id $this]
        $poa deactivate_object $oid
        ::destroy $toplevel
        itcl::delete object $this
    }
}

proc DomainManager::Refresh {} {
    ::UpdateChildren [lindex [$::widgets(tree) selection get] 0]
}

proc DomainManager::Remove {} {
    $::widgets(tree) delete [lindex [$::widgets(tree) selection get] 0]
}

#
# ----------------------------------------------------------------------
# Application Factory
# ----------------------------------------------------------------------
#

namespace eval ApplicationFactory {}

proc ApplicationFactory::ShowInfo {node afRef reuse} {
    set name [$::widgets(tree) itemcget $node -text]
    set ::status "Updating info for Application Factory $name ..."
    update

    set info $::widgets(info)

    $::widgets(title) configure -text "Application Factory"

    if {!$reuse} {
        LabelEntry $info.id -label "Identifier" \
            -labelwidth 16 -width 40 -labelanchor w
        LabelEntry $info.name -label "Application Type" \
            -labelwidth 16 -width 40 -labelanchor w
        LabelEntry $info.prof -label "Profile File" \
            -labelwidth 16 -width 40 -labelanchor w

        pack $info.id $info.name -side top -fill x -pady 5
        pack $info.prof -side top -fill x -pady 5
    } else {
        bind $info.prof.e <Double-Button-1> ""
    }

    corba::try {
        set identifier [$afRef identifier]
        set profile [$afRef softwareProfile]
        set name [$afRef name]
    } catch {... ex} {
        set ::status "Error: cannot talk to ApplicationFactory: [lindex $ex 0]"
        return
    }

    corba::try {
        set parent [$::widgets(tree) parent $node]
        set pdata [$::widgets(tree) itemcget $parent -data]
        set ptype [lindex $pdata 0]
        set pref [lindex $pdata 1]

        if {$ptype == "DomainManager"} {
            set fileMgr [lindex $pdata 2]
        } else {
            set fileMgr 0
        }
    } catch {... ex} {
        set fileMgr 0
    }

    $info.id configure -text $identifier
    $info.name configure -text $name

    if {[string range $profile 0 7] == "<profile" && \
            [regexp {filename(?: )*=(?: )*\"([^\"]*)\"} $profile match fileName] && \
            [regexp {type(?: )*=(?: )*\"([^\"]*)\"} $profile match type]} {
        $info.prof configure -label "Profile File" -text $fileName
        if {$fileMgr != 0} {
            bind $info.prof.e <Double-Button-1> "Directory::DisplayFile $fileMgr \"$fileName\""
        }
    } else {
        $info.prof configure -label "Software Profile" -text $profile
    }

    set ::status "Updated information for Application Factory $name."
}

proc ApplicationFactory::Popup {xpos ypos} {
    if {![info exists ::widgets(popup,ApplicationFactory)]} {
        set ::widgets(popup,ApplicationFactory) [menu .popupForAppFac]
        $::widgets(popup,ApplicationFactory) add command \
            -label "Create" \
            -command "ApplicationFactory::CreateApplication"
        $::widgets(popup,ApplicationFactory) add command \
            -label "Uninstall" \
            -command "ApplicationFactory::UninstallApplication"
    }

    tk_popup $::widgets(popup,ApplicationFactory) $xpos $ypos
}

proc ApplicationFactory::CreateApplication {} {
    set node [lindex [$::widgets(tree) selection get] 0]
    set parent [$::widgets(tree) parent $node]
    set data [$::widgets(tree) itemcget $node -data]
    set type [lindex $data 0]
    set ref  [lindex $data 1]

    set top .createApp

    if {[winfo exists $top]} {
        wm deiconify $top
        focus $top
        return
    }

    if {![info exists ::appname]} {
        set ::appname ""
    }

    toplevel $top
    wm title $top "Create Application"

    set title [label $top.title -anchor center \
                   -text "Create Application"]
    pack $title -side top -fill x -pady 10

    set sep1 [Separator $top.sep1 -orient horizontal]
    pack $sep1 -side top -fill x -pady 10

    LabelEntry $top.name -label "Name" \
        -labelwidth 18 -width 40 -labelanchor w \
        -textvariable ::appname
    pack $top.name -side top -expand yes -fill x -pady 5 -padx 10

    set sep2 [Separator $top.sep2 -orient horizontal]
    pack $sep2 -side top -fill x -pady 10

    frame $top.buts
    set but1 [button $top.buts.b1 -width 15 -text "OK" \
                  -command "set ::createappguimutex 1"]
    set but2 [button $top.buts.b2 -width 15 -text "Cancel" \
                  -command "set ::createappguimutex 2"]
    pack $but1 $but2 -side left -pady 10 -padx 20
    pack $top.buts

    focus $top.name.e
    bind $top.name.e <Return> "set ::createappguimutex 1"
    bind $top.buts.b1 <Return> "set ::createappguimutex 1"
    bind $top.buts.b2 <Return> "set ::createappguimutex 2"

    set ::createappguimutex 0
    vwait ::createappguimutex
    destroy $top

    if {$::createappguimutex != 1} {
        return
    }

    set ::status "Creating application $::appname ..."
    update

    set appRef 0
    corba::try {
        set appRef [$ref create $::appname [list] [list]]
    } catch {... ex} {
        set ::status "Error creating application: [lindex $ex 0]"
        tk_messageBox -parent . -type ok -icon error -default ok \
            -title "Error creating application" \
            -message $ex
        return
    }

    corba::try {
        set aname [$appRef name]
    } catch {...} {
        set aname "Unknown"
    }

    set ::status "Created application $aname."

    set nodename "node[incr ::uniquenodeindex]"
    $::widgets(tree) insert end $parent $nodename \
        -open 0 -drawcross allways \
        -image $::images(icon,Application) -text $aname \
        -data [list Application $appRef]
}

proc ApplicationFactory::UninstallApplication {} {
    set node [lindex [$::widgets(tree) selection get] 0]
    set data [$::widgets(tree) itemcget $node -data]
    set type [lindex $data 0]
    set ref  [lindex $data 1]

    set ::status "Introspecting ApplicationFactory identifier ..."
    update

    corba::try {
        set identifier [$ref identifier]
    } catch {... ex} {
        set ::status "Error: cannot talk to ApplicationFactory: [lindex $ex 0]"
        return
    }

    set parent [$::widgets(tree) parent $node]
    set pdata  [$::widgets(tree) itemcget $parent -data]
    set dmref  [lindex $pdata 1]

    set ::status "Uninstalling ApplicationFactory $identifier ..."

    corba::try {
        $dmref uninstallApplication $identifier
    } catch {... ex} {
        set ::status "Error uninstalling application: [lindex $ex 0]"
        tk_messageBox -parent . -type ok -icon error -default ok \
            -title "Error uninstalling application" \
            -message $ex
        return
    }

    set ::status "Uninstalled ApplicationFactory $identifier."

    foreach widget [winfo children $::widgets(info)] {
        destroy $widget
    }

    $::widgets(tree) delete $node
}

#
# ----------------------------------------------------------------------
# Application
# ----------------------------------------------------------------------
#

namespace eval Application {}

proc Application::UpdateChildren {node appRef} {
    set name [$::widgets(tree) itemcget $node -text]

    set ::status "Loading Components for Application $name ..."
    update

    corba::try {
        set namingContexts [$appRef componentNamingContexts]
    } catch {... ex} {
        set ::status "Error: cannot talk to Application $name: [lindex $ex 0]"
        return
    }

    set count 0
    foreach namingContext $namingContexts {
        set componentId [lindex $namingContext 1]
        set thisNamingContext [lindex $namingContext 3]

        corba::try {
            set componentRef [$::NamingService resolve_str [string range $thisNamingContext 1 end]]
            if {[$componentRef _is_a IDL:CF/ResourceFactory:1.0]} {
                set icon $::images(icon,ResFactory)
            } else {
                set icon $::images(icon,Resource)
            }
        } catch {... ex} {
            # set ::status "Error: cannot talk to Naming Service or Component: $componentId"
            set icon $::images(icon,Resource)
        }
        
        set nodename "node[incr ::uniquenodeindex]"
        $::widgets(tree) insert end $node $nodename \
            -open 0 -drawcross never \
            -image $icon -text $componentId \
            -data [list Component $appRef $componentId]
        incr count
    }

    set ::status "The application has $count component(s)."
}

proc Application::ShowInfo {node afRef reuse} {
    set name [$::widgets(tree) itemcget $node -text]
    set ::status "Updating info for Application $name ..."
    update

    set info $::widgets(info)

    $::widgets(title) configure -text "Application"

    if {!$reuse} {
        LabelEntry $info.id -label "Identifier" \
            -labelwidth 16 -width 40 -labelanchor w
        LabelEntry $info.name -label "Application Name" \
            -labelwidth 16 -width 40 -labelanchor w
        pack $info.id $info.name -side top -fill x -pady 5

        LabelEntry $info.prof -label "Profile File" \
            -labelwidth 16 -width 40 -labelanchor w
        pack $info.prof -side top -fill x -pady 5

        set proptitle [TitleFrame $info.props -text "Properties"]
        pack $proptitle -side top -fill both -expand yes -pady 5
    } else {
        bind $info.prof.e <Double-Button-1> ""
    }

    set propcont [$info.props getframe]

    corba::try {
        set identifier [$afRef identifier]
        set profile [$afRef profile]
        set name [$afRef name]
        set props [list]
        $afRef query props
    } catch {... ex} {
        set ::status "Error: cannot talk to Application: [lindex $ex 0]"
        return
    }

    corba::try {
        set parent [$::widgets(tree) parent $node]
        set pdata [$::widgets(tree) itemcget $parent -data]
        set ptype [lindex $pdata 0]
        set pref [lindex $pdata 1]

        if {$ptype == "DomainManager"} {
            set fileMgr [lindex $pdata 2]
        } else {
            set fileMgr 0
        }
    } catch {... ex} {
        set fileMgr 0
    }

    $info.id configure -text $identifier
    $info.name configure -text $name

    if {[string range $profile 0 7] == "<profile" && \
            [regexp {filename(?: )*=(?: )*\"([^\"]*)\"} $profile match fileName] && \
            [regexp {type(?: )*=(?: )*\"([^\"]*)\"} $profile match type]} {
        $info.prof configure -label "Profile File" -text $fileName

        if {$fileMgr != 0} {
            bind $info.prof.e <Double-Button-1> "Directory::DisplayFile $fileMgr \"$fileName\""
        }
    } else {
        $info.prof configure -label "Software Profile" -text $profile
    }

    ShowProperties $afRef $propcont $props
    set ::status "Updated information for Application $name."
}

proc Application::Popup {xpos ypos} {
    if {![info exists ::widgets(popup,Application)]} {
        set ::widgets(popup,Application) [menu .popupForApp]
        $::widgets(popup,Application) add command \
            -label "Start" \
            -command "Application::Start"
        $::widgets(popup,Application) add command \
            -label "Stop" \
            -command "Application::Stop"
        $::widgets(popup,Application) add separator
        $::widgets(popup,Application) add command \
            -label "Terminate" \
            -command "Application::Terminate"
    }

    tk_popup $::widgets(popup,Application) $xpos $ypos
}

proc Application::Start {} {
    set node [lindex [$::widgets(tree) selection get] 0]
    set name [$::widgets(tree) itemcget $node -text]
    set data [$::widgets(tree) itemcget $node -data]
    set type [lindex $data 0]
    set ref  [lindex $data 1]

    set ::status "Starting Application $name ..."
    update

    corba::try {
        $ref start
    } catch {... ex} {
        set ::status "Error starting application: [lindex $ex 0]"
        tk_messageBox -parent . -type ok -icon error -default ok \
            -title "Error starting application" \
            -message $ex
        return
    }

    set ::status "Application $name started."
}

proc Application::Stop {} {
    set node [lindex [$::widgets(tree) selection get] 0]
    set name [$::widgets(tree) itemcget $node -text]
    set data [$::widgets(tree) itemcget $node -data]
    set type [lindex $data 0]
    set ref  [lindex $data 1]

    set ::status "Stopping Application $name ..."
    update

    corba::try {
        $ref stop
    } catch {... ex} {
        set ::status "Error stopping application: [lindex $ex 0]"
        tk_messageBox -parent . -type ok -icon error -default ok \
            -title "Error stopping application" \
            -message $ex
        return
    }

    set ::status "Application $name stopped."
}

proc Application::Terminate {} {
    set node [lindex [$::widgets(tree) selection get] 0]
    set name [$::widgets(tree) itemcget $node -text]
    set data [$::widgets(tree) itemcget $node -data]
    set type [lindex $data 0]
    set ref  [lindex $data 1]

    set ::status "Releasing Application $name ..."
    update

    corba::try {
        $ref releaseObject
    } catch {... ex} {
        set ::status "Error releasing application: [lindex $ex 0]"
        tk_messageBox -parent . -type ok -icon error -default ok \
            -title "Error terminating application" \
            -message $ex
        # fall through
    }

    set ::status "Application $name terminated."

    foreach widget [winfo children $::widgets(info)] {
        destroy $widget
    }

    $::widgets(tree) delete $node
}

#
# ----------------------------------------------------------------------
# Component
# ----------------------------------------------------------------------
#

namespace eval Component {}

proc Component::ShowInfo {node appRef reuse} {
    set name [$::widgets(tree) itemcget $node -text]
    set ::status "Updating info for Component $name ..."

    set componentId [lindex [$::widgets(tree) itemcget $node -data] 2]

    corba::try {
        set namingContexts [$appRef componentNamingContexts]
        set processIds [$appRef componentProcessIds]
        set devices [$appRef componentDevices]
        set implementations [$appRef componentImplementations]
    } catch {... ex} {
        set ::status "Error: cannot talk to Application: [lindex $ex 0]"
        return
    }

    set thisNamingContext "unknown"
    set thisProcessId "unknown"
    set thisDevice "unknown"
    set thisImplementation "unknown"

    foreach el $namingContexts {
        if {[lindex $el 1] == $componentId} {
            set thisNamingContext [lindex $el 3]
            break
        }
    }

    foreach el $processIds {
        if {[lindex $el 1] == $componentId} {
            set thisProcessId [lindex $el 3]
            break
        }
    }

    foreach el $devices {
        if {[lindex $el 1] == $componentId} {
            set thisDevice [lindex $el 3]
            break
        }
    }

    foreach el $implementations {
        if {[lindex $el 1] == $componentId} {
            set thisImplementation [lindex $el 3]
            break
        }
    }

    set info $::widgets(info)

    if {!$reuse} {
        LabelEntry $info.id -label "Identifier" \
            -labelwidth 16 -width 40 -labelanchor w
        LabelEntry $info.nc -label "Naming Context" \
            -labelwidth 16 -width 40 -labelanchor w

        #
        # FP 02/15/06: Murat doesn't want to see device, process id or implementation
        #

        if {0} {
            LabelEntry $info.dev -label "Device" \
                -labelwidth 16 -width 40 -labelanchor w
            LabelEntry $info.pid -label "ProcessId" \
                -labelwidth 16 -width 40 -labelanchor w
            LabelEntry $info.impl -label "Implementation" \
                -labelwidth 16 -width 40 -labelanchor w
        }

        pack $info.id $info.nc -side top -fill x -pady 5

        if {0} {
            pack $info.dev $info.pid $info.impl  -side top -fill x -pady 5
        }

        set proptitle [TitleFrame $info.props -text "Properties"]
        pack $proptitle -side top -fill both -expand yes -pady 5
    }

    set propcont [$info.props getframe]

    corba::try {
        set componentRef [$::NamingService resolve_str [string range $thisNamingContext 1 end]]
        if {[$componentRef _is_a IDL:CF/ResourceFactory:1.0]} {
            set componentType "Resource Factory"
            set props [list]
            set componentRef 0
        } elseif {[$componentRef _is_a IDL:CF/Resource:1.0]} {
            set componentType "Resource"
            set props [list]
            $componentRef query props
        }
    } catch {... ex} {
        # set ::status "Error: cannot talk to Naming Service or Component: $componentId"
        set componentRef 0
        set componentType "Component"
        set props [list]
    }

    $::widgets(title) configure -text $componentType
    $info.id configure -text $componentId
    $info.nc configure -text $thisNamingContext

    if {0} {
        $info.dev configure -text $thisDevice
        $info.pid configure -text $thisProcessId
        $info.impl configure -text $thisImplementation
    }

    if {$componentRef != 0} {
        ShowProperties $componentRef $propcont $props
    }

    set ::status "Updated information for Component $name."
}

#
# ----------------------------------------------------------------------
# Device Manager
# ----------------------------------------------------------------------
#

namespace eval DeviceManager {}

proc DeviceManager::UpdateChildren {node dmRef} {
    set name [$::widgets(tree) itemcget $node -text]

    set ::status "Loading devices for Device Manager $name ..."
    update

    corba::try {
        set devices [$dmRef registeredDevices]
    } catch {... ex} {
        set ::status "Error: cannot talk to Device Manager $name: [lindex $ex 0]"
        return
    }

    foreach device $devices {
        corba::try {
            set devid [$device identifier]
        } catch {...} {
            set devid "Unknown"
        }
        
        set nodename "node[incr ::uniquenodeindex]"
        $::widgets(tree) insert end $node $nodename \
            -open 0 -drawcross never \
            -image $::images(icon,Device) -text $devid \
            -data [list Device $device]
    }

    set ::status "Loading services for Device Manager $name ..."
    update

    corba::try {
        set services [$dmRef registeredServices]
    } catch {... ex} {
        set ::status "Error: cannot talk to Device Manager $name: [lindex $ex 0]"
        return
    }

    foreach service $services {
        set serviceObject [lindex $service 1]
        set serviceName [lindex $service 3]
        
        set nodename "node[incr ::uniquenodeindex]"
        $::widgets(tree) insert end $node $nodename \
            -open 0 -drawcross never \
            -image $::images(icon,Service) -text $serviceName \
            -data [list Service $serviceObject]
    }

    set ::status "Loading file system for Device Manager $name ..."
    update

    corba::try {
        set fs [$dmRef fileSys]
    } catch {...} {
        set ::status "Error: cannot access file system for Device Manager $name."
        set fs 0
    }

    if {$fs != 0} {
        set nodename "node[incr ::uniquenodeindex]"
        $::widgets(tree) insert end $node $nodename \
            -open 0 -drawcross allways \
            -image [Bitmap::get folder] -text "File System" \
            -data [list FileSystem $fs "/"]
    }

    set ::status "Loaded devices for Device Manager $name."
}

proc DeviceManager::ShowInfo {node dmRef reuse} {
    set name [$::widgets(tree) itemcget $node -text]
    set ::status "Updating info for Device Manager $name ..."
    update

    set info $::widgets(info)
    $::widgets(title) configure -text "Device Manager"

    if {!$reuse} {
        LabelEntry $info.id -label "Identifier" \
            -labelwidth 16 -width 40 -labelanchor w
        LabelEntry $info.name -label "Label" \
            -labelwidth 16 -width 40 -labelanchor w
        pack $info.id $info.name -side top -fill x -pady 5
        LabelEntry $info.prof -label "Profile File" \
            -labelwidth 16 -width 40 -labelanchor w
        pack $info.prof -side top -fill x -pady 5
        set proptitle [TitleFrame $info.props -text "Properties"] 
        pack $proptitle -side top -fill both -expand yes -pady 5
    } else {
        bind $info.prof.e <Double-Button-1> ""
    }

    set propcont [$info.props getframe]

    corba::try {
        set identifier [$dmRef identifier]
        set profile [$dmRef deviceConfigurationProfile]
        set label [$dmRef label]
        set fileSys [$dmRef fileSys]
        set props [list]
        $dmRef query props
    } catch {... ex} {
        set ::status "Error: cannot talk to Device Manager: [lindex $ex 0]"
        return
    }

    $info.id configure -text $identifier
    $info.name configure -text $label

    if {[string range $profile 0 7] == "<profile" && \
            [regexp {filename(?: )*=(?: )*\"([^\"]*)\"} $profile match fileName] && \
            [regexp {type(?: )*=(?: )*\"([^\"]*)\"} $profile match type]} {
        $info.prof configure -label "Profile File" -text $fileName
        if {$fileSys != 0} {
            bind $info.prof.e <Double-Button-1> "Directory::DisplayFile $fileSys \"$fileName\""
        }
    } else {
        $info.prof configure -label "Config Profile" -text $profile
    }

    ShowProperties $dmRef $propcont $props
    set ::status "Updated information for Device Manager $name."
}

proc DeviceManager::Popup {xpos ypos} {
    if {![info exists ::widgets(popup,DeviceManager)]} {
        set ::widgets(popup,DeviceManager) [menu .popupForDevMan]
        $::widgets(popup,DeviceManager) add command \
            -label "Refresh" \
            -command "DeviceManager::Refresh"
        $::widgets(popup,DeviceManager) add separator
        $::widgets(popup,DeviceManager) add command \
            -label "Shutdown" \
            -command "DeviceManager::Shutdown"
    }

    tk_popup $::widgets(popup,DeviceManager) $xpos $ypos
}

proc DeviceManager::Refresh {} {
    set node [lindex [$::widgets(tree) selection get] 0]
    ::UpdateChildren $node
}

proc DeviceManager::Shutdown {} {
    set node [lindex [$::widgets(tree) selection get] 0]
    set name [$::widgets(tree) itemcget $node -text]
    set data [$::widgets(tree) itemcget $node -data]
    set type [lindex $data 0]
    set ref  [lindex $data 1]

    set ::status "Shutting down device manager $name ..."
    update

    corba::try {
        $ref shutdown
        set ::status "Device manager $name shut down."
    } catch {... ex} {
        set ::status "Error shutting down device manager: [lindex $ex 0]"
    }

    foreach widget [winfo children $::widgets(info)] {
        destroy $widget
    }

    $::widgets(tree) delete $node
}

#
# ----------------------------------------------------------------------
# Device
# ----------------------------------------------------------------------
#

namespace eval Device {}

proc Device::ShowInfo {node devRef reuse} {
    set name [$::widgets(tree) itemcget $node -text]
    set ::status "Updating info for Device $name ..."
    update

    set info $::widgets(info)
    $::widgets(title) configure -text "Device"

    if {!$reuse} {
        LabelEntry $info.id -label "Identifier" \
            -labelwidth 16 -width 40 -labelanchor w
        LabelEntry $info.name -label "Label" \
            -labelwidth 16 -width 40 -labelanchor w
        pack $info.id $info.name -side top -fill x -pady 5 

        LabelEntry $info.prof -label "Profile File" \
            -labelwidth 16 -width 40 -labelanchor w
        pack $info.prof -side top -fill x -pady 5

        LabelEntry $info.us -label "Usage State" \
            -labelwidth 16 -width 40 -labelanchor w
        LabelEntry $info.as -label "Admin State" \
            -labelwidth 16 -width 40 -labelanchor w
        LabelEntry $info.os -label "Operational State" \
            -labelwidth 16 -width 40 -labelanchor w
        pack $info.us $info.as $info.os -side top -fill x -pady 5

        set proptitle [TitleFrame $info.props -text "Properties"]
        pack $proptitle -side top -fill both -expand yes -pady 5
    } else {
        bind $info.prof.e <Double-Button-1> ""
    }

    set propcont [$info.props getframe]

    corba::try {
        set identifier [$devRef identifier]
        set profile [$devRef softwareProfile]
        set label [$devRef label]
        set usageState [$devRef usageState]
        set adminState [$devRef adminState]
        set opState [$devRef operationalState]
        set props [list]
        $devRef query props
    } catch {... ex} {
        set ::status "Error: cannot talk to Device: [lindex $ex 0]"
        return
    }

    corba::try {
        set parent [$::widgets(tree) parent $node]
        set pdata [$::widgets(tree) itemcget $parent -data]
        set ptype [lindex $pdata 0]
        set pref [lindex $pdata 1]

        if {$ptype == "DeviceManager"} {
            set fileSys [$pref fileSys]
        } else {
            set fileSys 0
        }
    } catch {... ex} {
        set fileSys 0
    }

    $info.id configure -text $identifier
    $info.name configure -text $label

    if {[string range $profile 0 7] == "<profile" && \
            [regexp {filename(?: )*=(?: )*\"([^\"]*)\"} $profile match fileName] && \
            [regexp {type(?: )*=(?: )*\"([^\"]*)\"} $profile match type]} {
        $info.prof configure -label "Profile File" -text $fileName
        if {$fileSys != 0} {
            bind $info.prof.e <Double-Button-1> "Directory::DisplayFile $fileSys \"$fileName\""
        }
    } else {
        $info.prof configure -label "Software Profile" -text $profile
    }

    $info.us configure -text $usageState
    $info.as configure -text $adminState
    $info.os configure -text $opState
    ShowProperties $devRef $propcont $props
    set ::status "Updated information for Device $name."
}

proc Device::Popup {xpos ypos} {
    if {![info exists ::widgets(popup,Device)]} {
        set ::widgets(popup,Device) [menu .popupForDevice]
        $::widgets(popup,Device) add command \
            -label "Start" \
            -command "Device::Start"
        $::widgets(popup,Device) add command \
            -label "Stop" \
            -command "Device::Stop"
    }

    tk_popup $::widgets(popup,Device) $xpos $ypos
}

proc Device::Start {} {
    set node [lindex [$::widgets(tree) selection get] 0]
    set name [$::widgets(tree) itemcget $node -text]
    set data [$::widgets(tree) itemcget $node -data]
    set type [lindex $data 0]
    set ref  [lindex $data 1]

    set ::status "Starting Device $name ..."
    update

    corba::try {
        $ref start
    } catch {... ex} {
        set ::status "Error starting device: [lindex $ex 0]"
        tk_messageBox -parent . -type ok -icon error -default ok \
            -title "Error starting device" \
            -message $ex
        return
    }

    set ::status "Device $name started."
}

proc Device::Stop {} {
    set node [lindex [$::widgets(tree) selection get] 0]
    set name [$::widgets(tree) itemcget $node -text]
    set data [$::widgets(tree) itemcget $node -data]
    set type [lindex $data 0]
    set ref  [lindex $data 1]

    set ::status "Stopping Device $name ..."
    update

    corba::try {
        $ref stop
    } catch {... ex} {
        set ::status "Error stopping device: [lindex $ex 0]"
        tk_messageBox -parent . -type ok -icon error -default ok \
            -title "Error stopping device" \
            -message $ex
        return
    }

    set ::status "Device $name stopped."
}

#
# ----------------------------------------------------------------------
# Service
# ----------------------------------------------------------------------
#

namespace eval Service {}

proc Service::ShowInfo {node serviceRef reuse} {
    set name [$::widgets(tree) itemcget $node -text]
    set ::status "Updating info for Service $name ..."
    update

    set info $::widgets(info)
    $::widgets(title) configure -text "Service"

    if {!$reuse} {
        LabelEntry $info.name -label "Name" \
            -labelwidth 16 -width 40 -labelanchor w
        LabelEntry $info.reference -label "Reference" \
            -labelwidth 16 -width 40 -labelanchor w
        pack $info.name $info.reference -side top -fill x -pady 5
    }

    set reference [corba::object_to_string $serviceRef]

    $info.name configure -text $name
    $info.reference configure -text $reference

    set ::status "Updated information for Service $name."
}

#
# ----------------------------------------------------------------------
# File Manager / File System / Directory
# ----------------------------------------------------------------------
#

namespace eval FileManager {}
namespace eval FileSystem {}
namespace eval Directory {}

proc Directory::UpdateChildrenIfNecessary {node} {
    if {[$::widgets(tree) itemcget $node -open]} {
        ::UpdateChildren $node
    } else {
        $::widgets(tree) itemconfigure $node -drawcross allways
    }
}

proc Directory::UpdateChildren {node fsRef} {
    set myName [lindex [$::widgets(tree) itemcget $node -data] 2]
    set myLen [string length $myName]
    set myLenmm [expr {$myLen - 1}]
    set myLenpp [expr {$myLen + 1}]

    if {$myName != "/"} {
        set myPattern $myName
    } else {
        set myPattern ""
    }

    append myPattern "/*"

    set ::status "Loading contents for File System $myName ..."
    update

    corba::try {
        set files [$fsRef list $myPattern]
    } catch {... ex} {
        set ::status "Error: cannot talk to File System: [lindex $ex 0]"
        return
    }

    foreach file [lsort -index 1 $files] {
        set filename [lindex $file 1]

        set bnl [expr {[string length $filename] - 1}]
        if {[string range $filename $bnl $bnl] == "/"} {
            set filename [string range $filename 0 [expr {$bnl - 1}]]
        }

        if {[string range $filename 0 $myLenmm] == $myName && \
                [string index $filename $myLen] == "/"} {
            set basename [string range $filename $myLenpp end]
            set absname $filename
        } elseif {[string index $filename 0] == "/"} {
            set lastSlashIdx [string last "/" $filename]
            incr lastSlashIdx
            set basename [string range $filename $lastSlashIdx end]
            set absname $filename
        } elseif {$myName == "/"} {
            set basename $filename
            set absname "/"
            append absname $basename
        } else {
            set basename $filename
            set absname $myName
            append absname "/" $basename
        }

        #
        # The Harris Core Framework lists "." and "..", but we don't want
        # them in our tree.
        #

        if {$basename == "." || $basename == ".."} {
            continue
        }

        switch -- [lindex $file 3] {
            FILE_SYSTEM {
                set filetype "FileSystem"
                set data [list $filetype $fsRef $absname]

                corba::try {
                    if {[$fsRef _is_a IDL:CF/FileManager:1.0]} {
                        set parentMounts [$fsRef getMounts]
                        foreach mount $parentMounts {
                            set mountPoint [lindex $mount 1]
                            if {$filename == $mountPoint} {
                                set theRef [lindex $mount 3]
                                set data [list $filetype $fsRef $filename $theRef]

                                corba::try {
                                    if {[$theRef _is_a IDL:CF/FileManager:1.0]} {
                                        set data [list FileManager $theRef ""]
                                    }
                                } catch {...} {
                                }

                                break
                            }
                        }
                    }
                } catch {...} {
                }
            }
            DIRECTORY {
                set filetype "Directory"
                set data [list $filetype $fsRef $absname]
            }
            default {
                set filetype "File"
            }
        }

        if {$filetype == "FileSystem" || $filetype == "Directory"} {
            set nodename "node[incr ::uniquenodeindex]"
            $::widgets(tree) insert end $node $nodename \
                -open 0 -drawcross allways \
                -image [Bitmap::get folder] -text $basename \
                -data $data
        }
    }

    $::widgets(tree) itemconfigure $node -drawcross auto
    set ::status "Loaded contents for File System $myName."
}

proc Directory::Popup {xpos ypos} {
    if {![info exists ::widgets(popup,Directory)]} {
        set ::widgets(popup,Directory) [menu .popupForDirectory]
        $::widgets(popup,Directory) add command \
            -label "Upload / Install" \
            -command "Directory::InstallApplication"
        $::widgets(popup,Directory) add separator
        $::widgets(popup,Directory) add command \
            -label "Create" \
            -command "Directory::Create"
        $::widgets(popup,Directory) add command \
            -label "Remove" \
            -command "Directory::Remove"
        $::widgets(popup,Directory) add separator
        $::widgets(popup,Directory) add command \
            -label "Refresh" \
            -command "Directory::Refresh"
    }

    tk_popup $::widgets(popup,Directory) $xpos $ypos
}

proc Directory::InstallApplication {} {
    ::InstallApplication [lindex [$::widgets(tree) selection get] 0]
}

proc Directory::Create {} {
    set node [lindex [$::widgets(tree) selection get] 0]
    set data [$::widgets(tree) itemcget $node -data]
    set fsRef [lindex $data 1]
    set absDir [lindex $data 2]

    set top .createDir

    if {[winfo exists $top]} {
        wm deiconify $top
        focus $top
        return
    }

    if {![info exists ::createDirName]} {
        set ::createDirName ""
    }

    toplevel $top
    wm title $top "Create Directory"

    set title [label $top.title -anchor center \
                   -text "Create Directory"]
    pack $title -side top -fill x -pady 10

    set sep1 [Separator $top.sep1 -orient horizontal]
    pack $sep1 -side top -fill x -pady 10

    LabelEntry $top.name -label "Name" \
        -labelwidth 18 -width 40 -labelanchor w \
        -textvariable ::createDirName
    pack $top.name -side top -expand yes -fill x -pady 5 -padx 10

    set sep2 [Separator $top.sep2 -orient horizontal]
    pack $sep2 -side top -fill x -pady 10

    frame $top.buts
    set but1 [button $top.buts.b1 -width 15 -text "OK" \
                  -command "set ::createDirGuiMutex 1"]
    set but2 [button $top.buts.b2 -width 15 -text "Cancel" \
                  -command "set ::createDirGuiMutex 2"]
    pack $but1 $but2 -side left -pady 10 -padx 20
    pack $top.buts

    focus $top.name.e
    bind $top.name.e <Return> "set ::createDirGuiMutex 1"

    set ::createDirGuiMutex 0
    vwait ::createDirGuiMutex
    destroy $top

    if {$::createDirGuiMutex != 1} {
        return
    }

    if {$absDir == "/"} {
        set dirName "/${::createDirName}"
    } else {
        set dirName "${absDir}/${::createDirName}"
    }

    set ::status "Creating Directory $dirName ..."
    update

    corba::try {
        $fsRef mkdir $dirName
    } catch {... ex} {
        set ::status "Error creating directory: [lindex $ex 0]"
        tk_messageBox -parent . -type ok -icon error -default ok \
            -title "Error creating directory" \
            -message "Failed to mkdir $dirName: $ex"
        return
    }

    UpdateChildrenIfNecessary $node
    set ::status "Created directory $dirName."
}

proc Directory::Remove {} {
    set node [lindex [$::widgets(tree) selection get] 0]
    set data [$::widgets(tree) itemcget $node -data]
    set type [lindex $data 0]
    set fsRef [lindex $data 1]
    set absDir [lindex $data 2]

    if {$type != "Directory"} {
        tk_messageBox -parent . -type ok -icon error -default ok \
            -title "Error removing directory" \
            -message "Can not remove directory $absDir of type $type."
        return
    }

    set ::status "Removing directory $absDir ..."
    update

    corba::try {
        Directory::rmrf $fsRef $absDir
    } catch {... ex} {
        set ::status "Error removing directory: [lindex $ex 0]"
        tk_messageBox -parent . -type ok -icon error -default ok \
            -title "Error removing directory" \
            -message "Failed to rmdir $absDir: $ex"
        return
    }

    set parentNode [$::widgets(tree) parent $node]
    TreeNodeSelect $parentNode
    $::widgets(tree) delete $node
    set ::status "Removed directory $absDir."
}

proc Directory::Refresh {} {
    ::UpdateChildren [lindex [$::widgets(tree) selection get] 0]
}

proc FileManager::ShowInfo {node fsRef reuse} {
    set myName [lindex [$::widgets(tree) itemcget $node -data] 2]
    set ::status "Updating info for File Manager $myName ..."
    update

    set info $::widgets(info)
    $::widgets(title) configure -text "File Manager"

    if {!$reuse} {
        TitleFrame $info.mounts -text "Mounted File Systems"
        pack $info.mounts -side top -fill both -expand yes -pady 5

        TitleFrame $info.contents -text "Contents"
        pack $info.contents -side top -fill both -expand yes -pady 5
    }

    set mountscont [$info.mounts getframe]
    set contentscont [$info.contents getframe]

    corba::try {
        set mounts [$fsRef getMounts]
    } catch {... ex} {
        set ::status "Error: cannot talk to File Manager: [lindex $ex 0]"
        return
    }

    foreach widget [winfo children $mountscont] {
        destroy $widget
    }

    if {[llength $mounts] == 0} {
        label $mountscont.label -text "No mounted file systems."
        pack $mountscont.label -fill both -expand yes
    } else {
        frame $mountscont.head
        label $mountscont.head.t1 -text "Mount Point" -bg "\#a0a0a0" -width 40
        label $mountscont.head.t2 -text "Available" -bg "\#a0a0a0" -width 16
        label $mountscont.head.t3 -text "Size" -bg "\#a0a0a0" -width 16
        pack $mountscont.head.t1 -side left -expand yes -fill x
        pack $mountscont.head.t2 $mountscont.head.t3 -side left
        pack $mountscont.head -side top -fill x -padx 5 -pady 5

        set mountssw [ScrolledWindow $mountscont.sw -relief sunken -borderwidth 1]
        set mountssf [ScrollableFrame $mountssw.sf -constrainedwidth 1 -height 50]
        set mountsframe [$mountssf getframe]
        $mountssw setwidget $mountssf

        set idx 0
        foreach mount $mounts {
            set mountName [lindex $mount 1]
            set mountRef [lindex $mount 3]
            set mountAvail "????"
            set mountSize "????"
            corba::try {
                set props [list \
                               [list id SIZE value {void {}}] \
                               [list id "AVAILABLE_SPACE" value {void {}}] \
                              ]
                $mountRef query props
                set mountSize [lindex [lindex [lindex $props 0] 3] 1]
                set mountAvail [lindex [lindex [lindex $props 1] 3] 1]
            } catch {...} {
            }
            set l "l[incr idx]"
            frame $mountsframe.$l
            label $mountsframe.$l.t1 -text $mountName -width 40
            label $mountsframe.$l.t2 -text $mountAvail -width 16
            label $mountsframe.$l.t3 -text $mountSize -width 16
            pack $mountsframe.$l.t1 -side left -expand yes -fill x
            pack $mountsframe.$l.t2 $mountsframe.$l.t3 -side left
            pack $mountsframe.$l -side top -fill x -padx 5
        }

        pack $mountssw -side top -fill both -expand yes
    }

    foreach widget [winfo children $contentscont] {
        destroy $widget
    }

    Directory::ShowContents $node $contentscont $fsRef $myName
    set ::status "Updated information for File Manager $myName."
}

proc FileSystem::ShowInfo {node fsRef reuse} {
    set myName [lindex [$::widgets(tree) itemcget $node -data] 2]
    set ::status "Updating info for File System $myName ..."
    update

    set info $::widgets(info)
    $::widgets(title) configure -text "File System"

    if {!$reuse} {
        LabelEntry $info.id -label "Mount Point" \
            -labelwidth 16 -width 40 -labelanchor w
        LabelEntry $info.avail -label "Available Size" \
            -labelwidth 16 -width 40 -labelanchor w
        LabelEntry $info.size -label "Filesystem Size" \
            -labelwidth 16 -width 40 -labelanchor w
        pack $info.id $info.avail $info.size -side top -fill x -pady 5

        TitleFrame $info.contents -text "Contents"
        pack $info.contents -side top -fill both -expand yes -pady 5
    }

    set contentscont [$info.contents getframe]

    corba::try {
        set props [list \
                       [list id SIZE value {void {}}] \
                       [list id "AVAILABLE_SPACE" value {void {}}] \
                      ]
        $fsRef query props
        set mountSize [lindex [lindex [lindex $props 0] 3] 1]
        set mountAvail [lindex [lindex [lindex $props 1] 3] 1]
    } catch {... ex} {
        set mountSize "<unknown>"
        set mountAvail "<unknown>"
    }

    $info.id configure -text $myName
    $info.avail configure -text $mountAvail
    $info.size configure -text $mountSize
    Directory::ShowContents $node $contentscont $fsRef $myName
    set ::status "Updated information for File System $myName."
}

proc Directory::ShowInfo {node fsRef reuse} {
    set myName [lindex [$::widgets(tree) itemcget $node -data] 2]
    set ::status "Updating info for Directory $myName ..."
    update

    set info $::widgets(info)
    $::widgets(title) configure -text "Directory"

    if {!$reuse} {
        LabelEntry $info.id -label "Name" \
            -labelwidth 16 -width 40 -labelanchor w
        pack $info.id -side top -fill x -pady 5

        TitleFrame $info.contents -text "Contents"
        pack $info.contents -side top -fill both -expand yes -pady 5
    }

    set contentscont [$info.contents getframe]

    $info.id configure -text $myName
    Directory::ShowContents $node $contentscont $fsRef $myName
    set ::status "Updated informtion for Directory $myName."
}

#
# ----------------------------------------------------------------------
# Helper to remove a directory and its contents.
# ----------------------------------------------------------------------
#

proc Directory::rmrf {fs files} {
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

#
# ----------------------------------------------------------------------
# List Directory Contents
# ----------------------------------------------------------------------
#

proc Directory::ShowContents {node frame fsRef myName} {
    set ::status "Updating contents of directory $myName ..."
    update

    foreach widget [winfo children $frame] {
        destroy $widget
    }

    set myLen [string length $myName]
    set myLenmm [expr {$myLen - 1}]
    set myLenpp [expr {$myLen + 1}]

    if {$myName != "/"} {
        set myPattern $myName
    } else {
        set myPattern ""
    }

    append myPattern "/*"

    corba::try {
        set files [$fsRef list $myPattern]
    } catch {... ex} {
        label $frame.label -text "Error accessing File System: [lindex $ex 0]"
        pack $frame.label -fill both -expand yes
        return
    }

    set numFiles 0
    foreach file $files {
        set filetype [lindex $file 3]
        if {$filetype == "PLAIN"} {
            incr numFiles
        }
    }

    if {$numFiles == 0} {
        label $frame.label -text "No Files"
        pack $frame.label -fill both -expand yes
        return
    }

    set filessw [ScrolledWindow $frame.sw -relief sunken -borderwidth 1]
    set filessf [ScrollableFrame $filessw.sf -constrainedwidth 1]
    set filesframe [$filessf getframe]
    $filessw setwidget $filessf

    set idx 0
    foreach file [lsort -index 1 $files] {
        set filename [lindex $file 1]
        set filetype [lindex $file 3]
        set filesize [lindex $file 5]

        if {$filetype != "PLAIN"} {
            continue
        }

        if {[string range $filename 0 $myLenmm] == $myName && \
                [string index $filename $myLen] == "/"} {
            set basename [string range $filename $myLenpp end]
            set absname $filename
        } elseif {[string index $filename 0] == "/"} {
            set lastSlashIdx [string last "/" $filename]
            incr lastSlashIdx
            set basename [string range $filename $lastSlashIdx end]
            set absname $filename
        } elseif {$myName == "/"} {
            set basename $filename
            set absname "/"
            append absname $basename
        } else {
            set basename $filename
            set absname $myName
            append absname "/" $basename
        }

        set l "l[incr idx]"
        frame $filesframe.$l
        label $filesframe.$l.name -text $basename -width 30 -anchor w
        label $filesframe.$l.size -text $filesize -width 16 -anchor e
        pack $filesframe.$l.name -side left -expand yes -fill x
        pack $filesframe.$l.size -side left
        pack $filesframe.$l -side top -fill x -expand yes -padx 10

        bind $filesframe.$l.name <Button-1> "Directory::SelectFile $filesframe $filesframe.$l $fsRef \"$absname\""
        bind $filesframe.$l.name <Double-Button-1> "Directory::OpenFile $filesframe $filesframe.$l $fsRef \"$absname\""

        if {[string first ".sad" $basename] >= 0} {
            bind $filesframe.$l.name <ButtonPress-3> "Directory::SADFilePopup $node $filesframe $filesframe.$l $fsRef \"$absname\""
        } else {
            bind $filesframe.$l.name <ButtonPress-3> "Directory::PlainFilePopup $node $filesframe $filesframe.$l $fsRef \"$absname\""
        }
    }

    pack $filessw -side top -fill both -expand yes
    set ::status "Updated contents of directory $myName."
}

proc Directory::SelectFile {frame selected fsRef filename} {
    foreach widget [winfo children $frame] {
        if {$widget == $selected} {
            $widget.name configure -background "\#c0f0c0"
            $widget.size configure -background "\#c0f0c0"
        } else {
            $widget.name configure -background [lindex [$widget.name configure -background] 3]
            $widget.size configure -background [lindex [$widget.name configure -background] 3]
        }
    }

    set ::status $filename
}

proc Directory::OpenFile {frame selected fsRef filename} {
    SelectFile $frame $selected $fsRef $filename
    DisplayFile $fsRef $filename
}

proc Directory::DisplayFile {fsRef filename} {
    if {$fsRef == 0 || $filename == ""} {
        return
    }

    if {![info exists ::fwcount]} {
        set ::fwcount 0
    } else {
        incr ::fwcount
    }

    set ::status "Loading $filename ..."

    set oops ""
    corba::try {
        set file [$fsRef open $filename 1]
        $file read contents 16384
    } catch {... oops} {
    }

    set top .file$::fwcount
    toplevel $top
    wm title $top $filename

    frame $top.buts
    set but1 [button $top.buts.b1 -width 15 -text "Close" -command "destroy $top"]
    pack $but1 -side left -pady 10 -padx 20
    pack $top.buts -side bottom

    set sep [Separator $top.sep -orient horizontal]
    pack $sep -side bottom -fill x -pady 10

    if {$oops != ""} {
        label $top.label -text "File Error" -width 40 -height 10
        pack $top.label -fill both -expand yes
        set ::status "Error loading file $filename: $oops"
        return
    }

    set filesw [ScrolledWindow $top.sw -relief sunken -auto vertical -borderwidth 1]
    set filetxt [text $filesw.text -wrap none]
    $filesw setwidget $filetxt

    set contents [string map [list "\x0d\x0a" "\x0a" "\x0d" "\x0a"] $contents]
    pack $filesw -fill both -expand yes

    $filetxt insert end $contents

    while {![catch {$file read contents 16384}] && [string length $contents] > 0} {
        set contents [string map [list "\x0d\x0a" "\x0a" "\x0d" "\x0a"] $contents]
        $filetxt insert end $contents
    }

    catch {
        $file close
    }

    set ::status "Loaded file $filename."
}

proc Directory::DownloadFile {fsRef filename} {
    set ::status "Downloading $filename ..."

    corba::try {
        set file [$fsRef open $filename 1]
        set size [$file sizeOf]
        $file read contents 16384
    } catch {... ex} {
        set msg "Failed to download $filename: $ex"
        set ::status $msg
        tk_messageBox -parent . -type ok -icon error -default ok \
            -title "Error downloading file" \
            -message $msg
        catch {
            $file close
        }
        return
    }

    if {[info exists ::defaultSaveDir]} {
        set initialDir $::defaultSaveDir
    } else {
        set initialDir [pwd]
    } 

    set tailName [file tail $filename]
    set saveName [tk_getSaveFile \
                      -parent . \
                      -initialdir $initialDir \
                      -initialfile $tailName]

    if {$saveName == ""} {
        set ::status "Canceled download of $filename."
        catch {
            $file close
        }
        return
    }

    if {[catch {set out [open $saveName "w"]} ex]} {
        set msg "Failed to open $saveName for writing: $ex"
        set ::status $msg
        tk_messageBox -parent . -type ok -icon error -default ok \
            -title "Error opening output file" \
            -message $msg
        catch {
            $file close
        }
        return
    }

    set ::defaultSaveDir [file dirname $saveName]
    fconfigure $out -translation binary
    set count 0

    if {[catch {
        while {[string length $contents] > 0} {
            incr count [string length $contents]
            set pc [expr {($count/1024)*100/($size/1024)}]
            set ::status "Downloading $filename ... $pc %"
            puts -nonewline $out $contents
            $file read contents 16384
        }
        $file close
    } ex]} {
        set msg "Failed to download $filename: $ex"
        set ::status $msg
        tk_messageBox -parent . -type ok -icon error -default ok \
            -title "I/O error" \
            -message $msg
        catch {
            $file close
        }
    }

    close $out
    set ::status "Downloaded $saveName."
}

proc Directory::RemoveFile {fsRef filename selected} {
    corba::try {
        $fsRef remove $filename
    } catch {... ex} {
        set msg "Failed to remove $filename: $ex"
        set ::status $msg
        tk_messageBox -parent . -type ok -icon error -default ok \
            -title "Error removing file" \
            -message $msg
        return
    }

    destroy $selected
}

proc Directory::SADFilePopup {node frame selected fsRef filename} {
    Directory::SelectFile $frame $selected $fsRef $filename

    if {[info exists ::widgets(popup,sadFilePopup)]} {
        catch {destroy $::widgets(popup,sadFilePopup)}
    }

    set ::widgets(popup,sadFilePopup) [menu .sadFilePopup]
    $::widgets(popup,sadFilePopup) add command \
        -label "Install Application" \
        -command "InstallSAD $node \"$filename\""
    $::widgets(popup,sadFilePopup) add separator
    $::widgets(popup,sadFilePopup) add command \
        -label "Save as ..." \
        -command "Directory::DownloadFile $fsRef \"$filename\""
    $::widgets(popup,sadFilePopup) add command \
        -label "Remove" \
        -command "Directory::RemoveFile $fsRef \"$filename\" $selected"

    set xpos [expr [winfo pointerx .] + 5]
    set ypos [expr [winfo pointery .] + 5]

    tk_popup $::widgets(popup,sadFilePopup) $xpos $ypos
}

proc Directory::PlainFilePopup {node frame selected fsRef filename} {
    Directory::SelectFile $frame $selected $fsRef $filename

    if {[info exists ::widgets(popup,plainFilePopup)]} {
        catch {destroy $::widgets(popup,plainFilePopup)}
    }

    set ::widgets(popup,plainFilePopup) [menu .plainFilePopup]
    $::widgets(popup,plainFilePopup) add command \
        -label "Save as ..." \
        -command "Directory::DownloadFile $fsRef \"$filename\""
    $::widgets(popup,plainFilePopup) add command \
        -label "Remove" \
        -command "Directory::RemoveFile $fsRef \"$filename\" $selected"

    set xpos [expr [winfo pointerx .] + 5]
    set ypos [expr [winfo pointery .] + 5]

    tk_popup $::widgets(popup,plainFilePopup) $xpos $ypos
}

#
# ----------------------------------------------------------------------
# Helper so that we can browse through an SCA File System
# ----------------------------------------------------------------------
#

package require vfs
catch {itcl::delete class ScaVfs}

itcl::class ScaVfs {
    public variable fs

    constructor {f} {
        set fs $f
    }

    private method realname {relative} {
        return "/$relative"
    }

    public method access {root relative actualpath mode} {
        # Let's assume that all is good.
    }

    public method createdirectory {root relative actualpath} {
        vfs::filesystem posixerror $::vfs::posix(EPERM)
    }

    public method deletefile {root relative actualpath} {
        vfs::filesystem posixerror $::vfs::posix(EPERM)
    }

    public method fileattributes {root relative actualpath args} {
        # No attributes for now.
        return [list]
    }

    public method matchindirectory {root relative actualpath pattern types} {
        set doMatchFiles [vfs::matchFiles $types]
        set doMatchDirs [vfs::matchDirectories $types]

        set name [realname $relative]

        if {$name == "/"} {
            set pat "/${pattern}"
        } else {
            set pat "${name}/${pattern}"
        }

        set result [list]
        foreach file [$fs list $pat] {
            set fn [lindex $file 1]
            set type [lindex $file 3]

            if {[string range $fn end end] == "/"} {
                set fn [string range $fn 0 end-1]
            }

            set tail [file tail $fn]
            set path [file join $actualpath $tail]

            if {$doMatchFiles && $doMatchDirs} {
                lappend result $path
            } elseif {$doMatchFiles} {
                if {$type == "PLAIN"} {
                    lappend result $path
                }
            } elseif {$doMatchDirs} {
                if {$type == "FILE_SYSTEM" || $type == "DIRECTORY"} {
                    lappend result $path
                }
            }
        }

        return $result
    }

    public method open {root relative actualpath mode permissions} {
        vfs::filesystem posixerror $::vfs::posix(EPERM)
    }

    public method removedirectory {root relative actualpath recursive} {
        vfs::filesystem posixerror $::vfs::posix(EPERM)
    }

    public method stat {root relative actualpath} {
        # At least the type must be returned because Tcl depends on it.
        set name [realname $relative]

        if {$name == "/"} {
            return [list type directory]
        }

        set list [$fs list $name]

        if {[llength $list] != 1} {
            vfs::filesystem posixerror $::vfs::posix(ENOENT)
        }

        set file [lindex $list 0]
        set type [lindex $file 3]

        if {$type == "FILE_SYSTEM" || $type == "DIRECTORY"} {
            set ttype "directory"
        } else {
            set ttype "file"
        }

        return [list type $ttype]
    }

    public method utime {root relative actualpath actime mtime} {
        # Not implemented yet
    }
}


#
# ----------------------------------------------------------------------
# Install Application
# ----------------------------------------------------------------------
#

proc InstallApplication {node} {
    package require vfs
    package require vfs::zip

    set data [$::widgets(tree) itemcget $node -data]
    set type [lindex $data 0]

    #
    # Figure out the DomainManager.
    #

    set dmnode $node
    while {[lindex [$::widgets(tree) itemcget $dmnode -data] 0] != "DomainManager"} {
        set dmnode [$::widgets(tree) parent $dmnode]
    }

    set dmdata [$::widgets(tree) itemcget $dmnode -data]
    set dmRef [lindex $dmdata 1]
    set fmRef [lindex $dmdata 2]

    #
    # Build toplevel dialog.
    #

    set top .installApp

    if {[winfo exists $top]} {
        wm deiconify $top
        focus $top
        return
    }

    if {![info exists ::installAppFile]} {
        set ::installAppFile ""
    }

    if {$type == "FileSystem" || $type == "Directory"} {
        set ::installAppDir [lindex $data 2]
    } elseif {![info exists ::installAppDir]} {
        set ::installAppDir "/"
    }

    if {![info exists ::installAppCreateSub]} {
        set ::installAppCreateSub 1
    }

    if {![info exists ::installAppSubDirectory]} {
        set ::installAppSubDirectory ""
    }

    if {![info exists ::installAppDoInstall]} {
        set ::installAppDoInstall 0
    }

    if {![info exists ::installAppSAD]} {
        set ::installAppSAD ""
    }

    if {![info exists ::installAppSADs]} {
        set ::installAppSADs [list]
    }

    toplevel $top
    wm title $top "Upload / Install"

    set title [label $top.title -anchor center -text "Upload / Install"]
    pack $title -side top -fill x -pady 10

    set sep1 [Separator $top.sep1 -orient horizontal]
    pack $sep1 -side top -fill x -pady 10

    frame $top.zip
    label $top.zip.l -text "Package" -width 12 -anchor w
    entry $top.zip.e -width 40 -textvariable ::installAppFile
    button $top.zip.b -width 10 -text "Browse" -command "set ::installAppMutex 3"
    pack $top.zip.l -side left -padx 5
    pack $top.zip.e -side left -padx 5 -expand yes -fill x
    pack $top.zip.b -side left -padx 5
    pack $top.zip -side top -expand yes -fill x -pady 5 -padx 10

    frame $top.dir
    label $top.dir.l -text "Directory" -width 12 -anchor w
    entry $top.dir.e -width 40 -textvariable ::installAppDir
    button $top.dir.b -width 10 -text "Browse" -command "set ::installAppMutex 5"
    pack $top.dir.l -side left -padx 5
    pack $top.dir.e -side left -padx 5 -expand yes -fill x
    pack $top.dir.b -side left -padx 5
    pack $top.dir -side top -expand yes -fill x -pady 5 -padx 10

    frame $top.csub
    label $top.csub.l -text "" -width 12 -anchor w
    checkbutton $top.csub.e -text "Create subdirectory:" -variable ::installAppCreateSub
    pack $top.csub.l -side left -padx 5
    pack $top.csub.e -side left -padx 5
    pack $top.csub -side top -expand yes -fill x -pady 5 -padx 10

    frame $top.sub
    label $top.sub.l -text "Subdirectory" -width 12 -anchor w
    entry $top.sub.e -width 40 -textvariable ::installAppSubDirectory
    pack $top.sub.l -side left -padx 5
    pack $top.sub.e -side left -padx 5 -expand yes -fill x
    pack $top.sub -side top -expand yes -fill x -pady 5 -padx 10

    frame $top.ia
    label $top.ia.l -text "" -width 12 -anchor w
    checkbutton $top.ia.e -text "Install application:" -variable ::installAppDoInstall
    pack $top.ia.l -side left -padx 5
    pack $top.ia.e -side left -padx 5
    pack $top.ia -side top -expand yes -fill x -pady 5 -padx 10

    frame $top.sad
    label $top.sad.l -text "SAD File" -width 12 -anchor w
    ComboBox $top.sad.e -width 40 \
        -textvariable ::installAppSAD \
        -values $::installAppSADs \
        -autocomplete 1
    pack $top.sad.l -side left -padx 5
    pack $top.sad.e -side left -padx 5 -expand yes -fill x
    pack $top.sad -side top -expand yes -fill x -pady 5 -padx 10

    set sep2 [Separator $top.sep2 -orient horizontal]
    pack $sep2 -side top -fill x -pady 10

    frame $top.buts
    set but1 [button $top.buts.b1 -width 15 -text "OK" \
                  -command "set ::installAppMutex 1"]
    set but2 [button $top.buts.b2 -width 15 -text "Cancel" \
                  -command "set ::installAppMutex 2"]
    pack $but1 $but2 -side left -pady 10 -padx 20
    pack $top.buts

    bind $top.zip.e <FocusOut> "set ::installAppMutex 4"

    while {42} {
        set ::installAppMutex 0
        vwait ::installAppMutex

        switch -- $::installAppMutex {
            1 -
            2 {
                break
            }
            3 -
            4 {
                if {$::installAppMutex == 3} {
                    set fileTypes {
                        {{Zip Files} {.zip}}
                        {{All Files} *}
                    }

                    set zipFile [tk_getOpenFile -title "Select Package ..." \
                                     -filetypes $fileTypes \
                                     -parent $top]

                    if {$zipFile == ""} {
                        continue
                    }

                    set ::installAppFile $zipFile
                }

                if {[catch {set zipFd [vfs::zip::Mount $::installAppFile /zip]}]} {
                    tk_messageBox -parent $top -type ok -icon error -default ok \
                        -title "Invalid package file" \
                        -message "$::installAppFile is not a ZIP file."
                    continue
                }

                set ::installAppSADs [glob -nocomplain -tails -directory /zip {*.[Ss][Aa][Dd]*}]
                vfs::zip::Unmount $zipFd /zip

                $top.sad.e configure -values $::installAppSADs

                set ::installAppSubDirectory [file rootname [file tail $::installAppFile]]

                if {[llength $::installAppSADs]} {
                    set ::installAppDoInstall 1
                    set ::installAppSAD [lindex $::installAppSADs 0]
                } else {
                    set ::installAppDoInstall 0
                    set ::installAppSAD ""
                }
            }
            5 {
                set fs [ScaVfs \#auto $fmRef]

                if {$::tcl_platform(platform) == "windows"} {
                    set mountpoint "S:"
                    vfs::filesystem mount -volume $mountpoint $fs
                } else {
                    set mountpoint "/sca"
                    vfs::filesystem mount $mountpoint $fs
                }

                set parDir [file dirname $::installAppDir]
                set curDir [file join $mountpoint [string range $parDir 1 end]]

                if {![file exists $curDir] || ![file isdirectory $curDir]} {
                    set curDir $mountpoint
                }

                #
                # Can't use tk_chooseDir because on Windows it is implemented
                # natively and doesn't know about the vfs that we just mounted.
                #

                set chosenDir [tk::dialog::file::chooseDir:: \
                                   -initialdir $curDir \
                                   -parent $top \
                                   -title "Choose Directory" \
                                   -mustexist 1]

                if {$chosenDir != "" && \
                        [file exists $chosenDir] && \
                        [file isdirectory $chosenDir]} {
                    set mpl [string length $mountpoint]
                    set rn [string range $chosenDir $mpl end]
                    if {[string range $rn 0 0] != "/"} {
                        set rn "/$rn"
                    }
                    set ::installAppDir $rn
                }

                vfs::filesystem unmount $mountpoint
                itcl::delete object $fs
            }
        }
    }

    destroy $top

    if {$::installAppMutex != 1} {
        return
    }

    if {$::installAppFile == ""} {
        return
    }

    if {$::installAppCreateSub && $::installAppSubDirectory != ""} {
        if {$::installAppDir == "/"} {
            set destDir "/$::installAppSubDirectory"
        } else {
            set destDir "${::installAppDir}/${::installAppSubDirectory}"
        }

        set ::status "Creating directory $destDir ..."
        if {![$fmRef exists $destDir]} {
            $fmRef mkdir $destDir
            Directory::UpdateChildrenIfNecessary $node
        }
    } else {
        set destDir $::installAppDir
    }

    set zipFd [vfs::zip::Mount $::installAppFile /zip]
    set files [glob -nocomplain /zip/*]
    set totalFiles [llength $files]
    set fileCount 0

    while {[set numFilesLeft [llength $files]] > 0} {
        set fileName [lindex $files 0]
        set files [lrange $files 1 end]

        #
        # Tcl 8.5 insists on prefixing the mount point with a drive letter.
        #

        if {[string range $fileName 1 2] == ":/"} {
            set relName [string range $fileName 6 end]
        } else {
            set relName [string range $fileName 4 end]
        }

        if {$destDir == "/"} {
            set remoteName $relName
        } else {
            set remoteName "${destDir}${relName}"
        }

        if {[file isdirectory $fileName]} {
            set ::status "Creating directory $remoteName ..."
            if {![$fmRef exists $remoteName]} {
                $fmRef mkdir $remoteName
            }

            set pattern $fileName
            append pattern "/*"
            incr totalFiles -1
            foreach newFile [glob -nocomplain $pattern] {
                lappend files $newFile
                incr totalFiles
            }
        } else {
            incr fileCount
            set ::status "Uploading file $fileCount/$totalFiles: $remoteName ..."
            update

            set localFile [open $fileName "r"]
            fconfigure $localFile -translation binary

            if {[$fmRef exists $remoteName]} {
                $fmRef remove $remoteName
            }

            set byteCount 0
            set remoteFile [$fmRef create $remoteName]
            set data [read $localFile 10240]

            while {[string length $data] > 0} {
                $remoteFile write $data
                incr byteCount [string length $data]
                set data [read $localFile 10240]
            }

            $remoteFile close
            close $localFile
            corba::release $remoteFile
        }
    }

    vfs::zip::Unmount $zipFd /zip

    if {$::installAppDoInstall} {
        set ::status "Installing $::installAppSAD ..."

        if {$destDir == "/"} {
            set sadName "/${::installAppSAD}"
        } else {
            set sadName "${destDir}/${::installAppSAD}"
        }

        InstallSAD $node $sadName
    } else {
        if {$fileCount != 1} {
            set files "files"
        } else {
            set files "file"
        }

        set ::status "Uploaded $fileCount $files from $::installAppFile to $destDir."
    }
}

proc InstallSAD {node filename} {
    #
    # Figure out the DomainManager.
    #

    set dmnode $node
    while {[lindex [$::widgets(tree) itemcget $dmnode -data] 0] != "DomainManager"} {
        set dmnode [$::widgets(tree) parent $dmnode]
    }

    set dmRef [lindex [$::widgets(tree) itemcget $dmnode -data] 1]

    set ::status "Installing application $filename ..."

    corba::try {
        $dmRef installApplication $filename
    } catch {... ex} {
        set ::status "Error installing application: [lindex $ex 0]"
        tk_messageBox -parent . -type ok -icon error -default ok \
            -title "Error installing application" \
            -message $ex
        return
    }

    set ::status "Application installed."
    update

    #
    # Add the new factory as a child to the DomainManager.
    # However, we don't need to do that if the DomainManager's
    # children are not currently shown, as then the TreeOpen
    # command will take care of it.
    #

    if {[$::widgets(tree) itemcget $dmnode -open] == 0} {
        return
    }

    set ::status "Updating list of Application Factories ..."
    update

    corba::try {
        set factories [$dmRef applicationFactories]
    } catch {... ex} {
        set ::status "Error: cannot talk to DomainManager: [lindex $ex 0]"
        return
    }

    foreach factory $factories {
        corba::try {
            set afid [$factory name]
            set afprofile [$factory softwareProfile]
        } catch {...} {
            set afid "Unknown"
            set afprofile "Unknown"
        }

        if {[string first $filename $afprofile] == -1} {
            continue
        }
        
        set nodename "node[incr ::uniquenodeindex]"
        $::widgets(tree) insert end $dmnode $nodename \
            -open 0 -drawcross never \
            -image $::images(icon,AppFactory) -text $afid \
            -data [list ApplicationFactory $factory]
        break
    }

    set ::status "Updated list of Application Factories."
}

#
# ----------------------------------------------------------------------
# Show a PropertyList
# ----------------------------------------------------------------------
#

proc ShowProperties {obj frame props} {
    foreach widget [winfo children $frame] {
        destroy $widget
    }

    if {[llength $props] == 0} {
        label $frame.label -text "No Properties"
        pack $frame.label -fill both -expand yes
        return
    }

    set propsw [ScrolledWindow $frame.sw -relief sunken -borderwidth 1]
    set propsf [ScrollableFrame $propsw.sf -constrainedwidth 1]
    set propsframe [$propsf getframe]
    $propsw setwidget $propsf

    set maxPropLen 16
    set propLenLimit 42
    foreach prop $props {
        set propNameLength [string length [lindex $prop 1]]
        if {$propNameLength > $maxPropLen && $propNameLength < $propLenLimit} {
            set maxPropLen $propNameLength
        }
    }

    #
    # Hm, sometimes the label doesn't seem to fit, when the label is
    # all capital letters. Adding 25% seems to work fine.
    #

    set maxPropLen [expr int($maxPropLen*1.25)]

    set idx 0
    foreach prop $props {
        set propname [lindex $prop 1]
        set proptype [lindex [lindex $prop 3] 0]
        set propvalue [lindex [lindex $prop 3] 1]

        if {[lindex $proptype 0] == "Object" && $propvalue != 0} {
            set propvalue [corba::object_to_string $propvalue]
        }

        if {$obj != 0} {
            set editable 1
        } else {
            set editable 0
        }

        set wname "l[incr idx]"
        LabelEntry $propsframe.$wname -label $propname -text $propvalue \
            -labelwidth $maxPropLen -width 20 -labelanchor w \
            -editable $editable \
            -command "ConfigureProperty $obj {$propname} {$proptype} $propsframe.$wname"
        pack $propsframe.$wname -side top -fill x
    }

    pack $propsw -side top -fill both -expand yes
}

proc ConfigureProperty {obj name type widget} {
    set ::status "Configuring $name property ..."

    corba::try {
        set value [$widget cget -text]

        if {[lindex $type 0] == "Object" && $value != 0} {
            set value [corba::string_to_object $value]
        }

        $obj configure [list [list id $name value [list $type $value]]]
    } catch {... ex} {
        set ::status "Error configuring $name property: [lindex $ex 0]"
        tk_messageBox -parent . -type ok -icon error -default ok \
            -title "Error configuring property" \
            -message $ex
        return
    }

    set ::status "Property $name set to $value."
}

#
# ----------------------------------------------------------------------
# Helpers to locate a Domain Manager in a Naming Service
# ----------------------------------------------------------------------
#

proc LookupDomainManagerNames {} {
    # Contact Naming Service and look up Domain Managers
    set ::NamingService 0

    corba::try {
        set ::NamingService [corba::string_to_object $::preference(NamingServiceIOR)]
    } catch {... ex} {
        corba::try {
            set ::NamingService [corba::string_to_object $::preference(NamingServiceIOR)]
        } catch {... ex} {
            set ::NamingService 0
        }
    }

    if {$::NamingService != 0 && ![$::NamingService _is_a IDL:omg.org/CosNaming/NamingContextExt:1.0]} {
        set ::NamingService 0
    }

    if {$::NamingService == 0} {
        error "Can not talk to the naming service"
    }

    set domainNames [list]
    set namingContexts [list $::NamingService]
    set namingNames [list ""]

    while {[llength $namingContexts]} {
        set nc [lindex $namingContexts 0]
        set namingContexts [lrange $namingContexts 1 end]

        set name [lindex $namingNames 0]
        set namingNames [lrange $namingNames 1 end]

        #
        # For our own sanity, limit the number of bindings to 100.  This
        # may cause us to ignore some listings in less sane configurations.
        #

        corba::try {
            $nc _is_a IDL:omg.org/CosNaming/NamingContextExt:1.0
            $nc list 100 bl bi
            corba::release $bi
        } catch {... ex} {
            set bl [list]
        }

        foreach binding $bl {
            array set b $binding
            array set bn [lindex $b(binding_name) end]

            if {$bn(kind) != ""} {
                continue
            }

            corba::try {
                set obj [$nc resolve $b(binding_name)]
            } catch {...} {
                continue
            }

            if {$b(binding_type) == "ncontext"} {
                lappend namingContexts $obj

                if {$name != ""} {
                    lappend namingNames "$name/$bn(id)"
                } else {
                    lappend namingNames "$bn(id)"
                }

                continue
            }

            corba::try {
                set isDm [$obj _is_a IDL:CF/DomainManager:1.0]
            } catch {...} {
                set isDm 0
            }

            if {$isDm} {
                if {$name != ""} {
                    lappend domainNames "$name/$bn(id)"
                } else {
                    lappend domainNames "$bn(id)"
                }
                break
            }
        }
    }

    return $domainNames
}

#
# ----------------------------------------------------------------------
# Add a Domain Manager by Naming Service
# ----------------------------------------------------------------------
#

proc AddDomainManager {} {
    set top .admDlg

    if {[winfo exists $top]} {
        wm deiconify $top
        focus $top
        return
    }

    if {$::busyMutex} {
        return
    }

    set ::busyMutex 1

    if {![info exists ::preference(NamingServiceIOR)]} {
        set ::preference(NamingServiceIOR) "corbaloc::"
        append ::preference(NamingServiceIOR) [string tolower [info hostname]]
        append ::preference(NamingServiceIOR) ":2809/NameService"
    }

    if {![info exists ::preference(DomainManagerName)]} {
        set ::preference(DomainManagerName) "Domain Name/Domain Manager"
    }

    if {![info exists ::DomainManagerNames]} {
        set ::DomainManagerNames [list]
    }

    set origNamingServiceIOR $::preference(NamingServiceIOR)
    set origDomainManagerName $::preference(DomainManagerName)

    toplevel $top
    wm title $top "Add Domain Manager"

    set title [label $top.title -anchor center -text "Add Domain Manager"]
    pack $title -side top -fill x -pady 10

    set sep1 [Separator $top.sep1 -orient horizontal]
    pack $sep1 -side top -fill x -pady 10

    frame $top.ns
    label $top.ns.l -text "Naming Service" -width 18 -anchor w
    entry $top.ns.e -width 40 -textvariable ::preference(NamingServiceIOR)
    pack $top.ns.l -side left -padx 5
    pack $top.ns.e -side left -padx 5 -expand yes -fill x
    pack $top.ns -side top -expand yes -fill x -pady 5 -padx 10

    frame $top.dn
    label $top.dn.l -text "Domain Manager" -width 18 -anchor w
    ComboBox $top.dn.e -width 40 \
        -textvariable ::preference(DomainManagerName) \
        -values $::DomainManagerNames \
        -autocomplete 1
    button $top.dn.b -width 10 -text "Lookup" -command "set ::admguimutex 3"
    pack $top.dn.l -side left -padx 5
    pack $top.dn.e -side left -padx 5 -expand yes -fill x
    pack $top.dn.b -side left -padx 5
    pack $top.dn -side top -expand yes -fill x -pady 5 -padx 10

    set sep2 [Separator $top.sep2 -orient horizontal]
    pack $sep2 -side top -fill x -pady 10

    frame $top.buts
    set but1 [button $top.buts.b1 -width 15 -text "OK" \
                  -command "set ::admguimutex 1"]
    set but2 [button $top.buts.b2 -width 15 -text "Cancel" \
                  -command "set ::admguimutex 2"]
    pack $but1 $but2 -side left -pady 10 -padx 20
    pack $top.buts

    focus $top.ns.e
    bind $top.ns.e <Return> "set ::admguimutex 1"
    bind $top.dn.e <Return> "set ::admguimutex 1"
    bind $top.buts.b1 <Return> "set ::admguimutex 1"
    bind $top.buts.b2 <Return> "set ::admguimutex 2"

    while {42} {
        set ::admguimutex 0
        vwait ::admguimutex

        switch -- $::admguimutex {
            1 -
            2 {
                break
            }
            3 {
                # Look up all Domain Managers
                corba::try {
                    set ::DomainManagerNames [LookupDomainManagerNames]
                } catch {... ex} {
                    set ::status "Error looking up domain names: $ex"
                }

                $top.dn.e configure -values $::DomainManagerNames

                if {[llength $::DomainManagerNames] > 0} {
                    $top.dn.e setvalue @0
                }
            }
        }
    }

    destroy $top

    if {$::admguimutex != 1} {
        set ::busyMutex 0
        return
    }

    set ::status "Contacting Naming Service ..."
    update

    set ::NamingService 0
    corba::try {
        set ::NamingService [corba::string_to_object $::preference(NamingServiceIOR)]
        if {![$::NamingService _is_a IDL:omg.org/CosNaming/NamingContextExt:1.0]} {
            set ::NamingService 0
        }
    } catch {... ex} {
        set ::NamingService 0
    }

    if {$::NamingService == 0} {
        corba::try {
            set ::NamingService [corba::string_to_object $::preference(NamingServiceIOR)]
            if {![$::NamingService _is_a IDL:omg.org/CosNaming/NamingContextExt:1.0]} {
                set ::NamingService 0
            }
        } catch {... ex} {
            set ::NamingService 0
        }
    }

    if {$::NamingService == 0} {
        set msg "Could not contact the Naming Service."
        set ::status $msg
        tk_messageBox -parent . -type ok -icon error -default ok \
            -title "No Naming Services" \
            -message $msg
        set ::busyMutex 0
        return
    }

    set ::status "Looking up Domain Manager ..."
    update

    set domainManager 0
    corba::try {
        set domainManager [$::NamingService resolve_str $::preference(DomainManagerName)]
    } catch {...} {
        set domainManager 0
    }

    if {$domainManager == 0} {
        set msg "Could not find DomainManager in Naming Service."
        set ::status $msg
        tk_messageBox -parent . -type ok -icon error -default ok \
            -title "Not found" \
            -message $msg
        set ::busyMutex 0
        return
    }

    #
    # Make sure that this Domain Manager is alive
    #

    set ::status "Contacting Domain Manager ..."
    update

    corba::try {
        if {![$domainManager _is_a IDL:CF/DomainManager:1.0]} {
            set domainManager 0
        } else {
            set identifier [$domainManager identifier]
            set fileMgr [$domainManager fileMgr]
        }
    } catch {...} {
        set domainManager 0
    }

    if {$domainManager == 0} {
        set msg "Could not contact DomainManager."
        set ::status $msg
        tk_messageBox -parent . -type ok -icon error -default ok \
            -title "Not found" \
            -message $msg
        set ::busyMutex 0
        return
    }

    #
    # looks good so far
    #

    set ::status "Found Domain Manager \"$identifier\"."

    #
    # add to our tree
    #

    set nodename "node[incr ::uniquenodeindex]"

    $::widgets(tree) insert end root $nodename \
        -open 0 -drawcross allways \
        -image $::images(icon,DomainManager) -text $identifier \
        -data [list DomainManager $domainManager $fileMgr]

    #
    # Remember preference.
    #

    if {$::preference(NamingServiceIOR) != $origNamingServiceIOR || \
            $::preference(DomainManagerName) != $origDomainManagerName} {
        set ::preferencesAreDirty 1
    }

    set ::busyMutex 0
}

#
# ----------------------------------------------------------------------
# Add a Domain Manager by URI
# ----------------------------------------------------------------------
#

proc AddDomainManagerByURI {} {
    set top .adm2Dlg

    if {[winfo exists $top]} {
        wm deiconify $top
        focus $top
        return
    }

    if {$::busyMutex} {
        return
    }

    set ::busyMutex 1

    if {![info exists ::preference(DomainManagerIOR)]} {
        set ::preference(DomainManagerIOR) "corbaname::"
        append ::preference(DomainManagerIOR) [string tolower [info hostname]]
        append ::preference(DomainManagerIOR) ":2809\#"
        append ::preference(DomainManagerIOR) "Domain Name"
        append ::preference(DomainManagerIOR) "/" "Domain Manager Name"
    }

    set origDomainManagerIOR $::preference(DomainManagerIOR)

    toplevel $top
    wm title $top "Add Domain Manager"

    set title [label $top.title -anchor center \
                   -text "Add Domain Manager"]
    pack $title -side top -fill x -pady 10

    set sep1 [Separator $top.sep1 -orient horizontal]
    pack $sep1 -side top -fill x -pady 10

    LabelEntry $top.uri -label "Domain Manager URI" \
        -labelwidth 18 -width 60 -labelanchor w \
        -textvariable ::preference(DomainManagerIOR)
    pack $top.uri -side top -expand yes -fill x -pady 5 -padx 10

    set sep2 [Separator $top.sep2 -orient horizontal]
    pack $sep2 -side top -fill x -pady 10

    frame $top.buts
    set but1 [button $top.buts.b1 -width 15 -text "OK" \
                  -command "set ::adm2guimutex 1"]
    set but2 [button $top.buts.b2 -width 15 -text "Cancel" \
                  -command "set ::adm2guimutex 2"]
    pack $but1 $but2 -side left -pady 10 -padx 20
    pack $top.buts

    focus $top.uri.e
    bind $top.uri.e <Return> "set ::adm2guimutex 1"
    bind $top.buts.b1 <Return> "set ::adm2guimutex 1"
    bind $top.buts.b2 <Return> "set ::adm2guimutex 2"
    
    set ::adm2guimutex 0
    vwait ::adm2guimutex
    destroy $top

    if {$::adm2guimutex != 1} {
        set ::busyMutex 0
        return
    }

    set ::status "Looking up Domain Manager ..."
    update

    set domainManager 0
    corba::try {
        set domainManager [corba::string_to_object $::preference(DomainManagerIOR)]
    } catch {...} {
        set domainManager 0
    }

    if {$domainManager == 0} {
        set msg "Could not read DomainManager URI."
        set ::status $msg
        tk_messageBox -parent . -type ok -icon error -default ok \
            -title "Not found" \
            -message $msg
        set ::busyMutex 0
        return
    }

    #
    # Make sure that this Domain Manager is alive
    #

    set ::status "Contacting Domain Manager ..."
    update

    corba::try {
        if {![$domainManager _is_a IDL:CF/DomainManager:1.0]} {
            set domainManager 0
        } else {
            set identifier [$domainManager identifier]
        }
    } catch {...} {
        set domainManager 0
    }

    if {$domainManager == 0} {
        set msg "Could not contact DomainManager."
        set ::status $msg
        tk_messageBox -parent . -type ok -icon error -default ok \
            -title "Not found" \
            -message $msg
        set ::busyMutex 0
        return
    }

    #
    # looks good so far
    #

    set ::status "Found Domain Manager \"$identifier\""

    #
    # add to our tree
    #

    set nodename "node[incr ::uniquenodeindex]"

    $::widgets(tree) insert end root $nodename \
        -open 0 -drawcross allways \
        -image $::images(icon,DomainManager) -text $identifier \
        -data [list DomainManager $domainManager]

    #
    # Remember preference.
    #

    if {$::preference(DomainManagerIOR) != $origDomainManagerIOR} {
        set ::preferencesAreDirty 1
    }

    set ::busyMutex 0
}

#
# ----------------------------------------------------------------------
# Remove all Domain Managers
# ----------------------------------------------------------------------
#

proc Reset {} {
    $::widgets(tree) selection clear
    $::widgets(tree) delete [$::widgets(tree) nodes root]

    set rightpane $::widgets(rightpane)

    foreach widget [winfo children $rightpane] {
        destroy $widget
    }

    frame $rightpane.bg -bg "\#f4e4d5"
    label $rightpane.bg.splash -image $::images(splash) -bg "\#f4e4d5" \
         -width 500 -height 400 -anchor center
    pack $rightpane.bg.splash -fill both -expand yes -anchor center
    pack $rightpane.bg -fill both -expand yes -anchor center

    catch {unset ::widgets(title)}
    catch {unset ::widgets(info)}
    set ::busyMutex 0
}

#
# ----------------------------------------------------------------------
# Preferences
# ----------------------------------------------------------------------
#

set registryKey {HKEY_CURRENT_USER\Software\Mercury\SDR Control Room}
set prefsFile ".scatmanrc"

set preferences {
    NamingServiceIOR ascii sz
    DomainManagerIOR ascii sz
    DomainManagerName ascii sz
}

set preferencesAreDirty 0

proc LoadPreferencesFromRegistry {} {
    if {![info exists ::tcl_platform(platform)] || \
            $::tcl_platform(platform) != "windows" || \
            [catch {package require registry}]} {
        return 0
    }

    if {[catch {registry values $::registryKey}]} {
        return 0
    }

    foreach {pref type rtype} $::preferences {
        if {[llength [registry values $::registryKey $pref]] == 1} {
            set value [registry get $::registryKey $pref]
            if {[string is $type $value]} {
                set ::preference($pref) $value
            }
        }
    }

    return 1
}

proc LoadPreferencesFromRCFile {} {
    if {[info exists ::env(HOME)] && [file isdirectory $::env(HOME)]} {
        set homeDir $::env(HOME)
    } else {
        set homeDir "~"
    }

    set fileName [file join $homeDir $::prefsFile]

    if {[catch {
        set f [open $fileName]
    }]} {
        return 0
    }

    while {![eof $f]} {
        set line [string trim [gets $f]]

        if {[string index $line 0] == "#"} {
            continue
        }

        if {[set index [string first "=" $line]] < 1} {
            continue
        }

        set pref [string trim [string range $line 0 [expr {$index-1}]]]
        set value [string trim [string range $line [expr {$index+1}] end]]

        if {[string index $value 0] == "\""} {
            set i 1
            set prefValue ""

            while {$i < [string length $value]} {
                set c [string index $value $i]
                if {$c == "\\"} {
                    set c [string index $value [incr i]]
                    switch -exact -- $c {
                        t {
                            set d "\t"
                        }
                        default {
                            set d $c
                        }
                    }
                    append prefValue $c
                } elseif {$c == "\""} {
                    break
                } else {
                    append prefValue $c
                }
                incr i
            }

            set value $prefValue
        }

        if {[set idx [lsearch $::preferences $pref]] != -1 && \
                [expr {($idx % 3) == 0}]} {
            set type [lindex $::preferences [expr {$idx + 1}]]
            if {[string is $type $value]} {
                set ::preference($pref) $value
            }
        }
    }

    catch {close $f}
    return 1
}


proc SavePreferencesToRegistry {} {
    if {![info exists ::tcl_platform(platform)] || \
            $::tcl_platform(platform) != "windows" || \
            [catch {package require registry}]} {
        return 0
    }

    foreach {pref type rtype} $::preferences {
        if {[info exists ::preference($pref)]} {
            registry set $::registryKey $pref $::preference($pref) $rtype
        }
    }

    return 1
}

proc SavePreferencesToRCFile {} {
    if {[info exists ::tcl_platform(platform)] && \
            $::tcl_platform(platform) == "windows"} {
        return 0
    }

    if {[info exists ::env(HOME)] && [file isdirectory $::env(HOME)]} {
        set homeDir $::env(HOME)
    } else {
        set homeDir "~"
    }

    set fileName [file join $homeDir $::prefsFile]

    if {[catch {
        set f [open $fileName "w"]
    }]} {
        return 0
    }

    foreach {pref type rtype} $::preferences {
        if {[info exists ::preference($pref)]} {
            puts $f "$pref=\"$::preference($pref)\""
        }
    }

    catch {
        close $f
    }
}

proc LoadPreferences {} {
    if {[LoadPreferencesFromRegistry]} {
        return 1
    } elseif {[LoadPreferencesFromRCFile]} {
        return 1
    }
}

proc SavePreferences {} {
    if {!$::preferencesAreDirty} {
        return 1
    }

    if {[SavePreferencesToRegistry]} {
        return 1
    } elseif {[SavePreferencesToRCFile]} {
        return 1
    }

    return 0
}

#
# ----------------------------------------------------------------------
# Miscellaneous
# ----------------------------------------------------------------------
#

proc About {} {
    set top .about

    if {[winfo exists $top]} {
        wm deiconify $top
        focus $top
        return
    }

    toplevel $top -bg "#ffffff"
    wm title $top "About SDR ControlRoom"

    set title [label $top.title -anchor center -bg "#ffffff" \
                   -text "SDR ControlRoom"]
    pack $title -side top -fill x -pady 10

    set topsep [Separator $top.topsep -orient horizontal]
    pack $topsep -side top -fill x -pady 10

    set imgframe [frame $top.imgs -bg "#ffffff"]
    set imgtop [label $imgframe.lab -width 25 -anchor center \
                    -font {Arial 14 bold} -bg "#ffffff" \
                    -text "Copyright \u00a9 2003-2005"]
    set imgbot [label $imgframe.img -bg "#ffffff" -anchor center \
                    -image $::images(mercury)]
    pack $imgtop $imgbot -side top -pady 10
    pack $imgframe -side top -fill x -pady 10

    set botsep [Separator $top.botsep -orient horizontal]
    pack $botsep -side top -fill x -pady 10

    catch {
        set revision "unknown"
        regexp {\$[[:alpha:]]+: ([^\$]*)\$$} $::cvsRevision dummy revision
    }
    catch {
        set lastModified "unknown"
        regexp {\$[[:alpha:]]+: ([^\$]*)\$$} $::cvsDate dummy lastModified
    }

    set revlab [label $top.revision -anchor center \
                    -font {Arial 8} -bg "#ffffff" \
                    -text "Revision: $revision"]
    set lmlab [label $top.lastModified -anchor center \
                   -font {Arial 8} -bg "#ffffff" \
                   -text "Last Modified: $lastModified"]
    pack $revlab -side top -fill x
    pack $lmlab -side top -fill x

    set botsep2 [Separator $top.botsep2 -orient horizontal]
    pack $botsep2 -side top -fill x -pady 10

    set botframe [frame $top.botframe -bg "#ffffff"]
    set botbut [button $botframe.but -width 10 -text "OK" \
                    -command "destroy $top"]
    pack $botbut
    pack $botframe -side top -fill x -pady 10

    wm deiconify $top
    wm resizable $top 0 0
    focus $top

    bind $top <Return> "destroy $top"

    # for Windows
    after 0 "wm deiconify $top"
}

proc Reload {} {
    set ::status "Reloading [file tail $::argv0] ..."
    update
    uplevel #0 {source $::argv0}
    set ::status "Reloading Done."
}

proc ToggleConsole {} {
    console show
}

proc Refresh {} {
    Reload
    set ::status "Refreshing ..."
    update
    catch {destroy $::widgets(main)}
    catch {unset ::widgets(info)}
    catch {unset ::widgets(title)}
    InitGui
    set ::status "Refreshing Done."
}

proc Exit {} {
    SavePreferences
    destroy .
    exit
}

#
# ----------------------------------------------------------------------
# Init the OCPI_CORBA_ORB
# ----------------------------------------------------------------------
#

proc InitORB {} {
    set hostname [info hostname]

    catch {
        set testsock [socket -server none -myaddr [info hostname] 0]
        catch {
            set hostname [lindex [fconfigure $testsock -sockname] 0] ;# IP address
        }
        close $testsock
    }

    eval corba::init -ORBHostName $hostname $::argv
    set ::NamingService 0

    corba::try {
        set ::NamingService [corba::resolve_initial_references NameService]
        $::NamingService _is_a IDL:omg.org/CosNaming/NamingContextExt:1.0
    } catch {... ex} {
        set ::NamingService 0
    }
}

#
# ----------------------------------------------------------------------
# Generated Files start here
# ----------------------------------------------------------------------
#

#
# ----------------------------------------------------------------------
# Mercury Logo
# ----------------------------------------------------------------------
#

set ::images(mercury) [image create photo -data "
R0lGODlh2wAyAIf/AAAAAAEBAQICAgMDAwQEBAUFBQYGBgcHBwgICAkJCQoKCgsLCwwMDA0NDQ4O
Dg8PDxAQEBERERISEhMTExQUFBUVFRYWFhcXFxgYGBkZGRoaGhsbGxwcHB0dHR4eHh8fHyAgICEh
ISIiIiMjIyQkJCUlJSYmJicnJygoKCkpKSoqKisrKywsLC0tLS4uLi8vLzAwMDExMTIyMjMzMzQ0
NDU1NTY2Njc3Nzg4ODk5OTo6Ojs7Ozw8PD09PT4+Pj8/P0BAQEFBQUJCQkNDQ0REREVFRUZGRkdH
R0hISElJSUpKSktLS0xMTE1NTU5OTk9PT1BQUFFRUVJSUlNTU1RUVFVVVVZWVldXV1hYWFlZWVpa
WltbW1xcXF1dXV5eXl9fX2BgYGFhYWJiYmNjY2RkZGVlZWZmZmdnZ2hoaGlpaWpqamtra2xsbG1t
bW5ubm9vb3BwcHFxcXJycnNzc3R0dHV1dXZ2dnd3d3h4eHl5eXp6ent7e3x8fH19fX5+fn9/f4CA
gIGBgYKCgoODg4SEhIWFhYaGhoeHh4iIiImJiYqKiouLi4yMjI2NjY6Ojo+Pj5CQkJGRkZKSkpOT
k5SUlJWVlZaWlpeXl5iYmJmZmZqampubm5ycnJ2dnZ6enp+fn6CgoKGhoaKioqOjo6SkpKWlpaam
pqenp6ioqKmpqaqqqqurq6ysrK2tra6urq+vr7CwsLGxsbKysrOzs7S0tLW1tba2tre3t7i4uLm5
ubq6uru7u7y8vL29vb6+vr+/v8DAwMHBwcLCwsPDw8TExMXFxcbGxsfHx8jIyMnJycrKysvLy8zM
zM3Nzc7Ozs/Pz9DQ0NHR0dLS0tPT09TU1NXV1dbW1tfX19jY2NnZ2dra2tvb29zc3N3d3d7e3t/f
3+Dg4OHh4eLi4uPj4+Tk5OXl5ebm5ufn5+jo6Onp6erq6uvr6+zs7O3t7e7u7u/v7/Dw8PHx8fLy
8vPz8/T09PX19fb29vf39/j4+Pn5+fr6+vv7+/z8/P39/f7+/v///yH5BAEAAP8ALAAAAADbADIA
AAj/AP8JHEiwoMGDCBMqXMiwIDhcoHBha0ixokV0yGABQ2ex40Vo0OB5HEmypEdwiHCgAIEBx0ST
FMFxrIgOEhKVGADRhKkQGAos4HgKHUoSGRAUgIABgwRmJlGEeKBVhAcHAiJoyABhoggN0dOCsBB4
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
ySap+DaUSUKxFxAAOw==
"]

#
# ----------------------------------------------------------------------
# CosNaming.tcl
# ----------------------------------------------------------------------
#

#
# This file was automatically generated from CosNaming.idl
# by idl2tcl. Do not edit.
#

combat::ir add \
{{module {IDL:omg.org/CosNaming:1.0 CosNaming 1.0} {{interface\
{IDL:omg.org/CosNaming/BindingIterator:1.0 BindingIterator 1.0}} {typedef\
{IDL:omg.org/CosNaming/Istring:1.0 Istring 1.0} string} {struct\
{IDL:omg.org/CosNaming/NameComponent:1.0 NameComponent 1.0} {{id\
IDL:omg.org/CosNaming/Istring:1.0} {kind IDL:omg.org/CosNaming/Istring:1.0}}\
{}} {typedef {IDL:omg.org/CosNaming/Name:1.0 Name 1.0} {sequence\
IDL:omg.org/CosNaming/NameComponent:1.0}} {enum\
{IDL:omg.org/CosNaming/BindingType:1.0 BindingType 1.0} {nobject ncontext}}\
{struct {IDL:omg.org/CosNaming/Binding:1.0 Binding 1.0} {{binding_name\
IDL:omg.org/CosNaming/Name:1.0} {binding_type\
IDL:omg.org/CosNaming/BindingType:1.0}} {}} {typedef\
{IDL:omg.org/CosNaming/BindingList:1.0 BindingList 1.0} {sequence\
IDL:omg.org/CosNaming/Binding:1.0}} {interface\
{IDL:omg.org/CosNaming/NamingContext:1.0 NamingContext 1.0} {} {{enum\
{IDL:omg.org/CosNaming/NamingContext/NotFoundReason:1.0 NotFoundReason 1.0}\
{missing_node not_context not_object}} {exception\
{IDL:omg.org/CosNaming/NamingContext/NotFound:1.0 NotFound 1.0} {{why\
IDL:omg.org/CosNaming/NamingContext/NotFoundReason:1.0} {rest_of_name\
IDL:omg.org/CosNaming/Name:1.0}} {}} {exception\
{IDL:omg.org/CosNaming/NamingContext/CannotProceed:1.0 CannotProceed 1.0}\
{{cxt IDL:omg.org/CosNaming/NamingContext:1.0} {rest_of_name\
IDL:omg.org/CosNaming/Name:1.0}} {}} {exception\
{IDL:omg.org/CosNaming/NamingContext/InvalidName:1.0 InvalidName 1.0} {} {}}\
{exception {IDL:omg.org/CosNaming/NamingContext/AlreadyBound:1.0 AlreadyBound\
1.0} {} {}} {exception {IDL:omg.org/CosNaming/NamingContext/NotEmpty:1.0\
NotEmpty 1.0} {} {}} {operation {IDL:omg.org/CosNaming/NamingContext/bind:1.0\
bind 1.0} void {{in n IDL:omg.org/CosNaming/Name:1.0} {in obj Object}}\
{IDL:omg.org/CosNaming/NamingContext/NotFound:1.0\
IDL:omg.org/CosNaming/NamingContext/CannotProceed:1.0\
IDL:omg.org/CosNaming/NamingContext/InvalidName:1.0\
IDL:omg.org/CosNaming/NamingContext/AlreadyBound:1.0}} {operation\
{IDL:omg.org/CosNaming/NamingContext/rebind:1.0 rebind 1.0} void {{in n\
IDL:omg.org/CosNaming/Name:1.0} {in obj Object}}\
{IDL:omg.org/CosNaming/NamingContext/NotFound:1.0\
IDL:omg.org/CosNaming/NamingContext/CannotProceed:1.0\
IDL:omg.org/CosNaming/NamingContext/InvalidName:1.0}} {operation\
{IDL:omg.org/CosNaming/NamingContext/bind_context:1.0 bind_context 1.0} void\
{{in n IDL:omg.org/CosNaming/Name:1.0} {in nc\
IDL:omg.org/CosNaming/NamingContext:1.0}}\
{IDL:omg.org/CosNaming/NamingContext/NotFound:1.0\
IDL:omg.org/CosNaming/NamingContext/CannotProceed:1.0\
IDL:omg.org/CosNaming/NamingContext/InvalidName:1.0\
IDL:omg.org/CosNaming/NamingContext/AlreadyBound:1.0}} {operation\
{IDL:omg.org/CosNaming/NamingContext/rebind_context:1.0 rebind_context 1.0}\
void {{in n IDL:omg.org/CosNaming/Name:1.0} {in nc\
IDL:omg.org/CosNaming/NamingContext:1.0}}\
{IDL:omg.org/CosNaming/NamingContext/NotFound:1.0\
IDL:omg.org/CosNaming/NamingContext/CannotProceed:1.0\
IDL:omg.org/CosNaming/NamingContext/InvalidName:1.0}} {operation\
{IDL:omg.org/CosNaming/NamingContext/resolve:1.0 resolve 1.0} Object {{in n\
IDL:omg.org/CosNaming/Name:1.0}}\
{IDL:omg.org/CosNaming/NamingContext/NotFound:1.0\
IDL:omg.org/CosNaming/NamingContext/CannotProceed:1.0\
IDL:omg.org/CosNaming/NamingContext/InvalidName:1.0}} {operation\
{IDL:omg.org/CosNaming/NamingContext/unbind:1.0 unbind 1.0} void {{in n\
IDL:omg.org/CosNaming/Name:1.0}}\
{IDL:omg.org/CosNaming/NamingContext/NotFound:1.0\
IDL:omg.org/CosNaming/NamingContext/CannotProceed:1.0\
IDL:omg.org/CosNaming/NamingContext/InvalidName:1.0}} {operation\
{IDL:omg.org/CosNaming/NamingContext/new_context:1.0 new_context 1.0}\
IDL:omg.org/CosNaming/NamingContext:1.0 {} {}} {operation\
{IDL:omg.org/CosNaming/NamingContext/bind_new_context:1.0 bind_new_context\
1.0} IDL:omg.org/CosNaming/NamingContext:1.0 {{in n\
IDL:omg.org/CosNaming/Name:1.0}}\
{IDL:omg.org/CosNaming/NamingContext/NotFound:1.0\
IDL:omg.org/CosNaming/NamingContext/AlreadyBound:1.0\
IDL:omg.org/CosNaming/NamingContext/CannotProceed:1.0\
IDL:omg.org/CosNaming/NamingContext/InvalidName:1.0}} {operation\
{IDL:omg.org/CosNaming/NamingContext/destroy:1.0 destroy 1.0} void {}\
IDL:omg.org/CosNaming/NamingContext/NotEmpty:1.0} {operation\
{IDL:omg.org/CosNaming/NamingContext/list:1.0 list 1.0} void {{in how_many\
{unsigned long}} {out bl IDL:omg.org/CosNaming/BindingList:1.0} {out bi\
IDL:omg.org/CosNaming/BindingIterator:1.0}} {}}}} {interface\
{IDL:omg.org/CosNaming/BindingIterator:1.0 BindingIterator 1.0} {}\
{{operation {IDL:omg.org/CosNaming/BindingIterator/next_one:1.0 next_one 1.0}\
boolean {{out b IDL:omg.org/CosNaming/Binding:1.0}} {}} {operation\
{IDL:omg.org/CosNaming/BindingIterator/next_n:1.0 next_n 1.0} boolean {{in\
how_many {unsigned long}} {out bl IDL:omg.org/CosNaming/BindingList:1.0}} {}}\
{operation {IDL:omg.org/CosNaming/BindingIterator/destroy:1.0 destroy 1.0}\
void {} {}}}} {interface {IDL:omg.org/CosNaming/NamingContextExt:1.0\
NamingContextExt 1.0} IDL:omg.org/CosNaming/NamingContext:1.0 {{typedef\
{IDL:omg.org/CosNaming/NamingContextExt/StringName:1.0 StringName 1.0}\
string} {typedef {IDL:omg.org/CosNaming/NamingContextExt/Address:1.0 Address\
1.0} string} {typedef {IDL:omg.org/CosNaming/NamingContextExt/URLString:1.0\
URLString 1.0} string} {operation\
{IDL:omg.org/CosNaming/NamingContextExt/to_string:1.0 to_string 1.0}\
IDL:omg.org/CosNaming/NamingContextExt/StringName:1.0 {{in n\
IDL:omg.org/CosNaming/Name:1.0}}\
IDL:omg.org/CosNaming/NamingContext/InvalidName:1.0} {operation\
{IDL:omg.org/CosNaming/NamingContextExt/to_name:1.0 to_name 1.0}\
IDL:omg.org/CosNaming/Name:1.0 {{in sn\
IDL:omg.org/CosNaming/NamingContextExt/StringName:1.0}}\
IDL:omg.org/CosNaming/NamingContext/InvalidName:1.0} {exception\
{IDL:omg.org/CosNaming/NamingContextExt/InvalidAddress:1.0 InvalidAddress\
1.0} {} {}} {operation {IDL:omg.org/CosNaming/NamingContextExt/to_url:1.0\
to_url 1.0} IDL:omg.org/CosNaming/NamingContextExt/URLString:1.0 {{in addr\
IDL:omg.org/CosNaming/NamingContextExt/Address:1.0} {in sn\
IDL:omg.org/CosNaming/NamingContextExt/StringName:1.0}}\
{IDL:omg.org/CosNaming/NamingContextExt/InvalidAddress:1.0\
IDL:omg.org/CosNaming/NamingContext/InvalidName:1.0}} {operation\
{IDL:omg.org/CosNaming/NamingContextExt/resolve_str:1.0 resolve_str 1.0}\
Object {{in n IDL:omg.org/CosNaming/NamingContextExt/StringName:1.0}}\
{IDL:omg.org/CosNaming/NamingContext/NotFound:1.0\
IDL:omg.org/CosNaming/NamingContext/CannotProceed:1.0\
IDL:omg.org/CosNaming/NamingContext/InvalidName:1.0\
IDL:omg.org/CosNaming/NamingContext/AlreadyBound:1.0}}}}}}}

#
# ----------------------------------------------------------------------
# CF.tcl
# ----------------------------------------------------------------------
#

#
# This file was automatically generated from CF.idl
# by idl2tcl. Do not edit.
#

combat::ir add \
{{module {IDL:CF:1.0 CF 1.0} {{typedef {IDL:CF/StringSequence:1.0\
StringSequence 1.0} {sequence string}} {interface {IDL:CF/LifeCycle:1.0\
LifeCycle 1.0} {} {{exception {IDL:CF/LifeCycle/InitializeError:1.0\
InitializeError 1.0} {{errorMessages IDL:CF/StringSequence:1.0}} {}}\
{exception {IDL:CF/LifeCycle/ReleaseError:1.0 ReleaseError 1.0}\
{{errorMessages IDL:CF/StringSequence:1.0}} {}} {operation\
{IDL:CF/LifeCycle/initialize:1.0 initialize 1.0} void {}\
IDL:CF/LifeCycle/InitializeError:1.0} {operation\
{IDL:CF/LifeCycle/releaseObject:1.0 releaseObject 1.0} void {}\
IDL:CF/LifeCycle/ReleaseError:1.0}}} {struct {IDL:CF/DataType:1.0 DataType\
1.0}} {typedef {IDL:CF/Properties:1.0 Properties 1.0} {sequence\
IDL:CF/DataType:1.0}} {exception {IDL:CF/UnknownProperties:1.0\
UnknownProperties 1.0} {{invalidProperties IDL:CF/Properties:1.0}} {}}\
{interface {IDL:CF/TestableObject:1.0 TestableObject 1.0} {} {{exception\
{IDL:CF/TestableObject/UnknownTest:1.0 UnknownTest 1.0} {} {}} {operation\
{IDL:CF/TestableObject/runTest:1.0 runTest 1.0} void {{in testid {unsigned\
long}} {inout testValues IDL:CF/Properties:1.0}}\
{IDL:CF/TestableObject/UnknownTest:1.0 IDL:CF/UnknownProperties:1.0}}}}\
{interface {IDL:CF/PropertySet:1.0 PropertySet 1.0} {} {{exception\
{IDL:CF/PropertySet/InvalidConfiguration:1.0 InvalidConfiguration 1.0} {{msg\
string} {invalidProperties IDL:CF/Properties:1.0}} {}} {exception\
{IDL:CF/PropertySet/PartialConfiguration:1.0 PartialConfiguration 1.0}\
{{invalidProperties IDL:CF/Properties:1.0}} {}} {operation\
{IDL:CF/PropertySet/configure:1.0 configure 1.0} void {{in configProperties\
IDL:CF/Properties:1.0}} {IDL:CF/PropertySet/InvalidConfiguration:1.0\
IDL:CF/PropertySet/PartialConfiguration:1.0}} {operation\
{IDL:CF/PropertySet/query:1.0 query 1.0} void {{inout configProperties\
IDL:CF/Properties:1.0}} IDL:CF/UnknownProperties:1.0}}} {interface\
{IDL:CF/PortSupplier:1.0 PortSupplier 1.0} {} {{exception\
{IDL:CF/PortSupplier/UnknownPort:1.0 UnknownPort 1.0} {} {}} {operation\
{IDL:CF/PortSupplier/getPort:1.0 getPort 1.0} Object {{in name string}}\
IDL:CF/PortSupplier/UnknownPort:1.0}}} {enum {IDL:CF/ErrorNumberType:1.0\
ErrorNumberType 1.0} {CF_NOTSET CF_E2BIG CF_EACCES CF_EAGAIN CF_EBADF\
CF_EBADMSG CF_EBUSY CF_ECANCELED CF_ECHILD CF_EDEADLK CF_EDOM CF_EEXIST\
CF_EFAULT CF_EFBIG CF_EINPROGRESS CF_EINTR CF_EINVAL CF_EIO CF_EISDIR\
CF_EMFILE CF_EMLINK CF_EMSGSIZE CF_ENAMETOOLONG CF_ENFILE CF_ENODEV CF_ENOENT\
CF_ENOEXEC CF_ENOLCK CF_ENOMEM CF_ENOSPC CF_ENOSYS CF_ENOTDIR CF_ENOTEMPTY\
CF_ENOTSUP CF_ENOTTY CF_ENXIO CF_EPERM CF_EPIPE CF_ERANGE CF_EROFS CF_ESPIPE\
CF_ESRCH CF_ETIMEDOUT CF_EXDEV}} {interface {IDL:CF/Resource:1.0 Resource\
1.0} {IDL:CF/LifeCycle:1.0 IDL:CF/TestableObject:1.0 IDL:CF/PropertySet:1.0\
IDL:CF/PortSupplier:1.0} {{exception {IDL:CF/Resource/StartError:1.0\
StartError 1.0} {{errorNumber IDL:CF/ErrorNumberType:1.0} {msg string}} {}}\
{exception {IDL:CF/Resource/StopError:1.0 StopError 1.0} {{errorNumber\
IDL:CF/ErrorNumberType:1.0} {msg string}} {}} {attribute\
{IDL:CF/Resource/identifier:1.0 identifier 1.0} string readonly} {operation\
{IDL:CF/Resource/start:1.0 start 1.0} void {} IDL:CF/Resource/StartError:1.0}\
{operation {IDL:CF/Resource/stop:1.0 stop 1.0} void {}\
IDL:CF/Resource/StopError:1.0}}} {interface {IDL:CF/AggregateDevice:1.0\
AggregateDevice 1.0}} {interface {IDL:CF/Device:1.0 Device 1.0}\
IDL:CF/Resource:1.0 {{exception {IDL:CF/Device/InvalidState:1.0 InvalidState\
1.0} {{msg string}} {}} {exception {IDL:CF/Device/InvalidCapacity:1.0\
InvalidCapacity 1.0} {{msg string} {capacities IDL:CF/Properties:1.0}} {}}\
{enum {IDL:CF/Device/AdminType:1.0 AdminType 1.0} {LOCKED SHUTTING_DOWN\
UNLOCKED}} {enum {IDL:CF/Device/OperationalType:1.0 OperationalType 1.0}\
{ENABLED DISABLED}} {enum {IDL:CF/Device/UsageType:1.0 UsageType 1.0} {IDLE\
ACTIVE BUSY}} {attribute {IDL:CF/Device/usageState:1.0 usageState 1.0}\
IDL:CF/Device/UsageType:1.0 readonly} {attribute\
{IDL:CF/Device/adminState:1.0 adminState 1.0} IDL:CF/Device/AdminType:1.0}\
{attribute {IDL:CF/Device/operationalState:1.0 operationalState 1.0}\
IDL:CF/Device/OperationalType:1.0 readonly} {attribute\
{IDL:CF/Device/softwareProfile:1.0 softwareProfile 1.0} string readonly}\
{attribute {IDL:CF/Device/label:1.0 label 1.0} string readonly} {attribute\
{IDL:CF/Device/compositeDevice:1.0 compositeDevice 1.0}\
IDL:CF/AggregateDevice:1.0 readonly} {operation\
{IDL:CF/Device/allocateCapacity:1.0 allocateCapacity 1.0} boolean {{in\
capacities IDL:CF/Properties:1.0}} {IDL:CF/Device/InvalidCapacity:1.0\
IDL:CF/Device/InvalidState:1.0}} {operation\
{IDL:CF/Device/deallocateCapacity:1.0 deallocateCapacity 1.0} void {{in\
capacities IDL:CF/Properties:1.0}} {IDL:CF/Device/InvalidCapacity:1.0\
IDL:CF/Device/InvalidState:1.0}}}} {struct {IDL:CF/DataType:1.0 DataType 1.0}\
{{id string} {value any}} {}} {typedef {IDL:CF/DeviceSequence:1.0\
DeviceSequence 1.0} {sequence IDL:CF/Device:1.0}} {exception\
{IDL:CF/InvalidObjectReference:1.0 InvalidObjectReference 1.0} {{msg string}}\
{}} {interface {IDL:CF/AggregateDevice:1.0 AggregateDevice 1.0} {}\
{{attribute {IDL:CF/AggregateDevice/devices:1.0 devices 1.0}\
IDL:CF/DeviceSequence:1.0 readonly} {operation\
{IDL:CF/AggregateDevice/addDevice:1.0 addDevice 1.0} void {{in\
associatedDevice IDL:CF/Device:1.0}} IDL:CF/InvalidObjectReference:1.0}\
{operation {IDL:CF/AggregateDevice/removeDevice:1.0 removeDevice 1.0} void\
{{in associatedDevice IDL:CF/Device:1.0}}\
IDL:CF/InvalidObjectReference:1.0}}} {typedef {IDL:CF/OctetSequence:1.0\
OctetSequence 1.0} {sequence octet}} {exception {IDL:CF/FileException:1.0\
FileException 1.0} {{errorNumber IDL:CF/ErrorNumberType:1.0} {msg string}}\
{}} {interface {IDL:CF/File:1.0 File 1.0} {} {{exception\
{IDL:CF/File/IOException:1.0 IOException 1.0} {{errorNumber\
IDL:CF/ErrorNumberType:1.0} {msg string}} {}} {exception\
{IDL:CF/File/InvalidFilePointer:1.0 InvalidFilePointer 1.0} {} {}} {attribute\
{IDL:CF/File/fileName:1.0 fileName 1.0} string readonly} {attribute\
{IDL:CF/File/filePointer:1.0 filePointer 1.0} {unsigned long} readonly}\
{operation {IDL:CF/File/read:1.0 read 1.0} void {{out data\
IDL:CF/OctetSequence:1.0} {in length {unsigned long}}}\
IDL:CF/File/IOException:1.0} {operation {IDL:CF/File/write:1.0 write 1.0}\
void {{in data IDL:CF/OctetSequence:1.0}} IDL:CF/File/IOException:1.0}\
{operation {IDL:CF/File/sizeOf:1.0 sizeOf 1.0} {unsigned long} {}\
IDL:CF/FileException:1.0} {operation {IDL:CF/File/close:1.0 close 1.0} void\
{} IDL:CF/FileException:1.0} {operation {IDL:CF/File/setFilePointer:1.0\
setFilePointer 1.0} void {{in filePointer {unsigned long}}}\
{IDL:CF/File/InvalidFilePointer:1.0 IDL:CF/FileException:1.0}}}} {struct\
{IDL:CF/DeviceAssignmentType:1.0 DeviceAssignmentType 1.0}} {typedef\
{IDL:CF/DeviceAssignmentSequence:1.0 DeviceAssignmentSequence 1.0} {sequence\
IDL:CF/DeviceAssignmentType:1.0}} {interface {IDL:CF/Application:1.0\
Application 1.0} IDL:CF/Resource:1.0 {{struct\
{IDL:CF/Application/ComponentProcessIdType:1.0 ComponentProcessIdType 1.0}\
{{componentId string} {processId {unsigned long}}} {}} {typedef\
{IDL:CF/Application/ComponentProcessIdSequence:1.0 ComponentProcessIdSequence\
1.0} {sequence IDL:CF/Application/ComponentProcessIdType:1.0}} {struct\
{IDL:CF/Application/ComponentElementType:1.0 ComponentElementType 1.0}\
{{componentId string} {elementId string}} {}} {typedef\
{IDL:CF/Application/ComponentElementSequence:1.0 ComponentElementSequence\
1.0} {sequence IDL:CF/Application/ComponentElementType:1.0}} {attribute\
{IDL:CF/Application/componentNamingContexts:1.0 componentNamingContexts 1.0}\
IDL:CF/Application/ComponentElementSequence:1.0 readonly} {attribute\
{IDL:CF/Application/componentProcessIds:1.0 componentProcessIds 1.0}\
IDL:CF/Application/ComponentProcessIdSequence:1.0 readonly} {attribute\
{IDL:CF/Application/componentDevices:1.0 componentDevices 1.0}\
IDL:CF/DeviceAssignmentSequence:1.0 readonly} {attribute\
{IDL:CF/Application/componentImplementations:1.0 componentImplementations\
1.0} IDL:CF/Application/ComponentElementSequence:1.0 readonly} {attribute\
{IDL:CF/Application/profile:1.0 profile 1.0} string readonly} {attribute\
{IDL:CF/Application/name:1.0 name 1.0} string readonly}}} {struct\
{IDL:CF/DeviceAssignmentType:1.0 DeviceAssignmentType 1.0} {{componentId\
string} {assignedDeviceId string}} {}} {interface\
{IDL:CF/ApplicationFactory:1.0 ApplicationFactory 1.0} {} {{exception\
{IDL:CF/ApplicationFactory/CreateApplicationRequestError:1.0\
CreateApplicationRequestError 1.0} {{invalidAssignments\
IDL:CF/DeviceAssignmentSequence:1.0}} {}} {exception\
{IDL:CF/ApplicationFactory/CreateApplicationError:1.0 CreateApplicationError\
1.0} {{errorNumber IDL:CF/ErrorNumberType:1.0} {msg string}} {}} {exception\
{IDL:CF/ApplicationFactory/InvalidInitConfiguration:1.0\
InvalidInitConfiguration 1.0} {{invalidProperties IDL:CF/Properties:1.0}} {}}\
{attribute {IDL:CF/ApplicationFactory/name:1.0 name 1.0} string readonly}\
{attribute {IDL:CF/ApplicationFactory/identifier:1.0 identifier 1.0} string\
readonly} {attribute {IDL:CF/ApplicationFactory/softwareProfile:1.0\
softwareProfile 1.0} string readonly} {operation\
{IDL:CF/ApplicationFactory/create:1.0 create 1.0} IDL:CF/Application:1.0 {{in\
name string} {in initConfiguration IDL:CF/Properties:1.0} {in\
deviceAssignments IDL:CF/DeviceAssignmentSequence:1.0}}\
{IDL:CF/ApplicationFactory/CreateApplicationError:1.0\
IDL:CF/ApplicationFactory/CreateApplicationRequestError:1.0\
IDL:CF/ApplicationFactory/InvalidInitConfiguration:1.0}}}} {interface\
{IDL:CF/FileSystem:1.0 FileSystem 1.0}} {interface {IDL:CF/DeviceManager:1.0\
DeviceManager 1.0} {IDL:CF/PropertySet:1.0 IDL:CF/PortSupplier:1.0} {{struct\
{IDL:CF/DeviceManager/ServiceType:1.0 ServiceType 1.0} {{serviceObject\
Object} {serviceName string}} {}} {typedef\
{IDL:CF/DeviceManager/ServiceSequence:1.0 ServiceSequence 1.0} {sequence\
IDL:CF/DeviceManager/ServiceType:1.0}} {attribute\
{IDL:CF/DeviceManager/deviceConfigurationProfile:1.0\
deviceConfigurationProfile 1.0} string readonly} {attribute\
{IDL:CF/DeviceManager/fileSys:1.0 fileSys 1.0} IDL:CF/FileSystem:1.0\
readonly} {attribute {IDL:CF/DeviceManager/identifier:1.0 identifier 1.0}\
string readonly} {attribute {IDL:CF/DeviceManager/label:1.0 label 1.0} string\
readonly} {attribute {IDL:CF/DeviceManager/registeredDevices:1.0\
registeredDevices 1.0} IDL:CF/DeviceSequence:1.0 readonly} {attribute\
{IDL:CF/DeviceManager/registeredServices:1.0 registeredServices 1.0}\
IDL:CF/DeviceManager/ServiceSequence:1.0 readonly} {operation\
{IDL:CF/DeviceManager/registerDevice:1.0 registerDevice 1.0} void {{in\
registeringDevice IDL:CF/Device:1.0}} IDL:CF/InvalidObjectReference:1.0}\
{operation {IDL:CF/DeviceManager/unregisterDevice:1.0 unregisterDevice 1.0}\
void {{in registeredDevice IDL:CF/Device:1.0}}\
IDL:CF/InvalidObjectReference:1.0} {operation\
{IDL:CF/DeviceManager/shutdown:1.0 shutdown 1.0} void {} {}} {operation\
{IDL:CF/DeviceManager/registerService:1.0 registerService 1.0} void {{in\
registeringService Object} {in name string}}\
IDL:CF/InvalidObjectReference:1.0} {operation\
{IDL:CF/DeviceManager/unregisterService:1.0 unregisterService 1.0} void {{in\
unregisteringService Object} {in name string}}\
IDL:CF/InvalidObjectReference:1.0} {operation\
{IDL:CF/DeviceManager/getComponentImplementationId:1.0\
getComponentImplementationId 1.0} string {{in componentInstantiationId\
string}} {}}}} {exception {IDL:CF/InvalidFileName:1.0 InvalidFileName 1.0}\
{{errorNumber IDL:CF/ErrorNumberType:1.0} {msg string}} {}} {interface\
{IDL:CF/FileSystem:1.0 FileSystem 1.0} {} {{exception\
{IDL:CF/FileSystem/UnknownFileSystemProperties:1.0\
UnknownFileSystemProperties 1.0} {{invalidProperties IDL:CF/Properties:1.0}}\
{}} {const {IDL:CF/FileSystem/SIZE:1.0 SIZE 1.0} string SIZE} {const\
{IDL:CF/FileSystem/AVAILABLE_SPACE:1.0 AVAILABLE_SPACE 1.0} string\
AVAILABLE_SPACE} {enum {IDL:CF/FileSystem/FileType:1.0 FileType 1.0} {PLAIN\
DIRECTORY FILE_SYSTEM}} {struct {IDL:CF/FileSystem/FileInformationType:1.0\
FileInformationType 1.0} {{name string} {kind IDL:CF/FileSystem/FileType:1.0}\
{size {unsigned long long}} {fileProperties IDL:CF/Properties:1.0}} {}}\
{typedef {IDL:CF/FileSystem/FileInformationSequence:1.0\
FileInformationSequence 1.0} {sequence\
IDL:CF/FileSystem/FileInformationType:1.0}} {const\
{IDL:CF/FileSystem/CREATED_TIME_ID:1.0 CREATED_TIME_ID 1.0} string\
CREATED_TIME} {const {IDL:CF/FileSystem/MODIFIED_TIME_ID:1.0 MODIFIED_TIME_ID\
1.0} string MODIFIED_TIME} {const {IDL:CF/FileSystem/LAST_ACCESS_TIME_ID:1.0\
LAST_ACCESS_TIME_ID 1.0} string LAST_ACCESS_TIME} {operation\
{IDL:CF/FileSystem/remove:1.0 remove 1.0} void {{in fileName string}}\
{IDL:CF/FileException:1.0 IDL:CF/InvalidFileName:1.0}} {operation\
{IDL:CF/FileSystem/copy:1.0 copy 1.0} void {{in sourceFileName string} {in\
destinationFileName string}} {IDL:CF/InvalidFileName:1.0\
IDL:CF/FileException:1.0}} {operation {IDL:CF/FileSystem/exists:1.0 exists\
1.0} boolean {{in fileName string}} IDL:CF/InvalidFileName:1.0} {operation\
{IDL:CF/FileSystem/list:1.0 list 1.0}\
IDL:CF/FileSystem/FileInformationSequence:1.0 {{in pattern string}}\
{IDL:CF/FileException:1.0 IDL:CF/InvalidFileName:1.0}} {operation\
{IDL:CF/FileSystem/create:1.0 create 1.0} IDL:CF/File:1.0 {{in fileName\
string}} {IDL:CF/InvalidFileName:1.0 IDL:CF/FileException:1.0}} {operation\
{IDL:CF/FileSystem/open:1.0 open 1.0} IDL:CF/File:1.0 {{in fileName string}\
{in read_Only boolean}} {IDL:CF/InvalidFileName:1.0\
IDL:CF/FileException:1.0}} {operation {IDL:CF/FileSystem/mkdir:1.0 mkdir 1.0}\
void {{in directoryName string}} {IDL:CF/InvalidFileName:1.0\
IDL:CF/FileException:1.0}} {operation {IDL:CF/FileSystem/rmdir:1.0 rmdir 1.0}\
void {{in directoryName string}} {IDL:CF/InvalidFileName:1.0\
IDL:CF/FileException:1.0}} {operation {IDL:CF/FileSystem/query:1.0 query 1.0}\
void {{inout fileSystemProperties IDL:CF/Properties:1.0}}\
IDL:CF/FileSystem/UnknownFileSystemProperties:1.0}}} {exception\
{IDL:CF/InvalidProfile:1.0 InvalidProfile 1.0} {} {}} {interface\
{IDL:CF/ResourceFactory:1.0 ResourceFactory 1.0} {} {{exception\
{IDL:CF/ResourceFactory/InvalidResourceId:1.0 InvalidResourceId 1.0} {} {}}\
{exception {IDL:CF/ResourceFactory/ShutdownFailure:1.0 ShutdownFailure 1.0}\
{{msg string}} {}} {exception\
{IDL:CF/ResourceFactory/CreateResourceFailure:1.0 CreateResourceFailure 1.0}\
{{errorNumber IDL:CF/ErrorNumberType:1.0} {msg string}} {}} {attribute\
{IDL:CF/ResourceFactory/identifier:1.0 identifier 1.0} string readonly}\
{operation {IDL:CF/ResourceFactory/createResource:1.0 createResource 1.0}\
IDL:CF/Resource:1.0 {{in resourceId string} {in qualifiers\
IDL:CF/Properties:1.0}} IDL:CF/ResourceFactory/CreateResourceFailure:1.0}\
{operation {IDL:CF/ResourceFactory/releaseResource:1.0 releaseResource 1.0}\
void {{in resourceId string}} IDL:CF/ResourceFactory/InvalidResourceId:1.0}\
{operation {IDL:CF/ResourceFactory/shutdown:1.0 shutdown 1.0} void {}\
IDL:CF/ResourceFactory/ShutdownFailure:1.0}}} {interface\
{IDL:CF/FileManager:1.0 FileManager 1.0} IDL:CF/FileSystem:1.0 {{struct\
{IDL:CF/FileManager/MountType:1.0 MountType 1.0} {{mountPoint string} {fs\
IDL:CF/FileSystem:1.0}} {}} {typedef {IDL:CF/FileManager/MountSequence:1.0\
MountSequence 1.0} {sequence IDL:CF/FileManager/MountType:1.0}} {exception\
{IDL:CF/FileManager/NonExistentMount:1.0 NonExistentMount 1.0} {} {}}\
{exception {IDL:CF/FileManager/InvalidFileSystem:1.0 InvalidFileSystem 1.0}\
{} {}} {exception {IDL:CF/FileManager/MountPointAlreadyExists:1.0\
MountPointAlreadyExists 1.0} {} {}} {operation {IDL:CF/FileManager/mount:1.0\
mount 1.0} void {{in mountPoint string} {in file_System\
IDL:CF/FileSystem:1.0}} {IDL:CF/InvalidFileName:1.0\
IDL:CF/FileManager/InvalidFileSystem:1.0\
IDL:CF/FileManager/MountPointAlreadyExists:1.0}} {operation\
{IDL:CF/FileManager/unmount:1.0 unmount 1.0} void {{in mountPoint string}}\
IDL:CF/FileManager/NonExistentMount:1.0} {operation\
{IDL:CF/FileManager/getMounts:1.0 getMounts 1.0}\
IDL:CF/FileManager/MountSequence:1.0 {} {}}}} {interface {IDL:CF/Port:1.0\
Port 1.0} {} {{exception {IDL:CF/Port/InvalidPort:1.0 InvalidPort 1.0}\
{{errorCode {unsigned short}} {msg string}} {}} {exception\
{IDL:CF/Port/OccupiedPort:1.0 OccupiedPort 1.0} {} {}} {operation\
{IDL:CF/Port/connectPort:1.0 connectPort 1.0} void {{in connection Object}\
{in connectionId string}} {IDL:CF/Port/InvalidPort:1.0\
IDL:CF/Port/OccupiedPort:1.0}} {operation {IDL:CF/Port/disconnectPort:1.0\
disconnectPort 1.0} void {{in connectionId string}}\
IDL:CF/Port/InvalidPort:1.0}}} {interface {IDL:CF/DomainManager:1.0\
DomainManager 1.0} IDL:CF/PropertySet:1.0 {{exception\
{IDL:CF/DomainManager/ApplicationInstallationError:1.0\
ApplicationInstallationError 1.0} {{errorNumber IDL:CF/ErrorNumberType:1.0}\
{msg string}} {}} {exception\
{IDL:CF/DomainManager/ApplicationAlreadyInstalled:1.0\
ApplicationAlreadyInstalled 1.0} {} {}} {typedef\
{IDL:CF/DomainManager/ApplicationSequence:1.0 ApplicationSequence 1.0}\
{sequence IDL:CF/Application:1.0}} {typedef\
{IDL:CF/DomainManager/ApplicationFactorySequence:1.0\
ApplicationFactorySequence 1.0} {sequence IDL:CF/ApplicationFactory:1.0}}\
{typedef {IDL:CF/DomainManager/DeviceManagerSequence:1.0\
DeviceManagerSequence 1.0} {sequence IDL:CF/DeviceManager:1.0}} {exception\
{IDL:CF/DomainManager/InvalidIdentifier:1.0 InvalidIdentifier 1.0} {} {}}\
{exception {IDL:CF/DomainManager/DeviceManagerNotRegistered:1.0\
DeviceManagerNotRegistered 1.0} {} {}} {exception\
{IDL:CF/DomainManager/ApplicationUninstallationError:1.0\
ApplicationUninstallationError 1.0} {{errorNumber IDL:CF/ErrorNumberType:1.0}\
{msg string}} {}} {exception {IDL:CF/DomainManager/RegisterError:1.0\
RegisterError 1.0} {{errorNumber IDL:CF/ErrorNumberType:1.0} {msg string}}\
{}} {exception {IDL:CF/DomainManager/UnregisterError:1.0 UnregisterError 1.0}\
{{errorNumber IDL:CF/ErrorNumberType:1.0} {msg string}} {}} {exception\
{IDL:CF/DomainManager/AlreadyConnected:1.0 AlreadyConnected 1.0} {} {}}\
{exception {IDL:CF/DomainManager/InvalidEventChannelName:1.0\
InvalidEventChannelName 1.0} {} {}} {exception\
{IDL:CF/DomainManager/NotConnected:1.0 NotConnected 1.0} {} {}} {attribute\
{IDL:CF/DomainManager/domainManagerProfile:1.0 domainManagerProfile 1.0}\
string readonly} {attribute {IDL:CF/DomainManager/deviceManagers:1.0\
deviceManagers 1.0} IDL:CF/DomainManager/DeviceManagerSequence:1.0 readonly}\
{attribute {IDL:CF/DomainManager/applications:1.0 applications 1.0}\
IDL:CF/DomainManager/ApplicationSequence:1.0 readonly} {attribute\
{IDL:CF/DomainManager/applicationFactories:1.0 applicationFactories 1.0}\
IDL:CF/DomainManager/ApplicationFactorySequence:1.0 readonly} {attribute\
{IDL:CF/DomainManager/fileMgr:1.0 fileMgr 1.0} IDL:CF/FileManager:1.0\
readonly} {attribute {IDL:CF/DomainManager/identifier:1.0 identifier 1.0}\
string readonly} {operation {IDL:CF/DomainManager/registerDevice:1.0\
registerDevice 1.0} void {{in registeringDevice IDL:CF/Device:1.0} {in\
registeredDeviceMgr IDL:CF/DeviceManager:1.0}}\
{IDL:CF/InvalidObjectReference:1.0 IDL:CF/InvalidProfile:1.0\
IDL:CF/DomainManager/DeviceManagerNotRegistered:1.0\
IDL:CF/DomainManager/RegisterError:1.0}} {operation\
{IDL:CF/DomainManager/registerDeviceManager:1.0 registerDeviceManager 1.0}\
void {{in deviceMgr IDL:CF/DeviceManager:1.0}}\
{IDL:CF/InvalidObjectReference:1.0 IDL:CF/InvalidProfile:1.0\
IDL:CF/DomainManager/RegisterError:1.0}} {operation\
{IDL:CF/DomainManager/unregisterDeviceManager:1.0 unregisterDeviceManager\
1.0} void {{in deviceMgr IDL:CF/DeviceManager:1.0}}\
{IDL:CF/InvalidObjectReference:1.0 IDL:CF/DomainManager/UnregisterError:1.0}}\
{operation {IDL:CF/DomainManager/unregisterDevice:1.0 unregisterDevice 1.0}\
void {{in unregisteringDevice IDL:CF/Device:1.0}}\
{IDL:CF/InvalidObjectReference:1.0 IDL:CF/DomainManager/UnregisterError:1.0}}\
{operation {IDL:CF/DomainManager/installApplication:1.0 installApplication\
1.0} void {{in profileFileName string}} {IDL:CF/InvalidProfile:1.0\
IDL:CF/InvalidFileName:1.0\
IDL:CF/DomainManager/ApplicationInstallationError:1.0\
IDL:CF/DomainManager/ApplicationAlreadyInstalled:1.0}} {operation\
{IDL:CF/DomainManager/uninstallApplication:1.0 uninstallApplication 1.0} void\
{{in applicationId string}} {IDL:CF/DomainManager/InvalidIdentifier:1.0\
IDL:CF/DomainManager/ApplicationUninstallationError:1.0}} {operation\
{IDL:CF/DomainManager/registerService:1.0 registerService 1.0} void {{in\
registeringService Object} {in registeredDeviceMgr IDL:CF/DeviceManager:1.0}\
{in name string}} {IDL:CF/InvalidObjectReference:1.0\
IDL:CF/DomainManager/DeviceManagerNotRegistered:1.0\
IDL:CF/DomainManager/RegisterError:1.0}} {operation\
{IDL:CF/DomainManager/unregisterService:1.0 unregisterService 1.0} void {{in\
unregisteringService Object} {in name string}}\
{IDL:CF/InvalidObjectReference:1.0 IDL:CF/DomainManager/UnregisterError:1.0}}\
{operation {IDL:CF/DomainManager/registerWithEventChannel:1.0\
registerWithEventChannel 1.0} void {{in registeringObject Object} {in\
registeringId string} {in eventChannelName string}}\
{IDL:CF/InvalidObjectReference:1.0\
IDL:CF/DomainManager/InvalidEventChannelName:1.0\
IDL:CF/DomainManager/AlreadyConnected:1.0}} {operation\
{IDL:CF/DomainManager/unregisterFromEventChannel:1.0\
unregisterFromEventChannel 1.0} void {{in unregisteringId string} {in\
eventChannelName string}} {IDL:CF/DomainManager/InvalidEventChannelName:1.0\
IDL:CF/DomainManager/NotConnected:1.0}}}} {interface\
{IDL:CF/LoadableDevice:1.0 LoadableDevice 1.0} IDL:CF/Device:1.0 {{enum\
{IDL:CF/LoadableDevice/LoadType:1.0 LoadType 1.0} {KERNEL_MODULE DRIVER\
SHARED_LIBRARY EXECUTABLE}} {exception\
{IDL:CF/LoadableDevice/InvalidLoadKind:1.0 InvalidLoadKind 1.0} {} {}}\
{exception {IDL:CF/LoadableDevice/LoadFail:1.0 LoadFail 1.0} {{errorNumber\
IDL:CF/ErrorNumberType:1.0} {msg string}} {}} {operation\
{IDL:CF/LoadableDevice/load:1.0 load 1.0} void {{in fs IDL:CF/FileSystem:1.0}\
{in fileName string} {in loadKind IDL:CF/LoadableDevice/LoadType:1.0}}\
{IDL:CF/Device/InvalidState:1.0 IDL:CF/LoadableDevice/InvalidLoadKind:1.0\
IDL:CF/InvalidFileName:1.0 IDL:CF/LoadableDevice/LoadFail:1.0}} {operation\
{IDL:CF/LoadableDevice/unload:1.0 unload 1.0} void {{in fileName string}}\
{IDL:CF/Device/InvalidState:1.0 IDL:CF/InvalidFileName:1.0}}}} {interface\
{IDL:CF/ExecutableDevice:1.0 ExecutableDevice 1.0} IDL:CF/LoadableDevice:1.0\
{{exception {IDL:CF/ExecutableDevice/InvalidProcess:1.0 InvalidProcess 1.0}\
{{errorNumber IDL:CF/ErrorNumberType:1.0} {msg string}} {}} {exception\
{IDL:CF/ExecutableDevice/InvalidFunction:1.0 InvalidFunction 1.0} {} {}}\
{typedef {IDL:CF/ExecutableDevice/ProcessID_Type:1.0 ProcessID_Type 1.0}\
long} {exception {IDL:CF/ExecutableDevice/InvalidParameters:1.0\
InvalidParameters 1.0} {{invalidParms IDL:CF/Properties:1.0}} {}} {exception\
{IDL:CF/ExecutableDevice/InvalidOptions:1.0 InvalidOptions 1.0} {{invalidOpts\
IDL:CF/Properties:1.0}} {}} {const {IDL:CF/ExecutableDevice/STACK_SIZE_ID:1.0\
STACK_SIZE_ID 1.0} string STACK_SIZE} {const\
{IDL:CF/ExecutableDevice/PRIORITY_ID:1.0 PRIORITY_ID 1.0} string PRIORITY}\
{exception {IDL:CF/ExecutableDevice/ExecuteFail:1.0 ExecuteFail 1.0}\
{{errorNumber IDL:CF/ErrorNumberType:1.0} {msg string}} {}} {operation\
{IDL:CF/ExecutableDevice/terminate:1.0 terminate 1.0} void {{in processId\
IDL:CF/ExecutableDevice/ProcessID_Type:1.0}}\
{IDL:CF/ExecutableDevice/InvalidProcess:1.0 IDL:CF/Device/InvalidState:1.0}}\
{operation {IDL:CF/ExecutableDevice/execute:1.0 execute 1.0}\
IDL:CF/ExecutableDevice/ProcessID_Type:1.0 {{in name string} {in options\
IDL:CF/Properties:1.0} {in parameters IDL:CF/Properties:1.0}}\
{IDL:CF/Device/InvalidState:1.0 IDL:CF/ExecutableDevice/InvalidFunction:1.0\
IDL:CF/ExecutableDevice/InvalidParameters:1.0\
IDL:CF/ExecutableDevice/InvalidOptions:1.0 IDL:CF/InvalidFileName:1.0\
IDL:CF/ExecutableDevice/ExecuteFail:1.0}}}}}}}

#
# ----------------------------------------------------------------------
# CosEventComm.tcl
# ----------------------------------------------------------------------
#

#
# This file was automatically generated from CosEventComm.idl
# by idl2tcl. Do not edit.
#

combat::ir add \
{{module {IDL:omg.org/CosEventComm:1.0 CosEventComm 1.0} {{exception\
{IDL:omg.org/CosEventComm/Disconnected:1.0 Disconnected 1.0} {} {}}\
{interface {IDL:omg.org/CosEventComm/PushConsumer:1.0 PushConsumer 1.0} {}\
{{operation {IDL:omg.org/CosEventComm/PushConsumer/push:1.0 push 1.0} void\
{{in data any}} IDL:omg.org/CosEventComm/Disconnected:1.0} {operation\
{IDL:omg.org/CosEventComm/PushConsumer/disconnect_push_consumer:1.0\
disconnect_push_consumer 1.0} void {} {}}}} {interface\
{IDL:omg.org/CosEventComm/PushSupplier:1.0 PushSupplier 1.0} {} {{operation\
{IDL:omg.org/CosEventComm/PushSupplier/disconnect_push_supplier:1.0\
disconnect_push_supplier 1.0} void {} {}}}} {interface\
{IDL:omg.org/CosEventComm/PullConsumer:1.0 PullConsumer 1.0} {} {{operation\
{IDL:omg.org/CosEventComm/PullConsumer/disconnect_pull_consumer:1.0\
disconnect_pull_consumer 1.0} void {} {}}}} {interface\
{IDL:omg.org/CosEventComm/PullSupplier:1.0 PullSupplier 1.0} {} {{operation\
{IDL:omg.org/CosEventComm/PullSupplier/pull:1.0 pull 1.0} any {}\
IDL:omg.org/CosEventComm/Disconnected:1.0} {operation\
{IDL:omg.org/CosEventComm/PullSupplier/try_pull:1.0 try_pull 1.0} any {{out\
has_event boolean}} IDL:omg.org/CosEventComm/Disconnected:1.0} {operation\
{IDL:omg.org/CosEventComm/PullSupplier/disconnect_pull_supplier:1.0\
disconnect_pull_supplier 1.0} void {} {}}}}}}}

#
# ----------------------------------------------------------------------
# Init
# ----------------------------------------------------------------------
#

if {$init == 0} {
    Init
    # InitGui
    InitImages
    LoadPreferences
    catch { InitORB }
    set init 1
}

set ::status "Ready."

# for Windows
after 0 "wm deiconify ."
