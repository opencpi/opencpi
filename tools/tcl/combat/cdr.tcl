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
# CVS Version Tag: $Id: cdr.tcl,v 1.24 2008/11/14 02:06:58 Owner Exp $
#
# ----------------------------------------------------------------------
# procs to read and write CDR data
# ----------------------------------------------------------------------
#

namespace eval Combat {
    namespace eval CDR {
	#
	# Simple ReadBuffer for primitive types
	#

	itcl::class ReadBuffer {
	    private variable data
	    private variable index
	    private variable size
	    private variable encaps
	    private variable endofchunk
	    public variable byteorder 	;# 0 big-endian, 1 little-endian
	    public variable cdecoder	;# Char Codeset Decoder
	    public variable wdecoder	;# WChar Codeset Decoder

	    #
	    # byteorder can be 0 for big-endian or 1 for little-endian
	    #

	    constructor {_data} {
		set data $_data
		set index 0
		set size [::string length $data]
		set encaps [list]
		set cdecoder ""
		set wdecoder ""
		set endofchunk -1

		global tcl_platform
		if {$tcl_platform(byteOrder) == "bigEndian"} {
		    set byteorder 0
		} else {
		    set byteorder 1
		}
	    }

	    public method get_data {} {
		return $data
	    }

            public method tell {} {
                return $index
            }

            public method seek {pos} {
                set index $pos
            }

	    public method align {alignment} {
		if {($index % $alignment) != 0} {
		    set increment [expr {$alignment - ($index % $alignment)}]
		    incr index $increment
		}
		check_chunk
	    }

	    public method begin_encaps {} {
		set encaps_length [ulong]
		lappend encaps [list [tell] $byteorder $encaps_length]
		set byteorder [boolean]
		return $encaps_length
	    }

	    public method end_encaps {} {
		set ep [llength $encaps]
		if {$ep <= 0} {
		    error "no encapsulation to end here"
		}
		incr ep -1
		set edata [lindex $encaps $ep]
		incr ep -1
		set encaps [lrange $encaps 0 $ep]

		set byteorder [lindex $edata 1]
		set index [expr {[lindex $edata 0] + [lindex $edata 2]}]
	    }

	    public method begin_chunk {} {
		set encaps_length [ulong]
		set endofchunk [expr {[tell] + $encaps_length}]
	    }

	    public method end_chunk {} {
		if {$endofchunk != -1} {
		    set index $endofchunk
		    set endofchunk -1
		}
	    }

	    public method check_chunk {} {
		if {$endofchunk != -1 && $index >= $endofchunk} {
		    end_chunk
		    begin_chunk
		}
	    }

	    public method char {} {
		check_chunk
		if {$cdecoder != ""} {
		    set res [$cdecoder get_char $this]
		} else {
		    if {[binary scan $data @[set index]a res] != 1} {
			error "[$this info class]::char"
		    }
		    set res [encoding convertfrom iso8859-1 $res]
		    incr index
		}
		return $res
	    }

	    public method chars {length} {
		check_chunk
		if {$cdecoder != ""} {
		    set res [$cdecoder get_chars $this $length]
		} else {
		    if {[binary scan $data @[set index]a[set length] res] != 1} {
			error "[$this info class]::chars"
		    }
		    set res [encoding convertfrom iso8859-1 $res]
		    incr index $length
		    return $res
		}
		return $res
	    }

	    public method wchar {} {
		check_chunk
		if {$wdecoder == ""} {
		    error "no wchar decoder in [$this info class]::wchar"
		}
		return [$wdecoder get_wchar $this]
	    }

	    public method wchars {length} {
		check_chunk
		if {$wdecoder == ""} {
		    error "no wchar decoder in [$this info class]::wchars"
		}
		return [$wdecoder get_wchars $this]
	    }

	    public method octet {} {
		check_chunk
		if {[binary scan $data @[set index]a res] != 1} {
		    error "[$this info class]::octet"
		}
		incr index
		return $res
	    }

	    public method octets {length} {
		check_chunk
		if {[binary scan $data @[set index]a[set length] res] != 1} {
		    error "[$this info class]::octets"
		}
		incr index $length
		return $res
	    }
	    
	    public method short {} {
		align 2

		if {$byteorder == 0} {
		    set code [binary scan $data @[set index]S res]
		} else {
		    set code [binary scan $data @[set index]s res]
		}
		
		if {$code != 1} {
		    error "[$this info class]::short"
		}
		
		incr index 2
		return $res
	    }
	    
	    public method ushort {} {
		return [expr {([short] + 0x10000) % 0x10000}]
	    }
	    
	    public method long {} {
		align 4
		check_chunk
		
		if {$byteorder == 0} {
		    set code [binary scan $data @[set index]I res]
		} else {
		    set code [binary scan $data @[set index]i res]
		}
		
		if {$code != 1} {
		    error "[$this info class]::long"
		}
		
		incr index 4
		return $res
	    }
	    
	    #
	    # In Tcl 8.4, with its 64 bit integer support, we can use
	    # the same trick as above (see ushort) to get an unsigned
	    # value. Before 8.4, we just have to hope that the value
	    # is not misinterpreted. (You might want to throw an error
	    # if the value is negative, but then, some people insist
	    # on sending an unsigned -1 for signaling purposes.)
	    #
	    
	    public method ulong {} {
		set res [long]

		if {[info tclversion] > 8.3} {
		    if {$res < 0} {
			set res [expr {4294967295 + $res}]
			incr res
		    }
		}

		return $res
	    }
	    
	    #
	    # In Tcl 8.4, we can use the 64 bit support. In older versions,
	    # we complain about off-range values.
	    #

	    public method longlong {} {
		align 8
		check_chunk

		if {[info tclversion] > 8.3} {
		    if {$byteorder == 0} {
			set code [binary scan $data @[set index]W res]
		    } else {
			set code [binary scan $data @[set index]w res]
		    }
		
		    if {$code != 1} {
			error "[$this info class]::longlong"
		    }
		} else {
		    if {$byteorder == 0} {
			set high [long]
			set res [long]
		    } else {
			set res [long]
			set high [long]
		    }

		    if {($high != 0 && $high != -1) ||
			($high == 0 && $res < 0) ||
			($high == -1 && $res > 0)} {
			error "[$this info class]::longlong: value out of range"
		    }
		}

		incr index 8
		return $res
	    }
	    
	    public method ulonglong {} {
		return [longlong]
	    }
	    
	    #
	    # groan ... Tcl only handles native floating point values
	    #
	    
	    public method float {} {
		align 4

		if {$byteorder == 0} {
		    set code [binary scan $data @[set index]cccc se1 e2f1 f2 f3]
		} else {
		    set code [binary scan $data @[set index]cccc f3 f2 e2f1 se1]
		}
		
		if {$code != 4} {
		    error "[$this info class]::float"
		}
		
		set se1  [expr {($se1 + 0x100) % 0x100}]
		set e2f1 [expr {($e2f1 + 0x100) % 0x100}]
		set f2   [expr {($f2 + 0x100) % 0x100}]
		set f3   [expr {($f3 + 0x100) % 0x100}]
		
		set sign [expr {$se1 >> 7}]
		set exponent [expr {(($se1 & 0x7f) << 1 | ($e2f1 >> 7))}]
		set f1 [expr {$e2f1 & 0x7f}]

		if {$exponent == 0} {
		    set res 0.0
		} else {
		    set fraction [expr {double($f1)*0.0078125 + \
			    double($f2)*3.0517578125e-05 + \
			    double($f3)*1.19209289550781e-07}]
		
		    set res [expr {($sign ? -1. : 1.) * \
			    pow(2.,double($exponent-127)) * \
			    (1. + $fraction)}]
		}
		
		incr index 4
		return $res
	    }

	    public method double {} {
		align 8
		
		if {$byteorder == 0} {
		    set code [binary scan $data @[set index]cccccccc \
			    se1 e2f1 f2 f3 f4 f5 f6 f7]
		} else {
		    set code [binary scan $data @[set index]cccccccc \
			    f7 f6 f5 f4 f3 f2 e2f1 se1]
		}
		
		if {$code != 8} {
		    error "[$this info class]::double"
		}
		
		set se1  [expr {($se1 + 0x100) % 0x100}]
		set e2f1 [expr {($e2f1 + 0x100) % 0x100}]
		set f2   [expr {($f2 + 0x100) % 0x100}]
		set f3   [expr {($f3 + 0x100) % 0x100}]
		set f4   [expr {($f4 + 0x100) % 0x100}]
		set f5   [expr {($f5 + 0x100) % 0x100}]
		set f6   [expr {($f6 + 0x100) % 0x100}]
		set f7   [expr {($f7 + 0x100) % 0x100}]
		
		set sign [expr {$se1 >> 7}]
		set exponent [expr {(($se1 & 0x7f) << 4 | ($e2f1 >> 4))}]
		set f1 [expr {$e2f1 & 0x0f}]

		if {$exponent == 0} {
		    set res 0.0
		} else {
		    set fraction [expr {double($f1)*0.0625 + \
			    double($f2)*0.000244140625 + \
			    double($f3)*9.5367431640625e-07 + \
			    double($f4)*3.7252902984619141e-09 + \
			    double($f5)*1.4551915228366852e-11 + \
			    double($f6)*5.6843418860808015e-14 + \
			    double($f7)*2.2204460492503131e-16}]
		
		    set res [expr {($sign ? -1. : 1.) * \
			    pow(2.,double($exponent-1023)) * \
			    (1. + $fraction)}]
		}
		
		incr index 8
		return $res
	    }

	    #
	    # I don't think this is getting any more precise ...
	    #

	    public method longdouble {} {
		align 8
		
		if {$byteorder == 0} {
		    set code [binary scan $data @[set index]cccccccccccccccc \
				  se1 e2 f1 f2 f3 f4 f5 f6 f7 f8 f9 f10 f11 f12 f13 f14]
		} else {
		    set code [binary scan $data @[set index]cccccccccccccccc \
				  f14 f13 f12 f11 f10 f9 f8 f7 f6 f5 f4 f3 f2 f1 e2 se1]
		}
		
		if {$code != 16} {
		    error "[$this info class]::longdouble"
		}
		
		set se1  [expr {($se1 + 0x100) % 0x100}]
		set e2   [expr {($e2 + 0x100) % 0x100}]
		set f1   [expr {($f1 + 0x100) % 0x100}]
		set f2   [expr {($f2 + 0x100) % 0x100}]
		set f3   [expr {($f3 + 0x100) % 0x100}]
		set f4   [expr {($f4 + 0x100) % 0x100}]
		set f5   [expr {($f5 + 0x100) % 0x100}]
		set f6   [expr {($f6 + 0x100) % 0x100}]
		set f7   [expr {($f7 + 0x100) % 0x100}]
		set f8   [expr {($f8 + 0x100) % 0x100}]
		set f9   [expr {($f9 + 0x100) % 0x100}]
		set f10  [expr {($f10 + 0x100) % 0x100}]
		set f11  [expr {($f11 + 0x100) % 0x100}]
		set f12  [expr {($f12 + 0x100) % 0x100}]
		set f13  [expr {($f13 + 0x100) % 0x100}]
		set f14  [expr {($f14 + 0x100) % 0x100}]
		
		set sign [expr {$se1 >> 7}]
		set exponent [expr {(($se1 & 0x7f) << 8 | $e2)}]

		if {$exponent == 0} {
		    set res 0.0
		} else {
		    set fraction [expr {double($f1)*0.00390625 + \
			    double($f2)*1.52587890625e-005 + \
			    double($f3)*5.96046447754e-008 + \
			    double($f4)*2.32830643654e-010 + \
			    double($f5)*9.09494701773e-013 + \
			    double($f6)*3.5527136788e-015 + \
			    double($f7)*1.38777878078e-017 + \
			    double($f8)*5.42101086243e-020 + \
			    double($f9)*2.11758236814e-022 + \
			    double($f10)*8.27180612553e-025 + \
			    double($f11)*3.23117426779e-027 + \
			    double($f12)*1.26217744835e-029 + \
			    double($f13)*4.93038065763e-032 + \
			    double($f14)*1.92592994439e-034}]
		
		    set res [expr {($sign ? -1. : 1.) * \
			    pow(2.,double($exponent-16383)) * \
			    (1. + $fraction)}]
		}
		
		incr index 16
		return $res
	    }

	    public method boolean {} {
		check_chunk
		if {[binary scan $data @[set index]c res] != 1} {
		    error "[$this info class]::boolean"
		}
		if {$res != 0 && $res != 1} {
		    error "illegal boolean value $res"
		}
		incr index
		return $res
	    }
	    
	    public method string {} {
		if {$cdecoder != ""} {
		    check_chunk
		    set res [$cdecoder get_string $this]
		} else {
		    #
		    # backwards compatibility using iso8859-1
		    #

		    #
		    # length includes the trailing null, which we don't want
		    #
		
		    set length [ulong]
		    set dlength [expr {$length - 1}]
		    set code [binary scan $data @[set index]a[set dlength] res]
		
		    if {$code != 1} {
			error "[$this info class]::string"
		    }

		    set res [encoding convertfrom iso8859-1 $res]
		    incr index $length
		}
		return $res
	    }

	    public method wstring {} {
		if {$wdecoder == ""} {
		    error "no wchar decoder in [$this info class]::wstring"
		}
		check_chunk
		return [$wdecoder get_wstring $this]
	    }
	}

	#
	# Simple WriteBuffer for primitive types
	#

	itcl::class WriteBuffer {
	    private variable data
	    private variable encaps
	    private variable chunk
	    public variable byteorder 	;# 0 big-endian, 1 little-endian
	    public variable cencoder	;# Char Codeset Encoder
	    public variable wencoder	;# WChar Codeset Encoder

	    constructor {} {
		set data ""
		set encaps [list]
		set cencoder ""
		set wencoder ""

		global tcl_platform
		if {$tcl_platform(byteOrder) == "bigEndian"} {
		    set byteorder 0
		} else {
		    set byteorder 1
		}
	    }

	    public method get_data {} {
		return $data
	    }

	    public method tell {} {
		return [::string length $data]
	    }

	    public method align {alignment} {
		set index [::string length $data]
		if {($index % $alignment) != 0} {
		    set increment [expr {$alignment - ($index % $alignment)}]
		    switch $increment {
			1 { append data f }
			2 { append data fo }
			3 { append data foo }
			4 { append data foob }
			5 { append data fooba }
			6 { append data foobar }
			7 { append data foobar! }
			default {
			    error "[$this info class]::align"
			}
		    }
		}
	    }

	    public method begin_encaps {{nbo {}}} {
		align 4
		ulong 0				;# dummy length
		lappend encaps [list [tell] $byteorder]
		if {$nbo == ""} {
		    boolean $byteorder
		} else {
		    set byteorder $nbo
		    boolean $byteorder
		}
	    }

	    public method end_encaps {} {
		set ep [llength $encaps]
		if {$ep <= 0} {
		    error "no encapsulation to end here"
		}
		incr ep -1
		set edata [lindex $encaps $ep]
		incr ep -1
		set encaps [lrange $encaps 0 $ep]

		set encaps_begin [lindex $edata 0]
		set byteorder [lindex $edata 1]
		set encaps_length [expr {[tell] - $encaps_begin}]

		if {$byteorder == 0} {
		    set data [::string replace $data \
			    [expr {$encaps_begin - 4}] \
			    [expr {$encaps_begin - 1}] \
			    [binary format I $encaps_length]]
		} else {
		    set data [::string replace $data \
			    [expr {$encaps_begin - 4}] \
			    [expr {$encaps_begin - 1}] \
			    [binary format i $encaps_length]]
		}
	    }

	    public method begin_chunk {} {
		align 4
		ulong 0				;# dummy length
		set chunk [tell]
	    }

	    public method end_chunk {} {
		set encaps_length [expr {[tell] - $chunk}]

		if {$byteorder == 0} {
		    set data [::string replace $data \
			    [expr {$chunk - 4}] [expr {$chunk - 1}] \
			    [binary format I $encaps_length]]
		} else {
		    set data [::string replace $data \
			    [expr {$chunk - 4}] [expr {$chunk - 1}] \
			    [binary format i $encaps_length]]
		}
	    }

	    public method char {val} {
		if {$cencoder != ""} {
		    $cencoder put_char $this $val
		} else {
		    set ival [encoding convertto iso8859-1 $val]
		    if {[::string length $ival] != 1} {
			error "[$this info class]::char"
		    }
		    append data $ival
		}
	    }

	    public method chars {val} {
		if {$cencoder != ""} {
		    $cencoder put_chars $this $val
		} else {
		    append data [encoding convertto iso8859-1 $val]
		}
	    }

	    public method wchar {val} {
		if {$wencoder == ""} {
		    error "no wchar encoder in [$this info class]::wchar"
		}
		$wencoder put_wchar $this $val
	    }

	    public method wchars {val} {
		if {$wencoder == ""} {
		    error "no wchar encoder in [$this info class]::wchars"
		}
		$wencoder put_wchars $this $val
	    }

	    public method octet {val} {
		if {[::string length $val] != 1} {
		    error "[$this info class]::octet"
		}
		append data $val
	    }

	    public method octets {val} {
		append data $val
	    }

	    public method short {val} {
		align 2

		if {$byteorder == 0} {
		    set bytes [binary format S $val]
		} else {
		    set bytes [binary format s $val]
		}

		append data $bytes
	    }

	    public method ushort {val} {
		short $val
	    }

	    public method long {val} {
		align 4

		if {$byteorder == 0} {
		    set bytes [binary format I $val]
		} else {
		    set bytes [binary format i $val]
		}

		append data $bytes
	    }

	    public method ulong {val} {
		long $val
	    }

	    public method longlong {val} {
		align 8

		if {[info tclversion] > 8.3} {
		    if {$byteorder == 0} {
			set bytes [binary format W $val]
		    } else {
			set bytes [binary format w $val]
		    }
		    append data $bytes
		} else {
		    if {$byteorder == 0} {
			if {$val >= 0} {
			    long 0
			    long $val
			} else {
			    long -1
			    long $val
			}
		    } else {
			if {$val >= 0} {
			    long $val
			    long 0
			} else {
			    long $val
			    long -1
			}
		    }
		}
	    }

	    #
	    # taking our luck with encoding -1 ...
	    #

	    public method ulonglong {val} {
		longlong $val
	    }

	    #
	    # groan ... Tcl only handles native floating point values
	    #
	    
	    public method float {val} {
		align 4

		if {$val > 0} {
		    set sign 0
		} else {
		    set sign 1
		    set val [expr {-1. * $val}]
		}

		#
		# If the following math fails, then it's because of the
		# logarithm. That means that val is indistinguishable from
		# zero
		#

		if {[catch {
		    set exponent [expr {int(floor(log($val)/0.69314718055994529))+127}]
		    set fraction [expr {($val/pow(2.,double($exponent-127)))-1.}]
		}]} {
		    set exponent 0
		    set fraction 0.0
		} else {
		    #
		    # round off too-small values to zero, throw error for
		    # too-large values
		    #

		    if {$exponent < 0} {
			set exponent 0
			set fraction 0.0
		    } elseif {$exponent > 255} {
			error "value $val outside legal range for a float"
		    }
		}

		set fraction [expr {$fraction * 128.}]
		set f1f      [expr {floor($fraction)}]
		set fraction [expr {($fraction - $f1f) * 256.}]
		set f2f      [expr {floor($fraction)}]
		set fraction [expr {($fraction - $f2f) * 256.}]
		set f3f      [expr {floor($fraction)}]

		set f1       [expr {int($f1f)}]
		set f2       [expr {int($f2f)}]
		set f3       [expr {int($f3f)}]

		set se1      [expr {($sign ? 128 : 0) | ($exponent >> 1)}]
		set e2f1     [expr {(($exponent & 0x1) << 7) | $f1}]

		if {$byteorder == 0} {
		    set bytes [binary format cccc $se1 $e2f1 $f2 $f3]
		} else {
		    set bytes [binary format cccc $f3 $f2 $e2f1 $se1]
		}

		append data $bytes
	    }

	    public method double {val} {
		align 8

		if {$val > 0} {
		    set sign 0
		} else {
		    set sign 1
		    set val [expr {-1. * $val}]
		}

		#
		# If the following math fails, then it's because of the
		# logarithm. That means that val is indistinguishable from
		# zero
		#

		if {[catch {
		    set exponent [expr {int(floor(log($val)/0.69314718055994529))+1023}]
		    set fraction [expr {($val/pow(2.,double($exponent-1023)))-1.}]
		}]} {
		    set exponent 0
		    set fraction 0.0
		} else {
		    #
		    # round off too-small values to zero, throw error for
		    # too-large values
		    #

		    if {$exponent < 0} {
			set exponent 0
			set fraction 0.0
		    } elseif {$exponent > 2047} {
			error "value $val outside legal range for a double"
		    }
		}

		set fraction [expr {$fraction * 16.}]
		set f1f      [expr {floor($fraction)}]
		set fraction [expr {($fraction - $f1f) * 256.}]
		set f2f      [expr {floor($fraction)}]
		set fraction [expr {($fraction - $f2f) * 256.}]
		set f3f      [expr {floor($fraction)}]
		set fraction [expr {($fraction - $f3f) * 256.}]
		set f4f      [expr {floor($fraction)}]
		set fraction [expr {($fraction - $f4f) * 256.}]
		set f5f      [expr {floor($fraction)}]
		set fraction [expr {($fraction - $f5f) * 256.}]
		set f6f      [expr {floor($fraction)}]
		set fraction [expr {($fraction - $f6f) * 256.}]
		set f7f      [expr {floor($fraction)}]

		set f1       [expr {int($f1f)}]
		set f2       [expr {int($f2f)}]
		set f3       [expr {int($f3f)}]
		set f4       [expr {int($f4f)}]
		set f5       [expr {int($f5f)}]
		set f6       [expr {int($f6f)}]
		set f7       [expr {int($f7f)}]

		set se1      [expr {($sign ? 128 : 0) | ($exponent >> 4)}]
		set e2f1     [expr {(($exponent & 0x0f) << 4) | $f1}]

		if {$byteorder == 0} {
		    set bytes [binary format cccccccc \
			    $se1 $e2f1 $f2 $f3 $f4 $f5 $f6 $f7]
		} else {
		    set bytes [binary format cccccccc \
			    $f7 $f6 $f5 $f4 $f3 $f2 $e2f1 $se1]
		}

		append data $bytes
	    }

	    public method longdouble {val} {
		align 8

		if {$val > 0} {
		    set sign 0
		} else {
		    set sign 1
		    set val [expr {-1. * $val}]
		}

		#
		# If the following math fails, then it's because of the
		# logarithm. That means that val is indistinguishable from
		# zero
		#

		if {[catch {
		    set exponent [expr {int(floor(log($val)/0.69314718055994529))+16383}]
		    set fraction [expr {($val/pow(2.,double($exponent-16383)))-1.}]
		}]} {
		    set exponent 0
		    set fraction 0.0
		} else {
		    #
		    # round off too-small values to zero, throw error for
		    # too-large values
		    #

		    if {$exponent < 0} {
			set exponent 0
			set fraction 0.0
		    } elseif {$exponent > 32767} {
			error "value $val outside legal range for a longdouble"
		    }
		}

		set fraction [expr {$fraction * 256.}]
		set f1f      [expr {floor($fraction)}]
		set fraction [expr {($fraction - $f1f) * 256.}]
		set f2f      [expr {floor($fraction)}]
		set fraction [expr {($fraction - $f2f) * 256.}]
		set f3f      [expr {floor($fraction)}]
		set fraction [expr {($fraction - $f3f) * 256.}]
		set f4f      [expr {floor($fraction)}]
		set fraction [expr {($fraction - $f4f) * 256.}]
		set f5f      [expr {floor($fraction)}]
		set fraction [expr {($fraction - $f5f) * 256.}]
		set f6f      [expr {floor($fraction)}]
		set fraction [expr {($fraction - $f6f) * 256.}]
		set f7f      [expr {floor($fraction)}]
		set fraction [expr {($fraction - $f7f) * 256.}]
		set f8f      [expr {floor($fraction)}]
		set fraction [expr {($fraction - $f8f) * 256.}]
		set f9f      [expr {floor($fraction)}]
		set fraction [expr {($fraction - $f9f) * 256.}]
		set f10f     [expr {floor($fraction)}]
		set fraction [expr {($fraction - $f10f) * 256.}]
		set f11f     [expr {floor($fraction)}]
		set fraction [expr {($fraction - $f11f) * 256.}]
		set f12f     [expr {floor($fraction)}]
		set fraction [expr {($fraction - $f12f) * 256.}]
		set f13f     [expr {floor($fraction)}]
		set fraction [expr {($fraction - $f13f) * 256.}]
		set f14f     [expr {floor($fraction)}]

		set f1       [expr {int($f1f)}]
		set f2       [expr {int($f2f)}]
		set f3       [expr {int($f3f)}]
		set f4       [expr {int($f4f)}]
		set f5       [expr {int($f5f)}]
		set f6       [expr {int($f6f)}]
		set f7       [expr {int($f7f)}]
		set f8       [expr {int($f8f)}]
		set f9       [expr {int($f9f)}]
		set f10      [expr {int($f10f)}]
		set f11      [expr {int($f11f)}]
		set f12      [expr {int($f12f)}]
		set f13      [expr {int($f13f)}]
		set f14      [expr {int($f14f)}]

		set se1      [expr {($sign ? 128 : 0) | ($exponent >> 8)}]
		set e2       [expr {$exponent & 0xff}]

		if {$byteorder == 0} {
		    set bytes [binary format cccccccccccccccc \
				   $se1 $e2 $f1 $f2 $f3 $f4 $f5 $f6 $f7 $f8 $f9 $f10 $f11 $f12 $f13 $f14]
		} else {
		    set bytes [binary format cccccccccccccccc \
				   $f14 $f13 $f12 $f11 $f10 $f9 $f8 $f7 $f6 $f5 $f4 $f3 $f2 $f1 $e2 $se1]
		}

		append data $bytes
	    }

	    public method boolean {val} {
		if {[catch {
		    if {$val} {
			append data \1
		    } else {
			append data \0
		    }
		}]} {
		    error "illegal boolean value $val"
		}
	    }

	    public method string {val} {
		if {$cencoder != ""} {
		    $cencoder put_string $this $val
		} else {
		    #
		    # backwards compatibility using iso8859-1
		    #

		    set ival [encoding convertto iso8859-1 $val]
		    set length [::string length $ival]
		    incr length ;# terminating null
		    ulong $length
		    append data $ival \0
		}
	    }

	    public method wstring {val} {
		if {$wencoder == ""} {
		    error "no wchar encoder in [$this info class]::wstring"
		}
		$wencoder put_wstring $this $val
	    }
	}

	#
	# ------------------------------------------------------------
	# CDR Decoder and Encoder for complex types
	# ------------------------------------------------------------
	#

	itcl::class Decoder {
	    private variable buffer
	    private variable tcrecursion
	    private variable marshalrecursion
	    private variable valuerecursion
	    private variable nestinglevel
	    private variable chunking

	    constructor {_data} {
		set buffer [namespace current]::[::Combat::CDR::ReadBuffer \#auto $_data]
		set nestinglevel 0
		set chunking 0
	    }

	    destructor {
		itcl::delete object $buffer
	    }

	    public method get_buffer {} {
		return $buffer
	    }

	    public method get_data {} {
		return [$buffer get_data]
	    }


	    private method get_indirect_string {} {
		set pos [$buffer tell]
		set ref [$buffer long]
		if {$ref != -1} {
		    $buffer seek $pos
		    return [$buffer string]
		}
		set oldpos [$buffer tell]
		set offset [$buffer long]
		$buffer seek [expr {$oldpos + $offset}]
		set res [$buffer string]
		$buffer seek $oldpos
		$buffer long
		return $res
	    }

	    private method get_indirect_string_seq {} {
		set pos [$buffer tell]
		set ref [$buffer long]
		set res [list]
		if {$ref != -1} {
		    for {set i 0} {$i < $ref} {incr i} {
			lappend res [get_indirect_string]
		    }
		    return $res
		}
		set oldpos [$buffer tell]
		set offset [$buffer long]
		$buffer seek [expr {$oldpos + $offset}]
		set ref [$buffer ulong]
		for {set i 0} {$i < $ref} {incr i} {
		    lappend res [get_indirect_string]
		}
		$buffer seek $oldpos
		$buffer long
		return $res
	    }

	    private method TypeCode {} {
		set kind [$buffer long]

		#
		# Handle recursion and indirection
		#
		# When entering a potentially recursive typecode like
		# a struct or valuetype, we store the tc's original
		# position in the tcrecursion array. Therefore, if we
		# find an indirection to an entry in that array, the
		# typecode is recursive. Otherwise, it's just an
		# indirection - we then just skip to the old position
		# and re-scan the entire tc again. (Otherwise we'd
		# have to cache all typecodes in the stream.)
		#

		if {$kind == -1} {
		    set curpos [$buffer tell]
		    set offset [$buffer long]
		    set origpos [expr {$curpos + $offset}]
		    if {[info exists tcrecursion($origpos)]} {
			return [list recursive $tcrecursion($origpos)]
		    }
		    $buffer seek $origpos
		    set tc [TypeCode]
		    $buffer seek [expr {$curpos + 4}]
		    return $tc
		}

		set tckind [lindex {null void short long {unsigned short} \
			{unsigned long} float double boolean char octet \
			any TypeCode Principal Object struct union enum \
			string sequence array alias exception {long long} \
			{unsigned long long} {long double} wchar wstring \
			fixed valuetype valuebox native abstractinterface} \
			$kind]

		if {$tckind == ""} {
		    error "unknown tckind $kind"
		}

		#
		# done in case of "empty" tckinds
		#

		if {($kind >= 0 && $kind <= 13) || \
			($kind >= 23 && $kind <= 26)} {
		    return $tckind
		}

		#
		# handle "simple" tckinds
		#

		switch -- $tckind {
		    string -
		    wstring {
			set bound [$buffer ulong]
			if {$bound == 0} {
			    return $tckind
			}
			return [list $tckind $bound]
		    }
		    fixed {
			set digits [$buffer ushort]
			set scale [$buffer ushort]
			return [list $tckind $digits $scale]
		    }
		}

		#
		# Complex types with encapsulation
		#

		set tckindpos [expr {[$buffer tell] - 4}]
		$buffer begin_encaps

		switch -- $tckind {
		    Object {
			set repoid [$buffer string]
			set name [$buffer string]
			set res [list Object $repoid]
		    }
		    struct -
		    exception {
			set repoid [$buffer string]
			set tcrecursion($tckindpos) $repoid
			set name [$buffer string]
			set count [$buffer ulong]
			set members [list]
			for {set i 0} {$i < $count} {incr i} {
			    set member_name [$buffer string]
			    set member_type [Demarshal TypeCode]
			    lappend members $member_name $member_type
			}
			unset tcrecursion($tckindpos)
			set res [list $tckind $repoid $members]
		    }
		    union {
			set repoid [$buffer string]
			set tcrecursion($tckindpos) $repoid
			set name [$buffer string]
			set disctype [Demarshal TypeCode]
			set default [$buffer long]
			set count [$buffer ulong]
			set members [list]
			for {set i 0} {$i < $count} {incr i} {
			    set member_label [Demarshal $disctype]
			    set member_name [$buffer string]
			    set member_type [Demarshal TypeCode]
			    if {$i == $default} {
				lappend members "(default)" $member_type
			    } else {
				lappend members $member_label $member_type
			    }
			}
			unset tcrecursion($tckindpos)
			set res [list union $repoid $disctype $members]
		    }
		    enum {
			set repoid [$buffer string]
			set name [$buffer string]
			set count [$buffer ulong]
			set members [list]
			for {set i 0} {$i < $count} {incr i} {
			    set member_name [$buffer string]
			    lappend members $member_name
			}
			set res [list enum $members]
		    }
		    sequence {
			set element_type [Demarshal TypeCode]
			set bound [$buffer ulong]
			if {$bound == 0} {
			    set res [list sequence $element_type]
			} else {
			    set res [list sequence $element_type $bound]
			}
		    }
		    array {
			set element_type [Demarshal TypeCode]
			set length [$buffer ulong]
			set res [list array $element_type $length]
		    }
		    alias {
			set repoid [$buffer string]
			set name [$buffer string]
			set original_type [Demarshal TypeCode]
			set res $original_type
		    }
		    value {
			set repoid [$buffer string]
			set tcrecursion($tckindpos) $repoid
			set name [$buffer string]

			switch -- [$buffer short] {
			    0 {
				set modifier ""
			    }
			    1 {
				set modifier "custom"
			    }
			    2 {
				set modifier "abstract"
			    }
			    3 {
				set modifier "truncatable"
			    }
			    default {
				error "illegal valuetype modifier"
			    }
			}

			set basetc [Demarshal TypeCode]
			
			if {$basetc == "null"} {
			    set basetc 0
			}
			
			set count [$buffer ulong]
			set members [list]
			for {set i 0} {$i < $count} {incr i} {
			    set member_name [$buffer string]
			    set member_type [Demarshal TypeCode]
			    switch -- [$buffer short] {
				0 {
				    set member_visi "private"
				}
				1 {
				    set member_visi "public"
				}
				default {
				    error "illegal member visibility"
				}
			    }
			    lappend members $member_visi $member_name \
				    $member_type
			}

			unset tcrecursion($tckindpos)
			set res [list valuetype $repoid $members \
				$basetc $modifier]
		    }
		    valuebox {
			set repoid [$buffer string]
			set name [$buffer string]
			set original_type [Demarshal TypeCode]
			set res [list valuebox $repoid $original_type]
		    }
		    native {
			set repoid [$buffer string]
			set name [$buffer string]
			set res [list native $repoid]
		    }
		    abstractinterface {
			set repoid [$buffer string]
			set name [$buffer string]
			set res [list abstractinterface $repoid]
		    }
		    default {
			error "unsupported type $tckind ($kind)"
		    }
		}

		$buffer end_encaps
		return $res
	    }

	    private method any {} {
		set type [Demarshal TypeCode]
		set value [Demarshal $type]
		return [list $type $value]
	    }

	    private method bstring {tc} {
		set length [lindex $tc 1]
		set got [$buffer string]

		if {$length > 0 && [string length $got] > $length} {
		    error "Combat::CDR::Decoder: string exceeds bound"
		}

		return $got
	    }

	    private method bwstring {tc} {
		set length [lindex $tc 1]
		set got [$buffer wstring]

		if {$length > 0 && [string length $got] > $length} {
		    error "Combat::CDR::Decoder: wstring exceeds bound"
		}

		return $got
	    }

	    private method struct {tc} {
		set repoid [lindex $tc 1]
		set marshalrecursion($repoid) $tc
		set result [list]
		foreach {name type} [lindex $tc 2] {
		    set value [Demarshal $type]
		    lappend result $name $value
		}
		catch {unset marshalrecursion($repoid)}
		return $result
	    }

	    private method union {tc} {
		set repoid [lindex $tc 1]
		set marshalrecursion($repoid) $tc
		set default ""
		set dtype [lindex $tc 2]
		set disc [Demarshal $dtype]

		foreach {dval type} [lindex $tc 3] {
		    if {$dval == "(default)"} {
			set default $type
		    } elseif {$dval == $disc} {
			set value [Demarshal $type]
			catch {unset marshalrecursion($repoid)}
			return [list $disc $value]
		    }
		}
		if {$default != ""} {
		    set value [Demarshal $default]
		    catch {unset marshalrecursion($repoid)}
		    return [list $disc $value]
		}
		catch {unset marshalrecursion($repoid)}
		return [list $disc]
	    }

	    private method exception {tc} {
		set repoid [$buffer string]
		if {$repoid != [lindex $tc 1]} {
		    error "Combat::CDR::Decoder: wrong exception tc"
		}
		set result [list]
		foreach {name type} [lindex $tc 2] {
		    set value [Demarshal $type]
		    lappend result $name $value
		}
		return [list $repoid $result]
	    }

	    private method sequence {tc} {
		set type [lindex $tc 1]
		set length [$buffer ulong]
		if {[llength $tc] == 3} {
		    set max [lindex $tc 2]
		    if {$max > 0 && $length > $max} {
			error "Combat::CDR::Decoder: sequence exceeds bound"
		    }
		}
		if {$type == "octet"} {
		    return [$buffer octets $length]
		} elseif {$type == "char"} {
		    return [$buffer chars $length]
		} elseif {$type == "wchar"} {
		    return [$buffer wchars $length]
		}
		set res [list]
		for {set i 0} {$i < $length} {incr i} {
		    lappend res [Demarshal $type]
		}
		return $res
	    }

	    private method Array {tc} {
		set type [lindex $tc 1]
		set length [lindex $tc 2]
		if {$type == "octet"} {
		    return [$buffer octets $length]
		} elseif {$type == "char"} {
		    return [$buffer chars $length]
		} elseif {$type == "wchar"} {
		    return [$buffer wchars $length]
		}
		set res [list]
		for {set i 0} {$i < $length} {incr i} {
		    lappend res [Demarshal $type]
		}
		return $res
	    }

	    private method enum {tc} {
		set value [$buffer ulong]
		return [lindex [lindex $tc 1] $value]
	    }

	    private method Object {tc} {
		set ior [::Combat::IOP::DemarshalIOR $buffer]
		if {[llength [$ior cget -profiles]] == 0 && \
			[$ior cget -type_id] == ""} {
		    itcl::delete object $ior
		    return 0
		}

		#
		# Use the object references's own Repository Id (which might
		# be different from the one in $tc), unless we don't have
		# info about that type
		#

		set marshalid [lindex $tc 1]
		set repoid [$ior cget -type_id]

		if {$repoid == "" || \
			(![::Combat::SimpleTypeRepository::HaveTypeInfoForId $repoid] && \
			[::Combat::SimpleTypeRepository::HaveTypeInfoForId $marshalid])} {
		    set repoid $marshalid
		}

		set obj [namespace current]::[::Combat::CORBA::Object \#auto]
		$obj configure -ior $ior
		$obj configure -type_id $repoid
		return [::Combat::CORBA::ORB::MakeObjProc $obj]
	    }

	    private method ValueType {tc} {
		set repoid [lindex $tc 1]
		$buffer align 4
		set valuepos [$buffer tell]
		set valuetag [$buffer long]

		if {$valuetag == 0} {
		    return 0
		}

		if {[lindex $tc 4] == "custom"} {
		    error "cannot demarshal custom valuetype"
		}

		if {$valuetag == -1} {
		    set markerpos [$buffer tell]
		    set offset [$buffer long]
		    set origpos [expr {$markerpos + $offset}]
		    if {![info exists valuerecursion($origpos)]} {
			error "illegal or recursive value type indirection"
		    }
		    return $valuerecursion($origpos)
		}

		if {[expr {$valuetag & 8}] == 8 || $chunking} {
		    set chunked 1
		} else {
		    set chunked 0
		}

		if {[expr {$valuetag & 1}] == 1} {
		    set codebase [get_indirect_string]
		}

		switch [expr {$valuetag & 6}] {
		    0 {
			set repoids [list $repoid]
		    }
		    2 {
			set repoids [list [get_indirect_string]]
		    }
		    6 {
			set repoids [get_indirect_string_seq]
		    }
		    default {
			error "oops"
		    }
		}

		set marshaltc ""

		foreach id $repoids {
		    if {$id == $repoid} {
			set marshaltc $tc
			break
		    }
		    if {![catch {::corba::type of $id} testtc]} {
			set marshaltc $testtc
			break
		    }
		}

		if {$marshaltc == ""} {
		    error "oops"
		}

		set itc $marshaltc
		set tcs [list]

		while {$itc != 0} {
		    set tcs [linsert $tcs 0 $itc]
		    set itc [lindex $itc 3]
		}

		set repoid [lindex $marshaltc 1]
		set marshalrecursion($repoid) $marshaltc
		
		if {$chunked} {
		    set chunking 1
		    incr nestinglevel
		    $buffer begin_chunk
		}

		set result [list]

		foreach mtc $tcs {
		    foreach {visi name type} [lindex $mtc 2] {
			set value [Demarshal $type]
			lappend result $name $value
		    }
		}

		lappend result _tc_ $marshaltc

		if {$chunked} {
		    $buffer end_chunk
		    set level [expr {$nestinglevel + 1}]

		    while {$level > $nestinglevel} {
			set pos [$buffer tell]
			set tag [$buffer long]

			if {$tag > 0 && $tag < 2147483392} {
			    $buffer seek $pos
			    $buffer begin_chunk
			    $buffer end_chunk
			} elseif {$tag < 0} {
			    set level [expr {-$tag}]
			} else {
			    $buffer seek $pos
			    ValueType [list valuetype \
				    IDL:omg.org/CORBA/ValueBase:1.0 \
				    [list] "" ""]
			}
		    }
		    
		    if {$level < $nestinglevel} {
			$buffer seek $pos
		    }

		    if {[incr nestinglevel -1] == 0} {
			set chunking 0
		    }
		}

		set valuerecursion($valuepos) $result
		catch {unset marshalrecursion($repoid)}
		return $result
	    }

	    private method ValueBox {tc} {
		set vtc [list valuetype [lindex $tc 1] \
			[list public foo [lindex $tc 2]] 0 ""]
		
		set vt [ValueType $vtc]
		if {$vt == "0"} {
		    return $vt
		}
		foreach {name value} $vt {
		    if {$name == "foo"} {
			set res $value
			break
		    }
		}
		return $res
	    }

	    private method AbstractInterface {tc} {
		set disc [$buffer boolean]
		if {$disc} {
		    return [Object [list Object IDL:omg.org/CORBA/Object:1.0]]
		} else {
		    return [ValueType [list valuetype \
			    IDL:omg.org/CORBA/ValueBase:1.0 [list] "" ""]]
		}
	    }

	    private method recursive {tc} {
		set repoid [lindex $tc 1]
		if {![info exists marshalrecursion($repoid)]} {
		    error "illegal marshalling recursion for $repoid"
		}
		set savetc $marshalrecursion($repoid)
 		set res [Demarshal $marshalrecursion($repoid)] 
		set marshalrecursion($repoid) $savetc
		return $res
	    }

	    public method Demarshal {tc} {
		#
		# handle primitive types
		#
		switch $tc {
		    null                 { return }
		    void                 { return }
		    boolean              { return [$buffer boolean] }
		    short                { return [$buffer short] }
		    long                 { return [$buffer long] }
		    {unsigned short}     { return [$buffer ushort] }
		    {unsigned long}      { return [$buffer ulong] }
		    {long long}          { return [$buffer longlong] }
		    {unsigned long long} { return [$buffer ulonglong] }
		    float                { return [$buffer float] }
		    double               { return [$buffer double] }
		    {long double}        { return [$buffer longdouble] }
		    char                 { return [$buffer char] }
		    octet                { return [$buffer octet] }
		    wchar                { return [$buffer wchar] }
		    string               { return [$buffer string] }
		    wstring              { return [$buffer wstring] }
		    any                  { return [any] }
		    TypeCode             { return [TypeCode] }
		}

		#
		# constructed types
		#

		set type [lindex $tc 0]

		switch $type {
		    string               { return [bstring $tc] }
		    wstring              { return [bwstring $tc] }
		    struct               { return [struct $tc] }
		    union                { return [union $tc] }
		    exception            { return [exception $tc] }
		    sequence             { return [sequence $tc] }
		    array                { return [Array $tc] }
		    enum                 { return [enum $tc] }
		    Object               { return [Object $tc] }
		    valuetype            { return [ValueType $tc] }
		    valuebox             { return [ValueBox $tc] }
		    abstractinterface    { return [AbstractInterface $tc] }
		    recursive            { return [recursive $tc] }
		}

		error "unknown type: $tc"
	    }
	}

	itcl::class Encoder {
	    private variable buffer
	    private variable tcrecursion
	    private variable marshalrecursion
	    private variable nestinglevel
	    private variable chunking

	    constructor {} {
		set buffer [namespace current]::[::Combat::CDR::WriteBuffer \#auto]
		set nestinglevel 0
		set chunking 0
	    }

	    destructor {
		itcl::delete object $buffer
	    }

	    public method get_data {} {
		return [$buffer get_data]
	    }

	    public method get_buffer {} {
		return $buffer
	    }

	    private method TypeCode {value} {
		set tckind [lindex $value 0]
		if {$value == "unsigned short" || \
			$value == "unsigned long" || \
			$value == "long long" || \
			$value == "unsigned long long" || \
			$value == "long double"} {
		    set tckind $value
		}

		#
		# Handle recursion
		#

		if {$tckind == "recursive"} {
		    set repoid [lindex $value 1]
		    if {![info exists tcrecursion($repoid)]} {
			error "illegal recursion for $repoid"
		    }
		    $buffer long -1
		    set offset [expr {$tcrecursion($repoid) - [$buffer tell]}]
		    $buffer long $offset
		    return
		}


		set kind [lsearch {null void short long {unsigned short} \
			{unsigned long} float double boolean char octet \
			any TypeCode Principal Object struct union enum \
			string sequence array alias exception {long long} \
			{unsigned long long} {long double} wchar wstring \
			fixed valuetype valuebox native abstractinterface} \
			$tckind]

		if {$kind == -1} {
		    error "unknown tckind $tckind"
		}

		#
		# done in case of "empty" tckinds
		#

		if {($kind >= 0 && $kind <= 13) || \
			($kind >= 23 && $kind <= 26)} {
		    $buffer ulong $kind
		    return
		}

		#
		# handle "simple" tckinds
		#

		switch -- $tckind {
		    string -
		    wstring {
			$buffer ulong $kind
			if {[llength $value] == 2} {
			    set bound [lindex $value 1]
			    $buffer ulong $bound
			} else {
			    $buffer ulong 0
			}
			return
		    }
		    fixed {
			$buffer ulong $kind
			$buffer ulong [lindex $value 1]
			$buffer ulong [lindex $value 2]
			return
		    }
		}

		#
		# Complex types with encapsulation
		#

		$buffer align 4
		set tckindpos [$buffer tell]
		$buffer ulong $kind
		$buffer begin_encaps

		switch -- $tckind {
		    Object {
			$buffer string [lindex $value 1]
			$buffer string ""
		    }
		    struct -
		    exception {
			set repoid [lindex $value 1]
			set tcrecursion($repoid) $tckindpos
			$buffer string $repoid
			$buffer string ""
			set members [lindex $value 2]
			$buffer ulong [expr {[llength $members] / 2}]
			foreach {member_name member_type} $members {
			    $buffer string $member_name
			    Marshal TypeCode $member_type
			}
			unset tcrecursion($repoid)
		    }
		    union {
			set repoid [lindex $value 1]
			set tcrecursion($repoid) $tckindpos
			set disctype [lindex $value 2]
			set members [lindex $value 3]
			set default -1
			set count 0
			foreach {member_label member_type} $members {
			    if {$member_label == "(default)"} {
				set default $count
				break
			    }
			    incr count
			}
			$buffer string $repoid
			$buffer string ""
			Marshal TypeCode $disctype
			$buffer long $default
			$buffer ulong [expr {[llength $members] / 2}]
			set count 0
			foreach {member_label member_type} $members {
			    if {$count == $default} {
				switch -- $disctype {
				    boolean {
					$buffer boolean 0
				    }
				    short {
					$buffer short 0
				    }
				    {unsigned short} {
					$buffer ushort 0
				    }
				    long {
					$buffer long 0
				    }
				    {unsigned long} {
					$buffer ulong 0
				    }
				    default { # should be enum
					$buffer ulong 0
				    }
				}
			    } else {
				Marshal $disctype $member_label
			    }
			    $buffer string ""
			    Marshal TypeCode $member_type
			    incr count
			}
			unset tcrecursion($repoid)
		    }
		    enum {
			$buffer string ""
			$buffer string ""
			$buffer ulong [llength [lindex $value 1]]
			foreach member [lindex $value 1] {
			    $buffer string $member
			}
		    }
		    sequence {
			Marshal TypeCode [lindex $value 1]
			if {[llength $value] == 3} {
			    $buffer ulong [lindex $value 2]
			} else {
			    $buffer ulong 0
			}
		    }
		    array {
			Marshal TypeCode [lindex $value 1]
			$buffer ulong [lindex $value 2]
		    }
		    valuetype {
			set repoid [lindex $value 1]
			set tcrecursion($repoid) $tckindpos
			$buffer string $repoid
			$buffer string ""
			switch -- [lindex $value 4] {
			    "" {
				$buffer short 0
			    }
			    custom {
				$buffer short 1
			    }
			    abstract {
				$buffer short 2
			    }
			    truncatable {
				$buffer short 3
			    }
			    default {
				error "illegal modifier for valuetype tc"
			    }
			}
			if {[lindex $value 3] == 0} {
			    Marshal TypeCode null
			} else {
			    Marshal TypeCode [lindex $value 3]
			}
			set members [lindex $value 2]
			$buffer ulong [expr {[llength $members] / 3}]
			foreach {visi member_name member_type} $members {
			    $buffer string $member_name
			    Marshal TypeCode $member_type
			    switch -- $visi {
				private {
				    $buffer short 0
				}
				public {
				    $buffer short 1
				}
				default {
				    error "illegal visibility $visi"
				}
			    }
			}
			unset tcrecursion($repoid)
		    }
		    valuebox {
			$buffer string [lindex $value 1]
			$buffer string ""
			Marshal TypeCode [lindex $value 2]
		    }
		    native {
			$buffer string [lindex $value 1]
			$buffer string ""
		    }
		    abstractinterface {
			$buffer string [lindex $value 1]
			$buffer string ""
		    }
		    alias {
			error "oops, there are no such things as aliases"
		    }
		    default {
			error "unsupported type $tckind"
		    }
		}

		$buffer end_encaps
	    }

	    private method any {value} {
		Marshal TypeCode [lindex $value 0]
		Marshal [lindex $value 0] [lindex $value 1]
	    }

	    private method bstring {tc value} {
		set length [lindex $tc 1]
		if {$length > 0 && [string length $value] > $length} {
		    error "Combat::CDR::Encoder: string exceeds bound"
		}
		$buffer string $value
	    }

	    private method bwstring {tc value} {
		set length [lindex $tc 1]
		if {$length > 0 && [string length $value] > $length} {
		    error "Combat::CDR::Encoder: wstring exceeds bound"
		}
		$buffer wstring $value
	    }

	    private method struct {tc value} {
		set repoid [lindex $tc 1]
		set marshalrecursion($repoid) $tc
		set members [lindex $tc 2]
		if {[llength $value] != [llength $members]} {
		    error "Combat::CDR::Encoder: wrong # of struct members for struct $repoid"
		}
		foreach {member_name member_value} $value {
		    set map($member_name) $member_value
		}
		foreach {member_name member_type} $members {
		    if {![info exists map($member_name)]} {
			error "Combat::CDR::Encoder: missing member $member_name in struct $repoid"
		    }
		    Marshal $member_type $map($member_name)
		}
		catch {unset marshalrecursion($repoid)}
		unset map
	    }

	    private method find_union_case {value cases} {
		foreach {dval type} $cases {
		    if {$dval != "(default)" && $dval == $value} {
			return 1
		    }
		}
		return 0
	    }

	    private method find_default_for_union {dtype cases} {
		switch -- $dtype {
		    boolean {
			foreach val {0 1} {
			    if {![find_union_case $val $cases]} {
				return $val
			    }
			}
		    }
		    short -
		    {unsigned short} -
		    long -
		    {unsigned long} {
			for {set val 0} {$val < 32767} {incr val} {
			    if {![find_union_case $val $cases]} {
				return $val
			    }
			    if {![find_union_case [expr {-$val}] $cases]} {
				return [expr {-$val}]
			    }
			}
		    }
		}
		if {[lindex $dtype 0] == "enum"} {
		    foreach val [lindex $dtype 1] {
			if {![find_union_case $val $cases]} {
			    return $val
			}
		    }
		}
		error "find_default_for_union"
	    }
	    
	    private method union {tc value} {
		set repoid [lindex $tc 1]
		set marshalrecursion($repoid) $tc
		set default ""
		set dtype [lindex $tc 2]
		set disc [lindex $value 0]

		if {$disc == "(default)"} {
		    set disc [find_default_for_union $dtype [lindex $tc 3]]
		}

		Marshal $dtype $disc
		foreach {dval type} [lindex $tc 3] {
		    if {$dval == "(default)"} {
			set default $type
		    } elseif {$dval == $disc} {
			Marshal $type [lindex $value 1]
			catch {unset marshalrecursion($repoid)}
			return
		    } elseif {$dtype == "boolean"} {
			set lv [expr {$dval ? 1 : 0}]
			set dv [expr {$disc ? 1 : 0}]
			if {$lv == $dv} {
			    Marshal $type [lindex $value 1]
			    catch {unset marshalrecursion($repoid)}
			    return
			}
		    }
		}			    
		if {$default != ""} {
		    Marshal $default [lindex $value 1]
		}
		catch {unset marshalrecursion($repoid)}
		return
	    }

	    private method exception {tc value} {
		$buffer string [lindex $value 0]
		if {[llength $value] == 2} {
		    foreach {member_name member_value} [lindex $value 1] {
			set map($member_name) $member_value
		    }
		    foreach {member_name member_type} [lindex $tc 2] {
			Marshal $member_type $map($member_name)
		    }
		}
		catch {unset map}
	    }

	    private method sequence {tc value} {
		set type [lindex $tc 1]
		if {$type == "char" || $type == "octet" || $type == "wchar"} {
		    set length [::string length $value]
		} else {
		    set length [llength $value]
		}
		if {[llength $tc] == 3} {
		    set max [lindex $tc 2]
		    if {$max > 0 && $length > $max} {
			error "Combat::CDR::Encoder: sequence exceeds bound"
		    }
		}

		$buffer ulong $length

		if {$type == "octet"} {
		    $buffer octets $value
		} elseif {$type == "char"} {
		    $buffer chars $value
		} elseif {$type == "wchar"} {
		    $buffer wchars $value
		} else {
		    foreach element $value {
			Marshal $type $element
		    }
		}
	    }

	    private method Array {tc value} {
		set type [lindex $tc 1]
		if {$type == "char" || $type == "octet" || $type == "wchar"} {
		    set length [::string length $value]
		} else {
		    set length [llength $value]
		}
		if {$length != [lindex $tc 2]} {
		    error "Combat::CDR::Encoder: array length does not match"
		}
		if {$type == "octet"} {
		    $buffer octets $value
		} elseif {$type == "char"} {
		    $buffer chars $value
		} elseif {$type == "wchar"} {
		    $buffer wchars $value
		} else {
		    foreach element $value {
			Marshal $type $element
		    }
		}
	    }

	    private method enum {tc value} {
		set idx [lsearch -exact [lindex $tc 1] $value]
		if {$idx == -1} {
		    error "Combat::CDR::Encoder: illegal enum value $value"
		}
		$buffer ulong $idx
	    }

	    private method Object {tc value} {
		if {$value == 0} {
		    set ior [::Combat::IOP::IOR \#auto]
		    $ior marshal $buffer
		    itcl::delete object $ior
		    return
		}
		set obj [::Combat::CORBA::ORB::GetObjFromRef $value]
		set ior [$obj get_ior]
		$ior marshal $buffer
	    }

	    private method ValueType {tc value} {
		if {$value == 0} {
		    $buffer long 0
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

		if {[lindex $marshaltc 4] == "custom"} {
		    error "cannot marshal custom valuetype"
		}

		set value_tag 2147483392	;# 0x7fffff00
		set value_tag [expr {$value_tag | 6}]

		if {[lindex $marshaltc 4] == "truncatable" || $chunking} {
		    set chunked 1
		} else {
		    set chunked 0
		}

		if {$chunked} {
		    set value_tag [expr {$value_tag | 8}]
		}

		set repoids [list]
		set itc $marshaltc

		while {[lindex $itc 4] == "truncatable"} {
		    lappend repoids [lindex $itc 1]
		    set itc [lindex $itc 3]
		}

		lappend repoids [lindex $itc 1]

		#
		# end previous chunk first
		#

		if {$nestinglevel > 0 && $chunked} {
		    $buffer end_chunk
		    $buffer long [expr {-$nestinglevel}]
		}

		if {$chunked} {
		    set chunking 1
		    incr nestinglevel
		}

		$buffer long $value_tag
		$buffer ulong [llength $repoids]

		foreach id $repoids {
		    $buffer string $id
		}

		set found 0
		set itc $marshaltc
		set tcs [list]

		while {$itc != 0} {
		    if {$repoid == [lindex $itc 1]} {
			set found 1
		    }
		    set tcs [linsert $tcs 0 $itc]
		    set itc [lindex $itc 3]
		}

		if {$repoid == "IDL:omg.org/CORBA/ValueBase:1.0"} {
		    set found 1
		}

		if {!$found} {
		    error "Combat::CDR::Encoder: _tc_ not compatible with expected type $repoid"
		}

		if {$chunked} {
		    $buffer begin_chunk
		}

		foreach mtc $tcs {
		    foreach {visi member_name member_type} [lindex $mtc 2] {
			if {![info exists map($member_name)]} {
			    error "Combat::CDR::Encoder: missing member $member_name in value $repoid"
			}
			Marshal $member_type $map($member_name)
		    }
		}

		if {$chunked} {
		    $buffer end_chunk
		    $buffer long [expr {-$nestinglevel}]

		    if {[incr nestinglevel -1] == 0} {
			set chunking 0
		    }
		}

		catch {unset map}
		catch {unset marshalrecursion($repoid)}
	    }

	    private method ValueBox {tc value} {
		set vtc [list valuetype [lindex $tc 1] \
			[list public foo [lindex $tc 2]] 0 ""]
		if {$value == 0} {
		    ValueType $vtc $value
		} else {
		    ValueType $vtc [list foo $value]
		}
	    }

	    private method AbstractInterface {tc value} {
		if {$value == "0"} {
		    $buffer boolean 0
		    $buffer long 0
		    return
		}

		if {[llength $value] == 1} {
		    $buffer boolean 1
		    Object [list Object IDL:omg.org/CORBA/Object:1.0] $value
		} else {
		    $buffer boolean 0
		    ValueType [list valuetype \
			    IDL:omg.org/CORBA/ValueBase:1.0 \
			    [list] "" ""] $value
		}
	    }

	    private method recursive {tc value} {
		set repoid [lindex $tc 1]
		if {![info exists marshalrecursion($repoid)]} {
		    error "illegal marshalling recursion for $repoid"
		}
		set savetc $marshalrecursion($repoid)
 		set res [Marshal $marshalrecursion($repoid) $value]
		set marshalrecursion($repoid) $savetc
		return $res
	    }

	    public method Marshal {tc value} {
		#
		# handle primitive types
		#

		switch $tc {
		    null                 { return }
		    void                 { return }
		    boolean              { $buffer boolean $value ; return }
		    short                { $buffer short $value ; return }
		    long                 { $buffer long $value ; return }
		    {unsigned short}     { $buffer ushort $value ; return }
		    {unsigned long}      { $buffer ulong $value ; return }
		    {long long}          { $buffer longlong $value ; return }
		    {unsigned long long} { $buffer ulonglong $value ; return }
		    float                { $buffer float $value ; return }
		    double               { $buffer double $value ; return }
		    {long double}        { $buffer longdouble $value ; return }
		    char                 { $buffer char $value ; return }
		    octet                { $buffer octet $value ; return }
		    wchar                { $buffer wchar $value ; return }
		    string               { $buffer string $value ; return }
		    wstring              { $buffer wstring $value ; return }
		    any                  { any $value ; return }
		    TypeCode             { TypeCode $value ; return }
		}

		#
		# constructed types
		#

		set type [lindex $tc 0]

		switch $type {
		    string               { bstring $tc $value ; return }
		    wstring              { bwstring $tc $value ; return }
		    struct               { struct $tc $value ; return }
		    union                { union $tc $value ; return }
		    exception            { exception $tc $value ; return }
		    sequence             { sequence $tc $value ; return }
		    array                { Array $tc $value ; return }
		    enum                 { enum $tc $value ; return }
		    Object               { Object $tc $value ; return }
		    valuetype            { ValueType $tc $value ; return }
		    valuebox             { ValueBox $tc $value ; return }
		    abstractinterface    { AbstractInterface $tc $value ; return }
		    recursive            { recursive $tc $value ; return }
		}

		error "unknown type: $tc"
	    }
	}
    }
}

