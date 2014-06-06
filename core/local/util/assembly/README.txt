This is the runtime or tooltime parsing class to manage assemblies for various purposes.
In general, an assembly has instances of workers or components, with per-instance properties,
connections between the ports of the instances, and external connections that externalize ports of instances.

In some cases assemblies may have workers (implementations), without having instances.
In this case the implication is that the instantiation can happen at runtime.
