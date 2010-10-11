/* 
 *  testsynchmanager.c
 *	Test class to test all of the methods of SyncManager
 *
 */


#include "syscall.h"



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
  Write("Test SyncManager\n", 17, ConsoleOutput);
  TestCreateLock();
  
  
  Halt();
}


/* ==================================
 * Test Implementation
 * ==================================*/

/*
 * Simply tests that a lock is made
 */
void TestCreateLock()
{
	Write("Test CreateLock\n", 16, ConsoleOutput);
}

/*
 * 
 */
void TestDestroyLock()
{
	
}

/*
 * 
 */
void TestAquire()
{
	
}

/*
 * 
 */		
void TestRelease()
{
	
}

/*
 * 
 */	
void TestCreateCondition()
{
	
}

/*
 * 
 */
void TestDestroyCondition()
{
	
}

/*
 * 
 */	
void TestWait()
{
	
}

/*
 * 
 */
void TestSignal()
{
	
}

/*
 * 
 */	
void TestBroadcast()
{
	
}


 
 
 
 
 
 
 
 
 
 