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
# CVS Version Tag: $Id: str.tcl,v 1.16 2008/11/14 02:06:59 Owner Exp $
#

namespace eval Combat {
    namespace eval SimpleTypeRepository {
	variable absnames	;# array absnames -> id
	variable elements	;# array id -> irobject
	variable contained	;# array id -> list of id
	variable ifcollected    ;# array id -> 1
	variable attributes	;# array ifaceid,name -> attribute desc
	variable operations	;# array ifaceid,name -> op desc
	variable downloaded
	variable downattrs
	variable downops

	#
	# TypeCode for FullInterfaceDescription <groan>
	#

	set fid_tc {struct \
		IDL:omg.org/CORBA/InterfaceDef/FullInterfaceDescription:1.0 \
		{name string id string defined_in string version string \
		operations {sequence {struct \
		IDL:omg.org/CORBA/OperationDescription:1.0 \
		{name string id string defined_in string version string \
		result TypeCode mode {enum {OP_NORMAL OP_ONEWAY}} \
		contexts {sequence string} parameters {sequence \
		{struct IDL:omg.org/CORBA/ParameterDescription:1.0 \
		{name string type TypeCode type_def {Object \
		IDL:omg.org/CORBA/IDLType:1.0} mode {enum {PARAM_IN \
		PARAM_OUT PARAM_INOUT}}}}} exceptions {sequence \
		{struct IDL:omg.org/CORBA/ExceptionDescription:1.0 \
		{name string id string defined_in string version \
		string type TypeCode}}}}}} attributes {sequence \
		{struct IDL:omg.org/CORBA/AttributeDescription:1.0 \
		{name string id string defined_in string version string \
		type TypeCode mode {enum {ATTR_NORMAL ATTR_READONLY}}}}} \
		base_interfaces {sequence string} type TypeCode}}

	variable efid_tc {struct \
		IDL:omg.org/CORBA/InterfaceAttrExtension/ExtFullInterfaceDescription:1.0 \
		{name string id string defined_in string version string \
		operations {sequence {struct \
		IDL:omg.org/CORBA/OperationDescription:1.0 {name string \
		id string defined_in string version string result TypeCode \
		mode {enum {OP_NORMAL OP_ONEWAY}} contexts {sequence string} \
		parameters {sequence {struct \
		IDL:omg.org/CORBA/ParameterDescription:1.0 {name string \
		type TypeCode type_def {Object IDL:omg.org/CORBA/IDLType:1.0} \
		mode {enum {PARAM_IN PARAM_OUT PARAM_INOUT}}}}} exceptions \
		{sequence {struct IDL:omg.org/CORBA/ExceptionDescription:1.0 \
		{name string id string defined_in string version string type \
		TypeCode}}}}}} attributes {sequence {struct \
		IDL:omg.org/CORBA/ExtAttributeDescription:1.0 {name string \
		id string defined_in string version string type TypeCode \
		mode {enum {ATTR_NORMAL ATTR_READONLY}} get_exceptions \
		{sequence {struct IDL:omg.org/CORBA/ExceptionDescription:1.0 \
		{name string id string defined_in string version string \
		type TypeCode}}} set_exceptions {sequence {struct \
		IDL:omg.org/CORBA/ExceptionDescription:1.0 {name string \
		id string defined_in string version string type \
		TypeCode}}}}}} base_interfaces {sequence string} type \
		TypeCode}}

	#
	# Some known absolute name to Repository Id mappings.  This is
	# necessary so that CORBA system exceptions can be caught by name.
	#

	array set absnames {
	    ::CORBA::Object IDL:omg.org/CORBA/Object:1.0
	    ::CORBA::UNKNOWN IDL:omg.org/CORBA/UNKNOWN:1.0
	    ::CORBA::BAD_PARAM IDL:omg.org/CORBA/BAD_PARAM:1.0
	    ::CORBA::NO_MEMORY IDL:omg.org/CORBA/NO_MEMORY:1.0
	    ::CORBA::IMP_LIMIT IDL:omg.org/CORBA/IMP_LIMIT:1.0
	    ::CORBA::COMM_FAILURE IDL:omg.org/CORBA/COMM_FAILURE:1.0
	    ::CORBA::INV_OBJREF IDL:omg.org/CORBA/INV_OBJREF:1.0
	    ::CORBA::NO_PERMISSION IDL:omg.org/CORBA/NO_PERMISSION:1.0
	    ::CORBA::INTERNAL IDL:omg.org/CORBA/INTERNAL:1.0
	    ::CORBA::MARSHAL IDL:omg.org/CORBA/MARSHAL:1.0
	    ::CORBA::INITIALIZE IDL:omg.org/CORBA/INITIALIZE:1.0
	    ::CORBA::NO_IMPLEMENT IDL:omg.org/CORBA/NO_IMPLEMENT:1.0
	    ::CORBA::BAD_TYPECODE IDL:omg.org/CORBA/BAD_TYPECODE:1.0
	    ::CORBA::BAD_OPERATION IDL:omg.org/CORBA/BAD_OPERATION:1.0
	    ::CORBA::NO_RESOURCES IDL:omg.org/CORBA/NO_RESOURCES:1.0
	    ::CORBA::NO_RESPONSE IDL:omg.org/CORBA/NO_RESPONSE:1.0
	    ::CORBA::PERSIST_STORE IDL:omg.org/CORBA/PERSIST_STORE:1.0
	    ::CORBA::BAD_INV_ORDER IDL:omg.org/CORBA/BAD_INV_ORDER:1.0
	    ::CORBA::TRANSIENT IDL:omg.org/CORBA/TRANSIENT:1.0
	    ::CORBA::FREE_MEM IDL:omg.org/CORBA/FREE_MEM:1.0
	    ::CORBA::INV_IDENT IDL:omg.org/CORBA/INV_IDENT:1.0
	    ::CORBA::INV_FLAG IDL:omg.org/CORBA/INV_FLAG:1.0
	    ::CORBA::INTF_REPOS IDL:omg.org/CORBA/INTF_REPOS:1.0
	    ::CORBA::BAD_CONTEXT IDL:omg.org/CORBA/BAD_CONTEXT:1.0
	    ::CORBA::OBJ_ADAPTER IDL:omg.org/CORBA/OBJ_ADAPTER:1.0
	    ::CORBA::DATA_CONVERSION IDL:omg.org/CORBA/DATA_CONVERSION:1.0
	    ::CORBA::OBJECT_NOT_EXIST IDL:omg.org/CORBA/OBJECT_NOT_EXIST:1.0
	    ::CORBA::TRANSACTION_REQUIRED IDL:omg.org/CORBA/TRANSACTION_REQUIRED:1.0
	    ::CORBA::TRANSACTION_ROLLEDBACK IDL:omg.org/CORBA/TRANSACTION_ROLLEDBACK:1.0
	    ::CORBA::INVALID_TRANSACTION IDL:omg.org/CORBA/INVALID_TRANSACTION:1.0
	    ::CORBA::INV_POLICY IDL:omg.org/CORBA/INV_POLICY:1.0
	    ::CORBA::CODESET_INCOMPATIBLE IDL:omg.org/CORBA/CODESET_INCOMPATIBLE:1.0
	    ::CORBA::TIMEOUT IDL:omg.org/CORBA/TIMEOUT:1.0
	}

	itcl::class IRObject {
	    public variable id
	    public variable name

	    constructor {theid thename} {
		set id $theid
		set name $thename
	    }
	}
	itcl::class PrimitiveDef {
	    public variable kind

	    public method type {} {
		if {$kind == "value base"} {
		    return [list valuetype \
			    IDL:omg.org/CORBA/ValueBase:1.0 \
			    [list] "" ""]
		}
		return $kind
	    }
	}

	itcl::class ConstantDef {
	    inherit ::Combat::SimpleTypeRepository::IRObject

	    public variable type_def
	    public variable value

	    constructor {theid thename} {
		::Combat::SimpleTypeRepository::IRObject::constructor $theid $thename
	    } {}

	    public method type {
		return [$type_def type]
	    }
	}

	itcl::class StructDef {
	    inherit ::Combat::SimpleTypeRepository::IRObject

	    public variable members
	    private variable visited

	    constructor {theid thename} {
		::Combat::SimpleTypeRepository::IRObject::constructor $theid $thename
	    } {
		set visited 0
	    }

	    public method type {} {
		if {$visited} {
		    return [list recursive $id]
		}
		set visited 1
		set mtc [list]
		foreach member $members {
		    set member_name [lindex $member 0]
		    set member_type [[lindex $member 1] type]
		    lappend mtc $member_name $member_type
		}
		set visited 0
		return [list struct $id $mtc]
	    }
	}

	itcl::class ExceptionDef {
	    inherit ::Combat::SimpleTypeRepository::IRObject

	    public variable members
	    private variable visited

	    constructor {theid thename} {
		::Combat::SimpleTypeRepository::IRObject::constructor $theid $thename
	    } {
		set visited 0
	    }

	    public method type {} {
		if {$visited} {
		    return [list recursive $id]
		}
		set visited 1
		set mtc [list]
		foreach member $members {
		    set member_name [lindex $member 0]
		    set member_type [[lindex $member 1] type]
		    lappend mtc $member_name $member_type
		}
		set visited 0
		return [list exception $id $mtc]
	    }

	    public method describe {} {
		return [list \
			name $name \
			id $id \
			defined_in dummy \
			version dummy \
			type [type]]
	    }
	}

	itcl::class UnionDef {
	    inherit ::Combat::SimpleTypeRepository::IRObject

	    public variable members
	    public variable discriminator_type_def
	    private variable visited

	    constructor {theid thename} {
		::Combat::SimpleTypeRepository::IRObject::constructor $theid $thename
	    } {
		set visited 0
	    }

	    public method type {} {
		if {$visited} {
		    return [list recursive $id]
		}
		set visited 1
		set mtc [list]
		foreach member $members {
		    set label [lindex $member 0]
		    set mname [lindex $member 1]
		    set mtype [[lindex $member 2] type]
		    lappend mtc $label $mtype
		}
		set visited 0
		return [list union $id [$discriminator_type_def type] $mtc]
	    }
	}

	itcl::class EnumDef {
	    inherit ::Combat::SimpleTypeRepository::IRObject

	    public variable members

	    constructor {theid thename} {
		::Combat::SimpleTypeRepository::IRObject::constructor $theid $thename
	    } {}

	    public method type {} {
		return [list enum $members]
	    }
	}

	itcl::class AliasDef {
	    inherit ::Combat::SimpleTypeRepository::IRObject

	    public variable original_type_def

	    constructor {theid thename} {
		::Combat::SimpleTypeRepository::IRObject::constructor $theid $thename
	    } {}

	    public method type {} {
		return [$original_type_def type]
	    }
	}

	itcl::class StringDef {
	    public variable bound

	    public method type {} {
		if {$bound == 0} {
		    return string
		}
		return [list string $bound]
	    }
	}

	itcl::class WStringDef {
	    public variable bound

	    public method type {} {
		if {$bound == 0} {
		    return wstring
		}
		return [list wstring $bound]
	    }
	}

	itcl::class FixedDef {
	    public variable digits
	    public variable scale

	    public method type {} {
		return [list fixed $digits $scale]
	    }
	}

	itcl::class SequenceDef {
	    public variable element_type_def
	    public variable bound

	    public method type {} {
		if {$bound != 0} {
		    return [list sequence [$element_type_def type] $bound]
		}
		return [list sequence [$element_type_def type]]
	    }
	}

	itcl::class ArrayDef {
	    public variable element_type_def
	    public variable length

	    public method type {} {
		return [list array [$element_type_def type] $length]
	    }
	}

	itcl::class AttributeDef {
	    inherit ::Combat::SimpleTypeRepository::IRObject

	    public variable type_def
	    public variable mode

	    constructor {theid thename} {
		::Combat::SimpleTypeRepository::IRObject::constructor $theid $thename
	    } {}

	    #
	    # Returns an ExtAttributeDescription
	    #

	    public method describe {} {
		return [list \
			name $name \
			id $id \
			defined_in dummy \
			version dummy \
			type [$type_def type] \
			mode $mode \
			get_exceptions [list] \
			set_exceptions [list]]
	    }
	}

	itcl::class OperationDef {
	    inherit ::Combat::SimpleTypeRepository::IRObject

	    public variable result_def
	    public variable params
	    public variable mode
	    public variable exceptions

	    constructor {theid thename} {
		::Combat::SimpleTypeRepository::IRObject::constructor $theid $thename
	    } {}

	    public method describe {} {
		set pardescs [list]
		foreach param $params {
		    set par_mode [lindex $param 0]
		    set par_name [lindex $param 1]
		    set par_type [[lindex $param 2] type]
		    lappend pardescs [list \
			    name $par_name \
			    type $par_type \
			    type_def 0 \
			    mode $par_mode]
		}

		set exdescs [list]
		foreach ex $exceptions {
		    lappend exdescs [$ex describe]
		}

		return [list \
			name $name \
			id $id \
			defined_in dummy \
			version dummy \
			result [$result_def type] \
			mode $mode \
			contexts [list] \
			parameters $pardescs \
			exceptions $exdescs]
	    }
	}

	itcl::class InterfaceDef {
	    inherit ::Combat::SimpleTypeRepository::IRObject

	    public variable base_interfaces

	    constructor {theid thename} {
		::Combat::SimpleTypeRepository::IRObject::constructor $theid $thename
	    } {
		set base_interfaces [list]
	    }

	    public method type {} {
		return [list Object $id]
	    }

	    public method _is_a {other_id} {
		if {$id == $other_id} {
		    return 1
		} elseif {$id == "IDL:omg.org/CORBA/Object:1.0"} {
		    return 1
		}
		foreach base $base_interfaces {
		    if {[$base cget -id] == $other_id} {
			return 1
		    }
		    if {[$base _is_a $other_id]} {
			return 1
		    }
		}
		return 0
	    }
	
	    public method describe {} {
		set bases [list]
		foreach base $base_interfaces {
		    lappend bases [$base cget -id]
		}
		return [list name $name id $id defined_in dummy \
			version dummy base_interfaces $bases]
	    }

	    public method describe_ext_interface {} {
		set bases [list]
		foreach base $base_interfaces {
		    lappend bases [$base cget -id]
		}

		set operations [list]
		set attributes [list]

		#
		# Collect the descriptions of attributes and operations
		# in the transitive closure of the inheritance graph.
		#

		set queue $this
		while {[llength $queue]} {
		    set iface [lindex $queue 0]
		    set queue [lrange $queue 1 end]
		    set ifaceid [$iface cget -id]
		    if {[info exists hadbase($ifaceid)]} {
			continue
		    }
		    set hadbase($ifaceid) 1
		    foreach base [$iface cget -base_interfaces] {
			lappend queue $base
		    }

		    foreach contained $::Combat::SimpleTypeRepository::contained($ifaceid) {
			set el $::Combat::SimpleTypeRepository::elements($contained)

			if {[catch {$el isa ::Combat::SimpleTypeRepository::AttributeDef} isattr]} {
			    set isattr 0
			}
			
			if {[catch {$el isa ::Combat::SimpleTypeRepository::OperationDef} isop]} {
			    set isop 0
			}

			if {$isattr} {
			    lappend attributes [$el describe]
			} elseif {$isop} {
			    lappend operations [$el describe]
			}
		    }
		}

		array unset hadbase

		return [list \
			name $name \
			id $id \
			defined_in dummy \
			version dummy \
			operations $operations \
			attributes $attributes \
			base_interfaces $bases \
			type [list Object $id]]
	    }
	}

	itcl::class AbstractInterfaceDef {
	    inherit ::Combat::SimpleTypeRepository::InterfaceDef

	    constructor {theid thename} {
		::Combat::SimpleTypeRepository::InterfaceDef::constructor $theid $thename
	    } {}

	    public method type {} {
		return [list abstractinterface $id]
	    }
	
	    public method describe {} {
		set bases [list]
		foreach base $base_interfaces {
		    lappend bases [$base cget -id]
		}
		return [list name $name id $id defined_in dummy \
			version dummy base_interfaces $bases]
	    }
	}

	itcl::class ValueMemberDef {
	    inherit ::Combat::SimpleTypeRepository::IRObject

	    public variable type_def
	    public variable access

	    constructor {theid thename} {
		::Combat::SimpleTypeRepository::IRObject::constructor $theid $thename
	    } {}

	    public method type {} {
		return [$type_def type]
	    }
	}

	itcl::class ValueDef {
	    inherit ::Combat::SimpleTypeRepository::IRObject

	    public variable supported_interfaces
	    public variable initializers
	    public variable base_value
	    public variable abstract_base_values
	    public variable is_abstract
	    public variable is_custom
	    public variable is_truncatable
	    private variable visited

	    constructor {theid thename} {
		::Combat::SimpleTypeRepository::IRObject::constructor $theid $thename
	    } {
		set visited 0
	    }

	    public method type {} {
		if {$visited} {
		    return [list recursive $id]
		}

		set visited 1
		set members [list]

		foreach el $::Combat::SimpleTypeRepository::contained($id) {
		    set member $::Combat::SimpleTypeRepository::elements($el)
		    if {[catch {$member isa ::Combat::SimpleTypeRepository::ValueMemberDef} isvm]} {
			set isvm 0
		    }

		    if {$isvm} {
			if {[$member cget -access] == "PRIVATE_MEMBER"} {
			    set visi private
			} else {
			    set visi public
			}

			lappend members $visi [$member cget -name] \
				[[$member cget -type_def] type]
		    }
		}

		if {$base_value == ""} {
		    set base_type 0
		} else {
		    set base_type [$base_value type]
		}

		if {$is_custom} {
		    set modifier "custom"
		} elseif {$is_abstract} {
		    set modifier "abstract"
		} elseif {$is_truncatable} {
		    set modifier "truncatable"
		} else {
		    set modifier ""
		}

		set visited 0
		return [list valuetype $id $members $base_type $modifier]
	    }
	
	    public method describe {} {
		return [list name $name id $id defined_in dummy \
			version dummy base_interfaces $base_interfaces]
	    }
	}

	itcl::class ValueboxDef {
	    inherit ::Combat::SimpleTypeRepository::IRObject

	    public variable original_type_def

	    constructor {theid thename} {
		::Combat::SimpleTypeRepository::IRObject::constructor $theid $thename
	    } {}

	    public method type {} {
		return [list valuebox $id [$original_type_def type]]
	    }
	}

	itcl::class NativeDef {
	    inherit ::Combat::SimpleTypeRepository::IRObject

	    constructor {theid thename} {
		::Combat::SimpleTypeRepository::IRObject::constructor $theid $thename
	    } {}

	    public method type {} {
		return [list native $id]
	    }
	}

	proc scanIDLTypeName {typename} {
	    switch -- $typename {
		void -
		short -
		long -
		{unsigned short} -
		{unsigned long} -
		float -
		double -
		boolean -
		char -
		octet -
		any -
		TypeCode -
		string -
		Object -
		{long long} -
		{unsigned long long} -
		{long double} -
		wchar -
		wstring -
		{value base} {
		    set o [namespace current]::[PrimitiveDef \#auto]
		    $o configure -kind $typename
		    return $o
		}
	    }

	    if {[llength $typename] == 1} {
		return $::Combat::SimpleTypeRepository::elements($typename)
	    }
	    
	    switch -- [lindex $typename 0] {
		string {
		    set o [namespace current]::[StringDef \#auto]
		    $o configure -bound [lindex $typename 1]
		    return $o
		}
		wstring {
		    set o [namespace current]::[StringDef \#auto]
		    $o configure -bound [lindex $typename 1]
		    return $o
		}
		fixed {
		    set o [namespace current]::[FixedDef \#auto]
		    $o configure -digits [lindex $typename 1] \
			    -scale [lindex $typename 2]
		    return $o
		}
		sequence {
		    set o [namespace current]::[SequenceDef \#auto]
		    $o configure -element_type_def \
			    [scanIDLTypeName [lindex $typename 1]]

		    if {[llength $typename] == 3} {
			$o configure -bound [lindex $typename 2]
		    } else {
			$o configure -bound 0
		    }
		    return $o
		}
		array {
		    set o [namespace current]::[ArrayDef \#auto]
		    $o configure -element_type_def \
			    [scanIDLTypeName [lindex $typename 1]]
		    $o configure -length [lindex $typename 2]
		    return $o
		}
	    }
	    
	    error "error: internal error, I should not be here for type $typename"
	}

	proc addItem {item container scope} {
	    set type [lindex $item 0]
	    set repoid [lindex [lindex $item 1] 0]
	    set name [lindex [lindex $item 1] 1]
	    set version [lindex [lindex $item 1] 2]

	    if {$scope != "::"} {
		set absolute_name "[set scope]::$name"
	    } else {
		set absolute_name "::$name"
	    }

	    set ::Combat::SimpleTypeRepository::absnames($absolute_name) $repoid
	    
	    if {![info exists ::Combat::SimpleTypeRepository::contained($container)]} {
		set ::Combat::SimpleTypeRepository::contained($container) [list]
	    }

	    if {[info exists ::Combat::SimpleTypeRepository::elements($repoid)]} {
		set o $::Combat::SimpleTypeRepository::elements($repoid)
	    } else {
		set o ""
	    }

	    switch -- $type {
		module {
		    foreach contained [lindex $item 2] {
			addItem $contained $repoid $absolute_name
		    }
		}
		const {
		    set type [scanIDLTypeName [lindex $item 2]]
		    set value [list [$type type] [lindex $item 3]]
		    if {$o == ""} {
			set o [namespace current]::[ConstantDef \#auto $repoid $name]
			set ::Combat::SimpleTypeRepository::elements($repoid) $o
			lappend ::Combat::SimpleTypeRepository::contained($container) $repoid
		    }
		    $o configure -type_def $type -value $value
		}
		struct {
		    if {$o == ""} {
			set o [namespace current]::[StructDef \#auto $repoid $name]
			set ::Combat::SimpleTypeRepository::elements($repoid) $o
			lappend ::Combat::SimpleTypeRepository::contained($container) $repoid
		    }

		    if {[llength $item] > 2} {
			foreach contained [lindex $item 3] {
			    addItem $contained $repoid $absolute_name
			}
			set members [list]
			foreach member [lindex $item 2] {
			    set member_name [lindex $member 0]
			    set member_type \
				    [scanIDLTypeName [lindex $member 1]]
			    lappend members [list $member_name $member_type]
			}
			$o configure -members $members
		    }
		}
		exception {
		    if {$o == ""} {
			set o [namespace current]::[ExceptionDef \#auto $repoid $name]
			set ::Combat::SimpleTypeRepository::elements($repoid) $o
			lappend ::Combat::SimpleTypeRepository::contained($container) $repoid
		    }

		    foreach contained [lindex $item 3] {
			addItem $contained $repoid $absolute_name
		    }
		    set members [list]
		    foreach member [lindex $item 2] {
			set member_name [lindex $member 0]
			set member_type \
				[scanIDLTypeName [lindex $member 1]]
			lappend members [list $member_name $member_type]
		    }
		    $o configure -members $members
		}
		union {
		    if {$o == ""} {
			set o [namespace current]::[UnionDef \#auto $repoid $name]
			set ::Combat::SimpleTypeRepository::elements($repoid) $o
			lappend ::Combat::SimpleTypeRepository::contained($container) $repoid
		    }

		    if {[llength $item] > 2} {
			foreach contained [lindex $item 4] {
			    addItem $contained $repoid $absolute_name
			}

			$o configure -discriminator_type_def \
				[scanIDLTypeName [lindex $item 2]]

			set members [list]
			foreach member [lindex $item 3] {
			    set label [lindex $member 0]
			    set mname [lindex $member 1]
			    set mtype [scanIDLTypeName [lindex $member 2]]
			    lappend members [list $label mname $mtype]
			}
			$o configure -members $members
		    }
		}
		enum {
		    if {$o == ""} {
			set o [namespace current]::[EnumDef \#auto $repoid $name]
			set ::Combat::SimpleTypeRepository::elements($repoid) $o
			lappend ::Combat::SimpleTypeRepository::contained($container) $repoid
		    }
		    $o configure -members [lindex $item 2]
		}
		typedef {
		    if {$o == ""} {
			set o [namespace current]::[AliasDef \#auto $repoid $name]
			set ::Combat::SimpleTypeRepository::elements($repoid) $o
			lappend ::Combat::SimpleTypeRepository::contained($container) $repoid
		    }
		    $o configure -original_type_def \
			    [scanIDLTypeName [lindex $item 2]]
		}
		attribute {
		    set mode "ATTR_NORMAL"
		    if {[llength $item] == 4} {
			if {[lindex $item 3] == "readonly"} {
			    set mode "ATTR_READONLY"
			}
		    }

		    if {$o == ""} {
			set o [namespace current]::[AttributeDef \#auto $repoid $name]
			set ::Combat::SimpleTypeRepository::elements($repoid) $o
			lappend ::Combat::SimpleTypeRepository::contained($container) $repoid
		    }

		    $o configure -type_def [scanIDLTypeName [lindex $item 2]]
		    $o configure -mode $mode

		}
		operation {
		    #
		    # We're not entirely true here wrt the parameters
		    # and exceptions entries. We don't use a ParDescription
		    # Seq (ExcDescriptionSeq), but directly the information
		    # that we need in ORB::send_request.
		    #
		    
		    set params [list]
		    foreach par [lindex $item 3] {
			switch -- [lindex $par 0] {
			    in      { set par_mode "PARAM_IN" }
			    out     { set par_mode "PARAM_OUT" }
			    inout   { set par_mode "PARAM_INOUT" }
			    default { error "oops" }
			}
			set par_name [lindex $par 1]
			set par_type [scanIDLTypeName [lindex $par 2]]
			lappend params [list $par_mode $par_name $par_type]
		    }
		    
		    set exceptions [list]
		    foreach ex [lindex $item 4] {
			lappend exceptions [scanIDLTypeName $ex]
		    }
		    
		    set mode "OP_NORMAL"
		    if {[llength $item] == 6} {
			if {[lindex $item 5] == "oneway"} {
			    set mode "OP_ONEWAY"
			}
		    }

		    if {$o == ""} {
			set o [namespace current]::[OperationDef \#auto $repoid $name]
			set ::Combat::SimpleTypeRepository::elements($repoid) $o
			lappend ::Combat::SimpleTypeRepository::contained($container) $repoid
		    }

		    $o configure -result_def [scanIDLTypeName [lindex $item 2]]
		    $o configure -params $params
		    $o configure -mode $mode
		    $o configure -exceptions $exceptions
		}
		interface -
		abstractinterface {
		    if {$o == ""} {
			if {$type == "interface"} {
			    set o [namespace current]::[InterfaceDef \#auto $repoid $name]
			} else {
			    set o [namespace current]::[AbstractInterfaceDef \#auto $repoid $name]
			}
			set ::Combat::SimpleTypeRepository::elements($repoid) $o
			lappend ::Combat::SimpleTypeRepository::contained($container) $repoid
			set ::Combat::SimpleTypeRepository::contained($repoid) [list]
		    }

		    if {[llength $item] > 2} {
			set bases [list]
			foreach base [lindex $item 2] {
			    lappend bases [scanIDLTypeName $base]
			}
			$o configure -base_interfaces $bases
			
			foreach contained [lindex $item 3] {
			    addItem $contained $repoid $absolute_name
			}
		    }
		}
		valuetype {
		    if {$o == ""} {
			set o [namespace current]::[ValueDef \#auto $repoid $name]
			set ::Combat::SimpleTypeRepository::elements($repoid) $o
			lappend ::Combat::SimpleTypeRepository::contained($container) $repoid
			set ::Combat::SimpleTypeRepository::contained($repoid) [list]
		    }

		    if {[llength $item] > 2} {
			if {[lindex $item 2] == "0"} {
			    set base_value ""
			} else {
			    set base_value [scanIDLTypeName [lindex $item 2]]
			}

			$o configure -base_value $base_value

			set abs_base_values [list]
			foreach abase [lindex $item 3] {
			    lappend abs_base_values [scanIDLTypeName $abase]
			}

			$o configure -abstract_base_values $abs_base_values

			set supported [list]
			foreach supp [lindex $item 4] {
			    lappend supported [scanIDLTypeName $supp]
			}

			$o configure -supported_interfaces $supported

			# ignore initializers for the moment

			$o configure -initializers [list]

			$o configure -is_abstract 0
			$o configure -is_custom 0
			$o configure -is_truncatable 0

			foreach modifier [lindex $item 6] {
			    switch -- $modifier {
				custom {
				    $o configure -is_custom 1
				}
				abstract {
				    $o configure -is_abstract 1
				}
				truncatable {
				    $o configure -is_truncatable 1
				}
			    }
			}

			foreach contained [lindex $item 7] {
			    addItem $contained $repoid $absolute_name
			}
		    }
		}
		valuebox {
		    if {$o == ""} {
			set o [namespace current]::[ValueboxDef \#auto $repoid $name]
			set ::Combat::SimpleTypeRepository::elements($repoid) $o
			lappend ::Combat::SimpleTypeRepository::contained($container) $repoid
		    }

		    $o configure -original_type_def \
			    [scanIDLTypeName [lindex $item 2]]
		}
		valuemember {
		    if {$o == ""} {
			set o [namespace current]::[ValueMemberDef \#auto $repoid $name]
			set ::Combat::SimpleTypeRepository::elements($repoid) $o
			lappend ::Combat::SimpleTypeRepository::contained($container) $repoid
		    }

		    $o configure -type_def [scanIDLTypeName [lindex $item 2]]

		    if {[lindex $item 3] == "private"} {
			$o configure -access PRIVATE_MEMBER
		    } else {
			$o configure -access PUBLIC_MEMBER
		    }
		}
		native {
		    if {$o == ""} {
			set o [namespace current]::[NativeDef \#auto $repoid $name]
			set ::Combat::SimpleTypeRepository::elements($repoid) $o
			lappend ::Combat::SimpleTypeRepository::contained($container) $repoid
		    }
		}
		localinterface -
		abstractinterface {
		    # ignored
		}
		default {
		    error "error: illegal type $type while adding item $item to type repository"
		}
	    }
	    # end of switch
	    # end of proc addItem
	}

	#
	# collect all attribute and operation information for an interface
	#

	proc collectInterface {repoid {toplevel ""}} {
	    if {$toplevel == ""} {
		if {[info exists ::Combat::SimpleTypeRepository::ifcollected($repoid)]} {
		    return
		}
		set ::Combat::SimpleTypeRepository::ifcollected($repoid) 1
		set toplevel $repoid
	    }

	    foreach contained $::Combat::SimpleTypeRepository::contained($repoid) {
		set el $::Combat::SimpleTypeRepository::elements($contained)

		if {[catch {$el isa ::Combat::SimpleTypeRepository::AttributeDef} isattr]} {
		    set isattr 0
		}

		if {[catch {$el isa ::Combat::SimpleTypeRepository::OperationDef} isop]} {
		    set isop 0
		}

		if {$isattr} {
		    set name [$el cget -name]
		    set desc [$el describe]
		    set ::Combat::SimpleTypeRepository::attributes($toplevel,$name) $desc
		} elseif {$isop} {
		    set name [$el cget -name]
		    set desc [$el describe]
		    set ::Combat::SimpleTypeRepository::operations($toplevel,$name) $desc
		}
	    }

	    set i $::Combat::SimpleTypeRepository::elements($repoid)

	    foreach base [$i cget -base_interfaces] {
		collectInterface [$base cget -id] $toplevel
	    }
	}

	#
	# Add data obtained from idl2tcl
	#

	proc add {data} {
	    array set ::Combat::SimpleTypeRepository::ifcollected {}
	    array set ::Combat::SimpleTypeRepository::attributes {}
	    array set ::Combat::SimpleTypeRepository::operations {}

	    foreach item $data {
		addItem $item "::" "::"
	    }
	}

	#
	# Check if we have type information in our repository, but don't
	# download it yet.
	#

	proc HaveTypeInfoForId {repoid} {
	    if {[info exists ::Combat::SimpleTypeRepository::elements($repoid)]} {
		if {![info exists ::Combat::SimpleTypeRepository::ifcollected($repoid)]} {
		    collectInterface $repoid
		}
		return 1
	    }

	    if {[info exists ::Combat::SimpleTypeRepository::downloaded($repoid)]} {
		return 1
	    }

	    return 0
	}

	#
	# Download a FullInterfaceDescription from the IFR
	#

	proc UpdateTypeInfoForObj {obj {force 0}} {
	    #
	    # Check if this object has a Repository Id
	    #

	    set repoid [$obj cget -type_id]

	    if {$force || $repoid == ""} {
		catch {$obj UpdateType}
		set repoid [$obj cget -type_id]
	    }

	    #
	    # Do we have a Repository Id?
	    #

	    if {$repoid != "" && !$force} {
		if {[HaveTypeInfoForId $repoid]} {
		    return 1
		}
	    }

	    #
	    # Try to download info for this Repository Id from the
	    # Interface Repository
	    #

	    if {$repoid != "" && !$force} {
		if {[UpdateTypeInfoForId $repoid]} {
		    return 1
		}
	    }

	    #
	    # Either we don't have a Repository Id, we don't have
	    # type information for that Repository Id, or we want to
	    # force an update of our type information. Ask the object
	    # about its interface.
	    #

	    #
	    # Does the object support reflection?
	    #

	    if {![catch {
		set efidany [::Combat::CORBA::ORB::invoke_sync 1 $obj \
			omg_get_ifr_metadata any 0 {} {}]
	    }]} {
		set efid [lindex $efidany 1]
		set repoid [AddInterfaceInfoFromEfid $efid]
		$obj configure -type_id $repoid
		::corba::release any $efidany
		return 1
	    }

	    #
	    # Does _get_interface work?
	    #

	    if {![catch {
		set iface [$obj _get_interface]
	    }]} {
		set repoid [DownloadFromIFR $iface]
		::corba::release $iface
		if {$repoid != 0} {
		    $obj configure -type_id $repoid
		    return 1
		}
	    }

	    #
	    # We are running out of options. See if we have any
	    # type information available.
	    #

	    if {$repoid == ""} {
		return 0
	    }

	    return [HaveTypeInfoForId $repoid]
	}

	proc UpdateTypeInfoForId {repoid} {
	    #
	    # (1) Is type data available in our own repository ?
	    #

	    if {[HaveTypeInfoForId $repoid]} {
		return 1
	    }

	    #
	    # (2) Contact Interface Repository and look for repoid
	    #

	    if {[catch {
		set ifr [::corba::resolve_initial_references InterfaceRepository]
	    }]} {
		return 0
	    }

	    set ifrobj [::Combat::CORBA::ORB::GetObjFromHandle $ifr]

	    if {[catch {
		set iface [::Combat::CORBA::ORB::invoke_sync 1 $ifrobj \
			lookup_id Object 0 {{in string}} [list $repoid]]
	    }]} {
		::corba::release $ifr
		return 0
	    }

	    set result [DownloadFromIFR $iface]
	    ::corba::release $iface
	    ::corba::release $ifr
	    return $result
	}

	proc DownloadFromIFR {iface} {
	    #
	    # Okay, download its FullInterfaceDescription
	    #

	    set ifaceobj [::Combat::CORBA::ORB::GetObjFromHandle $iface]

	    if {![catch {
		set efid [::Combat::CORBA::ORB::invoke_sync 1 $ifaceobj \
			describe_ext_interface $::Combat::SimpleTypeRepository::efid_tc 0 {} {}]
	    } res]} {
		return [AddInterfaceInfoFromEfid $efid]
	    }

	    if {![catch {
		set fid [::Combat::CORBA::ORB::invoke_sync 1 $ifaceobj \
			describe_interface $::Combat::SimpleTypeRepository::fid_tc 0 {} {}]
	    } res]} {
		return [AddInterfaceInfoFromFid $fid]
	    }

	    return 0
	}

	proc AddInterfaceInfoFromFid {fid} {
	    #
	    # The FullInterfaceDescription is the same as ExtFullInterface-
	    # Description but for the attributes. An ExtAttrDescription has
	    # two new members, get_exceptions and set_exceptions. We're taking
	    # care of that difference elsewhere.
	    #

	    return [AddInterfaceInfoFromEfid $fid]
	}

	proc AddInterfaceInfoFromEfid {efid} {
	    #
	    # add operation and attribute info
	    #

	    set repoid [lindex $efid 3]

	    foreach op [lindex $efid 9] {
		set opname [lindex $op 1]
		set ::Combat::SimpleTypeRepository::downops($repoid,$opname) $op
	    }

	    foreach at [lindex $efid 11] {
		set atname [lindex $at 1]
		set ::Combat::SimpleTypeRepository::downattrs($repoid,$atname) $at
	    }

	    #
	    # fine.
	    #

	    set ::Combat::SimpleTypeRepository::downloaded($repoid) 1
	    return $repoid
	}

	#
	# get info
	#

	proc _is_a {repoid type} {
	    if {[catch {
		set el $::Combat::SimpleTypeRepository::elements($repoid)
		set res [$el _is_a $type]
	    }]} {
		return 0
	    }
	    return $res
	}

	proc getTypeOf {repoidorscopedname} {
	    if {[info exists ::Combat::SimpleTypeRepository::absnames($repoidorscopedname)]} {
		set repoid $::Combat::SimpleTypeRepository::absnames($repoidorscopedname)
	    } elseif {[info exists ::Combat::SimpleTypeRepository::absnames(::$repoidorscopedname)]} {
		set repoid $::Combat::SimpleTypeRepository::absnames(::$repoidorscopedname)
	    } else {
		set repoid $repoidorscopedname
	    }
	    if {[catch {
		set el $::Combat::SimpleTypeRepository::elements($repoid)
		set type [$el type]
	    }]} {
		error "error: no such type in IFR: $repoidorscopedname"
	    }
	    return $type
	}
	
	proc getConst {repoidorscopedname} {
	    if {[info exists ::Combat::SimpleTypeRepository::absnames($repoidorscopedname)]} {
		set repoid $::Combat::SimpleTypeRepository::absnames($repoidorscopedname)
	    } elseif {[info exists ::Combat::SimpleTypeRepository::absnames(::$repoidorscopedname)]} {
		set repoid $::Combat::SimpleTypeRepository::absnames(::$repoidorscopedname)
	    } else {
		set repoid $repoidorscopedname
	    }
	    if {[catch {
		set el $::Combat::SimpleTypeRepository::elements($repoid)
		$el isa ::Combat::SimpleTypeRepository::ConstantDef
	    }]} {
		error "error: no such type in IFR: $repoidorscopedname"
	    }
	    return [$el cget -value]
	}
	
	proc getOp {iface opname} {
	    if {[info exists ::Combat::SimpleTypeRepository::operations($iface,$opname)]} {
		return $::Combat::SimpleTypeRepository::operations($iface,$opname)
	    }
	    if {[info exists ::Combat::SimpleTypeRepository::downops($iface,$opname)]} {
		return $::Combat::SimpleTypeRepository::downops($iface,$opname)
	    }
	    return ""
	}
	
	proc getAttr {iface attrname} {
	    if {[info exists ::Combat::SimpleTypeRepository::attributes($iface,$attrname)]} {
		return $::Combat::SimpleTypeRepository::attributes($iface,$attrname)
	    }
	    if {[info exists ::Combat::SimpleTypeRepository::downattrs($iface,$attrname)]} {
		return $::Combat::SimpleTypeRepository::downattrs($iface,$attrname)
	    }
	    return ""
	}

	#
	# Return an Any containing an ExtFullInterfaceDescription for a repoid
	#

	proc omgGetIfrMetadata {iface} {
	    if {![info exists ::Combat::SimpleTypeRepository::elements($iface)]} {
		error "error: no such type in IFR: $iface"
	    }

	    set el $::Combat::SimpleTypeRepository::elements($iface)
	    $el isa ::Combat::SimpleTypeRepository::InterfaceDef
	    return [list $::Combat::SimpleTypeRepository::efid_tc \
		    [$el describe_ext_interface]]
	}

	#
	# Get the repository id from a type that can be specified as a
	# repository id or a scoped name.
	#

	proc getRepoid {repoidorscopedname} {
	    if {[string range $repoidorscopedname 0 3] == "IDL:"} {
		return $repoidorscopedname
	    } elseif {[string range $repoidorscopedname 0 1] == "::"} {
		return $::Combat::SimpleTypeRepository::absnames($repoidorscopedname)
	    } elseif {[info exists ::Combat::SimpleTypeRepository::absnames(::$repoidorscopedname)]} {
		return $::Combat::SimpleTypeRepository::absnames(::$repoidorscopedname)
	    } else {
		error "unknown type \"$repoidorscopedname\""
	    }
	}

	#
	# Check if two types match, when each type can be specified as
	# a scoped name or repoid.
	#

	proc matchTypes {t1 t2} {
	    return [string equal [getRepoid $t1] [getRepoid $t2]]
	}

    }
}

