lib_LTLIBRARIES = libossieparser.la
libossieparser_la_SOURCES = ComponentAssemblyParser.cpp  Port.cpp ComponentInstantiation.cpp PRFParser.cpp ComponentPlacement.cpp PRFProperty.cpp PRFSimpleProperty.cpp PRFSimpleSequenceProperty.cpp ComponentSupportedInterface.cpp  ProvidesPort.cpp Connection.cpp SADComponentInstantiation.cpp DCDComponentInstantiation.cpp SADComponentPlacement.cpp DCDComponentPlacement.cpp SADHostCollocation.cpp DCDConnectionParser.cpp SADParser.cpp DCDInstantiationProperty.cpp SCDParser.cpp DCDParser.cpp SPDAuthor.cpp DMDParser.cpp SPDImplementation.cpp ExternalPort.cpp SPDParser.cpp FindBy.cpp SPDUsesDevice.cpp InstantiationProperty.cpp orb_wrap.cpp UsesPort.cpp ORB.cpp EventChannelSupport.cpp prop_helpers.cpp tinystr.cpp tinyxml.cpp tinyxmlerror.cpp tinyxmlparser.cpp SADComponentFile.cpp dcd_helpers.cpp PropertyFile.cpp DevicePkgRef.cpp DeviceClass.cpp HWDeviceRegistration.cpp DevicePkg.cpp DPDParser.cpp DASParser.cpp SPDDependency.cpp debug.cpp

pkginclude_HEADERS = tinystr.h tinyxml.h

EXTRA_DIST = ossieparser.cpp 

libossieparser_la_LDFLAGS = -version-info 0:4:0

AM_CXXFLAGS = -Wall -g -DTIXML_USE_STL

INCLUDES = -I../include -I.

