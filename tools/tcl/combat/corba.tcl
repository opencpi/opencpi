#
# ======================================================================
# This file is part of Combat/Tcl, a Tcl CORBA Object Request Broker
#
# Please visit the Combat Web site at http://www.fpx.de/Combat/ for
# more information.
#
# Copyright (c) Frank Pilhofer, combat@fpx.de
#
# ======================================================================
#
# CVS Version Tag: $Id: corba.tcl,v 1.30 2008/11/14 02:06:58 Owner Exp $
#
# ----------------------------------------------------------------------
# CORBA Module
# ----------------------------------------------------------------------
#

namespace eval Combat {
    variable initialized 0
    variable debug

    array set debug {
	info      0
	orb       0
	giop      0
	iiop      0
	transport 0
	poa       0
    }

    proc Debug {args} {
	if {[string tolower $args] == "all"} {
	    set args [array names ::Combat::debug]
	}
	foreach arg $args {
	    set ::Combat::debug([string tolower $arg]) 1
	}
    }

    namespace eval CORBA {
	variable VMCID 0
	variable OMGVMCID 1330446336	;# 0x4f4d0000
    }

    proc DumpOctets {desc data} {
	set index 0
	
	while {$index < [string length $data]} {
	    if {$index == 0} {
		puts -nonewline stderr "[format %12s $desc]  "
	    } else {
		puts -nonewline stderr "              "
	    }
	    
	    set count [binary scan $data \
		    @[set index]H2H2H2H2H2H2H2H2H2H2H2H2H2H2H2H2 \
		    c(0) c(1) c(2) c(3) c(4) c(5) c(6) c(7) c(8) \
		    c(9) c(10) c(11) c(12) c(13) c(14) c(15) c(16)]
	    
	    for {set i 0} {$i < $count} {incr i} {
		puts -nonewline stderr "$c($i) "
	    }
	    for {} {$i < 16} {incr i} {
		puts -nonewline stderr "   "
	    }
	    
	    binary scan $data \
		    @[set index]cccccccccccccccc \
		    c(0) c(1) c(2) c(3) c(4) c(5) c(6) c(7) c(8) \
		    c(9) c(10) c(11) c(12) c(13) c(14) c(15) c(16)
	    
	    for {set i 0} {$i < $count} {incr i} {
		#
		# Printable ISOLatin1 characters according to the
		# Red Book (Postscript RM)
		#
		if {($c($i) >= 0040 && $c($i) <= 0176) || \
			($c($i) >= 0220 && $c($i) != 0231 && $c($i) != 0234)} {
		    puts -nonewline stderr "[string index $data [expr {$index + $i}]]"
		} else {
		    puts -nonewline stderr "."
		}
	    }
	    
	    puts ""
	    incr index $count
	}
    }
}

#
# ----------------------------------------------------------------------
# corba Namespace
# ----------------------------------------------------------------------
#

namespace eval corba {
    #
    # corba::init <orb-specific-args> <other-args>
    #

    proc init {args} {
	if {$::Combat::initialized} {
	    return $args
	}

	set rest [list]
	set runserver 0
	set ::Combat::CORBA::ORB::iiop_serverports [list]
	set ::Combat::CORBA::ORB::hostname [info hostname]
	set native_codeset -1

	for {set idx 0} {$idx < [llength $args]} {incr idx} {
	    switch -glob -- [lindex $args $idx] {
		-ORBInitRef {
		    incr idx
		    set initref [lindex $args $idx]
		    set eql [string first = $initref]
		    if {$eql == -1} {
			error "oops"
		    }
		    set name [string range $initref 0 [expr {$eql - 1}]]
		    set value [string range $initref [expr {$eql + 1}] end]
		    set ::Combat::CORBA::ORB::initial_references($name) \
			    $value
		}
		-ORBDefaultInitRef {
		    incr idx
		    set ::Combat::CORBA::ORB::default_init_ref \
			    [lindex $args $idx]
		}
		-ORBDebug {
		    incr idx
		    set what [lindex $args $idx]
		    eval ::Combat::Debug [split $what ,]
		}
		-ORBServer {
		    set runserver 1
		}
		-ORBServerPort {
		    incr idx
		    lappend ::Combat::CORBA::ORB::iiop_serverports \
			    [lindex $args $idx]
		}
		-ORBGIOPMaxSize -
		-ORBGiopMaxSize {
		    incr idx
		    set arg [lindex $args $idx]
		    switch -- [scan $arg "%d%s" value postfix] {
			1 {}
			2 {
			    switch -- $postfix {
				k -
				K {
				    set value [expr {$value * 1024}]
				}
				m -
				M {
				    set value [expr {$value * 1024 * 1024}]
				}
				g -
				G {
				    set value [expr {$value*1024*1024*1024}]
				}
				default {
				    error "illegal value for -ORBGIOPMaxSize: $arg"
				}
			    }
			}
			default {
			    error "illegal value for -ORBGIOPMaxSize: $arg"
			}
		    }
		    set ::Combat::GIOP::ORBGIOPMaxSize $value
		}
		-ORBServerId -
		-ORBImplName -
		-ORBPOAImplName -
		-POAImplName {
		    incr idx
		    set ::Combat::PortableServer::POAImplName \
			    [lindex $args $idx]
		}
		-ORBNativeCodeSet {
		    incr idx
		    set native_codeset [lindex $args $idx]
		}
		-ORBHostName {
		    incr idx
		    set ::Combat::CORBA::ORB::hostname [lindex $args $idx]
		}
		-ORBConnectionIdleTimeout {
		    incr idx
		    set ::Combat::GIOP::idleTimeout [lindex $args $idx]
		}
		-ORBSendCancelRequestMessage {
		    incr idx
		    set ::Combat::GIOP::sendCancelRequest [lindex $args $idx]
		}
		-ORB* {
		    error "illegal -ORB* option: [lindex $args $idx]"
		}
		default {
		    lappend rest [lindex $args $idx]
		}
	    }
	}

	#
	# Init Subsystems. Server side is initialized when -ORBServer is
	# present; otherwise, server-side initialization is delayed until
	# the RootPOA initial reference is resolved.
	#

	::Combat::CONV_FRAME::init $native_codeset

	if {$runserver} {
	    init_server_side
	}

	#
	# Done
	#

	set ::Combat::initialized 1
	return $rest
    }

    proc init_server_side {} {
	if {[info exists ::Combat::CORBA::ORB::ior_template]} {
	    return
	}

	#
	# Initialize TCP Server
	#

	set profiles [list]
	set ::Combat::CORBA::ORB::iiop_servers [list]

	if {[llength $::Combat::CORBA::ORB::iiop_serverports] == 0} {
	    set prof [namespace current]::[::Combat::IIOP::TCPServer \#auto]
	    $prof listen
	    lappend profiles [$prof profile]
	    lappend ::Combat::CORBA::ORB::iiop_servers $prof
	} else {
	    foreach port $::Combat::CORBA::ORB::iiop_serverports {
		set prof [namespace current]::[::Combat::IIOP::TCPServer \#auto]
		$prof listen $port
		lappend profiles [$prof profile]
		lappend ::Combat::CORBA::ORB::iiop_servers $prof
	    }
	}

	#
	# Setup code set profile, speaking ISO 8859-1 and ISO 10646-1 (UTF-16)
	#

	set csinfo [::Combat::CONV_FRAME::MakeCodeSetInfo]
	set mcp [namespace current]::[::Combat::IOP::MultipleComponentProfile \#auto]
	$mcp configure -components [list $csinfo]
	lappend profiles $mcp

	#
	# Init IOR Template
	#

	set ::Combat::CORBA::ORB::ior_template \
		[namespace current]::[::Combat::IOP::IOR \#auto]
	$::Combat::CORBA::ORB::ior_template configure -profiles $profiles

	#
	# Init Portable Server module
	#
	
	::Combat::PortableServer::init
    }

    #
    # corba::resolve_initial_references <service>
    #
    
    proc resolve_initial_references {name} {
	::corba::init

	if {$name == "RootPOA" || $name == "POACurrent"} {
	    init_server_side
	}

	if {[info exists ::Combat::CORBA::ORB::initial_pseudo_references($name)]} {
	    return $::Combat::CORBA::ORB::initial_pseudo_references($name)
	}

	if {[info exists ::Combat::CORBA::ORB::initial_references($name)]} {
	    return [string_to_object \
		    $::Combat::CORBA::ORB::initial_references($name)]
	}

	if {$name == "CodecFactory"} {
	    set ::Combat::CORBA::ORB::initial_pseudo_references(CodecFactory) \
		[namespace current]::[::Combat::IOP::CodecFactory \#auto]
	    return $::Combat::CORBA::ORB::initial_pseudo_references(CodecFactory)
	}

	if {$::Combat::CORBA::ORB::default_init_ref != ""} {
	    set ref "[set ::Combat::CORBA::ORB::default_init_ref]/$name"
	    if {![catch {set res [string_to_object $ref]}]} {
		return $res
	    }
	}
	corba::throw [list IDL:omg.org/CORBA/ORB/InvalidName:1.0 \
		[list no_such_initial_reference $name]]
    }

    #
    # corba::list_initial_services
    #

    proc list_initial_services {} {
	::corba::init
	set refs [list]

	if {[info exists ::Combat::CORBA::ORB::initial_pseudo_references]} {
	    set refs [concat $refs [array names ::Combat::CORBA::ORB::initial_references]]
	    if {![info exists ::Combat::CORBA::ORB::initial_pseudo_references(CodecFactory)]} {
		lappend refs "CodecFactory"
	    }
	} else {
	    lappend refs "CodecFactory"
	}

	if {[info exists ::Combat::CORBA::ORB::initial_references]} {
	    set refs [concat $refs [array names ::Combat::CORBA::ORB::initial_references]]
	}

	return $refs
    }

    #
    # corba::register_initial_reference
    #

    proc register_initial_reference {id obj} {
	::corba::init
	set ::Combat::CORBA::ORB::initial_references($id) \
		[object_to_string $obj]
    }

    #
    # corba::throw <exception>
    #
    
    proc throw {ex} {
	set l [llength $ex]
	if {$l == 1} {
	    error [::Combat::SimpleTypeRepository::getRepoid [lindex $ex 0]]
	} elseif {$l == 2} {
	    set rid [::Combat::SimpleTypeRepository::getRepoid [lindex $ex 0]]
	    error [list $rid [lindex $ex 1]]
	} else {
	    error $ex
	}
    }
    
    #
    # corba::try <block> ?catch {exception ?var?} block? ... ?finally block?
    #

    proc try {block args} {
	set code [catch {uplevel 1 $block} res]
	if {$code == 1} {
	    if {[catch {set exid [::Combat::SimpleTypeRepository::getRepoid [lindex $res 0]]}]} {
		set exid "..."
	    }
	    set hascatch 0
	    foreach {name exception cblock} $args {
		if {$name == "catch"} {
		    set hascatch 1
		    set repoid [lindex $exception 0]
		    if {$repoid == "..." || [::Combat::SimpleTypeRepository::getRepoid $repoid] == $exid} {
			set varname [lindex $exception 1]
			if {$varname != ""} {
			    uplevel 1 "set $varname \{$res\}"
			}
			set code [catch {uplevel 1 $cblock} res]
			break
		    }
		}
	    }
	    if {!$hascatch} {
		set code 0
	    }
	}
	foreach {name fblock dummy} $args {
	    if {$name == "finally"} {
		catch {uplevel 1 $fblock}
		break
	    }
	}
	return -code $code $res
    }

    #
    # helpers
    #
    
    proc url_decode {str} {
	set res ""
	set beg 0
	set end [string first % $str]
	while {$end != -1} {
	    append res [string range $str $beg [expr {$end - 1}]]
	    append res [binary format H [string range $str \
		    [expr {$end + 1}] [expr {$end + 2}]]]
	    set beg [expr {$end + 3}]
	    set end [string first % $str $beg]
	}
	append res [string range $str $beg end]
	return $res
    }

    proc ior_to_object {str} {
	set ior [::Combat::IOP::DestringifyIOR $str]
	set obj [namespace current]::[::Combat::CORBA::Object \#auto]
	$obj configure -ior $ior
	return [::Combat::CORBA::ORB::MakeObjProc $obj]
    }
    
    proc corbaloc_to_object {str} {
	set pos [string first / $str]
	if {$pos == -1} {
	    set obj_addr_list [split [string range $str 9 end] ,]
	    set key_string ""
	} else {
	    set obj_addr_list [split [string range $str \
		    9 [expr {$pos - 1}]] ,]
	    set key_string [url_decode [string range $str \
		    [expr {$pos + 1}] end]]
	}
	if {[string range [lindex $obj_addr_list 0] 0 3] == "rir:"} {
	    if {[llength $obj_addr_list] > 1} {
		throw IDL:omg.org/CORBA/BAD_PARAM:1.0
	    }
	    if {$key_string == ""} {
		set key_string "NameService"
	    }
	    return [resolve_initial_references $key_string]
	}
	
	if {[llength $obj_addr_list] == 0} {
	    throw IDL:omg.org/CORBA/BAD_PARAM:1.0
	}
	
	set profiles [list]
	foreach obj_addr $obj_addr_list {
	    if {[string range $obj_addr 0 0] == ":"} {
		set iiop_addr [string range $obj_addr 1 end]
	    } elseif {[string range $obj_addr 0 4] == "iiop:"} {
		set iiop_addr [string range $obj_addr 5 end]
	    } else {
		throw IDL:omg.org/CORBA/BAD_PARAM:1.0
	    }
	    
	    set pos [string first @ $iiop_addr]
	    if {$pos != -1} {
		set version [string range $iiop_addr 0 [expr {$pos - 1}]]
		set iiop_addr [string range $iiop_addr \
			[expr {$pos + 1}] end]
	    } else {
		set version "1.0"
	    }
	    
	    set pos [string first : $iiop_addr]
	    if {$pos != -1} {
		set port [string range $iiop_addr \
			[expr {$pos + 1}] end]
		set iiop_addr [string range $iiop_addr \
			0 [expr {$pos - 1}]]
	    } else {
		set port 2089
	    }
	    
	    set host $iiop_addr
	    scan $version "%d.%d" major_version minor_version
	    
	    #
	    # add IIOP Profile
	    #
	    
	    set prof [namespace current]::[::Combat::IIOP::ProfileBody \#auto]
	    $prof configure -major_version $major_version \
		    -minor_version $minor_version \
		    -host $host -port $port \
		    -object_key $key_string
	    lappend profiles $prof
	}
	
	set ior [namespace current]::[::Combat::IOP::IOR \#auto]
	set obj [namespace current]::[::Combat::CORBA::Object \#auto]
	$ior configure -profiles $profiles
	$obj configure -ior $ior
	return [::Combat::CORBA::ORB::MakeObjProc $obj]
    }

    proc corbaname_to_object {str} {
	set pos [string first # $str]
	if {$pos == -1} {
	    set corbaloc_obj "corbaloc:[string range $str 10 end]"
	    set url_name ""
	} else {
	    set corbaloc_obj "corbaloc:[string range $str 10 [expr {$pos - 1}]]"
	    set url_name [string range $str [expr {$pos + 1}] end]
	}

	if {[string first / $corbaloc_obj] == -1} {
	    append corbaloc_obj "/NameService"
	}

	set nsref [string_to_object $corbaloc_obj]

	if {$url_name == ""} {
	    return $nsref
	}

	set string_name [url_decode $url_name]

	#
	# Ask Naming Service by DII
	#

	set nsobj [::Combat::CORBA::ORB::GetObjFromRef $nsref]
	set res [::Combat::CORBA::ORB::invoke_sync 1 $nsobj resolve_str \
		Object 0 {{in string}} [list $string_name]]
	release $nsref
	return $res
    }

    proc file_to_object {str} {
	set idx [string first / $str 7]
	if {$idx == -1} {
	    error "illegal URL: $str"
	}
	
	set host [url_decode [string range $str 7 [expr {$idx - 1}]]]
	set name [url_decode [string range $str $idx end]]

	#
	# accept file://<drive>:/path
	#
	
	if {[string length $host] == 2 && \
		[string is alpha [string index $host 0]] && \
		[string index $host 1] == ":"} {
	    set name "$host$name"
	    set host ""
	}
	
	if {$host != "" && $host != "localhost" && \
		$host != [info hostname]} {
	    set name "//$host$file"
	}
	    
	#
	# accept file:///drive:/path
	#

	if {[string length $name] > 3 && \
		[string index $name 0] == "/" && \
		[string is alpha [string index $name 1]] && \
		[string index $name 2] == ":" && \
		[string index $name 3] == "/"} {
	    set name [string range $name 1 end]
	}

	set file [open $name]
	set ior [read -nonewline $file]
	close $file
	
	return [string_to_object $ior]
    }

    proc http_to_object {str} {
	package require http
	set token [::http::geturl $str]
	if {[::http::status $token] != "ok"} {
	    ::http::cleanup $token
	    throw IDL:omg.org/CORBA/BAD_PARAM:1.0
	}
	set ior [::http::data $token]
	::http::cleanup $token
	return [string_to_object $ior]
    }

    #
    # corba::string_to_object <url>
    #

    proc string_to_object {str} {
	::corba::init
        set str [string trim $str]
	if {[string range $str 0 3] == "IOR:"} {
	    return [ior_to_object $str]
	} elseif {[string range $str 0 8] == "corbaloc:"} {
	    return [corbaloc_to_object $str]
	} elseif {[string range $str 0 9] == "corbaname:"} {
	    return [corbaname_to_object $str]
	} elseif {[string range $str 0 6] == "file://"} {
	    return [file_to_object $str]
	} elseif {[string range $str 0 6] == "http://"} {
	    return [http_to_object $str]
	} else {
	    throw IDL:omg.org/CORBA/BAD_PARAM:1.0
	}
    }

    #
    # corba::object_to_string <ref>
    #

    proc object_to_string {ref} {
	::corba::init
	set obj [::Combat::CORBA::ORB::GetObjFromRef $ref]
	set ior [$obj cget -ior]
	return [$ior stringify]
    }

    #
    # corba::dii ?-async? ref spec args
    #
    # spec: [list returntype opname params exceptions]
    #

    proc dii {args} {
	return [eval ::Combat::CORBA::ORB::dii_invoke $args]
    }

    #
    # corba::request
    #

    proc request {cmd args} {
	::corba::init
	switch -- $cmd {
	    get {
		if {[llength $args] != 1} {
		    error "usage: corba::request get ref"
		}
		set aid [lindex $args 0]
		return [::Combat::CORBA::ORB::get_async_reply $aid 1]
	    }
	    cancel {
		foreach aid $args {
		    ::Combat::CORBA::ORB::cancel_async_request $aid
		}
		return ""
	    }
	    poll {
		foreach aid $args {
		    if {[::Combat::CORBA::ORB::poll_async_reply $aid]} {
			return $aid
		    }
		}
		return ""
	    }
	    wait {
		if {[llength $args] == 0} {
		    set args [array names ::Combat::CORBA::ORB::reqids_completed]
		    if {[llength $args] == 0} {
			return ""
		    }
		}
		while {42} {
		    foreach aid $args {
			if {[::Combat::CORBA::ORB::poll_async_reply $aid]} {
			    return $aid
			}
		    }
		    vwait ::Combat::CORBA::ORB_conn_cond
		}
		error "oops, I shouldn't be here."
	    }
	    default {
		error "usage: corba::request get, poll or wait"
	    }
	}
    }

    #
    # corba::release ref          Release the object reference
    # corba::release tc value     Traverse the TypeCode and release all
    #                             object references within the value
    #

    proc release {args} {
	if {[llength $args] != 1 && [llength $args] != 2} {
	    error "usage: corba::release <ref> or corba::release <tc> <value>"
	}
	if {[llength $args] == 1} {
	    if {[lindex $args 0] == 0} {
		return
	    }
	    ::Combat::CORBA::ORB::ReleaseRef [lindex $args 0]
	    return
	}

	set tc [lindex $args 0]
	set value [lindex $args 1]

	switch -- $tc {
	    void -
	    boolean -
	    short -
	    long -
	    {unsigned short} -
	    {unsigned long} -
	    {long long} -
	    {unsigned long long} -
	    float -
	    double -
	    {long double} -
	    char -
	    octet -
	    wchar -
	    string -
	    wstring -
	    TypeCode {
	    }
	    any {
		release [lindex $value 0] [lindex $value 1]
	    }
	}

	set type [lindex $tc 0]

	switch -- $type {
	    enum -
	    string -
	    wstring {
		return
	    }
	    struct {
		set repoid [lindex $tc 1]
		set marshalrecursion($repoid) $tc
		set members [lindex $tc 2]
		foreach {member_name member_value} $value {
		    set map($member_name) $member_value
		}
		foreach {member_name member_type} $members {
		    release $member_type $map($member_name)
		}
		catch {unset map}
		catch {unset marshalrecursion($repoid)}
	    }
	    union {
		# groan
	    }
	    exception {
		foreach {member_name member_value} [lindex $value 1] {
		    set map($member_name) $member_value
		}
		foreach {member_name member_type} [lindex $tc 2] {
		    release $member_type $map($member_name)
		}
		catch {unset map}
	    }
	    sequence -
	    array {
		set type [lindex $tc 1]
		if {$type == "char" || $type == "octet"} {
		    return
		}
		foreach element $value {
		    release $type $element
		}
	    }
	    Object {
		release $value
	    }
	    ValueType {
		if {$value == 0} {
		    return
		}
		set repoid [lindex $tc 1]
		set marshalrecursion($repoid) $tc
		foreach {member_name member_value} $value {
		    set map($member_name) $member_value
		}
		if {[info exists map(_tc_)]} {
		    set marshaltc $map(_tc_)
		} else {
		    set marshaltc $tc
		}
		set itc $marshaltc
		set tcs [list]
		while {$itc != 0} {
		    set tcs [linsert $tcs 0 $itc]
		    set itc [lindex $itc 3]
		}
		foreach mtc $tcs {
		    foreach {visi member_name member_type} [lindex $mtc 2] {
			release $member_type $map($member_name)
		    }
		}
		catch {unset map}
		catch {unset marshalrecursion($repoid)}
	    }
	    ValueBox {
		if {$value == 0} {
		    return
		}
		release [lindex $tc 2] $value
	    }
	    recursive {
		set repoid [lindex $tc 1]
		set savetc $marshalrecursion($repoid)
 		release $marshalrecursion($repoid) $value
		set marshalrecursion($repoid) $savetc
		return $res
	    }
	}
    }

    #
    # corba::duplicate ref        Duplicate the object reference
    # corba::duplicate tc value   Traverse the TypeCode and dup all
    #                             object references within the value
    #

    proc duplicate {args} {
	if {[llength $args] != 1 && [llength $args] != 2} {
	    error "usage: corba::duplicate <ref> or corba::duplicate <tc> <value>"
	}
	if {[llength $args] == 1} {
	    if {[lindex $args 0] == 0} {
		return 0
	    }
	    return [[lindex $args 0] _duplicate]
	}

	set tc [lindex $args 0]
	set value [lindex $args 1]

	switch -- $tc {
	    void -
	    boolean -
	    short -
	    long -
	    {unsigned short} -
	    {unsigned long} -
	    {long long} -
	    {unsigned long long} -
	    float -
	    double -
	    {long double} -
	    char -
	    octet -
	    wchar -
	    string -
	    wstring -
	    TypeCode {
		return $value
	    }
	    any {
		return [list [lindex $value 0] \
			[duplicate [lindex $value 0] [lindex $value 1]]]
	    }
	}

	set type [lindex $tc 0]

	switch -- $type {
	    enum -
	    string -
	    wstring {
		return
	    }
	    struct {
		set repoid [lindex $tc 1]
		set marshalrecursion($repoid) $tc
		set members [lindex $tc 2]
		foreach {member_name member_value} $value {
		    set map($member_name) $member_value
		}
		set result [list]
		foreach {member_name member_type} $members {
		    lappend result $member_name \
			    [duplicate $member_type $map($member_name)]
		}
		catch {unset map}
		catch {unset marshalrecursion($repoid)}
		return $result
	    }
	    union {
		# groan
	    }
	    exception {
		foreach {member_name member_value} [lindex $value 1] {
		    set map($member_name) $member_value
		}
		set result [list]
		foreach {member_name member_type} [lindex $tc 2] {
		    lappend result $member_name \
			    [duplicate $member_type $map($member_name)]
		}
		catch {unset map}
		return $result
	    }
	    sequence -
	    array {
		set type [lindex $tc 1]
		if {$type == "char" || $type == "octet"} {
		    return $value
		}
		set result [list]
		foreach element $value {
		    lappend result [duplicate $type $element]
		}
		return $result
	    }
	    Object {
		return [$value _duplicate]
	    }
	    ValueType {
		if {$value == 0} {
		    return
		}
		set repoid [lindex $tc 1]
		set marshalrecursion($repoid) $tc
		foreach {member_name member_value} $value {
		    set map($member_name) $member_value
		}
		if {[info exists map(_tc_)]} {
		    set marshaltc $map(_tc_)
		} else {
		    set marshaltc $tc
		}
		set itc $marshaltc
		set tcs [list]
		while {$itc != 0} {
		    set tcs [linsert $tcs 0 $itc]
		    set itc [lindex $itc 3]
		}
		set result [list]
		foreach mtc $tcs {
		    foreach {visi member_name member_type} [lindex $mtc 2] {
			lappend result $member_name \
				[duplicate $member_type $map($member_name)]
		    }
		}
		if {[info exists map(_tc_)]} {
		    lappend result _tc_ $map(_tc_)
		}
		catch {unset map}
		catch {unset marshalrecursion($repoid)}
		return $result
	    }
	    ValueBox {
		if {$value == 0} {
		    return
		}
		return [duplicate [lindex $tc 2] $value]
	    }
	    recursive {
		set repoid [lindex $tc 1]
		set savetc $marshalrecursion($repoid)
 		set res [duplicate $marshalrecursion($repoid) $value]
		set marshalrecursion($repoid) $savetc
		return $res
	    }
	}
    }

    proc check_type_equivalence {value1 value2} {
	set tckind1 [lindex $value1 0]
	if {$value1 == "unsigned short" || \
		$value1 == "unsigned long" || \
		$value1 == "long long" || \
		$value1 == "unsigned long long" || \
		$value1 == "long double"} {
	    set tckind1 $value1
	}

	set tckind2 [lindex $value2 0]
	if {$value2 == "unsigned short" || \
		$value2 == "unsigned long" || \
		$value2 == "long long" || \
		$value2 == "unsigned long long" || \
		$value2 == "long double"} {
	    set tckind2 $value1
	}

	if {$tckind1 != $tckind2} {
	    return 0
	}

	switch -- $tckind1 {
	    struct -
	    union -
	    exception -
	    Object {
		set repoid1 [lindex $value1 1]
		set repoid2 [lindex $value2 1]
		if {$repoid1 != "" && $repoid2 != ""} {
		    if {$repoid1 == $repoid2} {
			return 1
		    } else {
			return 0
		    }
		}
	    }
	}

	switch -- $tckind1 {
	    string -
	    wstring {
		if {[lindex $value1 1] != [lindex $value2 1]} {
		    return 0
		}
	    }
	    fixed {
		if {[lindex $value1 1] != [lindex $value2 1] || \
			[lindex $value1 2] != [lindex $value2 2]} {
		    return 0
		}
	    }
	    recursive {
		if {[lindex $value1 1] != [lindex $value2 1]} {
		    return 0
		}
	    }
	    struct -
	    exception {
		set members1 [lindex $value1 2]
		set members2 [lindex $value2 2]
		if {[llength $members1] != [llength $members2]} {
		    return 0
		}
		for {set idx 1} {$idx < [llength $members1]} {incr idx 2} {
		    set member_type1 [lindex $members1 $idx]
		    set member_type2 [lindex $members2 $idx]
		    if {![check_type_equivalence $member_type1 $member_type2]} {
			return 0
		    }
		}
	    }
	    union {
		set disctype1 [lindex $value1 2]
		set disctype2 [lindex $value2 2]
		set members1 [lindex $value1 3]
		set members2 [lindex $value2 3]

		if {$disctype1 != $disctype2} {
		    return 0
		}
		if {[llength $members1] != [llength $members2]} {
		    return 0
		}
		for {set idx 0} {$idx < [llength $members1]} {incr idx} {
		    set member_label1 [lindex $members1 $idx]
		    set member_label2 [lindex $members2 $idx]
		    if {$member_label1 != $member_label2} {
			return 0
		    }
		    incr idx
		    set member_type1 [lindex $members1 $idx]
		    set member_type2 [lindex $members2 $idx]
		    if {![check_type_equivalence $member_type1 $member_type2]} {
			return 0
		    }
		}
	    }
	    enum {
		set members1 [lindex $value1 1]
		set members2 [lindex $value2 1]
		if {[llength $members1] != [llength $members2]} {
		    return 0
		}
		for {set idx 0} {$idx < [llength $members1]} {incr idx} {
		    if {[lindex $members1 $idx] != [lindex $members2 $idx]} {
			return 0
		    }
		}
	    }
	    sequence -
	    array {
		if {[llength $value1] == 3} {
		    set bound1 [lindex $value1 2]
		} else {
		    set bound1 0
		}
		if {[llength $value2] == 3} {
		    set bound2 [lindex $value2 2]
		} else {
		    set bound2 0
		}
		if {$bound1 != $bound2} {
		    return 0
		}
		if {![check_type_equivalence [lindex $value1 1] [lindex $value2 1]]} {
		    return 0
		}
	    }
	    default {
		error "oops: $tckind1"
	    }
	}

	return 1
    }

    #
    # corba::type
    #

    proc type {cmd args} {
	::corba::init
	switch -- $cmd {
	    of {
		if {[llength $args] != 1} {
		    error "usage: corba::type of <repoid-or-scoped-name>"
		}
		if {![catch {
		    set type [::Combat::SimpleTypeRepository::getTypeOf $args]
		}]} {
		    return $type
		}

		#
		# try to download type from IFR
		#

		set ifr [::corba::resolve_initial_references InterfaceRepository]
		set ifrobj [::Combat::CORBA::ORB::GetObjFromRef $ifr]

		if {[catch {
		    set contained [::Combat::CORBA::ORB::invoke_sync 1 \
			    $ifrobj lookup_id Object 0 {{in string}} $args]
		} res]} {
		    set contained 0
		}

		if {$contained == 0} {
		    if {[catch {
			set contained [::Combat::CORBA::ORB::invoke_sync 1 \
				$ifrobj lookup Object 0 {{in string}} $args]
		    } res]} {
			set contained 0
		    }
		}

		if {$contained == 0} {
		    ::corba::release $ifr
		    error "unable to find type $args in Interface Repository"
		}

		set cobj [::Combat::CORBA::ORB::GetObjFromRef $contained]
		
		if {[catch {
		    set type [::Combat::CORBA::ORB::invoke_sync 1 \
			    $cobj _get_type TypeCode 0 {} {}]
		}]} {
		    ::corba::release $contained
		    ::corba::release $ifr
		    error "not an IDL type: $args"
		}
		
		::corba::release $contained
		::corba::release $ifr
		return $type
	    }
	    match {
		if {[llength $args] != 2} {
		    error "usage: corba::type match <TypeCode> <value>"
		}
		set enc [::Combat::CDR::Encoder \#auto]
		if {[catch {
		    $enc Marshal [lindex $args 0] [lindex $args 1]
		} res]} {
		    itcl::delete object $enc
		    return 0
		}
		itcl::delete object $enc
		return 1
	    }
	    equivalent {
		if {[llength $args] != 2} {
		    error "usage: corba::type equivalent <TypeCode> <TypeCode>"
		}
		return [check_type_equivalence \
			[lindex $args 0] [lindex $args 1]]
	    }
	    default {
		error "usage: corba::type of, match or equivalent"
	    }
	}
    }

    #
    # corba::const
    #

    proc const {rors} {
	::corba::init
	if {![catch {
	    set value [::Combat::SimpleTypeRepository::getConst $rors]
	}]} {
	    return $value
	}

	#
	# try to download type from IFR
	#
	
	set ifr [::corba::resolve_initial_references InterfaceRepository]
	set ifrobj [::Combat::CORBA::ORB::GetObjFromRef $ifr]

	if {[catch {
	    set contained [::Combat::CORBA::ORB::invoke_sync 1 \
		    $ifrobj lookup_id Object 0 {{in string}} [list $rors]]
	} res]} {
	    set contained 0
	}

	if {$contained == 0} {
	    if {[catch {
		set contained [::Combat::CORBA::ORB::invoke_sync 1 $ifrobj \
			    lookup Object 0 {{in string}} [list $rors]]
	    } res]} {
		set contained 0
	    }
	}

	if {$contained == 0} {
	    ::corba::release $ifr
	    error "unable to find constant $rors in Interface Repository"
	}

	set cobj [::Combat::CORBA::ORB::GetObjFromRef $contained]

	if {[catch {
	    set value [::Combat::CORBA::ORB::invoke_sync 1 \
		    $cobj _get_value any 0 {} {}]
	}]} {
	    ::corba::release $contained
	    ::corba::release $ifr
	    error "not a constant: $rors"
	}
		
	::corba::release $contained
	::corba::release $ifr
	return $value
    }

    variable features
    array set features {
	core 0.8
	async 0.8
	callback 0.8
	type 0.8
	poa 0.8
	dii 0.8
	combat::ir 0.8
    }

    #
    # corba::feature
    #

    proc feature {cmd args} {
	variable features

	switch -- $cmd {
	    names {
		return [list core async callback type combat::ir]
	    }
	    require {
		if {[lindex $args 0] == "-exact"} {
		    set exact 1
		    set args [lrange $args 1 end]
		} else {
		    set exact 0
		}
		if {[llength $args] == 1} {
		    set feature [lindex $args 0]
		    set version "0.0"
		} elseif {[llength $args] == 2} {
		    set feature [lindex $args 0]
		    set version [lindex $args 1]
		} else {
		    error "usage: corba::feature require ?-exact? feature ?version?"
		}

		if {![info exists features($feature)]} {
		    error "corba::feature not available: $feature"
		}

		if {[llength $args] == 1} {
		    return 1
		}

		if {$exact} {
		    if {$version != $features($feature)} {
			error "corba::feature version mismatch: $version requested, $features($feature) available"
		    }
		    return 1
		}

		set comp [::package vcompare $version $features($feature)]

		if {$comp == 1} {
		    error "corba::feature version mismatch: $version requested, $features($feature) available"
		} elseif {$comp == -1 && [string index $features($feature) 0] == "0"} {
		    error "corba::feature version mismatch: $version requested, $features($feature) available"
		}

		return 1
	    }
	    default {
		error "usage: corba::feature names or require"
	    }
	}
    }
}
