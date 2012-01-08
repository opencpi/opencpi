
/*
 *                         OpenSplice DDS
 *
 *   This software and documentation are Copyright 2006 to 2009 PrismTech
 *   Limited and its licensees. All rights reserved. See file:
 *
 *                     $OSPL_HOME/LICENSE
 *
 *   for full copyright notice and license terms.
 *
 */

#ifndef OCPI_DDSGENERICSUPPORT_H
#define OCPI_DDSGENERICSUPPORT_H

#include "ccpp_dds_dcps.h"
#include <iostream>
#include "ccpp.h"
#include <orb_abstraction.h>
#include "sacpp_mapping.h"
#include "sacpp_DDS_DCPS.h"
#include <c_base.h>
#include <c_misc.h>
#include <c_sync.h>
#include <c_collection.h>
#include <c_field.h>
#include "dds_dcps_interfaces.h"
#include "ccpp_TypeSupport_impl.h"
#include "ccpp_DataWriter_impl.h"
#include "ccpp_DataReader_impl.h"
#include "ccpp_DataReaderView_impl.h"

using namespace std;


namespace  OCPI {
  namespace Msg  {
    namespace DDS {
      class Topic;
    }
  }
}

namespace OpenSpliceBindings {


  class DDSEntityManager
  {

    /* Generic DDS entities */
    DDS::DomainParticipantFactory_var dpf;
    DDS::DomainParticipant_var participant;
    DDS::Topic_var topic;

    /* QosPolicy holders */
    DDS::TopicQos setting_topic_qos;
    DDS::PublisherQos pub_qos;
    DDS::DataWriterQos dw_qos;
    DDS::SubscriberQos sub_qos;
    DDS::TopicQos reliable_topic_qos;

    DDS::DomainId_t domain;
    DDS::InstanceHandle_t userHandle;
    DDS::ReturnCode_t status;

    CORBA::String_var partition;
    CORBA::String_var typeName;
    //    OCPI::Msg::DDS::TopicData m_td;

  public:

    inline DDS::TopicQos & topicQos(){return reliable_topic_qos;}

    // One per process
    void createParticipant(const char *partitiontName);
    void deleteParticipant();
    DDS::DomainParticipant_ptr getParticipant();

    // One per topic
    DDSEntityManager(){};

    void registerType(DDS::TypeSupport *ts);
    void createTopic(const char *topicName);
    void deleteTopic();
    DDS::Topic_var & getTopic();

    // One per port
    DDS::Publisher_ptr createPublisher();
    DDS::Subscriber_ptr createSubscriber();

    ~DDSEntityManager();
  };



  /**
   * Returns the name of an error code.
   **/
  const char *getErrorName(DDS::ReturnCode_t status);

  /**
   * Check the return status for errors. If there is an error, then terminate.
   **/
  void checkStatus(DDS::ReturnCode_t status, const char *info);

  /**
   * Check whether a valid handle has been returned. If not, then terminate.
   **/
  void checkHandle(void *handle, string info);


  class MsgTypeSupportInterface;
  typedef MsgTypeSupportInterface * MsgTypeSupportInterface_ptr;
  typedef DDS_DCPSInterface_var < MsgTypeSupportInterface> MsgTypeSupportInterface_var;
  typedef DDS_DCPSInterface_out < MsgTypeSupportInterface> MsgTypeSupportInterface_out;

  class MsgDataWriter;
  typedef MsgDataWriter * MsgDataWriter_ptr;
  typedef DDS_DCPSInterface_var < MsgDataWriter> MsgDataWriter_var;
  typedef DDS_DCPSInterface_out < MsgDataWriter> MsgDataWriter_out;

  class MsgDataReader;
  typedef MsgDataReader * MsgDataReader_ptr;
  typedef DDS_DCPSInterface_var < MsgDataReader> MsgDataReader_var;
  typedef DDS_DCPSInterface_out < MsgDataReader> MsgDataReader_out;

  class MsgDataReaderView;
  typedef MsgDataReaderView * MsgDataReaderView_ptr;
  typedef DDS_DCPSInterface_var < MsgDataReaderView> MsgDataReaderView_var;
  typedef DDS_DCPSInterface_out < MsgDataReaderView> MsgDataReaderView_out;

  // Our generic message types used to integrate to the OpenSlice IDL binding library
  struct Msg{uint8_t  data;};
  struct MsgSeq_uniq_ {};
  typedef DDS_DCPSUVLSeq < Msg, MsgSeq_uniq_ > MsgSeq;
  typedef DDS_DCPSSequence_var < MsgSeq> MsgSeq_var;
  typedef DDS_DCPSSequence_out < MsgSeq> MsgSeq_out;

  class MsgDataWriter : virtual public DDS::DataWriter
  { 
  public:
    typedef MsgDataWriter_ptr _ptr_type;
    typedef MsgDataWriter_var _var_type;

    DDS::Boolean _local_is_a (const char * id);

    static MsgDataWriter_ptr _narrow (DDS::Object_ptr obj);

    static const char * _local_id;
    MsgDataWriter_ptr _this () { return this; }

    virtual DDS::InstanceHandle_t register_instance (const Msg& instance_data) = 0;
    virtual DDS::ReturnCode_t unregister_instance (const Msg& instance_data, DDS::InstanceHandle_t handle) = 0;
    virtual DDS::ReturnCode_t write (const void * instance_data, DDS::InstanceHandle_t handle) = 0;

  protected:
    MsgDataWriter () {};
    ~MsgDataWriter () {};
  private:
    MsgDataWriter (const MsgDataWriter &);
    MsgDataWriter & operator = (const MsgDataWriter &);
  };


  class  MsgDataWriter_impl : public virtual MsgDataWriter,
    public ::DDS::DataWriter_impl
      {
      public:
    
	virtual ::DDS::InstanceHandle_t register_instance( const Msg & instance_data) THROW_ORB_EXCEPTIONS;
	virtual ::DDS::ReturnCode_t unregister_instance(
							const Msg & instance_data,
							::DDS::InstanceHandle_t handle) THROW_ORB_EXCEPTIONS;    
	virtual ::DDS::ReturnCode_t write(
					  const void * instance_data,
					  ::DDS::InstanceHandle_t handle) THROW_ORB_EXCEPTIONS;    
	MsgDataWriter_impl ( gapi_dataWriter handle );    
	virtual ~MsgDataWriter_impl (void){};
    
      private:
	MsgDataWriter_impl(const MsgDataWriter_impl &);
	void operator= (const MsgDataWriter &);
      };

  class MsgTypeSupportInterface : virtual public ::DDS::TypeSupport
  { 
  public:
    typedef MsgTypeSupportInterface_ptr _ptr_type;
    typedef MsgTypeSupportInterface_var _var_type;

    static MsgTypeSupportInterface_ptr _duplicate (MsgTypeSupportInterface_ptr obj);
    ::DDS::Boolean _local_is_a (const char * id);

    static MsgTypeSupportInterface_ptr _narrow (::DDS::Object_ptr obj);
    static MsgTypeSupportInterface_ptr _unchecked_narrow (::DDS::Object_ptr obj);
    static MsgTypeSupportInterface_ptr _nil () { return 0; }
    static const char * _local_id;
    MsgTypeSupportInterface_ptr _this () { return this; }

  protected:
    MsgTypeSupportInterface () {};
    ~MsgTypeSupportInterface () {};
  private:
    MsgTypeSupportInterface (const MsgTypeSupportInterface &);
    MsgTypeSupportInterface & operator = (const MsgTypeSupportInterface &);
  };

    
  class  MsgTypeSupport : public virtual MsgTypeSupportInterface,
    public ::DDS::TypeSupport_impl
  {
  public:
    virtual ::DDS::ReturnCode_t register_type(
					      ::DDS::DomainParticipant_ptr participant,
					      const char * type_name) THROW_ORB_EXCEPTIONS;
	  
    virtual char * get_type_name() THROW_ORB_EXCEPTIONS;    
	  
    MsgTypeSupport ( OCPI::Msg::DDS::Topic * topic );


    virtual ~MsgTypeSupport (void){};
    
  private:
    MsgTypeSupport (const MsgTypeSupport &);
    void operator= (const MsgTypeSupport &);

    //    OCPI::Msg::DDS::TopicData m_data;
    static const char *metaDescriptor;
  };
  typedef MsgTypeSupportInterface_var MsgTypeSupport_var;
  typedef MsgTypeSupportInterface_ptr MsgTypeSupport_ptr;

  class  MsgTypeSupportFactory : public ::DDS::TypeSupportFactory_impl
  {
  private:
    //    OCPI::Msg::DDS::TopicData & m_td;
  public:
    MsgTypeSupportFactory() {}
    virtual ~MsgTypeSupportFactory() {}
  private:
    ::DDS::DataWriter_ptr 
    create_datawriter (gapi_dataWriter handle);
    
    ::DDS::DataReader_ptr 
    create_datareader (gapi_dataReader handle);


    ::DDS::DataReaderView_ptr 
    create_view (gapi_dataReaderView handle);

  };


  class MsgDataReader : virtual public DDS::DataReader
  { 
  public:
    typedef MsgDataReader_ptr _ptr_type;
    typedef MsgDataReader_var _var_type;


    static MsgDataReader_ptr _duplicate (MsgDataReader_ptr obj);
    DDS::Boolean _local_is_a (const char * id);
    static MsgDataReader_ptr _unchecked_narrow (DDS::Object_ptr obj);


    static MsgDataReader_ptr _narrow (DDS::Object_ptr obj);
    static MsgDataReader_ptr _nil () { return 0; }
    static const char * _local_id;
    MsgDataReader_ptr _this () { return this; }

    virtual DDS::ReturnCode_t read (void * received_data, DDS::SampleInfoSeq& info_seq, DDS::Long max_samples, DDS::SampleStateMask sample_states, DDS::ViewStateMask view_states, DDS::InstanceStateMask instance_states) = 0;
    virtual DDS::ReturnCode_t take ( void * received_data, DDS::SampleInfoSeq& info_seq, DDS::Long max_samples, DDS::SampleStateMask sample_states, DDS::ViewStateMask view_states, DDS::InstanceStateMask instance_states) = 0;
    virtual DDS::ReturnCode_t read_w_condition (void * received_data, DDS::SampleInfoSeq& info_seq, DDS::Long max_samples, DDS::ReadCondition_ptr a_condition) = 0;
    virtual DDS::ReturnCode_t take_w_condition (void * received_data, DDS::SampleInfoSeq& info_seq, DDS::Long max_samples, DDS::ReadCondition_ptr a_condition) = 0;
    virtual DDS::ReturnCode_t read_next_sample (void * received_data, DDS::SampleInfo& sample_info) = 0;
    virtual DDS::ReturnCode_t take_next_sample (void * received_data, DDS::SampleInfo& sample_info) = 0;
    virtual DDS::ReturnCode_t read_instance (void * received_data, DDS::SampleInfoSeq& info_seq, DDS::Long max_samples, DDS::InstanceHandle_t a_handle, DDS::SampleStateMask sample_states, DDS::ViewStateMask view_states, DDS::InstanceStateMask instance_states) = 0;
    virtual DDS::ReturnCode_t take_instance (void * received_data, DDS::SampleInfoSeq& info_seq, DDS::Long max_samples, DDS::InstanceHandle_t a_handle, DDS::SampleStateMask sample_states, DDS::ViewStateMask view_states, DDS::InstanceStateMask instance_states) = 0;
    virtual DDS::ReturnCode_t read_next_instance (void * received_data, DDS::SampleInfoSeq& info_seq, DDS::Long max_samples, DDS::InstanceHandle_t a_handle, DDS::SampleStateMask sample_states, DDS::ViewStateMask view_states, DDS::InstanceStateMask instance_states) = 0;
    virtual DDS::ReturnCode_t take_next_instance (void * received_data, DDS::SampleInfoSeq& info_seq, DDS::Long max_samples, DDS::InstanceHandle_t a_handle, DDS::SampleStateMask sample_states, DDS::ViewStateMask view_states, DDS::InstanceStateMask instance_states) = 0;
    virtual DDS::ReturnCode_t read_next_instance_w_condition (void * received_data, DDS::SampleInfoSeq& info_seq, DDS::Long max_samples, DDS::InstanceHandle_t a_handle, DDS::ReadCondition_ptr a_condition) = 0;
    virtual DDS::ReturnCode_t take_next_instance_w_condition (void * received_data, DDS::SampleInfoSeq& info_seq, DDS::Long max_samples, DDS::InstanceHandle_t a_handle, DDS::ReadCondition_ptr a_condition) = 0;
    virtual DDS::ReturnCode_t return_loan (void * received_data, DDS::SampleInfoSeq& info_seq) = 0;
    virtual DDS::ReturnCode_t get_key_value (Msg& key_holder, DDS::InstanceHandle_t handle) = 0;
    virtual DDS::InstanceHandle_t lookup_instance (const Msg& instance) = 0;

  protected:
    MsgDataReader () {};
    ~MsgDataReader () {};
  private:
    MsgDataReader (const MsgDataReader &);
    MsgDataReader & operator = (const MsgDataReader &);
  };


  class  MsgDataReader_impl : public virtual MsgDataReader,
    public ::DDS::DataReader_impl
  {

  private:
    //    OCPI::Msg::DDS::TopicData & m_td;	

  public:

    virtual ::DDS::ReturnCode_t read(
				     void * received_data,
				     ::DDS::SampleInfoSeq & info_seq,
				     CORBA::Long max_samples,
				     ::DDS::SampleStateMask sample_states,
				     ::DDS::ViewStateMask view_states,
				     ::DDS::InstanceStateMask instance_states) THROW_ORB_EXCEPTIONS;
        
    virtual ::DDS::ReturnCode_t take(
				     void * received_data,
				     ::DDS::SampleInfoSeq & info_seq,
				     CORBA::Long max_samples,
				     ::DDS::SampleStateMask sample_states,
				     ::DDS::ViewStateMask view_states,
				     ::DDS::InstanceStateMask instance_states) THROW_ORB_EXCEPTIONS;
        
    virtual ::DDS::ReturnCode_t read_w_condition(
						 void * received_data,
						 ::DDS::SampleInfoSeq & info_seq,
						 CORBA::Long max_samples,
						 ::DDS::ReadCondition_ptr a_condition) THROW_ORB_EXCEPTIONS;
        
    virtual ::DDS::ReturnCode_t take_w_condition(
						 void * received_data,
						 ::DDS::SampleInfoSeq & info_seq,
						 CORBA::Long max_samples,
						 ::DDS::ReadCondition_ptr a_condition) THROW_ORB_EXCEPTIONS;
    
    virtual ::DDS::ReturnCode_t read_next_sample(
						 void * received_data,
						 ::DDS::SampleInfo & sample_info) THROW_ORB_EXCEPTIONS;
    
    virtual ::DDS::ReturnCode_t take_next_sample(
						 void * received_data,
						 ::DDS::SampleInfo & sample_info) THROW_ORB_EXCEPTIONS;
        
    virtual ::DDS::ReturnCode_t read_instance(
					      void * received_data,
					      ::DDS::SampleInfoSeq & info_seq,
					      CORBA::Long max_samples,
					      ::DDS::InstanceHandle_t a_handle,
					      ::DDS::SampleStateMask sample_states,
					      ::DDS::ViewStateMask view_states,
					      ::DDS::InstanceStateMask instance_states) THROW_ORB_EXCEPTIONS;
        
    virtual ::DDS::ReturnCode_t take_instance(
					      void * received_data,
					      ::DDS::SampleInfoSeq & info_seq,
					      CORBA::Long max_samples,
					      ::DDS::InstanceHandle_t a_handle,
					      ::DDS::SampleStateMask sample_states,
					      ::DDS::ViewStateMask view_states,
					      ::DDS::InstanceStateMask instance_states) THROW_ORB_EXCEPTIONS;
        
    virtual ::DDS::ReturnCode_t read_next_instance(
						   void * received_data,
						   ::DDS::SampleInfoSeq & info_seq,
						   CORBA::Long max_samples,
						   ::DDS::InstanceHandle_t a_handle,
						   ::DDS::SampleStateMask sample_states,
						   ::DDS::ViewStateMask view_states,
						   ::DDS::InstanceStateMask instance_states) THROW_ORB_EXCEPTIONS;
        
    virtual ::DDS::ReturnCode_t take_next_instance(
						   void * received_data,
						   ::DDS::SampleInfoSeq & info_seq,
						   CORBA::Long max_samples,
						   ::DDS::InstanceHandle_t a_handle,
						   ::DDS::SampleStateMask sample_states,
						   ::DDS::ViewStateMask view_states,
						   ::DDS::InstanceStateMask instance_states) THROW_ORB_EXCEPTIONS;
        
    virtual ::DDS::ReturnCode_t read_next_instance_w_condition(
							       void * received_data,
							       ::DDS::SampleInfoSeq & info_seq,
							       CORBA::Long max_samples,
							       ::DDS::InstanceHandle_t a_handle,
							       ::DDS::ReadCondition_ptr a_condition) THROW_ORB_EXCEPTIONS;
    
    virtual ::DDS::ReturnCode_t take_next_instance_w_condition(
							       void * received_data,
							       ::DDS::SampleInfoSeq & info_seq,
							       CORBA::Long max_samples,
							       ::DDS::InstanceHandle_t a_handle,
							       ::DDS::ReadCondition_ptr a_condition) THROW_ORB_EXCEPTIONS;
    
    virtual ::DDS::ReturnCode_t return_loan(
					    void * received_data,
					    ::DDS::SampleInfoSeq & info_seq) THROW_ORB_EXCEPTIONS;
        
    virtual ::DDS::ReturnCode_t get_key_value(
					      Msg & key_holder,
					      ::DDS::InstanceHandle_t handle) THROW_ORB_EXCEPTIONS;
        
    virtual ::DDS::InstanceHandle_t lookup_instance(
						    const Msg & instance) THROW_ORB_EXCEPTIONS;
    
    MsgDataReader_impl ( 
			 gapi_dataReader handle
			 );
    
    virtual ~MsgDataReader_impl(void);
    
  private:
    MsgDataReader_impl(const MsgDataReader &);
    void operator= (const MsgDataReader &);
    
    static ::DDS::ReturnCode_t check_preconditions(
						   void * received_data,
						   ::DDS::SampleInfoSeq & info_seq,
						   CORBA::Long max_samples
						   );
  };

    
}


#endif 
