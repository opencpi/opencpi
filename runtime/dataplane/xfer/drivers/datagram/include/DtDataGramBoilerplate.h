// Poor man's template for datagram RDMA drivers
// Until there is time to do this better with real templates, etc.
// We just include this stuff inside the namespace for a particular datagram driver.
// XF and DG are defined namespace abbreviations


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// +++ Begin boilerplate
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

class Device : public DataTransfer::DeviceBase<XferFactory,Device> {
  Device(const char* name)
    : XF::DeviceBase<XferFactory,Device>(name, *this) {}
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
  XferRequest(XferServices &parent)
    : TransferBase<XferServices,XferRequest,DG::XferRequest>(parent, *this) {}
};
XF::XferRequest *XferServices::
createXferRequest() {
  return new XferRequest(*this);
}
XF::XferServices &XferFactory::
createXferServices(XF::EndPoint &source, XF::EndPoint &target) {
  XferServices &xfs = *new XferServices(source, target);
  static_cast<EndPoint *>(&source)->addXfer(xfs);
  return xfs;
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
