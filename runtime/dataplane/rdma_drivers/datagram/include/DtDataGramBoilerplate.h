// Poor man's template for datagram RDMA drivers
// Until there is time to do this better with real templates, etc.
// We just include this stuff inside the namespace for a particular datagram driver.


    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // +++ Begin boilerplate
    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

   class DatagramDevice : public DataTransfer::DeviceBase<DatagramXferFactory,DatagramDevice> {
     DatagramDevice(const char* a_name)
       : DataTransfer::DeviceBase<DatagramXferFactory,DatagramDevice>(a_name, *this) {}
    };
    class DatagramXferRequest;
    class DatagramXferServices :
      public ConnectionBase<DatagramXferFactory,
			    DatagramXferServices,
			    DatagramXferRequest,
			    DataTransfer::DatagramXferServices> {
      friend class DatagramXferFactory;
    protected:
      DatagramXferServices(DatagramSmemServices *a_source, DatagramSmemServices *a_target)
	: ConnectionBase<DatagramXferFactory,DatagramXferServices,DatagramXferRequest,
			 DataTransfer::DatagramXferServices>(*this, a_source, a_target){}
      // Here because the driver template classes can't inherit nicely
      XferRequest* createXferRequest();
      uint16_t maxPayloadSize() { return DATAGRAM_PAYLOAD_SIZE; }
    };
    class DatagramXferRequest :
       public TransferBase<DatagramXferServices,DatagramXferRequest,DataTransfer::DatagramXferRequest>
    {
      friend class DatagramXferServices;
    protected:
      DatagramXferRequest(DatagramXferServices &a_parent)
	: TransferBase<DatagramXferServices,DatagramXferRequest,DataTransfer::DatagramXferRequest>(a_parent, *this) {}
    };
    XferRequest* DatagramXferServices::
    createXferRequest() {
      return new DatagramXferRequest(*this);
    }
   DataTransfer::DatagramXferServices *DatagramXferFactory::
   createXferServices(DatagramSmemServices *source, DatagramSmemServices*target) {
     return new DatagramXferServices(source, target);
   }
    DatagramSocket *DatagramXferFactory::
    createSocket(DatagramSmemServices *smem) {
      return new DatagramSocket(smem);
    }
    EndPoint* DatagramXferFactory::
    createEndPoint(std::string& endpoint, bool local) {
      return new DatagramEndPoint(endpoint, local);
    }
    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // +++ End boilerplate
    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
