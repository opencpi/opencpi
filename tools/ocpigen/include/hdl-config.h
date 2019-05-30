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

#ifndef HDL_CONFIG_H
#define HDL_CONFIG_H
#include <assert.h>
#include <vector>
#include <list>
#include "hdl-platform.h"
#include "hdl-card.h"


struct DevInstance {
  const Device &device;
  const Card *card;
  const Slot *slot;
  bool m_control;
  const DevInstance *m_parent;
  std::string m_name;
  mutable std::vector<uint64_t> m_connected;
  OCPI::Util::Assembly::Properties m_instancePVs; // parameters beyond those spec'd in platform
  Worker *m_worker; // worker that is parameterized (sort of redundant with assy instance)
  DevInstance(const Device &d, const Card *c, const Slot *s, bool control,
	      const DevInstance *parent);
  const char *cname() const { return m_name.c_str(); }
};

typedef std::list<DevInstance> DevInstances;
typedef DevInstances::const_iterator DevInstancesIter;

#define HDL_CONFIG_ATTRS "platform", "sdpWidth", "constraints"
#define HDL_CONFIG_ELEMS "cpmaster", "nocmaster", "device", "property", "signal"

typedef std::vector<const Card*> Plugged;

class HdlHasDevInstances {
  Worker &m_parent;
protected:
  const HdlPlatform  &m_platform;
  Plugged      &m_plugged;
  DevInstances  m_devInstances; // instantiated in this config (or container)
 HdlHasDevInstances(const HdlPlatform &platform, Plugged &plugged, Worker &parent)
   : m_parent(parent), m_platform(platform), m_plugged(plugged) {}
  DevInstances &devInstances() { return m_devInstances; }
  const char *
  parseDevInstances(ezxml_t xml, const char *parentFile, Worker *parent,
		    DevInstances *baseInstances);
  const char *
  parseDevInstance(const char *device, ezxml_t x, const char *parentFile, Worker *parent,
		   bool control,
		   DevInstances *baseInstances, // other list to look at
		   const DevInstance **result,  // whether to report the result, and
		                                // if !NULL, its ok for it to already exist
		   bool *inBase);               // whether it was found in the base list

  const DevInstance *
  findDevInstance(const Device &dev, const Card *card, const Slot *slot,
		  DevInstances *baseInstances, bool *inBase);
  const char *
  addDevInstance(const Device &dev, const Card *, const Slot *slot, bool control,
		 const DevInstance *parent, DevInstances *baseInstances,
		 ezxml_t xml, const DevInstance *&devInstance);
  void emitSubdeviceConnections(std::string &assy, DevInstances *baseInstances);
};

class HdlContainer;
class HdlConfig : public Worker, public HdlHasDevInstances {
  friend class HdlContainer;
  const HdlPlatform &m_platform;
  Plugged      m_plugged;      // what card is in each slot in this configuration
  size_t       m_sdpWidth;
public:
  static HdlConfig *
  create(ezxml_t xml, const char *knownPlatform, const char *xfile, Worker *parent,
	 const char *&err);
  HdlConfig(HdlPlatform &pf, ezxml_t xml, const char *xfile, Worker *parent, const char *&err);
  virtual ~HdlConfig();

  size_t sdpWidth() { return m_sdpWidth; }
  const HdlPlatform &platform() { return m_platform; }
  const char
    *addControlConnection(std::string &assy),
    *emitConfig(FILE *f);
};

#endif
