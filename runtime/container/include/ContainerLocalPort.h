/*
 * This file is protected by Copyright. Please refer to the COPYRIGHT file
 * distributed with this source distribution.
 *
 * This file is part of OpenCPI <http://www.opencpi.org>
 *
 * OpenCPI is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * OpenCPI is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef CONTAINER_LOCAL_PORT_H
#define CONTAINER_LOCAL_PORT_H

#include "OcpiContainerApi.h"

#include "OcpiUtilSelfMutex.h"
#include "OcpiPValue.h"
#include "OcpiRDTInterface.h"
#include "OcpiUtilPort.h"
#include "OcpiParentChild.h"
#include "ContainerLauncher.h"
#include "ContainerBasicPort.h"

namespace OCPI {
  namespace Container {

    class Worker;
    class ExternalPort;
    class Container;

    // This class is the behavior common to both ports of local member workers,
    // as well as external ports in this process.
    // The common behavior includes:
    // Possible connection to remote ports outside the process
    // Possible connection to a multi-member crew inside or outside
    // Note that "BasicPort" is shared with BridgePort, but
    // LocalPorts actually own bridge ports (not parent/child yet).
    class BridgePort;
    class LocalPort : public BasicPort {
      friend class Container;
      friend class LocalLauncher;
      friend class Worker;
      friend class BridgePort;
      friend class Port;
      // Modes of bridge processing.
      // Input side has only: Cyclic, CyclicModulo, AsAvailable
      // Output side has more.
      enum BridgeMode {
	Cyclic,         // input: receive from output members in range
	                // output: rotate through all inputs in range
	CyclicSparse,   // output: rotate through all inputs, discard if not in set
	CyclicModulo,   // output: rotate through all inputs, skipping our crew size
	                //         module other crew size
	                // input:  ditto
	AsAvailable,    // input: receive from set as available, rotating the look
	All,            // output: send to all in the specified set
	Balanced,       // output: if least busy is in specified set, send, else discard
	Directed,       // output: take input member from API
	Hashed,         // output: compute input member based on hash of m_hashField
	Discard,        // output: discard messages
	ModeLimit
      };
      size_t                         m_scale;
      LocalPort                     *m_external;            // inserted external port
      // State relating to having bridge ports (member talking to multiple other members)
      std::vector<BridgePort*>       m_bridgePorts;
      unsigned                       m_connectedBridgePorts;// count to know when all are ready
      struct BridgeOp {
	size_t
	  m_first,  // first opposite member to deal with
	  m_last,   // last opposite member to deal with
	  m_next;   // next opposite member to deal with
	const OCPI::Util::Member *m_hashField;
	BridgeMode m_mode;
      } *m_bridgeOp;
      std::vector<BridgeOp> m_bridgeOps;
      // These are the functions to set up the bridge op according distribution and scale
      // on both sides, per op.

      typedef void BridgeSetup(Launcher::Connection &c, const OCPI::Util::Port &output,
			       const OCPI::Util::Port &input, unsigned op, BridgeOp &bo);
      static BridgeSetup
	*bridgeModes[OCPI::Util::Port::DistributionLimit] [OCPI::Util::Port::DistributionLimit][2],
	oAllP, oCycP, oFirst2, oBalP, oHashP, oAll, oCycMod, oFirst, oBal, oHash, oDirect,
	iOneP, iFirst2, iAny, iFirst, iFirstCyc, iCycMod, ioCyc,
	bad;

      // State of the current local buffer (external or member)
      ExternalBuffer                *m_localBuffer;         // current available buffer
      OCPI::Util::Port::Distribution m_localDistribution;   // distribution for current opcode
      // Indexing in bridges for current local buffer
      unsigned                       m_firstBridge;         // first one for current local buf
      unsigned                       m_currentBridge;       // current bridge for local buf
      unsigned                       m_nextBridge;          // next one to use for any op
    protected:
      LocalPort(Container &container, const OCPI::Util::Port &mPort, bool isProvider,
		const OCPI::Util::PValue *params);
      virtual ~LocalPort();
    private:
      // Is this port actually *operating* in this process, with its buffers in this process?
      // Essentially ports are either:
      // - in this process, with buffers here
      // - local, meaning managed from in this process, but with the buffers elsewhere
      // - remote, meaning managed and owned in another process
      // "other" is the other local maybe-in-process port involved
      // "other" being NULL means the other port is remote in another process
      virtual bool isInProcess(LocalPort *other) const = 0;
      bool getLocalBuffer();
      void setupBridging(Launcher::Connection &c);
      void determineBridgeOp(Launcher::Connection &c, const OCPI::Util::Port &output,
			     const OCPI::Util::Port &input, unsigned op, BridgeOp &bo);
    protected:
      bool initialConnect(Launcher::Connection &c);
      bool finalConnect(Launcher::Connection &c);
      //      void insertExternal(Launcher::Connection &c);
      virtual bool canBeExternal() const = 0;
      void prepareOthers(size_t nOthers, size_t myCrewSize);
      void runBridge(); // flow data between this port and its bridge ports
    public:
      size_t nOthers() const { return m_bridgePorts.size(); }
#if 0
      void applyConnectParams(const OCPI::RDT::Descriptors *other,
			      const OCPI::Util::PValue *params);
      // These may involve bridge ports.  The return value may point to the supplied buf.
      // The "done" output says whether this port needs anything more to be operational
      const OCPI::RDT::Descriptors
        *initialConnect(const Launcher::Connection &c, const OCPI::RDT::Descriptor *other,
			OCPI::RDT::Descriptors &buf, bool &done),
	*finalConnect(const Launcher::Connection &c, const OCPI::RDT::Descriptors *other,
		      OCPI::RDT::Descriptors &buf, bool &done);
#endif
#if 0
      // Remote connection: up to 5 steps! worst case.
      // Names are chosen for worst case.
      // "final" means all needed info, implying resource commitments
      // "initial" can mean only choices, without resource commitments
      //           but may contain "final" info in some nice cases.

      // Step 1:
      // Get initial info/choices about this local (provider) port
      // Worst case we have:
      //     only provider target choices, no commitments
      //     no provider->user information at all
      // Best case: (only one method for user->provider)
      //     final user->provider target info and provider->user target info
      bool getInitialProviderInfo(Launcher::Connection &c);

      // Step 2: (after passing initialProviderInfo to user side)
      // Give remote initial provider info to this local user port.
      // Get back user info:
      // Worst case:
      //     final user source info (user-source FIXED at user: 1 of 8))
      //     initial user target info/choices
      // Best case:
      //     we're done.  no further info exchange needed, return 0;
      bool setInitialProviderInfo(Launcher::Connection &c);

      // Step 3: (after passing initialUserInfo to provider side)
      // Give remote initial user info to this local provider port.
      // Get back final provider info:
      // Worst case:
      //     (user-source FIXED at provider: 2 of 8)
      //     (provider-target FIXED at provider: 3 of 8)
      //     (provider-source FIXED at provider: 4 of 8)
      // Best case:
      //     we're done, no further info exchange needed, return 0;
      bool setInitialUserInfo(Launcher::Connection &c);

      // Step 4: (after passing finalProviderInfo to user side)
      // Give remote final provider info to this local user port.
      // Get back final user info:
      // Worst case:
      //     (provider-target FIXED at user: 5 of 8)
      //     (provider-source FIXED at user: 6 of 8)
      //     (user-target FIXED at user: 7 of 8)
      //     final provider->user target info
      // Best case:
      //     we're done, no further info exchange needed, return 0;
      bool setFinalProviderInfo(Launcher::Connection &c);

      // Step 5: (after passing finalUserInfo to provider side)
      // Worst case:
      //     (user-target FIXED at provider: 8 of 8);
      void setFinalUserInfo(Launcher::Connection &c);
#endif
    };

  }
}
#endif



