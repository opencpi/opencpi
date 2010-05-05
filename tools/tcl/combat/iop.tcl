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
# CVS Version Tag: $Id: iop.tcl,v 1.7 2008/11/08 18:24:50 Owner Exp $
#
# ----------------------------------------------------------------------
# IOP Module
# ----------------------------------------------------------------------
#

namespace eval Combat {
    namespace eval IOP {
	itcl::class TaggedComponent {
	    public variable tag
	    public variable component_data

	    public method marshal {buffer} {
		$buffer ulong $tag
		$buffer ulong [string length $component_data]
		$buffer octets $component_data
	    }
	}

	proc DemarshalTaggedComponent {buffer} {
	    set tag [$buffer ulong]

	    switch -- $tag {
		1 { # TAG_CODE_SETS
		    set res [::Combat::CONV_FRAME::DemarshalCodeSetInfo $buffer]
		}
		default {
		    set length [$buffer ulong]
		    set component_data [$buffer octets $length]
		    set res [namespace current]::[TaggedComponent \#auto]
		    $res configure -tag $tag -component_data $component_data
		}
	    }

	    return $res
	}

	itcl::class ServiceContext {
	    public variable context_id
	    public variable context_data

	    public method marshal {buffer} {
		$buffer ulong $context_id
		$buffer ulong [string length $context_data]
		$buffer octets $context_data
	    }
	}

	proc DemarshalServiceContext {buffer} {
	    set context_id [$buffer ulong]

	    switch -- $context_id {
		1 { # CodeSets
		    set res [::Combat::CONV_FRAME::DemarshalCodeSetContext $buffer]
		}
		default {
		    set length [$buffer ulong]
		    set context_data [$buffer octets $length]
		    set res [namespace current]::[ServiceContext \#auto]
		    $res configure -context_id $context_id \
			    -context_data $context_data
		}
	    }

	    return $res
	}

	itcl::class TaggedProfile {
	    public variable tag
	    public variable profile_data

	    public method connect {} {
		error "not reachable"
	    }

	    public method marshal {buffer} {
		$buffer ulong $tag
		$buffer ulong [string length $profile_data]
		$buffer octets $profile_data
	    }
	}

	itcl::class MultipleComponentProfile {
	    inherit ::Combat::IOP::TaggedProfile
	    public variable components

	    constructor {} {
		set components [list]
	    }

	    destructor {
		foreach component $components {
		    itcl::delete object $component
		}
	    }

	    public method marshal {buffer} {
		$buffer ulong 1		;# TAG_MULTIPLE_COMPONENTS
		$buffer begin_encaps
		$buffer ulong [llength $components]
		foreach component $components {
		    $component marshal $buffer
		}
		$buffer end_encaps
	    }
	}

	proc DemarshalMultipleComponentProfile {buffer} {
	    $buffer begin_encaps
	    set components [list]
	    set count [$buffer ulong]
	    for {set i 0} {$i < $count} {incr i} {
		lappend components [::Combat::IOP::DemarshalTaggedComponent $buffer]
	    }
	    $buffer end_encaps

	    set res [namespace current]::[::Combat::IOP::MultipleComponentProfile \#auto]
	    $res configure -tag 1 -components $components
	    return $res
	}

	proc DemarshalTaggedProfile {buffer} {
	    set tag [$buffer ulong]

	    switch -- $tag {
		0 { # TAG_INTERNET_IOP
		    set res [::Combat::IIOP::DemarshalIIOPProfile $buffer]
		}
		1 { # TAG_MULTIPLE_COMPONENTS
		    set res [::Combat::IOP::DemarshalMultipleComponentProfile $buffer]
		}
		default {
		    set length [$buffer ulong]
		    set profile_data [$buffer octets $length]
		    set res [namespace current]::[TaggedProfile \#auto]
		    $res configure -tag $tag -profile_data $profile_data
		}
	    }

	    return $res
	}

	itcl::class IOR {
	    public variable type_id
	    public variable profiles

	    constructor {} {
		set type_id ""
		set profiles [list]
	    }

	    destructor {
		foreach profile $profiles {
		    itcl::delete object $profile
		}
	    }

	    public method marshal {buffer} {
		$buffer string $type_id
		$buffer ulong [llength $profiles]
		foreach profile $profiles {
		    $profile marshal $buffer
		}
	    }

	    public method stringify {} {
		set buffer [namespace current]::[::Combat::CDR::WriteBuffer \#auto]
		$buffer boolean [$buffer cget -byteorder]
		marshal $buffer
		set data [$buffer get_data]
		binary scan $data H* hex
		itcl::delete object $buffer
		return "IOR:$hex"
	    }

	    #
	    # Find Codeset info in a MultipleComponents profile.
	    #

	    public method getCodesetInfo {} {
		foreach profile $profiles {
		    # test for TAG_MULTIPLE_COMPONENTS
		    if {[$profile cget -tag] == 1} {
			foreach component [$profile cget -components] {
			    # test for TAG_CODE_SETS
			    if {[$component cget -tag] == 1} {
				return $component
			    }
			}
		    }
		}
		return ""
	    }
	}

	proc DemarshalIOR {buffer} {
	    set type_id [$buffer string]
	    set profiles [list]
	    set numofprofiles [$buffer ulong]
	    for {set i 0} {$i < $numofprofiles} {incr i} {
		lappend profiles [::Combat::IOP::DemarshalTaggedProfile $buffer]
	    }
	    set ior [namespace current]::[::Combat::IOP::IOR \#auto]
	    $ior configure -type_id $type_id -profiles $profiles
	    return $ior
	}

	proc DestringifyIOR {ior} {
	    if {[string range $ior 0 3] != "IOR:"} {
		error "not a stringified IOR: $ior"
	    }

	    set hex [string range $ior 4 end]
	    set data [binary format H* $hex]
	    set buffer [namespace current]::[::Combat::CDR::ReadBuffer \#auto $data]
	    set byteorder [$buffer boolean]
	    $buffer configure -byteorder $byteorder
	    set res [DemarshalIOR $buffer]
	    itcl::delete object $buffer
	    return $res
	}

	itcl::class CDRCodec {
	    public variable major_version
	    public variable minor_version
	    public variable wcodec

	    constructor {major minor} {
		set major_version $major
		set minor_version $minor
		if {$minor_version == 2} {
		    set wcodec [::Combat::CONV_FRAME::getConverter $major $minor 65792]
		} else {
		    set wcodec ""
		}
	    }

	    destructor {
		if {$wcodec != ""} {
		    itcl::delete object $wcodec
		}
	    }

	    public method encode {data} {
		set encoder [::Combat::CDR::Encoder \#auto]
		set buffer [$encoder get_buffer]
		$buffer configure -wencoder $wcodec
		$buffer boolean [$buffer cget -byteorder]
		corba::try {
		    $encoder Marshal any $data
		} catch {... ex} {
		    itcl::delete object $encoder
		    corba::throw $ex
		}
		set octets [$encoder get_data]
		itcl::delete object $encoder
		return $octets
	    }

	    public method encode_value {data} {
		set encoder [::Combat::CDR::Encoder \#auto]
		set buffer [$encoder get_buffer]
		$buffer configure -wencoder $wcodec
		$buffer boolean [$buffer cget -byteorder]
		corba::try {
		    $encoder Marshal [lindex $data 0] [lindex $data 1]
		} catch {... ex} {
		    itcl::delete object $encoder
		    corba::throw $ex
		}
		set octets [$encoder get_data]
		itcl::delete object $encoder
		return $octets
	    }

	    public method decode {octets} {
		set decoder [::Combat::CDR::Decoder \#auto $octets]
		set buffer [$decoder get_buffer]
		$buffer configure -wdecoder $wcodec
		$buffer configure -byteorder [$buffer boolean]
		corba::try {
		    set data [$decoder Demarshal any]
		} catch {... ex} {
		    itcl::delete object $encoder
		    corba::throw $ex
		}
		itcl::delete object $decoder
		return $data
	    }

	    public method decode_value {octets tc} {
		set decoder [::Combat::CDR::Decoder \#auto $octets]
		set buffer [$decoder get_buffer]
		$buffer configure -wdecoder $wcodec
		$buffer configure -byteorder [$buffer boolean]
		corba::try {
		    set data [$decoder Demarshal $tc]
		} catch {... ex} {
		    itcl::delete object $encoder
		    corba::throw $ex
		}
		itcl::delete object $decoder
		return $data
	    }
	}

	itcl::class CodecFactory {
	    public method create_codec {enc} {
		array set encoding $enc

		if {![info exists encoding(format)] || \
			![info exists encoding(major_version)] || \
			![info exists encoding(minor_version)]} {
		    array unset encoding
		    corba::throw [list IDL:omg.org/CORBA/BAD_PARAM \
				      [list minor 1 completion_status COMPLETED_NO]]
		}

		set format $encoding(format)
		set major $encoding(major_version)
		set minor $encoding(minor_version)
		array unset encoding

		if {$format != 0 || $major != 1 || $minor < 0 || $minor > 2} {
		    corba::throw IDL:omg.org/IOP/CodecFactory/UnknownEncoding:1.0
		}

		set codec [namespace current]::[::Combat::IOP::CDRCodec \#auto $major $minor]
		return $codec
	    }
	}

    }
}

