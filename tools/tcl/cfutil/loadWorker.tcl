
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

#
# ----------------------------------------------------------------------
# Implementation of the CF::File interface using a local file.
# ----------------------------------------------------------------------
#

namespace eval cfutil {}

catch {itcl::delete class cfutil::CFReadonlyFile}

itcl::class cfutil::CFReadonlyFile {
    inherit PortableServer::ServantBase

    public variable fileName
    public variable filePointer

    private variable m_poa
    private variable m_channel
    private variable m_nativeName

    public method _Interface {} {
        return "IDL:CF/File:1.0"
    }

    constructor {poa nativeName name} {
        set fileName $name
        set filePointer 0
        set m_poa $poa
        set m_nativeName $nativeName
        set m_channel [open $nativeName]
        fconfigure $m_channel -translation binary
    }

    public method read {data_name length} {
        upvar $data_name data
        set data [::read $m_channel $length]
        set filePointer [tell $m_channel]
    }

    public method write {data} {
        corba::throw {IDL:CF/File/IOException:1.0 {errorNumber EIO msg {read-only file}}}
    }

    public method sizeOf {} {
        return [file size $m_nativeName]
    }

    public method close {} {
        set oid [$m_poa servant_to_id $this]
        $m_poa deactivate_object $oid
        ::close $m_channel
        itcl::delete object $this
    }

    public method setFilePointer {fp} {
        seek $m_channel $fp
        set filePointer [tell $m_channel]
    }
}

#
# ----------------------------------------------------------------------
# Implementation of the CF::FileSystem interface that makes a single
# file available for download.
# ----------------------------------------------------------------------
#

catch {itcl::delete class cfutil::CFSingleFileSystem}

itcl::class cfutil::CFSingleFileSystem {
    inherit PortableServer::ServantBase

    private variable m_poa
    private variable m_nativeName
    private variable m_fileName

    public method _Interface {} {
        return "IDL:CF/FileSystem:1.0"
    }

    constructor {poa nativeName fileName} {
        set m_poa $poa
        set m_nativeName $nativeName
        set m_fileName $fileName
    }

    public method remove {fileName} {
        corba::throw {IDL:CF/FileException:1.0 {errorNumber EIO msg {read-only file}}}
    }

    public method copy {sourceFileName destinationFileName} {
        corba::throw {IDL:CF/FileException:1.0 {errorNumber EIO msg {read-only file}}}
    }

    public method exists {fileName} {
        if {$fileName == $m_fileName} {
            return 1
        }

        return 0
    }

    public method list {pattern} {
        if {[string match $pattern $m_fileName]} {
            return [::list [::list \
                              name $m_fileName \
                              kind PLAIN \
                              size [file size $m_nativeName] \
                              fileProperties [::list]]]
        }

        return [::list]
    }

    public method create {fileName} {
        corba::throw {IDL:CF/FileException:1.0 {errorNumber EIO msg {read-only file}}}
    }

    public method open {fileName read_Only} {
        if {$fileName != $m_fileName} {
            corba::throw {IDL:CF/FileException:1.0 {errorNumber EIO msg {read-only file}}}
        }

        set srv [namespace current]::[cfutil::CFReadonlyFile \#auto $m_poa $m_nativeName $m_fileName]
        set oid [$m_poa activate_object $srv]
        return [$m_poa id_to_reference $oid]
    }

    public method mkdir {directoryName} {
        corba::throw {IDL:CF/FileException:1.0 {errorNumber EIO msg {read-only file}}}
    }

    public method rmdir {directoryName} {
        corba::throw {IDL:CF/FileException:1.0 {errorNumber EIO msg {read-only file}}}
    }

    public method query {fileSystemProperties_name} {
        upvar $fileSystemProperties_name fileSystemProperties

        if {![llength $fileSystemProperties]} {
            return
        }

        corba::throw [::list IDL:CF/FileSystem/UnknownFileSystemProperties:1.0 \
                          [::list invalidProperties $fileSystemProperties]]
    }
}

#
# ----------------------------------------------------------------------
# Load a worker onto a CF Loadable Device.  If successful, returns a
# fileName with all path components removed, so that the name can be
# used with ExecutableDevice::execute() and LoadableDevice::unload().
# ----------------------------------------------------------------------
#

proc cfutil::loadWorker {device fileName loadType} {
    #
    # Sanity check: does the file exist?
    #

    if {![file exists $fileName]} {
        error "file not found: \"$fileName\""
    }

    #
    # Sanity check: Is this a loadable device?
    #

    if {![$device _is_a IDL:CF/LoadableDevice:1.0]} {
        error "not a loadable device"
    }

    #
    # To keep the API simple, use the RootPOA.
    #

    set poa [corba::resolve_initial_references "RootPOA"]
    set mgr [$poa the_POAManager]
    $mgr activate

    #
    # Create a CF::FileSystem that the Loadable Device can download the
    # file from.
    #

    append fsFileName "/" [file tail $fileName]

    set srv [namespace current]::[cfutil::CFSingleFileSystem \#auto $poa $fileName $fsFileName]
    set oid [$poa activate_object $srv]
    set fsref [$poa id_to_reference $oid]

    #
    # Load the file.
    #

    corba::try {
        $device load $fsref $fsFileName $loadType
    } catch {... ex} {
        $poa deactivate_object $oid
        itcl::delete object $srv
        corba::release $fsref
        corba::throw $ex
    }

    $poa deactivate_object $oid
    itcl::delete object $srv
    corba::release $fsref

    return $fsFileName
}
