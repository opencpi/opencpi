#! /bin/sh
# Copyright (c) 2009 Mercury Federal Systems.
# 
# This file is part of OpenCPI.
# 
# OpenCPI is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# OpenCPI is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
# 
# You should have received a copy of the GNU Lesser General Public License
# along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.

# the next line restarts using wish8.5 on unix \
if type wish8.5 > /dev/null 2>&1 ; then exec wish8.5 "$0" ${1+"$@"} ; fi
# the next line restarts using wish85 on Windows using Cygwin \
if type wish85 > /dev/null 2>&1 ; then exec wish85 "`cygpath --windows $0`" ${1+"$@"} ; fi
# the next line complains about a missing wish \
echo "This software requires Tcl/Tk 8.5 to run." ; \
echo "Make sure that \"wish8.5\" or \"wish85\" is in your \$PATH." ; \
exit 1

if {[catch {package require Tk}]} {
    puts stderr "Oops: Tk package not found."
    exit 1
}

foreach pd {. .. ../../tcl} {
    set pdp [file join [file dirname [info script]] $pd]

    foreach pkg {bwidget combat tktable} {
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
	-message "The Lumberjack Lightweight Log Viewer requires Tcl/Tk 8.4.\
		This appears to be Tcl/Tk $::tcl_version."
    exit 1
}

if {[catch {package require BWidget}]} {
    wm withdraw .
    tk_messageBox -type ok -icon error -default ok \
	-title "BWidget Required" \
	-message "The Lumberjack Lightweight Log Viewer requires the \"BWidget\" package,\
		which does not seem to be available."
    exit 1
}

if {[catch {package require Tktable}]} {
    wm withdraw .
    tk_messageBox -type ok -icon error -default ok \
	-title "TkTable Required" \
	-message "The Lumberjack Lightweight Log Viewer requires the \"TkTable\" package,\
		which does not seem to be available."
    exit 1
}

if {[catch {package require Itcl}]} {
    wm withdraw .
    tk_messageBox -type ok -icon error -default ok \
	-title "\[Incr Tcl\] Required" \
	-message "The Lumberjack Lightweight Log Viewer requires the \"\[Incr Tcl\]\" package,\
		which does not seem to be available."
    exit 1
}

if {[catch {package require combat}]} {
    wm withdraw .
    tk_messageBox -type ok -icon error -default ok \
	-title "Combat Required" \
	-message "The Lumberjack Lightweight Log Viewer requires the \"Combat\" package,\
		which does not seem to be available."
    exit 1
}

if {![info exists init]} {
    set init 0
}

#
# ----------------------------------------------------------------------
# GUI and other Initialization
# ----------------------------------------------------------------------
#

proc Init {} {
    set ::config(chaseTail) 0
    set ::config(idlePollTime) 5000
    set ::config(busyPollTime) 100
    set ::config(maxRecords) 100
    array set ::tableData {}
    array unset ::tableData *
    array set ::tableData {
	0,0 {Record Id}
	0,1 {Timestamp}
	0,2 {Producer Id}
	0,3 {Producer Name}
	0,4 {Log Level}
	0,5 {Data}
    }
}

proc InitGui {} {
    option add *title*font {Arial 16 bold}
    option add *Menu*tearOff 0

    set menudesc {
	"&File" "" "" 0 {	
	    {command "E&xit" {} "Exit Lumberjack Lightweight Log Viewer" {Ctrl x} -command "Exit"}
	}
	"&Setup" "" "" 0 {
	    {command "Find Log Service in &Naming Service" {} "Look up a Log Service in a Naming Service" {Ctrl a} -command "FindLogServiceInNamingService"}
	    {command "Find Log Service by &URI ..." {} "Look up a Log Service by its Object URI" {Ctrl u} -command "FindLogServiceByURI"}
	}
	"&Admin" "" "" 0 {
	    {command "&Write Record" {} "Write a record to the log" {} -command "WriteRecordToLog"}
	    {command "&Status" {} "Log service status" {} -command "LogServiceStatus"}
	}
	"Font" "" "" 0 {
	    {command "Courier" {} "Use Courier font" {} -command "SetFont family Courier"}
	    {command "Helvetica" {} "Use Helvetica font" {} -command "SetFont family Helvetica"}
	    {command "MS Sans Serif" {} "Use MS Sans Serif font" {} -command "SetFont family {MS Sans Serif}"}
	    {separator}
	    {command "Increase size" {} "Increase font size" {} -command "SetFont size 1"}
	    {command "Decrease size" {} "Decrease font size" {} -command "SetFont size -1"}
	}
	"&Help" "" "" 0 {
	    {command "&About" {} "About" {f1} -command "About"}
	}
    }

    if {![info exists ::status]} {
	set ::status "Initializing ..."
    }

    wm title . "Lumberjack Lightweight Log Viewer"

    set mainframe [MainFrame .mainframe \
		       -textvariable ::status \
		       -menu $menudesc]
    set allframe [$mainframe getframe]

    set queryframe [frame $allframe.query]
    set entryframe [frame $queryframe.entries]

    set timeframe [frame $entryframe.time]
    label $timeframe.dl -text "Start date:" -width 12
    entry $timeframe.date -width 12
    label $timeframe.tl -text "Time:"
    entry $timeframe.time -width 12
    pack $timeframe.dl $timeframe.date $timeframe.tl $timeframe.time \
	-side left -pady 5 -padx 5
    pack $timeframe -side top -anchor w

    set filterframe [frame $entryframe.filter]
    label $filterframe.fl -text "Filter by:" -width 12
    ComboBox $filterframe.cb -width 16 \
	-values {{(No Filter)} Level {Producer Id} {Producer Name}}
    entry $filterframe.filter -width 40
    pack $filterframe.fl $filterframe.cb $filterframe.filter \
	-side left -pady 5 -padx 5
    pack $filterframe -side top -anchor w

    pack $entryframe -side left

    button $queryframe.go -text "Go" -width 12 -command RunQuery
    checkbutton $queryframe.cb -text "Dog Mode" -padx 10 \
	-variable ::config(chaseTail) -command ToggleChaseTail
    pack $queryframe.go $queryframe.cb \
	-side left -anchor w -padx 10
    pack $queryframe -side top -fill x

    set sep1 [Separator $allframe.sep1 -orient horizontal]
    pack $sep1 -side top -fill x -pady 5

    set findframe [frame $allframe.find]
    label $findframe.dl -text "Find:" -width 12
    ComboBox $findframe.cb -width 16 \
	-values {{(Anywhere)} {Producer Id} {Producer Name} {Data}}
    entry $findframe.find -width 40
    button $findframe.go -text "Find" -width 12 -command RunFind
    button $findframe.clear -text "Clear" -width 12 -command "RunFind reset"
    pack $findframe.dl $findframe.cb $findframe.find \
	-side left -pady 5 -padx 5
    pack $findframe.go $findframe.clear \
	-side left -pady 5 -padx 10
    pack $findframe -side top -anchor w

    bind $findframe.find <Return> RunFind

    set sep2 [Separator $allframe.sep2 -orient horizontal]
    pack $sep2 -side top -fill x -pady 5

    set tf [frame $allframe.tf]
    set scrollbar [scrollbar $tf.scroll -orient vertical \
		       -command ScrollbarCallback]
    pack $scrollbar -side right -fill y
    set table [table $tf.table -multiline 0 -resizeborders col \
		   -selectmode extended \
		   -colseparator "\t" \
		   -rowseparator "\n" \
		   -cols 6 -colstretchmode last \
		   -titlerows 2 -rows 20 \
		   -variable ::tableData]

    $table width 0 10 1 12 2 20 3 20 4 20 5 40
    $table set row 0,0 {{Record Id} {Timestamp} {Producer Id} {Producer Name} {Log Level} {Data}}

    $table tag row TitleRow 0
    $table tag col RecordId 0
    $table tag col Timestamp 1
    $table tag col ProducerId 2
    $table tag col ProducerName 3
    $table tag col LogLevel 4
    $table tag col Data 5

    $table tag configure TitleRow -anchor center
    $table tag configure RecordId -anchor c -multiline 0 -state disabled
    $table tag configure Timestamp -anchor e -multiline 0 -state disabled
    $table tag configure ProducerId -anchor w -multiline 0 -state disabled
    $table tag configure ProducerName -anchor w -multiline 0 -state disabled
    $table tag configure LogLevel -anchor w -multiline 0 -state disabled
    $table tag configure Data -anchor w -multiline 0 -state disabled

    $table tag configure found -bg \#00ff00

    #
    # Embedded entry fields to filter producer id, producer name or log levels.
    #

    set clientFilterButton [button $table.filter -text "Filter" \
				-command "RunClientFilter set"]
    $table window configure 1,0 -window $clientFilterButton -sticky nsew

    set resetFilterButton [button $table.reset -text "Reset" \
				-command "RunClientFilter reset"]
    $table window configure 1,1 -window $resetFilterButton -sticky nsew

    set producerIdFilter [entry $table.producerId]
    $table window configure 1,2 -window $producerIdFilter -sticky ew

    set producerNameFilter [entry $table.producerName]
    $table window configure 1,3 -window $producerNameFilter -sticky ew

    set logLevelFilter [entry $table.logLevel]
    $table window configure 1,4 -window $logLevelFilter -sticky ew

    set dataFilter [entry $table.data]
    $table window configure 1,5 -window $dataFilter -sticky ew

    #
    # The Configure event is sent in case of a resize.
    #

    bind $table <Configure> TableResizeEvent

    #
    # Keyboard scrolling.
    #

    bind $table <Home> {ScrollbarCallback moveto 0}
    bind $table <End> {ScrollbarCallback moveto 1}
    bind $table <Up> {ScrollbarCallback scroll -1 units}
    bind $table <Down> {ScrollbarCallback scroll 1 units}
    bind . <Prior> {ScrollbarCallback scroll -1 pages}
    bind . <Next> {ScrollbarCallback scroll 1 pages}

    #
    # Make the mouse wheel work on Windows.
    #

    bind . <MouseWheel> {ScrollbarCallback scroll [expr {-%D/120}] units}

    #
    # This is supposed to make the mouse wheel work on Linux.
    #

    bind . <Button-4> {event generate [focus -displayof %W] <MouseWheel> -delta  120}
    bind . <Button-5> {event generate [focus -displayof %W] <MouseWheel> -delta -120}

    #
    # Pack table.
    #

    pack $table -side left -fill both -expand yes
    pack $tf -side top -fill both -expand yes

    #
    # Horizontal scrollbar at the bottom.
    #

    frame $allframe.xs
    scrollbar $allframe.xs.sb -orient horizontal -command "$table xview"
    frame $allframe.xs.pad -width [$scrollbar cget -width]
    pack $allframe.xs.sb -side left -fill x -expand yes -padx 0
    pack $allframe.xs.pad -side left -padx 0
    pack $allframe.xs -side top -fill x
    $table configure -xscrollcommand "$allframe.xs.sb set"

    #
    # Wrap up.
    #

    pack $mainframe -fill both -expand yes

    #
    # Remember widgets.
    #

    set ::widgets(main) $mainframe
    set ::widgets(startDate) $timeframe.date
    set ::widgets(startTime) $timeframe.time
    set ::widgets(filterType) $filterframe.cb
    set ::widgets(filterValue) $filterframe.filter
    set ::widgets(chaseTail) $queryframe.cb
    set ::widgets(findType) $findframe.cb
    set ::widgets(findValue) $findframe.find
    set ::widgets(producerIdFilter) $producerIdFilter
    set ::widgets(producerNameFilter) $producerNameFilter
    set ::widgets(logLevelFilter) $logLevelFilter
    set ::widgets(dataFilter) $dataFilter
    set ::widgets(scrollbar) $scrollbar
    set ::widgets(table) $table

    #
    # If this is done too soon, the table would show only its title rows.
    # Is this a Tk bug? A Tktable bug? My stupidity?
    #

    after 3000 {$::widgets(table) configure -rowstretchmode fill}

    #
    # Some useful bindings.
    #

    bind . <Control-f> "RunFind"
    bind . <Control-L> "Reload"
    bind . <Control-R> "Refresh"
    bind . <Control-C> "ToggleConsole"
    bind . <Control-q> "Exit"
}

#
# ----------------------------------------------------------------------
# Called when the table is being resized. This must be here because it
# may be called as soon as the GUI is initialized.
# ----------------------------------------------------------------------
#

proc TableResizeEvent {} {
    if {![info exists ::state(logRecords)]} {
	return
    }

    UpdateTable $::state(firstRowRecordIdx)

    #
    # If there are unfilled rows at the bottom, we want more data.
    #

    set totalRows [$::widgets(table) cget -rows]
    set titleRows [$::widgets(table) cget -titlerows]
    set numRows [expr {$totalRows - $titleRows}]

    if {$::state(firstEmptyRow) < $numRows} {
	#
	# If polling is in progress, we are all set. Otherwise, attempt to
	# download more data.
	#

	if {![info exists ::state(pollJobId)]} {
	    set isThereMore [GetMoreRecords]
	    UpdateTable $::state(firstRowRecordIdx)
	    SchedulePoll $isThereMore
	}
    }
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
# Map numeric log level to its name
# ----------------------------------------------------------------------
#

set ::config(logLevelEnums) [list "0" \
    "SECURITY_ALARM" "FAILURE_ALARM" "DEGRADED_ALARM" "EXCEPTION_ERROR" \
    "FLOW_CONTROL_ERROR" "RANGE_ERROR" "USAGE_ERROR" "ADMINISTRATIVE_EVENT" \
    "STATISTIC_REPORT" "PROGRAMMER_DEBUG1" "PROGRAMMER_DEBUG2" "PROGRAMMER_DEBUG3" \
    "PROGRAMMER_DEBUG4" "PROGRAMMER_DEBUG5" "PROGRAMMER_DEBUG6" "PROGRAMMER_DEBUG7" \
    "PROGRAMMER_DEBUG8" "PROGRAMMER_DEBUG9" "PROGRAMMER_DEBUG10" "PROGRAMMER_DEBUG11" \
    "PROGRAMMER_DEBUG12" "PROGRAMMER_DEBUG13" "PROGRAMMER_DEBUG14" "PROGRAMMER_DEBUG15" \
    "PROGRAMMER_DEBUG16"]

set ::config(logLevelNames) \
    [list "0" \
	 "Security Alarm" \
	 "Failure Alarm" \
	 "Degraded Alarm" \
	 "Exception Error" \
	 "Flow Control Error" \
	 "Range Error" \
	 "Usage Error" \
	 "Administrative Event" \
	 "Statistic Report"]

proc GetLogLevelName {level} {
    if {$level < [llength $::config(logLevelNames)]} {
	return [lindex $::config(logLevelNames) $level]
    } elseif {$level >= 10 && $level < 26} {
	return [format "Debug(%d)" [expr {$level-9}]]
    }

    return $level
}

proc GetLogLevelFromName {name} {
    set nl [string length $name]

    if {$nl > 0 && [string compare -nocase -length 5 $name "Debug"] == 0} {
	set idx 5
	while {$idx < $nl} {
	    set c [string index $name $idx]
	    if {[string is digit $c]} {
		break
	    }
	    incr idx
	}
	if {[scan [string range $name $idx end] "%d" debugLevel] != 1} {
	    set debugLevel 1
	}
	return [expr {$debugLevel + 9}]
    }

    for {set idx 0} {$idx < [llength $::config(logLevelNames)]} {incr idx} {
	if {$nl > 0 && [string compare -nocase -length $nl $name [lindex $::config(logLevelNames) $idx]] == 0} {
	    return $idx
	}
    }

    for {set idx 0} {$idx < [llength $::config(logLevelEnums)]} {incr idx} {
	if {$nl > 0 && [string compare -nocase -length $nl $name [lindex $::config(logLevelEnums) $idx]] == 0} {
	    return $idx
	}
    }

    if {[scan $name "%d" level] != 1} {
	set level 0
    }

    return $level
}

#
# ----------------------------------------------------------------------
# Initialize with a Log Service.
# ----------------------------------------------------------------------
#

proc InitializeLogService {logService} {
    CancelPoll

    set now [clock seconds]
    set today [clock format $now -format "%D"]
    set hour "00:00:00" ;# [clock format $now -format "%H:%M:00"]

    $::widgets(startDate) delete 0 end
    $::widgets(startDate) insert end $today

    $::widgets(startTime) delete 0 end
    $::widgets(startTime) insert end $hour

    set ::state(logService) $logService
    set ::state(logProducer) [corba::duplicate $logService]
    set ::state(logAdmin) [corba::duplicate $logService]

    if {[$::state(logService) _is_a IDL:omg.org/CosLwLog/LogStatus:1.0]} {
	set ::state(oldLogService) 0

	if {![$::state(logProducer) _is_a IDL:omg.org/CosLwLog/LogProducer:1.0]} {
	    corba::release $::state(logProducer)
	    set ::state(logProducer) 0
	}

	if {![$::state(logAdmin) _is_a IDL:omg.org/CosLwLog/LogAdministrator:1.0]} {
	    corba::release $::state(logAdmin)
	    set ::state(logAdmin) 0
	}
    } else {
	set ::state(oldLogService) 1
    }

    if {$::state(oldLogService)} {
	$::widgets(filterType) configure -state disabled
	$::widgets(filterValue) configure -state disabled
    } else {
	$::widgets(filterType) configure -state normal
	$::widgets(filterValue) configure -state normal
    }

    RunQuery
}

#
# ----------------------------------------------------------------------
# Start a new query
# ----------------------------------------------------------------------
#

proc RunQuery {} {
    if {![info exists ::state(logService)] || $::state(logService) == 0} {
	set ::status "No log service."
	return
    }

    set startDate [$::widgets(startDate) get]
    set startTime [$::widgets(startTime) get]

    if {[catch {
	set seconds [clock scan "$startTime $startDate"]
    }]} {
	set ::status "Could not read start time or date."
	tk_messageBox -parent . -type ok -icon error -default ok \
	    -title "Invalid Time Spec" \
	    -message $::status
	return
    }

    CancelPoll

    set ::state(logRecords) [list]
    set ::state(allLogRecords) [list]
    set ::state(firstRowRecordIdx) 0
    set ::state(firstEmptyRow) 0
    set ::state(numTableRows) [$::widgets(table) cget -rows]

    array unset ::tableData *
    array set ::tableData {
	0,0 {Record Id}
	0,1 {Timestamp}
	0,2 {Producer Id}
	0,3 {Producer Name}
	0,4 {Log Level}
	0,5 {Data}
    }

    corba::try {
	set logTime [list seconds $seconds nanoseconds 0]
	if {!$::state(oldLogService)} {
	    set recordId [$::state(logService) get_record_id_from_time $logTime]
	} else {
	    set recordId [$::state(logService) getRecordIdFromTime $logTime]
	}
    } catch {... ex} {
	set ::status $ex
	return
    }

    set ::state(nextRecordId) $recordId
    set ::state(filterType) [$::widgets(filterType) getvalue]
    set ::state(filterValue) [split [$::widgets(filterValue) get] ,]
    set ::state(findType) [$::widgets(findType) getvalue]
    set ::state(findValue) [$::widgets(findValue) get]
    set ::state(producerIdFilter) [string tolower [$::widgets(producerIdFilter) get]]
    set ::state(producerNameFilter) [string tolower [$::widgets(producerNameFilter) get]]
    set ::state(dataFilter) [string tolower [$::widgets(dataFilter) get]]

    set logLevelFilterTxt [$::widgets(logLevelFilter) get]
    set llft1 [string index $logLevelFilterTxt 0]
    set llft2 [string range $logLevelFilterTxt 0 1]

    switch -- $llft2 {
	"==" -
	">=" -
	"!=" -
	"<=" {
	    set logLevelFilter [string range $logLevelFilterTxt 2 end]
	    set ::state(logLevelFilterPredicate) $llft2
	}
	default {
	    switch -- $llft1 {
		">" -
		"<" {
		    set logLevelFilter [string range $logLevelFilterTxt 1 end]
		    set ::state(logLevelFilterPredicate) $llft1
		}
		"!" {
		    set logLevelFilter [string range $logLevelFilterTxt 1 end]
		    if {[string tolower [string trim $logLevelFilter]] eq "debug"} {
			set ::state(logLevelFilterPredicate) "<"
		    } else {
			set ::state(logLevelFilterPredicate) "!="
		    }
		}
		"=" {
		    set logLevelFilter [string range $logLevelFilterTxt 1 end]
		    set ::state(logLevelFilterPredicate) "=="
		}
		default {
		    set logLevelFilter $logLevelFilterTxt
		    set ::state(logLevelFilterPredicate) "=="
		}
	    }
	}
    }

    set ::state(logLevelFilter) [GetLogLevelFromName [string trim $logLevelFilter]]

    if {$::state(filterType) == 1} {
	set filterLevels [list]
	foreach name $::state(filterValue) {
	    lappend filterLevels [GetLogLevelFromName $name]
	}
	set ::state(filterValue) $filterLevels
    }

    set isThereMore [GetMoreRecords]
    UpdateTable $::state(firstRowRecordIdx)
    SchedulePoll $isThereMore
}

#
# ----------------------------------------------------------------------
# Find
# ----------------------------------------------------------------------
#

proc RunFind {{setOrReset "set"}} {
    if {$setOrReset eq "reset"} {
	$::widgets(findType) configure -text ""
	$::widgets(findValue) delete 0 end
    }

    set ::state(findType) [$::widgets(findType) getvalue]
    set ::state(findValue) [string tolower [$::widgets(findValue) get]]

    if {![info exists ::state(logRecords)]} {
	return
    }

    #
    # Clear existing tags.
    #

    set oldFound [$::widgets(table) tag cell found]

    if {[llength $oldFound]} {
	eval $::widgets(table) tag cell \{\} $oldFound
    }

    if {$::state(findValue) eq ""} {
	return
    }

    #
    # Iterate over table and find matches.
    #

    set totalRows [$::widgets(table) cget -rows]
    set titleRows [$::widgets(table) cget -titlerows]
    set numRows [expr {$totalRows - $titleRows}]
    set numRecords [llength $::state(logRecords)]

    set row 0
    set theRow [expr {$row + $titleRows}]
    set recordIdx $::state(firstRowRecordIdx)
    set foundCount 0

    while {$row < $numRows && $recordIdx < $numRecords} {
	incr foundCount [FindMarkRow $row $theRow $recordIdx]
	incr row
	incr theRow
	incr recordIdx
    }

    if {$foundCount} {
	return
    }

    #
    # No matches on the current page. Need to scroll.
    #

    CancelPoll

    set ft $::state(findType)
    set fv $::state(findValue)

    set count 0
    set wrapped 0

    if {$recordIdx >= $numRecords} {
	set recordIdx 0
	set wrapped 1
    }

    while {$recordIdx != $::state(firstRowRecordIdx)} {
	if {(($count+1) % 100) == 0} {
	    set percent [expr {100*$count/$numRecords}]
	    set ::status "Searching ... $percent %"
	    update idletasks
	}

	set record_ [lindex $::state(logRecords) $recordIdx]
	array set record $record_
	array set info $record(info)

	if {$ft == -1 || $ft == 0 || $ft == 1} {
	    if {[string first $fv [string tolower $info(producerId)]] != -1} {
		break
	    }
	}

	if {$ft == -1 || $ft == 0 || $ft == 2} {
	    if {[string first $fv [string tolower $info(producerName)]] != -1} {
		break
	    }
	}

	if {$ft == -1 || $ft == 0 || $ft == 3} {
	    if {[string first $fv [string tolower $info(logData)]] != -1} {
		break
	    }
	}

	incr count
	incr recordIdx

	if {$recordIdx >= $numRecords} {
	    set recordIdx 0
	    set wrapped 1
	}
    }

    if {$recordIdx == $::state(firstRowRecordIdx)} {
	set ::status "Search string \"$fv\" not found."
	return
    }

    #
    # Attempt to place the record in the middle of the screen.
    #

    set totalRows [$::widgets(table) cget -rows]
    set titleRows [$::widgets(table) cget -titlerows]
    set numRows [expr {$totalRows - $titleRows}]
    set middleRow [expr {$numRows / 2}]

    if {$recordIdx < $middleRow} {
	set newFirstRow 0
    } else {
	set newFirstRow [expr {$recordIdx - $middleRow}]
    }

    if {$wrapped} {
	set ::status "Search wrapped around."
    } else {
	set ::status "Found."
    }

    UpdateTable $newFirstRow
    SchedulePoll
}

proc FindMarkRow {row theRow recordIdx} {
    if {$::state(findValue) eq ""} {
	return 0
    }

    set ft $::state(findType)
    set fv $::state(findValue)

    #
    # ft == -1: find
    # ft ==  0: Anywhwere
    # ft ==  1: Producer Id
    # ft ==  2: Producer Name
    # ft ==  3: Data
    #

    set foundCount 0

    if {$ft == -1 || $ft == 0 || $ft == 1} {
	if {[string first $fv [string tolower $::tableData($theRow,2)]] != -1} {
	    $::widgets(table) tag cell found $theRow,2
	    incr foundCount
	}
    }

    if {$ft == -1 || $ft == 0 || $ft == 2} {
	if {[string first $fv [string tolower $::tableData($theRow,3)]] != -1} {
	    $::widgets(table) tag cell found $theRow,3
	    incr foundCount
	}
    }

    if {$ft == -1 || $ft == 0 || $ft == 3} {
	if {[string first $fv [string tolower $::tableData($theRow,5)]] != -1} {
	    $::widgets(table) tag cell found $theRow,5
	    incr foundCount
	}
    }

    return $foundCount
}

#
# ----------------------------------------------------------------------
# Run (client-side) Filter
# ----------------------------------------------------------------------
#

proc RunClientFilter {{setOrReset "set"}} {
    if {$setOrReset eq "reset"} {
	$::widgets(producerIdFilter) delete 0 end
	$::widgets(producerNameFilter) delete 0 end
	$::widgets(logLevelFilter) delete 0 end
	$::widgets(dataFilter) delete 0 end
    }

    set ::state(producerIdFilter) [string tolower [$::widgets(producerIdFilter) get]]
    set ::state(producerNameFilter) [string tolower [$::widgets(producerNameFilter) get]]
    set ::state(dataFilter) [string tolower [$::widgets(dataFilter) get]]

    set logLevelFilterTxt [$::widgets(logLevelFilter) get]
    set llft1 [string index $logLevelFilterTxt 0]
    set llft2 [string range $logLevelFilterTxt 0 1]

    switch -- $llft2 {
	"==" -
	">=" -
	"!=" -
	"<=" {
	    set logLevelFilter [string range $logLevelFilterTxt 2 end]
	    set ::state(logLevelFilterPredicate) $llft2
	}
	default {
	    switch -- $llft1 {
		">" -
		"<" {
		    set logLevelFilter [string range $logLevelFilterTxt 1 end]
		    set ::state(logLevelFilterPredicate) $llft1
		}
		"!" {
		    set logLevelFilter [string range $logLevelFilterTxt 1 end]
		    if {[string tolower [string trim $logLevelFilter]] eq "debug"} {
			set ::state(logLevelFilterPredicate) "<"
		    } else {
			set ::state(logLevelFilterPredicate) "!="
		    }
		}
		"=" {
		    set logLevelFilter [string range $logLevelFilterTxt 1 end]
		    set ::state(logLevelFilterPredicate) "=="
		}
		default {
		    set logLevelFilter $logLevelFilterTxt
		    set ::state(logLevelFilterPredicate) "=="
		}
	    }
	}
    }

    set ::state(logLevelFilter) [GetLogLevelFromName [string trim $logLevelFilter]]

    if {![info exists ::state(logRecords)]} {
	return
    }

    CancelPoll

    if {$::state(producerIdFilter) eq ""} {
	set filterByProducerId 0
    } else {
	set filterByProducerId 1
    }

    if {$::state(producerNameFilter) eq ""} {
	set filterByProducerName 0
    } else {
	set filterByProducerName 1
    }

    if {$::state(logLevelFilter) eq "" || \
	    $::state(logLevelFilter) == 0} {
	set filterByLogLevel 0
    } else {
	set filterByLogLevel 1
    }

    if {$::state(dataFilter) eq ""} {
	set filterByData 0
    } else {
	set filterByData 1
    }

    set numAllRecords [llength $::state(allLogRecords)]
    set newFirstRowRecordIdx -1

    if {!$filterByProducerId && \
	    !$filterByProducerName && \
	    !$filterByLogLevel && \
	    !$filterByData} {
	set filteredRecords $::state(allLogRecords)

	if {$::state(firstRowRecordIdx) < [llength $::state(logRecords)]} {
	    set record_ [lindex $::state(logRecords) $::state(firstRowRecordIdx)]
	    array set record $record_
	    set firstRowRecordId $record(id)
	    set count 0

	    foreach record_ $::state(allLogRecords) {
		array set record $record_
		if {$record(id) == $firstRowRecordId} {
		    set newFirstRowRecordIdx $count
		    break
		}
		incr count
	    }
	}
    } else {
	set filteredRecords [list]
	set count 0

	if {$::state(firstRowRecordIdx) < [llength $::state(logRecords)]} {
	    set record_ [lindex $::state(logRecords) $::state(firstRowRecordIdx)]
	    array set record $record_
	    set firstRowRecordId $record(id)
	} else {
	    set firstRowRecordId -1
	}

	foreach record_ $::state(allLogRecords) {
	    if {(($count+1) % 100) == 0} {
		set percent [expr {100*$count/$numAllRecords}]
		set ::status "Filtering ... $percent %"
		update idletasks
	    }

	    array set record $record_
	    array set info $record(info)

	    if {$record(id) == $firstRowRecordId} {
		set newFirstRowRecordIdx [llength $filteredRecords]
	    }

	    if {$filterByProducerId} {
		if {[string first $::state(producerIdFilter) [string tolower $info(producerId)]] == -1} {
		    continue
		}
	    }

	    if {$filterByProducerName} {
		if {[string first $::state(producerNameFilter) [string tolower $info(producerName)]] == -1} {
		    continue
		}
	    }

	    if {$filterByLogLevel} {
		if {$::state(oldLogService)} {
		    set level [GetLogLevelFromName $info(level)]
		} else {
		    set level $info(level)
		}

		if {$::state(logLevelFilterPredicate) eq "=="} {
		    if {$::state(logLevelFilter) != $level} {
			continue
		    }
		} else {
		    eval set pred \[expr $level $::state(logLevelFilterPredicate) $::state(logLevelFilter)\]
		    if {!$pred} {
			continue
		    }
		}
	    }

	    if {$filterByData} {
		if {[string first $::state(dataFilter) [string tolower $info(logData)]] == -1} {
		    continue
		}
	    }

	    lappend filteredRecords $record_
	    incr count
	}
    }

    set numFilteredRecords [llength $filteredRecords]

    if {$setOrReset eq "reset" || \
	    (!$filterByProducerId && !$filterByProducerName && \
		 !$filterByLogLevel && !$filterByData)} {
	set ::status "Reset filter."
    } elseif {$numFilteredRecords == $numAllRecords} {
	set ::status "Filter matches all $numAllRecords records."
    } else {
	set ::status "Filter matches $numFilteredRecords of $numAllRecords records."
    }

    set ::state(logRecords) $filteredRecords

    if {![info exists newFirstRowRecordIdx]} {
	set newFirstRowRecordIdx -1
    }

    UpdateTable $newFirstRowRecordIdx 1
    SchedulePoll
}

#
# ----------------------------------------------------------------------
# Get more records
# ----------------------------------------------------------------------
#

proc GetMoreRecords {} {
    if {![info exists ::state(logService)] || $::state(logService) == 0} {
	return 0
    }

    set currentId $::state(nextRecordId)
    set howMany $::config(maxRecords)

    corba::try {
	if {!$::state(oldLogService)} {
	    switch -- $::state(filterType) {
		1 {
		    set moreRecords [$::state(logService) \
					 retrieve_records_by_level \
					 currentId howMany \
					 $::state(filterValue)]
		}
		2 {
		    set moreRecords [$::state(logService) \
					 retrieve_records_by_producer_id \
					 currentId howMany \
					 $::state(filterValue)]
		}
		3 {
		    set moreRecords [$::state(logService) \
					 retrieve_records_by_producer_name \
					 currentId howMany \
					 $::state(filterValue)]
		}
		default {
		    set moreRecords [$::state(logService) \
					 retrieve_records \
					 currentId howMany]
		}
	    }
	} else {
	    set moreRecords [$::state(logService) \
				 retrieveById \
				 currentId $howMany]
	    set howMany [llength $moreRecords]
	}
    } catch {... ex} {
	set ::status $ex
	return -1
    }

    if {$howMany == $::config(maxRecords)} {
	set thereIsMore 1
    } else {
	set thereIsMore 0
    }

    if {$::state(producerIdFilter) eq ""} {
	set filterByProducerId 0
    } else {
	set filterByProducerId 1
    }

    if {$::state(producerNameFilter) eq ""} {
	set filterByProducerName 0
    } else {
	set filterByProducerName 1
    }

    if {$::state(logLevelFilter) eq "" || \
	    $::state(logLevelFilter) == 0} {
	set filterByLogLevel 0
    } else {
	set filterByLogLevel 1
    }

    if {$::state(dataFilter) eq ""} {
	set filterByData 0
    } else {
	set filterByData 1
    }

    set numNewFilteredRecords 0

    foreach oneRecord $moreRecords {
	lappend ::state(allLogRecords) $oneRecord

	set match 1

	if {$filterByProducerId || \
		$filterByProducerName || \
		$filterByLogLevel || \
		$filterByData} {
	    array set record $oneRecord
	    array set info $record(info)

	    if {$filterByProducerId} {
		if {[string first $::state(producerIdFilter) [string tolower $info(producerId)]] == -1} {
		    set match 0
		}
	    }

	    if {$filterByProducerName} {
		if {[string first $::state(producerNameFilter) [string tolower $info(producerName)]] == -1} {
		    set match 0
		}
	    }

	    if {$filterByLogLevel} {
		if {$::state(logLevelFilterPredicate) eq "=="} {
		    if {$::state(logLevelFilter) != $info(level)} {
			set match 0
		    }
		} else {
		    eval set pred \[expr $info(level) $::state(logLevelFilterPredicate) $::state(logLevelFilter)\]
		    if {!$pred} {
			set match 0
		    }
		}
	    }

	    if {$filterByData} {
		if {[string first $::state(dataFilter) [string tolower $info(logData)]] == -1} {
		    set match 0
		}
	    }
	}

	if {$match} {
	    lappend ::state(logRecords) $oneRecord
	    incr numNewFilteredRecords
	}
    }

    set ::state(nextRecordId) $currentId

    if {$howMany} {
	if {$howMany > 1} {
	    set records "records"
	} else {
	    set records "record"
	}
	if {$numNewFilteredRecords != $howMany} {
	    set ::status "Retrieved $howMany $records ($numNewFilteredRecords filtered)."
	} else {
	    set ::status "Retrieved $howMany $records."
	}
    } else {
	set ::status "No more records."
    }

    return $thereIsMore
}

#
# ----------------------------------------------------------------------
# Poll more records -- background task, called from the event loop
# ----------------------------------------------------------------------
#

proc PollMoreRecords {} {
    unset ::state(pollJobId)

    #
    # Calls UpdateNewRecords, which may or may not reschedule another
    # poll in the future.
    #

    set isThereMore [GetMoreRecords]
    UpdateTable $::state(firstRowRecordIdx)
    SchedulePoll $isThereMore
}

#
# ----------------------------------------------------------------------
# Schedule a new poll, if necessary
# ----------------------------------------------------------------------
#

proc SchedulePoll {{isThereMore 1}} {
    if {$isThereMore == -1} {
	#
	# GetMoreRecords experienced an error.
	#

	CancelPoll
	return
    }

    if {[info exists ::state(pollJobId)]} {
	return
    }

    set totalRows [$::widgets(table) cget -rows]
    set titleRows [$::widgets(table) cget -titlerows]
    set numRows [expr {$totalRows - $titleRows}]
    set row $::state(firstEmptyRow)

    #
    # If there are still unfilled rows at the bottom, arrange for another
    # poll. If the log actually contains more records at this time, we
    # don't pause for too long, using the busyPollTime. If the log does
    # not currently contain any more records, we can affort to wait a
    # bit longer before the next poll.
    #

    if {$row < $numRows} {
	if {$isThereMore} {
	    set nextPoll $::config(busyPollTime)
	} else {
	    set nextPoll $::config(idlePollTime)
	}
	set ::state(pollJobId) [after $nextPoll {PollMoreRecords}]
    }
}

#
# ----------------------------------------------------------------------
# Cancel polling
# ----------------------------------------------------------------------
#

proc CancelPoll {} {
    if {[info exists ::state(pollJobId)]} {
	after cancel $::state(pollJobId)
	unset ::state(pollJobId)
    }
}

#
# ----------------------------------------------------------------------
# Scroll table to start with the given line
# ----------------------------------------------------------------------
#

proc UpdateTable {newFirstRowRecordIdx {force 0}} {
    set totalRows [$::widgets(table) cget -rows]
    set titleRows [$::widgets(table) cget -titlerows]
    set numRows [expr {$totalRows - $titleRows}]
    set numRecords [llength $::state(logRecords)]

    #
    # If we did not scroll (i.e., if newFirstRowRecordIdx is the same
    # as the current firstRowRecordIdx), if there were empty rows at
    # the bottom, if these rows are now being completely filled, and
    # we are in chase mode, scroll down.
    #

    if {$newFirstRowRecordIdx == $::state(firstRowRecordIdx) && \
	    $::state(firstEmptyRow) < $numRows && \
	    ($newFirstRowRecordIdx + $numRows) <= $numRecords && \
	    $::config(chaseTail)} {
	#
	# Scroll so that the table will be half full.
	#

	set newFirstRowRecordIdx [expr {$numRecords - $numRows / 2}]
    }

    #
    # Some safety checks.
    #

    if {$newFirstRowRecordIdx < 0} {
	set newFirstRowRecordIdx 0
    } elseif {$newFirstRowRecordIdx >= $numRecords && $numRecords > 0} {
	set newFirstRowRecordIdx [expr {$numRecords - 1}]
    }

    if {$newFirstRowRecordIdx != $::state(firstRowRecordIdx) || $force} {
	#
	# Need to update all rows.
	#

	set row 0
	set theRow $titleRows
	set recordIdx $newFirstRowRecordIdx

	#
	# Reset "found" cells.
	#

	set oldFound [$::widgets(table) tag cell found]

	if {[llength $oldFound]} {
	    eval $::widgets(table) tag cell \{\} $oldFound
	}

	#
	# Clear selection
	#

	$::widgets(table) selection clear all
    } else {
	#
	# Only need to update bottom rows.
	#

	set row $::state(firstEmptyRow)
	set theRow [expr {$row + $titleRows}]
	set recordIdx [expr {$::state(firstRowRecordIdx) + $row}]
    }

    #
    # Update rows.
    #

    set tableWidth [$::widgets(table) width 5]

    while {$row < $numRows && $recordIdx < $numRecords} {
	set record_ [lindex $::state(logRecords) $recordIdx]
	array set record $record_
	array set time $record(time)
	array set info $record(info)

	set timestamp [clock format $time(seconds) -format "%H:%M:%S"]
	append timestamp [format ".%03d" [expr {$time(nanoseconds)/1000000}]]

	array set ::tableData [list \
				   $theRow,0 $record(id) \
				   $theRow,1 $timestamp \
				   $theRow,2 $info(producerId) \
				   $theRow,3 $info(producerName) \
				   $theRow,4 [GetLogLevelName $info(level)] \
				   $theRow,5 $info(logData)]

	if {[string length $info(logData)] > $tableWidth} {
	    set tableWidth [string length $info(logData)]
	}

	FindMarkRow $row $theRow $recordIdx

	incr row
	incr theRow
	incr recordIdx
    }

    #
    # Clear bottom rows.
    #

    if {$newFirstRowRecordIdx != $::state(firstRowRecordIdx) || $force} {
	while {$theRow < $totalRows} {
	    array unset ::tableData $theRow,*
	    incr theRow
	}
    }

    #
    # Set new state.
    #

    set ::state(numTableRows) $totalRows
    set ::state(firstRowRecordIdx) $newFirstRowRecordIdx
    set ::state(firstEmptyRow) $row

    #
    # Change the table width if necessary.
    #

    if {$tableWidth > [$::widgets(table) width 5]} {
	$::widgets(table) width 5 $tableWidth
    }

    #
    # Update the scrollbar.
    #

    UpdateScrollbar
}

#
# ----------------------------------------------------------------------
# Update the scrollbar
# ----------------------------------------------------------------------
#

proc UpdateScrollbar {} {
    set totalRows [$::widgets(table) cget -rows]
    set titleRows [$::widgets(table) cget -titlerows]
    set numRows [expr {$totalRows - $titleRows}]
    set numRecords [llength $::state(logRecords)]

    set upperBound [expr {double($numRecords + $numRows - 1)}]
    set scrollFirst [expr {double($::state(firstRowRecordIdx))/$upperBound}]
    set scrollLast [expr {double(($::state(firstRowRecordIdx)+$numRows-1))/$upperBound}]
    $::widgets(scrollbar) set $scrollFirst $scrollLast
}

#
# ----------------------------------------------------------------------
# Callback from the scrollbar
# ----------------------------------------------------------------------
#

proc UpdateAfterCallback {newFirstRowRecordIdx} {
    set totalRows [$::widgets(table) cget -rows]
    set titleRows [$::widgets(table) cget -titlerows]
    set numRows [expr {$totalRows - $titleRows}]
    set numRecords [llength $::state(logRecords)]

    UpdateTable $newFirstRowRecordIdx

    #
    # If there are empty rows at the bottom, try to download more data.
    #

    if {$::state(firstEmptyRow) < $numRows} {
	#
	# If polling is in progress, we are all set. Otherwise, attempt to
	# download more data.
	#

	if {![info exists ::state(pollJobId)]} {
	    set isThereMore [GetMoreRecords]
	    UpdateTable $newFirstRowRecordIdx
	    SchedulePoll $isThereMore
	}
    }
}

proc ScrollbarCallback {cmd {where none} {unit {}}} {
    if {![info exists ::state(logRecords)]} {
	return
    }

    #
    # For some reason, the callback sometimes gets called as
    # "ScrollbarCallback -1" when we are not connected to a log service
    # yet.
    #

    if {$where eq "none"} {
	return
    }

    set totalRows [$::widgets(table) cget -rows]
    set titleRows [$::widgets(table) cget -titlerows]
    set numRows [expr {$totalRows - $titleRows}]

    #
    # Figure out the new first line.
    #

    if {$cmd eq "moveto"} {
	set numRecords [llength $::state(logRecords)]
	set upperBound [expr {double($numRecords + $numRows - 1)}]
	set newFirstRowRecordIdx [expr {int($where*$upperBound)}]
    } elseif {$cmd eq "scroll"} {
	if {$unit eq "units"} {
	    set newFirstRowRecordIdx [expr {$::state(firstRowRecordIdx) + $where}]
	} elseif {$unit eq "pages"} {
	    set newFirstRowRecordIdx [expr {$::state(firstRowRecordIdx) + $where*$numRows}]

	    #
	    # If we scrolled by one page only, keep one line from the
	    # current page on screen.
	    #

	    if {$where == 1} {
		incr newFirstRowRecordIdx -1
	    } elseif {$where == -1} {
		incr newFirstRowRecordIdx
	    }
	}
    }

    #
    # Update the table, and attempt to download more data, if there are
    # unfilled rows at the bottom. But return from this callback first,
    # else the scrollbar might get confused on Unix.
    #

    after 0 "UpdateAfterCallback $newFirstRowRecordIdx"
}

#
# ----------------------------------------------------------------------
# Toggle "chase tail" mode
# ----------------------------------------------------------------------
#

proc ToggleChaseTail {} {
    set ::preferencesAreDirty 1

    if {![info exists ::state(logRecords)]} {
	return
    }

    #
    # There is nothing to do if chase tail mode is toggled off. If
    # chase tail mode is toggled on, we scroll to the bottom.
    #

    if {!$::config(chaseTail)} {
	return
    }

    set totalRows [$::widgets(table) cget -rows]
    set titleRows [$::widgets(table) cget -titlerows]
    set numRows [expr {$totalRows - $titleRows}]
    set numRecords [llength $::state(logRecords)]

    #
    # Arrange it so that the table is half full.
    #

    set newFirstRowRecordIdx [expr {$numRecords - $numRows / 2}]

    #
    # Update the table, and attempt to download more data, if there are
    # unfilled rows at the bottom. But return from this callback first,
    # else the scrollbar might get confused on Unix.
    #

    after 0 "UpdateAfterCallback $newFirstRowRecordIdx"
}

#
# ----------------------------------------------------------------------
# Write a record to the log service
# ----------------------------------------------------------------------
#

proc SendRecordToLog {} {
    set top .writeRecord

    if {![info exists ::state(logProducer)] || $::state(logProducer) == 0} {
	set ::status "No log producer."
	return
    }

    set producerId [$top.producerId.e get]
    set producerName [$top.producerName.e get]
    set logLevel [GetLogLevelFromName [$top.logLevel.e cget -text]]
    set data [$top.data.e get]

    if {$::state(oldLogService)} {
	set logLevel [lindex $::config(logLevelEnums) $logLevel]
    }

    set logRecord [list \
		       producerId $producerId \
		       producerName $producerName \
		       level $logLevel \
		       logData $data]

    corba::try {
	if {!$::state(oldLogService)} {
	    $::state(logProducer) write_record $logRecord
	} else {
	    $::state(logProducer) writeRecords [list $logRecord]
	}
    } catch {... ex} {
	set ::status $ex
    }
}

proc WriteRecordToLog {} {
    set top .writeRecord

    if {![info exists ::state(logService)] || $::state(logService) == 0} {
	set ::status "No log service."
	return
    }

    if {[winfo exists $top]} {
	wm deiconify $top
	focus $top
	return
    }

    toplevel $top
    wm title $top "Write Log Record"

    set title [label $top.title -anchor center -text "Write Log Record"]
    pack $title -side top -fill x -pady 10

    set sep1 [Separator $top.sep1 -orient horizontal]
    pack $sep1 -side top -fill x -pady 10

    frame $top.producerId
    label $top.producerId.l -text "Producer Id" -width 16
    entry $top.producerId.e -width 40
    pack $top.producerId.l -side left -padx 5 -pady 5
    pack $top.producerId.e -side left -padx 5 -pady 5 -expand yes -fill x
    pack $top.producerId -side top -anchor w

    frame $top.producerName
    label $top.producerName.l -text "Producer Name" -width 16
    entry $top.producerName.e -width 40
    pack $top.producerName.l -side left -padx 5 -pady 5
    pack $top.producerName.e -side left -padx 5 -pady 5 -expand yes -fill x
    pack $top.producerName -side top -anchor w

    frame $top.logLevel
    label $top.logLevel.l -text "Log Level" -width 16
    ComboBox $top.logLevel.e -width 20 -values $::config(logLevelNames)
    pack $top.logLevel.l -side left -padx 5 -pady 5
    pack $top.logLevel.e -side left -padx 5 -pady 5
    pack $top.logLevel -side top -anchor w

    frame $top.data
    label $top.data.l -text "Data" -width 16
    entry $top.data.e -width 40
    pack $top.data.l -side left -padx 5 -pady 5
    pack $top.data.e -side left -padx 5 -pady 5 -expand yes -fill x
    pack $top.data -side top -anchor w  -expand yes -fill x

    set sep2 [Separator $top.sep2 -orient horizontal]
    pack $sep2 -side top -fill x -pady 10

    frame $top.buts
    button $top.buts.b1 -width 15 -text "Send" \
	-command "SendRecordToLog"
    button $top.buts.b2 -width 15 -text "Close" \
	-command "wm withdraw $top"
    pack $top.buts.b1 $top.buts.b2 -side left -pady 10 -padx 20
    pack $top.buts

    # For Windows
    after 0 "wm deiconify $top"
}

#
# ----------------------------------------------------------------------
# Log Service Status
# ----------------------------------------------------------------------
#

proc AdministrateLogService {what {widget 0}} {
    if {![info exists ::state(logAdmin)] || $::state(logAdmin) == 0} {
	set ::status "No log admin."
	return
    }

    if {$::state(oldLogService)} {
	set ::status "Not supported with old log service."
	return
    }

    corba::try {
	switch -- $what {
	    maxSize {
		set size [$widget get]
		$::state(logAdmin) set_max_size $size
	    }
	    logFullAction {
		set value [$widget cget -text]
		if {$value eq "Wrap"} {
		    $::state(logAdmin) set_log_full_action WRAP
		} elseif {$value eq "Halt"} {
		    $::state(logAdmin) set_log_full_action HALT
		}
	    }
	    adminState {
		set value [$widget cget -text]
		if {$value eq "Locked"} {
		    $::state(logAdmin) set_administrative_state locked
		} elseif {$value eq "Unlocked"} {
		    $::state(logAdmin) set_administrative_state unlocked
		}
	    }
	    clearLog {
		$::state(logAdmin) clear_log
	    }
	    destroy {
		$::state(logAdmin) destroy
	    }
	}
    } catch {... ex} {
	set ::status $ex
    }
}

proc UpdateLogServiceStatus {} {
    set top .logServiceStatus

    if {[info exists ::state(logServiceStatusUpdate)] && \
	     $::state(logServiceStatusUpdate)} {
	return
    }

    if {![info exists ::state(logService)] || $::state(logService) == 0} {
	set u "<no log service>"
	$top.maxSize.e delete 0 end
	$top.maxSize.e insert 0 $u
	$top.currentSize.e configure -text $u
	$top.numRecords.e configure -text $u
	$top.logFullAction.e configure -text $u
	$top.availStatus.e configure -text $u
	$top.adminState.e configure -text $u
	$top.opState.e configure -text $u
	return
    }

    set ::state(logServiceStatusUpdate) 1
    set logService $::state(logService)

    corba::try {
	if {!$::state(oldLogService)} {
	    set maxSize [$logService get_max_size]
	} else {
	    set maxSize [$logService getMaxSize]
	}
    } catch {...} {
	set maxSize "<unavailable>"
    }

    $top.maxSize.e configure -state normal
    $top.maxSize.e delete 0 end
    $top.maxSize.e insert 0 $maxSize

    if {$::state(oldLogService)} {
        $top.maxSize.e configure -state readonly
    }

    corba::try {
	if {!$::state(oldLogService)} {
	    set currentSize [$logService get_current_size]
	} else {
	    set currentSize [$logService getCurrentSize]
	}
    } catch {...} {
	set currentSize "<unavailable>"
    }

    $top.currentSize.e configure -text $currentSize

    corba::try {
	if {!$::state(oldLogService)} {
	    set numRecords [$logService get_n_records]
	} else {
	    set numRecords [$logService getNumRecords]
	}
    } catch {...} {
	set numRecords "<unavailable>"
    }

    $top.numRecords.e configure -text $currentSize

    corba::try {
	if {!$::state(oldLogService)} {
	    set logFullAction [$logService get_log_full_action]
	} else {
	    set logFullAction [$logService getLogFullAction]
	}

	switch -- $logFullAction {
	    WRAP {
		set logFullAction "Wrap"
	    }
	    HALT {
		set logFullAction "Halt"
	    }
	    default {
		set logFullAction "<unknown>"
	    }
	}
    } catch {...} {
	set logFullAction "<unavailable>"
    }

    $top.logFullAction.e configure -text $logFullAction

    corba::try {
	if {!$::state(oldLogService)} {
	    array set as [$::state(logService) get_availability_status]
	    if {$as(off_duty) && $as(log_full)} {
		set availabilityStatus "Off duty, log full"
	    } elseif {$as(off_duty)} {
		set availabilityStatus "Off duty"
	    } elseif {$as(log_full)} {
		set availabilityStatus "Log full"
	    } else {
		set availabilityStatus "Available"
	    }
	} else {
	    array set as [$::state(logService) getAvailabilityStatus]
	    if {$as(offDuty) && $as(logFull)} {
		set availabilityStatus "Off duty, log full"
	    } elseif {$as(offDuty)} {
		set availabilityStatus "Off duty"
	    } elseif {$as(logFull)} {
		set availabilityStatus "Log full"
	    } else {
		set availabilityStatus "Available"
	    }
	}

    } catch {...} {
	set availabilityStatus "<unavailable>"
    }

    $top.availStatus.e configure -text $availabilityStatus

    corba::try {
	if {!$::state(oldLogService)} {
	    switch -- [$logService get_administrative_state] {
		locked {
		    set adminState "Locked"
		}
		unlocked {
		    set adminState "Unlocked"
		}
		default {
		    set adminState "<unknown>"
		}
	    }
	} else {
	    switch -- [$logService getAdministrativeState] {
		LOCKED {
		    set adminState "Locked"
		}
		UNLOCKED {
		    set adminState "Unlocked"
		}
		default {
		    set adminState "<unknown>"
		}
	    }
	}
    } catch {...} {
	set adminState "<unavailable>"
    }

    $top.adminState.e configure -text $adminState

    corba::try {
	if {!$::state(oldLogService)} {
	    switch -- [$logService get_operational_state] {
		disabled {
		    set opState "Disabled"
		}
		enabled {
		    set opState "Enabled"
		}
		default {
		    set opState "<unknown>"
		}
	    }
	} else {
	    switch -- [$logService getOperationalState] {
		DISABLED {
		    set opState "Disabled"
		}
		ENABLED {
		    set opState "Enabled"
		}
		default {
		    set opState "<unknown>"
		}
	    }
	}
    } catch {...} {
	set opState "<unavailable>"
    }

    $top.opState.e configure -text $opState

    set ::state(logServiceStatusUpdate) 0
}

proc WithdrawLogServiceStatus {} {
    set top .logServiceStatus

    #
    # If an update is in progress, wait until it has completed.
    #

    if {[info exists ::state(logServiceStatusUpdate)] && \
	     $::state(logServiceStatusUpdate)} {
	vwait ::state(logServiceStatusUpdate)
    }

    wm withdraw $top
}

proc LogServiceStatus {} {
    set top .logServiceStatus

    if {![info exists ::state(logService)] || $::state(logService) == 0} {
	set ::status "No log service."
	return
    }

    if {[winfo exists $top]} {
	wm deiconify $top
	focus $top
	UpdateLogServiceStatus
	return
    }

    toplevel $top
    wm title $top "Log Service Status"

    set title [label $top.title -anchor center -text "Log Service Status"]
    pack $title -side top -fill x -pady 10

    set sep1 [Separator $top.sep1 -orient horizontal]
    pack $sep1 -side top -fill x -pady 10

    frame $top.maxSize
    label $top.maxSize.l -text "Maximum Size" -width 20
    entry $top.maxSize.e -width 16
    pack $top.maxSize.l $top.maxSize.e -side left -padx 5 -pady 5
    pack $top.maxSize -side top -anchor w
    bind $top.maxSize.e <Return> "AdministrateLogService maxSize $top.maxSize.e"

    frame $top.currentSize
    label $top.currentSize.l -text "Current Size" -width 20
    label $top.currentSize.e -width 16 -anchor w
    pack $top.currentSize.l $top.currentSize.e -side left -padx 5 -pady 5
    pack $top.currentSize -side top -anchor w

    frame $top.numRecords
    label $top.numRecords.l -text "Number of Records" -width 20
    label $top.numRecords.e -width 16 -anchor w
    pack $top.numRecords.l $top.numRecords.e -side left -padx 5 -pady 5
    pack $top.numRecords -side top -anchor w

    frame $top.logFullAction
    label $top.logFullAction.l -text "Log Full Action" -width 20
    ComboBox $top.logFullAction.e -width 16 -values {Wrap Halt} \
	-modifycmd "AdministrateLogService logFullAction $top.logFullAction.e"
    pack $top.logFullAction.l $top.logFullAction.e -side left -padx 5 -pady 5
    pack $top.logFullAction -side top -anchor w
    bind $top.logFullAction.e <Return> "AdministrateLogService logFullAction $top.logFullAction.e"

    frame $top.availStatus
    label $top.availStatus.l -text "Availability Status" -width 20
    label $top.availStatus.e -width 16 -anchor w
    pack $top.availStatus.l $top.availStatus.e -side left -padx 5 -pady 5
    pack $top.availStatus -side top -anchor w

    frame $top.adminState
    label $top.adminState.l -text "Administrative State" -width 20
    ComboBox $top.adminState.e -width 16 -values {Locked Unlocked} \
	-modifycmd "AdministrateLogService adminState $top.adminState.e"
    pack $top.adminState.l $top.adminState.e -side left -padx 5 -pady 5
    pack $top.adminState -side top -anchor w
    bind $top.adminState.e <Return> "AdministrateLogService adminState $top.adminState.e"

    frame $top.opState
    label $top.opState.l -text "Operational State" -width 20
    label $top.opState.e -width 16 -anchor w
    pack $top.opState.l $top.opState.e -side left -padx 5 -pady 5
    pack $top.opState -side top -anchor w

    set sep2 [Separator $top.sep2 -orient horizontal]
    pack $sep2 -side top -fill x -pady 10

    frame $top.adminButs
    button $top.adminButs.b1 -width 15 -text "Clear Log" \
	-command "AdministrateLogService clearLog"
    button $top.adminButs.b2 -width 15 -text "Destroy" \
	-command "AdministrateLogService destroy"
    pack $top.adminButs.b1 $top.adminButs.b2 -side left -pady 10 -padx 20
    pack $top.adminButs

    set sep3 [Separator $top.sep3 -orient horizontal]
    pack $sep3 -side top -fill x -pady 10

    frame $top.buts
    button $top.buts.b1 -width 15 -text "Update" \
	-command "UpdateLogServiceStatus"
    button $top.buts.b2 -width 15 -text "Close" \
	-command "WithdrawLogServiceStatus"
    pack $top.buts.b1 $top.buts.b2 -side left -pady 10 -padx 20
    pack $top.buts

    if {![info exists ::state(logServiceStatusUpdate)]} {
	set ::state(logServiceStatusUpdate) 0
    }

    UpdateLogServiceStatus

    # For Windows
    after 0 "wm deiconify $top"
}

#
# ----------------------------------------------------------------------
# Look up Log Service in the Naming Service
# ----------------------------------------------------------------------
#

proc FindLogServiceInNamingService {} {
    set top .flsDlg

    if {[winfo exists $top]} {
	wm deiconify $top
	focus $top
	return
    }

    if {![info exists ::config(NamingServiceIOR)]} {
	set ::config(NamingServiceIOR) "corbaloc::"
	append ::config(NamingServiceIOR) [string tolower [info hostname]]
	append ::config(NamingServiceIOR) ":2809/NameService"
    }

    if {![info exists ::config(LogServiceName)]} {
	set ::config(LogServiceName) "Domain Name/Log Name"
    }

    set origNamingServiceIOR $::config(NamingServiceIOR)
    set origLogServiceName $::config(LogServiceName)

    toplevel $top
    wm title $top "Find Log Service"

    set title [label $top.title -anchor center -text "Find Log Service"]
    pack $title -side top -fill x -pady 10

    set sep1 [Separator $top.sep1 -orient horizontal]
    pack $sep1 -side top -fill x -pady 10

    LabelEntry $top.ns -label "Naming Service" \
	-labelwidth 18 -width 40 -labelanchor w \
	-textvariable ::config(NamingServiceIOR)
    pack $top.ns -side top -expand yes -fill x -pady 5 -padx 10

    LabelEntry $top.lsn -label "Log Service Name" \
	-labelwidth 18 -width 40 -labelanchor w \
	-textvariable ::config(LogServiceName)
    pack $top.lsn -side top -expand yes -fill x -pady 5 -padx 10

    set sep2 [Separator $top.sep2 -orient horizontal]
    pack $sep2 -side top -fill x -pady 10

    frame $top.buts
    set but1 [button $top.buts.b1 -width 15 -text "OK" \
		  -command "set ::lsnguimutex 1"]
    set but2 [button $top.buts.b2 -width 15 -text "Cancel" \
		  -command "set ::lsnguimutex 2"]
    pack $but1 $but2 -side left -pady 10 -padx 20
    pack $top.buts

    focus $top.ns.e
    bind $top.ns.e <Return> "set ::lsnguimutex 1"
    bind $top.lsn.e <Return> "set ::lsnguimutex 1"
    bind $top.buts.b1 <Return> "set ::lsnguimutex 1"
    bind $top.buts.b2 <Return> "set ::lsnguimutex 2"

    set ::lsnguimutex 0
    vwait ::lsnguimutex
    destroy $top

    if {$::lsnguimutex != 1} {
	return
    }

    set ::status "Contacting Naming Service ..."
    update

    set ::state(NamingService) 0
    corba::try {
	set ::state(NamingService) [corba::string_to_object $::config(NamingServiceIOR)]
	if {![$::state(NamingService) _is_a IDL:omg.org/CosNaming/NamingContextExt:1.0]} {
	    set ::state(NamingService) 0
	}
    } catch {... ex} {
	set ::state(NamingService) 0
    }

    if {$::state(NamingService) == 0} {
	set ::status "Could not contact the naming service."
	tk_messageBox -parent . -type ok -icon error -default ok \
	    -title "No Naming Service" \
	    -message $::status
	return
    }

    set ::status "Looking up log service ..."
    update

    set logService 0

    corba::try {
	set logService [corba::dii $::state(NamingService) {Object resolve_str {{in string}}} \
			    $::config(LogServiceName)]
    } catch {...} {
	set logService 0
    }

    if {$logService == 0} {
	set ::status "Could not find log service \"$::config(LogServiceName)\" in naming service."
	tk_messageBox -parent . -type ok -icon error -default ok \
	    -title "Not found" \
	    -message $::status
	return
    }

    #
    # Make sure that this Log Service is alive.
    #

    set ::status "Contacting log service ..."
    update

    corba::try {
	if {![$logService _is_a IDL:omg.org/CosLwLog/Log:1.0]} {
	    if {![$logService _is_a IDL:omg.org/CosLwLog/LogConsumer:1.0]} {
		if {![$logService _is_a IDL:LogService/Log:1.0]} {
		    set logService 0
		}
	    }
	}
    } catch {...} {
	set logService 0
    }

    if {$logService == 0} {
	set ::status "Could not contact log service."
	tk_messageBox -parent . -type ok -icon error -default ok \
	    -title "Not found" \
	    -message $::status
	return
    }

    #
    # Looks good so far.
    #

    set ::status "Found log service."
    InitializeLogService $logService

    #
    # Remember preference.
    #

    if {$origNamingServiceIOR != $::config(NamingServiceIOR) || \
	    $origLogServiceName != $::config(LogServiceName)} {
	set ::preferencesAreDirty 1
    }
}

#
# ----------------------------------------------------------------------
# Find Log Service by URI
# ----------------------------------------------------------------------
#

proc FindLogServiceByURI {} {
    set top .fls2Dlg

    if {[winfo exists $top]} {
	wm deiconify $top
	focus $top
	return
    }

    if {![info exists ::config(LogServiceIOR)]} {
	set ::config(LogServiceIOR) "corbaname:"
	append ::config(LogServiceIOR) [string tolower [info hostname]]
	append ::config(LogServiceIOR) ":2809\#Domain Name/Log Name"
    }

    set origLogServiceIOR $::config(LogServiceIOR)

    toplevel $top
    wm title $top "Find Log Service"

    set title [label $top.title -anchor center -text "Find Log Service"]
    pack $title -side top -fill x -pady 10

    set sep1 [Separator $top.sep1 -orient horizontal]
    pack $sep1 -side top -fill x -pady 10

    LabelEntry $top.uri -label "Log Service URI" \
	-labelwidth 18 -width 60 -labelanchor w \
	-textvariable ::config(LogServiceIOR)
    pack $top.uri -side top -expand yes -fill x -pady 5 -padx 10

    set sep2 [Separator $top.sep2 -orient horizontal]
    pack $sep2 -side top -fill x -pady 10

    frame $top.buts
    set but1 [button $top.buts.b1 -width 15 -text "OK" \
		  -command "set ::fls2guimutex 1"]
    set but2 [button $top.buts.b2 -width 15 -text "Cancel" \
		  -command "set ::fls2guimutex 2"]
    pack $but1 $but2 -side left -pady 10 -padx 20
    pack $top.buts

    focus $top.uri.e
    bind $top.uri.e <Return> "set ::fls2guimutex 1"
    bind $top.buts.b1 <Return> "set ::fls2guimutex 1"
    bind $top.buts.b2 <Return> "set ::fls2guimutex 2"
    
    set ::fls2guimutex 0
    vwait ::fls2guimutex
    destroy $top

    if {$::fls2guimutex != 1} {
	return
    }

    set ::status "Looking up log service ..."
    update

    set logService 0

    corba::try {
	set logService [corba::string_to_object $::config(LogServiceIOR)]
    } catch {...} {
	set logService 0
    }

    if {$logService == 0} {
	set ::status "Invalid log service URI."
	tk_messageBox -parent . -type ok -icon error -default ok \
	    -title "Not Found" \
	    -message $::status
	return
    }

    #
    # Make sure that this Log Service is alive.
    #

    set ::status "Contacting log service ..."
    update

    corba::try {
	if {![$logService _is_a IDL:omg.org/CosLwLog/Log:1.0]} {
	    if {![$logService _is_a IDL:omg.org/CosLwLog/LogConsumer:1.0]} {
		if {![$logService _is_a IDL:LogService/Log:1.0]} {
		    set logService 0
		}
	    }
	}
    } catch {...} {
	set logService 0
    }

    if {$logService == 0} {
	set ::status "Could not contact log service."
	tk_messageBox -parent . -type ok -icon error -default ok \
	    -title "Not found" \
	    -message $::status
	return
    }

    #
    # Looks good so far.
    #

    set ::status "Found log service."
    InitializeLogService $logService


    #
    # Remember preference.
    #

    if {$origLogServiceIOR != $::config(LogServiceIOR)} {
	set ::preferencesAreDirty 1
    }
}

#
# ----------------------------------------------------------------------
# Preferences
# ----------------------------------------------------------------------
#

set registryKey {HKEY_CURRENT_USER\Software\Mercury\Lumberjack}
set prefsFile ".lumberjackrc"

set preferences {
    NamingServiceIOR ascii sz
    LogServiceIOR ascii sz
    LogServiceName ascii sz
    chaseTail boolean dword
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
		set ::config($pref) $value
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
		set ::config($pref) $value
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
	if {[info exists ::config($pref)]} {
	    registry set $::registryKey $pref $::config($pref) $rtype
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
	if {[info exists ::config($pref)]} {
	    puts $f "$pref=\"$::config($pref)\""
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
    wm title $top "About Lumberjack"

    label $top.title -anchor center -bg "#ffffff" \
	-width 30 \
	-text "Lumberjack"
    pack $top.title -side top -fill x -pady 10

    label $top.subtitle -anchor center -bg "#ffffff" \
	-text "Cutting through logs most efficiently."
    pack $top.subtitle -side top -fill x -pady 10

    Separator $top.topsep -orient horizontal
    pack $top.topsep -side top -fill x -pady 10

    frame $top.imgs -bg "#ffffff"
    label $top.imgs.lab -anchor center \
	-font {Arial 14 bold} -bg "#ffffff" \
	-text "Copyright \u00a9 2009"
    label $top.imgs.img -bg "#ffffff" -anchor center \
	-image $::images(mercury)
    pack $top.imgs.lab $top.imgs.img -side top -pady 10
    pack $top.imgs -side top -fill x -pady 10

    Separator $top.botsep -orient horizontal
    pack $top.botsep -side top -fill x -pady 10

    catch {
	set revision "unknown"
	regexp {\$[[:alpha:]]+: ([^\$]*)\$$} $::cvsRevision dummy revision
    }
    catch {
	set lastModified "unknown"
	regexp {\$[[:alpha:]]+: ([^\$]*)\$$} $::cvsDate dummy lastModified
    }

    label $top.revision -anchor center -bg "#ffffff" \
	-text "Revision: $revision"
    label $top.lastModified -anchor center -bg "#ffffff" \
	-text "Last Modified: $lastModified"
    pack $top.revision -side top -fill x
    pack $top.lastModified -side top -fill x

    Separator $top.botsep2 -orient horizontal
    pack $top.botsep2 -side top -fill x -pady 10

    frame $top.botframe -bg "#ffffff"
    button $top.botframe.but -width 10 -text "OK" \
	-command "destroy $top"
    pack $top.botframe.but
    pack $top.botframe -side top -fill x -pady 10

    wm deiconify $top
    wm resizable $top 0 0
    focus $top

    bind $top <Return> "destroy $top"

    # for Windows
    after 0 "wm deiconify $top"
}

proc Reload {} {
    CancelPoll
    set ::status "Reloading [file tail $::argv0] ..."
    update
    uplevel #0 {source $::argv0}
    if {[info exists ::state(logRecords)]} {
	SchedulePoll
    }
    set ::status "Reloading Done."
}

proc ToggleConsole {} {
    if {![catch {package require tkcon}]} {
	tkcon show
    } elseif {[info commands console] eq "console"} {
	console show
    }
}

proc Refresh {} {
    CancelPoll
    set ::status "Refreshing ..."
    update
    catch {destroy $::widgets(main)}
    InitGui
    if {[info exists ::state(logRecords)]} {
	UpdateTable $::state(firstRowRecordIdx)
	SchedulePoll
    }
    set ::status "Refreshing Done."
}

proc Exit {} {
    CancelPoll
    SavePreferences
    destroy .
    exit
}

proc SetFont {cmd what} {
    if {![info exists ::widgets(table)]} {
	return
    }

    set currentFont [$::widgets(table) cget -font]
    set family [lindex $currentFont 0]
    set size [lindex $currentFont 1]
    set weight [lindex $currentFont 2]

    if {$size < 0} {
	set size [expr {-$size}]
    }

    switch -- $cmd {
	family {
	    set family $what
	}
	size {
	    incr size $what
	}
    }

    if {$weight ne ""} {
	set newFont [list $family $size]
    } else {
	set newFont [list $family $size $weight]
    }

    set ::status "Using $family-$size font."
    $::widgets(table) configure -font $newFont
}

#
# ----------------------------------------------------------------------
# Init the ORB
# ----------------------------------------------------------------------
#

proc InitORB {} {
    eval corba::init $::argv
    set ::state(NamingService) 0
}

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
# LwLogService.tcl
# ----------------------------------------------------------------------
#

#
# This file was automatically generated from CosLwLog.idl
# by idl2tcl. Do not edit.
#

package require combat

combat::ir add \
{{module {IDL:mc.com/CosLwLog:1.0 CosLwLog 1.0} {{const\
{IDL:omg.org/CosLwLog/SECURITY_ALARM:1.0 SECURITY_ALARM 1.0} {unsigned short}\
1} {const {IDL:omg.org/CosLwLog/FAILURE_ALARM:1.0 FAILURE_ALARM 1.0}\
{unsigned short} 2} {const {IDL:omg.org/CosLwLog/DEGRADED_ALARM:1.0\
DEGRADED_ALARM 1.0} {unsigned short} 3} {const\
{IDL:omg.org/CosLwLog/EXCEPTION_ERROR:1.0 EXCEPTION_ERROR 1.0} {unsigned\
short} 4} {const {IDL:omg.org/CosLwLog/FLOW_CONTROL_ERROR:1.0\
FLOW_CONTROL_ERROR 1.0} {unsigned short} 5} {const\
{IDL:omg.org/CosLwLog/RANGE_ERROR:1.0 RANGE_ERROR 1.0} {unsigned short} 6}\
{const {IDL:omg.org/CosLwLog/USAGE_ERROR:1.0 USAGE_ERROR 1.0} {unsigned\
short} 7} {const {IDL:omg.org/CosLwLog/ADMINISTRATIVE_EVENT:1.0\
ADMINISTRATIVE_EVENT 1.0} {unsigned short} 8} {const\
{IDL:omg.org/CosLwLog/STATISTIC_REPORT:1.0 STATISTIC_REPORT 1.0} {unsigned\
short} 9} {typedef {IDL:omg.org/CosLwLog/LogLevel:1.0 LogLevel 1.0} {unsigned\
short}} {typedef {IDL:omg.org/CosLwLog/LogLevelSequence:1.0 LogLevelSequence\
1.0} {sequence IDL:omg.org/CosLwLog/LogLevel:1.0}} {enum\
{IDL:omg.org/CosLwLog/OperationalState:1.0 OperationalState 1.0} {disabled\
enabled}} {enum {IDL:omg.org/CosLwLog/AdministrativeState:1.0\
AdministrativeState 1.0} {locked unlocked}} {enum\
{IDL:omg.org/CosLwLog/LogFullAction:1.0 LogFullAction 1.0} {WRAP HALT}}\
{typedef {IDL:omg.org/CosLwLog/RecordId:1.0 RecordId 1.0} {unsigned long\
long}} {typedef {IDL:omg.org/CosLwLog/StringSeq:1.0 StringSeq 1.0}\
{sequence string}} {struct {IDL:omg.org/CosLwLog/LogTime:1.0 LogTime 1.0}\
{{seconds long} {nanoseconds long}} {}} {struct\
{IDL:omg.org/CosLwLog/AvailabilityStatus:1.0 AvailabilityStatus 1.0}\
{{off_duty boolean} {log_full boolean}} {}} {struct\
{IDL:omg.org/CosLwLog/ProducerLogRecord:1.0 ProducerLogRecord 1.0}\
{{producerId string} {producerName string} {level\
IDL:omg.org/CosLwLog/LogLevel:1.0} {logData string}} {}} {typedef\
{IDL:omg.org/CosLwLog/ProducerLogRecordSequence:1.0 ProducerLogRecordSequence\
1.0} {sequence IDL:omg.org/CosLwLog/ProducerLogRecord:1.0}} {struct\
{IDL:omg.org/CosLwLog/LogRecord:1.0 LogRecord 1.0} {{id\
IDL:omg.org/CosLwLog/RecordId:1.0} {time IDL:omg.org/CosLwLog/LogTime:1.0}\
{info IDL:omg.org/CosLwLog/ProducerLogRecord:1.0}} {}} {typedef\
{IDL:omg.org/CosLwLog/LogRecordSequence:1.0 LogRecordSequence 1.0} {sequence\
IDL:omg.org/CosLwLog/LogRecord:1.0}} {interface\
{IDL:omg.org/CosLwLog/LogStatus:1.0 LogStatus 1.0} {} {{operation\
{IDL:omg.org/CosLwLog/LogStatus/get_max_size:1.0 get_max_size 1.0} {unsigned\
long long} {} {}} {operation\
{IDL:omg.org/CosLwLog/LogStatus/get_current_size:1.0 get_current_size 1.0}\
{unsigned long long} {} {}} {operation\
{IDL:omg.org/CosLwLog/LogStatus/get_n_records:1.0 get_n_records 1.0}\
{unsigned long long} {} {}} {operation\
{IDL:omg.org/CosLwLog/LogStatus/get_log_full_action:1.0 get_log_full_action\
1.0} IDL:omg.org/CosLwLog/LogFullAction:1.0 {} {}} {operation\
{IDL:omg.org/CosLwLog/LogStatus/get_availability_status:1.0\
get_availability_status 1.0} IDL:omg.org/CosLwLog/AvailabilityStatus:1.0 {}\
{}} {operation {IDL:omg.org/CosLwLog/LogStatus/get_administrative_state:1.0\
get_administrative_state 1.0} IDL:omg.org/CosLwLog/AdministrativeState:1.0 {}\
{}} {operation {IDL:omg.org/CosLwLog/LogStatus/get_operational_state:1.0\
get_operational_state 1.0} IDL:omg.org/CosLwLog/OperationalState:1.0 {} {}}}}\
{exception {IDL:omg.org/CosLwLog/InvalidParam:1.0 InvalidParam 1.0} {{Details\
string}} {}} {interface {IDL:omg.org/CosLwLog/LogConsumer:1.0 LogConsumer\
1.0} IDL:omg.org/CosLwLog/LogStatus:1.0 {{operation\
{IDL:omg.org/CosLwLog/LogConsumer/get_record_id_from_time:1.0\
get_record_id_from_time 1.0} IDL:omg.org/CosLwLog/RecordId:1.0 {{in fromTime\
IDL:omg.org/CosLwLog/LogTime:1.0}} IDL:omg.org/CosLwLog/InvalidParam:1.0}\
{operation {IDL:omg.org/CosLwLog/LogConsumer/retrieve_records:1.0\
retrieve_records 1.0} IDL:omg.org/CosLwLog/LogRecordSequence:1.0 {{inout\
currentId IDL:omg.org/CosLwLog/RecordId:1.0} {inout howMany {unsigned long}}}\
IDL:omg.org/CosLwLog/InvalidParam:1.0} {operation\
{IDL:omg.org/CosLwLog/LogConsumer/retrieve_records_by_level:1.0 retrieve_records_by_level\
1.0} IDL:omg.org/CosLwLog/LogRecordSequence:1.0 {{inout currentId\
IDL:omg.org/CosLwLog/RecordId:1.0} {inout howMany {unsigned long}} {in\
valueList IDL:omg.org/CosLwLog/LogLevelSequence:1.0}}\
IDL:omg.org/CosLwLog/InvalidParam:1.0} {operation\
{IDL:omg.org/CosLwLog/LogConsumer/retrieve_records_by_producer_id:1.0\
retrieve_records_by_producer_id 1.0} IDL:omg.org/CosLwLog/LogRecordSequence:1.0\
{{inout currentId IDL:omg.org/CosLwLog/RecordId:1.0} {inout howmany {unsigned\
long}} {in valueList IDL:omg.org/CosLwLog/StringSeq:1.0}}\
IDL:omg.org/CosLwLog/InvalidParam:1.0} {operation\
{IDL:omg.org/CosLwLog/LogConsumer/retrieve_records_by_producer_name:1.0\
retrieve_records_by_producer_name 1.0} IDL:omg.org/CosLwLog/LogRecordSequence:1.0\
{{inout currentId IDL:omg.org/CosLwLog/RecordId:1.0} {inout howmany {unsigned\
long}} {in valueList IDL:omg.org/CosLwLog/StringSeq:1.0}}\
IDL:omg.org/CosLwLog/InvalidParam:1.0}}} {interface\
{IDL:omg.org/CosLwLog/LogProducer:1.0 LogProducer 1.0}\
IDL:omg.org/CosLwLog/LogStatus:1.0 {{operation\
{IDL:omg.org/CosLwLog/LogProducer/write_record:1.0 write_record 1.0} void\
{{in record IDL:omg.org/CosLwLog/ProducerLogRecord:1.0}} {} oneway}\
{operation {IDL:omg.org/CosLwLog/LogProducer/write_records:1.0 write_records\
1.0} void {{in records IDL:omg.org/CosLwLog/ProducerLogRecordSequence:1.0}}\
{} oneway}}} {interface {IDL:omg.org/CosLwLog/LogAdministrator:1.0\
LogAdministrator 1.0} IDL:omg.org/CosLwLog/LogStatus:1.0 {{operation\
{IDL:omg.org/CosLwLog/LogAdministrator/set_max_size:1.0 set_max_size 1.0}\
void {{in size {unsigned long long}}} IDL:omg.org/CosLwLog/InvalidParam:1.0}\
{operation {IDL:omg.org/CosLwLog/LogAdministrator/set_log_full_action:1.0\
set_log_full_action 1.0} void {{in action\
IDL:omg.org/CosLwLog/LogFullAction:1.0}} {}} {operation\
{IDL:omg.org/CosLwLog/LogAdministrator/set_administrative_state:1.0\
set_administrative_state 1.0} void {{in state\
IDL:omg.org/CosLwLog/AdministrativeState:1.0}} {}} {operation\
{IDL:omg.org/CosLwLog/LogAdministrator/clear_log:1.0 clear_log 1.0} void {}\
{}} {operation {IDL:omg.org/CosLwLog/LogAdministrator/destroy:1.0 destroy\
1.0} void {} {}}}} {interface {IDL:omg.org/CosLwLog/Log:1.0\
Log 1.0} {IDL:omg.org/CosLwLog/LogConsumer:1.0\
IDL:omg.org/CosLwLog/LogProducer:1.0\
IDL:omg.org/CosLwLog/LogAdministrator:1.0} {}}}}}

#
# ----------------------------------------------------------------------
# LogService.tcl (from SCA 2.2)
# ----------------------------------------------------------------------
#

#
# This file was automatically generated from LogService.idl
# by idl2tcl. Do not edit.
#

package require combat

combat::ir add \
{{module {IDL:LogService:1.0 LogService 1.0} {{enum\
{IDL:LogService/LogLevelType:1.0 LogLevelType 1.0} {SECURITY_ALARM\
FAILURE_ALARM DEGRADED_ALARM EXCEPTION_ERROR FLOW_CONTROL_ERROR RANGE_ERROR\
USAGE_ERROR ADMINISTRATIVE_EVENT STATISTIC_REPORT PROGRAMMER_DEBUG1\
PROGRAMMER_DEBUG2 PROGRAMMER_DEBUG3 PROGRAMMER_DEBUG4 PROGRAMMER_DEBUG5\
PROGRAMMER_DEBUG6 PROGRAMMER_DEBUG7 PROGRAMMER_DEBUG8 PROGRAMMER_DEBUG9\
PROGRAMMER_DEBUG10 PROGRAMMER_DEBUG11 PROGRAMMER_DEBUG12 PROGRAMMER_DEBUG13\
PROGRAMMER_DEBUG14 PROGRAMMER_DEBUG15 PROGRAMMER_DEBUG16}} {typedef\
{IDL:LogService/LogLevelSequenceType:1.0 LogLevelSequenceType 1.0} {sequence\
IDL:LogService/LogLevelType:1.0}} {struct\
{IDL:LogService/ProducerLogRecordType:1.0 ProducerLogRecordType 1.0}\
{{producerId string} {producerName string} {level\
IDL:LogService/LogLevelType:1.0} {logData string}} {}} {interface\
{IDL:LogService/Log:1.0 Log 1.0} {} {{enum\
{IDL:LogService/Log/AdministrativeStateType:1.0 AdministrativeStateType 1.0}\
{LOCKED UNLOCKED}} {struct {IDL:LogService/Log/AvailabilityStatusType:1.0\
AvailabilityStatusType 1.0} {{offDuty boolean} {logFull boolean}} {}} {enum\
{IDL:LogService/Log/LogFullActionType:1.0 LogFullActionType 1.0} {WRAP HALT}}\
{enum {IDL:LogService/Log/OperationalStateType:1.0 OperationalStateType 1.0}\
{DISABLED ENABLED}} {exception {IDL:LogService/Log/InvalidLogFullAction:1.0\
InvalidLogFullAction 1.0} {{Details string}} {}} {exception\
{IDL:LogService/Log/InvalidParam:1.0 InvalidParam 1.0} {{details string}} {}}\
{struct {IDL:LogService/Log/LogTimeType:1.0 LogTimeType 1.0} {{seconds long}\
{nanoseconds long}} {}} {typedef {IDL:LogService/Log/RecordIdType:1.0\
RecordIdType 1.0} {unsigned long long}} {struct\
{IDL:LogService/Log/LogRecordType:1.0 LogRecordType 1.0} {{id\
IDL:LogService/Log/RecordIdType:1.0} {time\
IDL:LogService/Log/LogTimeType:1.0} {info\
IDL:LogService/ProducerLogRecordType:1.0}} {}} {typedef\
{IDL:LogService/Log/ProducerLogRecordSequence:1.0 ProducerLogRecordSequence\
1.0} {sequence IDL:LogService/ProducerLogRecordType:1.0}} {typedef\
{IDL:LogService/Log/LogRecordSequence:1.0 LogRecordSequence 1.0} {sequence\
IDL:LogService/Log/LogRecordType:1.0}} {operation\
{IDL:LogService/Log/getMaxSize:1.0 getMaxSize 1.0} {unsigned long long} {}\
{}} {operation {IDL:LogService/Log/setMaxSize:1.0 setMaxSize 1.0} void {{in\
size {unsigned long long}}} IDL:LogService/Log/InvalidParam:1.0} {operation\
{IDL:LogService/Log/getCurrentSize:1.0 getCurrentSize 1.0} {unsigned long\
long} {} {}} {operation {IDL:LogService/Log/getNumRecords:1.0 getNumRecords\
1.0} {unsigned long long} {} {}} {operation\
{IDL:LogService/Log/getLogFullAction:1.0 getLogFullAction 1.0}\
IDL:LogService/Log/LogFullActionType:1.0 {} {}} {operation\
{IDL:LogService/Log/setLogFullAction:1.0 setLogFullAction 1.0} void {{in\
action IDL:LogService/Log/LogFullActionType:1.0}} {}} {operation\
{IDL:LogService/Log/getAvailabilityStatus:1.0 getAvailabilityStatus 1.0}\
IDL:LogService/Log/AvailabilityStatusType:1.0 {} {}} {operation\
{IDL:LogService/Log/getAdministrativeState:1.0 getAdministrativeState 1.0}\
IDL:LogService/Log/AdministrativeStateType:1.0 {} {}} {operation\
{IDL:LogService/Log/setAdministrativeState:1.0 setAdministrativeState 1.0}\
void {{in state IDL:LogService/Log/AdministrativeStateType:1.0}} {}}\
{operation {IDL:LogService/Log/getOperationalState:1.0 getOperationalState\
1.0} IDL:LogService/Log/OperationalStateType:1.0 {} {}} {operation\
{IDL:LogService/Log/writeRecords:1.0 writeRecords 1.0} void {{in records\
IDL:LogService/Log/ProducerLogRecordSequence:1.0}} {} oneway} {operation\
{IDL:LogService/Log/getRecordIdFromTime:1.0 getRecordIdFromTime 1.0}\
IDL:LogService/Log/RecordIdType:1.0 {{in fromTime\
IDL:LogService/Log/LogTimeType:1.0}} {}} {operation\
{IDL:LogService/Log/retrieveById:1.0 retrieveById 1.0}\
IDL:LogService/Log/LogRecordSequence:1.0 {{inout currentId\
IDL:LogService/Log/RecordIdType:1.0} {in howMany {unsigned long}}} {}}\
{operation {IDL:LogService/Log/clearLog:1.0 clearLog 1.0} void {} {}}\
{operation {IDL:LogService/Log/destroy:1.0 destroy 1.0} void {} {}}}}}}}



#
# ----------------------------------------------------------------------
# Init
# ----------------------------------------------------------------------
#

if {$init == 0} {
    Init
    # InitGui
    InitORB
    set init 1
    LoadPreferences

    corba::try {
	set log [corba::resolve_initial_references LogService]
	InitializeLogService $log
    } catch {...} {
    }
}

set ::status "Ready."

# for Windows
after 0 "wm deiconify ."
