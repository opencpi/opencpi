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

// Poor man's template for datagram RDMA drivers
// Until there is time to do this better with real templates, etc.
// We just include this stuff inside the namespace for a particular datagram driver.
// XF and DG are defined namespace abbreviations


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// +++ Begin boilerplate
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

class Device : public DataTransfer::DeviceBase<XferFactory,Device> {
  Device(const char *a_name)
    : XF::DeviceBase<XferFactory,Device>(a_name, *this) {}
};
class XferRequest;
class XferServices : public ConnectionBase<XferFactory,
					   XferServices,
					   XferRequest,
					   DG::XferServices> {
  friend class XferFactory;
protected:
  XferServices(XF::EndPoint &source, XF::EndPoint &target)
    : ConnectionBase<XferFactory,XferServices,XferRequest,
		     DG::XferServices>(*this, source, target){}
  // Here because the driver template classes can't inherit nicely
  XF::XferRequest* createXferRequest();
  uint16_t maxPayloadSize() { return DATAGRAM_PAYLOAD_SIZE; }
};
class XferRequest
  : public TransferBase<XferServices,XferRequest,DG::XferRequest> {
  friend class XferServices;
protected:
  XferRequest(XferServices &a_parent)
    : TransferBase<XferServices,XferRequest,DG::XferRequest>(a_parent, *this) {}
};
XF::XferRequest *XferServices::
createXferRequest() {
  return new XferRequest(*this);
}
XF::XferServices &XferFactory::
createXferServices(XF::EndPoint &source, XF::EndPoint &target) {
  return *new XferServices(source, target);
}
DG::Socket &XferFactory::
createSocket(XF::EndPoint &source) {
  return *new Socket(*static_cast<EndPoint*>(&source));
}
XF::EndPoint &XferFactory::
createEndPoint(const char *protoInfo, const char *eps, const char *other, bool local,
	       size_t size, const OCPI::Util::PValue *params) {
  return *new EndPoint(*this, protoInfo, eps, other, local, size, params);
}
XF::SmemServices &EndPoint::
createSmemServices() { return *new DG::SmemServices(*this); }
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// +++ End boilerplate
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
