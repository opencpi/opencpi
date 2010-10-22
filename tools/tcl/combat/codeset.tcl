
# #####
#
#  Copyright (c)  Frank Pilhofer, combat@fpx.de
#
#  Please visit the Combat Web site at http://www.fpx.de/Combat/ for
#  more information.
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

#
# ======================================================================
# This file is part of Combat/Tcl, a Tcl CORBA Object Request Broker
# ======================================================================
#
# CVS Version Tag: $Id: codeset.tcl,v 1.13 2008/11/08 18:24:50 Owner Exp $
#
# ----------------------------------------------------------------------
# CONV_FRAME Module
# ----------------------------------------------------------------------
#

namespace eval Combat {
    namespace eval CONV_FRAME {
        #
        # charset registry
        #
        # This array maps codesets to Tcl encodings, and determines the
        # codesets supported by Combat in GIOP codeset negotiation. To
        # add support for a new codeset, you have to add a new entry here.
        # The array index is the "Registered Value", in decimal, from the
        # "OSF Character and Code Set Registry", which is available from
        #
        #   ftp://ftp.opengroup.org/pub/code_set_registry/cs_registry1.2h
        # 
        # The array element is a list of length two. The first list element
        # is the Tcl encoding (one from [encoding names]) that matches this
        # codeset, the second element is a "human-readable" name for that
        # codeset; it is used for debugging messages.
        #

        variable codesets
        array set codesets {
            65537 {
                iso8859-1
                "ISO 8859-1:1987; Latin Alphabet No. 1"
            }
            65551 {
                iso8859-15
                "ISO/IEC 8859-15:1999; Latin Alphabet No. 9"
            }
            65568 {
                ascii
                "ISO 646:1991 IRV (International Reference Version)"
            }
            65792 {
                unicode
                "ISO/IEC 10646-1:1993; UCS-2, Level 1"
            }
            65801 {
                unicode
                "ISO/IEC 10646-1:1993; UTF-16"
            }
            83951617 {
                utf-8
                "X/Open UTF-8; UCS Transformation Format 8 (UTF-8)"
            }
            268567780 {
                cp1252
                "IBM-1252 (CCSID 01252); MS Windows Latin-1"
            }
        }

        #
        # The native codeset that is to be used for strings (and advertised
        # as SNCS). This is an index into the above array. It is set upon
        # initialization from [encoding system] or -ORBNativeCodeSet.
        #

        variable native_codeset -1

        #
        # CodeSetInfo Tagged Component for MultipleComponentProfile
        #

        itcl::class CodeSetInfo {
            inherit ::Combat::IOP::TaggedComponent

            public variable char_native_code_set
            public variable char_conversion_code_sets
            public variable wchar_native_code_set
            public variable wchar_conversion_code_sets

            constructor {} {
                set tag 1                                ;# TAG_CODE_SETS
                set char_native_code_set 0
                set char_conversion_code_sets [list]
                set wchar_native_code_set 0
                set wchar_conversion_code_sets [list]
            }

            public method marshal {buffer} {
                $buffer ulong 1                                ;# TAG_CODE_SETS
                $buffer begin_encaps

                $buffer ulong $char_native_code_set
                $buffer ulong [llength $char_conversion_code_sets]
                foreach cs $char_conversion_code_sets {
                    $buffer ulong $cs
                }

                $buffer ulong $wchar_native_code_set
                $buffer ulong [llength $wchar_conversion_code_sets]
                foreach cs $wchar_conversion_code_sets {
                    $buffer ulong $cs
                }

                $buffer end_encaps
            }
        }

        proc DemarshalCodeSetInfo {buffer} {
            $buffer begin_encaps

            set char_native_code_set [$buffer ulong]
            set char_conversion_code_sets [list]
            set count [$buffer ulong]
            for {set i 0} {$i < $count} {incr i} {
                lappend char_conversion_code_sets [$buffer ulong]
            }

            set wchar_native_code_set [$buffer ulong]
            set wchar_conversion_code_sets [list]
            set count [$buffer ulong]
            for {set i 0} {$i < $count} {incr i} {
                lappend wchar_conversion_code_sets [$buffer ulong]
            }
            
            $buffer end_encaps

            set res [namespace current]::[::Combat::CONV_FRAME::CodeSetInfo \#auto]
            $res configure -tag 1 \
                    -char_native_code_set $char_native_code_set \
                    -char_conversion_code_sets $char_conversion_code_sets \
                    -wchar_native_code_set $wchar_native_code_set \
                    -wchar_conversion_code_sets $wchar_conversion_code_sets
            return $res
        }

        itcl::class CodeSetContext {
            inherit ::Combat::IOP::ServiceContext

            public variable char_data
            public variable wchar_data

            public method marshal {buffer} {
                $buffer ulong 1                                ;# CodeSets
                $buffer begin_encaps
                $buffer ulong $char_data
                $buffer ulong $wchar_data
                $buffer end_encaps
            }
        }

        proc DemarshalCodeSetContext {buffer} {
            $buffer begin_encaps
            set char_data [$buffer ulong]
            set wchar_data [$buffer ulong]
            $buffer end_encaps

            set res [namespace current]::[::Combat::CONV_FRAME::CodeSetContext \#auto]
            $res configure -context_id 1 \
                    -char_data $char_data -wchar_data $wchar_data
            return $res
        }

        #
        # Perform conversion between native and transmission code set. This
        # behaves differently for char and wchar values. As each instance
        # is concerned with a single tcs only, there are usually two Code-
        # SetConverters involved, one for tcs-c and one for tcs-w.
        #
        # tcs is a Charset Ids from the OSF Character and Code Set Registry,
        # see the codesets array above.
        #

        itcl::class CodeSetConverter {
            public variable tcs

            constructor {_tcs} {
                set tcs $_tcs
            }

            public method get_char {buffer} {
                error "pure virtual"
            }

            public method get_string {buffer} {
                error "pure virtual"
            }

            public method get_wchar {buffer} {
                error "pure virtual"
            }

            public method get_wstring {buffer} {
                error "pure virtual"
            }
        }

        #
        # Generic Converter for character sets supported by [encoding]
        # and registered with our codesets array. GIOP 1.2 version.
        #

        itcl::class GenericConverter12 {
            inherit ::Combat::CONV_FRAME::CodeSetConverter

            public variable encoding

            constructor {_tcs} {
                ::Combat::CONV_FRAME::CodeSetConverter::constructor $_tcs
            } {
                set encoding [lindex $::Combat::CONV_FRAME::codesets($_tcs) 0]
            }

            private method wordswap {data} {
                binary scan $data s* elements
                return [binary format S* $elements]
            }

            public method get_char {buffer} {
                return [::encoding convertfrom $encoding [$buffer octet]]
            }

            public method get_chars {buffer length} {
                return [::encoding convertfrom $encoding \
                        [$buffer octets $length]]
            }

            public method get_wchar {buffer} {
                set olen [$buffer octet]
                if {[binary scan $olen c length] != 1} {
                    error "[$this info class]::get_wchar"
                }
                set data [$buffer octets $length]
                #
                # UTF-16 special BOM handling
                #
                if {$tcs == 65801} {
                    global tcl_platform

                    if {[binary scan $data S bom] == 1} {
                        if {$bom == -257} {
                            if {$tcl_platform(byteOrder) == "littleEndian"} {
                                set data [wordswap [string range $data 2 end]]
                            } else {
                                set data [string range $data 2 end]
                            }
                        } elseif {$bom == -2} {
                            if {$tcl_platform(byteOrder) == "littleEndian"} {
                                set data [string range $data 2 end]
                            } else {
                                set data [wordswap [string range $data 2 end]]
                            }
                        } elseif {$tcl_platform(byteOrder) == "littleEndian"} {
                            set data [wordswap $data]
                        }
                    }
                }

                return [::encoding convertfrom $encoding $data]
            }
            
            public method get_wchars {buffer length} {
                set res ""
                for {set i 0} {$i < $length} {incr i} {
                    append res [get_wchar $buffer]
                }
            }

            public method get_string {buffer} {
                set length [$buffer ulong]
                incr length -1 ;# substract trailing NUL
                set data [$buffer octets $length]
                $buffer octet
                return [::encoding convertfrom $encoding $data]
            }

            #
            # wstrings aren't NUL-terminated in GIOP 1.2
            #

            public method get_wstring {buffer} {
                set length [$buffer ulong]
                set data [$buffer octets $length]
                #
                # UTF-16 special BOM handling
                #
                if {$tcs == 65801} {
                    global tcl_platform

                    if {[binary scan $data S bom] == 1} {
                        if {$bom == -257} {
                            if {$tcl_platform(byteOrder) == "littleEndian"} {
                                set data [wordswap [string range $data 2 end]]
                            } else {
                                set data [string range $data 2 end]
                            }
                        } elseif {$bom == -2} {
                            if {$tcl_platform(byteOrder) == "littleEndian"} {
                                set data [string range $data 2 end]
                            } else {
                                set data [wordswap [string range $data 2 end]]
                            }
                        } elseif {$tcl_platform(byteOrder) == "littleEndian"} {
                            set data [wordswap $data]
                        }
                    }
                }

                return [::encoding convertfrom $encoding $data]
            }

            public method put_char {buffer val} {
                if {[::string length $val] != 1} {
                    error "[$this info class]::put_char: length of $val != 1"
                }
                set data [::encoding convertto $encoding $val]
                $buffer octet [string range $data 0 0]
            }

            public method put_chars {buffer val} {
                $buffer octets [::encoding convertto $encoding $val]
            }

            public method put_wchar {buffer val} {
                if {[::string length $val] != 1} {
                    error "[$this info class]::put_wchar: length of $val != 1"
                }
                set data [::encoding convertto $encoding $val]

                #
                # UTF-16 special BOM handling
                #

                global tcl_platform

                if {$tcs == 65801 && \
                        $tcl_platform(byteOrder) == "littleEndian"} {
                    set olen [binary format c [expr {[string length $data] + 2}]]
                    $buffer octet $olen
                    $buffer octets "\xff\xfe"
                    $buffer octets $data
                } else {
                    set olen [binary format c [string length $data]]
                    $buffer octet $olen
                    $buffer octets $data
                }
            }

            public method put_wchars {buffer val} {
                set length [string length $val]
                for {set i 0} {$i < $length} {incr i} {
                    put_wchar $buffer [string index $val $i]
                }
            }

            public method put_string {buffer val} {
                set data [::encoding convertto $encoding $val]
                set length [::string length $data]
                incr length ;# terminating null
                $buffer ulong $length
                $buffer octets $data
                $buffer octet \0
            }

            #
            # wstrings aren't NUL-terminated in GIOP 1.2
            #

            public method put_wstring {buffer val} {
                set data [::encoding convertto $encoding $val]

                #
                # UTF-16 special BOM handling
                #

                global tcl_platform

                if {$tcs == 65801 && \
                        $tcl_platform(byteOrder) == "littleEndian"} {
                    $buffer ulong [expr {[::string length $data] + 2}]
                    $buffer octets "\xff\xfe"
                    $buffer octets $data
                } else {
                    $buffer ulong [::string length $data]
                    $buffer octets $data
                }
            }
        }

        #
        # The same for GIOP 1.1. wchar and wstring handling is too
        # complicated for us.
        #

        itcl::class GenericConverter11 {
            inherit ::Combat::CONV_FRAME::CodeSetConverter

            public variable encoding

            constructor {_tcs} {
                ::Combat::CONV_FRAME::CodeSetConverter::constructor $_tcs
            } {
                set encoding [lindex $::Combat::CONV_FRAME::codesets($_tcs) 0]
            }

            public method get_char {buffer} {
                return [::encoding convertfrom $encoding [$buffer octet]]
            }

            public method get_chars {buffer length} {
                return [::encoding convertfrom $encoding \
                        [$buffer octets $length]]
            }

            public method get_wchar {buffer} {
                error "don't wanna handle wchars in GIOP 1.1"
            }

            public method get_wchars {buffer} {
                error "don't wanna handle wchars in GIOP 1.1"
            }
            
            public method get_string {buffer} {
                set length [$buffer ulong]
                set data [$buffer octets $length]
                set res [::encoding convertfrom $encoding $data]
                #
                # data includes a trailing null
                #
                set sl [::string length $res]
                incr sl -2
                return [::string range $res 0 $sl]
            }

            public method get_wstring {buffer} {
                set length [$buffer ulong]
                set data [$buffer octets [expr {2*$length}]]
                set res [::encoding convertfrom unicode $data]
                set sl [::string length $res]
                incr sl -2
                return [::string range $res 0 $sl]
            }

            public method put_char {buffer val} {
                if {[::string length $val] != 1} {
                    error "[$this info class]::put_char: length of $val != 1"
                }
                set data [::encoding convertto $encoding $val]
                $buffer octet [::string range $data 0 0]
            }

            public method put_chars {buffer val} {
                $buffer octets [::encoding convertto $encoding $val]
            }

            public method put_wchar {buffer val} {
                error "don't wanna handle wchars in GIOP 1.1"
            }

            public method put_wchars {buffer val} {
                error "don't wanna handle wchars in GIOP 1.1"
            }

            public method put_string {buffer val} {
                set data [::encoding convertto $encoding $val]
                set length [::string length $data]
                incr length ;# terminating null
                $buffer ulong $length
                $buffer octets $data
                $buffer octet \0
            }

            public method put_wstring {buffer val} {
                set data [::encoding convertto unicode $val]
                set length [::string length $val]
                incr length
                $buffer ulong $length
                $buffer octets $data
                $buffer octets \0\0
            }
        }

        #
        # find Transmission CodeSet according to algorithm
        #

        proc findTransCodeSet {what sncs sccs} {
            if {$sncs != 0} {
                #
                # if (CNCS==SNCS) TCS=CNCS
                #
                if {$sncs == $::Combat::CONV_FRAME::native_codeset} {
                    return $sncs
                }
                #
                # if (elementOf(SNCS,CCCS)) TCS=SNCS
                #
                foreach ccs [array names ::Combat::CONV_FRAME::codesets] {
                    if {$sncs != 0 && $sncs == $ccs} {
                        return $sncs
                    }
                }
            }
            if {$::Combat::CONV_FRAME::native_codeset != 0} {
                #
                # if (elementOf(CNCS,SCCS)) TCS=CNCS
                #
                foreach scs $sccs {
                    if {$::Combat::CONV_FRAME::native_codeset == $scs} {
                        return $scs
                    }
                }
            }
            #
            # if (intersection(CCCS,SCCS)) TCS=oneOf(intersection(CCCS,SCCS))
            #
            foreach ccs [array names ::Combat::CONV_FRAME::codesets] {
                if {[lsearch -exact $sccs $ccs] != -1} {
                    return $ccs
                }
            }
            #
            # TCS=fallbackCS
            #
            if {$what == "char"} {
                return 83951617                ;# UTF-8
            }
            return 65801                ;# UTF-16
        }

        #
        # Get a converter
        #

        proc getConverter {giop_major giop_minor tcs} {
            if {$tcs == $::Combat::CONV_FRAME::native_codeset} {
                return ""        ;# no converter needed
            }

            if {![info exists ::Combat::CONV_FRAME::codesets($tcs)]} {
                error [list IDL:omg.org/CORBA/DATA_CONVERSION:1.0 [list \
                        minor 0 completion_status COMPLETED_NO]]
            }

            if {$giop_major == 1 && $giop_minor == 1} {
                return [namespace current]::[::Combat::CONV_FRAME::GenericConverter11 \#auto $tcs]
            } elseif {$giop_major == 1 && $giop_minor == 2} {
                return [namespace current]::[::Combat::CONV_FRAME::GenericConverter12 \#auto $tcs]
            }

            error [list IDL:omg.org/CORBA/DATA_CONVERSION:1.0 [list \
                    minor 0 completion_status COMPLETED_NO]]
        }

        proc MakeCodeSetInfo {} {
            set csinfo [namespace current]::[::Combat::CONV_FRAME::CodeSetInfo \#auto]
            $csinfo configure \
                    -char_native_code_set $::Combat::CONV_FRAME::native_codeset \
                    -wchar_native_code_set 65801

            return $csinfo
        }

        proc init {{native ""}} {
            if {$::Combat::CONV_FRAME::native_codeset != -1} {
                set native $::Combat::CONV_FRAME::native_codeset
            } elseif {$native == "" || $native == -1} {
                set native [encoding system]
            }

            #
            # Make sure that this encoding exists.
            #

            if {![info exists ::Combat::CONV_FRAME::codesets($native)]} {
                foreach index [array names ::Combat::CONV_FRAME::codesets] {
                    if {[lindex $::Combat::CONV_FRAME::codesets($index) 0] == $native} {
                        set native $index
                        break
                    }
                }
            }

            if {![info exists ::Combat::CONV_FRAME::codesets($native)]} {
                error "unknown native codeset: $native"
            }

            set ::Combat::CONV_FRAME::native_codeset $native

            if {$::Combat::debug(info)} {
                set csenc [lindex $::Combat::CONV_FRAME::codesets($native) 0]
                set csname [lindex $::Combat::CONV_FRAME::codesets($native) 1]
                puts stderr "INFO: Using $csname ($csenc) as native codeset"
            }
        }

    }
}
