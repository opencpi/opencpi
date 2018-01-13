Primitives are low level modules used in the various types of workers.
They are either simple source libraries of VHDL or verilog, OR cores
built from other authoring processes - like Xilinx coregen or Altera
megawizrds.  So the individual Makefiles include hdl-lib.mk if the
primitive is just a primitive library of simple source files, or it
includes hdl-core.mk if the primitive is a core.

Primitive libraries are processed (precompiled) enough to make them
accessible/usable by reference from higher level modules or libraries.
The actual resulting artifacts from (pre)compiling primitive libraries
vary by vendor and tool, but the common result is that compiling and
synthesizing higher level modules (workers) just needs to reference
the primitive module in a library via appropriate search rules.
I.e. the higher level module will declare that it requires a given
primitive library, and then it may just reference anything in that
library without any extra black box definitions or specific source
code references.  The bsv (small primitives used by BSV code
generation), ocpi (primitives used by code generation), and util
libraries are implicitly used whether they are mentioned or not.

In any library, but in particular, the "util" library, there can be
alternative implementations of modules for different families and
vendors.  A good example is ROM.v.  There is a generic, simple
implementation at util/ROM.v.  But in order to infer a Xilinx BRAM it
needs to be coded in a particular way, so there is also a xilinx/ROM.v
file.  Similarly, Altera requires a different code for it to infer
BRAM, hence there is an altera/ROM.v.  For any generic source file in
the top level of a library, the family or vendor-specific version is
used when compiling for that target.  A family-specific version will
override a vendor-specific version which will override a generic
version.

Primitive cores are similar to primitive libraries except:

1) the preprocessing  generally does full synthesis to make a true "core" as a
   resulting artifact, and

2) there is only one top-level referencable module name in a primitive core,
   whereas there are multiple in a primitive library.

Another difference is that if you reference a primitive core, you "get it all":
the whole core gets included in your design, whereas primitive libraries are
processed such that you only "pull in" what you reference, among the various
modules in the library.

In either libraries or cores, the Makefile may have an "import" make target
which copies source files from somewhere else, and puts them into an imports
subdirectory.
