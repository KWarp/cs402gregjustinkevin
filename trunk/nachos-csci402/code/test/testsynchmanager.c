/* 
 *  testsynchmanager.c
 *	Test class to test all of the methods of SyncManager
 *  by Greg Lieberman
 */

#include "syscall.h"

extern const int maxLocks;
extern const int maxCVs;

/* ==================================
 * Declarions
 * ==================================*/
 
void TestCreateLock();
void TestDestroyLock();		
void TestAquire();		
void TestRelease();	
void TestCreateCondition();
void TestDestroyCondition();	
void TestWait();
void TestWaitHelper();
void TestSignal();		
void TestBroadcast();


/* ==================================
 * Main
 * ==================================*/
 
int main()
{
  PrintOut("=== Test SyncManager ===\n", 25);
  TestCreateLock();
  TestDestroyLock();
  TestAquire();		
  TestRelease();

  TestCreateCondition();
  TestDestroyCondition();	
  TestWait();
  TestSignal();		
  TestBroadcast();
  
  Halt();
}


/* ==================================
 * Test Implementation
 * ==================================*/

/*
 * - Create 1 lock
 * - Create many locks
 * - Create maximum locks
 */
void TestCreateLock()
{
	int lockHandle;
	int i;
	PrintOut("-TestCreateLock-\n", 17);
	lockHandle = -1;
	PrintOut("Creating Lock...\n", 17);
	lockHandle = CreateLock();
	PrintOut("Index result: ", 14);
	PrintNumber(lockHandle);
	PrintOut("\n", 1);
	
	PrintOut("Create 20 locks:\n", 17);
	for(i = 0; i < 20; i++)
	{
		lockHandle = CreateLock();
		PrintOut("Index result: ", 14);
		PrintNumber(lockHandle);
		PrintOut("\n", 1);
	}
	
	PrintOut("Create locks until the OS runs out:\n", 36);
	while(CreateLock() != -1)
	{
		i++;
	}
	PrintOut("Error value: ", 13);
	PrintNumber(i+1);
	PrintOut("\n", 1);
	
	/* Clear all created locks */
	while(i > -1)
	{
		DestroyLock(i);
		i--;
	}
	
	PrintOut("Test CreateLock Complete\n\n", 26);
}

/*
 * - Create and Destroy 1 lock
 * - Pass in junk data to destroy lock
 * - Create several locks, and destroy them out of order, then build new ones
 *
 */
void TestDestroyLock()
{
	int lockHandles[5];
	int i;
	PrintOut("-TestDestroyLock-\n", 18);
	
	PrintOut("Create and Destory 1 lock\n", 26);
	lockHandles[0] = CreateLock();
	PrintOut("Index result: ", 14);
	PrintNumber(lockHandles[0]);
	PrintOut("\n", 1);
	DestroyLock(lockHandles[0]);
	PrintOut("Try to destory that lock a 2nd time:\n", 37);
	DestroyLock(lockHandles[0]);
	
	PrintOut("DestoryLock on bad data:\n", 25);
	DestroyLock(-1);
	
	PrintOut("Make 5 locks:\n", 14);
	lockHandles[0] = CreateLock();
	lockHandles[1] = CreateLock();
	lockHandles[2] = CreateLock();
	lockHandles[3] = CreateLock();
	lockHandles[4] = CreateLock();
	
	for(i = 0; i < 5; i++)
	{
		PrintOut("Index result: ", 14);
		PrintNumber(lockHandles[i]);
		PrintOut("\n", 1);
	}
	
	PrintOut("Destroy 3 of them, then create 3 more:\n", 39);
	DestroyLock(lockHandles[0]);
	DestroyLock(lockHandles[2]);
	DestroyLock(lockHandles[4]);
	
	lockHandles[0] = CreateLock();
	lockHandles[2] = CreateLock();
	lockHandles[4] = CreateLock();
	
	for(i = 0; i < 5; i++)
	{
		PrintOut("Index result: ", 14);
		PrintNumber(lockHandles[i]);
		PrintOut("\n", 1);
	}
	PrintOut("Results should be the same. Shows lock reuse:\n", 46);
	
	PrintOut("Test DestroyLock Complete\n\n", 27);
}

/*
 * - Acquire bad data
 * - Acquire a lock
 * - Acquire it again
 * - Delete the lock and Acquire
 * - Release lock for actual deletion
 */
void TestAquire()
{
	int lockHandle;
	PrintOut("-TestAquire-\n", 13);
	
	PrintOut("Aquire bad data\n", 16);
	Acquire(-1);
	
	PrintOut("Create lock and Aquire\n", 23);
	lockHandle = CreateLock();
	Acquire(lockHandle);
	PrintOut("Acquire lock a second time\n", 27);
	Acquire(lockHandle);
	PrintOut("Delete the lock while still acquired\n", 37);
	DestroyLock(lockHandle);
	PrintOut("Acquire lock a third time\n", 26);
	Acquire(lockHandle);
	PrintOut("Release lock to actually be deleted, then Acquire\n", 50);
	Release(lockHandle);
	Acquire(lockHandle);
	
	PrintOut("TestAquire Completed\n\n", 22);
}

/*
 * - Release bad data
 * - Acquire and Release a lock
 * - Release it again
 * - Delete the lock and try to release
 */	
void TestRelease()
{
	int lockHandle;
	PrintOut("-TestRelease-\n", 14);
	
	PrintOut("Release bad data\n", 17);
	Release(-1);
	
	PrintOut("Create lock, Aquire and Release\n", 32);
	lockHandle = CreateLock();
	Acquire(lockHandle);
	Release(lockHandle);
	PrintOut("Release lock a second time\n", 27);
	Release(lockHandle);
	PrintOut("Delete the lock and try to release\n", 35);
	DestroyLock(lockHandle);
	Release(lockHandle);
	
	PrintOut("TestRelease Completed\n\n", 23);	
}


/*
 * - Create 1 CV
 * - Create many CVs
 * - Create maximum CVs
 */	
void TestCreateCondition()
{
	int cvHandle;
	int i;
	PrintOut("-TestCreateCondition-\n", 22);
	cvHandle = -1;
	PrintOut("Creating CV...\n", 15);
	cvHandle = CreateCondition();
	PrintOut("Index result: ", 14);
	PrintNumber(cvHandle);
	PrintOut("\n", 1);
	
	PrintOut("Create 20 CVs:\n", 15);
	for(i = 0; i < 20; i++)
	{
		cvHandle = CreateCondition();
		PrintOut("Index result: ", 14);
		PrintNumber(cvHandle);
		PrintOut("\n", 1);
	}
	
	PrintOut("Create CVs until the OS runs out:\n", 34);
	while(CreateCondition() != -1)
	{
		i++;
	}
	PrintOut("Error value: ", 13);
	PrintNumber(i+1);
	PrintOut("\n", 1);
	
	/* Clear all created locks */
	while(i > -1)
	{
		DestroyCondition(i);
		i--;
	}
	
	PrintOut("TestCreateCondition Completed\n\n", 31);	
}

/*
 * - Create and Destroy 1 CV
 * - Pass in junk data to destroy CV
 * - Create several CVs, and destroy them out of order, then build new ones
 */
void TestDestroyCondition()
{
	int cvHandles[5];
	int i;
	PrintOut("-TestDestroyCondition-\n", 23);
	
	PrintOut("Create and Destory 1 CV\n", 26);
	cvHandles[0] = CreateCondition();
	PrintOut("Index result: ", 14);
	PrintNumber(cvHandles[0]);
	PrintOut("\n", 1);
	DestroyCondition(cvHandles[0]);
	PrintOut("Try to destory that CV a 2nd time:\n", 37);
	DestroyCondition(cvHandles[0]);
	
	PrintOut("DestroyCondition on bad data:\n", 30);
	DestroyCondition(-1);
	
	PrintOut("Make 5 CVs:\n", 14);
	cvHandles[0] = CreateCondition();
	cvHandles[1] = CreateCondition();
	cvHandles[2] = CreateCondition();
	cvHandles[3] = CreateCondition();
	cvHandles[4] = CreateCondition();
	
	for(i = 0; i < 5; i++)
	{
		PrintOut("Index result: ", 14);
		PrintNumber(cvHandles[i]);
		PrintOut("\n", 1);
	}
	
	PrintOut("Destroy 3 of them, then create 3 more:\n", 39);
	DestroyCondition(cvHandles[0]);
	DestroyCondition(cvHandles[2]);
	DestroyCondition(cvHandles[4]);
	
	cvHandles[0] = CreateCondition();
	cvHandles[2] = CreateCondition();
	cvHandles[4] = CreateCondition();
	
	for(i = 0; i < 5; i++)
	{
		PrintOut("Index result: ", 14);
		PrintNumber(cvHandles[i]);
		PrintOut("\n", 1);
	}
	PrintOut("Results should be the same. Shows CV reuse:\n", 46);
	
	PrintOut("TestDestroyCondition Completed\n\n", 32);	
}

/*
 * - Wait on bad CV/lock
 * - Fork another thread and test a Wait on a good CV/lock
 * - Delete the lock/CV and then Wait
 */	
int globalCVHandle;
int globalLockHandle;
void TestWait()
{
	PrintOut("-TestWait-\n", 11);
	globalCVHandle = CreateCondition();
	globalLockHandle = CreateLock();
	
	Acquire(globalLockHandle);
		PrintOut("Wait on Bad CV and Bad Lock\n", 28);
		Wait(-1, -1);
		PrintOut("Wait on Good CV and Bad Lock\n", 29);
		Wait(globalCVHandle, -1);
		PrintOut("Wait on Bad CV and Good Lock\n", 29);
		Wait(-1, globalLockHandle);
		
		PrintOut("Fork a helper thread\n", 21);
		Fork(TestWaitHelper);
		PrintOut("TestWait Waiting on Good CV and Good Lock\n", 42);
		Signal(globalCVHandle, globalLockHandle);
		Wait(globalCVHandle, globalLockHandle);
		PrintOut("TestWait woke up, signaling TestWaitHelper\n", 43);	
		Signal(globalCVHandle, globalLockHandle);
		Yield(100);
	Release(globalLockHandle);
	
	PrintOut("Delete CV, then Wait\n", 22);
	DestroyCondition(globalCVHandle);
	Wait(globalCVHandle, globalLockHandle);
	PrintOut("Make new CV, delete lock, then Wait\n", 37);
	globalCVHandle = CreateCondition();
	DestroyLock(globalLockHandle);
	Wait(globalCVHandle, globalLockHandle);
	PrintOut("Delete CV and Lock, then Wait\n", 31);
	DestroyCondition(globalCVHandle);
	Wait(globalCVHandle, globalLockHandle);

	PrintOut("TestWait Completed\n\n", 20);	
}

/*
 * Second thread for Wait testing
 */	
void TestWaitHelper()
{
	Acquire(globalLockHandle);
		PrintOut("TestWaitHelper starting...\n", 27);
		PrintOut("TestWaitHelper Signaling TestWait\n", 34);
		Signal(globalCVHandle, globalLockHandle);
		PrintOut("TestWaitHelper waiting for Signal from TestWait\n", 48);
		Wait(globalCVHandle, globalLockHandle);
		Signal(globalCVHandle, globalLockHandle);
		PrintOut("TestWaitHelper woke up\n", 23);
		PrintOut("TestWaitHelper exiting...\n", 26);
	Release(globalLockHandle);
	Exit(0);
}

/*
 * - Signal bad data, lock/CV
 * - Signal good data lock/CV
 * - Delete the lock/CV and then Signal
 */
void TestSignal()
{
	int cvHandle;
	int lockHandle;
	cvHandle = CreateCondition();
	lockHandle = CreateLock();
	
	PrintOut("-TestSignal-\n", 13);
	PrintOut("Signal Bad CV and Bad Lock\n", 27);
	Signal(-1, -1);
	PrintOut("Signal Good CV and Bad Lock\n", 28);
	Signal(cvHandle, -1);
	PrintOut("Signal Bad CV and Good Lock\n", 28);
	Signal(-1, lockHandle);
	PrintOut("Signal Good CV and Good Lock\n", 29);
	Signal(cvHandle, lockHandle);
	
	PrintOut("Delete CV, then Signal\n", 23);
	DestroyCondition(cvHandle);
	Signal(cvHandle, lockHandle);
	PrintOut("Make new CV, delete lock, then Signal\n", 38);
	cvHandle = CreateCondition();
	DestroyLock(lockHandle);
	Signal(cvHandle, lockHandle);
	PrintOut("Delete CV and Lock, then Signal\n", 32);
	DestroyCondition(cvHandle);
	Signal(cvHandle, lockHandle);
	
	PrintOut("TestSignal Completed\n\n", 22);	
}

/*
 * - Broadcast bad data, lock/CV
 * - Broadcast good data lock/CV
 * - Delete the lock/CV and then Broadcast
 */	
void TestBroadcast()
{
	int cvHandle;
	int lockHandle;
	cvHandle = CreateCondition();
	lockHandle = CreateLock();
	
	PrintOut("-TestBroadcast-\n", 16);
	
	PrintOut("Broadcast Bad CV and Bad Lock\n", 30);
	Broadcast(-1, -1);
	PrintOut("Broadcast Good CV and Bad Lock\n", 31);
	Broadcast(cvHandle, -1);
	PrintOut("Broadcast Bad CV and Good Lock\n", 31);
	Broadcast(-1, lockHandle);
	PrintOut("Broadcast Good CV and Good Lock\n", 32);
	Broadcast(cvHandle, lockHandle);
	
	PrintOut("Delete CV, then Broadcast\n", 26);
	DestroyCondition(cvHandle);
	Broadcast(cvHandle, lockHandle);
	PrintOut("Make new CV, delete lock, then Broadcast\n", 41);
	cvHandle = CreateCondition();
	DestroyLock(lockHandle);
	Broadcast(cvHandle, lockHandle);
	PrintOut("Delete CV and Lock, then Broadcast\n", 35);
	DestroyCondition(cvHandle);
	Broadcast(cvHandle, lockHandle);
	
	PrintOut("TestBroadcast Completed\n\n", 25);	
}



 
 
 
 
 
 
 