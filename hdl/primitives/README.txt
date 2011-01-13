Primitives are just low level source libraries OR cores built usually
from other authoring processes - like Xilinx coregen.  So the Makefile
includes hdl-lib.mk if the primitive is just a primitive library of
simple source files, or it includes hdl-core.mk if the primitive is a
core.

Primitive libraries are just processed enough to make them
accessible/usable by reference from higher level modules libraries.
The actual resulting artifacts from (pre)compiling primitive libraries
vary by vendor and tool, but the common result is that compiling and
synthesizing higher level modules (workers), just needs to reference
the primitive module via appropriately set up search rules.  I.e. the
module will note that it requires a given primitive library, and then
it may just reference anything in that library.


Primitive cores are similar to primitive libraries except: 1) the
preprocessing generally does full synthesis to make a true "core" as a
resulting artifact, and 2) there is only one top-level referencable
module name in a primitive core, whereas there are multiple in a
primitive library.  Another difference is that if you reference a
primitive core, you "get it all": the whole core gets included in your
design, whereas primitive libraries are processed such that you only
"pull in" what you reference, among the various modules in the
library.

In both cases, the Makefile may have an "import" make target which
copies source files from somewhere else, and puts them into an imports
subdirectory.  When the "import" is from the standard OpenCPI location
it can use the "OCPI_HDL_IMPORT_DIR" environment variable.

