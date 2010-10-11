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
/*  
  TestCreateCondition();
  TestDestroyCondition();	
  TestWait();
  TestSignal();		
  TestBroadcast();
 */
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
	PrintOut("Error value: ", 13);
	PrintNumber(i);
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
	PrintOut("Test DestroyLock\n", 17);
	
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
 * 
 */
void TestAquire()
{
	PrintOut("TestAquire\n", 11);
	
	PrintOut("TestAquire Completed\n\n", 22);
}

/*
 * 
 */		
void TestRelease()
{
	PrintOut("TestRelease\n", 12);
	
	PrintOut("TestRelease Completed\n\n", 23);	
}

/*
 * 
 */	
void TestCreateCondition()
{
	int cvHandle;
	int i;
	PrintOut("TestCreateCondition\n", 20);
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
	PrintNumber(i);
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
	PrintOut("TestDestroyCondition\n", 21);
	
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
 * 
 */	
void TestWait()
{
	PrintOut("TestWait\n", 9);
	
	PrintOut("TestWait Completed\n\n", 20);	
}

/*
 * 
 */
void TestSignal()
{
	PrintOut("TestSignal\n", 11);
	
	PrintOut("TestSignal Completed\n\n", 22);	
}

/*
 * 
 */	
void TestBroadcast()
{
	PrintOut("TestBroadcast\n", 14);
	
	PrintOut("TestBroadcast Completed\n\n", 25);	
}





 
 
 
 
 
 
 
 
 
 