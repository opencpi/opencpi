This tree holds code and tools specific to the HDL model.

The "prims" directory supplies low level/primitive utility libraries
and cores used to build hdl workers and other infrastructure.

The "apps" directory holds application configurations consisting of
workers, which will build/synthesize whole "apps" ready to be put into bitstream builds.

HDL OCPI components are not built here since they are built under the
top level "components" directory where multiple model implementations
of the same components are built.

At this time all the building is for synthesis only, not simulation.


