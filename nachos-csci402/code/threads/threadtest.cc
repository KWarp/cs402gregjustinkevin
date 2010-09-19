// threadtest.cc 
//	Simple test case for the threads assignment.
//
//	Create two threads, and have them context switch
//	back and forth between themselves by calling Thread::Yield, 
//	to illustratethe inner workings of the thread system.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"

#ifdef CHANGED
	#include "synch.h"
#endif
//=========================================================================--------------------
// SimpleThread
// 	Loop 5 times, yielding the CPU to another ready thread 
//	each iteration.
//
//	"which" is simply a number identifying the thread, for debugging
//	purposes.
//=========================================================================--------------------

void SimpleThread(int which)
{
    int num;
    
    for (num = 0; num < 5; num++) {
	printf("*** thread %d looped %d times\n", which, num);
        currentThread->Yield();
    }
}

//=========================================================================--------------------
// ThreadTest
// 	Set up a ping-pong between two threads, by forking a thread 
//	to call SimpleThread, and then calling SimpleThread ourselves.
//=========================================================================--------------------

void ThreadTest()
{
    DEBUG('t', "Entering SimpleTest");

    Thread * thread = new Thread("forked thread");

    thread->Fork(SimpleThread, 1);
    SimpleThread(0);
}

#ifdef CHANGED
// =========================================================================
// Test Suite - Test A through E
// =========================================================================


// =========================================================================
// test A
// Demonstrate that a waiting thread will not wake from a preexisting signal
// =========================================================================
Semaphore testA_semaphore1("testA_semaphore1",0);     	// Used so that the signal will appear before the wait.
Semaphore testA_done("testA_done",0);     				// Semaphore to show when test is complete
Lock testA_lock1("testA_lock1");            			// Lock to be used by wait and signal
Condition testA_condition1("testA_condition1");       	// Condition to be used by wait and signal
bool thread2_finished = false;							// boolean that describes whether or not thread2 has finished

// =========================================================================
// testA_thread1() -- test A thread 1
//     Signalling thread.
// =========================================================================
void testA_thread1() 
{
    testA_lock1.Acquire();
    printf("%s: Lock %s acquired, signalling %s\n",currentThread->getName(),
           testA_lock1.getName(), testA_condition1.getName());
    testA_condition1.Signal(&testA_lock1);
    printf("%s: Releasing Lock %s\n",currentThread->getName(),
           testA_lock1.getName());
    testA_lock1.Release();
    testA_semaphore1.V();  // release testA_thread2
    testA_done.V();
}

// =========================================================================
// testA_thread2() -- test A thread 2
//     Waiting thread - note this thread is waiting on a signal that has already been fired.
// =========================================================================
void testA_thread2() 
{
	thread2_finished = false;
    testA_semaphore1.P();  // Wait for testA_thread1 to be done with the lock
    testA_lock1.Acquire();
    printf("%s: Lock %s acquired, waiting on %s\n",currentThread->getName(),
           testA_lock1.getName(), testA_condition1.getName());
    testA_condition1.Wait(&testA_lock1);
    
	thread2_finished = true;
    testA_lock1.Release();
}

// =========================================================================
// initialize threads for testA
// =========================================================================
void testA()
{
    Thread * thread;
	
	printf("Initiating test A. \n");
    printf("Note that thread testA_thread2 should not complete\n");

	//start first thread
    thread = new Thread("testA_thread1");
    thread->Fork((VoidFunctionPtr)testA_thread1,0);

	//start second thread
    thread = new Thread("testA_thread2");
    thread->Fork((VoidFunctionPtr)testA_thread2,0);

	//wait for completion
    testA_done.P();
	
    printf("Completed test A\n");
	if(thread2_finished) //determine failure
	{
		printf("test A Failed - testA_thread2 completed\n");
	}
	else
	{
		printf("test A Succeeded - testA_thread2 did not complete\n");
	}
}


// =========================================================================
// test B 
// Demonstrate that a thread cannot release a lock which it does not hold.
// =========================================================================
Semaphore testB_semaphore1("testB_semaphore1",0);       // Used for <testB_thread1 acquires testB_lock1 before testB_thread2>
Semaphore testB_semaphore2("testB_semaphore2",0);       // Used for <testB_thread2 to wait on testB_lock1 before testB_thread3 releases>
Semaphore testB_semaphore3("testB_semaphore3",0);       // Used for <testB_thread3 acquires lock before testB_thread1 releases it>
Semaphore testB_done("testB_done",0);   				// Semaphore to show when test is complete
Lock testB_lock1("testB_lock1");              			// the lock tested in test B

// =========================================================================
// testB_thread1() -- testB thread 1
//     Lock Owner
// =========================================================================
void testB_thread1() 
{
    testB_lock1.Acquire();
    testB_semaphore1.V(); 
    printf ("%s: Acquired Lock %s, waiting for thread3\n",currentThread->getName(),
            testB_lock1.getName());
    testB_semaphore3.P();
    printf ("%s: working in CS\n",currentThread->getName());
    for (int i = 0; i < 1000000; i++) ;
    printf ("%s: Releasing Lock %s\n",currentThread->getName(),
            testB_lock1.getName());
    testB_lock1.Release();
    testB_done.V();
}

// =========================================================================
// testB_thread2() -- testB thread 2
//     Lock Needer
// =========================================================================
void testB_thread2() 
{

    testB_semaphore1.P();  // Wait till lock has been acquired
    testB_semaphore2.V();  // Let lock try to be aqcuired

    printf("%s: trying to acquire lock %s\n",currentThread->getName(),testB_lock1.getName());
    testB_lock1.Acquire();

    printf ("%s: Acquired Lock %s, working in CS\n",currentThread->getName(),testB_lock1.getName());
    
	for (int i = 0; i < 10; i++)
		currentThread->Yield();
		
	printf ("%s: Releasing Lock %s\n",currentThread->getName(),
            testB_lock1.getName());
    
	testB_lock1.Release();
    testB_done.V();
}

// =========================================================================
// testB_thread3() -- testB thread 3
//     Illegal Lock philanthropist
// =========================================================================
void testB_thread3() 
{

    testB_semaphore2.P();  // Wait for Lock Needer
    testB_semaphore3.V();  // Let Lock Owner run wild.
	
    for ( int i = 0; i < 3; i++ ) {
        printf("%s: Trying to release Lock %s\n",currentThread->getName(),
               testB_lock1.getName());
        testB_lock1.Release();
    }
}

// =========================================================================
// initialize all of the threads
// =========================================================================
void testB()
{
    Thread * thread;
    int i;

    printf("Initiating test B\n");

    thread = new Thread("testB_thread1");
    thread->Fork((VoidFunctionPtr)testB_thread1,0);

    thread = new Thread("testB_thread2");
    thread->Fork((VoidFunctionPtr)testB_thread2,0);

    thread = new Thread("testB_thread3");
    thread->Fork((VoidFunctionPtr)testB_thread3,0);

    for (  i = 0; i < 2; i++ )
        testB_done.P();
		
    printf("Completed test B\n");
}

// =========================================================================
// test C
// Demonstrate that broadcast will signal all waiting threads.
// =========================================================================
Semaphore testC_semaphore1("testC_semaphore1",0);     	// To make sure all of the threads are waiting before broadcast
Semaphore testC_done("testC_done",0); 					// Semaphore to show when test is complete
Lock testC_lock1("testC_lock1");            			// lock to use for braodcast and wait
Condition testC_condition1("testC_condition1");       	// Condition to use for broadcast and wait

// =========================================================================
// testC_waiter()
//     Wait on the condition variable until they are signalled to be released
// =========================================================================
void testC_waiter() 
{
    testC_lock1.Acquire();
    testC_semaphore1.V();          // Bam im waiting on you bitch
    printf("%s: Lock %s acquired, waiting on %s\n",currentThread->getName(),
           testC_lock1.getName(), testC_condition1.getName());
    testC_condition1.Wait(&testC_lock1);
    printf("%s: freed from %s\n",currentThread->getName(), testC_condition1.getName());
    testC_lock1.Release();
    testC_done.V();
}


// =========================================================================
// testC_signaller()
//     Broadcasting thread.  I yell loudly
// =========================================================================
void testC_signaller() 
{
	int i = 0;
	while(i < 5)
	{
		testC_semaphore1.P();
		i++;
	}
	
    testC_lock1.Acquire();
    printf("%s: Lock %s acquired, broadcasting %s\n",currentThread->getName(),testC_lock1.getName(), testC_condition1.getName());
    
	testC_condition1.Broadcast(&testC_lock1);
    
	printf("%s: Releasing %s\n",currentThread->getName(), testC_lock1.getName());
    
	testC_lock1.Release();
    testC_done.V();
}

// =========================================================================
// initialize threads for testC
// =========================================================================

void testC()
{
    Thread * thread;
    char *name;
    int i = 0;
	
	printf("Initiating test C\n");

    while (i < 5) 
	{
        name = new char [20];
        sprintf(name,"testC_waiter%d",i);
        thread = new Thread(name);
        thread->Fork((VoidFunctionPtr)testC_waiter,0);
		i ++;
	}
	
    thread = new Thread("testC_signaller");
    thread->Fork((VoidFunctionPtr)testC_signaller,0);

    // Wait for test C to complete
    i = 0;
	while(i < 6)
	{
		testC_done.P();
		i++;
	}
    printf("Completed test C\n");
}

// =========================================================================
// test D
// Show that when multiple threads are waiting, a signal will only wake one of them.
// =========================================================================
Semaphore testD_semaphore1("testD_semaphore1",0);     	// Used to set up ordering of threads
Semaphore testD_done("testD_done",0); 					// Semaphore to show when test is complete 
Lock testD_lock1("testD_lock1");            			// Lock to be used by wait and signal
Condition testD_condition1("testD_condition1");       	// Condition to be used by wait and signal

// =========================================================================
// testD_waiter()
// Waits for a signal.
// =========================================================================

void testD_waiter() 
{
    testD_lock1.Acquire();
    testD_semaphore1.V();          // Let the signaller know we're ready to wait
    printf("%s: Lock %s acquired, waiting on %s\n",currentThread->getName(),
           testD_lock1.getName(), testD_condition1.getName());
    testD_condition1.Wait(&testD_lock1);
    printf("%s: freed from %s\n",currentThread->getName(), testD_condition1.getName());
    testD_lock1.Release();
    testD_done.V();
}


// =========================================================================
// testD_signaller()
// signals waiters
// =========================================================================
void testD_signaller() 
{
    // Don'thread signal until someone's waiting
   
    for ( int i = 0; i < 5 ; i++ )
        testD_semaphore1.P();
 
	testD_lock1.Acquire();
    printf("%s: Lock %s acquired, signalling %s\n",currentThread->getName(),
           testD_lock1.getName(), testD_condition1.getName());
    testD_condition1.Signal(&testD_lock1);
    printf("%s: Releasing %s\n",currentThread->getName(), testD_lock1.getName());
    testD_lock1.Release();
    testD_done.V();
}

// =========================================================================
// initialize threads for testD
// =========================================================================
void testD()
{
    Thread * thread;
    char *name;
    int i = 0;
	
	printf("Initiating test D\n");
	
    while(i < 5) 
	{
        name = new char [14];
        sprintf(name,"testD_waiter%d",i);
        thread = new Thread(name);
        thread->Fork((VoidFunctionPtr)testD_waiter,0);
		i++;
    }
	
    thread = new Thread("testD_signaller");
    thread->Fork((VoidFunctionPtr)testD_signaller,0);

    // Wait until all threads are complete
    i = 0;
	while(i <2)
	{
        testD_done.P();
		i++;
	}
	
    printf("Completed test D\n");
}

// =========================================================================
// test E 
// Demonstrate that a thread holding one lock cannot release another lock.
// =========================================================================
Semaphore testE_semaphore1("testE_semaphore1",0);     	// Used to garantee order of thread opperations.
Lock testE_lock1("testE_lock1");            			// Lock to be held legally
Lock testE_lock2("testE_lock2");            			// Lock to be used illegally
Condition testE_condition1("testE_condition1");       	// Condition to be used to wake the sleeping thread.

// =========================================================================
// testE_thread1() -- test E thread 1
// Waits on the condition to be signalled
// =========================================================================
void testE_thread1() 
{
    testE_lock1.Acquire();
    testE_semaphore1.V();  // release testE_thread2
    printf("%s: Lock %s acquired, waiting on %s\n",currentThread->getName(),
           testE_lock1.getName(), testE_condition1.getName());
    testE_condition1.Wait(&testE_lock1);
    printf("%s: Releasing Lock %s\n",currentThread->getName(),
           testE_lock1.getName());
    testE_lock1.Release();
}

// =========================================================================
// testE_thread1() -- test E thread 1
// Waits on testE_condition1 with testE_lock2 which is illegal
// =========================================================================
void testE_thread2() 
{
    testE_semaphore1.P();  // Wait for testE_thread1 to get into the monitor
    testE_lock1.Acquire();
    testE_lock2.Acquire();
    printf("%s: Lock %s acquired, signalling %s\n",currentThread->getName(),
           testE_lock2.getName(), testE_condition1.getName());
    testE_condition1.Signal(&testE_lock2);
    printf("%s: Releasing Lock %s\n",currentThread->getName(),
           testE_lock2.getName());
    testE_lock2.Release();
    printf("%s: Releasing Lock %s\n",currentThread->getName(),
           testE_lock1.getName());
    testE_lock1.Release();
}

// =========================================================================
// initialize threads for testE
// =========================================================================
void testE()
{
    Thread * thread;
	
    printf("Initiating test E.\n");
    printf("!!! There is an error if the thread testE_thread1 completes\n");

    thread = new Thread("testE_thread1");
    thread->Fork((VoidFunctionPtr)testE_thread1,0);

    thread = new Thread("testE_thread2");
    thread->Fork((VoidFunctionPtr)testE_thread2,0);
}


// Run test A through E
void TestSuite() 
{	
	// =========================================================================
	// test A
	// Demonstrate that a waiting thread will not wake from a preexisting signal
	// =========================================================================
	testA();
	
	// =========================================================================
	// test B 
	// Demonstrate that a thread cannot release a lock which it does not hold.
	// =========================================================================
	testB();	

	// =========================================================================
	// test C
	// Show that Broadcast wakes all waiting threads
	// =========================================================================
	testC();
		
	// =========================================================================
	// test D
	// Show that when multiple threads are waiting, a signal will only wake one of them.
	// =========================================================================
	testD();
	
	// =========================================================================
	// test E 
	// Demonstrate that a thread holding one lock cannot release another lock.
	// =========================================================================
	testE();   
}
#endif

