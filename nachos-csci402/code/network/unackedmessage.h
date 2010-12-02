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
  
  bool equals(UnAckedMessage* msg)
  {
    return this->lastTimeSent.tv_sec == msg->lastTimeSent.tv_sec &&
            this->lastTimeSent.tv_usec == msg->lastTimeSent.tv_usec &&
            this->timeStamp.tv_sec == msg->timeStamp.tv_sec &&
            this->timeStamp.tv_usec == msg->timeStamp.tv_usec &&
            this->pktHdr.from == msg->pktHdr.from &&
            this->pktHdr.to == msg->pktHdr.to &&
            this->mailHdr.from == msg->mailHdr.from &&
            this->mailHdr.to == msg->mailHdr.to;
  }
  
  void print()
  {
    printf("UnAckedMessage: lastTimeSent: %d.%d, timeStamp: %d.%d,\n  from (%d, %d), to (%d, %d) msg: %s\n",
        (int)this->lastTimeSent.tv_sec, (int)this->lastTimeSent.tv_usec, (int)this->timeStamp.tv_sec, 
        (int)this->timeStamp.tv_usec, pktHdr.from, mailHdr.from, pktHdr.to, mailHdr.to, data);
  }
  
  PacketHeader pktHdr;
  MailHeader mailHdr;
  char* data;
  timeval lastTimeSent;
  timeval timeStamp;
};

#endif
