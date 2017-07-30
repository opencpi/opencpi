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
