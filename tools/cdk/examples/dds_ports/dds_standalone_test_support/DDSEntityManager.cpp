
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

#include "DDSEntityManager.h"
#include <iostream>

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

void DDSEntityManager::createTopic(char *topicName)
{
  status = participant->get_default_topic_qos(reliable_topic_qos);
  checkStatus(status, "DDS::DomainParticipant::get_default_topic_qos");
  reliable_topic_qos.reliability.kind = RELIABLE_RELIABILITY_QOS;
  reliable_topic_qos.durability.kind = TRANSIENT_DURABILITY_QOS;

  /* Make the tailored QoS the new default. */
  status = participant->set_default_topic_qos(reliable_topic_qos);
  checkStatus(status, "DDS::DomainParticipant::set_default_topic_qos");

  /* Use the changed policy when defining the HelloWorld topic */
  topic = participant->create_topic(topicName, typeName, reliable_topic_qos,
    NULL, STATUS_MASK_NONE);
  checkHandle(topic.in(), "DDS::DomainParticipant::create_topic ()");
}

void DDSEntityManager::deleteTopic()
{
  status = participant->delete_topic(topic);
  checkStatus(status, "DDS.DomainParticipant.delete_topic");
}

void DDSEntityManager::createPublisher()
{
  status = participant->get_default_publisher_qos(pub_qos);
  checkStatus(status, "DDS::DomainParticipant::get_default_publisher_qos");
  pub_qos.partition.name.length(1);
  pub_qos.partition.name[0] = partition;

  publisher = participant->create_publisher(pub_qos, NULL, STATUS_MASK_NONE);
  checkHandle(publisher.in(), "DDS::DomainParticipant::create_publisher");
}

void DDSEntityManager::deletePublisher()
{
	status = participant->delete_publisher(publisher.in());
    checkStatus(status, "DDS::DomainParticipant::delete_publisher ");
}


void DDSEntityManager::createWriter()
{
  writer = publisher->create_datawriter(topic.in(),
    DATAWRITER_QOS_USE_TOPIC_QOS, NULL, STATUS_MASK_NONE);
  checkHandle(writer, "DDS::Publisher::create_datawriter");
}

void DDSEntityManager::createWriter(bool autodispose_unregistered_instances)
{
  status = publisher->get_default_datawriter_qos(dw_qos);
  checkStatus(status, "DDS::DomainParticipant::get_default_publisher_qos");
  status = publisher->copy_from_topic_qos(dw_qos, reliable_topic_qos);
  checkStatus(status, "DDS::Publisher::copy_from_topic_qos");
  // Set autodispose to false so that you can start
  // the subscriber after the publisher
  dw_qos.writer_data_lifecycle.autodispose_unregistered_instances =
    autodispose_unregistered_instances;
  writer = publisher->create_datawriter(topic.in(), dw_qos, NULL,
    STATUS_MASK_NONE);
  checkHandle(writer, "DDS::Publisher::create_datawriter");
}

void DDSEntityManager::deleteWriter()
{
	status = publisher->delete_datawriter(writer);
	checkStatus(status, "DDS::Publisher::delete_datawriter ");
}

void DDSEntityManager::createSubscriber()
{
  int status = participant->get_default_subscriber_qos(sub_qos);
  checkStatus(status, "DDS::DomainParticipant::get_default_subscriber_qos");
  sub_qos.partition.name.length(1);
  sub_qos.partition.name[0] = partition;
  subscriber = participant->create_subscriber(sub_qos, NULL, STATUS_MASK_NONE);
  checkHandle(subscriber.in(), "DDS::DomainParticipant::create_subscriber");
}

void DDSEntityManager::deleteSubscriber()
{
  status = participant->delete_subscriber(subscriber);
  checkStatus(status, "DDS::DomainParticipant::delete_subscriber ");
}

void DDSEntityManager::createReader()
{
  reader = subscriber->create_datareader(topic.in(),
    DATAREADER_QOS_USE_TOPIC_QOS, NULL, STATUS_MASK_NONE);
  checkHandle(reader, "DDS::Subscriber::create_datareader ()");
}

void DDSEntityManager::deleteReader()
{
	status = subscriber->delete_datareader(reader);
	checkStatus(status, "DDS::Subscriber::delete_datareader ");
}

DataReader_ptr DDSEntityManager::getReader()
{
  return reader;
}

DataWriter_ptr DDSEntityManager::getWriter()
{
  return writer;
}

Publisher_ptr DDSEntityManager::getPublisher()
{
  return publisher._retn();
}

Subscriber_ptr DDSEntityManager::getSubscriber()
{
  return subscriber._retn();
}

Topic_ptr DDSEntityManager::getTopic()
{
  return topic._retn();
}

DomainParticipant_ptr DDSEntityManager::getParticipant()
{
  return participant._retn();
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
    cerr << "Error in " << info << ": " << getErrorName(status).c_str() << endl;
    exit(0);
  }
}

/**
 * Check whether a valid handle has been returned. If not, then terminate.
 **/
void checkHandle(void *handle, string info)
{

  if (!handle)
  {
    cerr << "Error in " << info.c_str() << ": Creation failed: invalid handle" << endl;
    exit(0);
  }
}
