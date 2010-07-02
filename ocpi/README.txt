This directory represents an installation of OpenCPI, and is the
default target of various "make install" actions.

In a real installation, without all the sources, etc.  This is what
would be installed somewhere like /opt or /usr/local etc.

The bin subdirectory is for executables, with subdirectories for each
executable platform.  It would normally be put in your PATH.  It is
currently a link into the export of the tools/sdk package since that
is the only place binaries come from that are part of the
installation.

The lib subdirectory is for shared libraries to support the
executables (when they are not statically built).  It would normally
be put in your LD_LIBRARY_PATH (or on MacOSX DYLD_LIBRARY_PATH).  The
lib subdirectory has subdirectories for each executable platform.

There is a subdirectory under "lib" for each model, where libraries
are placed build for that model.


