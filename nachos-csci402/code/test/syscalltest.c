/*
 *
 *	Syscall test
 *	Test suite for all of the system calls in Project 2
 *
 */

#include "syscall.h"


/* ==================================
 * Test Declarions
 * ==================================*/
 
void Test1CreateLock();		
void Test2CreateCondition();		
void Test3Aquire();		
void Test4Release();		
void Test5Wait();
void Test6Signal();		
void Test7Broadcast();		



/* ==================================
 * Main
 * ==================================*/

int main()
{
    Write("Syscall Test Suite\n", 19, ConsoleOutput);
	Test1CreateLock();
	Halt();
	Write("Should not get here...\n", 23, ConsoleOutput);
    /* not reached */
}

/* ==================================
 * Test Implementation
 * ==================================*/
 
void Test1CreateLock()
{
	 Write("Test1CreateLock\n", 16, ConsoleOutput);
	 
	 
}


