#include "netcall.h"
#include <sys/time.h>

extern "C" {
	int bcopy(char *, char *, int);
};

Message::Message(PacketHeader pHdr, MailHeader mHdr, char *d)
{
	pktHdr = pHdr;
  mailHdr = mHdr;
  data = d;
}

DistributedLock::DistributedLock(const char* debugName) 
{
	name = debugName;	
	lockState = FREE;
	waitQueue = new List;
	ownerMachID = -1;		
	ownerMailID = -1;
	globalId = -1;
}

void DistributedLock::QueueReply(Message* reply){	waitQueue->Append((void*)reply);}

Message* DistributedLock::RemoveReply(){	return (Message*)waitQueue->Remove();}

bool DistributedLock::isQueueEmpty(){	return (Message*)waitQueue->IsEmpty();}

void DistributedLock::setLockState(bool ls){ 	lockState = ls; }

void DistributedLock::setOwner(int machID, int mailID){	ownerMachID = machID;	ownerMailID = mailID;}

DistributedLock::~DistributedLock() {	delete waitQueue;}

DistributedMV::DistributedMV(const char* debugName) {	name = debugName;	globalId = -1;}

DistributedMV::~DistributedMV() {}

DistributedCV::DistributedCV(const char* debugName) {	name=debugName;	firstLock=NULL;	waitQueue=new List;	ownerMachID = -1;	ownerMailID = -1;	globalId = -1;}

void DistributedCV::QueueReply(Message* reply){	waitQueue->Append((void*)reply);}

Message* DistributedCV::RemoveReply(){	return (Message*)waitQueue->Remove();}

bool DistributedCV::isQueueEmpty(){	return (Message*)waitQueue->IsEmpty();}

void DistributedCV::setFirstLock(int fl){ 	firstLock = fl; }

void DistributedCV::setOwner(int machID, int mailID){	ownerMachID = machID;	ownerMailID = mailID;}

DistributedCV::~DistributedCV() {	delete waitQueue;}

char verifyBuffer[MAX_CLIENTS][4][5];
char lockName[MAX_CLIENTS][32];
int verifyResponses[MAX_CLIENTS];
int localLockIndex[MAX_CLIENTS];

enum CreationState {NO, MAYBE, YES};
CreationState lockCreationStates[MAX_CLIENTS];
CreationState cvCreationStates[MAX_CLIENTS];
CreationState mvCreationStates[MAX_CLIENTS];

timeval lockCreationTimeStamps[MAX_CLIENTS];
timeval cvCreationTimeStamps[MAX_CLIENTS];
timeval mvCreationTimeStamps[MAX_CLIENTS];

int waitingForCreateLock[MAX_CLIENTS];
int waitingForSignalCvs[MAX_CLIENTS];
int waitingForBroadcastCvs[MAX_CLIENTS];
int waitingForWaitLocks[MAX_CLIENTS];

DistributedMV* mvs[MAX_DMV]; 
DistributedCV* cvs[MAX_DCV]; 
DistributedLock* dlocks[MAX_DLOCK]; 
PacketHeader errorOutPktHdr, errorInPktHdr; 
MailHeader errorOutMailHdr, errorInMailHdr; 
char buffer[MaxMailSize];
int createDLockIndex = 0; 
int createDCVIndex = 0; 
int createDMVIndex = 0; 

bool processAck(PacketHeader inPktHdr, MailHeader inMailHdr, timeval timeStamp)
{
  #if 0
    printf("processing ACK: From (%d, %d) to (%d, %d) bytes %d, time %d.%d\n",
           inPktHdr.from, inMailHdr.from, 
           inPktHdr.to, inMailHdr.to, inMailHdr.length,
           (int)timeStamp.tv_sec, (int)timeStamp.tv_usec);
  #endif

  unAckedMessagesLock->Acquire();
    bool found = false;
    for (int i = 0; i < (int)unAckedMessages.size(); ++i)
    {
      if (unAckedMessages[i]->pktHdr.from == inPktHdr.to &&
          unAckedMessages[i]->pktHdr.to == inPktHdr.from &&
          unAckedMessages[i]->mailHdr.from == inMailHdr.to &&
          unAckedMessages[i]->mailHdr.to == inMailHdr.from &&
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
      printf("processAck: Ack message not found in unAckedMessages!!!\n");
  unAckedMessagesLock->Release();
  
  return found;
}

void Ack(PacketHeader inPktHdr, MailHeader inMailHdr, timeval timeStamp, char* inData)
{
  // If the message is from ourselves or our UserProg thread.
  if ((inPktHdr.from == postOffice->GetID() &&
      inMailHdr.from == currentThread->mailID) ||
      (inPktHdr.from == postOffice->GetID() &&
      inMailHdr.from == currentThread->mailID + 1) ||
      (inPktHdr.from == postOffice->GetID() &&
      inMailHdr.from == currentThread->mailID - 1))
  {
    // Find the message in unAckedMessages.
    unAckedMessagesLock->Acquire();
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
          #if 0
            printf("Doing Local ACK: From (%d, %d) to (%d, %d) bytes %d, time %d.%d\n",
                   inPktHdr.from, inMailHdr.from, 
                   inPktHdr.to, inMailHdr.to, inMailHdr.length,
                   (int)timeStamp.tv_sec, (int)timeStamp.tv_usec);
          #endif
          
          // Remove the entry from unAckedMessage to simulate an Ack.
          unAckedMessages.erase(unAckedMessages.begin() + i);
          found = true;
          break;
        }
      }
      if (!found)
        {/* Error! */}
    unAckedMessagesLock->Release(); 
  }
  else // If the message is from another thread (including self?).
  {
    // Send an Ack to the message sender.
    char request[MaxMailSize];
    sprintf(request, "%d_", ACK);
    
    PacketHeader outPktHdr;
    MailHeader outMailHdr;
    outPktHdr.from = postOffice->GetID();
    outMailHdr.from = currentThread->mailID;
    outPktHdr.to = inPktHdr.from;
    outMailHdr.to = inMailHdr.from;
    outMailHdr.length = strlen(request) + 1;
    
    #if 0 // For debugging.
      printf("Sending ACK: From (%d, %d) to (%d, %d) bytes %d, time %d.%d\n",
           outPktHdr.from, outMailHdr.from, 
           outPktHdr.to, outMailHdr.to, outMailHdr.length,
           (int)timeStamp.tv_sec, (int)timeStamp.tv_usec);
    #endif
    
    if (!postOffice->Send(outPktHdr, outMailHdr, timeStamp, request, false))
      interrupt->Halt();
  }
}

int Request(RequestType requestType, char* data, int machineID, int mailID)
{
  // Send an Ack to the message sender.
  timeval timeStamp;
  
  char request[MaxMailSize];
  sprintf(request, "%d_%s", requestType, data);
  
	for(int i = 0; i < (int)MaxMailSize; ++i)
		buffer[i]='\0';
		
  PacketHeader outPktHdr, inPktHdr;
  MailHeader outMailHdr, inMailHdr;

  outPktHdr.from = postOffice->GetID();
  outMailHdr.from = currentThread->mailID;
  outPktHdr.to = machineID;
  outMailHdr.to = mailID;
  outMailHdr.length = strlen(request) + 1;
    
  printf("Request: %s\n", request);

  if (!postOffice->Send(outPktHdr, outMailHdr, request))
    interrupt->Halt();

  RequestType receiveRequestType = INVALIDTYPE;
  
  // ACKs can occur here, keep waiting for anything else
  while (receiveRequestType == INVALIDTYPE || receiveRequestType == ACK)
  {
    postOffice->Receive(currentThread->mailID, &inPktHdr, &inMailHdr, &timeStamp, buffer);
    parseValue(0, buffer, (int*)(&receiveRequestType));
    
    if (requestType == ACK)
    {
      printf("Request: Processing ACK %s\n", buffer);
      processAck(inPktHdr, inMailHdr, timeStamp);
    }
  }
  
  printf("Request receive RequestType: %d, msg: %s\n", receiveRequestType, buffer);
  
  Ack(inPktHdr, inMailHdr, timeStamp, buffer);
  
  return atoi(buffer); 
}

// Parses timeStamp and requestType from the data packet, buf.
// Returns the index of the start of data.
int parseValue(int startIndex, const char* buf, int* value)
{
  char valueStr[MaxMailSize];
  char c = '?';
  int i = startIndex;
 
  ASSERT(i >= 0);
 
  // Parse requestType.
  while (c != '_' && c != '\0')
  {
    c = buf[i];
    if (c == '_' || c == '\0')
      break;
    valueStr[i++ - startIndex] = c;
  }
  valueStr[i++ - startIndex] = '\0';
  
  if (value != NULL)
    *value = atoi(valueStr);
  
  //printf("parsed value: %d\n", *value);
  // i will point to the next character of data.
  return i;
}

int uniqueGlobalID()
{
  // (machineID * total Net Threads) + mailID
  return (postOffice->GetID() * 100) + currentThread->mailID;
}


bool processMessage(PacketHeader inPktHdr, MailHeader inMailHdr, timeval timeStamp, char* msgData, 
                    vector<NetThreadInfoEntry*> *localNetThreadInfo)
{
	printf("Processing Message from (%d, %d) to (%d, %d), time %d.%d, msg: %s\n", inPktHdr.from, inMailHdr.from, 
         inPktHdr.to, inMailHdr.to, (int)timeStamp.tv_sec, (int)timeStamp.tv_usec, msgData);
	
  PacketHeader outPktHdr;
  MailHeader outMailHdr;
  
	RequestType requestType = INVALIDTYPE;
  
	char* localLockName = new char[32];		
	char lockIndexBuf[MaxMailSize];
	char mvIndexBuf[MaxMailSize];	
	char mvValBuf[MaxMailSize];	
	char* cvName = new char[32];	
	char cvNum[MaxMailSize];	
	char lockNum[MaxMailSize];
	
	bool success = false;
	
	int lockIndex = -1;  
	char* mvName = new char[32];
	int mvIndex = -1;
	int mvValue = 0;
	int cvIndex = -1;
	int lockNameIndex = 0;
	int cvNameIndex = 0;
	int cvNumIndex = 0;
	int lockNumIndex = 0;
	int otherServerLockIndex = -1;
	int otherServerIndex=0;
	char client[5];
	int clientNum;  /// Convention: clientNum = 100 * machineID + mailID 
                  ///   mailID can range from [0,99] so that means there is a maximum of 100 threads per machine id.
	char* request = NULL;
  
  ASSERT(serverCount == (int)localNetThreadInfo->size());
  
  // Fill buffer with '\0' chars.
  for(int i = 0; i < (int)MaxMailSize; ++i)
		buffer[i]='\0';
  
  // Parse timeStamp and requestType from message.
  int i = parseValue (0, msgData, (int*)(&requestType));
	char c = msgData[i];
	
  // Do this because all the code below assumes we are one the char before this, and I am too lazy to change it everywhere.
  i--;
  
  int a, m, j, x;
	switch (requestType)
	{
		case CREATELOCK:
      printf("CREATELOCK\n");
			clientNum = 100 * inPktHdr.from + inMailHdr.from - 1;
			if (createDLockIndex >= 0 && createDLockIndex < (MAX_DLOCK - 1))
			{
        // Parse the lock name.
				j = 0;
				while (c != '\0')
				{
					c = msgData[++i];
					if (c == '\0')
						break;
					lockName[clientNum][j++] = c;
				}
				lockName[clientNum][j] = '\0';

        // If we have already created a lock with lockName, set the appropriate lockIndex.
				for (int k = 0; k < createDLockIndex; ++k)
				{
					if (!strcmp(dlocks[k]->getName(), lockName[clientNum]))
					{
						lockIndex = k;
					}
				}
        
        // Store lock index for later.
				localLockIndex[clientNum] = lockIndex;
				if (serverCount > 1)
        {
          // If the lock didn't already exist.
					if (localLockIndex[clientNum] == -1)
					{
            // Store the current timeStamp so we can use it later to determine ownership.
            timeval currentTimeStamp;
            gettimeofday(&currentTimeStamp, NULL);
            lockCreationTimeStamps[clientNum] = currentTimeStamp;
            
            // Initilize the number of reponses we have received
            verifyResponses[clientNum] = 0;
            
            // Set the state to MAYBE, meaning we have a request pending.
            lockCreationStates[clientNum] = MAYBE;
            
            // Broadcast a request to all the other threads.
            CreateVerify(lockName[clientNum], CREATELOCKVERIFY, clientNum, localNetThreadInfo);
          }
					else  // if the lock already existed, simply return its value.
					{
						lockIndex = dlocks[localLockIndex[clientNum]]->GetGlobalId();
            
            // Send response to client.
            char ack[MaxMailSize];
            sprintf(ack, "%i", lockIndex);
            
            outPktHdr.to = clientNum / 100;
            outMailHdr.to = (clientNum + 100) % 100 + 1; // +1 to reach user thread
            outPktHdr.from = postOffice->GetID();
            outMailHdr.from = currentThread->mailID;
            outMailHdr.length = strlen(ack) + 1;
            
            printf("Sending CREATELOCK response: %s\n", ack);
            success = postOffice->Send(outPktHdr, outMailHdr, ack); 
            
            if ( !success ) 
              interrupt->Halt();
            
            verifyResponses[clientNum] = 0; // Set verifyResponses to 0 to prepare for incoming responses (which there won't be any since this is the single server case).
            localLockIndex[clientNum] = -1; // Reset localLockIndex for next time.
            fflush(stdout);
          }
        }
				else
				{
          // If the lock didn't already exist.
					if (localLockIndex[clientNum] == -1)
					{
            // Store the lockName for later.
						for (i = 0; lockName[clientNum][i] != '\0'; i++)
						{
							localLockName[i] = lockName[clientNum][i];
							localLockName[i + 1]= '\0';
						}
            
            // Create a new DistrbituedLock.
						dlocks[createDLockIndex] = new DistributedLock(localLockName); 
						dlocks[createDLockIndex]->SetGlobalId((uniqueGlobalID() * MAX_DLOCK) + createDLockIndex);
            
            // If we failed to create the new DistributedLock.
						if (dlocks[createDLockIndex] == NULL)
						{
							errorOutPktHdr.to = clientNum / 100;
							errorInPktHdr = inPktHdr;
							errorOutMailHdr.to = (clientNum + 100) % 100;
							errorInMailHdr = inMailHdr;
							return false;
						}

            // CreateDLock succeeded. Get the new lock's global id and increment createDLockIndex for next time.
            lockIndex = dlocks[createDLockIndex]->GetGlobalId();
            createDLockIndex++;
					}
					else  // if the lock already existed, simply grab its value.
					{		
						lockIndex = dlocks[localLockIndex[clientNum]]->GetGlobalId();
					}
          
          // Send response to client.
					char ack[MaxMailSize];
					sprintf(ack, "%i", lockIndex);
					
					outPktHdr.to = clientNum / 100;
          outMailHdr.to = (clientNum + 100) % 100 + 1; // +1 to reach user thread
          outPktHdr.from = postOffice->GetID();
          outMailHdr.from = currentThread->mailID;
          outMailHdr.length = strlen(ack) + 1;
          
          printf("Sending CREATELOCK response: %s\n", ack);
					success = postOffice->Send(outPktHdr, outMailHdr, ack); 
          
					if ( !success ) 
					  interrupt->Halt();
          
					verifyResponses[clientNum] = 0; // Set verifyResponses to 0 to prepare for incoming responses (which there won't be any since this is the single server case).
					localLockIndex[clientNum] = -1; // Reset localLockIndex for next time.
					fflush(stdout);
				}
			}
			else  // if the createDLockIndex is an invalid range.
			{
				errorOutPktHdr = outPktHdr;
				errorInPktHdr = inPktHdr;
				errorOutMailHdr = outMailHdr;
				errorInMailHdr = inMailHdr;
				return false;
			}
			break;
			
		case CREATELOCKANSWER:
			printf("CREATELOCKANSWER\n");
      x = 0;
      a = 0;
      
      // Parse clientNum.
			do 
			{
				c = msgData[++i];
				if (c == '_')
					break;
				client[x++] = c;	
			} while (c != '_');
			client[x] = '\0';
			clientNum = atoi(client);
      
      // Parse verifyResponse (as a string).
			while (c != '\0')
			{
				c = msgData[++i];
				verifyBuffer[clientNum][verifyResponses[uniqueGlobalID()]][a++] = c;
				if (c == '\0')
					break;
			}
      
      // If the lock didn't already exist.
      if (localLockIndex[clientNum] == -1)
      {
        // Parse the response.
        int verifyResponse;
        int index = parseValue(0, verifyBuffer[clientNum][verifyResponses[clientNum]], &verifyResponse);
        
        // If the message we just received is a "Yes", i.e. that server has already created the lock and is the owner.
        if (verifyResponse == YES)
        {
          // Parse the lockIndex.
          index = parseValue(index, verifyBuffer[clientNum][verifyResponses[clientNum]], &lockIndex);
          
          // Copy the lockName that our userprog sent to us in the initial request into localLockName.
					for (i = 0; lockName[clientNum][i] != '\0'; i++)
          {
						localLockName[i] = lockName[clientNum][i];
						localLockName[i + 1] = '\0';
					}
          
          lockCreationStates[clientNum] = YES;
          
          // Actually create the lock.
					dlocks[createDLockIndex] = new DistributedLock(localLockName); 
					dlocks[createDLockIndex]->SetGlobalId(lockIndex);
          
          // If we failed to create the lock.
					if (dlocks[createDLockIndex] == NULL)
					{
						printf("Error in creation of %s!\n", lockName[clientNum]);
						errorOutPktHdr.to = clientNum / 100;
						errorInPktHdr = inPktHdr;
						errorOutMailHdr.to = (clientNum + 100) % 100;
						errorInMailHdr = inMailHdr;
						return false;
					}

          // Communicate to my paired user thread to wake it up.
          char ack[MaxMailSize];
          sprintf(ack, "%i", lockIndex);
          
          outPktHdr.to = postOffice->GetID();
          outMailHdr.to = currentThread->mailID + 1; // +1 to reach user thread
          outPktHdr.from = postOffice->GetID();
          outMailHdr.from = currentThread->mailID;
          outMailHdr.length = strlen(ack) + 1;
          
          printf("Sending CREATELOCKANSWER from (%d, %d) to (%d, %d), msg: %s\n", 
                 outPktHdr.from, outMailHdr.from, outPktHdr.to, outMailHdr.to, ack);
          success = postOffice->Send(outPktHdr, outMailHdr, ack); 

          if (!success) 
            interrupt->Halt();

          // Increment the number of responses.
          verifyResponses[clientNum]++;
        }
        else if (verifyResponse == NO || verifyResponse == MAYBE)
        {
          // Parse the timeStamp when the other thread began creating its lock.
          timeval responseTimeStamp;
          int sec, usec;
          index = parseValue(index, verifyBuffer[clientNum][verifyResponses[clientNum]], &sec);
          index = parseValue(index, verifyBuffer[clientNum][verifyResponses[clientNum]], &usec);
          responseTimeStamp.tv_sec = sec;
          responseTimeStamp.tv_usec = usec;

          if (lockCreationTimeStamps[clientNum].tv_sec < responseTimeStamp.tv_sec ||
              (lockCreationTimeStamps[clientNum].tv_sec == responseTimeStamp.tv_sec &&
               lockCreationTimeStamps[clientNum].tv_usec < responseTimeStamp.tv_usec))
          {
            // Increment the number of responses we've received.
            verifyResponses[clientNum]++;
            
            // If all server threads have responded to my request.
            if (verifyResponses[clientNum] == serverCount - 1)
            {
              // Copy the lockName that our userprog sent to us in the initial request into localLockName.
              for (i = 0; lockName[clientNum][i] != '\0'; i++)
              {
                localLockName[i] = lockName[clientNum][i];
                localLockName[i + 1] = '\0';
              }
              
              lockCreationStates[clientNum] = YES;
              
              // Actually create the lock.
              dlocks[createDLockIndex] = new DistributedLock(localLockName); 
              dlocks[createDLockIndex]->SetGlobalId((uniqueGlobalID() * MAX_DLOCK) + createDLockIndex);
              
              // If we failed to create the lock.
              if (dlocks[createDLockIndex] == NULL)
              {
                printf("Error in creation of %s!\n", lockName[clientNum]);
                errorOutPktHdr.to = clientNum / 100;
                errorInPktHdr = inPktHdr;
                errorOutMailHdr.to = (clientNum + 100) % 100;
                errorInMailHdr = inMailHdr;
                return false;
              }
              
              // Save lockIndex and increment createDLockIndex for next time.
              lockIndex = dlocks[createDLockIndex]->GetGlobalId();
              createDLockIndex++;
                
              // Broadcast a "YES" to everyone so they know we are the new owner.
              char tmp[MaxMailSize];
              sprintf(tmp, "%d_%d_", YES, lockIndex);
              CreateVerify(tmp, CREATELOCKANSWER, clientNum, localNetThreadInfo);
                
              // Communicate to my paired user thread to wake it up.
              char ack[MaxMailSize];
              sprintf(ack, "%i", lockIndex);
              
              outPktHdr.to = postOffice->GetID();
              outMailHdr.to = currentThread->mailID + 1; // +1 to reach user thread
              outPktHdr.from = postOffice->GetID();
              outMailHdr.from = currentThread->mailID;
              outMailHdr.length = strlen(ack) + 1;
              
              printf("Sending CREATELOCKANSWER from (%d, %d) to (%d, %d), msg: %s\n", 
                     outPktHdr.from, outMailHdr.from, outPktHdr.to, outMailHdr.to, ack);
              success = postOffice->Send(outPktHdr, outMailHdr, ack); 

              if (!success) 
                interrupt->Halt();

              localLockIndex[clientNum] = -1;
              fflush(stdout);
            }
          }
        }
      }
			break;

		case CREATELOCKVERIFY:	
			printf("CREATELOCKVERIFY\n");
			lockIndex = -1;
			request = new char [MaxMailSize];
			x = 0;
      a = 0;
      
      // Parse the clientNum.
			do 
			{
				c = msgData[++i];
				if (c == '_')
					break;
				client[x++] = c;	
			} while (c != '_');
			client[x]='\0';
			clientNum = atoi(client);
      
      // Parse the lockName.
			while (c != '\0')
			{
				c = msgData[++i];
				if (c == '\0')
					break;
				localLockName[a++] = c;
			}
			localLockName[a] = '\0';
      
      // Check if the lock already exists.
			for (int k = 0; k < createDLockIndex; k++)
			{
				if (!strcmp(dlocks[k]->getName(), localLockName))
				{
					lockIndex = k;
					break;
				}
			}
			
			outPktHdr.to = inPktHdr.from;
			outMailHdr.to = inMailHdr.from;
      outPktHdr.from = postOffice->GetID();
			outMailHdr.from = currentThread->mailID;
      
      // If the lock hasn't already been created on this server.
      if (lockCreationStates[clientNum] == MAYBE)
      {
        printf("Lock named %s being created already\n", localLockName);
				sprintf(request, "%d_%d_%d_%d_%d", CREATELOCKANSWER, clientNum, MAYBE,
                (int)lockCreationTimeStamps[uniqueGlobalID()].tv_sec, (int)lockCreationTimeStamps[uniqueGlobalID()].tv_usec);
				outMailHdr.length = strlen(request) + 1;

				success = postOffice->Send(outPktHdr, outMailHdr, request);
      }
			else if (lockIndex == -1)
			{
        printf("Lock named %s not found\n", localLockName);
				sprintf(request, "%d_%d_%d", CREATELOCKANSWER, clientNum, NO);
				outMailHdr.length = strlen(request) + 1;

				success = postOffice->Send(outPktHdr, outMailHdr, request);
			}
			else  // If the lock does exist on the server.
			{
				sprintf(request, "%d_%d_%d_%d", CREATELOCKANSWER, clientNum, YES, dlocks[lockIndex]->GetGlobalId());
				outMailHdr.length = strlen(request) + 1;
				success = postOffice->Send(outPktHdr, outMailHdr, request);
			}
			break;

		case CREATECV:
      printf("CREATECV\n");
			clientNum = 100 * inPktHdr.from + inMailHdr.from - 1;
			if (createDCVIndex >= 0 && createDCVIndex < (MAX_DCV - 1))
			{
        // Parse lockName.
				j = 0;
				while (c != '\0')
				{
					c = msgData[++i];
					if (c == '\0')
						break;
					lockName[clientNum][j++] = c; 
				}
				lockName[clientNum][j] = '\0';
        
        // If the cv has already been created on this server.
				for (int k = 0; k < createDCVIndex; k++)
				{
					if (!strcmp(cvs[k]->getName(), lockName[clientNum]))
					{
						cvIndex = k;
					}
				}
        
				localLockIndex[clientNum]=cvIndex;
				if (serverCount > 1)
        {
          // If the cv hasn't been created yet.
					if (localLockIndex[clientNum] == -1) 
					{
            // Store the current timeStamp so we can use it later to determine ownership.
            timeval currentTimeStamp;
            gettimeofday(&currentTimeStamp, NULL);
            cvCreationTimeStamps[clientNum] = currentTimeStamp;
            
            // Set the state to MAYBE, meaning we have a request pending.
            cvCreationStates[clientNum] = MAYBE;
            
            // Broadcast a request to all the other threads.
            CreateVerify(lockName[clientNum], CREATECVVERIFY, clientNum, localNetThreadInfo);
          }
					else  // if the cv has already been created.
					{			
						cvIndex = cvs[cvIndex]->GetGlobalId();
          
            // Send the cvIndex to the client.
            char ack[MaxMailSize];
            sprintf(ack, "%i", cvIndex);
      
            outPktHdr.to = clientNum / 100;
            outMailHdr.to = (clientNum + 100) % 100;
            outMailHdr.from = postOffice->GetID();
            outMailHdr.length = strlen(ack) + 1;
            success = postOffice->Send(outPktHdr, outMailHdr, ack); 

            if (!success) 
              interrupt->Halt();

            // Reset counters for next time.
            verifyResponses[clientNum] = 0;
            localLockIndex[clientNum] = -1;
            fflush(stdout);
          }
        }
				else
				{
          // If the cv hasn't been created yet.
					if (localLockIndex[clientNum] == -1) 
					{
            // Store the lockName for later.
						for (i = 0; lockName[clientNum][i] != '\0'; i++)
						{
							localLockName[i] = lockName[clientNum][i];
							localLockName[i + 1] = '\0';
						}
            
            // Create the cv.
						cvs[createDCVIndex] = new DistributedCV(localLockName); 
						cvs[createDCVIndex]->SetGlobalId((uniqueGlobalID()*MAX_DCV)+createDCVIndex);
            
            // If creating the cv failed.
						if (cvs[createDCVIndex] == NULL)
						{
							errorOutPktHdr.to = clientNum / 100;
							errorInPktHdr = inPktHdr;
							errorOutMailHdr.to = (clientNum + 100) % 100;
							errorInMailHdr = inMailHdr;
							return false;
						}
						
            // Store the globalId and increment createDCVIndex for next time.
						cvIndex = cvs[createDCVIndex]->GetGlobalId();
						createDCVIndex++;
					}
					else  // if the cv has already been created.
					{			
						cvIndex = cvs[cvIndex]->GetGlobalId();
					}
          
          // Send the cvIndex to the client.
					char ack[MaxMailSize];
					sprintf(ack, "%i", cvIndex);
		
					outPktHdr.to = clientNum / 100;
					outMailHdr.to = (clientNum + 100) % 100;
					outMailHdr.from = postOffice->GetID();
					outMailHdr.length = strlen(ack) + 1;
					success = postOffice->Send(outPktHdr, outMailHdr, ack); 

					if (!success) 
					  interrupt->Halt();

          // Reset counters for next time.
					verifyResponses[clientNum] = 0;
					localLockIndex[clientNum] = -1;
					fflush(stdout);
				}
			}
			else
			{
				errorOutPktHdr = outPktHdr;
				errorInPktHdr = inPktHdr;
				errorOutMailHdr = outMailHdr;
				errorInMailHdr = inMailHdr;
				return false;
			}
			break;
      
		case CREATECVANSWER:
      printf("CREATECVANSWER\n");
			x=0;
      
      // Parse clientNum.
			do 
			{
				c = msgData[++i];
				if (c == '_')
					break;
				client[x++] = c;	
			} while(c != '_'); 
			client[x] = '\0';
			clientNum = atoi(client);
			
      // Parse verifyResponse.
			while (c != '\0') 
			{
				c = msgData[++i];
				verifyBuffer[clientNum][verifyResponses[clientNum]][a++] = c;
				if (c == '\0')
					break;
			}
			verifyResponses[clientNum]++;
      
      // If we have received verification from every other server.
			if (verifyResponses[clientNum] == serverCount - 1)
			{
				for (i = 0; i < serverCount - 1; i++)
				{
					if (verifyBuffer[clientNum][i][0] == '-' && verifyBuffer[clientNum][i][1] == '1')
					{
						otherServerLockIndex = -1;
					}
					else
					{	
						otherServerLockIndex = atoi(verifyBuffer[clientNum][i]);
						break;
					}
				}
				if (localLockIndex[clientNum] == -1 && otherServerLockIndex == -1) // If not found here or elsewhere
				{
          // Store the cv name for later.
					for (i = 0; lockName[clientNum][i] != '\0'; i++)
          {
						localLockName[i] = lockName[clientNum][i];
						localLockName[i + 1] = '\0';
					}
          
          // Actually create the cv.
					cvs[createDCVIndex] = new DistributedCV(localLockName); 
					cvs[createDCVIndex]->SetGlobalId((uniqueGlobalID() * MAX_DCV) + createDCVIndex);
          
          // If createCV failed.
					if (cvs[createDCVIndex] == NULL)
					{
						errorOutPktHdr.to = clientNum / 100;
						errorInPktHdr = inPktHdr;
						errorOutMailHdr.to = (clientNum + 100) % 100;
						errorInMailHdr = inMailHdr;
						return false;
					}
					
          // Set the cvIndex and increment createDCVIndex for next time.
					cvIndex = cvs[createDCVIndex]->GetGlobalId();
					createDCVIndex++;
				}
				else if (localLockIndex[clientNum] == -1 && otherServerLockIndex !=-1)
				{
					cvIndex=otherServerLockIndex; //set lock
				}
				else if (localLockIndex[clientNum] != -1 && otherServerLockIndex==-1) 
				{
					cvIndex=cvs[localLockIndex[clientNum]]->GetGlobalId();
				}
				else 
				{}
        
				char ack[MaxMailSize];
				sprintf(ack, "%i", cvIndex);
        
        // Communicate to my paired user thread.
				outPktHdr.to = postOffice->GetID();
				outMailHdr.to = currentThread->mailID + 1; // +1 to reach user thread
				outPktHdr.from = postOffice->GetID();
        outMailHdr.from = currentThread->mailID;
				outMailHdr.length = strlen(ack) + 1;

				success = postOffice->Send(outPktHdr, outMailHdr, ack); 

				if (!success) 
				  interrupt->Halt();

        // Reset counters for next time.
				verifyResponses[clientNum]=0;
				localLockIndex[clientNum]=-1;
				fflush(stdout);
			}
			break;
      
		case CREATECVVERIFY:
      printf("CREATECVVERIFY\n");
			lockIndex = -1;
			
			request = new char [MaxMailSize];
			x=0;
      
      // Parse clientNum.
			do 
			{
				c = msgData[++i];
				if (c == '_')
					break;
				client[x++] = c;	
			} while (c != '_');
			client[x]='\0';
			clientNum = atoi(client);
      
      // Parse cvName.
			while(c!='\0')
			{
				c = msgData[++i];
				if (c == '\0')
					break;
				localLockName[a++] = c; 
			}
			localLockName[a] = '\0';
      
      // If we have created the cv already on this server.
			for (j = 0; j < createDCVIndex; j++)
			{
				if (!strcmp(cvs[j]->getName(), localLockName))
				{
					cvIndex = j;
				}
			}
      
			outPktHdr.to = inPktHdr.from;
			outMailHdr.to = inMailHdr.from;
			outMailHdr.from = postOffice->GetID();
      
      // If we haven't created the cv.
			if (cvIndex == -1) 
			{
				sprintf(request,"%d_%d_%d", CREATECVANSWER, clientNum, -1);
				outMailHdr.length = strlen(request) + 1;
				success = postOffice->Send(outPktHdr, outMailHdr, request); 
			}
			else  // If we have created the cv
			{
				sprintf(request, "%d_%d_%d", CREATECVANSWER, clientNum, cvs[cvIndex]->GetGlobalId());
				outMailHdr.length = strlen(request) + 1;
				success = postOffice->Send(outPktHdr, outMailHdr, request); 
			}
			break;
      
		case CREATEMV:
      printf("CREATEMV\n");
			clientNum=100 * inPktHdr.from + inMailHdr.from - 1;
			if (createDMVIndex >= 0 && createDMVIndex < (MAX_DMV - 1))
			{
        // Parse mvName.
				j = 0;
				while (c != '\0')
				{
					c = msgData[++i];
					if (c == '\0')
						break;
					lockName[clientNum][j++] = c;  
				}
				lockName[clientNum][j] = '\0' ;  
				
        // Check if we have already created the mv on this server.
				for (int k = 0; k < createDMVIndex; k++)
				{
					if (!strcmp(mvs[k]->getName(), lockName[clientNum]))
					{
						mvIndex = k;
						break;
					}
				}
				localLockIndex[clientNum] = mvIndex;
        
				if (serverCount > 1)
        {
          // If the mv has not been created yet.
					if (localLockIndex[clientNum] == -1) 
					{
            // Store the current timeStamp so we can use it later to determine ownership.
            timeval currentTimeStamp;
            gettimeofday(&currentTimeStamp, NULL);
            mvCreationTimeStamps[clientNum] = currentTimeStamp;
            
            // Set the state to MAYBE, meaning we have a request pending.
            mvCreationStates[clientNum] = MAYBE;
            
            // Broadcast a request to all the other threads.
            CreateVerify(lockName[clientNum], CREATEMVVERIFY, clientNum, localNetThreadInfo);
          }
					else  // If we have already created the mv.
					{
						mvIndex = mvs[localLockIndex[clientNum]]->GetGlobalId();
          
            char ack[MaxMailSize];
            sprintf(ack, "%i", mvIndex);
            
            // Communicate to my paired user thread.
            outPktHdr.to = postOffice->GetID();
            outMailHdr.to = currentThread->mailID + 1; // +1 to reach user thread
            outPktHdr.from = postOffice->GetID();
            outMailHdr.from = currentThread->mailID;
            outMailHdr.length = strlen(ack) + 1;
            
            printf("Sending CREATEMV response: %s\n", ack);
            success = postOffice->Send(outPktHdr, outMailHdr, ack); 

            if (!success)
              interrupt->Halt();

            // Reset local vars for later.
            verifyResponses[clientNum] = 0;
            localLockIndex[clientNum] = -1;
            fflush(stdout);
          }
        }
				else
				{
          // If the mv has not been created yet.
					if (localLockIndex[clientNum] == -1) 
					{
            // Copy the mvName for later.
						for (i = 0; lockName[clientNum][i] != '\0'; i++)
						{
							localLockName[i] = lockName[clientNum][i];
							localLockName[i + 1] = '\0';
						}
            
            // Create the mv.
						mvs[createDMVIndex] = new DistributedMV(localLockName); 
						mvs[createDMVIndex]->SetGlobalId((uniqueGlobalID() * MAX_DMV) + createDMVIndex);
            
            // If CreateMV failed.
						if (mvs[createDMVIndex] == NULL)
						{
							errorOutPktHdr.to = clientNum / 100;
							errorInPktHdr = inPktHdr;
							errorOutMailHdr.to = (clientNum + 100) % 100;
							errorInMailHdr = inMailHdr;
							return false;
						}
						
            // Store mvIndex and increment createDMVIndex for later.
						mvIndex = mvs[createDMVIndex]->GetGlobalId();
						createDMVIndex++;
					}
					else  // If we have already created the mv.
					{
						mvIndex = mvs[localLockIndex[clientNum]]->GetGlobalId();
					}
          
					char ack[MaxMailSize];
					sprintf(ack, "%i", mvIndex);
          
          // Communicate to my paired user thread.
          outPktHdr.to = postOffice->GetID();
          outMailHdr.to = currentThread->mailID + 1; // +1 to reach user thread
          outPktHdr.from = postOffice->GetID();
          outMailHdr.from = currentThread->mailID;
          outMailHdr.length = strlen(ack) + 1;
					
          printf("Sending CREATEMV response: %s\n", ack);
					success = postOffice->Send(outPktHdr, outMailHdr, ack); 

					if (!success)
					  interrupt->Halt();

          // Reset local vars for later.
					verifyResponses[clientNum] = 0;
					localLockIndex[clientNum] = -1;
					fflush(stdout);
				}
			}
			else
			{
				errorOutPktHdr = outPktHdr;
				errorInPktHdr = inPktHdr;
				errorOutMailHdr = outMailHdr;
				errorInMailHdr = inMailHdr;
				return false;
			}
			break;

		case CREATEMVANSWER:
      printf("CREATEMVANSWER\n");
			x=0;
      
      // Parse clientNum.
			do 
			{
				c = msgData[++i];
				if (c == '_')
					break;
				client[x++] = c;	
			} while (c != '_');
			client[x]='\0';
			clientNum = atoi(client);
			
      // Parse verifyMessage.
			while (c !='\0') 
			{
				c = msgData[++i];
				verifyBuffer[clientNum][verifyResponses[clientNum]][a++] = c;
				if (c == '\0')
					break;
			}
			verifyResponses[clientNum]++;
      
      // If we have received verifyResponses from every other server.
			if (verifyResponses[clientNum] == serverCount - 1)
			{
        // Check if any other servers have already created the mv.
				for (i = 0; i < serverCount - 1; i++)
				{
					if (verifyBuffer[clientNum][i][0] == '-' && verifyBuffer[clientNum][i][1] == '1')
					{
						otherServerLockIndex = -1;
					}
					else
					{	
						otherServerLockIndex = atoi(verifyBuffer[clientNum][i]);
						break;
					}
				}
        
        // If the mv has not been created here or anywhere else.
				if (localLockIndex[clientNum] == -1 && otherServerLockIndex == -1) 
				{
          // Store the mvName.
					for (i = 0; lockName[clientNum][i] != '\0'; i++)
          {
						localLockName[i] = lockName[clientNum][i];
						localLockName[i + 1] = '\0';
					}
          
          // Actually create the mv.
					mvs[createDMVIndex] = new DistributedMV(localLockName); 
					mvs[createDMVIndex]->SetGlobalId((uniqueGlobalID()*MAX_DMV)+createDMVIndex);
          
          // If mv creation failed.
					if (mvs[createDMVIndex] == NULL)
					{
						errorOutPktHdr.to = clientNum / 100;
						errorInPktHdr = inPktHdr;
						errorOutMailHdr.to = (clientNum + 100) % 100;
						errorInMailHdr = inMailHdr;
						return false;
					}
					
          // Set the mvIndex and increment createDMVIndex for next time.
					mvIndex = mvs[createDMVIndex]->GetGlobalId();
					createDMVIndex++;
				}
        // If the mv has already been created on another server, but not this one.
				else if (localLockIndex[clientNum] == -1 && otherServerLockIndex != -1) 
				{
					mvIndex = otherServerLockIndex;
				}
        // If the mv has been created on this server and nowhere else.
				else if (localLockIndex[clientNum] != -1 && otherServerLockIndex == -1)
				{
					mvIndex=mvs[localLockIndex[clientNum]]->GetGlobalId();
				}
				else
				{}
        
				char ack[MaxMailSize];
				sprintf(ack, "%i", mvIndex);
        
        // Communicate to my paired user thread.
				outPktHdr.to = postOffice->GetID();
				outMailHdr.to = currentThread->mailID + 1; // +1 to reach user thread
				outPktHdr.from = postOffice->GetID();
        outMailHdr.from = currentThread->mailID;
				outMailHdr.length = strlen(ack) + 1;
				
				success = postOffice->Send(outPktHdr, outMailHdr, ack); 

				if (!success) 
				  interrupt->Halt();

        // Reset local vars for next time.
				verifyResponses[clientNum] = 0;
				localLockIndex[clientNum] = -1;
				fflush(stdout);
			}
			break;
      
		case CREATEMVVERIFY:
      printf("CREATEMVVERIFY\n");
			mvIndex = -1;
			request = new char [MaxMailSize];
			x = 0;
      
      // Parse clientNum.
			do
			{
				c = msgData[++i];
				if (c == '_')
					break;
				client[x++] = c;	
			} while (c != '_');
			client[x]='\0';
			clientNum = atoi(client);
      
      // Parse mvName.
			while (c != '\0') 
			{
				c = msgData[++i];
				if (c == '\0')
					break;
				localLockName[a++] = c; 
			}
			localLockName[a] = '\0';
      
      // Check if the mv has been created on this server.
			for (j = 0; j < createDMVIndex; j++)
			{
				if (!strcmp(mvs[j]->getName(), localLockName))
				{
					mvIndex = j;
					break;
				}
			}
      
			outPktHdr.to = inPktHdr.from;
			outMailHdr.to = inMailHdr.from;
			outMailHdr.from = postOffice->GetID();
      
      // If the mv has not been created on this server.
			if (mvIndex == -1)
			{
				sprintf(request, "%d_%d_%d", CREATEMVANSWER, clientNum, -1);
				outMailHdr.length = strlen(request) + 1;
				success = postOffice->Send(outPktHdr, outMailHdr, request);
			}
			else
			{
				sprintf(request, "%d_%d_%d", CREATEMVANSWER, clientNum, mvs[mvIndex]->GetGlobalId());
				outMailHdr.length = strlen(request) + 1;
				success = postOffice->Send(outPktHdr, outMailHdr, request);
			}
			break;

		case ACQUIRE:
			printf("ACQUIRE\n");
			clientNum=100*(inPktHdr.from)+inMailHdr.from - 1;
			m=0; 
			if(msgData[i+1] == '_')
			{
				i++;
				do 
				{
					c = msgData[++i];
					if(c=='_')
						break;
					client[m++] = c;	
				}while(c!='_');
        
				client[m]='\0';
				clientNum = atoi(client);
			}
			m=0;
			while(c!='\0')
			{
				c = msgData[++i];
				lockIndexBuf[m++] = c;
			}
			lockIndex = atoi(lockIndexBuf); 
			//printf("lockIndex/MAX_DLOCK != uniqueGlobalID(): %d != %d\n", lockIndex/MAX_DLOCK, uniqueGlobalID());
			if(lockIndex/MAX_DLOCK != uniqueGlobalID())
			{
				sprintf(buffer,"%d__%d_%s",ACQUIRE, clientNum, lockIndexBuf);
				outPktHdr.to = lockIndex/MAX_DLOCK;
				outMailHdr.to = inMailHdr.to + 1; // +1 to reach user thread
				outMailHdr.from = inMailHdr.from;
				outMailHdr.length = strlen(buffer) + 1;
        
				success = postOffice->Send(outPktHdr, outMailHdr, buffer);
			}
			else
				success = Acquire(lockIndex, outPktHdr, inPktHdr, outMailHdr, inMailHdr, clientNum);
			break;
      
		case RELEASE:
      printf("RELEASE\n");
			clientNum=100*(inPktHdr.from)+inMailHdr.from - 1;
			m=0; 
			if(msgData[i+1] == '_')
			{
				i++;
				do 
				{
					c = msgData[++i];
					if(c=='_')
						break;
					client[m++] = c;	
				}while(c!='_');

				client[m]='\0';
				clientNum = atoi(client);
			}
			m=0;
			while(c!='\0')
			{
				c = msgData[++i];
				lockIndexBuf[m++] = c; 
			}
		
			lockIndex = atoi(lockIndexBuf); 
			if(lockIndex/MAX_DLOCK != uniqueGlobalID())
			{
				sprintf(buffer,"%d__%d_%s",RELEASE, clientNum, lockIndexBuf);
				outPktHdr.to = lockIndex/MAX_DLOCK;
				outMailHdr.to = outMailHdr.to + 1; // +1 to reach user thread;
				outMailHdr.from = inMailHdr.from;
				outMailHdr.length = strlen(buffer) + 1;
        
        printf("Sending RELEASE response: %s\n", buffer);
				success = postOffice->Send(outPktHdr, outMailHdr, buffer);
			}
			else
				success = Release(lockIndex, outPktHdr, inPktHdr, outMailHdr, inMailHdr, clientNum);
			break;
		case DESTROYLOCK:
      printf("DESTROYLOCK\n");
			clientNum=100*(inPktHdr.from)+inMailHdr.from - 1;
			m=0; 
			if(msgData[i+1] == '_')
			{
				i++;
				do 
				{
					c = msgData[++i];
					if(c=='_')
						break;
					client[m++] = c;	
				}while(c!='_');

				client[m]='\0';
				clientNum = atoi(client);
			}
			m=0;
			while(c!='\0')
			{
				c = msgData[++i];
				lockIndexBuf[m++] = c; 
			}
			lockIndex = atoi(lockIndexBuf);
			if(lockIndex/MAX_DLOCK != uniqueGlobalID())
			{
				sprintf(buffer,"%d__%d_%s",DESTROYLOCK, clientNum, lockIndexBuf);
				outPktHdr.to = lockIndex/MAX_DLOCK;
				outMailHdr.to = outPktHdr.to;
				outMailHdr.from = inMailHdr.from;
				outMailHdr.length = strlen(buffer) + 1;
				success = postOffice->Send(outPktHdr, outMailHdr, buffer);
			}
			else
				success = DestroyLock(lockIndex, outPktHdr, inPktHdr, outMailHdr, inMailHdr, clientNum);
			break;
		case SETMV:
			printf("SETMV\n");
			clientNum=100*(inPktHdr.from)+inMailHdr.from - 1;
			m=0; 
			if(msgData[i+1] == '_')
			{
				i++;
				do
				{
					c = msgData[++i];
					if(c=='_')
						break;
					client[m++] = c;
				} while(c!='_');
				clientNum=atoi(client);
			}
			m=0; 
			do
			{
				c = msgData[++i];
				if(c=='_')
					break;
				mvIndexBuf[m++] = c; 
			}while(c!='_');
			mvIndexBuf[m] = '\0';
			mvIndex = atoi(mvIndexBuf); 
			m=0;
			i++;
			while(c!='\0') 
			{
				c = msgData[i++]; 
				mvValBuf[m++] = c; 
			}
			mvValue = atoi(mvValBuf); 
			if(mvIndex/MAX_DMV != uniqueGlobalID())
			{
				sprintf(buffer,"%d__%d_%s_%s",SETMV, clientNum, mvIndexBuf, mvValBuf);
				outPktHdr.to = mvIndex/MAX_DMV;
				outMailHdr.to = outMailHdr.to + 1; // +1 to reach user thread;
				outMailHdr.from = inMailHdr.from;
				outMailHdr.length = strlen(buffer) + 1;
                
        printf("Sending SETMV response: %s\n", buffer);
				success = postOffice->Send(outPktHdr, outMailHdr, buffer);
			}
			else
				success = SetMV(mvIndex, mvValue, outPktHdr, inPktHdr, outMailHdr, inMailHdr, clientNum);
			break;
		case GETMV:
      printf("GETMV\n");
			clientNum=100*(inPktHdr.from)+inMailHdr.from - 1;
			m=0; 
			if(msgData[i+1] == '_')
			{
				i++;
				do 
				{
					c = msgData[++i];
					if(c=='_')
						break;
					client[m++] = c;	
				}while(c!='_');

				client[m]='\0';
				clientNum = atoi(client);
			}
			m=0;
			while(c!='\0')
			{
				c = msgData[++i];
				mvIndexBuf[m++] = c;
			}

			mvIndex = atoi(mvIndexBuf); 
			if(mvIndex/MAX_DMV != uniqueGlobalID())
			{
				sprintf(buffer,"%d__%d_%s",GETMV, clientNum, mvIndexBuf);
				outPktHdr.to = mvIndex/MAX_DMV;
				outMailHdr.to = outMailHdr.to + 1; // +1 to reach user thread;
				outMailHdr.from = inMailHdr.from;
				outMailHdr.length = strlen(buffer) + 1;
        
        printf("Sending GETMV response: %s\n", buffer);
				success = postOffice->Send(outPktHdr, outMailHdr, buffer);
			}
			else
				success = GetMV(mvIndex, outPktHdr, inPktHdr, outMailHdr, inMailHdr, clientNum);
			
			break;
		case DESTROYMV:
      printf("DESTROYMV\n");
			clientNum=100*(inPktHdr.from)+inMailHdr.from - 1;
			m=0; 
			if(msgData[i+1] == '_')
			{
				i++;
				do 
				{
					c = msgData[++i];
					if(c=='_')
						break;
					client[m++] = c;	
				}while(c!='_');

				client[m]='\0';
				clientNum = atoi(client);
			}
			m=0;
			while(c!='\0') 
			{
				c = msgData[++i];
				mvIndexBuf[m++] = c;
			}

			mvIndex = atoi(mvIndexBuf);
			if(mvIndex/MAX_DMV != uniqueGlobalID())
			{
				sprintf(buffer,"%d__%d_%s",DESTROYMV, clientNum, mvIndexBuf);
				outPktHdr.to = mvIndex/MAX_DMV;
				outMailHdr.to = outMailHdr.to + 1; // +1 to reach user thread;
				outMailHdr.from = inMailHdr.from;
				outMailHdr.length = strlen(buffer) + 1;
				success = postOffice->Send(outPktHdr, outMailHdr, buffer);
        
        printf("Sending DESTROYMV response: %s\n", buffer);
			}
			else
				success = DestroyMV(mvIndex, outPktHdr, inPktHdr, outMailHdr, inMailHdr, clientNum);
			break;	
		case WAIT:
      printf("WAIT\n");
			clientNum=100*(inPktHdr.from)+inMailHdr.from - 1;
			m=0; 
			if(msgData[i+1] == '_')
			{
				i++;
				do 
				{
					c = msgData[++i];
					if(c=='_')
						break;
					client[m++] = c;	
				}while(c!='_');

				client[m]='\0';
				clientNum = atoi(client);
			}
			cvNumIndex=0;
			do {
				c = msgData[++i];
				if(c=='_')
						break;
				cvNum[cvNumIndex ++] = c;
			}while ( c!= '_');
			cvNum[cvNumIndex]='\0';
			cvIndex = atoi(cvNum);

			while ( c!= '\0'){
				c = msgData[++i];
				lockNum[lockNumIndex ++] = c;
			}

			lockIndex = atoi(lockNum); 
			lockNumIndex = 0;
			cvNumIndex = 0;
			if(lockIndex/MAX_DLOCK != uniqueGlobalID())
			{
				sprintf(buffer,"%d__%d_%s_%s",SIGNAL, clientNum, cvNum,lockNum);
				outPktHdr.to = lockIndex/MAX_DLOCK;
				outMailHdr.to = outPktHdr.to;
				outMailHdr.from = inMailHdr.from;
				outMailHdr.length = strlen(buffer) + 1;
				success = postOffice->Send(outPktHdr, outMailHdr, buffer);
			}
			else
				success = Wait(cvIndex, lockIndex, outPktHdr, inPktHdr, outMailHdr, inMailHdr, clientNum);
			break;

		case WAITCV:
      printf("WAITCV\n");
			clientNum=100*(inPktHdr.from)+inMailHdr.from - 1;
			m=0; 
			if(msgData[i+1] == '_')
			{
				i++;
				do 
				{
					c = msgData[++i];
					if(c=='_')
						break;
					client[m++] = c;	
				}while(c!='_');

				client[m]='\0';
				clientNum = atoi(client);
			}
			cvNumIndex=0;
			do {
				c = msgData[++i];
				if(c=='_')
						break;
				cvNum[cvNumIndex ++] = c;
			}while ( c!= '_');
			cvNum[cvNumIndex]='\0';
			cvIndex = atoi(cvNum);

			while ( c!= '\0'){
				c = msgData[++i];
				lockNum[lockNumIndex ++] = c;
			}

			lockIndex = atoi(lockNum); 
			lockNumIndex = 0;
			cvNumIndex = 0;
			if(cvs[cvIndex%MAX_DCV] == NULL)
			{
				sprintf(buffer,"%d_%d_%d",WAITRESPONSE, (int)client, -1);
				
				outPktHdr.to = clientNum/100;
				outMailHdr.to = (clientNum+100)%100;
				outMailHdr.from = postOffice->GetID();
				outMailHdr.length = strlen(buffer) + 1;
				success = postOffice->Send(outPktHdr, outMailHdr, buffer); 
			}
			else
			{
				if (cvs[cvIndex%MAX_DCV]->isQueueEmpty())
					cvs[cvIndex%MAX_DCV]-> setFirstLock(lockIndex);
				char * ack = new char [5];
				itoa(ack,5,cvIndex);
				
				outPktHdr.to = clientNum/100;
				outMailHdr.to = (clientNum+100)%100;
				outMailHdr.from = postOffice->GetID();
				outMailHdr.length = strlen(ack) + 1;
				
				Message* reply = new Message(outPktHdr, outMailHdr, ack);
				cvs[cvIndex%MAX_DCV]->QueueReply(reply);

				sprintf(buffer,"%d_%d_%d",WAITRESPONSE, (int)client, cvIndex);
				outPktHdr.to = clientNum/100;
				outMailHdr.to = (clientNum+100)%100;
				outMailHdr.from = postOffice->GetID();
				outMailHdr.length = strlen(buffer) + 1;
				success = postOffice->Send(outPktHdr, outMailHdr, buffer);
			}

			break;
		case WAITRESPONSE:
      printf("WAITRESPONSE\n");
			clientNum=-1;
			m=0; 
			if(msgData[i+1] == '_')
			{
				i++;
				do 
				{
					c = msgData[++i];
					if(c=='_')
						break;
					client[m++] = c;	
				}while(c!='_');

				client[m]='\0';
				clientNum = atoi(client);
			}
      else
        ASSERT(false);
        
			m=0;
			while(c!='\0')
			{
				c = msgData[++i];
				cvNum[m++] = c; 
			}
			cvIndex = atoi(cvNum); 
			if (cvIndex == -1) 
			{
				errorOutPktHdr = outPktHdr;
				errorOutPktHdr.to = clientNum/100;
				errorInPktHdr = inPktHdr;
				errorOutMailHdr = outMailHdr;
				errorOutMailHdr.to = (clientNum+100)%100;
				errorInMailHdr = inMailHdr;
				success = false;
			}
			else
			{
				lockIndex = waitingForWaitLocks[clientNum];
				dlocks[lockIndex]->setOwner(-1, -1); 
				dlocks[lockIndex]->setLockState(true); 
				if(!dlocks[lockIndex]->isQueueEmpty()) 
				{
					Message* reply = dlocks[lockIndex]->RemoveReply(); 
					dlocks[lockIndex]->setOwner(reply->pktHdr.to, reply->mailHdr.to); 
					dlocks[lockIndex]->setLockState(false); 
					success = postOffice->Send(reply->pktHdr, reply->mailHdr, reply->data); 

					if ( !success ) 
					{
					  interrupt->Halt();
					}
				}
			}
			break;
			
		case SIGNAL:
      printf("SIGNAL\n");
			clientNum=-1;
			m=0; 
			if(msgData[i+1] == '_')
			{
				i++;
				do 
				{
					c = msgData[++i];
					if(c=='_')
						break;
					client[m++] = c;	
				}while(c!='_');

				client[m]='\0';
				clientNum = atoi(client);
			}
      else
        ASSERT(false);
        
			cvNumIndex=0;
			do {
				c = msgData[++i];
				if(c=='_')
					break;
				cvNum[cvNumIndex ++] = c;
			}while ( c!= '_');
			cvNum[cvNumIndex]='\0';
			cvIndex = atoi(cvNum);

			while ( c!= '\0'){
				c = msgData[++i];
				lockNum[lockNumIndex ++] = c;
			}

			lockIndex = atoi(lockNum); 
			lockNumIndex = 0;
			cvNumIndex = 0;
			
			if(cvIndex/MAX_DCV != uniqueGlobalID())
			{
				sprintf(buffer,"%d__%d_%s_%s",SIGNAL, clientNum, cvNum,lockNum);
				outPktHdr.to = cvIndex/MAX_DCV;
				outMailHdr.to = outMailHdr.to + 1; // +1 to reach user thread;
				outMailHdr.from = inMailHdr.from;
				outMailHdr.length = strlen(buffer) + 1;
				success = postOffice->Send(outPktHdr, outMailHdr, buffer);
			}
			else
				success = Signal(cvIndex, lockIndex, outPktHdr, inPktHdr, outMailHdr, inMailHdr,clientNum);
			
			break;
			
		case SIGNALLOCK:
      printf("SIGNALLOCK\n");
			clientNum=100*(inPktHdr.from)+inMailHdr.from - 1;
			m=0; 
			if(msgData[i+1] == '_')
			{
				i++;
				do 
				{
					c = msgData[++i];
					if(c=='_')
						break;
					client[m++] = c;	
				}while(c!='_');

				client[m]='\0';
				clientNum = atoi(client);
			}
			m=0;
			while(c!='\0') 
			{
				c = msgData[++i];
				lockIndexBuf[m++] = c; 
			}
			lockIndex = atoi(lockIndexBuf); 
			if(dlocks[lockIndex%MAX_DLOCK] == NULL)
			{
				sprintf(buffer,"%d_%d_%d",SIGNALRESPONSE, (int)client, -1);
				
				outPktHdr.to = clientNum/100;
				outMailHdr.to = (clientNum+100)%100;
				outMailHdr.from = postOffice->GetID();
				outMailHdr.length = strlen(buffer) + 1;

				success = postOffice->Send(outPktHdr, outMailHdr, buffer); 
			}
			else //found
			{
				sprintf(buffer,"%d_%d_%d",SIGNALRESPONSE, (int)client, lockIndex);
				outPktHdr.to = clientNum/100;
				outMailHdr.to = (clientNum+100)%100;
				outMailHdr.from = postOffice->GetID();
				outMailHdr.length = strlen(buffer) + 1;

				success = postOffice->Send(outPktHdr, outMailHdr, buffer); 
			}
			break;
		case SIGNALRESPONSE:
      printf("SIGNALRESPONSE\n");
			clientNum=100*(inPktHdr.from)+inMailHdr.from - 1;
			m=0; 
			if(msgData[i+1] == '_')
			{
				i++;
				do 
				{
					c = msgData[++i];
					if(c=='_')
						break;
					client[m++] = c;	
				}while(c!='_');

				client[m]='\0';
				clientNum = atoi(client);
			}
			m=0;
			while(c!='\0') 
			{
				c = msgData[++i];
				lockIndexBuf[m++] = c; 
			}
			lockIndex = atoi(lockIndexBuf); 
			if (lockIndex == -1)
			{
				errorOutPktHdr = outPktHdr;
				errorOutPktHdr.to = clientNum/100;
				errorInPktHdr = inPktHdr;
				errorOutMailHdr = outMailHdr;
				errorOutMailHdr.to = (clientNum+100)%100;
				errorInMailHdr = inMailHdr;
				success = false;
			}
			else
			{
				cvIndex=waitingForSignalCvs[clientNum];
				char* ack = new char [5];
				itoa(ack,5,cvIndex);

				outPktHdr.to = clientNum/100;
				outMailHdr.to = (clientNum+100)%100;
				outMailHdr.from = postOffice->GetID();
				outMailHdr.length = strlen(ack) + 1;

				success = postOffice->Send(outPktHdr, outMailHdr, ack); 
				
				if ( !success ) 
				{
				  interrupt->Halt();
				}
				
				if (!(cvs[cvIndex]->isQueueEmpty())){
					if (cvs[cvIndex]->getFirstLock() != lockIndex)
					{
						errorOutPktHdr = outPktHdr;
						errorOutPktHdr.to = clientNum/100;
						errorInPktHdr = inPktHdr;
						errorOutMailHdr = outMailHdr;
						errorOutMailHdr.to = (clientNum+100)%100;
						errorInMailHdr = inMailHdr;
						return false;
					}
					else
					{
						Message* reply = cvs[cvIndex]->RemoveReply(); 
						reply->pktHdr.from = reply->pktHdr.to;
						reply->mailHdr.from = reply->mailHdr.to;
						bool success2 = Acquire(lockIndex, outPktHdr, reply->pktHdr,outMailHdr, reply->mailHdr, -2);
						
						if ( !success2 ) 
						{
						  interrupt->Halt();
						}
					}
				}
				if (cvs[cvIndex]->isQueueEmpty()){
					cvs[cvIndex]->setFirstLock(-1);
				}	
			}
			break;
		case BROADCAST:
      printf("BROADCAST\n");
			clientNum=100*(inPktHdr.from)+inMailHdr.from - 1;
			m=0; 
			if(msgData[i+1] == '_')
			{
				i++;
				do 
				{
					c = msgData[++i];
					if(c=='_')
						break;
					client[m++] = c;	
				}while(c!='_');

				client[m]='\0';
				clientNum = atoi(client);
			}
			cvNumIndex=0;
			do {
				c = msgData[++i];
				if(c=='_')
						break;
				cvNum[cvNumIndex ++] = c;
			}while ( c!= '_');
			cvNum[cvNumIndex]='\0';
			cvIndex = atoi(cvNum); 

			while ( c!= '\0'){
				c = msgData[++i];
				lockNum[lockNumIndex ++] = c;
			}

			lockIndex = atoi(lockNum); 
			lockNumIndex = 0;
			cvNumIndex = 0;
			
			if(cvIndex/MAX_DCV != uniqueGlobalID())
			{
				sprintf(buffer,"%d__%d_%s_%s",BROADCAST, clientNum, cvNum,lockNum);
				outPktHdr.to = cvIndex/MAX_DCV;
				outMailHdr.to = outPktHdr.to;
				outMailHdr.from = inMailHdr.from;
				outMailHdr.length = strlen(buffer) + 1;
				success = postOffice->Send(outPktHdr, outMailHdr, buffer);
			}
			else
				success = Broadcast(cvIndex, lockIndex, outPktHdr, inPktHdr, outMailHdr, inMailHdr, clientNum);
			
			break;

		case BROADCASTLOCK:
      printf("BROADCASTLOCK\n");
			clientNum=100*(inPktHdr.from)+inMailHdr.from - 1;
			m=0; 
			if(msgData[i+1] == '_')
			{
				i++;
				do 
				{
					c = msgData[++i];
					if(c=='_')
						break;
					client[m++] = c;	
				}while(c!='_');

				client[m]='\0';
				clientNum = atoi(client);
			}
			m=0;
			while(c!='\0') 
			{
				c = msgData[++i];
				lockIndexBuf[m++] = c;
			}
			lockIndex = atoi(lockIndexBuf);
			if(dlocks[lockIndex%MAX_DLOCK] == NULL)
			{
				sprintf(buffer,"%d_%d_%d",BROADCASTRESPONSE, (int)client, -1);
	
				outPktHdr.to = clientNum/100;
				outMailHdr.to = (clientNum+100)%100;
				outMailHdr.from = postOffice->GetID();
				outMailHdr.length = strlen(buffer) + 1;

				success = postOffice->Send(outPktHdr, outMailHdr, buffer); 
			}
			else if (dlocks[lockIndex]->getOwnerMailID() == -1)
			{
				sprintf(buffer,"%d_%d_%d",BROADCASTRESPONSE, (int)client, -1);
	
				outPktHdr.to = clientNum/100;
				outMailHdr.to = (clientNum+100)%100;
				outMailHdr.from = postOffice->GetID();
				outMailHdr.length = strlen(buffer) + 1;

				success = postOffice->Send(outPktHdr, outMailHdr, buffer);
			}
			else
			{
				sprintf(buffer,"%d_%d_%d",BROADCASTRESPONSE, (int)client, lockIndex);
				outPktHdr.to = clientNum/100;
				outMailHdr.to = (clientNum+100)%100;
				outMailHdr.from = postOffice->GetID();
				outMailHdr.length = strlen(buffer) + 1;

				success = postOffice->Send(outPktHdr, outMailHdr, buffer); 
			}
			break;
		case BROADCASTRESPONSE:
      printf("BROADCASTRESPONSE\n");
			clientNum=100*(inPktHdr.from)+inMailHdr.from - 1;
			m=0; 
			if(msgData[i+1] == '_')
			{
				i++;
				do 
				{
					c = msgData[++i];
					if(c=='_')
						break;
					client[m++] = c;	
				}while(c!='_');

				client[m]='\0';
				clientNum = atoi(client);
			}
			m=0;
			while(c!='\0') 
			{
				c = msgData[++i];
				lockIndexBuf[m++] = c;
			}
			lockIndex = atoi(lockIndexBuf);
			if (lockIndex == -1)
			{
				errorOutPktHdr = outPktHdr;
				errorOutPktHdr.to = clientNum/100;
				errorInPktHdr = inPktHdr;
				errorOutMailHdr = outMailHdr;
				errorOutMailHdr.to = (clientNum+100)%100;
				errorInMailHdr = inMailHdr;
				success = false;
			}
			else
			{
				cvIndex=waitingForBroadcastCvs[clientNum];
				lockIndex=lockIndex%MAX_DLOCK;
				char* ack = new char [5];
				itoa(ack,5,cvIndex);

				outPktHdr.to = clientNum/100;
				outMailHdr.to = (clientNum+100)%100;
				outMailHdr.from = postOffice->GetID();
				outMailHdr.length = strlen(ack) + 1;

				success = postOffice->Send(outPktHdr, outMailHdr, ack); 
				
				if ( !success ) 
				{
				  interrupt->Halt();
				}
				fflush(stdout);
				
				if (!(cvs[cvIndex]->isQueueEmpty())){
					if (cvs[cvIndex]->getFirstLock() != lockIndex)
					{
						errorOutPktHdr = outPktHdr;
						errorOutPktHdr.to = clientNum/100;
						errorInPktHdr = inPktHdr;
						errorOutMailHdr = outMailHdr;
						errorOutMailHdr.to = (clientNum+100)%100;
						errorInMailHdr = inMailHdr;
						return false;
					}
					else
					{
						Message* reply = cvs[cvIndex]->RemoveReply();
						reply->pktHdr.from = reply->pktHdr.to;
						reply->mailHdr.from = reply->mailHdr.to;
						bool success2 = Acquire(lockIndex, outPktHdr, reply->pktHdr,outMailHdr, reply->mailHdr, -2);
						
						if ( !success2 ) {
						  interrupt->Halt();
						}
						fflush(stdout);
					}
				}
				while (!(cvs[cvIndex]->isQueueEmpty()))
				{
					Message* reply = cvs[cvIndex]->RemoveReply();
					reply->pktHdr.from = reply->pktHdr.to;
					reply->mailHdr.from = reply->mailHdr.to;
					bool success2 = Acquire(lockIndex, outPktHdr, reply->pktHdr,outMailHdr, reply->mailHdr, -2);
					
					if ( !success2 ) {
					  interrupt->Halt();
					}
				}

				if (cvs[cvIndex]->isQueueEmpty()){
					cvs[cvIndex]->setFirstLock(-1);
				}
			}
	
			break;

		case DESTROYCV:	
      printf("DESTROYCV\n");
			clientNum=100*(inPktHdr.from)+inMailHdr.from - 1;
			m=0; 
			if(msgData[i+1] == '_')
			{
				i++;
				do 
				{
					c = msgData[++i];
					if(c=='_')
						break;
					client[m++] = c;	
				}while(c!='_');

				client[m]='\0';
				clientNum = atoi(client);
			}
			cvNumIndex=0;
			while ( c!= '\0'){
				c = msgData[++i];
				cvNum[cvNumIndex ++] = c;
			}
			cvIndex = atoi(cvNum); 
			if(cvIndex/MAX_DCV != uniqueGlobalID())
			{
				sprintf(buffer,"%d__%d_%s",DESTROYCV, clientNum, cvNum);
				outPktHdr.to = cvIndex/MAX_DCV;
				outMailHdr.to = outPktHdr.to;
				outMailHdr.from = inMailHdr.from;
				outMailHdr.length = strlen(buffer) + 1;
				success = postOffice->Send(outPktHdr, outMailHdr, buffer);
			}
			else
			{
				if ((cvIndex < 0)||(cvIndex > MAX_NUM_GLOBAL_IDS*MAX_DCV-1))
				{
					errorOutPktHdr = outPktHdr;
					errorOutPktHdr.to = clientNum/100;
					errorInPktHdr = inPktHdr;
					errorOutMailHdr = outMailHdr;
					errorOutMailHdr.to = (clientNum+100)%100;
					errorInMailHdr = inMailHdr;
					return false;
				}
				else if(cvIndex/MAX_DCV== uniqueGlobalID())
				{
					cvIndex=cvIndex%MAX_DCV;
					if (cvs[cvIndex] != NULL){

						delete cvs[cvIndex];
						cvs[cvIndex] = NULL;
						createDCVIndex --;
						char* ack = new char [4];
						itoa(ack,4,cvIndex); 
			
						outPktHdr.to = clientNum/100;
						outMailHdr.to = (clientNum+100)%100;
						outMailHdr.from = postOffice->GetID();
						outMailHdr.length = strlen(ack) + 1;

						success = postOffice->Send(outPktHdr, outMailHdr, ack); 

						if ( !success ) {
						  interrupt->Halt();
						}
					}
					else
					{
						errorOutPktHdr = outPktHdr;
						errorOutPktHdr.to = clientNum/100;
						errorInPktHdr = inPktHdr;
						errorOutMailHdr = outMailHdr;
						errorOutMailHdr.to = (clientNum+100)%100;
						errorInMailHdr = inMailHdr;
						return false;
					}
				}
				else
				{
					outPktHdr.to = cvIndex/MAX_DCV;
					outMailHdr.to = outPktHdr.to;
					outMailHdr.from = inMailHdr.from;
					outMailHdr.length = strlen(msgData) + 1;
					success = postOffice->Send(outPktHdr, outMailHdr, msgData);
				}
			}
			break;	
	
  case STARTUSERPROGRAM:
    printf("STARTUSERPROGRAM... should not get here\n");
    ASSERT(false);
    break;
  case REGNETTHREAD:
    printf("REGNETTHREAD... should not get here\n");
    ASSERT(false);
    break;
  case REGNETTHREADRESPONSE:
    printf("REGNETTHREADRESPONSE... should not get here\n");
    ASSERT(false);
    break;
  case GROUPINFO:
    printf("GROUPINFO... should not get here\n");
    ASSERT(false);
    break;
  case ACK:
    printf("ACK... should not get here\n");
    ASSERT(false);
    break;
  case TIMESTAMP:
    // Do nothing. The timeStamp updating will get handled in the total ordering code.
    break;
  case INVALIDTYPE:
    printf("INVALIDTYPE... should not get here\n");
    ASSERT(false);
    break;
    
  }

	if (!success)
	{ 
		errorOutPktHdr = outPktHdr;
		errorInPktHdr = inPktHdr;
		errorOutMailHdr = outMailHdr;
		errorInMailHdr = inMailHdr;
	}
	return success;
}

bool Acquire(int lockIndex, PacketHeader outPktHdr, PacketHeader inPktHdr, MailHeader outMailHdr, MailHeader inMailHdr, int clientNum)
{
	bool success = false;
	if (clientNum==-2)
		clientNum=100*(inPktHdr.from)+inMailHdr.from - 1;
	
	if((lockIndex < 0)||(lockIndex > MAX_NUM_GLOBAL_IDS*MAX_DLOCK-1))
	{
		printf("Cannot acquire: Lock index [%d] out of bounds [%d, %d].\n", lockIndex, 0, MAX_NUM_GLOBAL_IDS*(MAX_DLOCK-1));
		errorOutPktHdr = outPktHdr;
		errorOutPktHdr.to = clientNum/100;
		errorInPktHdr = inPktHdr;
		errorOutMailHdr = outMailHdr;
		errorOutMailHdr.to = (clientNum+100)%100;
		errorInMailHdr = inMailHdr;
		return false;
	}
	else if(lockIndex/MAX_DLOCK == uniqueGlobalID())
	{
		lockIndex=lockIndex%MAX_DLOCK;
		if(dlocks[lockIndex] == NULL)
		{
			printf("Cannot acquire: Lock does not exist at index %d.\n", lockIndex);
			return false;
		}
		else
		{
			char * ack = new char [5];
			itoa(ack,5,dlocks[lockIndex]->GetGlobalId()); 
			
			if(!dlocks[lockIndex]->getLockState()) 
			{
				printf("Machine %d being added to waitQueue for acquiring %s!\n", clientNum/100, dlocks[lockIndex]->getName());
				
				outPktHdr.to = clientNum/100;
				outMailHdr.to = (clientNum+100)%100;
				outMailHdr.from = postOffice->GetID();
				outMailHdr.length = strlen(ack) + 1;
				
				Message* reply = new Message(outPktHdr, outMailHdr, ack);
				dlocks[lockIndex]->QueueReply(reply);
				success=true;
			}
			else
			{
				dlocks[lockIndex]->setOwner(clientNum/100, (clientNum+100)%100); 
				dlocks[lockIndex]->setLockState(false); 
				
				outPktHdr.to = clientNum/100;
				outMailHdr.to = (clientNum+100)%100 + 1; // +1 to reach user thread
        outPktHdr.from = postOffice->GetID();
				outMailHdr.from = currentThread->mailID;
				outMailHdr.length = strlen(ack) + 1;
             
				printf("Sending Acquire response: %s\n", ack);
				success = postOffice->Send(outPktHdr, outMailHdr, ack); 

				if ( !success ) 
				{
				  interrupt->Halt();
				}

				fflush(stdout);
			}
			return success;
		}
	}
	else
	{
		sprintf(buffer,"%d__%d_%d",ACQUIRE, clientNum, lockIndex);	
		outPktHdr.to = lockIndex/MAX_DLOCK;
		outMailHdr.to = outPktHdr.to;
		outMailHdr.from = inMailHdr.from;
		outMailHdr.length = strlen(buffer) + 1;
		success = postOffice->Send(outPktHdr, outMailHdr, buffer);
		return success;
	}
}

void CreateVerify(char* data, int requestType, int client, vector<NetThreadInfoEntry*> *localNetThreadInfo)
{
	char request[MaxMailSize];
	sprintf(request, "%d_%d_%s", requestType, client, data);
	
	PacketHeader outPktHdr;
	MailHeader outMailHdr;
  
  outPktHdr.from = postOffice->GetID();
	outMailHdr.from = currentThread->mailID;
	outMailHdr.length = strlen(request) + 1; 
  
  // Broadcast the message to all the other NetworkThreads (servers).
	for (int i = 0; i < serverCount; i++)
	{
    // Don't send it to ourselves.
		if (localNetThreadInfo->at(i)->machineID != outPktHdr.from ||
        localNetThreadInfo->at(i)->mailID != outMailHdr.from)	
		{
			outPktHdr.to = localNetThreadInfo->at(i)->machineID;
		 	outMailHdr.to = localNetThreadInfo->at(i)->mailID;
      
      printf("CreateVerify Send: outPktHdr.to: %d, outMailHdr.to: %d, request: %s\n", outPktHdr.to, outMailHdr.to, request);
			bool success = postOffice->Send(outPktHdr, outMailHdr, request); 

      if (!success)
        interrupt->Halt();
			
      printf("CreateVerify Send successful\n");
		}
	}
  
  // Initialize the response to 0 so we wait for all of them.
	verifyResponses[client] = 0;
}

bool Release(int lockIndex, PacketHeader outPktHdr, PacketHeader inPktHdr, MailHeader outMailHdr, MailHeader inMailHdr, int clientNum)
{
	bool success = false;
	if((lockIndex < 0)||(lockIndex > MAX_NUM_GLOBAL_IDS*(MAX_DLOCK)-1))
	{
    printf("Cannot release: Lock index [%d] out of bounds [%d, %d].\n", lockIndex, 0, MAX_NUM_GLOBAL_IDS*(MAX_DLOCK-1));
		errorOutPktHdr = outPktHdr;
		errorOutPktHdr.to = clientNum/100;
		errorInPktHdr = inPktHdr;
		errorOutMailHdr = outMailHdr;
		errorOutMailHdr.to = (clientNum+100)%100;
		errorInMailHdr = inMailHdr;
		return false;
	}
	else if(lockIndex/MAX_DLOCK == uniqueGlobalID())
	{
		lockIndex=lockIndex%MAX_DLOCK;
		if(dlocks[lockIndex] == NULL) 
		{
			errorOutPktHdr = outPktHdr;
			errorOutPktHdr.to = clientNum/100;
			errorInPktHdr = inPktHdr;
			errorOutMailHdr = outMailHdr;
			errorOutMailHdr.to = (clientNum+100)%100;
			errorInMailHdr = inMailHdr;
			return false;
		}

		char * ack = new char [5];
		itoa(ack,5,dlocks[lockIndex]->GetGlobalId());
		printf("Machine %d has released %s at index %d\n", clientNum/100, dlocks[lockIndex]->getName(), dlocks[lockIndex]->GetGlobalId());

		
		dlocks[lockIndex]->setOwner(-1, -1); 
		dlocks[lockIndex]->setLockState(true); 
    
    outPktHdr.to = clientNum/100;
    outMailHdr.to = (clientNum+100)%100 + 1; // +1 to reach user thread
    outPktHdr.from = postOffice->GetID();
    outMailHdr.from = currentThread->mailID;
    outMailHdr.length = strlen(ack) + 1;

		success = postOffice->Send(outPktHdr, outMailHdr, ack); 

		if ( !success ) {
		  interrupt->Halt();
		}

		if(!dlocks[lockIndex]->isQueueEmpty()) 
		{
			Message* reply = dlocks[lockIndex]->RemoveReply(); 
			dlocks[lockIndex]->setOwner(reply->pktHdr.to, reply->mailHdr.to); 
			dlocks[lockIndex]->setLockState(false);
			success = postOffice->Send(reply->pktHdr, reply->mailHdr, reply->data);

			if ( !success ) 
			{
			  interrupt->Halt();
			}
		}
		fflush(stdout);
		return success;
	}
	else
	{
	}
  return false;
}

bool DestroyLock(int lockIndex, PacketHeader outPktHdr, PacketHeader inPktHdr, MailHeader outMailHdr, MailHeader inMailHdr, int clientNum)
{
	bool success = false;
	if((lockIndex < 0)||(lockIndex > MAX_NUM_GLOBAL_IDS*(MAX_DLOCK)-1))
	{
    printf("Cannot destroy: Lock index [%d] out of bounds [%d, %d].\n", lockIndex, 0, MAX_NUM_GLOBAL_IDS*(MAX_DLOCK-1));
		errorOutPktHdr = outPktHdr;
		errorOutPktHdr.to = clientNum/100;
		errorInPktHdr = inPktHdr;
		errorOutMailHdr = outMailHdr;
		errorOutMailHdr.to = (clientNum+100)%100;
		errorInMailHdr = inMailHdr;
		return false;
	}
	else if(lockIndex/MAX_DLOCK == uniqueGlobalID())
	{
		lockIndex=lockIndex%MAX_DLOCK;
		if(dlocks[lockIndex] == NULL) 
		{
			errorOutPktHdr = outPktHdr;
			errorOutPktHdr.to = clientNum/100;
			errorInPktHdr = inPktHdr;
			errorOutMailHdr = outMailHdr;
			errorOutMailHdr.to = (clientNum+100)%100;
			errorInMailHdr = inMailHdr;
			return false;
		}

		char * ack = new char [5];
		itoa(ack,5,dlocks[lockIndex]->GetGlobalId()); //Convert to string to send in ack

		delete dlocks[lockIndex];
		dlocks[lockIndex] = NULL;
		createDLockIndex --;
		if(dlocks[lockIndex] == NULL)
		{
		}

		outPktHdr.to = clientNum/100;
    outMailHdr.to = (clientNum+100)%100 + 1; // +1 to reach user thread
    outPktHdr.from = postOffice->GetID();
    outMailHdr.from = currentThread->mailID;
    outMailHdr.length = strlen(ack) + 1;

		success = postOffice->Send(outPktHdr, outMailHdr, ack); 

		if ( !success ) 
		{
		  interrupt->Halt();
		}

		fflush(stdout);
		return success;
	}
	else
	{
		printf("problem; shouldn't have gotten past switch statement w/o being forwarded\n");
	}
  return false;
}

bool SetMV(int mvIndex, int value, PacketHeader outPktHdr, PacketHeader inPktHdr, MailHeader outMailHdr, MailHeader inMailHdr, int clientNum)
{
	bool success = false;
	
	if((mvIndex < 0)||(mvIndex > MAX_NUM_GLOBAL_IDS*MAX_DMV-1))
	{
    printf("Cannot set: MV index [%d] out of bounds [%d, %d].\n", mvIndex, 0, MAX_NUM_GLOBAL_IDS*(MAX_DMV-1));
		errorOutPktHdr = outPktHdr;
		errorOutPktHdr.to = clientNum/100;
		errorInPktHdr = inPktHdr;
		errorOutMailHdr = outMailHdr;
		errorOutMailHdr.to = (clientNum+100)%100;
		errorInMailHdr = inMailHdr;
		return false;
	}
	else if (mvIndex/MAX_DMV == uniqueGlobalID())
	{
		mvIndex=mvIndex%MAX_DMV;
		if(mvs[mvIndex] == NULL) //If not found
		{
			errorOutPktHdr = outPktHdr;
			errorOutPktHdr.to = clientNum/100;
			errorInPktHdr = inPktHdr;
			errorOutMailHdr = outMailHdr;
			errorOutMailHdr.to = (clientNum+100)%100;
			errorInMailHdr = inMailHdr;
			return false;
		}

		char * ack = new char [24];
    
    for(int i = 0; i < 24; i++)
      ack[i] = '\0';
      
		sprintf(ack, "SetMV[%d] = %d", mvs[mvIndex]->GetGlobalId(), value); 
      
		mvs[mvIndex]->setValue(value); 
			
		outPktHdr.to = clientNum/100;
		outMailHdr.to = (clientNum+100)%100 + 1; // +1 to reach user thread
		outMailHdr.from = postOffice->GetID();
		outMailHdr.length = strlen(ack) + 1;
    
    printf("Sending SetMV response: %s\n", ack);
		success = postOffice->Send(outPktHdr, outMailHdr, ack); 

		if ( !success ) 
		{
		  interrupt->Halt();
		}

		fflush(stdout);
		return success;
	}
	else 
	{
		printf("problem; shouldn't have gotten past switch statement w/o being forwarded\n");
	}
  return false;
}

bool GetMV(int mvIndex, PacketHeader outPktHdr, PacketHeader inPktHdr, MailHeader outMailHdr, MailHeader inMailHdr, int clientNum)
{
	bool success = false;
	char* ack = new char [4];
	
	if((mvIndex < 0)||(mvIndex > MAX_NUM_GLOBAL_IDS*MAX_DMV-1))
	{
		itoa(ack,4,999);
		return SendReply(outPktHdr, inPktHdr, outMailHdr, inMailHdr, ack);
	}
	else if (mvIndex/MAX_DMV == uniqueGlobalID())
	{
		mvIndex=mvIndex%MAX_DMV;
		if(mvs[mvIndex] == NULL)
		{
      printf("Cannot get: MV index [%d] out of bounds [%d, %d].\n", mvIndex, 0, MAX_NUM_GLOBAL_IDS*(MAX_DMV-1));
			itoa(ack,4,999); 		
			outPktHdr.to = clientNum/100;
			outMailHdr.to = (clientNum+100)%100;
			outMailHdr.from = postOffice->GetID();
			outMailHdr.length = strlen(ack) + 1;
			return SendReply(outPktHdr, inPktHdr, outMailHdr, inMailHdr, ack);
		}

		outPktHdr.to = clientNum/100;
		outMailHdr.to = (clientNum+100)%100;
		outMailHdr.from = postOffice->GetID();
		outMailHdr.length = strlen(ack) + 1;
		
		itoa(ack,4,mvs[mvIndex]->getValue()); 

		return SendReply(outPktHdr, inPktHdr, outMailHdr, inMailHdr, ack);
	}
	else
	{
		printf("problem; shouldn't have gotten past switch statement w/o being forwarded\n");
	}
  return false;
}

bool DestroyMV(int mvIndex, PacketHeader outPktHdr, PacketHeader inPktHdr, MailHeader outMailHdr, MailHeader inMailHdr, int clientNum)
{
	bool success = false;
	if((mvIndex < 0)||(mvIndex > MAX_NUM_GLOBAL_IDS*MAX_DMV-1))
	{
		printf("Cannot destroy: MV index [%d] out of bounds [%d, %d].\n", mvIndex, 0, MAX_NUM_GLOBAL_IDS*(MAX_DMV-1));
		errorOutPktHdr = outPktHdr;
		errorOutPktHdr.to = clientNum/100;
		errorInPktHdr = inPktHdr;
		errorOutMailHdr = outMailHdr;
		errorOutMailHdr.to = (clientNum+100)%100;
		errorInMailHdr = inMailHdr;
		return false;
	}
	else if (mvIndex/MAX_DMV == uniqueGlobalID())
	{
		mvIndex=mvIndex%MAX_DMV;
		if(mvs[mvIndex] == NULL)
		{
			printf("Cannot get: MV does not exist at index %d.\n", mvIndex);
			errorOutPktHdr = outPktHdr;
			errorOutPktHdr.to = clientNum/100;
			errorInPktHdr = inPktHdr;
			errorOutMailHdr = outMailHdr;
			errorOutMailHdr.to = (clientNum+100)%100;
			errorInMailHdr = inMailHdr;
			return false;
		}

		char * ack = new char [5];
		itoa(ack,5,mvs[mvIndex]->GetGlobalId());
		
		delete mvs[mvIndex];
		mvs[mvIndex] = NULL;
		createDMVIndex --;
		if(mvs[mvIndex] == NULL)
		{
		}

		outPktHdr.to = clientNum/100;
		outMailHdr.to = (clientNum+100)%100;
		outMailHdr.from = postOffice->GetID();
		outMailHdr.length = strlen(ack) + 1;
    
    printf("Sending DestroyMV response: %s\n", ack);
		success = postOffice->Send(outPktHdr, outMailHdr, ack); 

		if ( !success ) 
		{
		  interrupt->Halt();
		}

		fflush(stdout);
		return success;
	}
	else
	{
	}
  return false;
}

bool Wait(int cvIndex, int lockIndex, PacketHeader outPktHdr, PacketHeader inPktHdr, MailHeader outMailHdr, MailHeader inMailHdr, int clientNum){
	bool success;
	
	if((lockIndex < 0)||(lockIndex > MAX_NUM_GLOBAL_IDS*(MAX_DLOCK-1)))
	{
		errorOutPktHdr = outPktHdr;
		errorOutPktHdr.to = clientNum/100;
		errorInPktHdr = inPktHdr;
		errorOutMailHdr = outMailHdr;
		errorOutMailHdr.to = (clientNum+100)%100;
		errorInMailHdr = inMailHdr;
		return false;
	}
	else if ((cvIndex < 0)||(cvIndex > MAX_NUM_GLOBAL_IDS*(MAX_DCV-1)))
	{ 
		errorOutPktHdr = outPktHdr;
		errorOutPktHdr.to = clientNum/100;
		errorInPktHdr = inPktHdr;
		errorOutMailHdr = outMailHdr;
		errorOutMailHdr.to = (clientNum+100)%100;
		errorInMailHdr = inMailHdr;
		return false;
	}
	if (lockIndex/MAX_DLOCK == uniqueGlobalID() && cvIndex/MAX_DCV == uniqueGlobalID())
	{
		lockIndex=lockIndex%MAX_DLOCK;
		cvIndex=cvIndex%MAX_DCV;
		if(dlocks[lockIndex] == NULL)
		{
			printf("Wait Failed: Lock does not exist at index %d.\n", lockIndex);
			errorOutPktHdr = outPktHdr;
			errorOutPktHdr.to = clientNum/100;
			errorInPktHdr = inPktHdr;
			errorOutMailHdr = outMailHdr;
			errorOutMailHdr.to = (clientNum+100)%100;
			errorInMailHdr = inMailHdr;
			return false;
		}
		else if (dlocks[lockIndex]->getOwnerMailID() == -1)
		{
			printf("Wait Failed: Lock does not acquire at index %d.\n", lockIndex);
			errorOutPktHdr = outPktHdr;
			errorOutPktHdr.to = clientNum/100;
			errorInPktHdr = inPktHdr;
			errorOutMailHdr = outMailHdr;
			errorOutMailHdr.to = (clientNum+100)%100;
			errorInMailHdr = inMailHdr;	
			return false;
		}
		else if(cvs[cvIndex] == NULL)
		{
			printf("Wait Failed: CV does not exist at index %d.\n", cvIndex);
			errorOutPktHdr = outPktHdr;
			errorOutPktHdr.to = clientNum/100;
			errorInPktHdr = inPktHdr;
			errorOutMailHdr = outMailHdr;
			errorOutMailHdr.to = (clientNum+100)%100;
			errorInMailHdr = inMailHdr;
			return false;
		}
		
		dlocks[lockIndex]->setOwner(-1, -1);
		dlocks[lockIndex]->setLockState(true);
		
		if (cvs[cvIndex]->isQueueEmpty())
			cvs[cvIndex]-> setFirstLock(lockIndex+uniqueGlobalID()*MAX_DLOCK);
			
		
		if(!dlocks[lockIndex]->isQueueEmpty())
		{
			Message* reply = dlocks[lockIndex]->RemoveReply(); 
			dlocks[lockIndex]->setOwner(reply->pktHdr.to, reply->mailHdr.to); 
			dlocks[lockIndex]->setLockState(false); 
			success = postOffice->Send(reply->pktHdr, reply->mailHdr, reply->data); 

			if ( !success ) 
			{
			  interrupt->Halt();
			}
			fflush(stdout);
		}
		
		char * ack = new char [5];
		itoa(ack,5,cvIndex+uniqueGlobalID()*MAX_DCV);
		
		outPktHdr.to = clientNum/100;
		outMailHdr.to = (clientNum+100)%100;
		outMailHdr.from = postOffice->GetID();
		outMailHdr.length = strlen(ack) + 1;
		
		Message* reply = new Message(outPktHdr, outMailHdr, ack);
		cvs[cvIndex]->QueueReply(reply);
		return true;
	}
	else if (lockIndex/MAX_DLOCK == uniqueGlobalID() && cvIndex/MAX_DCV != uniqueGlobalID()) 
	{
		lockIndex=lockIndex%MAX_DLOCK;
		cvIndex=cvIndex%MAX_DCV;
		if(dlocks[lockIndex] == NULL) 
		{
			errorOutPktHdr = outPktHdr;
			errorOutPktHdr.to = clientNum/100;
			errorInPktHdr = inPktHdr;
			errorOutMailHdr = outMailHdr;
			errorOutMailHdr.to = (clientNum+100)%100;
			errorInMailHdr = inMailHdr;
			return false;
		}
		else if (dlocks[lockIndex]->getOwnerMailID() == -1)
		{
			errorOutPktHdr = outPktHdr;
			errorOutPktHdr.to = clientNum/100;
			errorInPktHdr = inPktHdr;
			errorOutMailHdr = outMailHdr;
			errorOutMailHdr.to = (clientNum+100)%100;
			errorInMailHdr = inMailHdr;	
			return false;
		}
		char* request = new char [MaxMailSize];
		waitingForWaitLocks[clientNum] = lockIndex;
		sprintf(request,"%d_%d_%d_%d",WAITCV, clientNum,cvIndex+uniqueGlobalID()*MAX_DLOCK, lockIndex+uniqueGlobalID()*MAX_DLOCK);
		outPktHdr.to = cvIndex/MAX_DCV;
		outMailHdr.to = outPktHdr.to;
		outMailHdr.from = inMailHdr.from;
		outMailHdr.length = strlen(request) + 1;
		success = postOffice->Send(outPktHdr, outMailHdr, request);
	}
	return success;
}

bool Signal(int cvIndex, int lockIndex, PacketHeader outPktHdr, PacketHeader inPktHdr, MailHeader outMailHdr, MailHeader inMailHdr, int clientNum)
{	
	bool success = false;
	
	if((lockIndex < 0)||(lockIndex > MAX_NUM_GLOBAL_IDS*(MAX_DLOCK)-1))
	{
		errorOutPktHdr = outPktHdr;
		errorOutPktHdr.to = clientNum/100;
		errorInPktHdr = inPktHdr;
		errorOutMailHdr = outMailHdr;
		errorOutMailHdr.to = (clientNum+100)%100;
		errorInMailHdr = inMailHdr;
		return false;
	}
	else if ((cvIndex < 0)||(cvIndex > MAX_NUM_GLOBAL_IDS*MAX_DCV-1))
	{
		errorOutPktHdr = outPktHdr;
		errorOutPktHdr.to = clientNum/100;
		errorInPktHdr = inPktHdr;
		errorOutMailHdr = outMailHdr;
		errorOutMailHdr.to = (clientNum+100)%100;
		errorInMailHdr = inMailHdr;
		return false;
	}

	if (lockIndex/MAX_DLOCK == uniqueGlobalID() && cvIndex/MAX_DCV == uniqueGlobalID()) 
	{
		lockIndex=lockIndex%MAX_DLOCK;
		cvIndex=cvIndex%MAX_DCV;
		if(dlocks[lockIndex] == NULL) 
		{
			errorOutPktHdr = outPktHdr;
			errorOutPktHdr.to = clientNum/100;
			errorInPktHdr = inPktHdr;
			errorOutMailHdr = outMailHdr;
			errorOutMailHdr.to = (clientNum+100)%100;
			errorInMailHdr = inMailHdr;
			return false;
		}
		else if (dlocks[lockIndex]->getOwnerMailID() == -1)
		{
			errorOutPktHdr = outPktHdr;
			errorOutPktHdr.to = clientNum/100;
			errorInPktHdr = inPktHdr;
			errorOutMailHdr = outMailHdr;
			errorOutMailHdr.to = (clientNum+100)%100;
			errorInMailHdr = inMailHdr;
			return false;
		}
		else if(cvs[cvIndex] == NULL)
		{
			errorOutPktHdr = outPktHdr;
			errorOutPktHdr.to = clientNum/100;
			errorInPktHdr = inPktHdr;
			errorOutMailHdr = outMailHdr;
			errorOutMailHdr.to = (clientNum+100)%100;
			errorInMailHdr = inMailHdr;
			return false;
		}
		
		char* ack = new char [5];
		itoa(ack,5,cvIndex);

		outPktHdr.to = clientNum/100;
		outMailHdr.to = (clientNum+100)%100;
		outMailHdr.from = postOffice->GetID();
		outMailHdr.length = strlen(ack) + 1;

		success = postOffice->Send(outPktHdr, outMailHdr, ack); 
		
		if ( !success ) 
		{
		  interrupt->Halt();
		}
		fflush(stdout);
		
		if (!(cvs[cvIndex]->isQueueEmpty())){
			if (cvs[cvIndex]->getFirstLock() != lockIndex+uniqueGlobalID()*MAX_DLOCK)
			{ 
				
			errorOutPktHdr = outPktHdr;
			errorOutPktHdr.to = clientNum/100;
			errorInPktHdr = inPktHdr;
			errorOutMailHdr = outMailHdr;
			errorOutMailHdr.to = (clientNum+100)%100;
			errorInMailHdr = inMailHdr;
				return false;
			}
			else
			{
				Message* reply = cvs[cvIndex]->RemoveReply(); 
				reply->pktHdr.from = reply->pktHdr.to;
				reply->mailHdr.from = reply->mailHdr.to;
				bool success2 = Acquire(lockIndex+uniqueGlobalID()*MAX_DLOCK, outPktHdr, reply->pktHdr,outMailHdr, reply->mailHdr, -2);
				
				if ( !success2 )
				{
				  interrupt->Halt();
				}
				fflush(stdout);
			}
		}
		if (cvs[cvIndex]->isQueueEmpty()){
			cvs[cvIndex]->setFirstLock(-1);
		}	
		return success;
	}
	else if (lockIndex/MAX_DLOCK != uniqueGlobalID() && cvIndex/MAX_DCV == uniqueGlobalID())// lock not on this server
	{
		char* request = new char [MaxMailSize];
		waitingForSignalCvs[clientNum] = cvIndex;
		sprintf(request,"%d_%d_%d",SIGNALLOCK, clientNum, lockIndex+uniqueGlobalID()*MAX_DLOCK);
		outPktHdr.to = lockIndex/MAX_DLOCK;
		outMailHdr.to = outPktHdr.to;
		outMailHdr.from = inMailHdr.from;
		outMailHdr.length = strlen(request) + 1;
		success = postOffice->Send(outPktHdr, outMailHdr, request);
	}
	return success;
}

bool Broadcast(int cvIndex, int lockIndex, PacketHeader outPktHdr, PacketHeader inPktHdr, MailHeader outMailHdr, MailHeader inMailHdr, int clientNum){
	
	bool success = false;
	
	if((lockIndex < 0)||(lockIndex > (MAX_DLOCK-1)))
	{
		errorOutPktHdr = outPktHdr;
		errorOutPktHdr.to = clientNum/100;
		errorInPktHdr = inPktHdr;
		errorOutMailHdr = outMailHdr;
		errorOutMailHdr.to = (clientNum+100)%100;
		errorInMailHdr = inMailHdr;
		return false;
	}
	else if ((cvIndex < 0)||(cvIndex > (MAX_DCV-1)))
	{
		errorOutPktHdr = outPktHdr;
		errorOutPktHdr.to = clientNum/100;
		errorInPktHdr = inPktHdr;
		errorOutMailHdr = outMailHdr;
		errorOutMailHdr.to = (clientNum+100)%100;
		errorInMailHdr = inMailHdr;
		return false;
	}
	if (lockIndex/MAX_DLOCK == uniqueGlobalID() && cvIndex/MAX_DCV == uniqueGlobalID()) 
	{
		lockIndex=lockIndex%MAX_DLOCK;
		cvIndex=cvIndex%MAX_DCV;
		if(dlocks[lockIndex] == NULL)
		{
			errorOutPktHdr = outPktHdr;
			errorOutPktHdr.to = clientNum/100;
			errorInPktHdr = inPktHdr;
			errorOutMailHdr = outMailHdr;
			errorOutMailHdr.to = (clientNum+100)%100;
			errorInMailHdr = inMailHdr;
			return false;
		}
		else if (dlocks[lockIndex]->getOwnerMailID() == -1)
		{
			errorOutPktHdr = outPktHdr;
			errorOutPktHdr.to = clientNum/100;
			errorInPktHdr = inPktHdr;
			errorOutMailHdr = outMailHdr;
			errorOutMailHdr.to = (clientNum+100)%100;
			errorInMailHdr = inMailHdr;
			return false;
		}
		else if(cvs[cvIndex] == NULL)
		{
			errorOutPktHdr = outPktHdr;
			errorOutPktHdr.to = clientNum/100;
			errorInPktHdr = inPktHdr;
			errorOutMailHdr = outMailHdr;
			errorOutMailHdr.to = (clientNum+100)%100;
			errorInMailHdr = inMailHdr;
			return false;
		}
		char* ack = new char [5];
		itoa(ack,5,cvIndex);
		
		outPktHdr.to = inPktHdr.from;
		outMailHdr.to = inMailHdr.from;
		outMailHdr.from = postOffice->GetID();
		outMailHdr.length = strlen(ack) + 1;

		success = postOffice->Send(outPktHdr, outMailHdr, ack); 
		
		if ( !success ) 
		{
		  interrupt->Halt();
		}
		fflush(stdout);
		
		if (!(cvs[cvIndex]->isQueueEmpty())){
			if (cvs[cvIndex]->getFirstLock() != lockIndex+uniqueGlobalID()*MAX_DLOCK)
			{
			errorOutPktHdr = outPktHdr;
			errorInPktHdr = inPktHdr;
			errorOutMailHdr = outMailHdr;
			errorInMailHdr = inMailHdr;
				return false;
			}
			else{
				Message* reply = cvs[cvIndex]->RemoveReply();
				reply->pktHdr.from = reply->pktHdr.to;
				reply->mailHdr.from = reply->mailHdr.to;
				bool success2 = Acquire(lockIndex+uniqueGlobalID()*MAX_DLOCK, outPktHdr, reply->pktHdr,outMailHdr, reply->mailHdr, -2);
				
				if ( !success2 ) 
				{
				  interrupt->Halt();
				}
				fflush(stdout);
			}
		}
		while (!(cvs[cvIndex]->isQueueEmpty())){
				Message* reply = cvs[cvIndex]->RemoveReply(); 
				reply->pktHdr.from = reply->pktHdr.to;
				reply->mailHdr.from = reply->mailHdr.to;
				bool success2 = Acquire(lockIndex+uniqueGlobalID()*MAX_DLOCK, outPktHdr, reply->pktHdr,outMailHdr, reply->mailHdr, -2);
				
				if ( !success2 ) 
				{
				  interrupt->Halt();
				}
				fflush(stdout);
		}

		if (cvs[cvIndex]->isQueueEmpty()){
			cvs[cvIndex]->setFirstLock(-1);
		}
		
		return success;
	}
	else
	{
		char* request = new char [MaxMailSize];
		waitingForBroadcastCvs[clientNum] = cvIndex;
		sprintf(request,"%d_%d_%d",BROADCASTLOCK, clientNum, lockIndex+uniqueGlobalID()*MAX_DLOCK);
		outPktHdr.to = lockIndex/MAX_DLOCK;
		outMailHdr.to = outPktHdr.to;
		outMailHdr.from = inMailHdr.from;
		outMailHdr.length = strlen(request) + 1;
		success = postOffice->Send(outPktHdr, outMailHdr, request);
		return success;
	}
}

bool SendReply(PacketHeader outPktHdr, PacketHeader inPktHdr, MailHeader outMailHdr, MailHeader inMailHdr, char* data)
{
	bool success = false;
	outPktHdr.to = inPktHdr.from;
	outMailHdr.to = inMailHdr.from;
	outMailHdr.from = postOffice->GetID();
	outMailHdr.length = strlen(data) + 1;

	success = postOffice->Send(outPktHdr, outMailHdr, data); 

	if ( !success ) 
	{
	  interrupt->Halt();
	}

	fflush(stdout);
	return success;
}

int myexp ( int count ) {
  int i, val=1;
  for (i=0; i<count; i++ ) {
    val = val * 10;
  }
  return val;
}

void itoa( char arr[], int size, int val ) {
  int i, max, dig, subval, loc;
  for (i=0; i<size; i++ ) {
    arr[i] = '\0';
  }

  for ( i=1; i<=10; i++ ) {
    if (( val / myexp(i) ) == 0 ) {
      max = i-1;
      break;
    }
  }

  subval = 0;
  loc = 0;
  for ( i=max; i>=0; i-- ) {
    dig = 48 + ((val-subval) / myexp(i));
    subval += (dig-48) * myexp(i);
    arr[loc] = dig;
    loc++;
  }

  return;
}

void sendError(){
	bool success = false;
	
	int errorNum = -6;
	
	char * data = new char [4];
	itoa(data,4,errorNum);
	
	errorOutPktHdr.to = errorInPktHdr.from;
	errorOutMailHdr.to = errorInMailHdr.from;
	errorOutMailHdr.from = postOffice->GetID();
	errorOutMailHdr.length = strlen(data) + 1;

	success = postOffice->Send(errorOutPktHdr, errorOutMailHdr, data); 

	if ( !success ) 
	{
	  interrupt->Halt();
	}

	fflush(stdout);
}
