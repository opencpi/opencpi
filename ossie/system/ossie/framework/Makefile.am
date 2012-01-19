lib_LTLIBRARIES = libossiecf.la
libossiecf_la_SOURCES = ApplicationFactory_impl.cpp File_impl.cpp Application_impl.cpp FileManager_impl.cpp PortSupplier_impl.cpp Device_impl.cpp FileSystem_impl.cpp PropertySet_impl.cpp DeviceManager_impl.cpp LifeCycle_impl.cpp Resource_impl.cpp DomainManager_impl.cpp ExecutableDevice_impl.cpp LoadableDevice_impl.cpp TestableObject_impl.cpp applicationSupport.cpp helperFunctions.cpp ossieComponent.cpp

EXTRA_DIST = ossiecf.cpp dirent.h

libossiecf_la_LIBADD = $(BOOST_FILESYSTEM_LIB)
libossiecf_la_LDFLAGS = -version-info 0:4:0

AM_CXXFLAGS = $(BOOST_CPPFLAGS) -Wall -g
AM_LDFLAGS = $(BOOST_LDFLAGS)
INCLUDES = -I../include -I../parser

