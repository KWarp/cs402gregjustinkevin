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
	lockState=FREE;
	waitQueue=new List;
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

  postOffice->Receive(currentThread->mailID, &inPktHdr, &inMailHdr, &timeStamp, buffer);
  
  // Remove the message we just received from our network thread from unAckedMessages.
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
    printf("Request: Ack message not found in unAckedMessages\n");

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
  
  printf("parsed value: %d\n", *value);
  // i will point to the next character of data.
  return i;
}

void Ack(PacketHeader inPktHdr, MailHeader inMailHdr, timeval timeStamp, char* inData)
{
  // Find the message in unAckedMessages.
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
      found = true;
    }
    if (found)
    {
      // If the message is from our UserProg thread.
      if (inPktHdr.from == postOffice->GetID() &&
          inMailHdr.from == currentThread->mailID + 1)
      {
        // Remove the entry from unAckedMessage to simulate an Ack.
        unAckedMessages.erase(unAckedMessages.begin() + i);
        break;
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
        
        if (!postOffice->Send(outPktHdr, outMailHdr, request))
          interrupt->Halt();
      }
    }
    // Else
    //  Serious problems!!!
  }
}

bool processMessage(PacketHeader inPktHdr, MailHeader inMailHdr, timeval timeStamp, char* msgData)
{
	// printf("\nReady to handle a request!!!\n");
	
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
    printf("requestType: %d\n", requestType);
		case CREATELOCK:
			clientNum = 100 * inPktHdr.from + inMailHdr.from;
			if (createDLockIndex >= 0 && createDLockIndex < (MAX_DLOCK - 1))
			{
				j = 0;
				while (c != '\0')
				{
					c = msgData[++i];
					if (c == '\0')
						break;
					lockName[clientNum][j++] = c;
				}
				lockName[clientNum][j] = '\0';

				for (int k = 0; k < createDLockIndex; ++k)
				{
					if (!strcmp(dlocks[k]->getName(), lockName[clientNum]))
					{
						lockIndex = k;
					}
				}
        
				localLockIndex[clientNum]=lockIndex;
				if (serverCount > 1)
					CreateVerify(lockName[clientNum], CREATELOCKVERIFY, clientNum);
				else
				{
			
					if(localLockIndex[clientNum] == -1)
					{
						for(i=0;lockName[clientNum][i]!='\0';i++)
						{
							localLockName[i]=lockName[clientNum][i];
							localLockName[i+1]= '\0';
						}
						dlocks[createDLockIndex] = new DistributedLock(localLockName); 
						dlocks[createDLockIndex]->SetGlobalId((postOffice->GetID()*MAX_DLOCK)+createDLockIndex);
						if(dlocks[createDLockIndex] == NULL)
						{
							errorOutPktHdr.to = clientNum/100;
							errorInPktHdr = inPktHdr;
							errorOutMailHdr.to = (clientNum+100)%100;
							errorInMailHdr = inMailHdr;
							return false;
						}
					
					lockIndex = dlocks[createDLockIndex]->GetGlobalId();
					createDLockIndex++;
					}
					else
					{		
						lockIndex=dlocks[localLockIndex[clientNum]]->GetGlobalId();
					}
					char ack[MaxMailSize];
          
          
					sprintf(ack, "%i", lockIndex);
					
					outPktHdr.to = clientNum/100;
					outMailHdr.to = (clientNum+100)%100;
					outMailHdr.from = postOffice->GetID();
					outMailHdr.length = strlen(ack) + 1;

					success = postOffice->Send(outPktHdr, outMailHdr, ack); 
          
					if ( !success ) 
					{
					  interrupt->Halt();
					}
					verifyResponses[clientNum]=0;
					localLockIndex[clientNum]=-1;
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
			
		case CREATELOCKANSWER:
			x=0;
			do 
			{
				c = msgData[++i];
				if(c=='_')
					break;
				client[x++] = c;	
			}while(c!='_'); 
			client[x]='\0';
			clientNum = atoi(client);
			while(c!='\0')
			{
				c = msgData[++i];
				verifyBuffer[clientNum][verifyResponses[clientNum]][a++] = c;
				if(c=='\0')
					break;
			}
			verifyResponses[clientNum]++;
			if (verifyResponses[clientNum]==serverCount-1)
			{
				for (i=0; i< serverCount-1; i++)
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
				if(localLockIndex[clientNum] == -1 && otherServerLockIndex ==-1) 
				{
					for(i=0;lockName[clientNum][i]!='\0';i++){
						localLockName[i]=lockName[clientNum][i];
						localLockName[i+1]= '\0';
					}
					dlocks[createDLockIndex] = new DistributedLock(localLockName); 
					dlocks[createDLockIndex]->SetGlobalId((postOffice->GetID()*MAX_DLOCK)+createDLockIndex);
					if(dlocks[createDLockIndex] == NULL)
					{
						//Handle error...
						printf("Error in creation of %s!\n", lockName[clientNum]);
						errorOutPktHdr.to = clientNum/100;
						errorInPktHdr = inPktHdr;
						errorOutMailHdr.to = (clientNum+100)%100;
						errorInMailHdr = inMailHdr;
						return false;
					}
					
					lockIndex = dlocks[createDLockIndex]->GetGlobalId();
					createDLockIndex++;
				}
				else if (localLockIndex[clientNum] == -1 && otherServerLockIndex !=-1)
				{
					lockIndex=otherServerLockIndex; //set lock
				}
				else if (localLockIndex[clientNum] != -1 && otherServerLockIndex==-1)
				{
					lockIndex=dlocks[localLockIndex[clientNum]]->GetGlobalId();
				}
				else
				{		
				}
				char* ack = new char [5];
				sprintf(ack, "%i", lockIndex);
	
				outPktHdr.to = clientNum/100;
				outMailHdr.to = (clientNum+100)%100;
				outMailHdr.from = postOffice->GetID();
				outMailHdr.length = strlen(ack) + 1;

				success = postOffice->Send(outPktHdr, outMailHdr, ack); 

				if ( !success ) 
				{
				  interrupt->Halt();
				}
				verifyResponses[clientNum]=0;
				localLockIndex[clientNum]=-1;
				fflush(stdout);
			}
			break;
		case CREATELOCKVERIFY:
			
			printf("Verifying Lock\n");
			lockIndex=-1;
			request = new char [MaxMailSize];
			x=0;
			do 
			{
				c = msgData[++i];
				if(c=='_')
					break;
				client[x++] = c;	
			}while(c!='_');

			client[x]='\0';
			clientNum = atoi(client);
			while(c!='\0')
			{
				c = msgData[++i];
				if(c=='\0')
					break;
				localLockName[a++] = c; 
			}
			localLockName[a] = '\0';
			
			for(int k=0; k<createDLockIndex; k++)
			{
				if(!strcmp(dlocks[k]->getName(), localLockName))
				{
					lockIndex = k;
					break;
				}
			}
			
			outPktHdr.to = inPktHdr.from;
			outMailHdr.to = inMailHdr.from;
			outMailHdr.from = postOffice->GetID();
			
			if (lockIndex==-1) 
			{	
				sprintf(request,"%d_%d_%d",CREATELOCKANSWER, clientNum, -1);
				outMailHdr.length = strlen(request) + 1;

				success = postOffice->Send(outPktHdr, outMailHdr, request); 
			}
			else 
			{
				sprintf(request,"%d_%d_%d",CREATELOCKANSWER, clientNum, dlocks[lockIndex]->GetGlobalId());
				outMailHdr.length = strlen(request) + 1;
				success = postOffice->Send(outPktHdr, outMailHdr, request); 
			}
			break;
		case CREATECV:
			clientNum=100*(inPktHdr.from)+inMailHdr.from;
			if((createDCVIndex >= 0)&&(createDCVIndex < (MAX_DCV-1)))
			{
				j = 0;
				while(c!='\0')
				{
					c = msgData[++i];
					if(c=='\0')
						break;
					lockName[clientNum][j++] = c; 
				}
				lockName[clientNum][j] = '\0'; 
				for(int k=0; k<createDCVIndex; k++)
				{
					if(!strcmp(cvs[k]->getName(), lockName[clientNum]))
					{
						cvIndex = k;
					}
				}
				localLockIndex[clientNum]=cvIndex;
				if(serverCount>1)
					CreateVerify(lockName[clientNum], CREATECVVERIFY, clientNum);
				else
				{
					if(localLockIndex[clientNum] == -1) 
					{
						for(i=0;lockName[clientNum][i]!='\0';i++)
						{
							localLockName[i]=lockName[clientNum][i];
							localLockName[i+1]= '\0';
						}
						cvs[createDCVIndex] = new DistributedCV(localLockName); 
						cvs[createDCVIndex]->SetGlobalId((postOffice->GetID()*MAX_DCV)+createDCVIndex);
						if(cvs[createDCVIndex] == NULL)
						{
							errorOutPktHdr.to = clientNum/100;
							errorInPktHdr = inPktHdr;
							errorOutMailHdr.to = (clientNum+100)%100;
							errorInMailHdr = inMailHdr;
							return false;
						}
						
						cvIndex = cvs[createDCVIndex]->GetGlobalId();
						createDCVIndex++;
					}
					else
					{			
						cvIndex=cvs[cvIndex]->GetGlobalId();
					}
					char* ack = new char [5];
					sprintf(ack, "%i", cvIndex);
		
					outPktHdr.to = clientNum/100;
					outMailHdr.to = (clientNum+100)%100;
					outMailHdr.from = postOffice->GetID();
					outMailHdr.length = strlen(ack) + 1;
					success = postOffice->Send(outPktHdr, outMailHdr, ack); 

					if ( !success ) 
					{
					  interrupt->Halt();
					}
					verifyResponses[clientNum]=0;
					localLockIndex[clientNum]=-1;
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
			x=0;
			do 
			{
				c = msgData[++i];
				if(c=='_')
					break;
				client[x++] = c;	
			}while(c!='_'); 
			client[x]='\0';
			clientNum = atoi(client);
			
			while(c!='\0') 
			{
				c = msgData[++i];
				verifyBuffer[clientNum][verifyResponses[clientNum]][a++] = c;
				if(c=='\0')
					break;
			}
			verifyResponses[clientNum]++;
			if (verifyResponses[clientNum]==serverCount-1)
			{
				for (i=0; i< serverCount-1; i++)
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
				if(localLockIndex[clientNum] == -1 && otherServerLockIndex ==-1) //If not found here or elsewhere
				{
					for(i=0;lockName[clientNum][i]!='\0';i++){
						localLockName[i]=lockName[clientNum][i];
						localLockName[i+1]= '\0';
					}
					cvs[createDCVIndex] = new DistributedCV(localLockName); 
					cvs[createDCVIndex]->SetGlobalId((postOffice->GetID()*MAX_DCV)+createDCVIndex);
					if(cvs[createDCVIndex] == NULL)
					{
						errorOutPktHdr.to = clientNum/100;
						errorInPktHdr = inPktHdr;
						errorOutMailHdr.to = (clientNum+100)%100;
						errorInMailHdr = inMailHdr;
						return false;
					}
					
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
				{
						
				}
				char* ack = new char [5];
				sprintf(ack, "%i", cvIndex);
				outPktHdr.to = clientNum/100;
				outMailHdr.to = (clientNum+100)%100;
				outMailHdr.from = postOffice->GetID();
				outMailHdr.length = strlen(ack) + 1;

				success = postOffice->Send(outPktHdr, outMailHdr, ack); 

				if ( !success ) 
				{
				  interrupt->Halt();
				}
				verifyResponses[clientNum]=0;
				localLockIndex[clientNum]=-1;
				fflush(stdout);
			}
			break;
		case CREATECVVERIFY:
			lockIndex=-1;
			
			request = new char [MaxMailSize];
			x=0;
			do 
			{
				c = msgData[++i];
				if(c=='_')
					break;
				client[x++] = c;	
			}while(c!='_');

			client[x]='\0';
			clientNum = atoi(client);
			while(c!='\0')
			{
				c = msgData[++i];
				if(c=='\0')
					break;
				localLockName[a++] = c; 
			}
			localLockName[a] = '\0';
			for(j=0; j<createDCVIndex; j++)
			{
				if(!strcmp(cvs[j]->getName(), localLockName))
				{
					cvIndex = j;
				}
			}
			outPktHdr.to = inPktHdr.from;
			outMailHdr.to = inMailHdr.from;
			outMailHdr.from = postOffice->GetID();
			if(cvIndex == -1) 
			{
				sprintf(request,"%d_%d_%d",CREATECVANSWER, clientNum, -1);
				outMailHdr.length = strlen(request) + 1;
				success = postOffice->Send(outPktHdr, outMailHdr, request); 
			}
			else 
			{
				sprintf(request,"%d_%d_%d",CREATECVANSWER, clientNum, cvs[cvIndex]->GetGlobalId());
				outMailHdr.length = strlen(request) + 1;
				success = postOffice->Send(outPktHdr, outMailHdr, request); 
			}

			break;
		case CREATEMV:
			clientNum=100*(inPktHdr.from)+inMailHdr.from;
			if((createDMVIndex >= 0)&&(createDMVIndex < (MAX_DMV-1)))
			{
				j = 0;
				while(c!='\0')
				{
					c = msgData[++i];
					if(c=='\0')
						break;
					lockName[clientNum][j++] = c;  
				}
				lockName[clientNum][j] = '\0' ;  
				
				for(int k=0; k<createDMVIndex; k++)
				{
					if(!strcmp(mvs[k]->getName(), lockName[clientNum]))
					{
						mvIndex = k;
						break;
					}
				}
				localLockIndex[clientNum]=mvIndex;
				if (serverCount>1)
					CreateVerify(lockName[clientNum], CREATEMVVERIFY, clientNum);
				else
				{
					if(localLockIndex[clientNum] == -1) 
					{
						for(i=0;lockName[clientNum][i]!='\0';i++)
						{
							localLockName[i]=lockName[clientNum][i];
							localLockName[i+1]= '\0';
						}
						mvs[createDMVIndex] = new DistributedMV(localLockName); 
						mvs[createDMVIndex]->SetGlobalId((postOffice->GetID()*MAX_DMV)+createDMVIndex);
						if(mvs[createDMVIndex] == NULL)
						{
							errorOutPktHdr.to = clientNum/100;
							errorInPktHdr = inPktHdr;
							errorOutMailHdr.to = (clientNum+100)%100;
							errorInMailHdr = inMailHdr;
							return false;
						}
						
						mvIndex = mvs[createDMVIndex]->GetGlobalId();
						createDMVIndex++;
					}
					else
					{
						mvIndex=mvs[localLockIndex[clientNum]]->GetGlobalId();
					}
					char* ack = new char [5];
					sprintf(ack, "%i", mvIndex);
					
					outPktHdr.to = clientNum/100;
					outMailHdr.to = (clientNum+100)%100;
					outMailHdr.from = postOffice->GetID();
					outMailHdr.length = strlen(ack) + 1;
					
					success = postOffice->Send(outPktHdr, outMailHdr, ack); 

					if ( !success ) 
					{
					  interrupt->Halt();
					}
					verifyResponses[clientNum]=0;
					localLockIndex[clientNum]=-1;
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
			x=0;
			do 
			{
				c = msgData[++i];
				if(c=='_')
					break;
				client[x++] = c;	
			}while(c!='_'); 
			client[x]='\0';
			clientNum = atoi(client);
			
			while(c!='\0') 
			{
				c = msgData[++i];
				verifyBuffer[clientNum][verifyResponses[clientNum]][a++] = c;
				if(c=='\0')
					break;
			}
			verifyResponses[clientNum]++;
			if (verifyResponses[clientNum]==serverCount-1)
			{
				for (i=0; i< serverCount-1; i++)
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
				if(localLockIndex[clientNum] == -1 && otherServerLockIndex ==-1) 
				{
					for(i=0;lockName[clientNum][i]!='\0';i++){
						localLockName[i]=lockName[clientNum][i];
						localLockName[i+1]= '\0';
					}
					mvs[createDMVIndex] = new DistributedMV(localLockName); 
					mvs[createDMVIndex]->SetGlobalId((postOffice->GetID()*MAX_DMV)+createDMVIndex);
					if(mvs[createDMVIndex] == NULL)
					{
						errorOutPktHdr.to = clientNum/100;
						errorInPktHdr = inPktHdr;
						errorOutMailHdr.to = (clientNum+100)%100;
						errorInMailHdr = inMailHdr;
						return false;
					}
					
					mvIndex = mvs[createDMVIndex]->GetGlobalId();
					createDMVIndex++;
				}
				else if (localLockIndex[clientNum] == -1 && otherServerLockIndex !=-1) 
				{
					mvIndex=otherServerLockIndex;
				}
				else if (localLockIndex[clientNum] != -1 && otherServerLockIndex==-1)
				{
					
					mvIndex=mvs[localLockIndex[clientNum]]->GetGlobalId();
				}
				else
				{
				}
				char* ack = new char [5];
				sprintf(ack, "%i", mvIndex);
				
				outPktHdr.to = clientNum/100;
				outMailHdr.to = (clientNum+100)%100;
				outMailHdr.from = postOffice->GetID();
				outMailHdr.length = strlen(ack) + 1;
				
				success = postOffice->Send(outPktHdr, outMailHdr, ack); 

				if ( !success ) 
				{
				  interrupt->Halt();
				}
				verifyResponses[clientNum]=0;
				localLockIndex[clientNum]=-1;
				fflush(stdout);
			}

			break;
		case CREATEMVVERIFY:
			mvIndex=-1;
			request = new char [MaxMailSize];
			x=0;
			do 
			{
				c = msgData[++i];
				if(c=='_')
					break;
				client[x++] = c;	
			}while(c!='_');

			client[x]='\0';
			clientNum = atoi(client);
			while(c!='\0') 
			{
				c = msgData[++i];
				if(c=='\0')
					break;
				localLockName[a++] = c; 
			}
			localLockName[a] = '\0';
			for(j=0; j<createDMVIndex; j++)
			{
				if(!strcmp(mvs[j]->getName(), localLockName))
				{
					mvIndex = j;
					break;
				}
			}
			outPktHdr.to = inPktHdr.from;
			outMailHdr.to = inMailHdr.from;
			outMailHdr.from = postOffice->GetID();
			if(mvIndex == -1) 
			{
				sprintf(request,"%d_%d_%d",CREATEMVANSWER, clientNum, -1);
				outMailHdr.length = strlen(request) + 1;
				success = postOffice->Send(outPktHdr, outMailHdr, request); 
			}
			else
			{
				sprintf(request,"%d_%d_%d",CREATEMVANSWER, clientNum, mvs[mvIndex]->GetGlobalId());
				outMailHdr.length = strlen(request) + 1;
				success = postOffice->Send(outPktHdr, outMailHdr, request); 
			}
			
			break;
		case ACQUIRE:
			printf("here\n");
			clientNum=100*(inPktHdr.from)+inMailHdr.from;
			m=0; 
			if(msgData[i+1] == '_')
			{
        printf("here2\n");
				i++;
				do 
				{
					c = msgData[++i];
					if(c=='_')
						break;
					client[m++] = c;	
				}while(c!='_');
        printf("here3\n");
        
				client[m]='\0';
				clientNum = atoi(client);
			}
      printf("here4\n");
			m=0;
			while(c!='\0')
			{
				c = msgData[++i];
				lockIndexBuf[m++] = c; 
			}
			lockIndex = atoi(lockIndexBuf); 
			printf("here5\n");
			if(lockIndex/MAX_DLOCK != postOffice->GetID())
			{
				sprintf(buffer,"%d__%d_%s",ACQUIRE, clientNum, lockIndexBuf);
				outPktHdr.to = lockIndex/MAX_DLOCK;
				outMailHdr.to = outPktHdr.to;
				outMailHdr.from = inMailHdr.from;
				outMailHdr.length = strlen(buffer) + 1;
				success = postOffice->Send(outPktHdr, outMailHdr, buffer);
			}
			else
				success = Acquire(lockIndex, outPktHdr, inPktHdr, outMailHdr, inMailHdr, clientNum);
      printf("here6\n");
			break;
      
		case RELEASE:
			clientNum=100*(inPktHdr.from)+inMailHdr.from;
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
			if(lockIndex/MAX_DLOCK != postOffice->GetID())
			{
				sprintf(buffer,"%d__%d_%s",RELEASE, clientNum, lockIndexBuf);
				outPktHdr.to = lockIndex/MAX_DLOCK;
				outMailHdr.to = outPktHdr.to;
				outMailHdr.from = inMailHdr.from;
				outMailHdr.length = strlen(buffer) + 1;
				success = postOffice->Send(outPktHdr, outMailHdr, buffer);
			}
			else
				success = Release(lockIndex, outPktHdr, inPktHdr, outMailHdr, inMailHdr, clientNum);
			break;
		case DESTROYLOCK:
		
			clientNum=100*(inPktHdr.from)+inMailHdr.from;
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
			if(lockIndex/MAX_DLOCK != postOffice->GetID())
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
			
			clientNum=100*(inPktHdr.from)+inMailHdr.from;
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
			if(mvIndex/MAX_DMV != postOffice->GetID())
			{
				sprintf(buffer,"%d__%d_%s_%s",SETMV, clientNum, mvIndexBuf, mvValBuf);
				outPktHdr.to = mvIndex/MAX_DMV;
				outMailHdr.to = outPktHdr.to;
				outMailHdr.from = inMailHdr.from;
				outMailHdr.length = strlen(buffer) + 1;
				success = postOffice->Send(outPktHdr, outMailHdr, buffer);
			}
			else
				success = SetMV(mvIndex, mvValue, outPktHdr, inPktHdr, outMailHdr, inMailHdr, clientNum);
			break;
		case GETMV:
			clientNum=100*(inPktHdr.from)+inMailHdr.from;
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
			if(mvIndex/MAX_DMV != postOffice->GetID())
			{
				sprintf(buffer,"%d__%d_%s",GETMV, clientNum, mvIndexBuf);
				outPktHdr.to = mvIndex/MAX_DMV;
				outMailHdr.to = outPktHdr.to;
				outMailHdr.from = inMailHdr.from;
				outMailHdr.length = strlen(buffer) + 1;
				success = postOffice->Send(outPktHdr, outMailHdr, buffer);
			}
			else
				success = GetMV(mvIndex, outPktHdr, inPktHdr, outMailHdr, inMailHdr, clientNum);
			
			break;
		case DESTROYMV:
			clientNum=100*(inPktHdr.from)+inMailHdr.from;
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
			if(mvIndex/MAX_DMV != postOffice->GetID())
			{
				sprintf(buffer,"%d__%d_%s",DESTROYMV, clientNum, mvIndexBuf);
				outPktHdr.to = mvIndex/MAX_DMV;
				outMailHdr.to = outPktHdr.to;
				outMailHdr.from = inMailHdr.from;
				outMailHdr.length = strlen(buffer) + 1;
				success = postOffice->Send(outPktHdr, outMailHdr, buffer);
			}
			else
				success = DestroyMV(mvIndex, outPktHdr, inPktHdr, outMailHdr, inMailHdr, clientNum);
			break;	
		case WAIT:
			clientNum=100*(inPktHdr.from)+inMailHdr.from;
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
			if(lockIndex/MAX_DLOCK != postOffice->GetID())
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
			clientNum=100*(inPktHdr.from)+inMailHdr.from;
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
			clientNum=100*(inPktHdr.from)+inMailHdr.from;
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
			clientNum=100*(inPktHdr.from)+inMailHdr.from;
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
			
			if(cvIndex/MAX_DCV != postOffice->GetID())
			{
				sprintf(buffer,"%d__%d_%s_%s",SIGNAL, clientNum, cvNum,lockNum);
				outPktHdr.to = cvIndex/MAX_DCV;
				outMailHdr.to = outPktHdr.to;
				outMailHdr.from = inMailHdr.from;
				outMailHdr.length = strlen(buffer) + 1;
				success = postOffice->Send(outPktHdr, outMailHdr, buffer);
			}
			else
				success = Signal(cvIndex, lockIndex, outPktHdr, inPktHdr, outMailHdr, inMailHdr,clientNum);
			
			break;
			
		case SIGNALLOCK:
			clientNum=100*(inPktHdr.from)+inMailHdr.from;
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
			clientNum=100*(inPktHdr.from)+inMailHdr.from;
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
			clientNum=100*(inPktHdr.from)+inMailHdr.from;
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
			
			if(cvIndex/MAX_DCV != postOffice->GetID())
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
			clientNum=100*(inPktHdr.from)+inMailHdr.from;
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
			clientNum=100*(inPktHdr.from)+inMailHdr.from;
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
			clientNum=100*(inPktHdr.from)+inMailHdr.from;
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
			if(cvIndex/MAX_DCV != postOffice->GetID())
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
				if ((cvIndex < 0)||(cvIndex > serverCount*MAX_DCV-1))
				{
					errorOutPktHdr = outPktHdr;
					errorOutPktHdr.to = clientNum/100;
					errorInPktHdr = inPktHdr;
					errorOutMailHdr = outMailHdr;
					errorOutMailHdr.to = (clientNum+100)%100;
					errorInMailHdr = inMailHdr;
					return false;
				}
				else if(cvIndex/MAX_DCV== postOffice->GetID())
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
		clientNum=100*(inPktHdr.from)+inMailHdr.from;
	
	if((lockIndex < 0)||(lockIndex > serverCount*(MAX_DLOCK)-1))
	{
		printf("Cannot acquire: Lock index [%d] out of bounds [%d, %d].\n", lockIndex, 0, (MAX_DLOCK-1));
		errorOutPktHdr = outPktHdr;
		errorOutPktHdr.to = clientNum/100;
		errorInPktHdr = inPktHdr;
		errorOutMailHdr = outMailHdr;
		errorOutMailHdr.to = (clientNum+100)%100;
		errorInMailHdr = inMailHdr;
		return false;
	}
	else if(lockIndex/MAX_DLOCK == postOffice->GetID())
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
				outMailHdr.to = (clientNum+100)%100;
				outMailHdr.from = postOffice->GetID();
				outMailHdr.length = strlen(ack) + 1;
				
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

void CreateVerify(char* data, int requestType, int client)
{
	char* request = new char [MaxMailSize];
	sprintf(request,"%d_%d_%s",requestType, client, data);
	
	PacketHeader outPktHdr;
	MailHeader outMailHdr;

	outMailHdr.from = postOffice->GetID();
	outMailHdr.length = strlen(request) + 1; 
	for (int i = 0; i< serverCount; i++)
	{
		if (i != outMailHdr.from)	
		{
			outPktHdr.to = i;		
		 	outMailHdr.to = i;
			bool success = postOffice->Send(outPktHdr, outMailHdr, request); 

			 if ( !success ) {
  				interrupt->Halt();
			 }
			 else
				 printf("Verify send successful\n");
		}
	}
	verifyResponses[client]=0;
}

bool Release(int lockIndex, PacketHeader outPktHdr, PacketHeader inPktHdr, MailHeader outMailHdr, MailHeader inMailHdr, int clientNum)
{
	bool success = false;
	if((lockIndex < 0)||(lockIndex > serverCount*(MAX_DLOCK)-1))
	{
		errorOutPktHdr = outPktHdr;
		errorOutPktHdr.to = clientNum/100;
		errorInPktHdr = inPktHdr;
		errorOutMailHdr = outMailHdr;
		errorOutMailHdr.to = (clientNum+100)%100;
		errorInMailHdr = inMailHdr;
		return false;
	}
	else if(lockIndex/MAX_DLOCK == postOffice->GetID())
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
		outMailHdr.to = (clientNum+100)%100;
		outMailHdr.from = postOffice->GetID();
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
	if((lockIndex < 0)||(lockIndex > serverCount*(MAX_DLOCK)-1))
	{
		errorOutPktHdr = outPktHdr;
		errorOutPktHdr.to = clientNum/100;
		errorInPktHdr = inPktHdr;
		errorOutMailHdr = outMailHdr;
		errorOutMailHdr.to = (clientNum+100)%100;
		errorInMailHdr = inMailHdr;
		return false;
	}
	else if(lockIndex/MAX_DLOCK == postOffice->GetID())
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
		outMailHdr.to = (clientNum+100)%100;
		outMailHdr.from = postOffice->GetID();
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
	
	if((mvIndex < 0)||(mvIndex > serverCount*MAX_DMV-1))
	{
		errorOutPktHdr = outPktHdr;
		errorOutPktHdr.to = clientNum/100;
		errorInPktHdr = inPktHdr;
		errorOutMailHdr = outMailHdr;
		errorOutMailHdr.to = (clientNum+100)%100;
		errorInMailHdr = inMailHdr;
		return false;
	}
	else if (mvIndex/MAX_DMV == postOffice->GetID())
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

		char * ack = new char [32];
		sprintf(ack, "Machine %d has set MV[%d] = %d!", postOffice->GetID(), mvs[mvIndex]->GetGlobalId(), value);

		mvs[mvIndex]->setValue(value); 
			
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
	
	if((mvIndex < 0)||(mvIndex > serverCount*MAX_DMV-1))
	{
		itoa(ack,4,999);
		return SendReply(outPktHdr, inPktHdr, outMailHdr, inMailHdr, ack);
	}
	else if (mvIndex/MAX_DMV == postOffice->GetID())
	{
		mvIndex=mvIndex%MAX_DMV;
		if(mvs[mvIndex] == NULL)
		{
			printf("Cannot get: MV does not exist at index %d.\n", mvIndex);
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
	if((mvIndex < 0)||(mvIndex > serverCount*MAX_DMV-1))
	{
		printf("Cannot get: MV index [%d] out of bounds [%d, %d].\n", mvIndex, 0, (MAX_DMV-1));
		errorOutPktHdr = outPktHdr;
		errorOutPktHdr.to = clientNum/100;
		errorInPktHdr = inPktHdr;
		errorOutMailHdr = outMailHdr;
		errorOutMailHdr.to = (clientNum+100)%100;
		errorInMailHdr = inMailHdr;
		return false;
	}
	else if (mvIndex/MAX_DMV == postOffice->GetID())
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
	if (lockIndex/MAX_DLOCK == postOffice->GetID() && cvIndex/MAX_DCV == postOffice->GetID())
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
			cvs[cvIndex]-> setFirstLock(lockIndex+postOffice->GetID()*MAX_DLOCK);
			
		
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
		itoa(ack,5,cvIndex+postOffice->GetID()*MAX_DCV);
		
		outPktHdr.to = clientNum/100;
		outMailHdr.to = (clientNum+100)%100;
		outMailHdr.from = postOffice->GetID();
		outMailHdr.length = strlen(ack) + 1;
		
		Message* reply = new Message(outPktHdr, outMailHdr, ack);
		cvs[cvIndex]->QueueReply(reply);
		return true;
	}
	else if (lockIndex/MAX_DLOCK == postOffice->GetID() && cvIndex/MAX_DCV != postOffice->GetID()) 
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
		sprintf(request,"%d_%d_%d_%d",WAITCV, clientNum,cvIndex+postOffice->GetID()*MAX_DLOCK, lockIndex+postOffice->GetID()*MAX_DLOCK);
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
	
	if((lockIndex < 0)||(lockIndex > serverCount*(MAX_DLOCK)-1))
	{
		errorOutPktHdr = outPktHdr;
		errorOutPktHdr.to = clientNum/100;
		errorInPktHdr = inPktHdr;
		errorOutMailHdr = outMailHdr;
		errorOutMailHdr.to = (clientNum+100)%100;
		errorInMailHdr = inMailHdr;
		return false;
	}
	else if ((cvIndex < 0)||(cvIndex > serverCount*MAX_DCV-1))
	{
		errorOutPktHdr = outPktHdr;
		errorOutPktHdr.to = clientNum/100;
		errorInPktHdr = inPktHdr;
		errorOutMailHdr = outMailHdr;
		errorOutMailHdr.to = (clientNum+100)%100;
		errorInMailHdr = inMailHdr;
		return false;
	}

	if (lockIndex/MAX_DLOCK == postOffice->GetID() && cvIndex/MAX_DCV == postOffice->GetID()) 
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
			if (cvs[cvIndex]->getFirstLock() != lockIndex+postOffice->GetID()*MAX_DLOCK)
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
				bool success2 = Acquire(lockIndex+postOffice->GetID()*MAX_DLOCK, outPktHdr, reply->pktHdr,outMailHdr, reply->mailHdr, -2);
				
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
	else if (lockIndex/MAX_DLOCK != postOffice->GetID() && cvIndex/MAX_DCV == postOffice->GetID())// lock not on this server
	{
		char* request = new char [MaxMailSize];
		waitingForSignalCvs[clientNum] = cvIndex;
		sprintf(request,"%d_%d_%d",SIGNALLOCK, clientNum, lockIndex+postOffice->GetID()*MAX_DLOCK);
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
	if (lockIndex/MAX_DLOCK == postOffice->GetID() && cvIndex/MAX_DCV == postOffice->GetID()) 
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
			if (cvs[cvIndex]->getFirstLock() != lockIndex+postOffice->GetID()*MAX_DLOCK)
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
				bool success2 = Acquire(lockIndex+postOffice->GetID()*MAX_DLOCK, outPktHdr, reply->pktHdr,outMailHdr, reply->mailHdr, -2);
				
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
				bool success2 = Acquire(lockIndex+postOffice->GetID()*MAX_DLOCK, outPktHdr, reply->pktHdr,outMailHdr, reply->mailHdr, -2);
				
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
		sprintf(request,"%d_%d_%d",BROADCASTLOCK, clientNum, lockIndex+postOffice->GetID()*MAX_DLOCK);
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
