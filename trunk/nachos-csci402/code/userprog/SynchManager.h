/*

	SynchManager.h
	This class keeps track of Locks and CVs in the OS level.
	by Greg Lieberman
	
*/

#include "synch.h"
#include "bitmap.h"
#include "addrspace.h"
#include "system.h"

#define MaxNumLocks 2100
#define MaxNumCVs 2100

// Keeps track of the state of a Lock in my entry table
struct LockEntry
{
	Lock* lock;
	AddrSpace* ownerProcess;
	bool toBeUsed;
	bool toBeDeleted;
	LockEntry()
	{
		lock = NULL;
		ownerProcess = NULL;
		toBeUsed = false;
		toBeDeleted = false;
	}
	~LockEntry()
	{
		if(lock != NULL)
			delete lock;
	}
};

// Keeps track of the state of a CV in my entry table
struct CVEntry
{
	Condition* cv;
	AddrSpace* ownerProcess;
	bool toBeUsed;
	bool toBeDeleted;
	CVEntry()
	{
		cv = NULL;
		ownerProcess = NULL;
		toBeUsed = false;
		toBeDeleted = false;
	}
	~CVEntry()
	{
		if(cv != NULL)
			delete cv;
	}
};

class SynchManager 
{
  public:
    SynchManager();		// initialize
    ~SynchManager();	// deallocate the condition
    
	int CreateLock();
    int CreateCondition();
	
	void DestroyLock(int index);
	void DestroyCondition(int index);
	
	void Acquire(int lockIndex);
	void Release(int lockIndex);
	
	void Wait(int cvIndex, int lockIndex);
	void Signal(int cvIndex, int lockIndex);
	void Broadcast(int cvIndex, int lockIndex);

  private:
	LockEntry* lockEntries[MaxNumLocks];
	BitMap* lockBitMap;
	Lock* lock_ForLockTable;
	
	CVEntry* cvEntries[MaxNumLocks];
	BitMap* cvBitMap;
	Lock* lock_ForCVTable;
    
};



