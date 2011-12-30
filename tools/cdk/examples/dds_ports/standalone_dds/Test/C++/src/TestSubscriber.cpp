
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

/************************************************************************
 * LOGICAL_NAME:    HelloWorldDataSubscriber.cpp
 * FUNCTION:        OpenSplice HelloWorld example code.
 * MODULE:          HelloWorld for the C++ programming language.
 * DATE             September 2010.
 ************************************************************************
 *
 * This file contains the implementation for the 'HelloWorldDataSubscriber' executable.
 *
 ***/
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
  //  mgr->createParticipant("OCPI");

  mgr->createParticipant("TEST");

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
  Msg1DataReader_var HelloWorldReader = Msg1DataReader::_narrow(dreader);
  checkHandle(HelloWorldReader, "MsgDataReader::_narrow");

  cout << "=== [Subscriber] Ready ..." << endl;

  bool closed = false;
  ReturnCode_t status =  - 1;
  int count = 0;
  int n;
  while (!closed && count < 1500) // We dont want the example to run undefinitely
  {
    status = HelloWorldReader->read(msgList, infoSeq, LENGTH_UNLIMITED,
				    DDS::NOT_READ_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);
    checkStatus(status, "msgDataReader::take");


    for (CORBA::ULong j = 0; j < msgList.length(); j++)
    {
      cout << "=== [Subscriber] message received :" << endl;




      cout << "    LONG  : " << msgList[j].userId << endl;
      cout << "    LONG  : " << msgList[j].u1 << endl;
      cout << "    v_longlong : " << msgList[j].v_longlong1 << endl;
      cout << "    v_short  : " << msgList[j].v_short << endl;

      cout << endl;
      for (n=0; n<3; n++ ) {
	cout << "Long ary = " << msgList[j].v_l_array[n] << endl;
      }

      cout << endl;
      for (n=0; n<3; n++ ) {
	for (int i=0; i<4; i++ ) {
	  cout << "Long ary 1 = " << msgList[j].v_l_array1[n][i] << endl;
	}
      }

      cout << "    v_bool  : " << msgList[j].v_bool << endl;
      cout << "    v_double: " << msgList[j].v_double << endl;
      cout << "    v_long2: " << msgList[j].v_long2 << endl;
      for ( n=0; n<5; n++ ) {
	cout << "v_oct_arry(" << n << ") = " << (int)msgList[j].v_oct_array[n] << endl;
      };

      cout << " v_embmsg value = " << msgList[j].v_embmsg.value << endl;
      cout << " v_embmsg seq count = " << msgList[j].v_embmsg.sdbl.length() << endl;
      for ( n=0; n<msgList[j].v_embmsg.sdbl.length(); n++ ) {
	cout << "Value " << n << " = " << msgList[j].v_embmsg.sdbl[n] << endl;
      }

      cout << "    Message : \"" << msgList[j].v_message << "\"" << endl;

      cout << " long seq count = " << msgList[j].long_seq.length() << endl;
      for ( n=0; n<msgList[j].long_seq.length(); n++ ) {
	cout << "Value " << n << " = " << msgList[j].long_seq[n] << endl;
      }

      cout << " double seq count = " << msgList[j].double_seq.length() << endl;
      for ( n=0; n<msgList[j].double_seq.length(); n++ ) {
	cout << "Value " << n << " = " << msgList[j].double_seq[n] << endl;
      }

      cout << " short seq count = " << msgList[j].short_seq.length() << endl;
      for ( n=0; n<msgList[j].short_seq.length(); n++ ) {
	cout << "Value " << n << " = " << (int)msgList[j].short_seq[n] << endl;
      }
      cout << " char seq count = " << msgList[j].char_seq.length() << endl;
      for ( n=0; n<msgList[j].char_seq.length(); n++ ) {
	cout << "Value " << n << " = " << (int)msgList[j].char_seq[n] << endl;
      }
      cout << " uchar seq count = " << msgList[j].uchar_seq.length() << endl;
      for ( n=0; n<msgList[j].uchar_seq.length(); n++ ) {
	cout << "Value " << n << " = " << (int)msgList[j].uchar_seq[n] << endl;
      }



      cout << " v_sseq seq count = " << msgList[j].v_sseq.length() << endl;
      for ( n=0; n<msgList[j].v_sseq.length(); n++ ) {
	cout << "Value " << n << " = " << msgList[j].v_sseq[n] << endl;
      }

      cout << endl;
      for (n=0; n<3; n++ ) {
	cout << "V = " << msgList[j].v_embmsgar[n].value << endl;
	for ( int y=0; y< msgList[j].v_embmsgar[n].sdbl.length(); y++ ) {
	  cout << "  Dbl = " << msgList[j].v_embmsgar[n].sdbl[y] << endl;
	};
      }

      cout << "Sequence of arrays" << endl;
      for (n=0; n< msgList[j].lseqar1.length(); n++ ) {
	cout << " s " << n << endl;
	for ( int y=0; y<4; y++ ) {
	  cout <<  "Value = " << msgList[j].lseqar1[n][y] << endl;
	}
      }

      // Array of sequences of EmbMsgs
      cout << "Array[2][3] of sequences of embedded messages" << endl;
      for ( n=0; n<2; n++ ) {
	for ( int nn=0; nn<3; nn++ ) {
	  cout << " Seq len = " << msgList[j].v_embmsgseqar[n][nn].length() << endl;
	  for ( int r=0; r<msgList[j].v_embmsgseqar[n][nn].length(); r++ ) {
	    cout << "  V = " <<  msgList[j].v_embmsgseqar[n][nn][r].value  << endl;
	    for ( int rr=0; rr<msgList[j].v_embmsgseqar[n][nn][r].sdbl.length(); rr++ ) {
	      cout << "   " << msgList[j].v_embmsgseqar[n][nn][r].sdbl[rr] << endl;
	    };
	  }
	}
      }

      cout << "v_enum = " << msgList[j].v_enum << endl;

      // Sequence of strings
      cout << "Seq string length = " << msgList[j].sseq2.length() << endl;
      for ( n=0; n<msgList[j].sseq2.length(); n++ ) {
	cout << "Seq string = " << msgList[j].sseq2[n] << endl;
      }






      /*

      cout << "Array of sequences" << endl;
      for (n=0; n<3; n++ ) {
	cout << "Values for s - " << n << endl;
	for ( int y=0; y<msgList[j].lseqar[n].length(); y++ ) {
	  cout << msgList[j].lseqar[n][y] << endl;
	}
      }




      cout << " v_sseq1 seq count = " << msgList[j].v_sseq1.length() << endl;
      for ( n=0; n<msgList[j].v_sseq1.length(); n++ ) {
	cout << "Value " << n << " = " << msgList[j].v_sseq1[n] << endl;
      }

      */

      /*
      cout << endl;
      for (n=0; n<2; n++ ) {
	for (int z=0; z<5; z++ ) {
	  cout << "v_mar1 ary = " << msgList[j].v_mar1[n][z] << endl;
	}
      }

      cout << endl;
      for (int y=0; y<4; y++ ) {  
	for (n=0; n<3; n++ ) {
	  cout << "v_embmsgar1 " << y << " " << n << " = " << msgList[j].v_embmsgar1[y][n].value << endl;
	  for ( int z=0; z<msgList[j].v_embmsgar1[y][n].sdbl.length(); z++ ) {
	    cout << "   dbl = " << msgList[j].v_embmsgar1[y][n].sdbl[z] << endl;
	  };
	}
      }
      */






     
      //      closed = true;
    }


    status = HelloWorldReader->return_loan(msgList, infoSeq);
    checkStatus(status, "MsgDataReader::return_loan");
    os_nanoSleep(delay_200ms);
    //++count;
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
