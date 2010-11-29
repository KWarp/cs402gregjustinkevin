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
    vector<UnAckedMessage*> receivedMessages;
    Lock* receivedMessagesLock;
    vector<NetThreadInfoEntry*> globalNetThreadInfo;
    
    // Hard-coded network location of the registration server.
    static const int RegistrationServerMachineID = 0;
    static const int RegistrationServerMailID = 0;
    int mailIDCounter = 0;
    int totalNumNetworkThreads = -1;
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
// Gets called on a timer.
static void MsgResendInterruptHandler(int dummy)
{
  static int ResendTimeout = 5;  // Num seconds without receiving an Ack until we resend a message.

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
      }
    }
  unAckedMessagesLock->Release();
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
  static const int ThreadsPerMessage = 5; 
  
  // verify that I'm the server
  ASSERT(postOffice->GetID() == RegistrationServerMachineID);
  ASSERT(currentThread->mailID == RegistrationServerMailID);
  
  // verify critical args
  ASSERT(totalNumNetworkThreads > 0 && totalNumNetworkThreads < 100);
  
  while ((int)globalNetThreadInfo.size() < totalNumNetworkThreads)
  {
    printf("SERVER: Wait for a REGNETTHREAD message\n");
    // Wait for a REGNETTHREAD message.
    requestType = INVALIDTYPE;
    while (requestType != REGNETTHREAD)
    {
      postOffice->Receive(currentThread->mailID, &inPktHdr, &inMailHdr, &timeStamp, buffer);
      i = parseValue(0, buffer, (int*)(&requestType));
    }
    printf("SERVER: Received REGNETTHREAD message, send ACK\n");
    Ack(inPktHdr, inMailHdr, timeStamp, buffer);

    // Parse machineID.
    i = parseValue(i, buffer, (int*)(&machineID));
    
    // Parse mailID.
    i = parseValue(i, buffer, (int*)(&mailID));
    
    printf("SERVER: buffer %s\n", buffer);
    printf("SERVER: Parsed machineID: %d, mailID: %d\n", machineID, mailID);
    
    // Clear the input buffer for next time.
    for(i = 0; i < (int)MaxMailSize; ++i)
      buffer[i] = '\0';
    
    // Add the network thread that just messaged us to globalNetThreadInfo.
    NetThreadInfoEntry* entry = new NetThreadInfoEntry(machineID, mailID);
    globalNetThreadInfo.push_back(entry);
    
    // Respond to the network thread with the number of messages it will receive containing globalNetThreadInfo.
    printf("SERVER: Tell network thread to expect %d messages\n", divRoundUp(totalNumNetworkThreads, ThreadsPerMessage));
 
    // Clear the output buffer.
    for(i = 0; i < (int)MaxMailSize; ++i)
      request[i] = '\0';
    
    sprintf(request, "%d_%d", REGNETTHREADRESPONSE, 
            divRoundUp(totalNumNetworkThreads, ThreadsPerMessage));
    
    outPktHdr.from = postOffice->GetID();
    outMailHdr.from = currentThread->mailID;
    outPktHdr.to = inPktHdr.from;
    outMailHdr.to = inMailHdr.from;
    outMailHdr.length = strlen(request) + 1;
    
    printf("SERVER: %s\n", request);
    if (!postOffice->Send(outPktHdr, outMailHdr, request))
      interrupt->Halt();
  }
  
  // Message all the threads with globalNetThreadInfo.
  printf("SERVER: Message all the threads with globalNetThreadInfo\n");
  for (i = 0; i < (int)globalNetThreadInfo.size(); )
  {
    // Clear the output buffer.
    for(j = 0; j < (int)MaxMailSize; ++j)
      request[j] = '\0';
  
    sprintf(request, "%d_", GROUPINFO);

    for (j = 0; j < ThreadsPerMessage && i < (int)globalNetThreadInfo.size(); ++j, ++i)
      sprintf(request + strlen(request), "%1d%02d", globalNetThreadInfo[i]->machineID, globalNetThreadInfo[i]->mailID);
    
    // Send the all the threads who are waiting.
    for (j = 0; j < (int)globalNetThreadInfo.size(); ++j)
    {
      outPktHdr.from = postOffice->GetID();
      outMailHdr.from = currentThread->mailID;
      outPktHdr.to = globalNetThreadInfo[j]->machineID;
      outMailHdr.to = globalNetThreadInfo[j]->mailID;
      outMailHdr.length = strlen(request) + 1;
      
      printf("SERVER: Sending to machineID: %d, mailID: %d\n", outPktHdr.to, outMailHdr.to);
      printf("SERVER: %s\n", request);
      
      if (!postOffice->Send(outPktHdr, outMailHdr, request))
        interrupt->Halt();
    }
  }
  printf("=== RegServer() Complete ===\n");
  interrupt->Halt();
}

// Register with the Registration Server before we start processing messages.
void RegisterNetworkThread()
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
  
  printf("NET THREAD: Wait for StartSimulation message from UserProgram\n");
  // Wait for StartSimulation message from UserProgram.
  while (requestType != STARTUSERPROGRAM)
  {
    postOffice->Receive(currentThread->mailID, &inPktHdr, &inMailHdr, &timeStamp, buffer);
    parseValue(0, buffer, (int*)(&requestType));
  }
  printf("NET THREAD: Recieved STARTUSERPROGRAM\n");
  
  // Ack that we got the message by removing it from unAckedMessages.
  Ack(inPktHdr, inMailHdr, timeStamp, buffer);

  printf("NET THREAD: Message server with machineID and mailBoxID\n");
  
  // Message server with machineID and mailBoxID.
  sprintf(request, "%d_%d_%d", REGNETTHREAD, postOffice->GetID(), currentThread->mailID);
	
  // Do error checking for length of request here?
  
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
  
  printf("NET THREAD request: %s\n", request);
  printf("NET THREAD: Wait for an Ack message from Registration Server\n");
  // Wait for an Ack message from Registration Server.
  // The Ack will contain numMessages.
  requestType = INVALIDTYPE;
  while (requestType != REGNETTHREADRESPONSE)
  {
    postOffice->Receive(currentThread->mailID, &inPktHdr, &inMailHdr, &timeStamp, buffer);
    i = parseValue(0, buffer, (int*)(&requestType));
  }
  
  Ack(inPktHdr, inMailHdr, timeStamp, buffer);
  
  printf("NET THREAD: Parse numMessages\n");
  printf("NET THREAD: buffer %s\n", buffer);
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
  
  printf("NET THREAD: Wait for %d messages from server\n", numMessages);
  // Wait for all of the messages.
  for (int msgNum = 0; msgNum < numMessages; ++msgNum)
  {
    i = 0;
    requestType = INVALIDTYPE;
    while (requestType != GROUPINFO)
    {
      postOffice->Receive(currentThread->mailID, &inPktHdr, &inMailHdr, &timeStamp, buffer);
      i = parseValue(0, buffer, (int*)(&requestType));
    }
    printf("NET THREAD: Received message %d of %d\n", msgNum+1, numMessages);
    printf("NET THREAD: buffer %s\n", buffer);
    Ack(inPktHdr, inMailHdr, timeStamp, buffer);

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
      globalNetThreadInfo.push_back(entry);
    
      i += 3;
      printf("NET THREAD: parsed machineID: %d, mailID: %d\n", machineID, mailID); 
    }
   
  }
  
  // Reply to my user thread stating that it can proceed
  printf("NET THREAD: Reply to my user thread\n");
  sprintf(request, "%d_", STARTUSERPROGRAM);
  
  outPktHdr.from = postOffice->GetID();
  outMailHdr.from = currentThread->mailID;
  outPktHdr.to = postOffice->GetID();
  outMailHdr.to = currentThread->mailID + 1;
  outMailHdr.length = strlen(request) + 1;
  
  if (!postOffice->Send(outPktHdr, outMailHdr, request))
    interrupt->Halt();
  
}

// Returns true if the input message has already been received before.
bool messageIsRedundant(PacketHeader inPktHdr, MailHeader inMailHdr, char* inData)
{
  // Check if any messages in receivedMessage matches the new message.
  bool found = false;
  for (int i = 0; i < (int)receivedMessages.size(); ++i)
  {
    if (receivedMessages[i]->pktHdr.from == inPktHdr.from &&
        receivedMessages[i]->pktHdr.to == inPktHdr.to &&
        receivedMessages[i]->mailHdr.from == inMailHdr.from &&
        receivedMessages[i]->mailHdr.to == inMailHdr.to &&
        receivedMessages[i]->mailHdr.length == inMailHdr.length)
    {
      found = true;
      for (int j = 0; j < (int)inMailHdr.length; ++j)
      {
        if (receivedMessages[i]->data[i] != inData[j])
          found = false;
      }
    }
    if (found)
      break;
  }
  return found;
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

bool processAck(PacketHeader inPktHdr, MailHeader inMailHdr, timeval timeStamp)
{
  bool found = false;
  for (int i = 0; i < (int)unAckedMessages.size(); ++i)
  {    
    if (unAckedMessages[i]->pktHdr.from == inPktHdr.from &&
        unAckedMessages[i]->pktHdr.to == inPktHdr.to &&
        unAckedMessages[i]->mailHdr.from == inMailHdr.from &&
        unAckedMessages[i]->mailHdr.to == inMailHdr.to &&
        unAckedMessages[i]->timeStamp.tv_sec == timeStamp.tv_sec &&
        unAckedMessages[i]->timeStamp.tv_usec == timeStamp.tv_usec)
    {
      // Remove the message from unAckedMessages, since it has now been Acked.
      unAckedMessages.erase(unAckedMessages.begin() + i);
      found = true;
      break;
    }
  }
  if (!found) // Error!!! This should never happen.
    printf("Ack message not found in unAckedMessages\n");
    
  return found;
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
  int numMessages = 0;
  char c = '?';
  char tmpStr[MaxMailSize];
  vector<UnAckedMessage*> msgQueue;
  
  printf("Before RegisterNetworkThread\n");
  RegisterNetworkThread();
  printf("After RegisterNetworkThread\n");
  
  while (true)
  {
    // Wait for a message to be received.
    postOffice->Receive(currentThread->mailID, &inPktHdr, &inMailHdr, &timeStamp, buffer);
    parseValue(0, buffer, (int*)(&requestType));
    printf("NET THREAD: recieved message: %s\n", buffer);
    // If the packet is an Ack, process it.
    if (requestType == ACK)
    {
      processAck(inPktHdr, inMailHdr, timeStamp);
      continue;
    }
    
    // Signal that we have received the message.
    Ack(inPktHdr, inMailHdr, timeStamp, buffer);
    
    // If we have already received and handled the message (the sender failed to receive our response Ack).
    if (messageIsRedundant(inPktHdr, inMailHdr, buffer))
    {
      // Do not process the redundant message.
      continue;
    }
    
    // Add message to receivedMessages.
    timeval tmpTimeStamp;
    gettimeofday(&tmpTimeStamp, NULL);
    receivedMessages.push_back(new UnAckedMessage(tmpTimeStamp, tmpTimeStamp, inPktHdr, inMailHdr, buffer));
    
    // If the message is from our UserProg thread.
    if (inPktHdr.from == postOffice->GetID() &&
        inMailHdr.from == currentThread->mailID + 1)
    {
      printf("NET THREAD: msg from paired User Thread\n");

      // Update the timeStamp since this is a new message.
      gettimeofday(&timeStamp, NULL);
      
      // Forward message to all NetworkThreads (including self).
      for (i = 0; i < (int)globalNetThreadInfo.size(); ++i)
      {
        outPktHdr.from = postOffice->GetID();
        outMailHdr.from = currentThread->mailID;
        outPktHdr.to = globalNetThreadInfo[i]->machineID;
        outMailHdr.to = globalNetThreadInfo[i]->mailID;
        outMailHdr.length = strlen(buffer) + 1;
        
        if (!postOffice->Send(outPktHdr, outMailHdr, timeStamp, buffer))
          interrupt->Halt();
      }
    }
    else // if the message is from another thread (including self).
    {
      // Do Total Ordering.
      printf("NET THREAD: Do Total Ordering\n");
      // 1. Extract timestamp and member's ID.
      // This is already done above.
      
      // 2. Update last timestamp, in the table, for that member.
      for (i = 0; i < (int)globalNetThreadInfo.size(); ++i)
      {
        if (inPktHdr.from  == globalNetThreadInfo[i]->machineID &&
            inMailHdr.from == globalNetThreadInfo[i]->mailID)
        {
          globalNetThreadInfo[i]->timeStamp = timeStamp;
          break;
        }
      }
      
      // 3. Insert the message into msgQueue in timestamp order.
      for (i = 0; i < (int)msgQueue.size(); ++i)
      {
        if (msgQueue[i]->lastTimeSent.tv_sec > timeStamp.tv_sec ||
            (msgQueue[i]->lastTimeSent.tv_sec == timeStamp.tv_sec &&
             msgQueue[i]->lastTimeSent.tv_usec > timeStamp.tv_usec))
        {
          msgQueue.insert(msgQueue.begin() + i, new UnAckedMessage(timeStamp, timeStamp, inPktHdr, inMailHdr, buffer));
          break;
        }
      }
      
      // 4. Extract the earliest timestamp value from the table.
      timeval earliestTimeStamp = globalNetThreadInfo[0]->timeStamp;
      for (i = 1; i < (int)globalNetThreadInfo.size(); ++i)
      {
        if (globalNetThreadInfo[i]->timeStamp.tv_sec < earliestTimeStamp.tv_sec ||
            (globalNetThreadInfo[i]->timeStamp.tv_sec == earliestTimeStamp.tv_sec &&
             globalNetThreadInfo[i]->timeStamp.tv_usec < earliestTimeStamp.tv_usec))
        {
          earliestTimeStamp = globalNetThreadInfo[i]->timeStamp;
        }
      }
      
      // 5. Process any message, in timestamp order, with a timestamp <=
      //    value from step 4.
      for (i = 1; i < (int)globalNetThreadInfo.size(); ++i)
      {
        if (msgQueue[i]->lastTimeSent.tv_sec < earliestTimeStamp.tv_sec ||
            (msgQueue[i]->lastTimeSent.tv_sec == earliestTimeStamp.tv_sec &&
             msgQueue[i]->lastTimeSent.tv_usec <= earliestTimeStamp.tv_usec))
        {
          // Process message.
          processMessage(inPktHdr, inMailHdr, timeStamp, buffer);
          
          // Remove the message from the queue.
          msgQueue.erase(msgQueue.begin() + i);
          i--;
        }
      }
    }    
  }
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
      receivedMessagesLock = new Lock("ReceivedMessages Lock");
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
      printf("Commented out msgResendTimer\n");
      //msgResendTimer = new Timer(MsgResendInterruptHandler, 0, false);
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

