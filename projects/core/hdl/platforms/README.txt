Creating a new platform.

First, make the changes to hdl-targets, which defines the platform
name and the part-speed-package it uses.  If it is a new part family,
that has to be added as well.  That may involve updating the tool
scripts, e.g. xst.mk or quartus.mk.

Next, make a copy of an existing platform directory as a starting point.

In the platform's directory you must:

0. Define the platform worker in <platform>.xml

1. Specify the cores used by setting the Cores variable to specify cores found in the
hdl/primitives directory.

2. Specify the SourceFiles variable to the top level source files that are 
presumably placed in this directory other than the platform worker itself.

3. Create a <platform>.mk file for use by the final bit-stream-building scripts as needed.
This is needed when tool scripts need to know certain settings for the platform being built.
Examples are the "HdlPromArgs_<platform>" variable that xilinx back end scripts need.

4. Create any other per-platform files, like Xilinx UCF/XCF files.






