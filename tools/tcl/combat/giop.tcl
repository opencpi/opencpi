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
# CVS Version Tag: $Id: giop.tcl,v 1.26 2008/11/08 18:24:50 Owner Exp $
#
# ----------------------------------------------------------------------
# Manage GIOP Connections
# ----------------------------------------------------------------------
#

namespace eval Combat {
    namespace eval GIOP {
	#
	# The maximum GIOP message size in bytes. We refuse messages
	# that are larger than this, avoiding denial of service issues
	# when a client sends huge messages that would consume all
	# local memory. No limit if the value is 0.
	#

	variable ORBGIOPMaxSize 0

	#
	# Idle connection timeout. After this time (in milliseconds),
	# the connection is closed if no activity occurs. If 0, the
	# timeout is infinite.
	#

	variable idleTimeout 3600000

	#
	# Whether to send CancelRequest messages to a server when a
	# request is canceled or times out. The default should be 1,
	# but some ORBs don't like them and close the connection
	# when seeing one.
	#

	variable sendCancelRequest 1

	itcl::class Conn {
	    protected variable timeout_token
	    protected variable connected
	    protected variable peer
	    protected variable active_requests
	    private variable refs
	    private variable major_version
	    private variable minor_version
	    private variable isclient
	    private variable id
	    public variable broken

	    #
	    # global conn id
	    #

	    private common connids 0
	    public variable connid

	    #
	    # en/decoder for char and wchar codeset
	    #

	    private variable cndone	;# whether Codeset Negotiation
	    private variable ccodec	;# has been done yet
	    private variable wcodec

	    #
	    # info for the messages read, indexed by id
	    #

	    private variable callbacks
	    private variable reply_status
	    private variable reply_contexts
	    private variable reply_decoder

	    #
	    # info for the data we're currently reading
	    #

	    private variable pending_minor
	    private variable pending_major
	    private variable pending_flags
	    private variable pending_byteorder
	    private variable pending_fragment
	    private variable pending_type
	    private variable pending_size
	    private variable pending_header
	    private variable pending_message

	    #
	    # received fragments
	    #

	    private variable pending_frag_size
	    private variable pending_fragments

	    #
	    # Methods
	    #

	    constructor {major minor client} {
		set connected 0
		set peer ""
		set active_requests 0
		set major_version $major
		set minor_version $minor
		set isclient $client
		set pending_header ""
		set broken 0
		set refs 1
		set id 0
		set cndone 0
		set ccodec ""
		set wcodec ""
		set connid [incr connids]
		set pending_frag_size 0

		if {$::Combat::GIOP::idleTimeout} {
		    set timeout_token [after $::Combat::GIOP::idleTimeout "$this idle_timeout_callback"]
		} else {
		    set timeout_token ""
		}
	    }

	    destructor {
		if {$ccodec != ""} {
		    itcl::delete object $ccodec
		}
		if {$wcodec != ""} {
		    itcl::delete object $wcodec
		}
		if {$timeout_token != ""} {
		    after cancel $timeout_token
		}
	    }

	    public method close {} {
		error "pure virtual"
	    }

	    public method send {data} {
		error "pure virtual"
	    }

	    public method recv {length} {
		error "pure virtual"
	    }

	    public method size {} {
		error "pure virtual"
	    }

	    public method listen {cb} {
		error "pure virtual"
	    }

	    public method ref {} {
		incr refs
	    }

	    public method deref {} {
		incr refs -1
		if {$refs == 0} {
		    itcl::delete object $this
		}
	    }

	    #
	    # Send a GIOP request
	    #

	    public method send_request {response_expected object \
		    operation plevel params values {contexts {}} \
		    {callback {}}} {
		#
		# Bump up id, get buffer
		#

		set theid [incr id]
		set encoder [::Combat::CDR::Encoder \#auto]
		set buffer [$encoder get_buffer]

		#
		# Build message header, with a dummy message length at
		# first. We must do this here to get our alignment right.
		#

		set header [binary format a4ccccI \
			GIOP \
			$major_version $minor_version \
			[$buffer cget -byteorder] 0 0]
		$buffer octets $header

		#
		# Do codeset negotiation upon first request
		#

		if {!$cndone} {
		    set csi [$object cget -CodeSetInfo]
		    if {$major_version == 1 && $minor_version == 0} {
			#
			# no codeset negotiation for GIOP 1.0
			#
		    } elseif {$csi == ""} {
			#
			# backwards compatibility using ISO 8859-1
			#
		    } else {
			set tcsc [::Combat::CONV_FRAME::findTransCodeSet char \
				[$csi cget -char_native_code_set] \
				[$csi cget -char_conversion_code_sets]]
			set tcsw [::Combat::CONV_FRAME::findTransCodeSet wchar\
				[$csi cget -wchar_native_code_set] \
				[$csi cget -wchar_conversion_code_sets]]

			if {$::Combat::debug(giop)} {
			    puts stderr "GIOP: Codeset Negotiation with $peer:"
			    if {$tcsc == 0} {
				puts stderr "GIOP:    TCS-C: $tcsc (native)"
			    } else {
				puts stderr "GIOP:    TCS-C: $tcsc ([lindex $::Combat::CONV_FRAME::codesets($tcsc) 1])"
			    }
			    if {$tcsw == 0} {
				puts stderr "GIOP:    TCS-W: $tcsw (native)"
			    } else {
				puts stderr "GIOP:    TCS-W: $tcsw ([lindex $::Combat::CONV_FRAME::codesets($tcsw) 1])"
			    }
			}

			set ccodec [::Combat::CONV_FRAME::getConverter \
				$major_version $minor_version $tcsc]
			set wcodec [::Combat::CONV_FRAME::getConverter \
				$major_version $minor_version $tcsw]

			#
			# attach CodeSetContext
			#

			set csc [namespace current]::[::Combat::CONV_FRAME::CodeSetContext \#auto]
			$csc configure -char_data $tcsc -wchar_data $tcsw
			lappend contexts $csc
		    }
		    set cndone 1
		}

		#
		# Build request header
		#

		if {$major_version == 1 && \
			($minor_version == 0 || $minor_version == 1)} {
		    $buffer ulong [llength $contexts]   ;# service contexts
		    foreach context $contexts {
			$context marshal $buffer
		    }
		    $buffer ulong $theid                ;# request_id
		    $buffer boolean $response_expected
		    $buffer octets "\0\0\0"		;# reserved
		    set object_key [$object cget -object_key]
		    $buffer ulong [string length $object_key]
		    $buffer octets $object_key
		    $buffer string $operation
		    $buffer ulong 0                     ;# principal
		} elseif {$major_version == 1 && $minor_version == 2} {
		    $buffer ulong $theid                ;# request_id
		    if {$response_expected} {
			$buffer octet "\3"
		    } else {
			$buffer octet "\0"
		    }
		    $buffer octets "\0\0\0"             ;# reserved
		    switch [$object cget -AddressingDisposition] {
			0 { # KeyAddr
			    $buffer short 0
			    set object_key [$object cget -object_key]
			    $buffer ulong [string length $object_key]
			    $buffer octets $object_key
			}
			1 { # ProfileAddr
			    $buffer short 1
			    set ior [$object get_fwd_ior]
			    set profile [lindex \
				    [$ior cget -profiles] \
				    [$object cget -profile_index]]
			    $profile marshal $buffer
			}
			2 { # ReferenceAddr
			    $buffer short 2
			    $buffer ulong [$object cget -profile_index]
			    set ior [$object get_fwd_ior]
			    $ior marshal $buffer
			}
			default {
			    error "illegal addressing disposition \
				    [$object cget -AddressingDisposition]"
			}
		    }
		    $buffer string $operation
		    $buffer ulong [llength $contexts]   ;# service contexts
		    foreach context $contexts {
			$context marshal $buffer
		    }
		} else {
		    error "unknown GIOP version $major.$minor"
		}
	    
		#
		# GIOP 1.2 request body needs extra alignment
		#

		if {$major_version == 1 && $minor_version == 2} {
		    $buffer align 8
		}

		#
		# Switch buffer from ISO-8859-1 to the negotiated charset
		#

		$buffer configure -cencoder $ccodec -wencoder $wcodec
	    
		#
		# Request body
		#

		incr plevel

		if {[catch {
		    for {set idx 0} {$idx < [llength $params]} {incr idx} {
			set param [lindex $params $idx]
			set value [lindex $values $idx]

			if {[llength $param] == 8} {
			    #
			    # It's a ParameterDescription
			    #
			    set dir [lindex $param 7]
			    set type [lindex $param 3]
			} else {
			    #
			    # It's a simple list with the direction and type
			    #
			    set dir  [lindex $param 0]
			    set type [lindex $param 1]
			}
			
			switch -- $dir {
			    PARAM_IN -
			    in {
				$encoder Marshal $type $value
			    }
			    PARAM_INOUT -
			    inout {
				upvar $plevel $value pdata
				$encoder Marshal $type $pdata
			    }
			    PARAM_OUT -
			    out {
			    }
			    default {
				error "unknown parameter direction: $dir"
			    }
			}
		    }
		} err]} {
		    if {$::Combat::debug(giop)} {
			global errorInfo
			puts stderr "GIOP: marshalling error for $operation: $err"
			puts $errorInfo
		    }
		    itcl::delete object $encoder
		    ::corba::throw [list IDL:omg.org/CORBA/MARSHAL:1.0 \
			    [list minor 0 completion_status COMPLETED_NO]]
		}

		#
		# Get data and fiddle in the data length; we must substract
		# the length of the GIOP header (12 bytes).
		#

		set data [$buffer get_data]
		set data_length [expr {[string length $data] - 12}]

		if {[$buffer cget -byteorder] == 0} {
		    set data [string replace $data 8 11 \
			    [binary format I $data_length]]
		} else {
		    set data [string replace $data 8 11 \
			    [binary format i $data_length]]
		}

		#
		# Send request
		#

		if {$::Combat::debug(giop)} {
		    puts stderr "GIOP: sending request to $peer, id is $theid"
		}

		if {$::Combat::debug(transport)} {
		    ::Combat::DumpOctets Request $data
		}

		if {$response_expected} {
		    incr active_requests
		    set reply_status($theid) -1
		}

		if {$timeout_token != ""} {
		    after cancel $timeout_token
		    set timeout_token ""
		}

		if {!$active_requests && $::Combat::GIOP::idleTimeout} {
		    set timeout_token [after $::Combat::GIOP::idleTimeout "$this idle_timeout_callback"]
		}

		catch {
		    send $data
		}

		#
		# Delete encoder and CodeSetContext
		#

		itcl::delete object $encoder
		if {[info exists csc]} {
		    itcl::delete object $csc
		    set cndone 1
		}

		#
		# arrange to call callback
		#

		if {$callback != "" && $response_expected} {
		    if {$broken == 1} {
			after 0 "$callback 2"
		    } elseif {$broken == -1} {
			after 0 "$callback 3"
		    } else {
			set callbacks($id) $callback
		    }
		}

		#
		# done return
		#

		return $theid
	    }

	    #
	    # Decipher a reply
	    #

	    public method receive_reply {theid rtype plevel \
		    params values exceptions {contexts {}}} {
		#
		# Wait until reply is there or until conn is broken
		#

		if {[info exists reply_status($theid)] && \
			   $reply_status($theid) == -1 && !$broken} {
		    vwait [itcl::scope reply_status($theid)]
		}

		if {![info exists reply_status($theid)]} {
		    #
		    # Most likely, we were canceled.
		    #

		    error "no such request: $theid"
		}

		#
		# One less request is active
		#

		incr active_requests -1
	
		if {!$active_requests && $::Combat::GIOP::idleTimeout} {
		    set timeout_token [after $::Combat::GIOP::idleTimeout "$this idle_timeout_callback"]
		}

		#
		# Handle broken connection
		#

		if {$reply_status($theid) == -1} {
		    unset reply_status($theid)

		    if {$broken == 1} {
			#
			# CloseConnection received. Outstanding messages
			# were not processed and may be safely resent on
			# a new connection
			#
			return 3	;# LOCATION_FORWARD
		    } else {
			#
			# Connection closure without receiving CloseConn.
			# Assume an abortive disconnect, treat as error.
			# Report COMM_FAILURE for all pending requests
			# on the connection, with completion_status values
			# set to COMPLETED_MAYBE
			#
			return [list 2 [list \
				IDL:omg.org/CORBA/COMM_FAILURE:1.0 \
				[list minor_code_value 0 \
				completion_status COMPLETED_MAYBE]]]
		    }
		}

		#
		# Got a reply
		#

		set decoder $reply_decoder($theid)
		set thestatus $reply_status($theid)

		incr plevel

		if {$contexts != ""} {
		    upvar $plevel $contexts cvar
		    set cvar $reply_contexts($theid)
		} else {
		    foreach context $reply_contexts($theid) {
			itcl::delete object $context
		    }
		}

		unset reply_decoder($theid)
		unset reply_status($theid)
		unset reply_contexts($theid)

		set return_value ""

		if {[catch {
		    switch -- $thestatus {
			0 { # NO_EXCEPTION
			    set res [$decoder Demarshal $rtype]
			    
			    for {set idx 0} {$idx < [llength $params]} {incr idx} {
				set param [lindex $params $idx]
				set value [lindex $values $idx]
			    
				if {[llength $param] == 8} {
				    #
				    # It's a ParameterDescription
				    #
				    set dir [lindex $param 7]
				    set type [lindex $param 3]
				} else {
				    #
				    # It's a simple list with the direction and type
				    #
				    set dir  [lindex $param 0]
				    set type [lindex $param 1]
				}
				
				switch -- $dir {
				    PARAM_IN -
				    in {
				    }
				    PARAM_INOUT -
				    PARAM_OUT -
				    inout -
				    out {
					upvar $plevel $value pdata
					set pdata [$decoder Demarshal $type]
				    }
				}
			    }
			    
			    set return_value [list 0 $res]
			}
			1 { # USER_EXCEPTION
			    #
			    # Peek for repoid
			    #
			    set buffer [$decoder get_buffer]
			    set pos [$buffer tell]
			    set repoid [$buffer string]
			    $buffer seek $pos
			    set data ""
			    foreach exception $exceptions {
				if {[llength $exception] == 10} {
				    #
				    # It's an ExceptionDescription
				    #
				    set exrid [lindex $exception 3]
				    set extype [lindex $exception 9]
				} else {
				    #
				    # It's an exception typecode
				    #
				    set exrid [lindex $exception 1]
				    set extype $exception
				}

				if {$repoid == $exrid} {
				    set data [$decoder Demarshal $extype]
				    break
				}
			    }
			    if {$data == ""} {
				set data "IDL:omg.org/CORBA/UnknownUserException:1.0"
			    }
			    set return_value [list 1 $data]
			}
			2 { # SYSTEM_EXCEPTION
			    set buffer [$decoder get_buffer]
			    set repoid [$buffer string]
			    set minor_code_value [$buffer ulong]
			    switch [$buffer ulong] {
				0 {
				    set completion_status COMPLETED_YES
				}
				1 {
				    set completion_status COMPLETED_NO
				}
				2 {
				    set completion_status COMPLETED_MAYBE
				}
				default {
				    set completion_status oops
				}
			    }
			    set return_value [list 2 [list $repoid \
				    [list minor_code_value $minor_code_value \
				    completion_status $completion_status]]]
			}
			3 -
			4 { # LOCATION_FORWARD, LOCATION_FORWARD_PERM
			    set buffer [$decoder get_buffer]
			    set ior [::Combat::IOP::DemarshalIOR $buffer]
			    set return_value [list $thestatus $ior]
			}
			default {
			    error "unknown GIOP reply status $thestatus"
			}
		    }
		} err]} {
		    if {$::Combat::debug(giop)} {
			global errorInfo
			puts stderr "GIOP: error demarshalling reply: $err"
			puts $errorInfo
		    }
		    
		    set return_value [list 2 \
			    [list IDL:omg.org/CORBA/MARSHAL:1.0 \
			    [list minor 0 completion_status COMPLETED_YES]]]
		}

		itcl::delete object $decoder
		return $return_value
	    }

	    #
	    # Check for a request's completion
	    #

	    public method poll_reply {theid} {
		if {$broken == 1} {
		    return 3
		} elseif {$broken == -1} {
		    return 2
		}

		if {![info exists reply_status($theid)]} {
		    error "no such request: $theid"
		}

		#
		# May be -1.
		#

		return $reply_status($theid)
	    }

	    #
	    # Cancel a request
	    #

	    public method cancel_request {theid} {
		if {![info exists reply_status($theid)]} {
		    error "no such request: $theid"
		}

		if {$::Combat::debug(giop)} {
		    puts stderr "GIOP: canceling request $theid with $peer"
		}

		unset reply_status($theid)

		if {[info exists reply_decoder($theid)]} {
		    itcl::delete object $reply_decoder($theid)
		    unset reply_decoder($theid)
		}

		if {[info exists reply_contexts($theid)]} {
		    foreach context $reply_contexts($theid) {
			itcl::delete object $context
		    }
		    unset reply_contexts($theid)
		}

		if {[info exists callbacks($theid)]} {
		    unset callbacks($theid)
		}

		if {$::Combat::GIOP::sendCancelRequest} {
		    #
		    # Send CancelRequest message
		    #

		    if {$::tcl_platform(byteOrder) == "bigEndian"} {
			set data [binary format a4ccccII \
				      GIOP \
				      $major_version $minor_version \
				      0 2 4 $theid]
		    } else {
			set data [binary format a4ccccii \
				      GIOP \
				      $major_version $minor_version \
				      1 2 4 $theid]
		    }

		    if {$::Combat::debug(giop)} {
			puts stderr "GIOP: sending CancelRequest message to $peer for id $theid"
		    }

		    if {$::Combat::debug(transport)} {
			::Combat::DumpOctets Cancel $data
		    }

		    catch {
			send $data
		    }
	        }

		incr active_requests -1

		if {!$active_requests && $::Combat::GIOP::idleTimeout} {
		    set timeout_token [after $::Combat::GIOP::idleTimeout "$this idle_timeout_callback"]
		}
	    }

	    #
	    # Send a reply after finishing a request
	    #

	    public method send_reply {theid status rtype rvalue plevel \
		    params values {contexts {}}} {
		set encoder [::Combat::CDR::Encoder \#auto]
		set buffer [$encoder get_buffer]

		#
		# Build message header, with a dummy message length at
		# first. We must do this here to get our alignment right.
		#

		set header [binary format a4ccccI \
			GIOP \
			$major_version $minor_version \
			[$buffer cget -byteorder] 1 0]
		$buffer octets $header

		#
		# Build reply header
		#

		if {$major_version == 1 && \
			($minor_version == 0 || $minor_version == 1)} {
		    $buffer ulong [llength $contexts]
		    foreach context $contexts {
			$context marshal $buffer
		    }
		    $buffer ulong $theid
		    $buffer ulong $status
		} else {
		    $buffer ulong $theid
		    $buffer ulong $status
		    $buffer ulong [llength $contexts]
		    foreach context $contexts {
			$context marshal $buffer
		    }
		}

		#
		# GIOP 1.2 request body needs extra alignment
		#

		if {$major_version == 1 && $minor_version == 2} {
		    $buffer align 8
		}

		#
		# Switch buffer from ISO-8859-1 to the negotiated charset
		#

		$buffer configure -cencoder $ccodec -wencoder $wcodec
	    
		#
		# Reply body
		#

		incr plevel

		if {[catch {
		    switch -- $status {
			0 { # NO_EXCEPTION
			    $encoder Marshal $rtype $rvalue
			    
			    for {set idx 0} {$idx < [llength $params]} {incr idx} {
				set param [lindex $params $idx]
				set value [lindex $values $idx]
				
				if {[llength $param] == 8} {
				    #
				    # It's a ParameterDescription
				    #
				    set dir [lindex $param 7]
				    set type [lindex $param 3]
				} else {
				    #
				    # It's a simple list with the direction and type
				    #
				    set dir  [lindex $param 0]
				    set type [lindex $param 1]
				}

				switch -- $dir {
				    PARAM_IN -
				    in {
				    }
				    PARAM_INOUT -
				    inout {
					upvar $plevel $value pdata
					$encoder Marshal $type $pdata
				    }
				    PARAM_OUT -
				    out {
					upvar $plevel $value pdata
					$encoder Marshal $type $pdata
				    }
				    default {
					error "unknown parameter direction: $dir"
				    }
				}
			    }
			}
			1 { # USER_EXCEPTION
			    #
			    # User exception's type and value must be given
			    # instead of a reply
			    #
			    $encoder Marshal $rtype $rvalue
			}
			2 { # SYSTEM_EXCEPTION
			    #
			    # Instead of a reply, we expect value to be the
			    # exception, [list repo-id [list "minor" value
			    # "completion_status" value]]
			    #
			    
			    $buffer string [lindex $rvalue 0]
			    $buffer ulong [lindex [lindex $rvalue 1] 1]
			    
			    switch -- [lindex [lindex $rvalue 1] 3] {
				COMPLETED_YES {
				    $buffer ulong 0
				}
				COMPLETED_NO {
				    $buffer ulong 1
				}
				COMPLETED_MAYBE {
				    $buffer ulong 2
				}
				default {
				    # oops
				    $buffer ulong 2
				}
			    }
			}
			3 -
			4 -
			5 { # LOCATION_FORWARD
			    # NEEDS_ADDRESSING_MODE
			    # LOCATION_FORWARD_PERM
			    error "reply status $status not supported"
			}
		    }
		} err]} {
		    if {$::Combat::debug(giop)} {
			global errorInfo
			puts stderr "GIOP: error marshalling reply: $err"
			puts $errorInfo
		    }
		    #
		    # restart, send INTERNAL error
		    #
		    if {$status == 0} {
			set completion_status COMPLETED_YES
		    } else {
			set completion_status COMPLETED_MAYBE
		    }
		    itcl::delete object $encoder
		    send_reply $theid 2 dummy \
			    [list IDL:omg.org/CORBA/INTERNAL:1.0 \
			    [list minor 0 completion_status $completion_status]] \
			    0 "" ""
		    return
		}

		#
		# Get data and fiddle in the data length; we must substract
		# the length of the GIOP header (12 bytes).
		#
		
		set data [$buffer get_data]
		set data_length [expr {[string length $data] - 12}]

		if {[$buffer cget -byteorder] == 0} {
		    set data [string replace $data 8 11 \
			    [binary format I $data_length]]
		} else {
		    set data [string replace $data 8 11 \
			    [binary format i $data_length]]
		}

		#
		# Send reply
		#

		if {$::Combat::debug(giop)} {
		    puts stderr "GIOP: sending reply for id $theid to $peer, status is $status"
		}

		if {$::Combat::debug(transport)} {
		    ::Combat::DumpOctets Reply $data
		}

		incr active_requests -1

		if {!$active_requests && $::Combat::GIOP::idleTimeout} {
		    set timeout_token [after $::Combat::GIOP::idleTimeout "$this idle_timeout_callback"]
		}

		catch {
		    send $data
		}

		#
		# Delete encoder and CodeSetContext
		#

		itcl::delete object $encoder
	    }

	    #
	    # Send a locate reply
	    #

	    public method send_locate_reply {theid status body} {
		set encoder [::Combat::CDR::Encoder \#auto]
		set buffer [$encoder get_buffer]

		#
		# Build message header, with a dummy message length at
		# first. We must do this here to get our alignment right.
		#

		set header [binary format a4ccccI \
			GIOP \
			$major_version $minor_version \
			[$buffer cget -byteorder] 4 0]
		$buffer octets $header

		#
		# Build locate reply header
		#

		$buffer ulong $theid
		$buffer ulong $status

		#
		# GIOP 1.2 locate reply body does *not* need extra alignment
		# (issue 4314).
		#
		
		switch -- $status {
                    0 -
                    1 { # OBJCT_UNKNOWN
                        # OBJECT_HERE
                        # OK empty body
                    }
		    4 { # LOC_SYSTEM_EXCEPTION

			$buffer string [lindex $body 0]
			$buffer ulong [lindex [lindex $body 1] 1]

			switch -- [lindex [lindex $body 1] 3] {
			    COMPLETED_YES {
				$buffer ulong 0
			    }
			    COMPLETED_NO {
				$buffer ulong 1
			    }
			    COMPLETED_MAYBE {
				$buffer ulong 2
			    }
			    default {
				# oops
				$buffer ulong 2
			    }
			}
		    }
		    default {
			error "oops"
		    }
		}

		#
		# Get data and fiddle in the data length; we must substract
		# the length of the GIOP header (12 bytes).
		#
		
		set data [$buffer get_data]
		set data_length [expr {[string length $data] - 12}]

		if {[$buffer cget -byteorder] == 0} {
		    set data [string replace $data 8 11 \
			    [binary format I $data_length]]
		} else {
		    set data [string replace $data 8 11 \
			    [binary format i $data_length]]
		}

		#
		# Send reply
		#

		if {$::Combat::debug(giop)} {
		    puts stderr "GIOP: sending locate reply for id $theid to $peer, status is $status"
		}

		if {$::Combat::debug(transport)} {
		    ::Combat::DumpOctets LocReply $data
		}

		incr active_requests -1

		if {!$active_requests && $::Combat::GIOP::idleTimeout} {
		    set timeout_token [after $::Combat::GIOP::idleTimeout "$this idle_timeout_callback"]
		}

		catch {
		    send $data
		}

		#
		# Delete encoder and CodeSetContext
		#

		itcl::delete object $encoder
	    }

	    #
	    # Find out the request id for which this is a reply, then file it
	    #
	    
	    private method HandleIncomingReply {message} {
		binary scan $message @4ccc major minor flags
		set byteorder [expr {$flags & 1} ? 1 : 0]
		set decoder [::Combat::CDR::Decoder \#auto $message]
		set buffer [$decoder get_buffer]
		$buffer configure -byteorder $byteorder

		#
		# skip over 12 byte GIOP header
		#

		$buffer seek 12

		#
		# Header depends on GIOP version
		#

		if {$major == 1 && ($minor == 0 || $minor == 1)} {
		    set sccount [$buffer ulong]
		    set contexts [list]
		    for {set i 0} {$i < $sccount} {incr i} {
			lappend contexts [::Combat::IOP::DemarshalServiceContext $buffer]
		    }
		    set theid [$buffer ulong]
		    set status [$buffer ulong]
		} elseif {$major == 1 && $minor == 2} {
		    set theid [$buffer ulong]
		    set status [$buffer ulong]
		    set sccount [$buffer ulong]
		    set contexts [list]
		    for {set i 0} {$i < $sccount} {incr i} {
			lappend contexts [::Combat::IOP::DemarshalServiceContext $buffer]
		    }
		    $buffer align 8
		} else {
		    error "cannot decode GIOP $major $minor"
		}

		#
		# Switch buffer from ISO-8859-1 to the negotiated charset
		#

		$buffer configure -cdecoder $ccodec -wdecoder $wcodec

		#
		# File message
		#

		if {![info exists reply_status($theid)]} {
		    if {$::Combat::debug(giop)} {
			puts stderr "GIOP: unexpected reply from $peer for id $theid"
		    }
		    foreach context $contexts {
			itcl::delete object $context
		    }
		    itcl::delete object $decoder
		} elseif {$reply_status($theid) == -1} {
		    if {$::Combat::debug(giop)} {
			puts stderr "GIOP: reply from $peer for id $theid, status is $status"
		    }

		    set reply_status($theid) $status
		    set reply_contexts($theid) $contexts
		    set reply_decoder($theid) $decoder

		    if {[info exists callbacks($theid)]} {
			set cb $callbacks($theid)
			unset callbacks($theid)
			uplevel #0 "$cb $status"
		    }
		} else {
		    if {$::Combat::debug(giop)} {
			puts stderr "GIOP: duplicate reply from $peer for id $theid"
		    }
		    foreach context $contexts {
			itcl::delete object $context
		    }
		    itcl::delete object $decoder
		}
	    }

	    private method HandleIncomingRequest {message} {
		binary scan $message @4ccc major minor flags
		set byteorder [expr {$flags & 1} ? 1 : 0]
		set decoder [namespace current]::[::Combat::CDR::Decoder \#auto $message]
		set buffer [$decoder get_buffer]
		$buffer configure -byteorder $byteorder

		#
		# skip over 12 byte GIOP header
		#

		$buffer seek 12

		#
		# Header depends on GIOP version
		#

		if {$major == 1 && ($minor == 0 || $minor == 1)} {
		    set sccount [$buffer ulong]
		    set contexts [list]
		    for {set i 0} {$i < $sccount} {incr i} {
			lappend contexts [::Combat::IOP::DemarshalServiceContext \
				$buffer]
		    }
		    set request_id [$buffer ulong]
		    if {[$buffer boolean]} {
			set response_flags 3
		    } else {
			set response_flags 0
		    }
		    set olen [$buffer ulong]
		    set object_key [$buffer octets $olen]
		    set operation [$buffer string]
		    set plength [$buffer ulong]
		    set principal [$buffer octets $plength]
		} elseif {$major == 1 && $minor == 2} {
		    set request_id [$buffer ulong]
		    set response_flags_data [$buffer octet]
		    binary scan $response_flags_data c response_flags
		    set reserved [$buffer octets 3]
		    set adressing_disposition [$buffer short]
		    switch -- $adressing_disposition {
			0 { # KeyAddr
			    set klength [$buffer ulong]
			    set object_key [$buffer octets $klength]
			}
			1 { # ProfileAddr
			    set profile [::Combat::IOP::DemarshalTaggedProfile $buffer]
			    if {![$profile isa ::Combat::IIOP::ProfileBody]} {
				error "oops, expected IIOP Profile"
			    }

			    set object_key [$profile cget -object_key]
			    itcl::delete object $profile
			}
			2 { # ReferenceAddr
			    set selected_profile_index [$buffer ulong]
			    set ior [::Combat::IOP::DemarshalIOR $buffer]
			    set profile [lindex [$ior cget -profiles] \
				    $selected_profile_index]
			    set object_key [$profile cget -object_key]
			    itcl::delete object $ior
			}
		    }
		    set operation [$buffer string]
		    set sccount [$buffer ulong]
		    set contexts [list]
		    for {set i 0} {$i < $sccount} {incr i} {
			lappend contexts [::Combat::IOP::DemarshalServiceContext \
				$buffer]
		    }
		    $buffer align 8
		} else {
		    error "cannot decode GIOP $major.$minor"
		}

		if {$::Combat::debug(giop)} {
		    puts stderr "GIOP: incoming request from $peer as id $request_id"
		}

		#
		# If the client requests a GIOP version lower than the
		# one we're configured for, speak down to it
		#

		if {$major < $major_version || \
			($major == $major_version && $minor < $minor_version)} {
		    set major_version $major
		    set minor_version $minor
		}

		#
		# Do codeset negotiation upon first request
		# See if we've got a CodeSetContext
		#

		for {set i 0} {$i < [llength $contexts]} {incr i} {
		    set context [lindex $contexts $i]
		    if {[$context isa ::Combat::CONV_FRAME::CodeSetContext]} {
			break
		    }
		}

		if {!$cndone} {
		    if {$major_version == 1 && $minor_version == 0} {
			#
			# no codeset negotiation for GIOP 1.0
			#
		    } elseif {$i == [llength $contexts]} {
			#
			# didn't get a CodeSetContext
			#
		    } else {
			set tcsc [$context cget -char_data]
			set tcsw [$context cget -wchar_data]

			if {$::Combat::debug(giop)} {
			    puts stderr "GIOP: Codeset Negotiation with $peer:"
			    if {$tcsc == 0} {
				puts stderr "GIOP:    TCS-C: $tcsc (native)"
			    } else {
				puts stderr "GIOP:    TCS-C: $tcsc ([lindex $::Combat::CONV_FRAME::codesets($tcsc) 1])"
			    }
			    if {$tcsw == 0} {
				puts stderr "GIOP:    TCS-W: $tcsw (native)"
			    } else {
				puts stderr "GIOP:    TCS-W: $tcsw ([lindex $::Combat::CONV_FRAME::codesets($tcsw) 1])"
			    }
			}

			#
			# for ORBs that send a broken CodeSetContext, we
			# accept a zero TCS value.
			#

			if {$tcsc != 0} {
			    set ccodec [::Combat::CONV_FRAME::getConverter \
				    $major_version $minor_version $tcsc]
			}
			if {$tcsw != 0} {
			    set wcodec [::Combat::CONV_FRAME::getConverter \
				    $major_version $minor_version $tcsw]
			}
		    }
		    set cndone 1
		}

		#
		# Count as an active request if a response is expected
		#

		if {$response_flags & 0x01} {
		    incr active_requests 1
		}

		if {$timeout_token != ""} {
		    after cancel $timeout_token
		    set timeout_token ""
		}

		if {!$active_requests && $::Combat::GIOP::idleTimeout} {
		    set timeout_token [after $::Combat::GIOP::idleTimeout "$this idle_timeout_callback"]
		}

		#
		# GIOP 1.2 request body needs extra alignment
		#
		
		if {$major_version == 1 && $minor_version == 2} {
		    $buffer align 8
		}

		#
		# Switch buffer from ISO-8859-1 to the negotiated charset
		#

		$buffer configure -cdecoder $ccodec -wdecoder $wcodec

		#
		# Pass invocation to the ORB
		#

		::Combat::CORBA::ORB::invoke $response_flags \
			$this $request_id $object_key $operation \
			$decoder $contexts
	    }

	    private method HandleIncomingLocateRequest {message} {
		binary scan $message @4ccc major minor flags
		set byteorder [expr {$flags & 1} ? 1 : 0]
		set decoder [namespace current]::[::Combat::CDR::Decoder \#auto $message]
		set buffer [$decoder get_buffer]
		$buffer configure -byteorder $byteorder

		#
		# skip over 12 byte GIOP header
		#

		$buffer seek 12

		#
		# Header depends on GIOP version
		#

		if {$major == 1 && ($minor == 0 || $minor == 1)} {
		    set request_id [$buffer ulong]
		    set olen [$buffer ulong]
		    set object_key [$buffer octets $olen]
		} elseif {$major == 1 && $minor == 2} {
		    set request_id [$buffer ulong]
		    set adressing_disposition [$buffer short]
		    switch -- $adressing_disposition {
			0 { # KeyAddr
			    set klength [$buffer ulong]
			    set object_key [$buffer octets $klength]
			}
			1 { # ProfileAddr
			    set profile [::Combat::IOP::DemarshalTaggedProfile $buffer]
			    if {![$profile isa ::Combat::IIOP::ProfileBody]} {
				error "oops, expected IIOP Profile"
			    }

			    set object_key [$profile cget -object_key]
			    itcl::delete object $profile
			}
			2 { # ReferenceAddr
			    set selected_profile_index [$buffer ulong]
			    set ior [::Combat::IOP::DemarshalIOR $buffer]
			    set profile [lindex [$ior cget -profiles] \
				    $selected_profile_index]
			    set object_key [$profile cget -object_key]
			    itcl::delete object $ior
			}
		    }
		} else {
		    error "cannot decode GIOP $major.$minor"
		}

		if {$::Combat::debug(giop)} {
		    puts stderr "GIOP: incoming locate request from $peer as id $request_id"
		}

		#
		# If the client requests a GIOP version lower than the
		# one we're configured for, speak down to it
		#

		if {$major < $major_version || \
			($major == $major_version && $minor < $minor_version)} {
		    set major_version $major
		    set minor_version $minor
		}

		itcl::delete object $decoder

		#
		# Count as an active request
		#

		incr active_requests 1

		if {$timeout_token != ""} {
		    after cancel $timeout_token
		    set timeout_token ""
		}

		#
		# Pass invocation to the ORB
		#

		::Combat::CORBA::ORB::locate $this $request_id $object_key
	    }

	    #
	    # Close connection, either upon CloseConnection or in case
	    # of a broken connection.
	    #
	    # Failure before a connection was established is treated as
	    # orderly shutdown
	    #

	    protected method CloseConnection {{orderly 0}} {
		if {!$orderly} {
		    set broken -1		;# no CloseConnection received
		    set status 2		;# guise as SYSTEM_EXCEPTION
		    if {$::Combat::debug(giop)} {
			puts stderr "GIOP: connection with $peer is broken"
		    }
		} elseif {$connected && $orderly} {
		    set broken 1		;# got a CloseConnection
		    set status 3		;# guise as LOCATION_FORWARD
		    if {$::Combat::debug(giop)} {
			puts stderr "GIOP: got CloseConnection from $peer"
		    }
		} else {
		    set broken 1		;# got a CloseConnection
		    set status 3		;# guise as LOCATION_FORWARD
		    if {$::Combat::debug(giop)} {
			puts stderr "GIOP: connecting to $peer has failed"
		    }
		}

		if {$timeout_token != ""} {
		    after cancel $timeout_token
		    set timeout_token ""
		}

		foreach theid [array names callbacks] {
		    set cb $callbacks($theid)
		    unset callbacks($theid)
		    uplevel #0 "$cb $status"
		}

		foreach theid [array names reply_status] {
		    unset reply_status($theid)

		    if {[info exists reply_decoder($theid)]} {
			itcl::delete object $reply_decoder($theid)
			unset reply_decoder($theid)
		    }

		    if {[info exists reply_contexts($theid)]} {
			foreach context $reply_contexts($theid) {
			    itcl::delete object $context
			}
			unset reply_contexts($theid)
		    }

		    incr active_requests -1
		}

		close
	    }

	    #
	    # callback for incoming data
	    #

	    public method callback {} {
		if {$pending_header == ""} {
		    set inlength [size]

		    if {$inlength < 12 && $inlength != -1} {
			# header incomplete, wait for next callback
			return
		    }

		    if {[catch {set pending_header [recv 12]}]} {
			# EOF
			CloseConnection
			return
		    }

		    set res [binary scan $pending_header a4cccc \
			    magic pending_major pending_minor \
			    pending_flags pending_type]
		    
		    if {$res != 5 || $magic != "GIOP"} {
			#
			# oops
			#
			if {$::Combat::debug(giop)} {
			    puts stderr "GIOP: got invalid data from $peer"
			    ::Combat::DumpOctets IllData $pending_header
			}
			CloseConnection
			return
		    }

		    if {$pending_major == 1 && $pending_minor == 0} {
			set pending_byteorder $pending_flags
			set pending_fragment 0
		    } elseif {$pending_major == 1 && \
			    ($pending_minor == 1 || $pending_minor == 2)} {
			set pending_byteorder [expr {$pending_flags & 1}]
			set pending_fragment [expr {($pending_flags & 2) >> 1}]
		    } else {
			if {$::Combat::debug(giop)} {
			    puts stderr "GIOP: $peer sends unsupported GIOP $pending_major.$pending_minor data"
			    ::Combat::DumpOctets IllData $pending_header
			}
			CloseConnection
			return
		    }

		    if {$pending_byteorder == 0} {
			binary scan $pending_header @8I pending_size
		    } else {
			binary scan $pending_header @8i pending_size
		    }

		    if {$::Combat::GIOP::ORBGIOPMaxSize != 0 && \
			    $pending_size > $::Combat::GIOP::ORBGIOPMaxSize} {
			if {$::Combat::debug(giop)} {
			    puts stderr "GIOP: refusing to handle message of size $pending_size, threshold is $::Combat::GIOP::ORBGIOPMaxSize"
			}
			CloseConnection
			return
		    }

		    if {$pending_fragment || $pending_type == 7} {
			if {$::Combat::GIOP::ORBGIOPMaxSize != 0 && \
				[expr {$pending_frag_size + $pending_size}] > \
				$::Combat::GIOP::ORBGIOPMaxSize} {
			    if {$::Combat::debug(giop)} {
				puts stderr "GIOP: incoming fragment of size $pending_size, already got $pending_frag_size"
				puts stderr "GIOP: exceeding threshold is $::Combat::GIOP::ORBGIOPMaxSize"
			    }
			    CloseConnection
			    return
			}
		    }
		}

		#
		# See if we can read the message body
		#

		set inlength [size]

		if {$inlength < $pending_size && $inlength != -1} {
		    # message incomplete, wait for next callback
		    return
		}

		if {[catch {set data [recv $pending_size]}]} {
		    # EOF
		    CloseConnection
		    return
		}

		if {$::Combat::debug(transport)} {
		    puts stderr "GIOP: Incoming data from $peer:"
		    Combat::DumpOctets Data "$pending_header$data"
		}

		#
		# reset header information
		#

		set header $pending_header
		set pending_header ""

		#
		# If there is still incoming data available, arrange to
		# call me again, as the readable fileevent won't
		#

		if {[size] != 0} {
		    after 0 "$this callback"
		}

		#
		# Handle fragments
		#

		if {$pending_type == 7} {
		    if {$pending_major == 1 && $pending_minor == 2} {
			if {$pending_byteorder == 0} {
			    binary scan $data I request_id
			} else {
			    binary scan $data i request_id
			}
			if {![info exists pending_fragments($request_id)]} {
			    error "oops, got GIOP fragment for request $request_id, which wasn't fragmented"
			}
			if {$::Combat::debug(giop)} {
			    if {$pending_fragment} {
				set txt "(more follow)"
			    } else {
				set txt "(complete)"
			    }
			    puts stderr "GIOP: got fragment for id $request_id $txt"
			}
			incr pending_frag_size $pending_size
			append pending_fragments($request_id) [string range $data 4 end]
			if {$pending_fragment} {
			    return
			}
			set pending_message $pending_fragments($request_id)
			set pending_frag_size [expr {$pending_frag_size - \
				[string length $pending_message]}]
			unset pending_fragments($request_id)
		    } else {
			CloseConnection
			error "don't wanna handle GIOP $pending_major.$pending_minor fragments."
		    }

		    binary scan $pending_message @7c pending_type
		} elseif {$pending_fragment} {
		    if {$pending_major == 1 && $pending_minor == 2} {
			if {$pending_byteorder == 0} {
			    binary scan $data I request_id
			} else {
			    binary scan $data i request_id
			}
			if {$::Combat::debug(giop)} {
			    puts stderr "GIOP: got initial fragment for id $request_id"
			}
			set pending_fragments($request_id) $header
			append pending_fragments($request_id) $data
			return
		    } else {
			CloseConnection
			error "don't wanna handle GIOP $pending_major.$pending_minor fragments."
		    }
		} else {
		    set pending_message $header
		    append pending_message $data
		}

		switch -- $pending_type {
		    0 { # Request
			if {[catch {
			    HandleIncomingRequest $pending_message
			} err]} {
			    if {$::Combat::debug(giop)} {
				global errorInfo
				puts stderr "GIOP: error in Request handler for $peer: $err"
				puts stderr $errorInfo
			    }
			}
		    }
		    1 { # Reply
			if {[catch {
			    HandleIncomingReply $pending_message
			} err]} {
			    if {$::Combat::debug(giop)} {
				global errorInfo
				puts stderr "GIOP: error in Reply handler for $peer: $err"
				puts stderr $errorInfo
			    }
			}
		    }
		    2 { # CancelRequest
			#
			# Currently ignored. This is acceptable behavior.
			#
			# "When a client issues a cancel request message, it
			# serves in an advisory capacity only. The server is
			# not required to acknowledge the cancellation, and
			# may subsequently send the corresponding reply. The
			# client should have no expectation about whether a
			# reply (including an exceptional one) arrives."
			#
		    }
		    3 { # LocateRequest
			if {[catch {
			    HandleIncomingLocateRequest $pending_message
			} err]} {
			    if {$::Combat::debug(giop)} {
				global errorInfo
				puts stderr "GIOP: error in LocateRequest handler for $peer: $err"
				puts stderr $errorInfo
			    }
			}
		    }
		    4 { # LocateReply
			CloseConnection
			error "not implemented"
		    }
		    5 { # CloseConnection
			CloseConnection 1
		    }
		    6 { # MessageError
			CloseConnection
			error "not implemented"
		    }
		    7 { # Fragment
			error "oops, shouldn't be here"
		    }
		    default {
			if {$::Combat::debug(giop)} {
			    puts stderr "GIOP: unknown GIOP message type $pending_type from $peer"
			}
			CloseConnection
		    }
		}
	    }

	    public method idle_timeout_callback {} {
		if {$::Combat::debug(giop)} {
		    puts stderr "GIOP: connection to $peer idle timeout"
		}

		#
		# Send CloseConnection message
		#
		# CloseConnection messages are sent only by servers in GIOP
		# protocol versions 1.0 and 1.1.  [...]  In GIOP version 1.2
		# and 1.3 both sides of the connection may send the
		# CloseConnection message.
		#

		if {!$isclient || \
			($major_version == 1 && $minor_version <= 1)} {
		    if {$::tcl_platform(byteOrder) == "bigEndian"} {
			set byteorder 0
		    } else {
			set byteorder 1
		    }

		    set header [binary format a4ccccI \
				    GIOP \
				    $major_version $minor_version \
				    $byteorder 5 0]

		    if {$::Combat::debug(giop)} {
			puts stderr "GIOP: sending CloseConnection message to $peer"
		    }

		    if {$::Combat::debug(transport)} {
			::Combat::DumpOctets Close $header
		    }

		    catch {
			send $header
		    }
		}

		#
		# Close our end of the connection
		#

		CloseConnection 1
	    }
	}
    }
}

