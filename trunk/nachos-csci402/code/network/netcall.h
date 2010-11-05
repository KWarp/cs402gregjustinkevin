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

#define MAX_CLIENTS 200 // Max number of threads that the distributed servers can handle
#define MAX_DLOCK 100 //Max number of distributed locks that can be created
#define MAX_DMV 500 //Max number of distributed monitor variables that can be created
#define MAX_DCV 100 //Max number of distributed CVs that can be created

//enum for request types
enum RequestType
{
        CREATELOCK,
        ACQUIRE,
        RELEASE,
        DESTROYLOCK,
        CREATECV,
        SIGNAL,
        BROADCAST,
        WAIT,
        DESTROYCV,
        CREATEMV,
        SETMV,
        GETMV,
        DESTROYMV,
        CREATELOCKVERIFY,
        CREATECVVERIFY,
        CREATEMVVERIFY,
        CREATELOCKANSWER,
        CREATECVANSWER,
        CREATEMVANSWER,
        SIGNALLOCK,
        SIGNALRESPONSE,
        BROADCASTLOCK,
        BROADCASTRESPONSE,
        WAITCV,
        WAITRESPONSE
};

//Struct for a message. Will be used for packaging request/reply information.
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
// Locks can be in two states, FREE and BUSY.  We have here defined
// FREE to be a true boolean value, and BUSY to be false.
#define FREE true;
#define BUSY false;
#endif
//Class for a distributed lock. Does not deals with threads directly, instead manages reply messages that will notify waiting processes
class DistributedLock {
  public:
    DistributedLock(const char* debugName);             // initialize lock to be FREE
    ~DistributedLock();                         // deallocate lock
    const char* getName() { return name; }      // debugging assist
    void setLockState(bool ls);
        bool getLockState(){ return lockState; }
        void setOwner(int machID, int mailID);
        int getOwnerMachID(){ return ownerMachID; }
        int getOwnerMailID(){ return ownerMailID; }
        void QueueReply(Message* reply); //Adds reply to back of queue
        Message* RemoveReply(); //Takes reply at front of queue
        bool isQueueEmpty();
        void SetGlobalId(int id) { globalId = id; }
        int GetGlobalId() { return globalId; }
  private:
    const char* name;                           // for debugging
    bool lockState;                     // lock's current state
        List* waitQueue;                // wait queue of replies to send once lock is free
        int ownerMachID;                //Machine ID of owner. Only valid when lockstate is busy.
        int ownerMailID;                //Mail ID of owner. Only valid when lockstate is busy.
        int globalId;
};

//Class for a distributed MV. 
class DistributedMV {
  public:
    DistributedMV(const char* debugName);               
    ~DistributedMV();                           // deallocate lock
    const char* getName() { return name; }      // debugging assist
        void setValue(int x) { value = x; }     // Setting the value
        int getValue() { return value; }        // Getting the value
        int GetGlobalId() { return globalId; }
        void SetGlobalId(int id) { globalId = id; }
        //void QueueReply(Message* reply); //Adds reply to back of queue
        //Message* RemoveReply(); //Takes reply at front of queue
        //bool isQueueEmpty();
  private:
    const char* name;                           // for debugging 
        int value;
        int globalId;
        //List* waitQueue;              // wait queue of replies to send once lock is free      
};

//Class for a distributed CV.
class DistributedCV {
  public:
    DistributedCV(const char* debugName);               // initialize lock to be FREE
    ~DistributedCV();                           // deallocate lock
    const char* getName() { return name; }      // debugging assist
    void setFirstLock(int fl);
        int getFirstLock(){ return firstLock; }
        void setOwner(int machID, int mailID);
        int getOwnerMachID(){ return ownerMachID; }
        int getOwnerMailID(){ return ownerMailID; }
        void QueueReply(Message* reply); //Adds reply to back of queue
        Message* RemoveReply(); //Takes reply at front of queue
        bool isQueueEmpty();
        int GetGlobalId() { return globalId; }
        void SetGlobalId(int id) { globalId = id; }
  private:
    const char* name;           // for debugging
    int firstLock;                      // 1st lock if Wait Q is empty
        List* waitQueue;                // wait queue of replies to send once lock is free
        int ownerMachID;                //Machine ID of owner. Only valid when lockstate is busy.
        int ownerMailID;                //Mail ID of owner. Only valid when lockstate is busy.
        int globalId;
};

//For client use:
int Request(int requestType, char* data, int mailID);

//For server use:
bool HandleRequest(); //Handles any request, inside there is a switch for enum sent in request

void sendError();//if handle request has return false
                                //send erro message to the client
//For internal use in above functions:
//bool Send(PacketHeader pktHdr, MailHeader mailHdr, char* data);
//bool Send(Message msg);

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


#endif