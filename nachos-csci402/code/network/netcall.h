// netcall.h 
//Layer of abstraction for use of network for user programs

#include "copyright.h"

#include "system.h"
#include "interrupt.h"
#include "network.h"
#include "post.h"

#ifndef NETCALL_H
#define NETCALL_H

#define SERVERID 0
#define SERVERMAILID 0

#define MAX_CLIENTS 1000
#define MAX_DLOCK 100 
#define MAX_DMV 500 
#define MAX_DCV 100 

enum RequestType
{
	CREATELOCK,ACQUIRE,RELEASE,DESTROYLOCK,CREATECV,SIGNAL,BROADCAST,WAIT,DESTROYCV,
	CREATEMV,SETMV,GETMV,DESTROYMV,CREATELOCKVERIFY,CREATECVVERIFY,CREATEMVVERIFY,
	CREATELOCKANSWER,CREATECVANSWER,CREATEMVANSWER,SIGNALLOCK,SIGNALRESPONSE,
	BROADCASTLOCK,BROADCASTRESPONSE,WAITCV,WAITRESPONSE,STARTUSERPROGRAM,REGNETTHREAD,
  REGNETTHREADRESPONSE,GROUPINFO,ACK,INVALIDTYPE
};

int parseMessage(const char* buffer, timeval& timeStamp, RequestType& requestType);
int Request(RequestType requestType, char* data, int machineID, int mailID); //client
void Ack(PacketHeader inPktHdr, MailHeader inMailHdr, char* buffer);
bool processMessage(PacketHeader inPktHdr, MailHeader inMailHdr, char* msgData);
void sendError();

void itoa(char arr[], int size, int val);
bool Acquire(int lockIndex, PacketHeader outPktHdr, PacketHeader inPktHdr, MailHeader outMailHdr, MailHeader inMailHdr,int clientNum);
bool Release(int lockIndex, PacketHeader outPktHdr, PacketHeader inPktHdr, MailHeader outMailHdr, MailHeader inMailHdr, int clientNum);
bool DestroyLock(int lockIndex, PacketHeader outPktHdr, PacketHeader inPktHdr, MailHeader outMailHdr, MailHeader inMailHdr, int clientNum);
bool SetMV(int mvIndex, int value, PacketHeader outPktHdr, PacketHeader inPktHdr, MailHeader outMailHdr, MailHeader inMailHdr, int clientNum);
bool GetMV(int mvIndex, PacketHeader outPktHdr, PacketHeader inPktHdr, MailHeader outMailHdr, MailHeader inMailHdr, int clientNum);
bool DestroyMV(int mvIndex, PacketHeader outPktHdr, PacketHeader inPktHdr, MailHeader outMailHdr, MailHeader inMailHdr, int clientNum);
bool SendReply(PacketHeader outPktHdr, PacketHeader inPktHdr, MailHeader outMailHdr, MailHeader inMailHdr, char* data);
bool Wait(int cvIndex, int lockIndex, PacketHeader outPktHdr, PacketHeader inPktHdr, MailHeader outMailHdr, MailHeader inMailHdr, int clientNum);
bool Signal(int cvIndex, int lockIndex, PacketHeader outPktHdr, PacketHeader inPktHdr, MailHeader outMailHdr, MailHeader inMailHdr,int clientNum);
bool Broadcast(int cvIndex, int lockIndex, PacketHeader outPktHdr, PacketHeader inPktHdr, MailHeader outMailHdr, MailHeader inMailHdr, int clientNum);
void CreateVerify(char* data, int, int);
int CreateMVVerify(char* data);

class Message
{
public:
	Message(PacketHeader pHdr, MailHeader mHdr, char *d);
    ~Message();
	PacketHeader pktHdr;
    MailHeader mailHdr;
    char *data;
};

#ifdef CHANGED
#define FREE true;
#define BUSY false;
#endif

class DistributedLock 
{
  public:
    DistributedLock(const char* debugName);
    ~DistributedLock();     
    void setLockState(bool ls);
	void setOwner(int machID, int mailID);
	void QueueReply(Message* reply); 
	void SetGlobalId(int id) { globalId = id; }
	bool isQueueEmpty(); 
	bool getLockState(){ return lockState; } 
	int getOwnerMachID(){ return ownerMachID; }
	int getOwnerMailID(){ return ownerMailID; }
	int GetGlobalId() { return globalId; }
    const char* getName() { return name; }    
	Message* RemoveReply(); 
  private:
    const char* name;
    bool lockState; 
	List* waitQueue;
	int ownerMachID;
	int ownerMailID;
	int globalId;
};

//Class for a distributed MV. 
class DistributedMV 
{
  private:
  	int value;
	int globalId; 
    const char* name;     
  public:
    DistributedMV(const char* debugName);               
    ~DistributedMV();                        
    const char* getName() { return name; }     
	void setValue(int x) { value = x; }  
	void SetGlobalId(int id) { globalId = id; } 
	int getValue() { return value; }
	int GetGlobalId() { return globalId; }       
};

//Class for a distributed CV.
class DistributedCV 
{
  private:                
	int ownerMachID;                
	int ownerMailID;                
	int globalId;        
    int firstLock;     
    const char* name;                    
	List* waitQueue;
  public:
    DistributedCV(const char* debugName); 
    ~DistributedCV();                        
	void setOwner(int machID, int mailID);
	void SetGlobalId(int id) { globalId = id; }
	void QueueReply(Message* reply); 
    void setFirstLock(int fl);
	bool isQueueEmpty();
	int getFirstLock(){ return firstLock; }
	int getOwnerMachID(){ return ownerMachID; }
	int getOwnerMailID(){ return ownerMailID; }
	int GetGlobalId() { return globalId; }
	Message* RemoveReply(); 
    const char* getName() { return name; } 
};

#endif
