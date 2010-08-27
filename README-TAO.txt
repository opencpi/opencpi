We currently only use TAO for the ocpisca tool, and we may eliminate
this need (and use OMNI instead).  To avoid a system-level
installation requirement (i.e. a "make install" in TAO), we have set things up to
assume that there is a ACE_wrappers directory at the same level as the top
level opencpi directory, and that the ACE/TAO build directories are subdirectories
of that directories with the standard opencpi subdir names (linux-x86_64-bin, darwin-i386-bin etc.).

Since we don't do a full "make install", we reference the includes and libraries
in these ACE trees.  Includes generally reference the source tree (but codegenerated includes reference the built tree), and libraries reference the built tree.

As far as execution goes, to set up the dynamlic library paths go:

On a mac do, here in this directory, in csh:

  source darwin-ldpath.csh

On linux do, here in this directory, in bash:

  . linux-ldpath.sh

These point both to the ocpi lib directory (./lib/<target>) as well as the TAO library directories necessary to run ocpisca.


