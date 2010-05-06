#include "CpiContainerPort.h"
#include "CpiWorker.h"
#include "CpiArtifact.h"
#include "CpiApplication.h"
#include "CpiContainerMisc.h"
#include "CpiPValue.h"

namespace CPI {
  namespace Container {
    namespace CM = CPI::Metadata;

    BasePort::BasePort(CPI::Metadata::Port & metaData ) :
      PortData(metaData), myDesc(connectionData.data.desc)
    {
      // FIXME: put these in the default PortData constructor
      myDesc.nBuffers = DEFAULT_NBUFFERS;
      myDesc.dataBufferSize = DEFAULT_BUFFER_SIZE;
      CPI::RDT::Descriptors &d = connectionData.data;
      d.type = myMetaPort.provider ? CPI::RDT::ConsumerDescT : CPI::RDT::ProducerDescT;
      d.role = CPI::RDT::NoRole;
    }
    BasePort::~BasePort(){}


#ifdef WAS    
    // REMOVE ME!! for interim merge use only
    static CPI::Metadata::Port tmp;
    Port::Port(Worker &w, bool provider)
      : BasePort("Invalid", provider),
        CPI::Util::Child<Worker,Port>(w),
        myMetaPort(tmp), 
        myContainer(*myParent->myParent->myParent)
    {
      myMetaPort.provider = provider;
    }
#endif


    // This base class constructor for generic initialization
    Port::Port(Worker &w, CPI::Metadata::Port &mPort) :
      BasePort( mPort ),
      CPI::Util::Child<Worker,Port>(w),
      canBeExternal(true),
      myContainer(*myParent->myParent->myParent)
    {
      uint32_t n;
      bool found;
      
      // Some of the tests use the default values of the meta-port so there may
      // not be xml associated with it.
      if ( mPort.myXml ) {
        n = getAttrNum(mPort.myXml, "minBufferSize", true, &found);
        if (found)
          myMetaPort.minBufferSize = n;
        n = getAttrNum(mPort.myXml, "maxBufferSize", true, &found);
        if (found)
          myMetaPort.maxBufferSize = n; // no max if not specified.
        n = getAttrNum(mPort.myXml, "minNumBuffers", true, &found);
        if (found)
          myMetaPort.minBufferCount = n;
      }
      connectionData.port = (intptr_t)this;
    }

    void Port::loopback(Port &) {}

    bool Port::hasName(const char *name) {
      return (name == myMetaPort.name );
    }

    // The default behavior is that there is nothing special to do between
    // ports of like containers.
    bool Port::connectLike(Port &other, CPI::Util::PValue *myProps,  CPI::Util::PValue *otherProps) {
      return false;
    }

    // The general case of connecting ports that are in the same process.
    void Port::connect(Port &other, CPI::Util::PValue *myProps, CPI::Util::PValue *otherProps) {
      if (isProvider())
        if (other.isProvider())
          throw ApiError("Cannot connect two provider ports", NULL);
        else
          other.connect( *this, otherProps, myProps);
      else if (!other.isProvider()) {
        throw ApiError("Cannot connect to user ports", NULL);
      }
      else {
        Interface
          *myContainer = myParent->myParent->myParent,
          *pContainer = other.myParent->myParent->myParent;
        // Containers know how to do internal connections
        if (myContainer == pContainer) {
          connectInside(other, myProps, otherProps);
        }
        // Container MAY know how to do intercontainer connections between like containers.
        else if (&myContainer->myParent == &pContainer->myParent &&
                 connectLike( other, myProps, otherProps)){

          // empty ??

        }
        // We're in different containers managed locally.  Perform the connection protocol
        else {
          // FIXME:  a more "binary" way to do this locally?
          const std::string
            &ipi = other.getInitialProviderInfo(otherProps),
            &iui = setInitialProviderInfo(myProps, ipi);
          if (!iui.empty()) {
            const std::string &fpi = other.setInitialUserInfo(iui);
            if (!fpi.empty()) {
              const std::string &fui = setFinalProviderInfo(fpi);
              if (!fui.empty())
                other.setFinalUserInfo(fui);
            }
          }
        }
      }
    }

    // Start the remote/intercontainer connection process
    // FIXME: Need simpler protocol to connect between containers in same process
    // without all this pack/unpack overhead.
    // We do not set up the OCDP here since we don't know everything.
    const std::string &Port::getInitialProviderInfo(CPI::Util::PValue *props) {
      cpiAssert(isProvider());
      applyConnectParams(props);
      myInitialPortInfo = myContainer.packPortDesc(*this);
      return myInitialPortInfo;
    }

    // User side initial method, that carries provider info and returns user info
    const std::string &Port::setInitialProviderInfo(CPI::Util::PValue *props, const std::string &ipi) {
      // User side, producer side.
      cpiAssert(!isProvider());
      PortData otherPortData;
      myContainer.unpackPortDesc(ipi, &otherPortData);
      // Adjust any parameters from connection metadata
      applyConnectParams(props);
      // We now know the role aspects of both sides.  Make the decision so we know what
      // resource allocations to make in finishConnection.
      establishRoles(otherPortData.connectionData.data);
      finishConnection(otherPortData.connectionData.data);
      // We're done but other size still needs info.
      // FIXME done here?
      myInitialPortInfo = myContainer.packPortDesc(*this);
      return myInitialPortInfo;
    }

    const std::string Port::empty;    // Provider Only
    const std::string &Port::setInitialUserInfo(const std::string &iui) {
      cpiAssert(isProvider());
      PortData otherPortData;
      myContainer.unpackPortDesc(iui, &otherPortData);
      establishRoles(otherPortData.connectionData.data);
      // Adjust any parameters from connection metadata
      finishConnection(otherPortData.connectionData.data);
      return empty;
    }

    // User only
    const std::string &Port::setFinalProviderInfo(const std::string &) {
      return Port::empty;
    }
    // Provider Only
    void Port::setFinalUserInfo(const std::string &) {
    }
    // Establish the roles, which might happen earlier than the finalization of the connection
    // Since roles can determine resource allocations
    // This could be table-driven...
    void Port::establishRoles(CPI::RDT::Descriptors &other) {
      CPI::RDT::Descriptors
        &pDesc = isProvider() ? connectionData.data : other,
        &uDesc = isProvider() ? other : connectionData.data;
      static char *roleName[] =
        {"NoRole", "ActiveMessage", "ActiveFlowControl", "ActiveOnly", "Passive", "MaxRole"};
      printf("Port %s, a %s, has options 0x%x, initial role %s\n"
             "  other has options 0x%x, initial role %s\n",
             myMetaPort.name, isProvider() ? "provider/consumer" : "user/producer",
             connectionData.data.options, roleName[connectionData.data.role],
             other.options, roleName[other.role]);
      chooseRoles(uDesc.role, uDesc.options, pDesc.role, pDesc.options);
      printf("  after negotiation, port %s, a %s, has role %s\n"
             "  other has role %s\n",
             myMetaPort.name, isProvider() ? "provider/consumer" : "user/producer",
             roleName[connectionData.data.role], roleName[other.role]);
      // We must make sure other side doesn't mess with roles anymore.
      uDesc.options |= 1 << CPI::RDT::MandatedRole;
      pDesc.options |= 1 << CPI::RDT::MandatedRole;
    }
    // Convert PValues into descriptor values, with constraint checking
    // A static method to be able use in other contexts for objects that are not this class.
    void BasePort::setConnectParams(CPI::Util::PValue *props) {
      if (!props)
        return;
      for (CPI::Util::PValue *p = props; p->name; p++) {
        if (strcmp(p->name, "bufferCount") == 0) {
          if (p->type != CM::Property::CPI_ULong)
            throw ApiError("bufferCount property has wrong type, should be ULong", NULL);
          if (p->vULong < myMetaPort.minBufferCount)
            throw ApiError("bufferCount is below worker's minimum", NULL);
          myDesc.nBuffers = p->vULong;
        } else if (strcmp(p->name, "bufferSize") == 0) {
          if (p->type != CM::Property::CPI_ULong)
            throw ApiError("bufferSize property has wrong type, should be ULong", NULL);
          if (p->vULong < myMetaPort.minBufferSize)
            throw ApiError("bufferSize is below worker's minimum", NULL);
          if (myMetaPort.maxBufferSize && p->vULong > myMetaPort.maxBufferSize)
            throw ApiError("bufferSize exceeds worker's maximum", NULL);
          myDesc.dataBufferSize = p->vULong;
        } else if (strcmp(p->name, "xferRole") == 0 && p->vString) {
          if (p->type != CM::Property::CPI_String)
            throw ApiError("xferRole property has wrong type, should be string", NULL);
          CPI::RDT::PortRole role;
          if (!strcmp(p->vString, "passive"))
            role = CPI::RDT::Passive;
          else if (!strcmp(p->vString, "active"))
            role = CPI::RDT::ActiveMessage;
          else if (!strcmp(p->vString, "flowcontrol"))
            role = CPI::RDT::ActiveFlowControl;
          else if (!strcmp(p->vString, "activeonly"))
            role = CPI::RDT::ActiveOnly;
          else
            throw ApiError("xferRole property must be passive|active|flowcontrol|activeonly", NULL);
          if (!(connectionData.data.options & (1 << role)))
            throw ApiError("xferRole of \"%s\" not supported by port \"%s\"", p->vString, myMetaPort.name);
          connectionData.data.role = role;
        }
      }
    }
    // Default = we don't check or do anything here as generic parameters are set.
    void BasePort::checkConnectParams() {}
    // Do the work on this port when connection properties are specified.
    // This is still prior to receiving info from the other side, thus this is not necessarily
    // the final info.
    // FIXME: we should error check against bitstream-fixed parameters
    void BasePort::applyConnectParams(CPI::Util::PValue *props) {
      setConnectParams(props);
      checkConnectParams();
    }
    // coming in, specified roles are preferences or explicit instructions.
    // The existing settings are either NoRole, a preference, or a mandate
    void BasePort::chooseRoles(int32_t &uRole, uint32_t uOptions, int32_t &pRole, uint32_t pOptions) {
      // FIXME this relies on knowledge of the values of the enum constants
      static CPI::RDT::PortRole otherRoles[] =
        {CPI::RDT::NoRole, CPI::RDT::ActiveFlowControl, CPI::RDT::ActiveMessage,
         CPI::RDT::Passive, CPI::RDT::ActiveOnly};
      CPI::RDT::PortRole
        pOther = otherRoles[pRole],
        uOther = otherRoles[uRole];
      if (pOptions & (1 << CPI::RDT::MandatedRole)) {
        // provider has a mandate
        cpiAssert(pRole != CPI::RDT::NoRole);
        if (uRole == pOther)
          return;
        if (uOptions & (1 << CPI::RDT::MandatedRole))
          throw ApiError("Incompatible mandated transfer roles", NULL);
        if (uOptions & (1 << pOther)) {
          uRole = pOther;
          return;
        }
        throw ApiError("No compatible role available against mandated role", NULL);
      } else if (pRole != CPI::RDT::NoRole) {
        // provider has a preference
        if (uOptions & (1 << CPI::RDT::MandatedRole)) {
          // user has a mandate
          cpiAssert(uRole != CPI::RDT::NoRole);
          if (pRole == uOther)
            return;
          if (pOptions & (1 << uOther)) {
            pRole = uOther;
            return;
          }
          throw ApiError("No compatible role available against mandated role", NULL);
        } else if (uRole != CPI::RDT::NoRole) {
          // We have preferences on both sides, but no mandate
          // If preferences match, all is well
          if (pRole == uOther)
            return;
          // If one preference is against push, we better listen to it.
          if (uRole == CPI::RDT::ActiveFlowControl &&
              pOptions & (1 << CPI::RDT::ActiveMessage)) {
            pRole = CPI::RDT::ActiveMessage;
            return;
          }
          // Let's try active push if we can
          if (uRole == CPI::RDT::ActiveMessage &&
              pOptions & (1 << CPI::RDT::ActiveFlowControl)) {
            pRole = CPI::RDT::ActiveFlowControl;
            return;
          }
          if (pRole == CPI::RDT::ActiveFlowControl &&
              uOptions & (1 << CPI::RDT::ActiveMessage)) {
            uRole = CPI::RDT::ActiveFlowControl;
            return;
          }
          // Let's try activeonly push if we can
          if (uRole == CPI::RDT::ActiveOnly &&
              pOptions & (1 << CPI::RDT::Passive)) {
            pRole = CPI::RDT::Passive;
            return;
          }
          if (pRole == CPI::RDT::Passive &&
              pOptions & (1 << CPI::RDT::ActiveOnly)) {
            pRole = CPI::RDT::ActiveOnly;
            return;
          }
          // Let's give priority to the "better" role.
          if (uRole < pRole &&
              pOptions & (1 << uOther)) {
            pRole = uOther;
            return;
          }
          // Give priority to the provider
          if (uOptions & (1 << pOther)) {
            uRole = pOther;
            return;
          }
          if (pOptions & (1 << uOther)) {
            pRole = uOther;
            return;
          }
          // Can't use either preference.  Fall throught to no mandates, no preferences
        } else {
          // User role unspecified, but provider has a preference
          if (uOptions & (1 << pOther)) {
            uRole = pOther;
            return;
          }
          // Can't use provider preference, Fall through to no mandates, no preferences
        }
      } else if (uOptions & (1 << CPI::RDT::MandatedRole)) {
        // Provider has no mandate or preference, but user has a mandate
        if (pOptions & (1 << uOther)) {
          pRole = uOther;
          return;
        }
        throw ApiError("No compatible role available against mandated role", NULL);
      } else if (uRole != CPI::RDT::NoRole) {
        // Provider has no mandate or preference, but user has a preference
        if (pOptions & (1 << uOther)) {
          pRole = uOther;
          return;
        }
        // Fall through to no mandates, no preferences.
      }
      // Neither has useful mandates or preferences.  Find anything, biasing to push
      for (int i = 0; i < CPI::RDT::MaxRole; i++)
        // Provider has no mandate or preference
        if (uOptions & (1 << i) &&
            pOptions & (1 << otherRoles[i])) {
          uRole = i;
          pRole = otherRoles[i];
          return;
        }
      throw ApiError("No compatible combination of roles exist", NULL);
    }            
    
  }
}
