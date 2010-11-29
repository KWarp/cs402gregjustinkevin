#ifndef UNACKEDMESSAGE_H
#define UNACKEDMESSAGE_H

#include <time.h>
#include "post.h"

class UnAckedMessage
{
 public:
  UnAckedMessage(timeval initLastTimeSent, timeval initTimeStamp, PacketHeader initPktHdr, MailHeader initMailHdr, char* initData)
  {
    this->lastTimeSent = initLastTimeSent;
    this->timeStamp = initTimeStamp;
    this->pktHdr = initPktHdr;
    this->mailHdr = initMailHdr;
    
    // Deep copy data so it doesn't get deleted in the function that creates this.
    this->data = new char[mailHdr.length];
    for (unsigned int i = 0; i < mailHdr.length; ++i)
      this->data[i] = initData[i];
  }
  
  ~UnAckedMessage()
  {
    delete data;
  }
  
  PacketHeader pktHdr;
  MailHeader mailHdr;
  char* data;
  timeval lastTimeSent;
  timeval timeStamp;
};

#endif
