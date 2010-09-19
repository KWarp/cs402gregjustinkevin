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
//----------------------------------------------------------------------
// SimpleThread
// 	Loop 5 times, yielding the CPU to another ready thread 
//	each iteration.
//
//	"which" is simply a number identifying the thread, for debugging
//	purposes.
//----------------------------------------------------------------------

void
SimpleThread(int which)
{
    int num;
    
    for (num = 0; num < 5; num++) {
	printf("*** thread %d looped %d times\n", which, num);
        currentThread->Yield();
    }
}

//----------------------------------------------------------------------
// ThreadTest
// 	Set up a ping-pong between two threads, by forking a thread 
//	to call SimpleThread, and then calling SimpleThread ourselves.
//----------------------------------------------------------------------

void
ThreadTest()
{
    DEBUG('t', "Entering SimpleTest");

    Thread *t = new Thread("forked thread");

    t->Fork(SimpleThread, 1);
    SimpleThread(0);
}

#ifdef CHANGED
// --------------------------------------------------
// Test Suite
// --------------------------------------------------


// --------------------------------------------------
// Test 1 
// Show that a thread trying to release a lock it does not
// hold does not work
// --------------------------------------------------
Semaphore test1_semaphore1("test1_semaphore1",0);       // To make sure test1_thread1 acquires the
                                  // lock before test1_thread2
Semaphore test1_semaphore2("test1_semaphore2",0);       // To make sure test1_thread2 Is waiting on the
                                  // lock before test1_thread3 releases it
Semaphore test1_semaphore3("test1_semaphore3",0);       // To make sure test1_thread1 does not release the
                                  // lock before test1_thread3 tries to acquire it
Semaphore test1_done("test1_done",0);   // So that TestSuite knows when Test 1 is
                                  // done
Lock test1_lock1("test1_lock1");              // the lock tested in Test 1

// --------------------------------------------------
// test1_thread1() -- test1 thread 1
//     This is the rightful lock owner
// --------------------------------------------------
void test1_thread1() 
{
    test1_lock1.Acquire();
    test1_semaphore1.V();  // Allow test1_thread2 to try to Acquire Lock
    printf ("%s: Acquired Lock %s, waiting for t3\n",currentThread->getName(),
            test1_lock1.getName());
    test1_semaphore3.P();
    printf ("%s: working in CS\n",currentThread->getName());
    for (int i = 0; i < 1000000; i++) ;
    printf ("%s: Releasing Lock %s\n",currentThread->getName(),
            test1_lock1.getName());
    test1_lock1.Release();
    test1_done.V();
}

// --------------------------------------------------
// test1_thread2() -- test1 thread 2
//     This thread will wait on the held lock.
// --------------------------------------------------
void test1_thread2() 
{

    test1_semaphore1.P();  // Wait until t1 has the lock
    test1_semaphore2.V();  // Let t3 try to acquire the lock

    printf("%s: trying to acquire lock %s\n",currentThread->getName(),test1_lock1.getName());
    test1_lock1.Acquire();

    printf ("%s: Acquired Lock %s, working in CS\n",currentThread->getName(),test1_lock1.getName());
    
	for (int i = 0; i < 10; i++)
		currentThread->Yield();
		
	printf ("%s: Releasing Lock %s\n",currentThread->getName(),
            test1_lock1.getName());
    
	test1_lock1.Release();
    test1_done.V();
}

// --------------------------------------------------
// test1_thread3() -- test1 thread 3
//     This thread will try to release the lock illegally
// --------------------------------------------------
void test1_thread3() 
{

    test1_semaphore2.P();  // Wait until t2 is ready to try to acquire the lock
    test1_semaphore3.V();  // Let t1 do it's stuff
	
    for ( int i = 0; i < 3; i++ ) {
        printf("%s: Trying to release Lock %s\n",currentThread->getName(),
               test1_lock1.getName());
        test1_lock1.Release();
    }
}

// --------------------------------------------------
// initialize all of the threads
// --------------------------------------------------
void Test1()
{
    Thread *t;
    int i;

    printf("Starting Test 1\n");

    t = new Thread("test1_thread1");
    t->Fork((VoidFunctionPtr)test1_thread1,0);

    t = new Thread("test1_thread2");
    t->Fork((VoidFunctionPtr)test1_thread2,0);

    t = new Thread("test1_thread3");
    t->Fork((VoidFunctionPtr)test1_thread3,0);

    for (  i = 0; i < 2; i++ )
        test1_done.P();
		
    printf("Completed Test 1\n");
}



// --------------------------------------------------
// Test 2
// Show that Signals are not stored -- a Signal with no
// thread waiting is ignored
// --------------------------------------------------
Lock test2_lock1("test2_lock1");            // For mutual exclusion
Condition test2_condition1("test2_condition1");       // The condition variable to test
Semaphore test2_semaphore1("test2_semaphore1",0);     // To ensure the Signal comes before the wait
Semaphore test2_done("test2_done",0);     // So that TestSuite knows when Test 2 is
                                  // done
bool thread2_finished = false;
// --------------------------------------------------
// test2_thread1() -- test 2 thread 1
//     This thread will signal a variable with nothing waiting
// --------------------------------------------------
void test2_thread1() 
{
    test2_lock1.Acquire();
    printf("%s: Lock %s acquired, signalling %s\n",currentThread->getName(),
           test2_lock1.getName(), test2_condition1.getName());
    test2_condition1.Signal(&test2_lock1);
    printf("%s: Releasing Lock %s\n",currentThread->getName(),
           test2_lock1.getName());
    test2_lock1.Release();
    test2_semaphore1.V();  // release test2_thread2
    test2_done.V();
}

// --------------------------------------------------
// test2_thread2() -- test 2 thread 2
//     This thread will wait on a pre-signalled variable
// --------------------------------------------------
void test2_thread2() 
{
	thread2_finished = false;
    test2_semaphore1.P();  // Wait for test2_thread1 to be done with the lock
    test2_lock1.Acquire();
    printf("%s: Lock %s acquired, waiting on %s\n",currentThread->getName(),
           test2_lock1.getName(), test2_condition1.getName());
    test2_condition1.Wait(&test2_lock1);
    
	thread2_finished = true;
    test2_lock1.Release();
}

// --------------------------------------------------
// initialize threads for test2
// --------------------------------------------------
void Test2()
{
    Thread *t;
	
	printf("Starting Test 2. \n");
    printf("Note that thread test2_thread2 should not complete\n");

    t = new Thread("test2_thread1");
    t->Fork((VoidFunctionPtr)test2_thread1,0);

    t = new Thread("test2_thread2");
    t->Fork((VoidFunctionPtr)test2_thread2,0);

    test2_done.P();
	
    printf("Completed Test 2\n");
	if(thread2_finished)
	{
		printf("Test 2 Failed - test2_thread2 completed 1\n");
	}
	else
	{
		printf("Test 2 Succeeded - test2_thread2 did not complete 1\n");
	}
}

// --------------------------------------------------
// Test 3
// Show that Signal only wakes 1 thread
// --------------------------------------------------
Lock test3_lock1("test3_lock1");            // For mutual exclusion
Condition test3_condition1("test3_condition1");       // The condition variable to test
Semaphore test3_semaphore1("test3_semaphore1",0);     // To ensure the Signal comes before the wait
Semaphore test3_done("test3_done",0); // So that TestSuite knows when Test 3 is
                                // done

// --------------------------------------------------
// test3_waiter()
//     These threads will wait on the test3_condition1 condition variable.  Only
//     one test3_waiter will be released
// --------------------------------------------------
void test3_waiter() 
{
    test3_lock1.Acquire();
    test3_semaphore1.V();          // Let the signaller know we're ready to wait
    printf("%s: Lock %s acquired, waiting on %s\n",currentThread->getName(),
           test3_lock1.getName(), test3_condition1.getName());
    test3_condition1.Wait(&test3_lock1);
    printf("%s: freed from %s\n",currentThread->getName(), test3_condition1.getName());
    test3_lock1.Release();
    test3_done.V();
}


// --------------------------------------------------
// test3_signaller()
//     This threads will signal the test3_condition1 condition variable.  Only
//     one test3_signaller will be released
// --------------------------------------------------
void test3_signaller() 
{
    // Don't signal until someone's waiting
   
    for ( int i = 0; i < 5 ; i++ )
        test3_semaphore1.P();
 
	test3_lock1.Acquire();
    printf("%s: Lock %s acquired, signalling %s\n",currentThread->getName(),
           test3_lock1.getName(), test3_condition1.getName());
    test3_condition1.Signal(&test3_lock1);
    printf("%s: Releasing %s\n",currentThread->getName(), test3_lock1.getName());
    test3_lock1.Release();
    test3_done.V();
}

// --------------------------------------------------
// initialize threads for test3
// --------------------------------------------------
void Test3()
{
    Thread *t;
    char *name;
    int i = 0;
	
	printf("Starting Test 3\n");
	
    while(i < 5) 
	{
        name = new char [14];
        sprintf(name,"test3_waiter%d",i);
        t = new Thread(name);
        t->Fork((VoidFunctionPtr)test3_waiter,0);
		i++;
    }
	
    t = new Thread("test3_signaller");
    t->Fork((VoidFunctionPtr)test3_signaller,0);

    // Wait for Test 3 to complete
    i = 0;
	while(i <5)
	{
        test3_done.P();
		i++;
	}
}

// --------------------------------------------------
// Test 4
// Show that Broadcast wakes all waiting threads
// --------------------------------------------------
Lock test4_lock1("test4_lock1");            // For mutual exclusion
Condition test4_condition1("test4_condition1");       // The condition variable to test
Semaphore test4_semaphore1("test4_semaphore1",0);     // To ensure the Signal comes before the wait
Semaphore test4_done("test4_done",0); // So that TestSuite knows when Test 4 is
                                // done

// --------------------------------------------------
// test4_waiter()
//     These threads will wait on the test4_condition1 condition variable.  All
//     test4_waiters will be released
// --------------------------------------------------
void test4_waiter() 
{
    test4_lock1.Acquire();
    test4_semaphore1.V();          // Let the signaller know we're ready to wait
    printf("%s: Lock %s acquired, waiting on %s\n",currentThread->getName(),
           test4_lock1.getName(), test4_condition1.getName());
    test4_condition1.Wait(&test4_lock1);
    printf("%s: freed from %s\n",currentThread->getName(), test4_condition1.getName());
    test4_lock1.Release();
    test4_done.V();
}


// --------------------------------------------------
// test4_signaller()
//     This thread will broadcast to the test4_condition1 condition variable.
//     All test4_waiters will be released
// --------------------------------------------------
void test4_signaller() 
{

    // Don't broadcast until someone's waiting
   
    for ( int i = 0; i < 5 ; i++ )
        test4_semaphore1.P();
    test4_lock1.Acquire();
    printf("%s: Lock %s acquired, broadcasting %s\n",currentThread->getName(),
           test4_lock1.getName(), test4_condition1.getName());
    test4_condition1.Broadcast(&test4_lock1);
    printf("%s: Releasing %s\n",currentThread->getName(), test4_lock1.getName());
    test4_lock1.Release();
    test4_done.V();
}

// --------------------------------------------------
// initialize threads for test4
// --------------------------------------------------
void test4()
{
    Thread *t;
    char *name;
    int i = 0;
	
	printf("Starting Test 4\n");

    while (i < 5) 
	{
        name = new char [20];
        sprintf(name,"test4_waiter%d",i);
        t = new Thread(name);
        t->Fork((VoidFunctionPtr)test4_waiter,0);
		i ++;
	}
	
    t = new Thread("test4_signaller");
    t->Fork((VoidFunctionPtr)test4_signaller,0);

    // Wait for Test 4 to complete
    i = 0;
	while(i < 6)
	{
		test4_done.P();
		i++;
	}
}


// --------------------------------------------------
// Test 5 
// Show that Signalling a thread waiting under one lock
// while holding another is a Fatal error
// --------------------------------------------------
Lock test5_lock1("test5_lock1");            // For mutual exclusion
Lock test5_lock2("test5_lock2");            // Second lock for the bad behavior
Condition test5_condition1("test5_condition1");       // The condition variable to test
Semaphore test5_semaphore1("test5_semaphore1",0);     // To make sure test5_thread2 acquires the lock after
                                // test5_thread1

// --------------------------------------------------
// test5_thread1() -- test 5 thread 1
//     This thread will wait on a condition under test5_lock1
// --------------------------------------------------
void test5_thread1() 
{
    test5_lock1.Acquire();
    test5_semaphore1.V();  // release test5_thread2
    printf("%s: Lock %s acquired, waiting on %s\n",currentThread->getName(),
           test5_lock1.getName(), test5_condition1.getName());
    test5_condition1.Wait(&test5_lock1);
    printf("%s: Releasing Lock %s\n",currentThread->getName(),
           test5_lock1.getName());
    test5_lock1.Release();
}

// --------------------------------------------------
// test5_thread1() -- test 5 thread 1
//     This thread will wait on a test5_condition1 condition under test5_lock2, which is
//     a Fatal error
// --------------------------------------------------
void test5_thread2() 
{
    test5_semaphore1.P();  // Wait for test5_thread1 to get into the monitor
    test5_lock1.Acquire();
    test5_lock2.Acquire();
    printf("%s: Lock %s acquired, signalling %s\n",currentThread->getName(),
           test5_lock2.getName(), test5_condition1.getName());
    test5_condition1.Signal(&test5_lock2);
    printf("%s: Releasing Lock %s\n",currentThread->getName(),
           test5_lock2.getName());
    test5_lock2.Release();
    printf("%s: Releasing Lock %s\n",currentThread->getName(),
           test5_lock1.getName());
    test5_lock1.Release();
}

// --------------------------------------------------
// initialize threads for test5
// --------------------------------------------------
void test5()
{
    Thread *t;
	
    printf("Starting Test 5.\n");
    printf("Note that it is an error if thread test5_thread1 completes\n");

    t = new Thread("test5_thread1");
    t->Fork((VoidFunctionPtr)test5_thread1,0);

    t = new Thread("test5_thread2");
    t->Fork((VoidFunctionPtr)test5_thread2,0);
}


// Run test 1 through 5
void TestSuite() 
{
	// --------------------------------------------------
	// Test 1 
	// Show that a thread trying to release a lock it does not
	// hold does not work
	// --------------------------------------------------
	test1();	

	// --------------------------------------------------
	// Test 2
	// Show that Signals are not stored -- a Signal with no
	// thread waiting is ignored
	// --------------------------------------------------
	test2();
		
	// --------------------------------------------------
	// Test 3
	// Show that Signal only wakes 1 thread
	// --------------------------------------------------
	test3();
	
	// --------------------------------------------------
	// Test 4
	// Show that Broadcast wakes all waiting threads
	// --------------------------------------------------
	test4();
	
	// --------------------------------------------------
	// Test 5 
	// Show that Signalling a thread waiting under one lock
	// while holding another is a Fatal error
	// --------------------------------------------------
	test5();   
}
