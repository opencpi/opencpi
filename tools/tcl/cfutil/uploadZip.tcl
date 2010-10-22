
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
# Upload the contents of a Zip file into a remote CF::FileSystem
# ----------------------------------------------------------------------
#

namespace eval cfutil {
    variable zipFileIndex 0
}

#
# ----------------------------------------------------------------------
# Upload the contents of a Zip file.
#
# fs:  Object reference to a FileSystem or FileManager.
# zip: The file name of the Zip file in the local file system.
# dir: Remote directory; must be an absolute name of a directory
#      in the remote file system; if this directory does not exist,
#      it is created.
# ----------------------------------------------------------------------
#

proc cfutil::uploadZip {fs zip dir} {
    package require vfs
    package require vfs::zip

    if {![$fs exists $dir]} {
        $fs mkdir $dir
    }

    set idx [incr ::cfutil::zipFileIndex]
    set vfsd "/zip$idx"
    set vfsdl [string length $vfsd]

    set zipFd [vfs::zip::Mount $zip $vfsd]
    set files [glob -nocomplain "$vfsd/*"]

    #
    # Tcl 8.5 on Windows insists on prefixing the mount point with a drive letter.
    #

    if {[string range [lindex $files 0] 1 2] == ":/"} {
        incr vfsdl 2
    }

    corba::try {
        while {[llength $files]} {
            set fileName [lindex $files 0]
            set files [lrange $files 1 end]
            set relName [string range $fileName $vfsdl end]
            set absName "$dir$relName"

            if {[file isdirectory $fileName]} {
                if {![$fs exists $absName]} {
                    $fs mkdir $absName
                }
                foreach newFile [glob -nocomplain "$fileName/*"] {
                    lappend files $newFile
                }
            } else {
                if {[$fs exists $absName]} {
                    $fs remove $absName
                }

                set lf [open $fileName "r"]
                fconfigure $lf -translation binary

                corba::try {
                    set rf [$fs create $absName]

                    corba::try {
                        set data [read $lf 16384]

                        while {[string length $data] > 0} {
                            $rf write $data
                            set data [read $lf 16384]
                        }
                    } catch {... ex} {
                        corba::try {
                            $rf close
                            corba::release $rf
                        } catch {...} {
                        }
                        corba::throw $ex
                    }

                    $rf close
                    corba::release $rf
                } catch {... ex} {
                    catch {close $lf}
                    corba::throw $ex
                }

                close $lf
            }
        }
    } catch {... ex} {
        catch {vfs::zip::Unmount $zipFd $vfsd}
        corba::throw $ex
    }

    vfs::zip::Unmount $zipFd $vfsd
}
