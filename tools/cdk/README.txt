This sdk directory contains tools and makefile scripts that comprise
the sdk.

The "export" subdirectory is a directory of hand-create
simlinks, NOT SOURCE FILES, that can be tar'd to export the sdk.

The top level "include" directory contains make scripts, and other
constant files.  Tools are in their own subdirectories built normally,
starting with ocpigen.  The target directory names for the sdk are
based on the `uname -s`-`uname -m` formula, while the overall OCPI
build files use things like linux-bin and macos-bin.  The export link
tree converts one to the other.

Thus the export tree is expected to be copied into a real
"installation" tree that might include stuff build from other areas.

Adding support for an authoring model:

Things are generally structured so that adding an authoring model is adding a directory in certain areas and populating those directories.
Some things are not ideal...

For "make" support, add the include scripts include/mmm/*.mk.
For model-specific include/header files, put them in include/mmm also.

For now, new models must be added to include/lib.mk also.

The ocpigen tool is partially, though not cleanly modular for adding authoring models.
TODO: the ocpigen tool will have a more open "framework" so that it is cleaner to add authoring models.



