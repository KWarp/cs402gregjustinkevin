/* 
 *  testsynchmanager.c
 *	Test class to test all of the methods of SyncManager
 *
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
 * Tests creating several locks
 */
void TestCreateLock()
{
	int lockHandle;
	int i;
	PrintOut("Test CreateLock\n", 16);
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
	
	/* Clear all created locks */
	while(i > -1)
	{
		DestroyLock(i);
		i--;
	}
	
	PrintOut("Test CreateLock Complete\n\n", 26);
}

/*
 * Tests:
 * - Create and Destroy 1 lock
 * - Pass in junk data to destroy lock
 * - Create several locks, and destroy them out of order
 *
 */
void TestDestroyLock()
{
	int lockHandles[20];
	int i;
	PrintOut("Test DestroyLock\n", 17);
	
	PrintOut("Create and Destory 1 lock\n", 26);
	lockHandles[0] = CreateLock();
	PrintOut("Index result: ", 14);
	PrintNumber(lockHandles[0]);
	PrintOut("\n", 1);
	DestroyLock(lockHandles[0]);
	
	PrintOut("DestoryLock on bad data:\n", 25);
	DestroyLock(-1);
	
	PrintOut("DestoryLock on bad data:\n", 25);
	DestroyLock(-1);
	
	PrintOut("Test DestroyLock Complete\n\n", 27);
}

/*
 * 
 */
void TestAquire()
{
	PrintOut("TestAquire\n", 11);
	
	PrintOut("TestAquire Completed\n", 21);
}

/*
 * 
 */		
void TestRelease()
{
	PrintOut("TestAquire\n", 11);
	
	PrintOut("TestAquire Completed\n", 21);	
}

/*
 * 
 */	
void TestCreateCondition()
{
	PrintOut("TestAquire\n", 11);
	
	PrintOut("TestAquire Completed\n", 21);	
}

/*
 * 
 */
void TestDestroyCondition()
{
	PrintOut("TestAquire\n", 11);
	
	PrintOut("TestAquire Completed\n", 21);	
}

/*
 * 
 */	
void TestWait()
{
	PrintOut("TestAquire\n", 11);
	
	PrintOut("TestAquire Completed\n", 21);	
}

/*
 * 
 */
void TestSignal()
{
	PrintOut("TestAquire\n", 11);
	
	PrintOut("TestAquire Completed\n", 21);	
}

/*
 * 
 */	
void TestBroadcast()
{
	PrintOut("TestAquire\n", 11);
	
	PrintOut("TestAquire Completed\n", 21);	
}





 
 
 
 
 
 
 
 
 
 