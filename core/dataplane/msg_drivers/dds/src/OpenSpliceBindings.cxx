
#ifdef OPENSPLICE_MSG_SUPPORT

#include <OpenSpliceBindings.h>
#include <OcpiOsAssert.h>

using namespace DDS;
namespace OpenSpliceBindings {

#if DDS_USE_EXPLICIT_TEMPLATES
  template class DDS_DCPSUVLSeq < OCPIDDSGenIO::Msg, struct MsgSeq_uniq_>;
#endif

  const char * MsgTypeSupportInterface::_local_id = "IDL:OCPIDDSGenIO/MsgTypeSupportInterface:1.0";

  MsgTypeSupportInterface_ptr MsgTypeSupportInterface::_duplicate (MsgTypeSupportInterface_ptr p)
  {
    if (p) p->m_count++;
    return p;
  }

  DDS::Boolean MsgTypeSupportInterface::_local_is_a (const char * _id)
  {
    if (strcmp (_id, MsgTypeSupportInterface::_local_id) == 0)
      {
	return true;
      }

    typedef DDS::TypeSupport NestedBase_1;

    if (NestedBase_1::_local_is_a (_id))
      {
	return true;
      }

    return false;
  }

  MsgTypeSupportInterface_ptr MsgTypeSupportInterface::_narrow (DDS::Object_ptr p)
  {
    MsgTypeSupportInterface_ptr result = NULL;
    if (p && p->_is_a (MsgTypeSupportInterface::_local_id))
      {
	result = dynamic_cast < MsgTypeSupportInterface_ptr> (p);
	result->m_count++;
      }
    return result;
  }

  MsgTypeSupportInterface_ptr MsgTypeSupportInterface::_unchecked_narrow (DDS::Object_ptr p)
  {
    MsgTypeSupportInterface_ptr result;
    result = dynamic_cast < MsgTypeSupportInterface_ptr> (p);
    result->m_count++;
    return result;
  }

  const char * MsgDataWriter::_local_id = "IDL:OCPIDDSGenIO/MsgDataWriter:1.0";


  DDS::Boolean MsgDataWriter::_local_is_a (const char * _id)
  {
    if (strcmp (_id, MsgDataWriter::_local_id) == 0)
      {
	return true;
      }

    typedef DDS::DataWriter NestedBase_1;

    if (NestedBase_1::_local_is_a (_id))
      {
	return true;
      }

    return false;
  }

  MsgDataWriter_ptr MsgDataWriter::_narrow (DDS::Object_ptr p)
  {
    MsgDataWriter_ptr result = NULL;
    if (p && p->_is_a (MsgDataWriter::_local_id))
      {
	result = dynamic_cast < MsgDataWriter_ptr> (p);
	result->m_count++;
      }
    return result;
  }

  const char * MsgDataReader::_local_id = "IDL:OCPIDDSGenIO/MsgDataReader:1.0";

  MsgDataReader_ptr MsgDataReader::_duplicate (MsgDataReader_ptr p)
  {
    if (p) p->m_count++;
    return p;
  }

  DDS::Boolean MsgDataReader::_local_is_a (const char * _id)
  {
    if (strcmp (_id, MsgDataReader::_local_id) == 0)
      {
	return true;
      }

    typedef DDS::DataReader NestedBase_1;

    if (NestedBase_1::_local_is_a (_id))
      {
	return true;
      }

    return false;
  }

  MsgDataReader_ptr MsgDataReader::_narrow (DDS::Object_ptr p)
  {
    MsgDataReader_ptr result = NULL;
    if (p && p->_is_a (MsgDataReader::_local_id))
      {
	result = dynamic_cast < MsgDataReader_ptr> (p);
	result->m_count++;
      }
    return result;
  }

  MsgDataReader_ptr MsgDataReader::_unchecked_narrow (DDS::Object_ptr p)
  {
    MsgDataReader_ptr result;
    result = dynamic_cast < MsgDataReader_ptr> (p);
    result->m_count++;
    return result;
  }



#include "gapi.h"
#include "gapi_loanRegistry.h"
#include "ccpp_DataReader_impl.h"
#include "ccpp_DataReaderView_impl.h"

  struct OCPIMsgSeq_uniq_ {};
  struct OCPIGenericMsg_ {};
  typedef DDS_DCPSUVLSeq < OCPIGenericMsg_, struct MsgSeq_uniq_> OCPIGenericMsgSeq;


  // DDS Msg TypeSupportFactory Object Body
  DDS::DataWriter_ptr 
  MsgTypeSupportFactory::create_datawriter (gapi_dataWriter handle)
  {
    return new MsgDataWriter_impl(handle);
  }

  DDS::DataReader_ptr 
  MsgTypeSupportFactory::create_datareader (gapi_dataReader handle)
  {
    return new MsgDataReader_impl (handle);
  }



  DDS::DataReaderView_ptr 
  MsgTypeSupportFactory::create_view (gapi_dataReaderView /* handle */)
  {
    return NULL;
  }


  DDS::ReturnCode_t
  MsgTypeSupport::register_type(
				DDS::DomainParticipant_ptr domain,
				const char * type_name) THROW_ORB_EXCEPTIONS
  {
    return TypeSupport_impl::register_type(domain, type_name);
  }

  char *
  MsgTypeSupport::get_type_name() THROW_ORB_EXCEPTIONS
  {
    return TypeSupport_impl::get_type_name();
  }

  // DDS Msg DataWriter_impl Object Body
  MsgDataWriter_impl::MsgDataWriter_impl (
					  gapi_dataWriter handle
					  ) : DDS::DataWriter_impl(handle){}


  DDS::InstanceHandle_t
  MsgDataWriter_impl::register_instance(
					const Msg & instance_data) THROW_ORB_EXCEPTIONS
  {
    return DataWriter_impl::register_instance(&instance_data);
  }

  DDS::ReturnCode_t
  MsgDataWriter_impl::unregister_instance(
					  const Msg & instance_data,
					  DDS::InstanceHandle_t handle) THROW_ORB_EXCEPTIONS
  {
    return DataWriter_impl::unregister_instance(&instance_data, handle);
  }

  DDS::ReturnCode_t
  MsgDataWriter_impl::write(
			    const void * instance_data,
			    DDS::InstanceHandle_t handle) THROW_ORB_EXCEPTIONS
  {
    return DataWriter_impl::write(instance_data, handle);
  }


  // DDS Msg DataReader_impl Object Body
  MsgDataReader_impl::MsgDataReader_impl (
					  gapi_dataReader handle
					  ) 
    : DDS::DataReader_impl(handle)
  {
    // Parent constructor takes care of everything.
  }

  MsgDataReader_impl::~MsgDataReader_impl(void)
  {
    // Parent destructor takes care of everything.
  }

  DDS::ReturnCode_t
  MsgDataReader_impl::read(
			   void *  received_data,
			   DDS::SampleInfoSeq & info_seq,
			   CORBA::Long max_samples,
			   DDS::SampleStateMask sample_states,
			   DDS::ViewStateMask view_states,
			   DDS::InstanceStateMask instance_states) THROW_ORB_EXCEPTIONS
  {
    DDS::ReturnCode_t status;
    
    status = check_preconditions(received_data, info_seq, max_samples);
    if ( status == DDS::RETCODE_OK ) {
      status = DataReader_impl::read(received_data, info_seq, max_samples, sample_states, view_states, instance_states);
    }
    return status;
  }



  DDS::ReturnCode_t
  MsgDataReader_impl::take(
			   void * received_data,
			   DDS::SampleInfoSeq & info_seq,
			   CORBA::Long max_samples,
			   DDS::SampleStateMask sample_states,
			   DDS::ViewStateMask view_states,
			   DDS::InstanceStateMask instance_states) THROW_ORB_EXCEPTIONS
  {
    DDS::ReturnCode_t status;
    status = check_preconditions( received_data, info_seq, max_samples);
    if ( status == DDS::RETCODE_OK ) {
      status = DataReader_impl::take( received_data, info_seq, max_samples, sample_states, view_states, instance_states);
    }
    return status;
  }

  DDS::ReturnCode_t
  MsgDataReader_impl::read_w_condition(
				       void *  received_data,
				       DDS::SampleInfoSeq & info_seq,
				       CORBA::Long max_samples,
				       DDS::ReadCondition_ptr a_condition) THROW_ORB_EXCEPTIONS
  {
    DDS::ReturnCode_t status;
    
    status = check_preconditions(received_data, info_seq, max_samples);
    if ( status == DDS::RETCODE_OK ) {
      status = DataReader_impl::read_w_condition( received_data, info_seq, max_samples, a_condition);
    }
    return status;
  }

  DDS::ReturnCode_t
  MsgDataReader_impl::take_w_condition(
				       void *  received_data,
				       DDS::SampleInfoSeq & info_seq,
				       CORBA::Long max_samples,
				       DDS::ReadCondition_ptr a_condition) THROW_ORB_EXCEPTIONS
  {
    DDS::ReturnCode_t status;
    
    status = check_preconditions(received_data, info_seq, max_samples);
    if ( status == DDS::RETCODE_OK ) {
      status = DataReader_impl::take_w_condition( received_data, info_seq, max_samples, a_condition);
    }
    return status;
  }


  DDS::ReturnCode_t 
  MsgDataReader_impl::read_next_sample(
				       void * received_data,
				       DDS::SampleInfo & sample_info) THROW_ORB_EXCEPTIONS
  {
    return DataReader_impl::read_next_sample(received_data, sample_info);
  }


  DDS::ReturnCode_t 
  MsgDataReader_impl::take_next_sample(
				       void * received_data,
				       DDS::SampleInfo & sample_info) THROW_ORB_EXCEPTIONS
  {
    return DataReader_impl::take_next_sample(received_data, sample_info);
  }


  DDS::ReturnCode_t
  MsgDataReader_impl::read_instance(
				    void *  received_data,
				    DDS::SampleInfoSeq & info_seq,
				    CORBA::Long max_samples,
				    DDS::InstanceHandle_t a_handle,
				    DDS::SampleStateMask sample_states,
				    DDS::ViewStateMask view_states,
				    DDS::InstanceStateMask instance_states) THROW_ORB_EXCEPTIONS
  {
    DDS::ReturnCode_t status;
    
    status = check_preconditions(received_data, info_seq, max_samples);
    if ( status == DDS::RETCODE_OK ) {
      status = DataReader_impl::read_instance( received_data, info_seq, max_samples, a_handle, sample_states, view_states, instance_states);
    }
    return status;
  }

  DDS::ReturnCode_t
  MsgDataReader_impl::take_instance(
				    void * received_data,
				    DDS::SampleInfoSeq & info_seq,
				    CORBA::Long max_samples,
				    DDS::InstanceHandle_t a_handle,
				    DDS::SampleStateMask sample_states,
				    DDS::ViewStateMask view_states,
				    DDS::InstanceStateMask instance_states) THROW_ORB_EXCEPTIONS
  {
    DDS::ReturnCode_t status;
    
    status = check_preconditions(received_data, info_seq, max_samples);
    if ( status == DDS::RETCODE_OK ) {
      status = DataReader_impl::take_instance( received_data, info_seq, max_samples, a_handle, sample_states, view_states, instance_states);
    }
    return status;
  }

  DDS::ReturnCode_t
  MsgDataReader_impl::read_next_instance(
					 void *  received_data,
					 DDS::SampleInfoSeq & info_seq,
					 CORBA::Long max_samples,
					 DDS::InstanceHandle_t a_handle,
					 DDS::SampleStateMask sample_states,
					 DDS::ViewStateMask view_states,
					 DDS::InstanceStateMask instance_states) THROW_ORB_EXCEPTIONS
  {
    DDS::ReturnCode_t status;
    
    status = check_preconditions(received_data, info_seq, max_samples);
    if ( status == DDS::RETCODE_OK ) {
      status = DataReader_impl::read_next_instance( received_data, info_seq, max_samples, a_handle, sample_states, view_states, instance_states);
    }
    return status;
  }

  DDS::ReturnCode_t
  MsgDataReader_impl::take_next_instance(
					 void *  received_data,
					 DDS::SampleInfoSeq & info_seq,
					 CORBA::Long max_samples,
					 DDS::InstanceHandle_t a_handle,
					 DDS::SampleStateMask sample_states,
					 DDS::ViewStateMask view_states,
					 DDS::InstanceStateMask instance_states) THROW_ORB_EXCEPTIONS
  {
    DDS::ReturnCode_t status;
    
    status = check_preconditions(received_data, info_seq, max_samples);
    if ( status == DDS::RETCODE_OK ) {
      status = DataReader_impl::take_next_instance( received_data, info_seq, max_samples, a_handle, sample_states, view_states, instance_states);
    }
    return status;
  }


  DDS::ReturnCode_t 
  MsgDataReader_impl::read_next_instance_w_condition(
						     void * received_data,
						     DDS::SampleInfoSeq & info_seq,
						     CORBA::Long max_samples,
						     DDS::InstanceHandle_t a_handle,
						     DDS::ReadCondition_ptr a_condition) THROW_ORB_EXCEPTIONS
  {
    DDS::ReturnCode_t status;
    
    status = check_preconditions(received_data, info_seq, max_samples);
    if ( status == DDS::RETCODE_OK ) {
      status = DataReader_impl::read_next_instance_w_condition( received_data, info_seq, max_samples, a_handle, a_condition);
    }
    return status;
  }


  DDS::ReturnCode_t 
  MsgDataReader_impl::take_next_instance_w_condition(
						     void * received_data,
						     DDS::SampleInfoSeq & info_seq,
						     CORBA::Long max_samples,
						     DDS::InstanceHandle_t a_handle,
						     DDS::ReadCondition_ptr a_condition) THROW_ORB_EXCEPTIONS
  {
    DDS::ReturnCode_t status;
    
    status = check_preconditions(received_data, info_seq, max_samples);
    if ( status == DDS::RETCODE_OK ) {
      status = DataReader_impl::take_next_instance_w_condition( received_data, info_seq, max_samples, a_handle, a_condition);
    }
    return status;
  }


  DDS::ReturnCode_t
  MsgDataReader_impl::return_loan(
				  void * rd,
				  DDS::SampleInfoSeq & info_seq) THROW_ORB_EXCEPTIONS
  {
    DDS::ReturnCode_t status = DDS::RETCODE_OK;
    OCPIGenericMsgSeq * received_data = reinterpret_cast<OCPIGenericMsgSeq *>(rd);

    if ( received_data->length() > 0 ) {
      if (received_data->length() == info_seq.length() && 
	  received_data->release() == info_seq.release() ) {
	if (!received_data->release()) {
	  status = DataReader_impl::return_loan( received_data->get_buffer(),
						 info_seq.get_buffer() );
	  if ( status == DDS::RETCODE_OK ) {
	    if ( !received_data->release() ) {
	      received_data->replace(0, 0, NULL, false);
	      DDS::SampleInfoSeq::freebuf( info_seq.get_buffer(false) );
	      info_seq.replace(0, 0, NULL, false);
	    }
	  } else if ( status == DDS::RETCODE_NO_DATA ) {
	    if ( received_data->release() ) {
	      status = DDS::RETCODE_OK;
	    } else {
	      status = DDS::RETCODE_PRECONDITION_NOT_MET;
	    }
	  }
	}
      } else {
	status = DDS::RETCODE_PRECONDITION_NOT_MET;
      }
    }
    return status;
  }


  DDS::ReturnCode_t 
  MsgDataReader_impl::get_key_value(
				    Msg & key_holder,
				    DDS::InstanceHandle_t handle) THROW_ORB_EXCEPTIONS
  {
    return DataReader_impl::get_key_value(&key_holder, handle);
  }

  DDS::InstanceHandle_t 
  MsgDataReader_impl::lookup_instance(
				      const Msg & instance) THROW_ORB_EXCEPTIONS
  {
    return DataReader_impl::lookup_instance(&instance);
  }



  DDS::ReturnCode_t 
  MsgDataReader_impl::check_preconditions(
					  void *  rd,
					  DDS::SampleInfoSeq & info_seq,
					  CORBA::Long max_samples)
  {
    DDS::ReturnCode_t status = DDS::RETCODE_PRECONDITION_NOT_MET;
    OCPIGenericMsgSeq * received_data = reinterpret_cast<OCPIGenericMsgSeq *>(rd);
    
    if ( received_data->length() == info_seq.length() &&
	 received_data->maximum() == info_seq.maximum() &&
	 received_data->release() == info_seq.release() ) {
      if ( received_data->maximum() == 0 || received_data->release() ) {
	if (received_data->maximum() == 0 ||
	    max_samples <= static_cast<CORBA::Long>(received_data->maximum()) ||
	    max_samples == DDS::LENGTH_UNLIMITED ) {
	  status = DDS::RETCODE_OK;
	}
      }
    }
    return status;
  }



  void DDSEntityManager::createParticipant(const char *partitiontName)
  {
    domain = NULL;
    dpf = DomainParticipantFactory::get_instance();
    checkHandle(dpf.in(), "DDS::DomainParticipantFactory::get_instance");
    participant = dpf->create_participant(domain, PARTICIPANT_QOS_DEFAULT, NULL,
					  STATUS_MASK_NONE);
    checkHandle(participant.in(),
		"DDS::DomainParticipantFactory::create_participant");
    partition = partitiontName;
  }

  void DDSEntityManager::deleteParticipant()
  {
    status = dpf->delete_participant(participant.in());
    checkStatus(status, "DDS::DomainParticipant::delete_participant ");
  }

  void DDSEntityManager::registerType(TypeSupport *ts)
  {
    typeName = ts->get_type_name();
    status = ts->register_type(participant.in(), typeName);
    checkStatus(status, "register_type");
  }

  void DDSEntityManager::createTopic(const char *topicName)
  {
    status = participant->get_default_topic_qos(reliable_topic_qos);
    checkStatus(status, "DDS::DomainParticipant::get_default_topic_qos");
    reliable_topic_qos.reliability.kind = RELIABLE_RELIABILITY_QOS;
    reliable_topic_qos.durability.kind = TRANSIENT_DURABILITY_QOS;

    /* Make the tailored QoS the new default. */
    status = participant->set_default_topic_qos(reliable_topic_qos);
    checkStatus(status, "DDS::DomainParticipant::set_default_topic_qos");

    /* Use the changed policy when defining the topic */
    topic = participant->create_topic(topicName, typeName, reliable_topic_qos,
				      NULL, STATUS_MASK_NONE);
    checkHandle(topic.in(), "DDS::DomainParticipant::create_topic ()");
  }

  void DDSEntityManager::deleteTopic()
  {
    status = participant->delete_topic(topic);
    checkStatus(status, "DDS.DomainParticipant.delete_topic");
  }

  DDS::Publisher_ptr DDSEntityManager::createPublisher()
  {
    DDS::ReturnCode_t status;
    DDS::Publisher_ptr  publisher;
    status = participant->get_default_publisher_qos(pub_qos);
    checkStatus(status, "DDS::DomainParticipant::get_default_publisher_qos");
    pub_qos.partition.name.length(1);
    pub_qos.partition.name[0] = partition;
    publisher = participant->create_publisher(pub_qos, NULL, STATUS_MASK_NONE);
    checkHandle(publisher, "DDS::DomainParticipant::create_publisher");
    return publisher;
  }

  DDS::Subscriber_ptr DDSEntityManager::createSubscriber()
  {
    int status = participant->get_default_subscriber_qos(sub_qos);
    checkStatus(status, "DDS::DomainParticipant::get_default_subscriber_qos");
    sub_qos.partition.name.length(1);
    sub_qos.partition.name[0] = partition;
    DDS::Subscriber_var subscriber = participant->create_subscriber(sub_qos, NULL, STATUS_MASK_NONE);
    checkHandle(subscriber.in(), "DDS::DomainParticipant::create_subscriber");
    return subscriber;
  }

  Topic_var & DDSEntityManager::getTopic()
  {
    return topic;
  }

  DomainParticipant_ptr DDSEntityManager::getParticipant()
  {
    //    return participant._retn();
    return participant.in();
  }

  DDSEntityManager::~DDSEntityManager(){

  }

  /* Array to hold the names for all ReturnCodes. */
  string RetCodeName[13] = 
    {
      "DDS_RETCODE_OK", "DDS_RETCODE_ERROR", "DDS_RETCODE_UNSUPPORTED", 
      "DDS_RETCODE_BAD_PARAMETER", "DDS_RETCODE_PRECONDITION_NOT_MET", 
      "DDS_RETCODE_OUT_OF_RESOURCES", "DDS_RETCODE_NOT_ENABLED", 
      "DDS_RETCODE_IMMUTABLE_POLICY", "DDS_RETCODE_INCONSISTENT_POLICY", 
      "DDS_RETCODE_ALREADY_DELETED", "DDS_RETCODE_TIMEOUT", "DDS_RETCODE_NO_DATA",
      "DDS_RETCODE_ILLEGAL_OPERATION"
    };

  /**
   * Returns the name of an error code.
   **/
  string getErrorName(DDS::ReturnCode_t status)
  {
    return RetCodeName[status];
  }

  /**
   * Check the return status for errors. If there is an error, then terminate.
   **/
  void checkStatus(DDS::ReturnCode_t status, const char *info)
  {
    if (status != DDS::RETCODE_OK && status != DDS::RETCODE_NO_DATA)
      {
	std::string err("Error in ");
	err += info;
	err += ": ";
	err +=  getErrorName(status);
	throw err;
      }
  }

  /**
   * Check whether a valid handle has been returned. If not, then terminate.
   **/
  void checkHandle(void *handle, string info)
  {    
    if (!handle)
      {
	std::string err("Error in ");
	err += info;
	err += ": Creation failed: invalid handle";
	throw err;	  
      }
  }


}



#endif


