
/*
	SynchManager.cc
	


*/

#include "SynchManager.h"

/* =============================================================	
 * Initialize 
 * =============================================================*/

SynchManager::SynchManager()
{
	lockBitMap = new BitMap(MaxNumLocks);
	cvBitMap = new BitMap(MaxNumLocks);
	lock_ForLockTable = new Lock("Lock for Lock Table");
	lock_ForCVTable = new Lock("Lock for CV Table");
    
}

/* =============================================================	
 * Uninitialize 
 * =============================================================*/
 
SynchManager::~SynchManager()
{
    delete lockBitMap;
	delete cvBitMap;
	delete lock_ForLockTable;
	delete lock_ForCVTable;
}

/* =============================================================	
 * Create 
 * =============================================================*/

int SynchManager::CreateLock()
{
	lock_ForLockTable->Acquire();
		int lockIndex = lockBitMap->Find(); // Marks index if found one
		// check that I can create a lock
		if(lockIndex == -1)
		{
			printf("ERROR: Cannot create lock\n");
			lock_ForLockTable->Release();
			return -1;
		}
		// make a lock entry
		LockEntry* entry = new LockEntry();
		entry->lock = new Lock("Lock #"+lockIndex);
		lockEntries[lockIndex] = entry;
	lock_ForLockTable->Release();
	return lockIndex;
}

int SynchManager::CreateCondition()
{
	lock_ForCVTable->Acquire();
		int cvIndex = cvBitMap->Find();
		// check that I can create a cv
		if(cvIndex == -1)
		{
			printf("ERROR: Cannot create Condition\n");
			lock_ForCVTable->Release();
			return -1;
		}
		// make a cv entry
		CVEntry* entry = new CVEntry();
		entry->cv = new Condition("CV #"+cvIndex);
		cvEntries[cvIndex] = entry;
	lock_ForCVTable->Release();
	return cvIndex;
}

/* =============================================================	
 * Destroy 
 * =============================================================*/

void SynchManager::DestroyLock(int index)
{
	lock_ForLockTable->Acquire();
		// validate
		if(index < 0 || index > MaxNumLocks || !lockBitMap->Test(index))
		{
			printf("ERROR: No Lock to destroy\n");
			lock_ForLockTable->Release();
			return;
		}
		// if lock is free, delete immediately
		if(lockEntries[index]->ownerProcess == NULL
			&& lockEntries[index]->toBeUsed == false)
		{
			delete lockEntries[index];
			lockBitMap->Clear(index);
		}
		else
		{
			lockEntries[index]->toBeDeleted = true;
		}
	lock_ForLockTable->Release();
}


void SynchManager::DestroyCondition(int index)
{
	lock_ForCVTable->Acquire();
		// validate
		if(index < 0 || index > MaxNumCVs || !cvBitMap->Test(index))
		{
			printf("ERROR: No Condition to destroy\n");
			lock_ForCVTable->Release();
			return;
		}
		// if CV is free, delete immediately
		if(cvEntries[index]->ownerProcess == NULL
			&& cvEntries[index]->toBeUsed == false)
		{
			delete cvEntries[index];
			cvBitMap->Clear(index);
		}
		else
		{
			cvEntries[index]->toBeDeleted = true;
		}
	lock_ForCVTable->Release();
}

/* =============================================================	
 * Acquire/Release
 * =============================================================*/

void SynchManager::Acquire(int lockIndex)
{
	lock_ForLockTable->Acquire();
		// validate
		if(lockIndex < 0 || lockIndex > MaxNumCVs || !lockBitMap->Test(lockIndex))
		{
			printf("ERROR: No Lock available to Acquire\n");
			return;
		}
		lockEntries[lockIndex]->toBeUsed = true;
	lock_ForLockTable->Release();
	
	// acquire lock, set table values
	lockEntries[lockIndex]->lock->Acquire();
	lockEntries[lockIndex]->ownerProcess = currentThread->space;
	lockEntries[lockIndex]->toBeUsed = false; // possible race condition
}

void SynchManager::Release(int lockIndex)
{
	lock_ForLockTable->Acquire();
		// validate
		if(lockIndex < 0 || lockIndex > MaxNumCVs || !lockBitMap->Test(lockIndex))
		{
			printf("ERROR: No Lock available to Release\n");
			return;
		}
		
		lockEntries[lockIndex]->lock->Release();
		lockEntries[lockIndex]->ownerProcess = NULL;
		// at end of release, check if lock needs to be deleted
		if(lockEntries[lockIndex]->toBeDeleted)
		{
			delete lockEntries[lockIndex];
			lockBitMap->Clear(lockIndex);
		}
	lock_ForLockTable->Release();
	
}


/* =============================================================	
 * Wait/Signal/Broadcast
 * =============================================================*/
 
void SynchManager::Wait(int cvIndex, int lockIndex)
{
	lock_ForCVTable->Acquire();
	lock_ForLockTable->Acquire();
		// validate
		if(cvIndex < 0 || cvIndex > MaxNumCVs || !cvBitMap->Test(cvIndex))
		{
			printf("ERROR: No Condition to available for Wait\n");
			lock_ForLockTable->Release();
			lock_ForCVTable->Release();
			return;
		}
		if(lockIndex < 0 || lockIndex > MaxNumCVs || !lockBitMap->Test(lockIndex))
		{
			printf("ERROR: No Lock available for Wait\n");
			lock_ForLockTable->Release();
			lock_ForCVTable->Release();
			return;
		}
		cvEntries[cvIndex]->toBeUsed = true;	
	lock_ForLockTable->Release();
	lock_ForCVTable->Release();
	
	// ok to Wait
	cvEntries[cvIndex]->ownerProcess = currentThread->space;
	cvEntries[cvIndex]->cv->Wait(lockEntries[lockIndex]->lock);
	cvEntries[cvIndex]->ownerProcess = NULL;
	cvEntries[cvIndex]->toBeUsed = false; // possible race condition
}

void SynchManager::Signal(int cvIndex, int lockIndex)
{
	lock_ForCVTable->Acquire();
	lock_ForLockTable->Acquire();
		// validate
		if(cvIndex < 0 || cvIndex > MaxNumCVs || !cvBitMap->Test(cvIndex))
		{
			printf("ERROR: No Condition to available for Signal\n");
			lock_ForLockTable->Release();
			lock_ForCVTable->Release();
			return;
		}
		if(lockIndex < 0 || lockIndex > MaxNumCVs || !lockBitMap->Test(lockIndex))
		{
			printf("ERROR: No Lock available for Signal\n");
			lock_ForLockTable->Release();
			lock_ForCVTable->Release();
			return;
		}
		// ok to Signal
		cvEntries[cvIndex]->cv->Signal(lockEntries[lockIndex]->lock);
	lock_ForLockTable->Release();
	lock_ForCVTable->Release();
}


void SynchManager::Broadcast(int cvIndex, int lockIndex)
{
	lock_ForCVTable->Acquire();
	lock_ForLockTable->Acquire();
		// validate
		if(cvIndex < 0 || cvIndex > MaxNumCVs || !cvBitMap->Test(cvIndex))
		{
			printf("ERROR: No Condition to available for Broadcast\n");
			lock_ForLockTable->Release();
			lock_ForCVTable->Release();
			return;
		}
		if(lockIndex < 0 || lockIndex > MaxNumCVs || !lockBitMap->Test(lockIndex))
		{
			printf("ERROR: No Lock available for Broadcast\n");
			lock_ForLockTable->Release();
			lock_ForCVTable->Release();
			return;
		}
		// ok to Broadcast
		cvEntries[cvIndex]->cv->Broadcast(lockEntries[lockIndex]->lock);
	lock_ForLockTable->Release();
	lock_ForCVTable->Release();
}
 
 
 
 
 
