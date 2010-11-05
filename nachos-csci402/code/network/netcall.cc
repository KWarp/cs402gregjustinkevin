#include "netcall.h"

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

void DistributedLock::QueueReply(Message* reply)
{
	waitQueue->Append((void*)reply);
}

Message* DistributedLock::RemoveReply()
{
	return (Message*)waitQueue->Remove();
}

bool DistributedLock::isQueueEmpty()
{
	return (Message*)waitQueue->IsEmpty();
}

void DistributedLock::setLockState(bool ls)
{ 
	lockState = ls; 
}

void DistributedLock::setOwner(int machID, int mailID)
{
	ownerMachID = machID;
	ownerMailID = mailID;
}

DistributedLock::~DistributedLock() 
{
	delete waitQueue;
}

DistributedMV::DistributedMV(const char* debugName) 
{
	name = debugName;
	globalId = -1;
}

DistributedMV::~DistributedMV() 
{
	//delete name;
}

DistributedCV::DistributedCV(const char* debugName) 
{
	name=debugName;
	firstLock=NULL;
	waitQueue=new List;
	ownerMachID = -1;		
	ownerMailID = -1;
	globalId = -1;
}

void DistributedCV::QueueReply(Message* reply)
{
	waitQueue->Append((void*)reply);
}

Message* DistributedCV::RemoveReply()
{
	return (Message*)waitQueue->Remove();
}

bool DistributedCV::isQueueEmpty()
{
	return (Message*)waitQueue->IsEmpty();
}

void DistributedCV::setFirstLock(int fl)
{ 
	firstLock = fl; 
}

void DistributedCV::setOwner(int machID, int mailID)
{
	ownerMachID = machID;
	ownerMailID = mailID;
}

DistributedCV::~DistributedCV() 
{
	delete waitQueue;
}

DistributedLock* locks[MAX_DLOCK]; //array of locks
DistributedMV* mvs[MAX_DMV]; //array of MVs
DistributedCV* cvs[MAX_DCV]; //array of CVs
int createDLockIndex = 0; // Counter of locks being created
int createDCVIndex = 0; // Counter of CVs being created
int createDMVIndex = 0; // Counter of MVs being created
PacketHeader errorOutPktHdr, errorInPktHdr; // used for sending Error Message
MailHeader errorOutMailHdr, errorInMailHdr; // used for sending Error Message
char buffer[MaxMailSize];

///////locks/////////////////
char verifyBuffer[MAX_CLIENTS][4][5];
char lockName[MAX_CLIENTS][32];
int verifyResponses[MAX_CLIENTS];
int localLockIndex[MAX_CLIENTS];
///////locks/////////////////

//////cvs//////////////////
int waitingForSignalCvs[MAX_CLIENTS];
int waitingForBroadcastCvs[MAX_CLIENTS];
int waitingForWaitLocks[MAX_CLIENTS];

//Request - For use by clients
int Request(int requestType, char* data, int mailID)
{
	char* request = new char [MaxMailSize];
    sprintf(request,"%d_%s",requestType, data);
	
	for(int i=0;i<MaxMailSize;i++)
		buffer[i]='\0';
		
	PacketHeader outPktHdr, inPktHdr;
    MailHeader outMailHdr, inMailHdr;

    outPktHdr.to = ((int)Random())%serverCount;		
    outMailHdr.to = outPktHdr.to;
    outMailHdr.from = postOffice->GetID(); 
    outMailHdr.length = strlen(request) + 1;

    if ( !postOffice->Send(outPktHdr, outMailHdr, request)) 
	{
      interrupt->Halt();
    }

    postOffice->Receive(postOffice->GetID(), &inPktHdr, &inMailHdr, buffer);
	
	return atoi(buffer); //buffer should contain the index lock/mv created/destroyed at or value of mv getted
}

//HandleRequest - For use by server
bool HandleRequest()
{
	printf("\nReady to handle a request!!!\n");

	PacketHeader outPktHdr, inPktHdr;
    MailHeader outMailHdr, inMailHdr;
	
	//char buffer[MaxMailSize];		//All the data sent in request
	for(int i=0;i<MaxMailSize;buffer[i++]='\0');
	char reqType[MaxMailSize];		//The extracted request type
	int requestType = -1;			//The extracted request type converted to int
	char* localLockName = new char[32];		//The extracted lockName if included
	char lockIndexBuf[MaxMailSize];	//The extracted lockIndex if included
	char mvIndexBuf[MaxMailSize];	//The extracted mvIndex if included
	char mvValBuf[MaxMailSize];	//The extracted value to set MV if included
	char* cvName = new char[32];		//CV Name from the Client for Create
	char cvNum[MaxMailSize];		//CV number from Client for Signal & Wait
	char lockNum[MaxMailSize];		//Lock number from Client for Signal & Wait
	
	bool success = false;
	
	int lockIndex = -1;  //Lock index to send back to client
	char* mvName = new char[32];
	int mvIndex = -1; //MV index to send back to client
	int mvValue = 0; //MV value to set
	int cvIndex = -1; //CV index to send back to client
	int lockNameIndex = 0; //counter reading lock name from client
	int cvNameIndex = 0; //counter reading CV name from client
	int cvNumIndex = 0; //counter reading CV Number from client
	int lockNumIndex = 0; //counter reading lock Number from client
	int otherServerLockIndex = -1;
	int otherServerIndex=0;
	char client[5];
	int clientNum;  /// Convention: clientNum = 10*(X-serverCount)+Y where X = machine id (ie inPktHdr.from) and Y = mailbox number (ie inMailHdr.from).  Y can range from [0,9]
					/// so that means there is a maximum of 10 threads per machine id
	char* request = NULL;

	// Wait for a request from client
	postOffice->Receive(postOffice->GetID(), &inPktHdr, &inMailHdr, buffer);

	char c = '?';
	int i,a ,m ,j,x;
	
	while(c!='_') //Until find underscore, search string. Want to extract request type. Format: 'requestType_data'
	{
		c = buffer[i];
		if(c=='_')
			break;
		reqType[i++] = c;
	}

	reqType[i] = '\0'; //End string
	requestType = atoi(reqType); //Convert request type to integer for use in switch
	
	//Handle request based on request type!
	switch(requestType)
	{
		case CREATELOCK:
			//Check if already reached max locks if no,
			clientNum=10*(inPktHdr.from-serverCount)+inMailHdr.from;
			if((createDLockIndex >= 0)&&(createDLockIndex < (MAX_DLOCK-1)))
			{
				j = 0;
				while(c!='\0') //Continue to search data string Until reach end. Want to extract lock name.
				{
					c = buffer[++i];
					if(c=='\0')
						break;
					lockName[clientNum][j++] = c; 
				}
				lockName[clientNum][j] = '\0'; //End string

				//Check if lock already exists, just send index. If not, create it.
				for(int k=0; k<createDLockIndex; k++)
				{
					if(!strcmp(locks[k]->getName(), lockName[clientNum]))
					{
						lockIndex = k;
					}
				}
				localLockIndex[clientNum]=lockIndex;
				if (serverCount > 1)
					CreateVerify(lockName[clientNum], CREATELOCKVERIFY, clientNum);
				else
				{
			
					if(localLockIndex[clientNum] == -1) //If not found here or elsewhere
					{
						for(int i=0;lockName[clientNum][i]!='\0';i++)
						{
							localLockName[i]=lockName[clientNum][i];
							localLockName[i+1]= '\0';
						}
						locks[createDLockIndex] = new DistributedLock(localLockName); 
						locks[createDLockIndex]->SetGlobalId((postOffice->GetID()*MAX_DLOCK)+createDLockIndex);
						if(locks[createDLockIndex] == NULL)
						{
						//Handle error...
							printf("Error in creation of %s!\n", lockName[clientNum]);
							errorOutPktHdr.to = clientNum/10+serverCount;
							errorInPktHdr = inPktHdr;
							errorOutMailHdr.to = (clientNum+10)%10;
							errorInMailHdr = inMailHdr;
							return false;
						}
					
					lockIndex = locks[createDLockIndex]->GetGlobalId();
					createDLockIndex++;
					}
					else
					{
						//printf("Exists here: %i\n", locks[localLockIndex[clientNum]]->GetGlobalId());				
						lockIndex=locks[localLockIndex[clientNum]]->GetGlobalId();
					}
					//Include index of lock in reply message 
					char* ack = new char [5];
					sprintf(ack, "%i", lockIndex);
					//itoa(ack,5,lockIndex); //Convert to string to send in ack
		
					// Send reply (using "reply to" mailbox
					// in the message that just arrived
					outPktHdr.to = clientNum/10+serverCount;
					outMailHdr.to = (clientNum+10)%10;
					outMailHdr.from = postOffice->GetID();
					outMailHdr.length = strlen(ack) + 1;

					printf("Server replying to client with ID: %s\n", ack);
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
				c = buffer[++i];
				if(c=='_')
					break;
				client[x++] = c;	
			}while(c!='_'); //Until find underscore, search string. Want to extract request type. Format: 'requestType_data'
			client[x]='\0';
			clientNum = atoi(client);
			//printf("client: %s\n", client);
			while(c!='\0') //Continue to search data string Until reach end. Want to extract lock name.
			{
				c = buffer[++i];
				verifyBuffer[clientNum][verifyResponses[clientNum]][a++] = c;
				if(c=='\0')
					break;
			}
			verifyResponses[clientNum]++;
			if (verifyResponses[clientNum]==serverCount-1)
			{
				for (int i=0; i< serverCount-1; i++)
				{
					if (verifyBuffer[clientNum][i][0] == '-' && verifyBuffer[clientNum][i][1] == '1')
					{
						//printf("not on other server\n");
						otherServerLockIndex = -1;
					}
					else
					{	
						otherServerLockIndex = atoi(verifyBuffer[clientNum][i]);
						//printf("on other server with index: %i and string %s\n",otherServerLockIndex,verifyBuffer[clientNum][i]);
						break;
					}
				}
				if(localLockIndex[clientNum] == -1 && otherServerLockIndex ==-1) //If not found here or elsewhere
				{
					for(int i=0;lockName[clientNum][i]!='\0';i++){
						localLockName[i]=lockName[clientNum][i];
						localLockName[i+1]= '\0';
					}
					locks[createDLockIndex] = new DistributedLock(localLockName); 
					locks[createDLockIndex]->SetGlobalId((postOffice->GetID()*MAX_DLOCK)+createDLockIndex);
					if(locks[createDLockIndex] == NULL)
					{
						//Handle error...
						printf("Error in creation of %s!\n", lockName[clientNum]);
						errorOutPktHdr.to = clientNum/10+serverCount;
						errorInPktHdr = inPktHdr;
						errorOutMailHdr.to = (clientNum+10)%10;
						errorInMailHdr = inMailHdr;
						return false;
					}
					
					lockIndex = locks[createDLockIndex]->GetGlobalId();
					createDLockIndex++;
				}
				else if (localLockIndex[clientNum] == -1 && otherServerLockIndex !=-1) //exists on other server and not here
				{
					lockIndex=otherServerLockIndex; //set lock
				}
				else if (localLockIndex[clientNum] != -1 && otherServerLockIndex==-1) //exists here
				{
					lockIndex=locks[localLockIndex[clientNum]]->GetGlobalId();
				}
				else //exists multiple places; should never happen
				{
					printf("problem: localLockIndex[clientNum] %i otherServerLockIndex %i\n", localLockIndex[clientNum],otherServerLockIndex);			
				}
				//Include index of lock in reply message 
				char* ack = new char [5];
				sprintf(ack, "%i", lockIndex);
	
				// Send reply (using "reply to" mailbox
				// in the message that just arrived
				outPktHdr.to = clientNum/10+serverCount;
				outMailHdr.to = (clientNum+10)%10;
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
			//handle incomming requests to other servers to verify lock
			printf("Verifying Lock\n");
			lockIndex=-1;
			char client[3];
			int clientNum;
			request = new char [MaxMailSize];
			x=0;
			do 
			{
				c = buffer[++i];
				if(c=='_')
					break;
				client[x++] = c;	
			}while(c!='_');

			client[x]='\0';
			clientNum = atoi(client);
			while(c!='\0') //Continue to search data string Until reach end. Want to extract lock name.
			{
				c = buffer[++i];
				if(c=='\0')
					break;
				localLockName[a++] = c; 
			}
			localLockName[a] = '\0'; //End string
			//printf("localLockName: %s\n", localLockName);
			//Check if lock exists
			for(int k=0; k<createDLockIndex; k++)
			{
				if(!strcmp(locks[k]->getName(), localLockName))
				{
					lockIndex = k;
					break;
				}
			}
			// Send reply (using "reply to" mailbox
			// in the message that just arrived
			outPktHdr.to = inPktHdr.from;
			outMailHdr.to = inMailHdr.from;
			outMailHdr.from = postOffice->GetID();
			
			if (lockIndex==-1) // not found
			{	
				sprintf(request,"%d_%d_%d",CREATELOCKANSWER, clientNum, -1);
				outMailHdr.length = strlen(request) + 1;

				success = postOffice->Send(outPktHdr, outMailHdr, request); 
			}
			else //found
			{
				sprintf(request,"%d_%d_%d",CREATELOCKANSWER, clientNum, locks[lockIndex]->GetGlobalId());
				outMailHdr.length = strlen(request) + 1;
				success = postOffice->Send(outPktHdr, outMailHdr, request); 
			}
			break;
		case CREATECV:
			//Check if already reached max cv if no,
			clientNum=10*(inPktHdr.from-serverCount)+inMailHdr.from;
			if((createDCVIndex >= 0)&&(createDCVIndex < (MAX_DCV-1)))
			{
				j = 0;
				while(c!='\0') //Continue to search data string Until reach end. Want to extract lock name.
				{
					c = buffer[++i];
					if(c=='\0')
						break;
					lockName[clientNum][j++] = c; 
				}
				lockName[clientNum][j] = '\0'; //End string

				//Check if lock already exists, just send index. If not, create it.
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
					if(localLockIndex[clientNum] == -1) //If not found here or elsewhere
					{
						//Create distributed lock
						for(int i=0;lockName[clientNum][i]!='\0';i++)
						{
							localLockName[i]=lockName[clientNum][i];
							localLockName[i+1]= '\0';
						}
						cvs[createDCVIndex] = new DistributedCV(localLockName); 
						cvs[createDCVIndex]->SetGlobalId((postOffice->GetID()*MAX_DCV)+createDCVIndex);
						if(cvs[createDCVIndex] == NULL)
						{
							//Handle error...
							errorOutPktHdr.to = clientNum/10+serverCount;
							errorInPktHdr = inPktHdr;
							errorOutMailHdr.to = (clientNum+10)%10;
							errorInMailHdr = inMailHdr;
							return false;
						}
						
						cvIndex = cvs[createDCVIndex]->GetGlobalId();
						createDCVIndex++;
					}
					else
					{
					//	printf("Exists here: %i\n", cvs[cvIndex]->GetGlobalId());				
						cvIndex=cvs[cvIndex]->GetGlobalId();
					}
					//Include index of lock in reply message 
					char* ack = new char [5];
					sprintf(ack, "%i", cvIndex);
		
					// Send reply (using "reply to" mailbox
					// in the message that just arrived
					outPktHdr.to = clientNum/10+serverCount;
					outMailHdr.to = (clientNum+10)%10;
					outMailHdr.from = postOffice->GetID();
					outMailHdr.length = strlen(ack) + 1;

					//printf("Server replying to client with ID: %s\n", ack);
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
				c = buffer[++i];
				if(c=='_')
					break;
				client[x++] = c;	
			}while(c!='_'); //Until find underscore, search string. Want to extract request type. Format: 'requestType_data'
			client[x]='\0';
			clientNum = atoi(client);
			//printf("client: %s\n", client);
			while(c!='\0') //Continue to search data string Until reach end. Want to extract lock name.
			{
				c = buffer[++i];
				verifyBuffer[clientNum][verifyResponses[clientNum]][a++] = c;
				if(c=='\0')
					break;
			}
			verifyResponses[clientNum]++;
			if (verifyResponses[clientNum]==serverCount-1)
			{
				for (int i=0; i< serverCount-1; i++)
				{
					if (verifyBuffer[clientNum][i][0] == '-' && verifyBuffer[clientNum][i][1] == '1')
					{
						//printf("not on other server\n");
						otherServerLockIndex = -1;
					}
					else
					{	
						otherServerLockIndex = atoi(verifyBuffer[clientNum][i]);
						//printf("on other server with index: %i and string %s\n",otherServerLockIndex,verifyBuffer[clientNum][i]);
						break;
					}
				}
				if(localLockIndex[clientNum] == -1 && otherServerLockIndex ==-1) //If not found here or elsewhere
				{
					//Create distributed lock
					for(int i=0;lockName[clientNum][i]!='\0';i++){
						localLockName[i]=lockName[clientNum][i];
						localLockName[i+1]= '\0';
					}
					cvs[createDCVIndex] = new DistributedCV(localLockName); 
					cvs[createDCVIndex]->SetGlobalId((postOffice->GetID()*MAX_DCV)+createDCVIndex);
					if(cvs[createDCVIndex] == NULL)
					{
						//Handle error...
						errorOutPktHdr.to = clientNum/10+serverCount;
						errorInPktHdr = inPktHdr;
						errorOutMailHdr.to = (clientNum+10)%10;
						errorInMailHdr = inMailHdr;
						return false;
					}
					
					cvIndex = cvs[createDCVIndex]->GetGlobalId();
					createDCVIndex++;
				}
				else if (localLockIndex[clientNum] == -1 && otherServerLockIndex !=-1) //exists on other server and not here
				{
					cvIndex=otherServerLockIndex; //set lock
				}
				else if (localLockIndex[clientNum] != -1 && otherServerLockIndex==-1) //exists here
				{
					cvIndex=cvs[localLockIndex[clientNum]]->GetGlobalId();
				}
				else //exists multiple places; should never happen
				{
						
				}
				//Include index of lock in reply message 
				char* ack = new char [5];
				sprintf(ack, "%i", cvIndex);
				//itoa(ack,5,lockIndex); //Convert to string to send in ack
	
				// Send reply (using "reply to" mailbox
				// in the message that just arrived
				outPktHdr.to = clientNum/10+serverCount;
				outMailHdr.to = (clientNum+10)%10;
				outMailHdr.from = postOffice->GetID();
				outMailHdr.length = strlen(ack) + 1;

				//printf("Server replying to client with ID: %s\n", ack);
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
				c = buffer[++i];
				if(c=='_')
					break;
				client[x++] = c;	
			}while(c!='_');

			client[x]='\0';
			clientNum = atoi(client);
			while(c!='\0') //Continue to search data string Until reach end. Want to extract lock name.
			{
				c = buffer[++i];
				if(c=='\0')
					break;
				localLockName[a++] = c; 
			}
			localLockName[a] = '\0'; //End string
			//printf("localLockName: %s\n", localLockName);
			//Check if cv already exists, just send index. If not, we'll send -1
			for(int j=0; j<createDCVIndex; j++)
			{
				if(!strcmp(cvs[j]->getName(), localLockName))
				{
					cvIndex = j;
				}
			}
			// Send reply (using "reply to" mailbox
			// in the message that just arrived
			outPktHdr.to = inPktHdr.from;
			outMailHdr.to = inMailHdr.from;
			outMailHdr.from = postOffice->GetID();
			if(cvIndex == -1) //If not found
			{
				sprintf(request,"%d_%d_%d",CREATECVANSWER, clientNum, -1);
				outMailHdr.length = strlen(request) + 1;
				success = postOffice->Send(outPktHdr, outMailHdr, request); 
			}
			else //found
			{
				sprintf(request,"%d_%d_%d",CREATECVANSWER, clientNum, cvs[cvIndex]->GetGlobalId());
				outMailHdr.length = strlen(request) + 1;
				success = postOffice->Send(outPktHdr, outMailHdr, request); 
			}

			break;
		case CREATEMV:
			//Check if already reached max locks if no,
			clientNum=10*(inPktHdr.from-serverCount)+inMailHdr.from;
			if((createDMVIndex >= 0)&&(createDMVIndex < (MAX_DMV-1)))
			{
				j = 0;
				while(c!='\0') //Continue to search data string Until reach end. Want to extract mv name.
				{
					c = buffer[++i];
					if(c=='\0')
						break;
					lockName[clientNum][j++] = c;  
				}
				lockName[clientNum][j] = '\0' ;  //End string

				//Check if mv already exists, just send index. If not, create it.
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
					if(localLockIndex[clientNum] == -1) //If not found here or elsewhere
					{
						//Create distributed lock
						for(int i=0;lockName[clientNum][i]!='\0';i++)
						{
							localLockName[i]=lockName[clientNum][i];
							localLockName[i+1]= '\0';
						}
						mvs[createDMVIndex] = new DistributedMV(localLockName); 
						mvs[createDMVIndex]->SetGlobalId((postOffice->GetID()*MAX_DMV)+createDMVIndex);
						if(mvs[createDMVIndex] == NULL)
						{
							errorOutPktHdr.to = clientNum/10+serverCount;
							errorInPktHdr = inPktHdr;
							errorOutMailHdr.to = (clientNum+10)%10;
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
					//Include index of lock in reply message 
					char* ack = new char [5];
					sprintf(ack, "%i", mvIndex);

					// Send reply (using "reply to" mailbox
					// in the message that just arrived
					outPktHdr.to = clientNum/10+serverCount;
					outMailHdr.to = (clientNum+10)%10;
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
				c = buffer[++i];
				if(c=='_')
					break;
				client[x++] = c;	
			}while(c!='_'); //Until find underscore, search string. Want to extract request type. Format: 'requestType_data'
			client[x]='\0';
			clientNum = atoi(client);
			
			while(c!='\0') //Continue to search data string Until reach end. Want to extract lock name.
			{
				c = buffer[++i];
				verifyBuffer[clientNum][verifyResponses[clientNum]][a++] = c;
				if(c=='\0')
					break;
			}
			verifyResponses[clientNum]++;
			if (verifyResponses[clientNum]==serverCount-1)
			{
				for (int i=0; i< serverCount-1; i++)
				{
					if (verifyBuffer[clientNum][i][0] == '-' && verifyBuffer[clientNum][i][1] == '1')
					{
						//printf("not on other server\n");
						otherServerLockIndex = -1;
					}
					else
					{	
						otherServerLockIndex = atoi(verifyBuffer[clientNum][i]);
						//printf("on other server with index: %i and string %s\n",otherServerLockIndex,verifyBuffer[clientNum][i]);
						break;
					}
				}
				if(localLockIndex[clientNum] == -1 && otherServerLockIndex ==-1) //If not found here or elsewhere
				{
					//Create distributed lock
					for(int i=0;lockName[clientNum][i]!='\0';i++){
						localLockName[i]=lockName[clientNum][i];
						localLockName[i+1]= '\0';
					}
					mvs[createDMVIndex] = new DistributedMV(localLockName); 
					mvs[createDMVIndex]->SetGlobalId((postOffice->GetID()*MAX_DMV)+createDMVIndex);
					if(mvs[createDMVIndex] == NULL)
					{
						//Handle error...
						errorOutPktHdr.to = clientNum/10+serverCount;
						errorInPktHdr = inPktHdr;
						errorOutMailHdr.to = (clientNum+10)%10;
						errorInMailHdr = inMailHdr;
						return false;
					}
					
					mvIndex = mvs[createDMVIndex]->GetGlobalId();
					createDMVIndex++;
				}
				else if (localLockIndex[clientNum] == -1 && otherServerLockIndex !=-1) //exists on other server and not here
				{
					//printf("Exists on other server with index: %i\n", otherServerLockIndex);
					mvIndex=otherServerLockIndex; //set lock
				}
				else if (localLockIndex[clientNum] != -1 && otherServerLockIndex==-1) //exists here
				{
					//printf("Exists here: %i\n", locks[localLockIndex[clientNum]]->GetGlobalId());
					
					mvIndex=mvs[localLockIndex[clientNum]]->GetGlobalId();
				}
				else //exists multiple places; should never happen
				{
				}
				//Include index of lock in reply message 
				char* ack = new char [5];
				sprintf(ack, "%i", mvIndex);
				//itoa(ack,5,lockIndex); //Convert to string to send in ack
	
				// Send reply (using "reply to" mailbox
				// in the message that just arrived
				outPktHdr.to = clientNum/10+serverCount;
				outMailHdr.to = (clientNum+10)%10;
				outMailHdr.from = postOffice->GetID();
				outMailHdr.length = strlen(ack) + 1;

				//printf("Server replying to client with ID: %s\n", ack);
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
			//handle incomming requests to other servers to verify mv
			mvIndex=-1;
			request = new char [MaxMailSize];
			x=0;
			do 
			{
				c = buffer[++i];
				if(c=='_')
					break;
				client[x++] = c;	
			}while(c!='_');

			client[x]='\0';
			clientNum = atoi(client);
			while(c!='\0') //Continue to search data string Until reach end. Want to extract lock name.
			{
				c = buffer[++i];
				if(c=='\0')
					break;
				localLockName[a++] = c; 
			}
			localLockName[a] = '\0'; //End string
			//printf("localLockName: %s\n", localLockName);
			//Check if cv already exists, just send index. If not, we'll send -1
			for(int j=0; j<createDMVIndex; j++)
			{
				if(!strcmp(mvs[j]->getName(), localLockName))
				{
					mvIndex = j;
					break;
				}
			}
			// Send reply (using "reply to" mailbox
			// in the message that just arrived
			outPktHdr.to = inPktHdr.from;
			outMailHdr.to = inMailHdr.from;
			outMailHdr.from = postOffice->GetID();
			if(mvIndex == -1) //If not found
			{
				sprintf(request,"%d_%d_%d",CREATEMVANSWER, clientNum, -1);
				outMailHdr.length = strlen(request) + 1;
				success = postOffice->Send(outPktHdr, outMailHdr, request); 
			}
			else //found
			{
				sprintf(request,"%d_%d_%d",CREATEMVANSWER, clientNum, mvs[mvIndex]->GetGlobalId());
				outMailHdr.length = strlen(request) + 1;
				success = postOffice->Send(outPktHdr, outMailHdr, request); 
			}
			
			break;
		case ACQUIRE:
			//Extract requested lock index from message
			clientNum=10*(inPktHdr.from-serverCount)+inMailHdr.from;
			m=0; 
			if(buffer[i+1] == '_')
			{
				i++;
				do 
				{
					c = buffer[++i];
					if(c=='_')
						break;
					client[m++] = c;	
				}while(c!='_');

				client[m]='\0';
				clientNum = atoi(client);
			}
			m=0;
			while(c!='\0') //Continue search of data string Until reach end of string, search string. Want to extract lock index.
			{
				c = buffer[++i];
				lockIndexBuf[m++] = c; //Actually want to copy '\0' too
			}
			lockIndex = atoi(lockIndexBuf); //Convert request type to integer for indexing array
			//printf("%d %d %s\n",ACQUIRE, clientNum, lockIndexBuf);
			if(lockIndex/MAX_DLOCK != postOffice->GetID())
			{
				sprintf(buffer,"%d__%d_%s",ACQUIRE, clientNum, lockIndexBuf);
				//printf("%d %d %s\n",ACQUIRE, clientNum, lockIndexBuf);
				outPktHdr.to = lockIndex/MAX_DLOCK;
				outMailHdr.to = outPktHdr.to;
				outMailHdr.from = inMailHdr.from;
				outMailHdr.length = strlen(buffer) + 1;
				success = postOffice->Send(outPktHdr, outMailHdr, buffer);
			}
			else
				success = Acquire(lockIndex, outPktHdr, inPktHdr, outMailHdr, inMailHdr, clientNum);
			break;
		case RELEASE:
			//Extract requested lock index from message
			clientNum=10*(inPktHdr.from-serverCount)+inMailHdr.from;
			m=0; 
			if(buffer[i+1] == '_')
			{
				i++;
				do 
				{
					c = buffer[++i];
					if(c=='_')
						break;
					client[m++] = c;	
				}while(c!='_');

				client[m]='\0';
				clientNum = atoi(client);
			}
			m=0;
			while(c!='\0') //Continue search of data string Until reach end of string, search string. Want to extract lock index.
			{
				c = buffer[++i];
				lockIndexBuf[m++] = c; //Actually want to copy '\0' too
			}
		
			lockIndex = atoi(lockIndexBuf); //Convert request type to integer for indexing array
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
			//Extract requested lock index from message
			clientNum=10*(inPktHdr.from-serverCount)+inMailHdr.from;
			m=0; 
			if(buffer[i+1] == '_')
			{
				i++;
				do 
				{
					c = buffer[++i];
					if(c=='_')
						break;
					client[m++] = c;	
				}while(c!='_');

				client[m]='\0';
				clientNum = atoi(client);
			}
			m=0;
			while(c!='\0') //Continue search of data string Until reach end of string, search string. Want to extract lock index.
			{
				c = buffer[++i];
				lockIndexBuf[m++] = c; //Actually want to copy '\0' too
			}
			lockIndex = atoi(lockIndexBuf); //Convert request type to integer for indexing array
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
			//Extract requested lock index from message
			
			clientNum=10*(inPktHdr.from-serverCount)+inMailHdr.from;
			m=0; 
			if(buffer[i+1] == '_')
			{
				i++;
				do
				{
					c = buffer[++i];
					if(c=='_')
						break;
					client[m++] = c;
				} while(c!='_');
				clientNum=atoi(client);
			}
			m=0; 
			do
			{
				c = buffer[++i];
				if(c=='_')
					break;
				mvIndexBuf[m++] = c; //Actually want to copy '\0' too
			}while(c!='_'); //Continue search of data string Until reach end of string, search string. Want to extract lock index.
			mvIndexBuf[m] = '\0';
			mvIndex = atoi(mvIndexBuf); //Convert request type to integer for indexing array
			m=0; //reset m so can reuse it	
			i++;
			while(c!='\0') //Continue search of data string Until reach end of string, search string. Want to extract lock index and set value.
			{
				c = buffer[i++]; 
				mvValBuf[m++] = c; //Actually want to copy '\0' too				
			}
			mvValue = atoi(mvValBuf); //Convert to integer for setting mv
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
			//Extract requested mv index from message
			clientNum=10*(inPktHdr.from-serverCount)+inMailHdr.from;
			m=0; 
			if(buffer[i+1] == '_')
			{
				i++;
				do 
				{
					c = buffer[++i];
					if(c=='_')
						break;
					client[m++] = c;	
				}while(c!='_');

				client[m]='\0';
				clientNum = atoi(client);
			}
			m=0;
			while(c!='\0') //Continue search of data string Until reach end of string, search string. Want to extract lock index.
			{
				c = buffer[++i];
				mvIndexBuf[m++] = c; //Actually want to copy '\0' too
			}

			mvIndex = atoi(mvIndexBuf); //Convert request type to integer for indexing array
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
			//Extract requested mv index from message
			clientNum=10*(inPktHdr.from-serverCount)+inMailHdr.from;
			m=0; 
			if(buffer[i+1] == '_')
			{
				i++;
				do 
				{
					c = buffer[++i];
					if(c=='_')
						break;
					client[m++] = c;	
				}while(c!='_');

				client[m]='\0';
				clientNum = atoi(client);
			}
			m=0;
			while(c!='\0') //Continue search of data string Until reach end of string, search string. Want to extract lock index.
			{
				c = buffer[++i];
				mvIndexBuf[m++] = c; //Actually want to copy '\0' too
			}

			mvIndex = atoi(mvIndexBuf); //Convert request type to integer for indexing array
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
			clientNum=10*(inPktHdr.from-serverCount)+inMailHdr.from;
			m=0; 
			if(buffer[i+1] == '_')
			{
				i++;
				do 
				{
					c = buffer[++i];
					if(c=='_')
						break;
					client[m++] = c;	
				}while(c!='_');

				client[m]='\0';
				clientNum = atoi(client);
			}
			cvNumIndex=0;
			do {
				c = buffer[++i];
				if(c=='_')
						break;
				cvNum[cvNumIndex ++] = c;
			}while ( c!= '_');
			cvNum[cvNumIndex]='\0';
			cvIndex = atoi(cvNum); //Convert request type to integer for indexing array

			while ( c!= '\0'){
				c = buffer[++i];
				lockNum[lockNumIndex ++] = c;
			}

			lockIndex = atoi(lockNum); //Convert request type to integer for indexing array
			lockNumIndex = 0;
			cvNumIndex = 0;
			//printf("lockNum: %s; cvNum: %s\n", lockNum, cvNum);
			if(lockIndex/MAX_DLOCK != postOffice->GetID())//forward to server holding lock
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
			clientNum=10*(inPktHdr.from-serverCount)+inMailHdr.from;
			m=0; 
			if(buffer[i+1] == '_')
			{
				i++;
				do 
				{
					c = buffer[++i];
					if(c=='_')
						break;
					client[m++] = c;	
				}while(c!='_');

				client[m]='\0';
				clientNum = atoi(client);
			}
			cvNumIndex=0;
			do {
				c = buffer[++i];
				if(c=='_')
						break;
				cvNum[cvNumIndex ++] = c;
			}while ( c!= '_');
			cvNum[cvNumIndex]='\0';
			cvIndex = atoi(cvNum); //Convert request type to integer for indexing array

			while ( c!= '\0'){
				c = buffer[++i];
				lockNum[lockNumIndex ++] = c;
			}

			lockIndex = atoi(lockNum); //Convert request type to integer for indexing array
			lockNumIndex = 0;
			cvNumIndex = 0;
			if(cvs[cvIndex%MAX_DCV] == NULL)
			{
				sprintf(buffer,"%d_%d_%d",WAITRESPONSE, client, -1);
				
				outPktHdr.to = clientNum/10+serverCount;
				outMailHdr.to = (clientNum+10)%10;
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
				
				//add to wait Q
				outPktHdr.to = clientNum/10+serverCount;
				outMailHdr.to = (clientNum+10)%10;
				outMailHdr.from = postOffice->GetID();
				outMailHdr.length = strlen(ack) + 1;
				
				Message* reply = new Message(outPktHdr, outMailHdr, ack);
				cvs[cvIndex%MAX_DCV]->QueueReply(reply);

				sprintf(buffer,"%d_%d_%d",WAITRESPONSE, client, cvIndex);
				outPktHdr.to = clientNum/10+serverCount;
				outMailHdr.to = (clientNum+10)%10;
				outMailHdr.from = postOffice->GetID();
				outMailHdr.length = strlen(buffer) + 1;
				success = postOffice->Send(outPktHdr, outMailHdr, buffer);
			}

			break;
		case WAITRESPONSE:
			clientNum=10*(inPktHdr.from-serverCount)+inMailHdr.from;
			m=0; 
			if(buffer[i+1] == '_')
			{
				i++;
				do 
				{
					c = buffer[++i];
					if(c=='_')
						break;
					client[m++] = c;	
				}while(c!='_');

				client[m]='\0';
				clientNum = atoi(client);
			}
			m=0;
			while(c!='\0') //Continue search of data string Until reach end of string, search string. Want to extract lock index.
			{
				c = buffer[++i];
				cvNum[m++] = c; //Actually want to copy '\0' too
			}
			cvIndex = atoi(cvNum); //Convert request type to integer for indexing array
			if (cvIndex == -1) //cv didn't exist on other server, wait fails
			{
				printf("Wait Failed: CV does not exist at index %d.\n", cvIndex);
				errorOutPktHdr = outPktHdr;
				errorOutPktHdr.to = clientNum/10+serverCount;
				errorInPktHdr = inPktHdr;
				errorOutMailHdr = outMailHdr;
				errorOutMailHdr.to = (clientNum+10)%10;
				errorInMailHdr = inMailHdr;
				success = false;
			}
			else
			{
				lockIndex = waitingForWaitLocks[clientNum];
				locks[lockIndex]->setOwner(-1, -1); //Reset distributed lock owner 
				locks[lockIndex]->setLockState(true); //Set distributed lock to available. 
				if(!locks[lockIndex]->isQueueEmpty()) //if someone was waiting, make them owner and give them their reply msg
				{
					Message* reply = locks[lockIndex]->RemoveReply(); //get reply from lock's waitQ
					locks[lockIndex]->setOwner(reply->pktHdr.to, reply->mailHdr.to); //Set distributed lock owner 
					locks[lockIndex]->setLockState(false); //Set distributed lock to busy.
					success = postOffice->Send(reply->pktHdr, reply->mailHdr, reply->data); //Let new owner know they have acquired the lock

					if ( !success ) 
					{
					  interrupt->Halt();
					}
				}
			}
			break;
			
		case SIGNAL:
			clientNum=10*(inPktHdr.from-serverCount)+inMailHdr.from;
			m=0; 
			if(buffer[i+1] == '_')
			{
				i++;
				do 
				{
					c = buffer[++i];
					if(c=='_')
						break;
					client[m++] = c;	
				}while(c!='_');

				client[m]='\0';
				clientNum = atoi(client);
			}
			cvNumIndex=0;
			do {
				c = buffer[++i];
				if(c=='_')
					break;
				cvNum[cvNumIndex ++] = c;
			}while ( c!= '_');
			cvNum[cvNumIndex]='\0';
			cvIndex = atoi(cvNum); //Convert request type to integer for indexing array

			while ( c!= '\0'){
				c = buffer[++i];
				lockNum[lockNumIndex ++] = c;
			}

			lockIndex = atoi(lockNum); //Convert request type to integer for indexing array
			lockNumIndex = 0;
			cvNumIndex = 0;
			
			if(cvIndex/MAX_DCV != postOffice->GetID())//forward to server holdign cv
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
			
		case SIGNALLOCK://signal call in which the server has a lock and is signaling the cv associated with it on another server
			clientNum=10*(inPktHdr.from-serverCount)+inMailHdr.from;
			m=0; 
			if(buffer[i+1] == '_')
			{
				i++;
				do 
				{
					c = buffer[++i];
					if(c=='_')
						break;
					client[m++] = c;	
				}while(c!='_');

				client[m]='\0';
				clientNum = atoi(client);
			}
			m=0;
			while(c!='\0') //Continue search of data string Until reach end of string, search string. Want to extract lock index.
			{
				c = buffer[++i];
				lockIndexBuf[m++] = c; //Actually want to copy '\0' too
			}
			lockIndex = atoi(lockIndexBuf); //Convert request type to integer for indexing array
			if(locks[lockIndex%MAX_DLOCK] == NULL)//If cv not found
			{
				sprintf(buffer,"%d_%d_%d",SIGNALRESPONSE, client, -1);
				
				outPktHdr.to = clientNum/10+serverCount;
				outMailHdr.to = (clientNum+10)%10;
				outMailHdr.from = postOffice->GetID();
				outMailHdr.length = strlen(buffer) + 1;

				success = postOffice->Send(outPktHdr, outMailHdr, buffer); 
			}
			else //found
			{
				sprintf(buffer,"%d_%d_%d",SIGNALRESPONSE, client, lockIndex);
				outPktHdr.to = clientNum/10+serverCount;
				outMailHdr.to = (clientNum+10)%10;
				outMailHdr.from = postOffice->GetID();
				outMailHdr.length = strlen(buffer) + 1;

				success = postOffice->Send(outPktHdr, outMailHdr, buffer); 
			}
			break;
		case SIGNALRESPONSE:
			clientNum=10*(inPktHdr.from-serverCount)+inMailHdr.from;
			m=0; 
			if(buffer[i+1] == '_')
			{
				i++;
				do 
				{
					c = buffer[++i];
					if(c=='_')
						break;
					client[m++] = c;	
				}while(c!='_');

				client[m]='\0';
				clientNum = atoi(client);
			}
			m=0;
			while(c!='\0') //Continue search of data string Until reach end of string, search string. Want to extract lock index.
			{
				c = buffer[++i];
				lockIndexBuf[m++] = c; //Actually want to copy '\0' too
			}
			lockIndex = atoi(lockIndexBuf); //Convert request type to integer for indexing array
			if (lockIndex == -1) //lock didn't exist on other server, signal fails
			{
				errorOutPktHdr = outPktHdr;
				errorOutPktHdr.to = clientNum/10+serverCount;
				errorInPktHdr = inPktHdr;
				errorOutMailHdr = outMailHdr;
				errorOutMailHdr.to = (clientNum+10)%10;
				errorInMailHdr = inMailHdr;
				success = false;
			}
			else
			{
				cvIndex=waitingForSignalCvs[clientNum];
				char* ack = new char [5];
				itoa(ack,5,cvIndex);

				// Send acknowledgement (using "reply to" mailbox
				// in the message that just arrived
				outPktHdr.to = clientNum/10+serverCount;
				outMailHdr.to = (clientNum+10)%10;
				outMailHdr.from = postOffice->GetID();
				outMailHdr.length = strlen(ack) + 1;

				success = postOffice->Send(outPktHdr, outMailHdr, ack); 
				
				if ( !success ) 
				{
				  interrupt->Halt();
				}
				
				//if wait Q is not Empty
				if (!(cvs[cvIndex]->isQueueEmpty())){
					if (cvs[cvIndex]->getFirstLock() != lockIndex)
					{ //Check if it's the same lock
						errorOutPktHdr = outPktHdr;
						errorOutPktHdr.to = clientNum/10+serverCount;
						errorInPktHdr = inPktHdr;
						errorOutMailHdr = outMailHdr;
						errorOutMailHdr.to = (clientNum+10)%10;
						errorInMailHdr = inMailHdr;
						return false;
					}
					else
					{
						Message* reply = cvs[cvIndex]->RemoveReply(); //get reply from lock's waitQ
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
			clientNum=10*(inPktHdr.from-serverCount)+inMailHdr.from;
			m=0; 
			if(buffer[i+1] == '_')
			{
				i++;
				do 
				{
					c = buffer[++i];
					if(c=='_')
						break;
					client[m++] = c;	
				}while(c!='_');

				client[m]='\0';
				clientNum = atoi(client);
			}
			cvNumIndex=0;
			do {
				c = buffer[++i];
				if(c=='_')
						break;
				cvNum[cvNumIndex ++] = c;
			}while ( c!= '_');
			cvNum[cvNumIndex]='\0';
			cvIndex = atoi(cvNum); //Convert request type to integer for indexing array

			while ( c!= '\0'){
				c = buffer[++i];
				lockNum[lockNumIndex ++] = c;
			}

			lockIndex = atoi(lockNum); //Convert request type to integer for indexing array
			lockNumIndex = 0;
			cvNumIndex = 0;
			
			if(cvIndex/MAX_DCV != postOffice->GetID())//forward to server holdign cv
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
			clientNum=10*(inPktHdr.from-serverCount)+inMailHdr.from;
			m=0; 
			if(buffer[i+1] == '_')
			{
				i++;
				do 
				{
					c = buffer[++i];
					if(c=='_')
						break;
					client[m++] = c;	
				}while(c!='_');

				client[m]='\0';
				clientNum = atoi(client);
			}
			m=0;
			while(c!='\0') //Continue search of data string Until reach end of string, search string. Want to extract lock index.
			{
				c = buffer[++i];
				lockIndexBuf[m++] = c; //Actually want to copy '\0' too
			}
			lockIndex = atoi(lockIndexBuf); //Convert request type to integer for indexing array
			if(locks[lockIndex%MAX_DLOCK] == NULL)//If cv not found
			{
				sprintf(buffer,"%d_%d_%d",BROADCASTRESPONSE, client, -1);
	
				// Send reply (using "reply to" mailbox
				// in the message that just arrived
				outPktHdr.to = clientNum/10+serverCount;
				outMailHdr.to = (clientNum+10)%10;
				outMailHdr.from = postOffice->GetID();
				outMailHdr.length = strlen(buffer) + 1;

				success = postOffice->Send(outPktHdr, outMailHdr, buffer); 
			}
			else if (locks[lockIndex]->getOwnerMailID() == -1)
			{//check if lock acquired
				sprintf(buffer,"%d_%d_%d",BROADCASTRESPONSE, client, -1);
	
				// Send reply (using "reply to" mailbox
				// in the message that just arrived
				outPktHdr.to = clientNum/10+serverCount;
				outMailHdr.to = (clientNum+10)%10;
				outMailHdr.from = postOffice->GetID();
				outMailHdr.length = strlen(buffer) + 1;

				success = postOffice->Send(outPktHdr, outMailHdr, buffer);
			}
			else //found
			{
				sprintf(buffer,"%d_%d_%d",BROADCASTRESPONSE, client, lockIndex);
				outPktHdr.to = clientNum/10+serverCount;
				outMailHdr.to = (clientNum+10)%10;
				outMailHdr.from = postOffice->GetID();
				outMailHdr.length = strlen(buffer) + 1;

				success = postOffice->Send(outPktHdr, outMailHdr, buffer); 
			}
			break;
		case BROADCASTRESPONSE:
			clientNum=10*(inPktHdr.from-serverCount)+inMailHdr.from;
			m=0; 
			if(buffer[i+1] == '_')
			{
				i++;
				do 
				{
					c = buffer[++i];
					if(c=='_')
						break;
					client[m++] = c;	
				}while(c!='_');

				client[m]='\0';
				clientNum = atoi(client);
			}
			m=0;
			while(c!='\0') //Continue search of data string Until reach end of string, search string. Want to extract lock index.
			{
				c = buffer[++i];
				lockIndexBuf[m++] = c; //Actually want to copy '\0' too
			}
			lockIndex = atoi(lockIndexBuf); //Convert request type to integer for indexing array
			if (lockIndex == -1) //lock didn't exist on other server, signal fails
			{
				errorOutPktHdr = outPktHdr;
				errorOutPktHdr.to = clientNum/10+serverCount;
				errorInPktHdr = inPktHdr;
				errorOutMailHdr = outMailHdr;
				errorOutMailHdr.to = (clientNum+10)%10;
				errorInMailHdr = inMailHdr;
				success = false;
			}
			else
			{
				cvIndex=waitingForBroadcastCvs[clientNum];
				lockIndex=lockIndex%MAX_DLOCK;
				char* ack = new char [5];
				itoa(ack,5,cvIndex);

				// Send acknowledgement (using "reply to" mailbox
				// in the message that just arrived
				outPktHdr.to = clientNum/10+serverCount;
				outMailHdr.to = (clientNum+10)%10;
				outMailHdr.from = postOffice->GetID();
				outMailHdr.length = strlen(ack) + 1;

				printf("Server Sending: Broadcast Ack!!!\n");
				success = postOffice->Send(outPktHdr, outMailHdr, ack); 
				
				if ( !success ) 
				{
				  interrupt->Halt();
				}
				fflush(stdout);
				
				//if wait Q is not Empty
				if (!(cvs[cvIndex]->isQueueEmpty())){
					if (cvs[cvIndex]->getFirstLock() != lockIndex)
					{ //Check if it's the same lock
						errorOutPktHdr = outPktHdr;
						errorOutPktHdr.to = clientNum/10+serverCount;
						errorInPktHdr = inPktHdr;
						errorOutMailHdr = outMailHdr;
						errorOutMailHdr.to = (clientNum+10)%10;
						errorInMailHdr = inMailHdr;
						return false;
					}
					else
					{
						Message* reply = cvs[cvIndex]->RemoveReply(); //get reply from lock's waitQ
						reply->pktHdr.from = reply->pktHdr.to;
						reply->mailHdr.from = reply->mailHdr.to;
						bool success2 = Acquire(lockIndex, outPktHdr, reply->pktHdr,outMailHdr, reply->mailHdr, -2);
						
						if ( !success2 ) {
						  printf("The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
						  interrupt->Halt();
						}
						fflush(stdout);
					}
				}
				while (!(cvs[cvIndex]->isQueueEmpty()))
				{
					Message* reply = cvs[cvIndex]->RemoveReply(); //get reply from lock's waitQ
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
			clientNum=10*(inPktHdr.from-serverCount)+inMailHdr.from;
			m=0; 
			if(buffer[i+1] == '_')
			{
				i++;
				do 
				{
					c = buffer[++i];
					if(c=='_')
						break;
					client[m++] = c;	
				}while(c!='_');

				client[m]='\0';
				clientNum = atoi(client);
			}
			cvNumIndex=0;
			while ( c!= '\0'){
				c = buffer[++i];
				cvNum[cvNumIndex ++] = c;
			}
			cvIndex = atoi(cvNum); //Convert request type to integer for indexing array
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
				//check range
				if ((cvIndex < 0)||(cvIndex > serverCount*MAX_DCV-1))
				{
					errorOutPktHdr = outPktHdr;
					errorOutPktHdr.to = clientNum/10+serverCount;
					errorInPktHdr = inPktHdr;
					errorOutMailHdr = outMailHdr;
					errorOutMailHdr.to = (clientNum+10)%10;
					errorInMailHdr = inMailHdr;
					return false;
				}
				else if(cvIndex/MAX_DCV== postOffice->GetID()){
					//check if cv exists
					cvIndex=cvIndex%MAX_DCV;
					if (cvs[cvIndex] != NULL){

						delete cvs[cvIndex];
						cvs[cvIndex] = NULL;
						createDCVIndex --;
						//Include index of CV in reply message 
						char* ack = new char [4];
						itoa(ack,4,cvIndex); //Convert to string to send in ack
			
						// Send reply (using "reply to" mailbox
						// in the message that just arrived
						outPktHdr.to = clientNum/10+serverCount;
						outMailHdr.to = (clientNum+10)%10;
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
						errorOutPktHdr.to = clientNum/10+serverCount;
						errorInPktHdr = inPktHdr;
						errorOutMailHdr = outMailHdr;
						errorOutMailHdr.to = (clientNum+10)%10;
						errorInMailHdr = inMailHdr;
						return false;
					}
				}
				else
				{
					outPktHdr.to = cvIndex/MAX_DCV;
					outMailHdr.to = outPktHdr.to;
					outMailHdr.from = inMailHdr.from;
					outMailHdr.length = strlen(buffer) + 1;
					success = postOffice->Send(outPktHdr, outMailHdr, buffer);
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
		clientNum=10*(inPktHdr.from-serverCount)+inMailHdr.from;
	
	//Check if lock exists
	if((lockIndex < 0)||(lockIndex > serverCount*(MAX_DLOCK)-1))
	{
		printf("Cannot acquire: Lock index [%d] out of bounds [%d, %d].\n", lockIndex, 0, (MAX_DLOCK-1));
		errorOutPktHdr = outPktHdr;
		errorOutPktHdr.to = clientNum/10+serverCount;
		errorInPktHdr = inPktHdr;
		errorOutMailHdr = outMailHdr;
		errorOutMailHdr.to = (clientNum+10)%10;
		errorInMailHdr = inMailHdr;
		return false;
	}
	else if(lockIndex/MAX_DLOCK == postOffice->GetID())
	{
		lockIndex=lockIndex%MAX_DLOCK;
		if(locks[lockIndex] == NULL) //If not found
		{
			printf("Cannot acquire: Lock does not exist at index %d.\n", lockIndex);
			return false;
		}
		else
		{
			//Create reply. Send back index of acquired lock for verification.
			char * ack = new char [5];
			itoa(ack,5,locks[lockIndex]->GetGlobalId()); //Convert to string to send in ack
			printf("Machine %d has acquired %s at index %d\n", clientNum/10+serverCount, locks[lockIndex]->getName(), lockIndex);
			//Check if already acquired/busy
			
			if(!locks[lockIndex]->getLockState()) //if busy, create message for reply and add to lock's waitqueue
			{
				printf("Machine %d being added to waitQueue for acquiring %s!\n", clientNum/10+serverCount, locks[lockIndex]->getName());
				
				outPktHdr.to = clientNum/10+serverCount;
				outMailHdr.to = (clientNum+10)%10;
				outMailHdr.from = postOffice->GetID();
				outMailHdr.length = strlen(ack) + 1;
				
				Message* reply = new Message(outPktHdr, outMailHdr, ack);
				locks[lockIndex]->QueueReply(reply);
				success=true;
			}
			else //else, set lock owner and send reply to notify that acquire occured
			{
				locks[lockIndex]->setOwner(clientNum/10+serverCount, (clientNum+10)%10); //Set distributed lock owner to whoever requested acquire
				locks[lockIndex]->setLockState(false); //Set distributed lock to busy. Gives me error when try to just put 'BUSY'
				
				// Send acknowledgement (using "reply to" mailbox
				// in the message that just arrived
				
				outPktHdr.to = clientNum/10+serverCount;
				outMailHdr.to = (clientNum+10)%10;
				outMailHdr.from = postOffice->GetID();
				outMailHdr.length = strlen(ack) + 1;
				
				printf("Server Sending: Lock %d Acquired!!!\n",locks[lockIndex]->GetGlobalId());
				success = postOffice->Send(outPktHdr, outMailHdr, ack); 

				if ( !success ) {
				  printf("The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
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
	//client = client - serverCount; //to convert to array index [0,MAX_CLIENTS]
	printf("In CreateVerify.\n");
	char* request = new char [MaxMailSize];
	sprintf(request,"%d_%d_%s",requestType, client, data);
	
	PacketHeader outPktHdr;
	//PacketHeader inPktHdr[serverCount-1];
	MailHeader outMailHdr;
	//MailHeader inMailHdr[serverCount-1];
	//char verifyBuffer[serverCount-1][MaxMailSize];

	outMailHdr.from = postOffice->GetID();
	//printf("outMailHdr.from: %i\n", outMailHdr.from);
	outMailHdr.length = strlen(request) + 1; 
	for (int i = 0; i< serverCount; i++)
	{
		if (i != outMailHdr.from)	
		{
			outPktHdr.to = i;		
		 	outMailHdr.to = i;
			bool success = postOffice->Send(outPktHdr, outMailHdr, request); 

			 if ( !success ) {
 				 printf("The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
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
	//Check if lock exists
	if((lockIndex < 0)||(lockIndex > serverCount*(MAX_DLOCK)-1))
	{
		printf("Cannot release: Lock index [%d] out of bounds [%d, %d].\n", lockIndex, 0, serverCount*(MAX_DLOCK)-1);
		errorOutPktHdr = outPktHdr;
		errorOutPktHdr.to = clientNum/10+serverCount;
		errorInPktHdr = inPktHdr;
		errorOutMailHdr = outMailHdr;
		errorOutMailHdr.to = (clientNum+10)%10;
		errorInMailHdr = inMailHdr;
		return false;
	}
	else if(lockIndex/MAX_DLOCK == postOffice->GetID())
	{
		lockIndex=lockIndex%MAX_DLOCK;
		if(locks[lockIndex] == NULL) //If not found
		{
			printf("Cannot release: Lock does not exist at index %d.\n", lockIndex);
			errorOutPktHdr = outPktHdr;
			errorOutPktHdr.to = clientNum/10+serverCount;
			errorInPktHdr = inPktHdr;
			errorOutMailHdr = outMailHdr;
			errorOutMailHdr.to = (clientNum+10)%10;
			errorInMailHdr = inMailHdr;
			return false;
		}

		//Create reply. Send back index of released lock for verification.
		char * ack = new char [5];
		itoa(ack,5,locks[lockIndex]->GetGlobalId()); //Convert to string to send in ack
		printf("Machine %d has released %s at index %d\n", clientNum/10+serverCount, locks[lockIndex]->getName(), locks[lockIndex]->GetGlobalId());

		//send reply to notify that release occured
		
		locks[lockIndex]->setOwner(-1, -1); //Reset distributed lock owner 
		locks[lockIndex]->setLockState(true); //Set distributed lock to available. 
		
		// Send acknowledgement (using "reply to" mailbox
		// in the message that just arrived
		
		outPktHdr.to = clientNum/10+serverCount;
		outMailHdr.to = (clientNum+10)%10;
		outMailHdr.from = postOffice->GetID();
		outMailHdr.length = strlen(ack) + 1;

		printf("Server Sending: Lock %d Released!!!\n", locks[lockIndex]->GetGlobalId());
		success = postOffice->Send(outPktHdr, outMailHdr, ack); 

		if ( !success ) {
		  printf("The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
		  interrupt->Halt();
		}

		if(!locks[lockIndex]->isQueueEmpty()) //if someone was waiting, make them owner and give them their reply msg
		{
			Message* reply = locks[lockIndex]->RemoveReply(); //get reply from lock's waitQ
			locks[lockIndex]->setOwner(reply->pktHdr.to, reply->mailHdr.to); //Set distributed lock owner 
			locks[lockIndex]->setLockState(false); //Set distributed lock to busy.
			success = postOffice->Send(reply->pktHdr, reply->mailHdr, reply->data); //Let new owner know they have acquired the lock

			if ( !success ) {
			  printf("The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
			  interrupt->Halt();
			}
		}
		fflush(stdout);
		return success;
	}
	else
	{
		//problem; shouldn't have gotten past switch statement w/o being forwarded
		printf("problem; shouldn't have gotten past switch statement w/o being forwarded\n");
	}
}

bool DestroyLock(int lockIndex, PacketHeader outPktHdr, PacketHeader inPktHdr, MailHeader outMailHdr, MailHeader inMailHdr, int clientNum)
{
	bool success = false;
	//Check if lock exists
	if((lockIndex < 0)||(lockIndex > serverCount*(MAX_DLOCK)-1))
	{
		printf("Cannot destroy: Lock index [%d] out of bounds [%d, %d].\n", lockIndex, 0, (MAX_DLOCK-1));
		errorOutPktHdr = outPktHdr;
		errorOutPktHdr.to = clientNum/10+serverCount;
		errorInPktHdr = inPktHdr;
		errorOutMailHdr = outMailHdr;
		errorOutMailHdr.to = (clientNum+10)%10;
		errorInMailHdr = inMailHdr;
		return false;
	}
	else if(lockIndex/MAX_DLOCK == postOffice->GetID())
	{
		lockIndex=lockIndex%MAX_DLOCK;
		if(locks[lockIndex] == NULL) //If not found
		{
			printf("Cannot destroy: Lock does not exist at index %d.\n", lockIndex);
			errorOutPktHdr = outPktHdr;
			errorOutPktHdr.to = clientNum/10+serverCount;
			errorInPktHdr = inPktHdr;
			errorOutMailHdr = outMailHdr;
			errorOutMailHdr.to = (clientNum+10)%10;
			errorInMailHdr = inMailHdr;
			return false;
		}

		//Create reply. Send back index of destroyed lock for verification.
		char * ack = new char [5];
		itoa(ack,5,locks[lockIndex]->GetGlobalId()); //Convert to string to send in ack

		delete locks[lockIndex];
		locks[lockIndex] = NULL;
		createDLockIndex --;
		if(locks[lockIndex] == NULL)
		{
			printf("Machine %d has destroyed lock at index %d\n", postOffice->GetID(),  postOffice->GetID()*MAX_DLOCK+lockIndex);
		}

		// Send acknowledgement (using "reply to" mailbox
		// in the message that just arrived
		outPktHdr.to = clientNum/10+serverCount;
		outMailHdr.to = (clientNum+10)%10;
		outMailHdr.from = postOffice->GetID();
		outMailHdr.length = strlen(ack) + 1;

		printf("Sending!!!\n");
		success = postOffice->Send(outPktHdr, outMailHdr, ack); 

		if ( !success ) {
		  printf("The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
		  interrupt->Halt();
		}

		fflush(stdout);
		return success;
	}
	else
	{
		//problem; shouldn't have gotten past switch statement w/o being forwarded
		printf("problem; shouldn't have gotten past switch statement w/o being forwarded\n");
	}
}

bool SetMV(int mvIndex, int value, PacketHeader outPktHdr, PacketHeader inPktHdr, MailHeader outMailHdr, MailHeader inMailHdr, int clientNum)
{
	bool success = false;
	//Check if lock exists
	if((mvIndex < 0)||(mvIndex > serverCount*MAX_DMV-1))
	{
		printf("Cannot set: MV index [%d] out of bounds [%d, %d].\n", mvIndex, 0, (MAX_DMV-1));
		errorOutPktHdr = outPktHdr;
		errorOutPktHdr.to = clientNum/10+serverCount;
		errorInPktHdr = inPktHdr;
		errorOutMailHdr = outMailHdr;
		errorOutMailHdr.to = (clientNum+10)%10;
		errorInMailHdr = inMailHdr;
		return false;
	}
	else if (mvIndex/MAX_DMV == postOffice->GetID())
	{
		mvIndex=mvIndex%MAX_DMV;
		if(mvs[mvIndex] == NULL) //If not found
		{
			printf("Cannot set: MV does not exist at index %d.\n", mvIndex);
			errorOutPktHdr = outPktHdr;
			errorOutPktHdr.to = clientNum/10+serverCount;
			errorInPktHdr = inPktHdr;
			errorOutMailHdr = outMailHdr;
			errorOutMailHdr.to = (clientNum+10)%10;
			errorInMailHdr = inMailHdr;
			return false;
		}

		//Create reply
		char * ack = new char [32];
		sprintf(ack, "Machine %d has set MV[%d] = %d!", postOffice->GetID(), mvs[mvIndex]->GetGlobalId(), value);

		//Set value of MV owner and send reply to notify that set occured
		mvs[mvIndex]->setValue(value); //Set mv to requested value		
			
		// Send acknowledgement (using "reply to" mailbox
		// in the message that just arrived
		outPktHdr.to = clientNum/10+serverCount;
		outMailHdr.to = (clientNum+10)%10;
		outMailHdr.from = postOffice->GetID();
		outMailHdr.length = strlen(ack) + 1;

		printf("Sending!!!\n");
		success = postOffice->Send(outPktHdr, outMailHdr, ack); 

		if ( !success ) {
		  printf("The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
		  interrupt->Halt();
		}

		fflush(stdout);
		return success;
	}
	else //forward situation
	{
		//problem; shouldn't have gotten past switch statement w/o being forwarded
		printf("problem; shouldn't have gotten past switch statement w/o being forwarded\n");
	}
}

bool GetMV(int mvIndex, PacketHeader outPktHdr, PacketHeader inPktHdr, MailHeader outMailHdr, MailHeader inMailHdr, int clientNum)
{
	bool success = false;
	char* ack = new char [4];
	//Check if mv exists
	if((mvIndex < 0)||(mvIndex > serverCount*MAX_DMV-1))
	{
		printf("Cannot get: MV index [%d] out of bounds [%d, %d].\n", mvIndex, 0, (MAX_DMV-1));
		itoa(ack,4,999); //Convert error code to string to send in ack
		return SendReply(outPktHdr, inPktHdr, outMailHdr, inMailHdr, ack);
	}
	else if (mvIndex/MAX_DMV == postOffice->GetID())
	{
		mvIndex=mvIndex%MAX_DMV;
		if(mvs[mvIndex] == NULL) //If not found
		{
			printf("Cannot get: MV does not exist at index %d.\n", mvIndex);
			itoa(ack,4,999); //Convert error code to string to send in ack
			//printf("errorstr %s\n", ack);
			outPktHdr.to = clientNum/10+serverCount;
			outMailHdr.to = (clientNum+10)%10;
			outMailHdr.from = postOffice->GetID();
			outMailHdr.length = strlen(ack) + 1;
			return SendReply(outPktHdr, inPktHdr, outMailHdr, inMailHdr, ack);
		}

		//Create reply
		//Include value of mv in reply message 
		outPktHdr.to = clientNum/10+serverCount;
		outMailHdr.to = (clientNum+10)%10;
		outMailHdr.from = postOffice->GetID();
		outMailHdr.length = strlen(ack) + 1;
		
		itoa(ack,4,mvs[mvIndex]->getValue()); //Convert to string to send in ack

		// Send acknowledgement (using "reply to" mailbox
		// in the message that just arrived
		return SendReply(outPktHdr, inPktHdr, outMailHdr, inMailHdr, ack);
	}
	else
	{
		//problem; shouldn't have gotten past switch statement w/o being forwarded
		printf("problem; shouldn't have gotten past switch statement w/o being forwarded\n");
	}
}

bool DestroyMV(int mvIndex, PacketHeader outPktHdr, PacketHeader inPktHdr, MailHeader outMailHdr, MailHeader inMailHdr, int clientNum)
{
	bool success = false;
	//Check if mv exists
	if((mvIndex < 0)||(mvIndex > serverCount*MAX_DMV-1))
	{
		printf("Cannot get: MV index [%d] out of bounds [%d, %d].\n", mvIndex, 0, (MAX_DMV-1));
		errorOutPktHdr = outPktHdr;
		errorOutPktHdr.to = clientNum/10+serverCount;
		errorInPktHdr = inPktHdr;
		errorOutMailHdr = outMailHdr;
		errorOutMailHdr.to = (clientNum+10)%10;
		errorInMailHdr = inMailHdr;
		return false;
	}
	else if (mvIndex/MAX_DMV == postOffice->GetID())
	{
		mvIndex=mvIndex%MAX_DMV;
		if(mvs[mvIndex] == NULL) //If not found
		{
			printf("Cannot get: MV does not exist at index %d.\n", mvIndex);
			errorOutPktHdr = outPktHdr;
			errorOutPktHdr.to = clientNum/10+serverCount;
			errorInPktHdr = inPktHdr;
			errorOutMailHdr = outMailHdr;
			errorOutMailHdr.to = (clientNum+10)%10;
			errorInMailHdr = inMailHdr;
			return false;
		}

		//Create reply. Send back index of destroyed lock for verification.
		char * ack = new char [5];
		itoa(ack,5,mvs[mvIndex]->GetGlobalId()); //Convert to string to send in ack

		delete mvs[mvIndex];
		mvs[mvIndex] = NULL;
		createDMVIndex --;
		if(mvs[mvIndex] == NULL)
		{
			printf("Machine %d has destroyed mv at index %d\n", postOffice->GetID(), mvIndex+MAX_DMV*postOffice->GetID());
		}

		// Send acknowledgement (using "reply to" mailbox
		// in the message that just arrived
		outPktHdr.to = clientNum/10+serverCount;
		outMailHdr.to = (clientNum+10)%10;
		outMailHdr.from = postOffice->GetID();
		outMailHdr.length = strlen(ack) + 1;

		printf("Sending!!!\n");
		success = postOffice->Send(outPktHdr, outMailHdr, ack); 

		if ( !success ) {
		  printf("The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
		  interrupt->Halt();
		}

		fflush(stdout);
		return success;
	}
	else
	{
		//problem; shouldn't have gotten past switch statement w/o being forwarded
		printf("problem; shouldn't have gotten past switch statement w/o being forwarded\n");
	}
}

bool Wait(int cvIndex, int lockIndex, PacketHeader outPktHdr, PacketHeader inPktHdr, MailHeader outMailHdr, MailHeader inMailHdr, int clientNum){
	bool success;
	//Check if numbers are in range
	if((lockIndex < 0)||(lockIndex > (MAX_DLOCK-1)))
	{
		printf("Wait Failed: Lock index [%d] out of bounds [%d, %d].\n", lockIndex, 0, (MAX_DLOCK-1));
		errorOutPktHdr = outPktHdr;
		errorOutPktHdr.to = clientNum/10+serverCount;
		errorInPktHdr = inPktHdr;
		errorOutMailHdr = outMailHdr;
		errorOutMailHdr.to = (clientNum+10)%10;
		errorInMailHdr = inMailHdr;
		return false;
	}
	else if ((cvIndex < 0)||(cvIndex > (MAX_DCV-1))){ // check if cv is in correct range
		printf("Wait Failed: CV index [%d] out of bounds [%d, %d].\n", cvIndex, 0, (MAX_DCV-1));
		errorOutPktHdr = outPktHdr;
		errorOutPktHdr.to = clientNum/10+serverCount;
		errorInPktHdr = inPktHdr;
		errorOutMailHdr = outMailHdr;
		errorOutMailHdr.to = (clientNum+10)%10;
		errorInMailHdr = inMailHdr;
		return false;
	}
	if (lockIndex/MAX_DLOCK == postOffice->GetID() && cvIndex/MAX_DCV == postOffice->GetID()) //everything on this server; proceed as normal
	{
		//check if CV & Lock exist
		lockIndex=lockIndex%MAX_DLOCK;
		cvIndex=cvIndex%MAX_DCV;
		//check if CV & Lock exist
		if(locks[lockIndex] == NULL) //If lock not found
		{
			printf("Wait Failed: Lock does not exist at index %d.\n", lockIndex);
			errorOutPktHdr = outPktHdr;
			errorOutPktHdr.to = clientNum/10+serverCount;
			errorInPktHdr = inPktHdr;
			errorOutMailHdr = outMailHdr;
			errorOutMailHdr.to = (clientNum+10)%10;
			errorInMailHdr = inMailHdr;
			return false;
		}
		else if (locks[lockIndex]->getOwnerMailID() == -1){//check if lock acquired
			printf("Wait Failed: Lock does not acquire at index %d.\n", lockIndex);
			errorOutPktHdr = outPktHdr;
			errorOutPktHdr.to = clientNum/10+serverCount;
			errorInPktHdr = inPktHdr;
			errorOutMailHdr = outMailHdr;
			errorOutMailHdr.to = (clientNum+10)%10;
			errorInMailHdr = inMailHdr;	
			return false;
		}
		else if(cvs[cvIndex] == NULL){//If cv not found
			printf("Wait Failed: CV does not exist at index %d.\n", cvIndex);
			errorOutPktHdr = outPktHdr;
			errorOutPktHdr.to = clientNum/10+serverCount;
			errorInPktHdr = inPktHdr;
			errorOutMailHdr = outMailHdr;
			errorOutMailHdr.to = (clientNum+10)%10;
			errorInMailHdr = inMailHdr;
			return false;
		}
		//cv & lock are correct!!
		//Release the lock
		locks[lockIndex]->setOwner(-1, -1); //Reset distributed lock owner 
		locks[lockIndex]->setLockState(true); //Set distributed lock to available. 
		//check wait Q, if I'm the 1st, then set the lock to be this lock
		if (cvs[cvIndex]->isQueueEmpty())
			cvs[cvIndex]-> setFirstLock(lockIndex+postOffice->GetID()*MAX_DLOCK);
			
		//wake the one waiting for acquiring this lock
		if(!locks[lockIndex]->isQueueEmpty()) //if someone was waiting, make them owner and give them their reply msg
		{
			Message* reply = locks[lockIndex]->RemoveReply(); //get reply from lock's waitQ
			locks[lockIndex]->setOwner(reply->pktHdr.to, reply->mailHdr.to); //Set distributed lock owner 
			locks[lockIndex]->setLockState(false); //Set distributed lock to busy.
			success = postOffice->Send(reply->pktHdr, reply->mailHdr, reply->data); //Let new owner know they have acquired the lock

			if ( !success ) {
			  printf("The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
			  interrupt->Halt();
			}
			fflush(stdout);
		}
		
		//Create reply
		char * ack = new char [5];
		itoa(ack,5,cvIndex+postOffice->GetID()*MAX_DCV);
		
		//add to wait Q
		printf("Machine %d being added to waitQueue for Wait with CV: %s Lock: %s\n", 
					(int)inPktHdr.from, cvs[cvIndex]->getName(), locks[lockIndex]->getName());
		outPktHdr.to = clientNum/10+serverCount;
		outMailHdr.to = (clientNum+10)%10;
		outMailHdr.from = postOffice->GetID();
		outMailHdr.length = strlen(ack) + 1;
		//printf("Wait outPktHdr.to = %d and outMailHdr.to  = %d\n",outPktHdr.to , outMailHdr.to  ); 
		Message* reply = new Message(outPktHdr, outMailHdr, ack);
		cvs[cvIndex]->QueueReply(reply);
		return true;
	}
	else if (lockIndex/MAX_DLOCK == postOffice->GetID() && cvIndex/MAX_DCV != postOffice->GetID()) //cv not on this server
	{
		lockIndex=lockIndex%MAX_DLOCK;
		cvIndex=cvIndex%MAX_DCV;
		//check if CV & Lock exist
		if(locks[lockIndex] == NULL) //If lock not found
		{
			printf("Wait Failed: Lock does not exist at index %d.\n", lockIndex);
			errorOutPktHdr = outPktHdr;
			errorOutPktHdr.to = clientNum/10+serverCount;
			errorInPktHdr = inPktHdr;
			errorOutMailHdr = outMailHdr;
			errorOutMailHdr.to = (clientNum+10)%10;
			errorInMailHdr = inMailHdr;
			return false;
		}
		else if (locks[lockIndex]->getOwnerMailID() == -1){//check if lock acquired
			printf("Wait Failed: Lock does not acquire at index %d.\n", lockIndex);
			errorOutPktHdr = outPktHdr;
			errorOutPktHdr.to = clientNum/10+serverCount;
			errorInPktHdr = inPktHdr;
			errorOutMailHdr = outMailHdr;
			errorOutMailHdr.to = (clientNum+10)%10;
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
	
	//Check if numbers are in range
	if((lockIndex < 0)||(lockIndex > serverCount*(MAX_DLOCK)-1))
	{
		printf("Signal Failed: Lock index [%d] out of bounds [%d, %d].\n", lockIndex, 0, serverCount*(MAX_DLOCK)-1);
		errorOutPktHdr = outPktHdr;
		errorOutPktHdr.to = clientNum/10+serverCount;
		errorInPktHdr = inPktHdr;
		errorOutMailHdr = outMailHdr;
		errorOutMailHdr.to = (clientNum+10)%10;
		errorInMailHdr = inMailHdr;
		return false;
	}
	else if ((cvIndex < 0)||(cvIndex > serverCount*MAX_DCV-1))
	{
		printf("Signal Failed: CV index [%d] out of bounds [%d, %d].\n", cvIndex, 0, serverCount*MAX_DCV-1);
		errorOutPktHdr = outPktHdr;
		errorOutPktHdr.to = clientNum/10+serverCount;
		errorInPktHdr = inPktHdr;
		errorOutMailHdr = outMailHdr;
		errorOutMailHdr.to = (clientNum+10)%10;
		errorInMailHdr = inMailHdr;
		return false;
	}

	if (lockIndex/MAX_DLOCK == postOffice->GetID() && cvIndex/MAX_DCV == postOffice->GetID()) //everything on this server; proceed as normal
	{
		//check if CV & Lock exist
		lockIndex=lockIndex%MAX_DLOCK;
		cvIndex=cvIndex%MAX_DCV;
		if(locks[lockIndex] == NULL) //If lock not found
		{
			printf("Signal Failed: Lock does not exist at index %d.\n", lockIndex);
			errorOutPktHdr = outPktHdr;
			errorOutPktHdr.to = clientNum/10+serverCount;
			errorInPktHdr = inPktHdr;
			errorOutMailHdr = outMailHdr;
			errorOutMailHdr.to = (clientNum+10)%10;
			errorInMailHdr = inMailHdr;
			return false;
		}
		else if (locks[lockIndex]->getOwnerMailID() == -1){//check if lock acquired
			printf("Signal Failed: Lock does not acquire at index %d.\n", lockIndex);
			errorOutPktHdr = outPktHdr;
			errorOutPktHdr.to = clientNum/10+serverCount;
			errorInPktHdr = inPktHdr;
			errorOutMailHdr = outMailHdr;
			errorOutMailHdr.to = (clientNum+10)%10;
			errorInMailHdr = inMailHdr;
			return false;
		}
		else if(cvs[cvIndex] == NULL){//If cv not found
			printf("Signal Failed: CV does not exist at index %d.\n", cvIndex);
			errorOutPktHdr = outPktHdr;
			errorOutPktHdr.to = clientNum/10+serverCount;
			errorInPktHdr = inPktHdr;
			errorOutMailHdr = outMailHdr;
			errorOutMailHdr.to = (clientNum+10)%10;
			errorInMailHdr = inMailHdr;
			return false;
		}
		//cv & lock are correct!!
		//no matter what, send the signal back
		//Create reply
		char* ack = new char [5];
		itoa(ack,5,cvIndex);
		printf("Machine %d requests signaling lock %s\n", (int)inPktHdr.from, locks[lockIndex]->getName());

		// Send acknowledgement (using "reply to" mailbox
		// in the message that just arrived
		outPktHdr.to = clientNum/10+serverCount;
		outMailHdr.to = (clientNum+10)%10;
		outMailHdr.from = postOffice->GetID();
		outMailHdr.length = strlen(ack) + 1;

		printf("Server Sending: Signal Ack!!!\n");
		success = postOffice->Send(outPktHdr, outMailHdr, ack); 
		
		if ( !success ) {
		  printf("The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
		  interrupt->Halt();
		}
		fflush(stdout);
		
		//if wait Q is not Empty
		if (!(cvs[cvIndex]->isQueueEmpty())){
			if (cvs[cvIndex]->getFirstLock() != lockIndex+postOffice->GetID()*MAX_DLOCK){ //Check if it's the same lock
				printf("Signal Failed: signal the wrong lock\n");
			errorOutPktHdr = outPktHdr;
			errorOutPktHdr.to = clientNum/10+serverCount;
			errorInPktHdr = inPktHdr;
			errorOutMailHdr = outMailHdr;
			errorOutMailHdr.to = (clientNum+10)%10;
			errorInMailHdr = inMailHdr;
				return false;
			}
			else{
				Message* reply = cvs[cvIndex]->RemoveReply(); //get reply from lock's waitQ
				reply->pktHdr.from = reply->pktHdr.to;
				reply->mailHdr.from = reply->mailHdr.to;
				bool success2 = Acquire(lockIndex+postOffice->GetID()*MAX_DLOCK, outPktHdr, reply->pktHdr,outMailHdr, reply->mailHdr, -2);
				
				if ( !success2 ) {
				  printf("The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
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
	
	//Check if numbers are in range
	if((lockIndex < 0)||(lockIndex > (MAX_DLOCK-1)))
	{
		printf("Broadcast Failed: Lock index [%d] out of bounds [%d, %d].\n", lockIndex, 0, (MAX_DLOCK-1));
		errorOutPktHdr = outPktHdr;
		errorOutPktHdr.to = clientNum/10+serverCount;
		errorInPktHdr = inPktHdr;
		errorOutMailHdr = outMailHdr;
		errorOutMailHdr.to = (clientNum+10)%10;
		errorInMailHdr = inMailHdr;
		return false;
	}
	else if ((cvIndex < 0)||(cvIndex > (MAX_DCV-1))){ // check if cv is in correct range
		printf("Broadcast Failed: CV index [%d] out of bounds [%d, %d].\n", cvIndex, 0, (MAX_DCV-1));
		errorOutPktHdr = outPktHdr;
		errorOutPktHdr.to = clientNum/10+serverCount;
		errorInPktHdr = inPktHdr;
		errorOutMailHdr = outMailHdr;
		errorOutMailHdr.to = (clientNum+10)%10;
		errorInMailHdr = inMailHdr;
		return false;
	}
	if (lockIndex/MAX_DLOCK == postOffice->GetID() && cvIndex/MAX_DCV == postOffice->GetID()) //everything on this server; proceed as normal
	{
		//check if CV & Lock exist
		lockIndex=lockIndex%MAX_DLOCK;
		cvIndex=cvIndex%MAX_DCV;
		if(locks[lockIndex] == NULL) //If lock not found
		{
			printf("Broadcast Failed: Lock does not exist at index %d.\n", lockIndex);
			errorOutPktHdr = outPktHdr;
			errorOutPktHdr.to = clientNum/10+serverCount;
			errorInPktHdr = inPktHdr;
			errorOutMailHdr = outMailHdr;
			errorOutMailHdr.to = (clientNum+10)%10;
			errorInMailHdr = inMailHdr;
			return false;
		}
		else if (locks[lockIndex]->getOwnerMailID() == -1){//check if lock acquired
			printf("Broadcast Failed: Lock does not acquire at index %d.\n", lockIndex);
			errorOutPktHdr = outPktHdr;
			errorOutPktHdr.to = clientNum/10+serverCount;
			errorInPktHdr = inPktHdr;
			errorOutMailHdr = outMailHdr;
			errorOutMailHdr.to = (clientNum+10)%10;
			errorInMailHdr = inMailHdr;
			return false;
		}
		else if(cvs[cvIndex] == NULL){//If cv not found
			printf("Broadcast Failed: CV does not exist at index %d.\n", cvIndex);
			errorOutPktHdr = outPktHdr;
			errorOutPktHdr.to = clientNum/10+serverCount;
			errorInPktHdr = inPktHdr;
			errorOutMailHdr = outMailHdr;
			errorOutMailHdr.to = (clientNum+10)%10;
			errorInMailHdr = inMailHdr;
			return false;
		}
		//cv & lock are correct!!
		//no matter what, send the Broadcast back
		//Create reply
		char* ack = new char [5];
		itoa(ack,5,cvIndex);
		printf("Machine %d requests broadcasting lock %s\n", (int)inPktHdr.from, locks[lockIndex]->getName());

		// Send acknowledgement (using "reply to" mailbox
		// in the message that just arrived
		outPktHdr.to = inPktHdr.from;
		outMailHdr.to = inMailHdr.from;
		outMailHdr.from = postOffice->GetID();
		outMailHdr.length = strlen(ack) + 1;

		printf("Server Sending: Broadcast Ack!!!\n");
		success = postOffice->Send(outPktHdr, outMailHdr, ack); 
		
		if ( !success ) {
		  printf("The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
		  interrupt->Halt();
		}
		fflush(stdout);
		
		//if wait Q is not Empty
		if (!(cvs[cvIndex]->isQueueEmpty())){
			if (cvs[cvIndex]->getFirstLock() != lockIndex+postOffice->GetID()*MAX_DLOCK){ //Check if it's the same lock
				printf("Broadcast Failed: Broadcast the wrong lock\n");
			errorOutPktHdr = outPktHdr;
			errorInPktHdr = inPktHdr;
			errorOutMailHdr = outMailHdr;
			errorInMailHdr = inMailHdr;
				return false;
			}
			else{
				Message* reply = cvs[cvIndex]->RemoveReply(); //get reply from lock's waitQ
				reply->pktHdr.from = reply->pktHdr.to;
				reply->mailHdr.from = reply->mailHdr.to;
				bool success2 = Acquire(lockIndex+postOffice->GetID()*MAX_DLOCK, outPktHdr, reply->pktHdr,outMailHdr, reply->mailHdr, -2);
				
				if ( !success2 ) {
				  printf("The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
				  interrupt->Halt();
				}
				fflush(stdout);
			}
		}
		while (!(cvs[cvIndex]->isQueueEmpty())){
				Message* reply = cvs[cvIndex]->RemoveReply(); //get reply from lock's waitQ
				reply->pktHdr.from = reply->pktHdr.to;
				reply->mailHdr.from = reply->mailHdr.to;
				bool success2 = Acquire(lockIndex+postOffice->GetID()*MAX_DLOCK, outPktHdr, reply->pktHdr,outMailHdr, reply->mailHdr, -2);
				
				if ( !success2 ) {
				  printf("The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
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

//Sends a message back to whomever just contacted it
bool SendReply(PacketHeader outPktHdr, PacketHeader inPktHdr, MailHeader outMailHdr, MailHeader inMailHdr, char* data)
{
	bool success = false;
	outPktHdr.to = inPktHdr.from;
	outMailHdr.to = inMailHdr.from;
	outMailHdr.from = postOffice->GetID();
	outMailHdr.length = strlen(data) + 1;

	printf("Sending!!!\n");
	success = postOffice->Send(outPktHdr, outMailHdr, data); 

	if ( !success ) {
	  printf("The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
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
	
	//sending out neg number
	int errorNum = -6;
	
	char * data = new char [4];
	itoa(data,4,errorNum); //Convert to string to send
	
	errorOutPktHdr.to = errorInPktHdr.from;
	errorOutMailHdr.to = errorInMailHdr.from;
	errorOutMailHdr.from = postOffice->GetID();
	errorOutMailHdr.length = strlen(data) + 1;

	printf("\nServer Sending Error Message to Machine %d!!!\n",(int)errorInPktHdr.from);
	success = postOffice->Send(errorOutPktHdr, errorOutMailHdr, data); 

	if ( !success ) {
	  printf("The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
	  interrupt->Halt();
	}

	fflush(stdout);
}
