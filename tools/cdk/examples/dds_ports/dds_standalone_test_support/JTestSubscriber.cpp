
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
  os_time delay_200ms = { 0, 200000000 };
  Msg1Seq msgList;
  SampleInfoSeq infoSeq;

  DDSEntityManager *mgr = new DDSEntityManager();

  // create domain participant
  mgr->createParticipant("OCPI");

  //create type
  Msg1TypeSupport_var mt = new Msg1TypeSupport();
  mgr->registerType(mt.in());

 //create Topic
 char topic_name[] = "JTest_Msg1";
 mgr->createTopic(topic_name);

  //create Subscriber
  mgr->createSubscriber();

  // create DataReader
  mgr->createReader();

  DataReader_ptr dreader = mgr->getReader();
  Msg1DataReader_var JTestReader = Msg1DataReader::_narrow(dreader);
  checkHandle(JTestReader, "MsgDataReader::_narrow");

  cout << "=== [Subscriber] Ready ..." << endl;

  bool closed = false;
  ReturnCode_t status =  - 1;
  int count = 0;
  while (!closed && count < 1500) // We dont want the example to run undefinitely
  {
    status = JTestReader->read(msgList, infoSeq, LENGTH_UNLIMITED,
				    DDS::NOT_READ_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);
    checkStatus(status, "msgDataReader::take");
    for (CORBA::ULong j = 0; j < msgList.length(); j++)
    {
      cout << "=== [Subscriber] message received :" << endl;
      cout << "    LONG  : " << msgList[j].userId << endl;
      cout << "    LONG  : " << msgList[j].u1 << endl;
      cout << " long seq count = " << msgList[j].long_seq.length() << endl;
      for ( int n=0; n<msgList[j].long_seq.length(); n++ ) {
	cout << "Value " << n << " = " << msgList[j].long_seq[n] << endl;
      }
      cout << "    v_bool  : " << msgList[j].v_bool << endl;
      cout << "    Message : \"" << msgList[j].v_message << "\"" << endl;
      cout << "    v_short  : " << msgList[j].v_short << endl;
      cout << "    v_longlong : " << msgList[j].v_longlong << endl;
      cout << "    v_double: " << msgList[j].v_double << endl;
    }
    status = JTestReader->return_loan(msgList, infoSeq);
    checkStatus(status, "MsgDataReader::return_loan");
  }

  os_nanoSleep(delay_2ms);

  //cleanup
  mgr->deleteReader();
  mgr->deleteSubscriber();
  mgr->deleteTopic();
  mgr->deleteParticipant();

  delete mgr;
  return 0;
}
