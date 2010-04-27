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
