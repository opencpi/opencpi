#ifndef OCPIRH_PROXY_BASE_H
#define OCPIRH_PROXY_BASE_H

#include <boost/thread.hpp>
#include <ossie/Component.h>
#include <ossie/ThreadedComponent.h>
#include <bulkio/bulkio.h>
#include <bulkio/bulkio_in_port.h>
#include <bulkio/bulkio_out_port.h>
#include <PortSupplier_impl.h>
#include <Port_impl.h>

namespace OCPI {
  namespace API {
    class Application;
    class ExternalPort;
  }
}
class ocpirh_proxy_base : public Component, protected ThreadedComponent {
  ENABLE_LOGGING
  OCPI::API::Application *m_application;
  bool m_started;
  // Our property structure that correlates ocpi properties with rh properties
 public:
  struct PropertyBase;
 private:
  std::vector<PropertyBase *> m_properties;
  // Our port structure that correlates ocpi ports with rh ports
  struct PortBase;
  std::vector<PortBase *> m_ports;

protected:
  ocpirh_proxy_base(const char *uuid, const char *label);
  ~ocpirh_proxy_base();
  // These 3 are normally implemented as generated in this base class
  void start() throw (CF::Resource::StartError, CORBA::SystemException);
  void stop() throw (CF::Resource::StopError, CORBA::SystemException);
  void releaseObject() throw (CF::LifeCycle::ReleaseError, CORBA::SystemException);
  // These 2 are delegated to us by the top level class (by our patch)
  void constructor();
  int serviceFunction();
private:
  // Called in our constructor to be consistent with others
  void loadProperties();
  void propChangedS(const std::string *oldValue, const std::string *newValue);
  void propChangedUI(const unsigned *oldValue, const unsigned *newValue);
};

#endif // OCPIRH_PROXY_BASE_H
