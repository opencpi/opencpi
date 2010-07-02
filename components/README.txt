A component library, consisting of different models built for
different targets

The expectation is that a library has spec XML in the top level, and
subdirectories for each implementation.  So for component abc, there
should he a abc_specs.xml file here, and if there is an rcc
implementation it will be in the directory abc.rcc.  If there is an
HDL implementation, it should should be in the subdirectory abc.hdl.

Active implementations (those are are built) are listed in the Make
variables for each model: RccImplementations, HdlImplementations,
XmImplementations, etc.  Thus this makefile just names this library
and lists implementations to be built.

The Makefile also lists the targets per model.
