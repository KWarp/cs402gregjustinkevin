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
        // Resend the message.
        postOffice->Send(unAckedMessages[i]->pktHdr, unAckedMessages[i]->mailHdr, 
                         unAckedMessages[i]->data);
        
        // The line above will add another message to the end of unAckedMessages with 
        //  lastTimeSent updated and with the same data. Therefore, we need to remove the entry at this index.
        unAckedMessages.erase(unAckedMessages.begin() + i);
        
        // Decrement i to point at the next expected value to examine.
        i--;
      }
    }
  unAckedMessagesLock->Release();
}

int totalNumNetworkThreads = 10;
void RegServer()
{
  PacketHeader inPktHdr, outPktHdr;
  MailHeader inMailHdr, outMailHdr;
  char buffer[MaxMailSize];
  RequestType requestType = INVALIDTYPE;
  timeval timeStamp;
  int i = 0;
  int j = 0;
  int machineID = 0;
  int mailID = 0;
  char c = '?';
  char tmpStr[MaxMailSize];
  char request[MaxMailSize];
  static const int ThreadsPerMessage = 5; 
  
  while ((int)globalNetThreadInfo.size() < totalNumNetworkThreads)
  {
    // Wait for a REGNETTHREAD message.
    while (requestType != REGNETTHREAD)
    {
      postOffice->Receive(postOffice->GetID(), &inPktHdr, &inMailHdr, buffer);
      i = parseMessage(buffer, timeStamp, requestType);
    }
    
    Ack(inPktHdr, inMailHdr, buffer);

    // Parse machineID.
    c = '?';
    while (c != '-')
    {
      c = buffer[i];
      if (c == '-')
        break;
      tmpStr[i++] = c;
    }
    tmpStr[i++] = '\0';
    machineID = atoi(tmpStr);
    
    // Parse mailID.
    c = '?';
    while (c != '\0')
    {
      c = buffer[i];
      if (c == '\0')
        break;
      tmpStr[i++] = c;
    }
    tmpStr[i++] = '\0';
    mailID = atoi(tmpStr);
    
    // Clear the input buffer for next time.
    for(i = 0; i < (int)MaxMailSize; ++i)
      buffer[i] = '\0';
    
    // Add the network thread that just messaged us to globalNetThreadInfo.
    NetThreadInfoEntry* entry = new NetThreadInfoEntry(machineID, mailID);
    globalNetThreadInfo.push_back(entry);
    
    // Respond to the network thread with the number of messages it will receive containing globalNetThreadInfo.
    
    // Clear the output buffer.
    for(i = 0; i < (int)MaxMailSize; ++i)
      request[i] = '\0';
    
    sprintf(request, "%dl:%dl!%d_%d", (int)timeStamp.tv_sec, (int)timeStamp.tv_usec, REGNETTHREADRESPONSE, 
            divRoundUp(totalNumNetworkThreads, ThreadsPerMessage));
    
    outPktHdr.from = postOffice->GetID();
    outMailHdr.from = currentThread->mailID;
    outPktHdr.to = inPktHdr.from;
    outMailHdr.to = inMailHdr.from;
    outMailHdr.length = strlen(request) + 1;
    
    if (!postOffice->Send(outPktHdr, outMailHdr, request))
      interrupt->Halt();
  }
  
  // Message all the threads with globalNetThreadInfo.
  for (i = 0; i < (int)globalNetThreadInfo.size(); ++i)
  {
    // Clear the output buffer.
    for(i = 0; i < (int)MaxMailSize; ++i)
      request[i] = '\0';
  
    sprintf(request, "%dl:%dl!%d_", (int)timeStamp.tv_sec, (int)timeStamp.tv_usec, GROUPINFO);
    
    for (j = 0; j < ThreadsPerMessage && i < (int)globalNetThreadInfo.size(); ++j, ++i)
    {
      // Convert ints to char* strings we can send.
      char machineIDBuf[2];
      char mailIDBuf[3];
      itoa(machineIDBuf, globalNetThreadInfo[i]->machineID, 10);
      itoa(mailIDBuf, globalNetThreadInfo[i]->mailID, 10);
      sprintf(request + strlen(request), "%s%s", machineIDBuf, mailIDBuf);
    }

    // Send the all the threads who are waiting.
    for (j = 0; j < (int)globalNetThreadInfo.size(); ++j)
    {
      outPktHdr.from = postOffice->GetID();
      outMailHdr.from = currentThread->mailID;
      outPktHdr.to = globalNetThreadInfo[j]->machineID;
      outMailHdr.to = globalNetThreadInfo[j]->mailID;
      outMailHdr.length = strlen(request) + 1;
      
      if (!postOffice->Send(outPktHdr, outMailHdr, request))
        interrupt->Halt();
    }
  }
}

// Register with the Registration Server before we start processing messages.
void RegisterNetworkThread()
{
  PacketHeader inPktHdr, outPktHdr;
  MailHeader inMailHdr, outMailHdr;
  char buffer[MaxMailSize];
  RequestType requestType = INVALIDTYPE;
  timeval timeStamp;
  int i = 0;
  int numMessages = 0;
  char c = '?';
  char tmpStr[MaxMailSize];
  char request[MaxMailSize];
  
  // Wait for StartSimulation message from UserProgram.
  while (requestType != STARTSIMULATION)
  {
    postOffice->Receive(postOffice->GetID(), &inPktHdr, &inMailHdr, buffer);
    parseMessage(buffer, timeStamp, requestType);
  }
  
  // Ack that we got the message by removing it from unAckedMessages.
  Ack(inPktHdr, inMailHdr, buffer);
  
  // Message server with machineID and mailBoxID.
  sprintf(request, "%dl:%dl!%d_%d-%d", (int)timeStamp.tv_sec, (int)timeStamp.tv_usec, REGNETTHREAD, 
          postOffice->GetID(), currentThread->mailID);
	
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
  
  // Wait for an Ack message from Registration Server.
  // The Ack will contain numMessages.
  requestType = INVALIDTYPE;
  while (requestType != REGNETTHREADRESPONSE)
  {
    postOffice->Receive(postOffice->GetID(), &inPktHdr, &inMailHdr, buffer);
    i = parseMessage(buffer, timeStamp, requestType);
  }
  
  Ack(inPktHdr, inMailHdr, buffer);
  
  // Parse numMessages.
  c = '?';
  while (c != '\0')
  {
    c = buffer[i];
    if (c == '\0')
      break;
    tmpStr[i++] = c;
  }
  tmpStr[i++] = '\0';
  numMessages = atoi(tmpStr);
  
  // Clear the input buffer for next time.
	for(i = 0; i < (int)MaxMailSize; ++i)
		buffer[i] = '\0';
  
  // Wait for all of the messages.
  for (i = 0; i < numMessages; ++i)
  {
    while (requestType != GROUPINFO)
    {
      postOffice->Receive(postOffice->GetID(), &inPktHdr, &inMailHdr, buffer);
      i = parseMessage(buffer, timeStamp, requestType);
    }
    
    Ack(inPktHdr, inMailHdr, buffer);

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
      tmpStr[2] = buffer[i + 2];
      tmpStr[3] = buffer[i + 3];
      tmpStr[4] = '\0';
      
      machineID = atoi(tmpStr);
      mailID = atoi(&tmpStr[2]);
      NetThreadInfoEntry* entry = new NetThreadInfoEntry(machineID, mailID);
      globalNetThreadInfo.push_back(entry);
    
      i += 3;
    }
  }
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
  sprintf(tmpStr, "%dl:%dl!", (int)timeStamp.tv_sec, (int)timeStamp.tv_usec);
  
  // Copy the timeStamp to buffer.
  int i = 0;
  while (tmpStr[i] != '!')
  {
    buffer[i] = tmpStr[i];
    i++;
  }
}

void NetworkThread()
{
  PacketHeader inPktHdr, outPktHdr;
  MailHeader inMailHdr, outMailHdr;
  char buffer[MaxMailSize];
  int requestType = -1;
  timeval timeStamp;
  int i = 0;
  int numMessages = 0;
  char c = '?';
  char tmpStr[MaxMailSize];
  
  RegisterNetworkThread();
  
  while (true)
  {
    // Wait for a message to be received.
    postOffice->Receive(postOffice->GetID(), &inPktHdr, &inMailHdr, buffer);
    
    // Signal that we have received the message.
    Ack(inPktHdr, inMailHdr, buffer);
    
    // If we have already received and handled the message (the sender failed to receive our response Ack).
    if (messageIsRedundant(inPktHdr, inMailHdr, buffer))
    {
      // Ignore the redundant message.
      continue;
    }
    
    // Add message to receivedMessages.
    gettimeofday(&timeStamp, NULL);
    receivedMessages.push_back(new UnAckedMessage(timeStamp, inPktHdr, inMailHdr, buffer));
    
    // If the message is from my UserProgram thread.
    if (inPktHdr.from == postOffice->GetID() &&
        inMailHdr.from == currentThread->mailID + 1)
    {
      // Append a new timeStamp but keep the same data.
      updateTimeStamp(buffer);
      
      // Forward message to all NetworkThreads (including self).
      for (i = 0; i < (int)globalNetThreadInfo.size(); ++i)
      {
        outPktHdr.from = postOffice->GetID();
        outMailHdr.from = currentThread->mailID;
        outPktHdr.to = globalNetThreadInfo[i]->machineID;
        outMailHdr.to = globalNetThreadInfo[i]->mailID;
        outMailHdr.length = strlen(buffer) + 1;
        
        if (!postOffice->Send(outPktHdr, outMailHdr, buffer))
          interrupt->Halt();
      }
    }
    else // if the message is from another thread (including self).
    {
      // Do Total Ordering.
      // Process all the messages we can.
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
      msgResendTimer = new Timer(MsgResendInterruptHandler, 0, false);
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
    postOffice = new PostOffice(netname, rely, 10);
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

