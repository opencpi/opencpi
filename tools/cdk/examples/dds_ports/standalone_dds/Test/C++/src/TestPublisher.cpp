
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
 * LOGICAL_NAME:    HelloWorldDataPublisher.cpp
 * FUNCTION:        OpenSplice Tutorial example code.
 * MODULE:          Tutorial for the C++ programming language.
 * DATE             September 2010.
 ************************************************************************
 *
 * This file contains the implementation for the 'HelloWorldDataPublisher' executable.
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
  Msg1DataWriter_var HelloWorldWriter = Msg1DataWriter::_narrow(dwriter);

  int n;
  int vi=0;
  Msg1 msgInstance; /* Example on Stack */



  msgInstance.userId = 2;
  msgInstance.u1 = 333;
  msgInstance.v_longlong1 = 97531246;
  msgInstance.v_short = 67;

  for (n=0; n<3; n++ ) {
    msgInstance.v_l_array[n] = n +11;
  }

  for (n=0; n<3; n++ ) {
    for (int i=0; i<4; i++ ) {
      msgInstance.v_l_array1[n][i] = 10 + n + i;
    }
  }
  msgInstance.v_bool = 0;
  msgInstance.v_double = 1234.9876;
  for ( n=0; n<5; n++ ) {
    msgInstance.v_oct_array[n] = 32+n;
  };
  msgInstance.v_long2 = 56;
  
  msgInstance.v_embmsg.value = 7;
  msgInstance.v_embmsg.sdbl.length(4);
  for ( n=0; n<msgInstance.v_embmsg.sdbl.length(); n++ ) {
      msgInstance.v_embmsg.sdbl[n] = n*10;  
  };

  msgInstance.v_message = CORBA::string_dup("Hello OCPI  World, from DDS   ");

  msgInstance.long_seq.length(5);
  for ( n=0; n<msgInstance.long_seq.length(); n++ ) {
    msgInstance.long_seq[n] = n*10;  
  };


  msgInstance.double_seq.length(12);
  for ( n=0; n<12; n++ ) {
    msgInstance.double_seq[n] = n*11;  
  };

  msgInstance.short_seq.length(4);
  for ( n=0; n<msgInstance.short_seq.length(); n++ ) {
    msgInstance.short_seq[n] = n*10;  
  };

  msgInstance.char_seq.length(8);
  for ( n=0; n<msgInstance.char_seq.length(); n++ ) {
    msgInstance.char_seq[n] = n*10;  
  };

  msgInstance.uchar_seq.length(12);
  for ( n=0; n<msgInstance.uchar_seq.length(); n++ ) {
    msgInstance.uchar_seq[n] = n*10;  
  };



  msgInstance.v_sseq.length(13);
  for ( n=0; n<msgInstance.v_sseq.length(); n++ ) {
    char buf[100];
    sprintf(buf,"DDS Message %d", n );
    msgInstance.v_sseq[n] = CORBA::string_dup(buf);
  };


  for (n=0; n<3; n++ ) {
    msgInstance.v_embmsgar[n].value = n;
    msgInstance.v_embmsgar[n].sdbl.length(n+2);
    for ( int y=0; y<msgInstance.v_embmsgar[n].sdbl.length(); y++ ) {
      msgInstance.v_embmsgar[n].sdbl[y] = y*10+n;  
    };
  }


  vi=25;
  msgInstance.lseqar1.length(4);
  for (n=0; n< msgInstance.lseqar1.length(); n++ ) {
    for ( int y=0; y<4; y++ ) {
      msgInstance.lseqar1[n][y] = vi++;
    }
  }



  // Array of sequences of EmbMsgs
  vi = 37;
  for ( n=0; n<2; n++ ) {
    for ( int nn=0; nn<3; nn++ ) {
      msgInstance.v_embmsgseqar[n][nn].length( 2+nn+n);
      for ( int r=0; r<msgInstance.v_embmsgseqar[n][nn].length(); r++ ) {
	msgInstance.v_embmsgseqar[n][nn][r].value = vi++;
	msgInstance.v_embmsgseqar[n][nn][r].sdbl.length( 4+nn );
	for ( int rr=0; rr<msgInstance.v_embmsgseqar[n][nn][r].sdbl.length(); rr++ ) {
	  msgInstance.v_embmsgseqar[n][nn][r].sdbl[rr] = vi+10;  
	};
      }
    }
  }

  msgInstance.v_enum = JTest::Msg1::b;


  // Sequence of strings
  msgInstance.sseq2.length(3);
  for ( n=0; n<msgInstance.sseq2.length(); n++ ) {
    char buf[1024];
    sprintf(buf,"Hello OCPI  World, from DDS %d",n);    
    msgInstance.sseq2[n] = CORBA::string_dup(buf);
  }


  ReturnCode_t status = HelloWorldWriter->write(msgInstance, NULL);
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
