// post.cc 
// 	Routines to deliver incoming network messages to the correct
//	"address" -- a mailbox, or a holding area for incoming messages.
//	This module operates just like the US postal service (in other
//	words, it works, but it's slow, and you can't really be sure if
//	your mail really got through!).
//
//	Note that once we prepend the MailHdr to the outgoing message data,
//	the combination (MailHdr plus data) looks like "data" to the Network 
//	device.
//
// 	The implementation synchronizes incoming messages with threads
//	waiting for those messages.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "post.h"

#ifdef CHANGED
  #include <sys/time.h>
  #include "system.h"
  #include "netcall.h"
#endif

extern "C" {
	int bcopy(char *, char *, int);
};

//----------------------------------------------------------------------
// Mail::Mail
//      Initialize a single mail message, by concatenating the headers to
//	the data.
//
//	"pktH" -- source, destination machine ID's
//	"mailH" -- source, destination mailbox ID's
//	"data" -- payload data
//----------------------------------------------------------------------

Mail::Mail(PacketHeader pktH, MailHeader mailH, timeval timeS, char *msgData)
{
    ASSERT(mailH.length <= MaxMailSize);

    pktHdr = pktH;
    mailHdr = mailH;
    timeStamp = timeS;
    bcopy(msgData, data, mailHdr.length);
}

//----------------------------------------------------------------------
// MailBox::MailBox
//      Initialize a single mail box within the post office, so that it
//	can receive incoming messages.
//
//	Just initialize a list of messages, representing the mailbox.
//----------------------------------------------------------------------


MailBox::MailBox()
{ 
    messages = new SynchList(); 
}

//----------------------------------------------------------------------
// MailBox::~MailBox
//      De-allocate a single mail box within the post office.
//
//	Just delete the mailbox, and throw away all the queued messages 
//	in the mailbox.
//----------------------------------------------------------------------

MailBox::~MailBox()
{ 
    delete messages; 
}

//----------------------------------------------------------------------
// PrintHeader
// 	Print the message header -- the destination machine ID and mailbox
//	#, source machine ID and mailbox #, and message length.
//
//	"pktHdr" -- source, destination machine ID's
//	"mailHdr" -- source, destination mailbox ID's
//----------------------------------------------------------------------

static void 
PrintHeader(PacketHeader pktHdr, MailHeader mailHdr, timeval timeStamp)
{
    printf("From (%d, %d) to (%d, %d) bytes %d time %d.%d\n",
    	    pktHdr.from, mailHdr.from, pktHdr.to, mailHdr.to, mailHdr.length,
          (int)timeStamp.tv_sec, (int)timeStamp.tv_usec);
}

//----------------------------------------------------------------------
// MailBox::Put
// 	Add a message to the mailbox.  If anyone is waiting for message
//	arrival, wake them up!
//
//	We need to reconstruct the Mail message (by concatenating the headers
//	to the data), to simplify queueing the message on the SynchList.
//
//	"pktHdr" -- source, destination machine ID's
//	"mailHdr" -- source, destination mailbox ID's
//	"data" -- payload message data
//----------------------------------------------------------------------

void 
MailBox::Put(PacketHeader pktHdr, MailHeader mailHdr, timeval timeStamp, char *data)
{ 
    Mail *mail = new Mail(pktHdr, mailHdr, timeStamp, data); 

    messages->Append((void *)mail);	// put on the end of the list of 
					// arrived messages, and wake up 
					// any waiters
}

//----------------------------------------------------------------------
// MailBox::Get
// 	Get a message from a mailbox, parsing it into the packet header,
//	mailbox header, and data. 
//
//	The calling thread waits if there are no messages in the mailbox.
//
//	"pktHdr" -- address to put: source, destination machine ID's
//	"mailHdr" -- address to put: source, destination mailbox ID's
//	"data" -- address to put: payload message data
//----------------------------------------------------------------------

void 
MailBox::Get(PacketHeader *pktHdr, MailHeader *mailHdr, timeval *timeStamp, char *data) 
{ 
  DEBUG('n', "Waiting for mail in mailbox\n");
  Mail *mail = (Mail *) messages->Remove();	// remove message from list;
                                            // will wait if list is empty

  *pktHdr = mail->pktHdr;
  *mailHdr = mail->mailHdr;
  *timeStamp = mail->timeStamp;
  
  if (DebugIsEnabled('n'))
  {
    printf("Got mail from mailbox: ");
    PrintHeader(*pktHdr, *mailHdr, *timeStamp);
  }
  
  bcopy(mail->data, data, mail->mailHdr.length);
  // copy the message data into
  // the caller's buffer
  
  delete mail;			// we've copied out the stuff we
  // need, we can now discard the message
}

//----------------------------------------------------------------------
// PostalHelper, ReadAvail, WriteDone
// 	Dummy functions because C++ can't indirectly invoke member functions
//	The first is forked as part of the "postal worker thread; the
//	later two are called by the network interrupt handler.
//
//	"arg" -- pointer to the Post Office managing the Network
//----------------------------------------------------------------------

static void PostalHelper(int arg)
{ PostOffice* po = (PostOffice *) arg; po->PostalDelivery(); }
static void ReadAvail(int arg)
{ PostOffice* po = (PostOffice *) arg; po->IncomingPacket(); }
static void WriteDone(int arg)
{ PostOffice* po = (PostOffice *) arg; po->PacketSent(); }

//----------------------------------------------------------------------
// PostOffice::PostOffice
// 	Initialize a post office as a collection of mailboxes.
//	Also initialize the network device, to allow post offices
//	on different machines to deliver messages to one another.
//
//      We use a separate thread "the postal worker" to wait for messages 
//	to arrive, and deliver them to the correct mailbox.  Note that
//	delivering messages to the mailboxes can't be done directly
//	by the interrupt handlers, because it requires a Lock.
//
//	"addr" is this machine's network ID 
//	"reliability" is the probability that a network packet will
//	  be delivered (e.g., reliability = 1 means the network never
//	  drops any packets; reliability = 0 means the network never
//	  delivers any packets)
//	"nBoxes" is the number of mail boxes in this Post Office
//----------------------------------------------------------------------

PostOffice::PostOffice(NetworkAddress addr, double reliability, int nBoxes)
{
// First, initialize the synchronization with the interrupt handlers
    messageAvailable = new Semaphore("message available", 0);
    messageSent = new Semaphore("message sent", 0);
    sendLock = new Lock("message send lock");

// Second, initialize the mailboxes
    netAddr = addr; 
    numBoxes = nBoxes;
    boxes = new MailBox[nBoxes];

// Third, initialize the network; tell it which interrupt handlers to call
    network = new Network(addr, reliability, ReadAvail, WriteDone, (int) this);


// Finally, create a thread whose sole job is to wait for incoming messages,
//   and put them in the right mailbox. 
    Thread *t = new Thread("postal worker");

    t->Fork(PostalHelper, (int) this);
}

//----------------------------------------------------------------------
// PostOffice::~PostOffice
// 	De-allocate the post office data structures.
//----------------------------------------------------------------------

PostOffice::~PostOffice()
{
    delete network;
    delete [] boxes;
    delete messageAvailable;
    delete messageSent;
    delete sendLock;
}

//----------------------------------------------------------------------
// PostOffice::PostalDelivery
// 	Wait for incoming messages, and put them in the right mailbox.
//
//      Incoming messages have had the PacketHeader stripped off,
//	but the MailHeader is still tacked on the front of the data.
//----------------------------------------------------------------------

void
PostOffice::PostalDelivery()
{
  PacketHeader pktHdr;
  MailHeader mailHdr;
  timeval timeStamp;
  char *buffer = new char[MaxPacketSize];

  for (;;)
  {
    // first, wait for a message
    messageAvailable->P();	
    pktHdr = network->Receive(buffer);
    mailHdr = *(MailHeader *)buffer;
    timeStamp = *(timeval *)(buffer + sizeof(MailHeader));
    
    if (DebugIsEnabled('n'))
    {
      printf("Putting mail into mailbox: ");
      PrintHeader(pktHdr, mailHdr, timeStamp);
    }

    // check that arriving message is legal!
    ASSERT(0 <= mailHdr.to && mailHdr.to < numBoxes);
    ASSERT(mailHdr.length <= MaxMailSize);

    // put into mailbox
    boxes[mailHdr.to].Put(pktHdr, mailHdr, timeStamp, buffer + sizeof(MailHeader) + sizeof(timeval));
  }
}

//----------------------------------------------------------------------
// PostOffice::Send
// 	Concatenate the MailHeader to the front of the data, and pass 
//	the result to the Network for delivery to the destination machine.
//
//	Note that the MailHeader + data looks just like normal payload
//	data to the Network.
//
//	"pktHdr" -- source, destination machine ID's
//	"mailHdr" -- source, destination mailbox ID's
//	"data" -- payload message data
//----------------------------------------------------------------------

bool PostOffice::Send(PacketHeader pktHdr, MailHeader mailHdr, char* data, bool doAck)
{
  timeval timeStamp;
  gettimeofday(&timeStamp, NULL);
  return Send(pktHdr, mailHdr, timeStamp, data, doAck);
}

bool PostOffice::Send(PacketHeader pktHdr, MailHeader mailHdr, timeval timeStamp, char* data, bool doAck)
{
  char tmp[MaxMailSize];
  
  if (DebugIsEnabled('n'))
  {
    printf("Post send: ");
    PrintHeader(pktHdr, mailHdr, timeStamp);
  }

  #if 1 // For debugging
    printf("Sending from (%d, %d) to (%d, %d) bytes %d time %d.%d data %s\n",
          pktHdr.from, mailHdr.from, pktHdr.to, mailHdr.to, mailHdr.length,
          (int)timeStamp.tv_sec, (int)timeStamp.tv_usec, data);
  #endif
  
  // Make sure there's room after adding the timestamp (2 ints and 2 delimiting chars) if appropriate and header data.
  ASSERT(mailHdr.length <= MaxMailSize);
  ASSERT(0 <= mailHdr.to && mailHdr.to < numBoxes);

  // Fill in pktHdr, for the Network layer.
  pktHdr.from = netAddr;

  // Add sent message to the list of messages that have not been acked 
  //  so we can resend it later if necessary.
  unAckedMessagesLock->Acquire();
  
    // If we are resending the message, don't re-add it to unAckedMessages.
    bool foundInUnAckedMessages = false;
    for (int i = 0; i < (int)unAckedMessages.size(); ++i)
    {
      // If we are resending the message, update its lastTimeSent value and don't re-add it to unAckedMessages.
      if (unAckedMessages[i]->pktHdr.to == pktHdr.to &&
          unAckedMessages[i]->pktHdr.from == pktHdr.from &&
          unAckedMessages[i]->mailHdr.to == mailHdr.to &&
          unAckedMessages[i]->mailHdr.from == mailHdr.from &&
          unAckedMessages[i]->timeStamp.tv_sec == timeStamp.tv_sec &&
          unAckedMessages[i]->timeStamp.tv_usec == timeStamp.tv_usec)
      {
        foundInUnAckedMessages = true;
        timeval currentTime;
        gettimeofday(&currentTime, NULL);
        unAckedMessages[i]->lastTimeSent = currentTime;
        break;
      }
    }
    
    // If we are resending the message, we don't want to update the threadCounter.
    if (!foundInUnAckedMessages)
    {
      bool foundInSendCounters = false;
      int counter = 0;

      // If the message is an Ack, make counter a special char because an Ack can be lost and not resent.
      if (!doAck)
      {
        DEBUG_PACKET_LOSS( printf("Sending Ack\n"); )
        counter = -1;
      }
      else
      {      
        for (int i = 0; i < (int)currentThread->sendCounters.size(); ++i)
        {
          // Look and see if we have sent anything to this thread before.
          if (currentThread->sendCounters[i]->machineID == pktHdr.to &&
              currentThread->sendCounters[i]->mailID == mailHdr.to)
          {
            foundInSendCounters = true;
            counter = ++currentThread->sendCounters[i]->counter;          
            break;
          }
        }
        
        // If we didn't find a counter for this thread, add it for next time.
        if (!foundInSendCounters)
        {
          DEBUG_PACKET_LOSS( printf("Sending to (%d, %d) for the first time\n", pktHdr.to, mailHdr.to); )
          currentThread->sendCounters.push_back(new ThreadCounterEntry(pktHdr.to, mailHdr.to));
        }
      }

      // Insert the counter (which will be 0 if we just created it) in front of the data.
      sprintf(tmp, "%d_%s", counter, data);
      
      // Update the message length.
      mailHdr.length = strlen(tmp) + 1;
      ASSERT(mailHdr.length < MaxMailSize);
      
      // Only tell the system to resend if we want to require an Ack.
      // doAck will be false when the message we are sending is an ACK message.
      if (doAck)
        unAckedMessages.push_back(new UnAckedMessage(timeStamp, timeStamp, pktHdr, mailHdr, tmp));
    }
    else
    {
      // Do not insert the counter in front of the data in this case.
      sprintf(tmp, "%s", data);
      
      // Update the message length.
      mailHdr.length = strlen(tmp) + 1;
      ASSERT(mailHdr.length < MaxMailSize);
    }
    
  unAckedMessagesLock->Release();
  
  // Concatenate MailHeader and data.
  char buffer[MaxPacketSize];	    // space to hold concatenated mailHdr + timeStamp + data
  
  pktHdr.length = mailHdr.length + sizeof(MailHeader) + sizeof(timeval);
  bcopy((char *) &mailHdr, buffer, sizeof(MailHeader));
  bcopy((char *) &timeStamp, buffer + sizeof(MailHeader), sizeof(timeval));
  bcopy(tmp, buffer + sizeof(MailHeader) + sizeof(timeval), mailHdr.length);
  
  // Actually send the packet.
  sendLock->Acquire();  // Only one message can be sent to the network at any one time.
    bool success = network->Send(pktHdr, buffer);
    messageSent->P();	  // Wait for interrupt to tell us ok to send the next message.
  sendLock->Release();

  #if 0 // For debugging
    printf("Sending from (%d, %d) to (%d, %d) bytes %d time %d.%d data %s\n",
          pktHdr.from, mailHdr.from, pktHdr.to, mailHdr.to, mailHdr.length,
          (int)timeStamp.tv_sec, (int)timeStamp.tv_usec, data);
  #endif

  return success;
}

//----------------------------------------------------------------------
// PostOffice::Receive
// 	Retrieve a message from a specific box if one is available, 
//	otherwise wait for a message to arrive in the box.
//
//	Note that the MailHeader + data looks just like normal payload
//	data to the Network.
//
//  Assumes data is of size MaxMailSize.
//
//	"box" -- mailbox ID in which to look for message
//	"pktHdr" -- address to put: source, destination machine ID's
//	"mailHdr" -- address to put: source, destination mailbox ID's
//	"data" -- address to put: payload message data
//----------------------------------------------------------------------

void PostOffice::Receive(int box, PacketHeader *pktHdr, MailHeader *mailHdr, timeval *timeStamp, char* data)
{
  ASSERT((box >= 0) && (box < numBoxes));

  int success = false;
  while (!success)
  {
    char buffer[MaxMailSize];
    boxes[box].Get(pktHdr, mailHdr, timeStamp, buffer);
    
    int counter = -1;
    int index = parseValue(0, buffer, &counter);
    
    // Copy everything after the threadCounter to data.
    for (int j = 0; j < (int)MaxMailSize - index; ++j)
      data[j] = buffer[j + index];
    
    bool found = false;
    // We don't care about finding the receiveCounter if the message is an Ack (counter == -1).
    if (counter >= 0)
    {
      for (int i = 0; i < (int)currentThread->receiveCounters.size(); ++i)
      {
        // Look and see if we have received anything from this thread before.
        if (currentThread->receiveCounters[i]->machineID == pktHdr->from &&
            currentThread->receiveCounters[i]->mailID == mailHdr->from)
        {
          found = true;
          success = true;
          
          // If the counter value is valid (i.e. we are not processing messages our of order due to packet loss).
          // If the counter value is NOT valid, we ignore the message and wait for it to be resent (so we receive it in order).
          if (currentThread->receiveCounters[i]->counter + 1 == counter)
          {
            DEBUG_PACKET_LOSS( printf("Received valid receiveCounter from (%d, %d) expected %d actual %d\n", pktHdr->from, mailHdr->from, currentThread->receiveCounters[i]->counter + 1, counter); )
            currentThread->receiveCounters[i]->counter++;
          }
          // This would happen (most likely) when we sent an Ack that wasn't received, so now this message was resent to us.
          // In this case, we don't want to do anything here, but we want to return so the function calling receive can resend another Ack.
          else
          {
            DEBUG_PACKET_LOSS( printf("Invalid found receiveCounter from (%d, %d) expected %d actual %d\n", pktHdr->from, mailHdr->from, currentThread->receiveCounters[i]->counter + 1, counter); )
          }
          break;
        }
      }
    }

    // If this is the first message we have received from this thread.
    if (!found)
    {      
      // counter must be 0 or we have received this message out of order and we should discard it.
      if (counter == 0)
      {
        DEBUG_PACKET_LOSS( printf("Receiving from (%d, %d) for the first time\n", pktHdr->from, mailHdr->from); )
        success = true;
        
        // Create an entry for the thread.
        currentThread->receiveCounters.push_back(new ThreadCounterEntry(pktHdr->from, mailHdr->from));
      }
      else if (counter < 0) // If the message is an Ack.
      {
        DEBUG_PACKET_LOSS( printf("Receiving Ack\n"); )
        success = true;
      }
      else
      {
        DEBUG_PACKET_LOSS( printf("Invalid notfound receiveCounter from (%d, %d) expected 0 actual %d\n", pktHdr->from, mailHdr->from, counter); )
      }
    }
  }
  
  #if 0 // For debugging.
    printf("Receiving from (%d, %d) to (%d, %d) bytes %d time %d.%d data %s\n",
          pktHdr->from, mailHdr->from, pktHdr->to, mailHdr->to, mailHdr->length,
          (int)timeStamp->tv_sec, (int)timeStamp->tv_usec, data);
  #endif
  
  ASSERT(mailHdr->length <= MaxMailSize);
}

//----------------------------------------------------------------------
// PostOffice::IncomingPacket
// 	Interrupt handler, called when a packet arrives from the network.
//
//	Signal the PostalDelivery routine that it is time to get to work!
//----------------------------------------------------------------------

void
PostOffice::IncomingPacket()
{ 
    messageAvailable->V(); 
}

//----------------------------------------------------------------------
// PostOffice::PacketSent
// 	Interrupt handler, called when the next packet can be put onto the 
//	network.
//
//	The name of this routine is a misnomer; if "reliability < 1",
//	the packet could have been dropped by the network, so it won't get
//	through.
//----------------------------------------------------------------------

void 
PostOffice::PacketSent()
{ 
    messageSent->V();
}

#ifdef CHANGED
int PostOffice::GetID()
{ 
  #if 1 // This is the machine ID.
    return netAddr;
  #else
    return currentThread->mailID;
  #endif
}
#endif
