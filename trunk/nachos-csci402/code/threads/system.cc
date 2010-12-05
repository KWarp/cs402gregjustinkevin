// system.cc 
//	Nachos initialization and cleanup routines.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"

#ifdef CHANGED
  #include <sys/time.h>
#endif

// This defines *all* of the global data structures used by Nachos.
// These are all initialized and de-allocated by this file.

Thread *currentThread;			  // the thread we are running now
Thread *threadToBeDestroyed;  // the thread that just finished
Scheduler *scheduler;			    // the ready list
Interrupt *interrupt;			    // interrupt status
Statistics *stats;			      // performance metrics
Timer *timer;				          // the hardware timer device, for invoking context switches
Thread *msgResendThread;

#ifdef FILESYS_NEEDED
  FileSystem  *fileSystem;
#endif

#ifdef FILESYS
  SynchDisk   *synchDisk;
#endif

#ifdef USER_PROGRAM	// requires either FILESYS or FILESYS_STUB
  Machine *machine;	// user program memory and registers
  
  #ifdef CHANGED
    BitMap* ppnInUseBitMap;
    Lock* ppnInUseLock;
    ProcessTable* processTable;
  #endif
#endif

#ifdef CHANGED
  #ifdef USE_TLB
    int currentTLBIndex;
    IPTEntry* ipt;
    SwapFile* swapFile;
    int useRandomPageEviction;
    SynchList* ppnQueue;
  #endif
#endif

#ifdef NETWORK
  PostOffice *postOffice;
	int serverCount = 1;
  
  #ifdef CHANGED
    #include "netcall.h"
    vector<UnAckedMessage*> unAckedMessages;
    Lock* unAckedMessagesLock;
    Timer* msgResendTimer;
    
    // Hard-coded network location of the registration server.
    static const int RegistrationServerMachineID = 0;
    static const int RegistrationServerMailID = 0;
    int mailIDCounter = 0;
    int totalNumNetworkThreads = -1;
    
    void PrintNetThreadHeader();
  #endif
#endif

// External definition, to allow us to take a pointer to this function
extern void Cleanup();

//----------------------------------------------------------------------
// TimerInterruptHandler
// 	Interrupt handler for the timer device.  The timer device is
//	set up to interrupt the CPU periodically (once every TimerTicks).
//	This routine is called each time there is a timer interrupt,
//	with interrupts disabled.
//
//	Note that instead of calling Yield() directly (which would
//	suspend the interrupt handler, not the interrupted thread
//	which is what we wanted to context switch), we set a flag
//	so that once the interrupt handler is done, it will appear as 
//	if the interrupted thread called Yield at the point it is 
//	was interrupted.
//
//	"dummy" is because every interrupt handler takes one argument,
//		whether it needs it or not.
//----------------------------------------------------------------------
static void TimerInterruptHandler(int dummy)
{
  if (interrupt->getStatus() != IdleMode)
    interrupt->YieldOnReturn();
}

#ifdef CHANGED
#ifdef NETWORK
// Gets forked as a kernel thread
static void MsgResendInterruptHandler(int dummy)
{
  static int ResendTimeout = 2;  // Num seconds without receiving an Ack until we resend a message.
  
  while(true)
  {

    unAckedMessagesLock->Acquire();
      // Resend any UnAckedMessages that have not been resent for too long.
      time_t currentTime = time(NULL);
      for (unsigned int i = 0; i < unAckedMessages.size(); ++i)
      {     
        if (currentTime - unAckedMessages[i]->lastTimeSent.tv_sec > ResendTimeout)
        {
          // Resend the message (addTimeStamp = false).
          postOffice->Send(unAckedMessages[i]->pktHdr, unAckedMessages[i]->mailHdr, 
                           unAckedMessages[i]->timeStamp, unAckedMessages[i]->data);
          
          printf("Resending: From (%d, %d) to (%d, %d) bytes %d time %d.%d data %s\n",
                unAckedMessages[i]->pktHdr.from, unAckedMessages[i]->mailHdr.from, 
                unAckedMessages[i]->pktHdr.to, unAckedMessages[i]->mailHdr.to, unAckedMessages[i]->mailHdr.length,
                (int)unAckedMessages[i]->timeStamp.tv_sec, (int)unAckedMessages[i]->timeStamp.tv_usec,
                unAckedMessages[i]->data);
        }
      }
    unAckedMessagesLock->Release();
    
    for (int i = 0; i < 5000; ++i)
    {
      currentThread->Yield();
    }
  
  }
}

// Returns true if the input message has already been received before.
bool messageIsRedundant(vector<UnAckedMessage*> *receivedMessages, PacketHeader inPktHdr, MailHeader inMailHdr, timeval timeStamp)
{
  // Check if any messages in receivedMessage matches the new message.
  for (int i = 0; i < (int)receivedMessages->size(); ++i)
  {
    if (receivedMessages->at(i)->pktHdr.from == inPktHdr.from &&
        receivedMessages->at(i)->pktHdr.to == inPktHdr.to &&
        receivedMessages->at(i)->mailHdr.from == inMailHdr.from &&
        receivedMessages->at(i)->mailHdr.to == inMailHdr.to &&
        receivedMessages->at(i)->timeStamp.tv_sec == timeStamp.tv_sec &&
        receivedMessages->at(i)->timeStamp.tv_usec == timeStamp.tv_usec)
    {
      return true;
    }
  }
  return false;
}

// Returns true if the thread has already registered with the server
bool threadHasRegistered(vector<NetThreadInfoEntry*> serverNetThreadInfo, int machineID, int mailID)
{
  for (int i = 0; i < (int)serverNetThreadInfo.size(); ++i)
  {
    if (serverNetThreadInfo[i]->machineID == machineID && 
        serverNetThreadInfo[i]->mailID == mailID)
    {
      return true;
    }
  }
  return false;
}

void RegServer()
{
  printf("===Starting Registration Server===\n");
  PacketHeader inPktHdr, outPktHdr;
  MailHeader inMailHdr, outMailHdr;
  timeval timeStamp;
  char buffer[MaxMailSize];
  RequestType requestType = INVALIDTYPE;
  int i = 0;
  int j = 0;
  int machineID = 0;
  int mailID = 0;
  char c = '?';
  char tmpStr[MaxMailSize];
  char request[MaxMailSize];
  static const int ThreadsPerMessage = 8; 
  vector<NetThreadInfoEntry*> serverNetThreadInfo;
  vector<UnAckedMessage*> *receivedMessages = new vector<UnAckedMessage*>();
  
  // verify that I'm the server
  ASSERT(postOffice->GetID() == RegistrationServerMachineID);
  ASSERT(currentThread->mailID == RegistrationServerMailID);
  
  // verify critical args
  ASSERT(totalNumNetworkThreads > 0 && totalNumNetworkThreads < 100);
  
  while ((int)serverNetThreadInfo.size() < totalNumNetworkThreads)
  {
    printf("SERVER: Wait for a REGNETTHREAD message\n");
    // Wait for a REGNETTHREAD message.
    requestType = INVALIDTYPE;
    while (requestType != REGNETTHREAD)
    {
      postOffice->Receive(currentThread->mailID, &inPktHdr, &inMailHdr, &timeStamp, buffer);
      i = parseValue(0, buffer, (int*)(&requestType));
      
      // Manually process any ACKs we receive.
      if (requestType == ACK)
        processAck(inPktHdr, inMailHdr, timeStamp);
      else
      {
        Ack(inPktHdr, inMailHdr, timeStamp, buffer);
        if (!messageIsRedundant(receivedMessages, inPktHdr, inMailHdr, timeStamp))
          receivedMessages->push_back(new UnAckedMessage(timeStamp, timeStamp, inPktHdr, inMailHdr, buffer));
      }
    }
    printf("SERVER: Received REGNETTHREAD message\n");

    // Parse machineID.
    i = parseValue(i, buffer, (int*)(&machineID));
    
    // Parse mailID.
    i = parseValue(i, buffer, (int*)(&mailID));
    
    printf("SERVER: buffer %s\n", buffer);
    printf("SERVER: Parsed machineID: %d, mailID: %d\n", machineID, mailID);
    
    // Clear the input buffer for next time.
    for(i = 0; i < (int)MaxMailSize; ++i)
      buffer[i] = '\0';
    
    // Add the network thread that just messaged us to serverNetThreadInfo.
    // threads can accidentally register twice when packet loss is enabled
    if( !threadHasRegistered(serverNetThreadInfo, machineID, mailID) )
    {
      NetThreadInfoEntry* entry = new NetThreadInfoEntry(machineID, mailID);
      serverNetThreadInfo.push_back(entry);
    }
    
    // Respond to the network thread with the number of messages it will receive containing serverNetThreadInfo.
    printf("SERVER: Tell network thread to expect %d messages\n", divRoundUp(totalNumNetworkThreads, ThreadsPerMessage));
 
    // Clear the output request.
    for(i = 0; i < (int)MaxMailSize; ++i)
      request[i] = '\0';
    
    sprintf(request, "%d_%d", REGNETTHREADRESPONSE, 
            divRoundUp(totalNumNetworkThreads, ThreadsPerMessage));
    
    outPktHdr.from = postOffice->GetID();
    outMailHdr.from = currentThread->mailID;
    outPktHdr.to = inPktHdr.from;
    outMailHdr.to = inMailHdr.from;
    outMailHdr.length = strlen(request) + 1;
    
    printf("SERVER: sending REGNETTHREADRESPONSE: From (%d, %d) to (%d, %d) bytes %d, time %d.%d\n",
         outPktHdr.from, outMailHdr.from, 
         outPktHdr.to, outMailHdr.to, outMailHdr.length,
         (int)timeStamp.tv_sec, (int)timeStamp.tv_usec);
    printf("SERVER: %s\n", request);
    
    if (!postOffice->Send(outPktHdr, outMailHdr, request))
      interrupt->Halt();
  }
  
  // Message all the threads with serverNetThreadInfo.
  printf("SERVER: Message all the threads with serverNetThreadInfo\n");
  for (i = 0; i < (int)serverNetThreadInfo.size(); )
  {
    // Clear the output request.
    for(j = 0; j < (int)MaxMailSize; ++j)
      request[j] = '\0';
  
    sprintf(request, "%d_", GROUPINFO);
    
    for (j = 0; j < ThreadsPerMessage && i < (int)serverNetThreadInfo.size(); ++j, ++i)
      sprintf(request + strlen(request), "%1d%02d", serverNetThreadInfo[i]->machineID, serverNetThreadInfo[i]->mailID);
    
    // Send the all the threads who are waiting.
    for (j = 0; j < (int)serverNetThreadInfo.size(); ++j)
    {
      outPktHdr.from = postOffice->GetID();
      outMailHdr.from = currentThread->mailID;
      outPktHdr.to = serverNetThreadInfo[j]->machineID;
      outMailHdr.to = serverNetThreadInfo[j]->mailID;
      outMailHdr.length = strlen(request) + 1;
      
      printf("SERVER: Sending to machineID: %d, mailID: %d\n", outPktHdr.to, outMailHdr.to);
      printf("SERVER: %s\n", request);
      
      if (!postOffice->Send(outPktHdr, outMailHdr, request))
        interrupt->Halt();
    }
  }
  
  // Wait in case any packets were dropped and need to be resent.
  printf("SERVER: Waiting for all GROUPINFO messages to be ACKed\n");
  while (unAckedMessages.size() > 0)
  {
    requestType = INVALIDTYPE;
    while (requestType != ACK)
    {
      postOffice->Receive(currentThread->mailID, &inPktHdr, &inMailHdr, &timeStamp, buffer);
      parseValue(0, buffer, (int*)(&requestType));
      
      // Manually process any ACKs we receive.
      if (requestType == ACK)
        processAck(inPktHdr, inMailHdr, timeStamp);
      else
      {
        Ack(inPktHdr, inMailHdr, timeStamp, buffer);
        if (!messageIsRedundant(receivedMessages, inPktHdr, inMailHdr, timeStamp))
          receivedMessages->push_back(new UnAckedMessage(timeStamp, timeStamp, inPktHdr, inMailHdr, buffer));
      }
    }
  }
  
  printf("=== RegServer() Complete ===\n");
  // interrupt->Halt(); // Don't do this if doing extra credit! We need to resend any acks if they were dropped.
  
  // Wait around in case any of our Acks were dropped.
  while (true)
  {
    postOffice->Receive(currentThread->mailID, &inPktHdr, &inMailHdr, &timeStamp, buffer);
    parseValue(0, buffer, (int*)(&requestType));
    
    // Manually process any ACKs we receive.
    if (requestType == ACK)
      processAck(inPktHdr, inMailHdr, timeStamp); // This shouldn't happen by the time we get here.
    else
    {
      Ack(inPktHdr, inMailHdr, timeStamp, buffer);
      if (!messageIsRedundant(receivedMessages, inPktHdr, inMailHdr, timeStamp))
        receivedMessages->push_back(new UnAckedMessage(timeStamp, timeStamp, inPktHdr, inMailHdr, buffer));
    }
  }
}

// Register with the Registration Server before we start processing messages.
void RegisterNetworkThread(vector<NetThreadInfoEntry*> *localNetThreadInfo, vector<UnAckedMessage*> *receivedMessages)
{
  PacketHeader inPktHdr, outPktHdr;
  MailHeader inMailHdr, outMailHdr;
  timeval timeStamp;
  char buffer[MaxMailSize];
  RequestType requestType = INVALIDTYPE;
  int i = 0;
  int j = 0;
  int numMessages = 0;
  char c = '?';
  char tmpStr[MaxMailSize];
  char request[MaxMailSize];
  
  PrintNetThreadHeader();
  printf("Wait for StartSimulation message from UserProgram\n");
  // Wait for StartSimulation message from UserProgram.
  while (requestType != STARTUSERPROGRAM)
  {
    postOffice->Receive(currentThread->mailID, &inPktHdr, &inMailHdr, &timeStamp, buffer);
    parseValue(0, buffer, (int*)(&requestType));
    
    // Manually process any ACKs we receive.
    if (requestType == ACK)
      processAck(inPktHdr, inMailHdr, timeStamp);
    else
    {
      Ack(inPktHdr, inMailHdr, timeStamp, buffer);
      if (!messageIsRedundant(receivedMessages, inPktHdr, inMailHdr, timeStamp))
        receivedMessages->push_back(new UnAckedMessage(timeStamp, timeStamp, inPktHdr, inMailHdr, buffer));
    }
  }

  PrintNetThreadHeader();
  printf("Message server with machineID and mailBoxID\n");
  
  // Message server with machineID and mailBoxID.
  sprintf(request, "%d_%d_%d", REGNETTHREAD, postOffice->GetID(), currentThread->mailID);

  // Clear the input buffer for next time.
	for(i = 0; i < (int)MaxMailSize; ++i)
		buffer[i] = '\0';
  
  outPktHdr.from = postOffice->GetID();
  outMailHdr.from = currentThread->mailID;
  outPktHdr.to = RegistrationServerMachineID;
  outMailHdr.to = RegistrationServerMailID;
  outMailHdr.length = strlen(request) + 1;
  
  if (!postOffice->Send(outPktHdr, outMailHdr, request))
    interrupt->Halt();
  
  PrintNetThreadHeader();
  printf("request: %s\n", request);
  PrintNetThreadHeader();
  printf("Wait for an Ack message from Registration Server\n");
  // Wait for an Ack message from Registration Server.
  // The Ack will contain numMessages.
  requestType = INVALIDTYPE;
  while (requestType != REGNETTHREADRESPONSE)
  {
    postOffice->Receive(currentThread->mailID, &inPktHdr, &inMailHdr, &timeStamp, buffer);
    i = parseValue(0, buffer, (int*)(&requestType));
    
    // Manually process any ACKs we receive.
    if (requestType == ACK)
      processAck(inPktHdr, inMailHdr, timeStamp);
    else
    {
      Ack(inPktHdr, inMailHdr, timeStamp, buffer);
      if (!messageIsRedundant(receivedMessages, inPktHdr, inMailHdr, timeStamp))
        receivedMessages->push_back(new UnAckedMessage(timeStamp, timeStamp, inPktHdr, inMailHdr, buffer));
    }
  }
  
  PrintNetThreadHeader();
  printf("Parse numMessages\n");
  PrintNetThreadHeader();
  printf("buffer %s\n", buffer);
  // Parse numMessages.
  c = '?';
  j = i;
  while (c != '\0')
  {
    c = buffer[i];
    if (c == '\0')
      break;
    tmpStr[i++] = c;
  }
  tmpStr[i++] = '\0';
  numMessages = atoi(tmpStr + j);
  j = 0;
  
  // Clear the input buffer for next time.
	for(i = 0; i < (int)MaxMailSize; ++i)
		buffer[i] = '\0';
  
  PrintNetThreadHeader();
  printf("Wait for %d messages from server\n", numMessages);
  // Wait for all of the messages.
  for (int msgNum = 0; msgNum < numMessages; ++msgNum)
  {
    i = 0;
    requestType = INVALIDTYPE;
    while (requestType != GROUPINFO)
    {
      postOffice->Receive(currentThread->mailID, &inPktHdr, &inMailHdr, &timeStamp, buffer);
      i = parseValue(0, buffer, (int*)(&requestType));
      
      // Manually process any ACKs we receive.
      if (requestType == ACK)
        processAck(inPktHdr, inMailHdr, timeStamp);
      else
      {
        Ack(inPktHdr, inMailHdr, timeStamp, buffer);
        if (!messageIsRedundant(receivedMessages, inPktHdr, inMailHdr, timeStamp))
          receivedMessages->push_back(new UnAckedMessage(timeStamp, timeStamp, inPktHdr, inMailHdr, buffer));
      }
    }
    
    PrintNetThreadHeader();
    printf("Received message %d of %d\n", msgNum+1, numMessages);
    PrintNetThreadHeader();
    printf("buffer %s\n", buffer);
    
    int machineID, mailID;
    
    // Parse machineID and mailID.
    c = '?';
    while (c != '\0')
    {
      c = buffer[i];
      if (c == '\0')
        break;
        
      tmpStr[0] = buffer[i];
      tmpStr[1] = '\0';
      tmpStr[2] = buffer[i + 1];
      tmpStr[3] = buffer[i + 2];
      tmpStr[4] = '\0';
      
      machineID = atoi(tmpStr);
      mailID = atoi(&tmpStr[2]);
      NetThreadInfoEntry* entry = new NetThreadInfoEntry(machineID, mailID);
      localNetThreadInfo->push_back(entry);
    
      i += 3;
      PrintNetThreadHeader();
      printf("parsed machineID: %d, mailID: %d\n", machineID, mailID); 
    }
  }
  
  // serverCount corresponds to the number of PostOffice instances we expect
  // alt: we might need to set this to localNetThreadInfo->size()
  serverCount = (int)localNetThreadInfo->size(); 
  
  PrintNetThreadHeader();
  printf("localNetThreadInfo->size(): %d\n", localNetThreadInfo->size());
  
  // Reply to my user thread stating that it can proceed
  PrintNetThreadHeader();
  printf("Reply to my user thread\n");
  sprintf(request, "%d_", STARTUSERPROGRAM);
  
  outPktHdr.from = postOffice->GetID();
  outMailHdr.from = currentThread->mailID;
  outPktHdr.to = postOffice->GetID();
  outMailHdr.to = currentThread->mailID + 1;
  outMailHdr.length = strlen(request) + 1;
  
  if (!postOffice->Send(outPktHdr, outMailHdr, request))
    interrupt->Halt();
  
}

void updateTimeStamp(char* buffer)
{
  char tmpStr[MaxMailSize];
  timeval timeStamp;
  
  // Get the current timeStamp.
  gettimeofday(&timeStamp, NULL);
  sprintf(tmpStr, "%d:%d!", (int)timeStamp.tv_sec, (int)timeStamp.tv_usec);
  
  // Copy the timeStamp to buffer.
  int i = 0;
  while (tmpStr[i] != '!')
  {
    buffer[i] = tmpStr[i];
    i++;
  }
}

void sendTimeStampMessage(vector<NetThreadInfoEntry*> *localNetThreadInfo)
{
  char buffer[MaxMailSize];
  sprintf(buffer, "%d_", TIMESTAMP);
  
  PacketHeader outPktHdr;
  MailHeader outMailHdr;
  
  // Forward message to all NetworkThreads (including self).
  for (int i = 0; i < (int)localNetThreadInfo->size(); ++i)
  {
    outPktHdr.from = postOffice->GetID();
    outMailHdr.from = currentThread->mailID;
    outPktHdr.to = localNetThreadInfo->at(i)->machineID;
    outMailHdr.to = localNetThreadInfo->at(i)->mailID;
    outMailHdr.length = strlen(buffer) + 1;
    
    if (!postOffice->Send(outPktHdr, outMailHdr, buffer))
      interrupt->Halt();
  }
}

void NetworkThread()
{
  printf("Starting NetworkThread\n");
  printf("machineID: %d, mailID %d\n", postOffice->GetID(), currentThread->mailID);
  PacketHeader inPktHdr, outPktHdr;
  MailHeader inMailHdr, outMailHdr;
  char buffer[MaxMailSize];
  RequestType requestType = INVALIDTYPE;
  timeval timeStamp;
  int i = 0;
  
  // must be local to each network thread
  vector<NetThreadInfoEntry*> *localNetThreadInfo = new vector<NetThreadInfoEntry*>();
  vector<UnAckedMessage*> *receivedMessages = new vector<UnAckedMessage*>();
  vector<UnAckedMessage*> msgQueue;
  
  RegisterNetworkThread(localNetThreadInfo, receivedMessages);
  
  PrintNetThreadHeader();
  printf("Entering while(true) loop\n");
  while (true)
  {
    // Clear the input buffer for next time.
    for(i = 0; i < (int)MaxMailSize; ++i)
      buffer[i] = '\0';
  
    // Wait for a message to be received.
    //PrintNetThreadHeader();
    //printf("Waiting for message to be received\n");
    
    requestType = INVALIDTYPE;
    postOffice->Receive(currentThread->mailID, &inPktHdr, &inMailHdr, &timeStamp, buffer);

    parseValue(0, buffer, (int*)(&requestType));
    //PrintNetThreadHeader();
    //printf("received message: %s\n", buffer);
    
    // If the packet is an Ack, process it.
    if (requestType == ACK)
    {
      // PrintNetThreadHeader();
      // printf("processAck from (%d, %d) \n", inPktHdr.from, inMailHdr.from);
      processAck(inPktHdr, inMailHdr, timeStamp);
      continue;
    }

    // Signal that we have received the message.
    Ack(inPktHdr, inMailHdr, timeStamp, buffer);
    
    // If we have already received and handled the message (the sender failed to receive our response Ack).
    if (messageIsRedundant(receivedMessages, inPktHdr, inMailHdr, timeStamp))
    {
      // Do not process the redundant message.
      PrintNetThreadHeader();
      printf("not processing redundant message: %s\n", buffer);
      continue;
    }
    
    if (requestType != TIMESTAMP)
    {
      // Update the other threads' timeStamp for this server.
      sendTimeStampMessage(localNetThreadInfo);
    }
	
    // Add message to receivedMessages.
    PrintNetThreadHeader();
    printf("Add message to receivedMessages: %s\n", buffer);
    receivedMessages->push_back(new UnAckedMessage(timeStamp, timeStamp, inPktHdr, inMailHdr, buffer));
    
    // If the message is from our UserProg thread.
    if (inPktHdr.from == postOffice->GetID() &&
        inMailHdr.from == currentThread->mailID + 1)
    {
      PrintNetThreadHeader();
      printf("=== Message from paired USER THREAD ===\n");
      
      PrintNetThreadHeader();
      printf("Process message: %s\n", buffer);
      processMessage(inPktHdr, inMailHdr, timeStamp, buffer, localNetThreadInfo);
    }
    else // if the message is from another thread (including self).
    {
      // Do Total Ordering.
      #if 0
        PrintNetThreadHeader();
        printf("=== Do Total Ordering ===\n");
      #endif
      // 1. Extract timestamp and member's ID.
      // This is already done above.
      
      // 2. Update last timestamp, in the table, for that member.
      for (i = 0; i < (int)localNetThreadInfo->size(); ++i)
      {
        if (inPktHdr.from  == localNetThreadInfo->at(i)->machineID &&
            inMailHdr.from == localNetThreadInfo->at(i)->mailID)
        {
          localNetThreadInfo->at(i)->timeStamp = timeStamp;
          break;
        }
      }
      
      // 3. Insert the message into msgQueue in timestamp order (if not a TIMESTAMP message).
      if (requestType != TIMESTAMP)
      {
        for (i = 0; i < (int)msgQueue.size(); ++i)
        {
          if (msgQueue[i]->timeStamp.tv_sec > timeStamp.tv_sec ||
              (msgQueue[i]->timeStamp.tv_sec == timeStamp.tv_sec &&
               msgQueue[i]->timeStamp.tv_usec > timeStamp.tv_usec))
          {
            break;
          }
        }
        
        PrintNetThreadHeader();
        printf("Adding to msgQueue[%d]: from (%d, %d) to (%d, %d) time %d.%d data %s\n", 
            i, inPktHdr.from, inMailHdr.from,
            inPktHdr.to, inMailHdr.to,
            (int)timeStamp.tv_sec, (int)timeStamp.tv_usec, buffer);
            
        UnAckedMessage* newMsg = new UnAckedMessage(timeStamp, timeStamp, inPktHdr, inMailHdr, buffer);
        msgQueue.insert(msgQueue.begin() + i, newMsg);
      }
      
      // 4. Extract the earliest timestamp value from the table.
      timeval earliestTimeStamp = localNetThreadInfo->at(0)->timeStamp;
      //PrintNetThreadHeader();
      //printf("Initial earliest TimeStamp:   %d.%d\n", 
      //    (int)earliestTimeStamp.tv_sec, (int)earliestTimeStamp.tv_usec);
      for (i = 1; i < (int)localNetThreadInfo->size(); ++i)
      {
        if (localNetThreadInfo->at(i)->timeStamp.tv_sec > 0 &&  // If the timeStamp is 0, it is still invalid, so ignore it.
            (localNetThreadInfo->at(i)->timeStamp.tv_sec < earliestTimeStamp.tv_sec ||
            (localNetThreadInfo->at(i)->timeStamp.tv_sec == earliestTimeStamp.tv_sec &&
             localNetThreadInfo->at(i)->timeStamp.tv_usec < earliestTimeStamp.tv_usec)))
        {
          earliestTimeStamp = localNetThreadInfo->at(i)->timeStamp;
        }
      }
      
      // 5. Process any message, in timestamp order, with a timestamp <=
      //    value from step 4.   
      for (i = 0; i < (int)msgQueue.size(); ++i)
      {
        if (localNetThreadInfo->at(i)->timeStamp.tv_sec > 0 &&  // If the timeStamp is 0, it is still invalid, so ignore it.
            (msgQueue[i]->timeStamp.tv_sec < earliestTimeStamp.tv_sec ||
            (msgQueue[i]->timeStamp.tv_sec == earliestTimeStamp.tv_sec &&
             msgQueue[i]->timeStamp.tv_usec <= earliestTimeStamp.tv_usec)))
        {
          // Process message.
          PrintNetThreadHeader();
          printf("Processing message: from (%d, %d) to (%d, %d) time %d.%d data %s\n", 
              msgQueue[i]->pktHdr.from, msgQueue[i]->mailHdr.from,
              msgQueue[i]->pktHdr.to, msgQueue[i]->mailHdr.to,
              (int)timeStamp.tv_sec, (int)timeStamp.tv_usec, buffer);
              
          processMessage(msgQueue[i]->pktHdr, msgQueue[i]->mailHdr, msgQueue[i]->timeStamp, msgQueue[i]->data, localNetThreadInfo);
          
          // Remove the message from the queue.
          msgQueue.erase(msgQueue.begin() + i);
          i--;
        }
      }
      #if 0
        PrintNetThreadHeader();
        printf("=== Completed Total Ordering ===\n");
      #endif
    }
  }
}

void PrintNetThreadHeader()
{
  printf("NET THREAD[%d,%d]: ", postOffice->GetID(), currentThread->mailID);
}

#endif  // NETWORK
#endif  // CHANGED

//----------------------------------------------------------------------
// Initialize
// 	Initialize Nachos global data structures.  Interpret command
//	line arguments in order to determine flags for the initialization.  
// 
//	"argc" is the number of command line arguments (including the name
//		of the command) -- ex: "nachos -d +" -> argc = 3 
//	"argv" is an array of strings, one for each command line argument
//		ex: "nachos -d +" -> argv = {"nachos", "-d", "+"}
//----------------------------------------------------------------------
void Initialize(int argc, char **argv)
{
  int argCount;
  char* debugArgs = "";
  bool randomYield = FALSE;

  #ifdef USER_PROGRAM
    bool debugUserProg = FALSE;	// single step user program
  #endif
  #ifdef FILESYS_NEEDED
    bool format = FALSE;	// format disk
  #endif
  #ifdef CHANGED
    #ifdef USE_TLB
      useRandomPageEviction = FALSE; // default is FIFO
    #endif
  #endif
  #ifdef NETWORK
    double rely = 1;		// network reliability
    int netname = 0;		// UNIX socket name
    
    #ifdef CHANGED
      unAckedMessagesLock = new Lock("UnAckedMessages Lock");
    #endif
  #endif
    
  for (argc--, argv++; argc > 0; argc -= argCount, argv += argCount)
  {
    argCount = 1;
    if (!strcmp(*argv, "-d"))
    {
        if (argc == 1)
          debugArgs = "+";	// turn on all debug flags
        else
        {
          debugArgs = *(argv + 1);
          argCount = 2;
        }
    }
    else if (!strcmp(*argv, "-rs"))
    {
      ASSERT(argc > 1);
      RandomInit(atoi(*(argv + 1)));	// initialize pseudo-random number generator
      randomYield = TRUE;
      argCount = 2;
    }
    #ifdef USER_PROGRAM
      if (!strcmp(*argv, "-s"))
        debugUserProg = TRUE;
    #endif
    #ifdef FILESYS_NEEDED
      if (!strcmp(*argv, "-f"))
        format = TRUE;
    #endif
    #ifdef CHANGED
      #ifdef USE_TLB
        if (!strcmp(*argv, "-P"))
        {
          //printf("Here is the argv: %s \n", *(argv+1));
          if(!strcmp(*(argv + 1), "RAND") || !strcmp(*(argv + 1), "rand"))
          {
            //printf("USING RANDOM PAGE EVICTION\n");
            useRandomPageEviction = TRUE;
          }
        }
      #endif
    #endif
    #ifdef NETWORK
      if (!strcmp(*argv, "-l"))
      {
        ASSERT(argc > 1);
        rely = atof(*(argv + 1));
        argCount = 2;
      } 
      else if (!strcmp(*argv, "-m"))
      {
        ASSERT(argc > 1);
        netname = atoi(*(argv + 1));
        argCount = 2;
      }
	  else if (!strcmp(*argv, "-sc")) 
	  {
		serverCount=atoi(*(argv + 1));
		if(serverCount<0 || serverCount > 5)
		{
				printf("Invalid number of servers");
				interrupt->Halt();
		}
		argCount=2;
      }
    #endif
  }

  DebugInit(debugArgs);			// initialize DEBUG messages
  stats = new Statistics();			// collect statistics
  interrupt = new Interrupt;			// start up interrupt handling
  scheduler = new Scheduler();		// initialize the ready queue
  if (randomYield)				// start the timer (if needed)
    timer = new Timer(TimerInterruptHandler, 0, randomYield);
    
  #ifdef CHANGED
    #ifdef NETWORK
      // printf("Commented out msgResendTimer\n");
      // msgResendTimer = new Timer(MsgResendInterruptHandler, 0, false);
      msgResendThread = new Thread("MsgResendThread");
      msgResendThread->Fork((VoidFunctionPtr)MsgResendInterruptHandler, 0);
    #endif
  #endif

  threadToBeDestroyed = NULL;

  // We didn't explicitly allocate the current thread we are running in.
  // But if it ever tries to give up the CPU, we better have a Thread
  // object to save its state. 
  currentThread = new Thread("main");	
  currentThread->setStatus(RUNNING);

  interrupt->Enable();
  CallOnUserAbort(Cleanup);			// if user hits ctl-C
    
  #ifdef USER_PROGRAM
    machine = new Machine(debugUserProg);	// this must come first
    
    #ifdef CHANGED
      ppnInUseBitMap = new BitMap(NumPhysPages);
      ppnInUseLock = new Lock("ppnInUseLock");
      // processTable = new ProcessTable();
    #endif
  #endif

  #ifdef CHANGED

    #ifdef USE_TLB
      currentTLBIndex = 0;
      ipt = new IPTEntry[NumPhysPages];
      swapFile = new SwapFile();
      ppnQueue = new SynchList();
      
      for (int i = 0; i < NumPhysPages; ++i)
      {
        // Initialize the ipt so every entry is invalid.
        ipt[i].virtualPage  = -1;
        ipt[i].physicalPage = i;
        ipt[i].valid        = FALSE;
        ipt[i].use          = FALSE;
        ipt[i].dirty        = FALSE;
        ipt[i].readOnly     = FALSE;  // If the code segment was entirely on a separate page, we could set its pages to be read-only.
        ipt[i].processID    = NULL;
      }
    #endif
  #endif
  
  #ifdef FILESYS
    synchDisk = new SynchDisk("DISK");
  #endif

  #ifdef FILESYS_NEEDED
    fileSystem = new FileSystem(format);
  #endif

  #ifdef NETWORK
    postOffice = new PostOffice(netname, rely, 100);
  #endif
}

//----------------------------------------------------------------------
// Cleanup
// 	Nachos is halting.  De-allocate global data structures.
//----------------------------------------------------------------------
void Cleanup()
{
  printf("\nCleaning up...\n");
  
  #ifdef NETWORK
    delete postOffice;
  #endif
    
  #ifdef USER_PROGRAM
    delete machine;
    
    #ifdef CHANGED
      delete ppnInUseBitMap;
      delete ppnInUseLock;
      // delete processTable;
    #endif
  #endif

  #ifdef FILESYS_NEEDED
    delete fileSystem;
  #endif

  #ifdef FILESYS
    delete synchDisk;
  #endif
    
  delete timer;
  delete scheduler;
  delete interrupt;
    
  Exit(0);
}

