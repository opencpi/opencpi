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
# CVS Version Tag: $Id: iiop.tcl,v 1.13 2008/11/08 18:24:50 Owner Exp $
#
# ----------------------------------------------------------------------
# IIOP Module
# ----------------------------------------------------------------------
#

namespace eval Combat {
    namespace eval IIOP {
	#
	# active_iiop_conns is an array host:port -> conn
	#

	variable active_iiop_conns

	#
	# Chunk size. Data is read and written in chunks of this size
	#

	variable chunk_size 10240

	#
	# Trimming threshold. Buffers grow up to this size before being
	# trimmed. This is to avoid too much copying of the data.
	#

	variable trimming_threshold 65536

	# --------------------------------------------------------------
	# IIOP connection
	# --------------------------------------------------------------

	itcl::class Conn {
	    inherit ::Combat::GIOP::Conn

	    private variable channel
	    private variable send_pending
	    private variable recv_pending
	    private variable send_index
	    private variable recv_index
	    private variable send_broken
	    private variable recv_broken
	    
	    constructor {ch hp major minor client} {
		::Combat::GIOP::Conn::constructor $major $minor $client
	    } {
		set channel $ch
		set send_pending ""
		set recv_pending ""
		set send_index 0
		set recv_index 0
		set send_broken 0
		set recv_broken 0
		set connected 0
		set peer $hp
		fconfigure $channel -translation binary
		fconfigure $channel -blocking false
		fileevent $channel readable "$this readable_callback"
	    }

	    destructor {
		close
	    }

	    public method close {} {
		if {$channel != ""} {
		    if {$::Combat::debug(iiop)} {
			puts stderr "IIOP: Closing connection to $peer"
		    }
		    unset ::Combat::IIOP::active_iiop_conns($peer)
		    set send_broken 1
		    set recv_broken 1
		    ::close $channel
		    set channel ""
		}
	    }

	    public method writable_callback {} {
		if {[string length $send_pending] - $send_index > \
			$::Combat::IIOP::chunk_size} {
		    set eod [expr {$send_index+$::Combat::IIOP::chunk_size-1}]
		    set data [string range $send_pending $send_index $eod]
		    set send_index $eod
		    incr send_index
		    if {$send_index > $::Combat::IIOP::trimming_threshold} {
			set send_pending [string range $send_pending $send_index end]
			set send_index 0
		    }
		} else {
		    set data [string range $send_pending $send_index end]
		    set send_pending ""
		    set send_index 0
		}

		if {[catch {
		    puts -nonewline $channel $data
		    flush $channel
		}]} {
		    if {$::Combat::debug(iiop)} {
			if {!$connected} {
			    puts stderr "IIOP: error connecting to $peer"
			} else {
			    puts stderr "IIOP: send to $peer failed, broken pipe"
			}
		    }
		    fileevent $channel writable ""
		    set send_broken 1
		    CloseConnection
		    return
		}

		set connected 1

		if {[string length $send_pending] == 0} {
		    fileevent $channel writable ""
		}
	    }

	    public method readable_callback {} {
		if {[catch {set data [read $channel $::Combat::IIOP::chunk_size]}]} {
		    if {$::Combat::debug(iiop)} {
			puts stderr "IIOP: read from $peer failed, broken pipe"
		    }
		    fileevent $channel readable ""
		    set recv_broken 1
		} elseif {[string length $data] == 0} {
		    if {$::Combat::debug(iiop)} {
			puts stderr "IIOP: read from $peer returns end of file"
		    }
		    fileevent $channel readable ""
		    set recv_broken 1
		} else {
		    append recv_pending $data
		    set connected 1
		}

		callback
	    }

	    public method send {data} {
		if {$send_broken} {
		    error "error: broken pipe"
		}
		if {[string length $send_pending] == 0} {
		    set send_pending $data
		    fileevent $channel writable "$this writable_callback"
		} else {
		    append send_pending $data
		}
	    }

	    public method recv {length} {
		if {$recv_broken && $length > [string length $recv_pending] - $recv_index} {
		    error "error: read beyond end of file"
		}

		if {$length >= [string length $recv_pending] - $recv_index} {
		    set data [string range $recv_pending $recv_index end]
		    set recv_pending ""
		    set recv_index 0
		} else {
		    set eod [expr {$recv_index + $length - 1}]
		    set data [string range $recv_pending $recv_index $eod]
		    set recv_index $eod
		    incr recv_index
		    if {$recv_index > $::Combat::IIOP::trimming_threshold} {
			set recv_pending [string range $recv_pending $recv_index end]
			set recv_index 0
		    }
		}

		return $data
	    }

	    public method size {} {
		if {$recv_broken} {
		    return -1
		}
		return [expr {[string length $recv_pending] - $recv_index}]
	    }
	}

	itcl::class TCPServer {
	    private variable port
	    private variable sock
	    private variable host

	    constructor {} {
		set port -1
	    }

	    destructor {
		if {$port != -1} {
		    close $sock
		}
	    }

	    public method listen {{myport 0}} {
		if {[catch {
		    set sock [socket -server Combat::IIOP::TCPServer::accept $myport]
		} err]} {
		    if {$::Combat::debug(iiop)} {
			puts stderr "IIOP: cannot listen on port $myport: $err"
		    }
		    error $err
		}

		set name [fconfigure $sock -sockname]
		set host $::Combat::CORBA::ORB::hostname ;# [lindex $name 1]
		set port [lindex $name 2]

		if {$::Combat::debug(iiop)} {
		    puts stderr "IIOP: Listening on port $port"
		}
	    }

	    public proc accept {channel host port} {
		if {$::Combat::debug(iiop)} {
		    puts stderr "IIOP: incoming connection from $host:$port"
		}

		set conn [namespace current]::[::Combat::IIOP::Conn \#auto \
			$channel $host:$port 1 2 0]
		set ::Combat::IIOP::active_iiop_conns($host:$port) $conn
	    }

	    public method profile {} {
		set prof [namespace current]::[::Combat::IIOP::ProfileBody \#auto]
		$prof configure -major_version 1 -minor_version 2
		$prof configure -host $host -port $port
		return $prof
	    }
	}


	# --------------------------------------------------------------
	# IIOP Profile
	# --------------------------------------------------------------

	itcl::class ProfileBody {
	    inherit ::Combat::IOP::TaggedProfile

	    public variable major_version
	    public variable minor_version
	    public variable host
	    public variable port
	    public variable object_key
	    public variable components

	    constructor {} {
		set tag 0
		set major_version 1
		set minor_version 2
		set object_key ""
		set components [list]
	    }

	    destructor {
		foreach component $components {
		    itcl::delete object $component
		}
	    }

	    public method marshal {buffer} {
		$buffer ulong 0		;# TAG_INTERNET_IOP
		$buffer begin_encaps

		$buffer octet [binary format c1 $major_version]
		$buffer octet [binary format c1 $minor_version]
		$buffer string $host
		$buffer ushort $port
		$buffer ulong [string length $object_key]
		$buffer octets $object_key

		if {$minor_version >= 1} {
		    $buffer ulong [llength $components]
		    foreach component $components {
			$component marshal $buffer
		    }
		}
		$buffer end_encaps
	    }

	    public method connect {} {
		if {[info exists ::Combat::IIOP::active_iiop_conns($host:$port)]} {
		    set conn $::Combat::IIOP::active_iiop_conns($host:$port)
		    if {[$conn cget -broken] == 0} {
			$conn ref
			return [list $conn $object_key]
		    }
		}

		set major $major_version
		set minor $minor_version

		if {$major != 1} {
		    error "unknown IIOP version $major.$minor"
		}

		if {$minor >= 3} {
		    set minor 2
		}

		if {$::Combat::debug(iiop)} {
		    puts stderr "IIOP: Opening new IIOP $major.$minor connection to $host:$port"
		}

		if {[catch {set channel [socket $host $port]} err]} {
		    if {$::Combat::debug(iiop)} {
			puts stderr "IIOP: couldn't connect to $host:$port: $err"
		    }
		    error $err
		}

		set conn [namespace current]::[::Combat::IIOP::Conn \#auto \
			$channel $host:$port $major $minor 1]
		set ::Combat::IIOP::active_iiop_conns($host:$port) $conn

		#
		# Does this profile have some CodeSetInfo?
		#

		foreach component $components {
		    if {[$component cget -tag] == 1} {
			return [list $conn $object_key $component]
		    }
		}

		return [list $conn $object_key ""]
	    }
	}

	proc DemarshalIIOPProfile {buffer} {
	    $buffer begin_encaps

	    set major [$buffer octet]
	    set minor [$buffer octet]

	    binary scan $major c1 major_version
	    binary scan $minor c1 minor_version
	    
	    set host  [$buffer string]
	    set port  [$buffer ushort]
	    set klen  [$buffer ulong]
	    set object_key [$buffer octets $klen]
	    
	    set components [list]
	    if {$minor_version >= 1} {
		set compcount [$buffer ulong]
		for {set i 0} {$i < $compcount} {incr i} {
		    lappend components [::Combat::IOP::DemarshalTaggedComponent $buffer]
		}
	    }

	    $buffer end_encaps

	    set res [namespace current]::[::Combat::IIOP::ProfileBody \#auto]
	    $res configure -tag 0 -major_version $major_version \
		    -minor_version $minor_version -host $host -port $port \
		    -object_key $object_key -components $components

	    return $res
	}
    }
}

