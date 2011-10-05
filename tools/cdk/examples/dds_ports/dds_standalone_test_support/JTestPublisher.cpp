
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


#include <string>
#include <sstream>
#include <iostream>
#include "DDSEntityManager.h"
#include "ccpp_JTest.h"
#include "os.h"

using namespace DDS;
using namespace JTest;

int main(int argc, char *argv[])
{
  os_time delay_2ms = { 0, 2000000 };
  DDSEntityManager *mgr = new DDSEntityManager();


  // create domain participant
  mgr->createParticipant("OCPI");

  //create type
  Msg1TypeSupport_var mt = new Msg1TypeSupport();
  mgr->registerType(mt.in());

  //create Topic
 char topic_name[] = "JTest_Msg1";
 mgr->createTopic(topic_name);

  //create Publisher
  mgr->createPublisher();

  // create DataWriter :
  // If autodispose_unregistered_instances is set to true (default value),
  // you will have to start the subscriber before the publisher
  bool autodispose_unregistered_instances = true;
  mgr->createWriter(autodispose_unregistered_instances);

  // Publish Events
  DataWriter_ptr dwriter = mgr->getWriter();
  Msg1DataWriter_var JTestWriter = Msg1DataWriter::_narrow(dwriter);

  Msg1 msgInstance; /* Example on Stack */
  msgInstance.userId = 1;
  msgInstance.u1 = 2;
  msgInstance.long_seq.length(4);
  msgInstance.long_seq[0] = 0;  
  msgInstance.long_seq[1] = 1;  
  msgInstance.long_seq[2] = 2;  
  msgInstance.long_seq[3] = 3;  
  msgInstance.v_bool = 0;
  msgInstance.v_message = CORBA::string_dup("Hello OCPI  World");
  msgInstance.v_short = 67;
  msgInstance.v_longlong = 999988881;
  msgInstance.v_double = 456.123;

  ReturnCode_t status = JTestWriter->write(msgInstance, NULL);
  checkStatus(status, "MsgDataWriter::write");
  os_nanoSleep(delay_2ms);

  /* Remove the DataWriters */
  mgr->deleteWriter();

  /* Remove the Publisher. */
  mgr->deletePublisher();

  /* Remove the Topics. */
  mgr->deleteTopic();

  /* Remove Participant. */
  mgr->deleteParticipant();

  delete mgr;
  return 0;
}
