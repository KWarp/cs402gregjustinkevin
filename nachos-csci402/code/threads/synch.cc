// synch.cc 
//	Routines for synchronizing threads.  Three kinds of
//	synchronization routines are defined here: semaphores, locks 
//   	and condition variables (the implementation of the last two
//	are left to the reader).
//
// Any implementation of a synchronization routine needs some
// primitive atomic operation.  We assume Nachos is running on
// a uniprocessor, and thus atomicity can be provided by
// turning off interrupts.  While interrupts are disabled, no
// context switch can occur, and thus the current thread is guaranteed
// to hold the CPU throughout, until interrupts are reenabled.
//
// Because some of these routines might be called with interrupts
// already disabled (Semaphore::V for one), instead of turning
// on interrupts at the end of the atomic operation, we always simply
// re-set the interrupt state back to its original value (whether
// that be disabled or enabled).
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "synch.h"
#include "system.h"

//----------------------------------------------------------------------
// Semaphore::Semaphore
// 	Initialize a semaphore, so that it can be used for synchronization.
//
//	"debugName" is an arbitrary name, useful for debugging.
//	"initialValue" is the initial value of the semaphore.
//----------------------------------------------------------------------

Semaphore::Semaphore(char* debugName, int initialValue)
{
    name = debugName;
    value = initialValue;
    queue = new List;
}

//----------------------------------------------------------------------
// Semaphore::Semaphore
// 	De-allocate semaphore, when no longer needed.  Assume no one
//	is still waiting on the semaphore!
//----------------------------------------------------------------------

Semaphore::~Semaphore()
{
    delete queue;
}

//----------------------------------------------------------------------
// Semaphore::P
// 	Wait until semaphore value > 0, then decrement.  Checking the
//	value and decrementing must be done atomically, so we
//	need to disable interrupts before checking the value.
//
//	Note that Thread::Sleep assumes that interrupts are disabled
//	when it is called.
//----------------------------------------------------------------------

void
Semaphore::P()
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);	// disable interrupts
    
    while (value == 0) { 			// semaphore not available
	queue->Append((void *)currentThread);	// so go to sleep
	currentThread->Sleep();
    } 
    value--; 					// semaphore available, 
						// consume its value
    
    (void) interrupt->SetLevel(oldLevel);	// re-enable interrupts
}

//----------------------------------------------------------------------
// Semaphore::V
// 	Increment semaphore value, waking up a waiter if necessary.
//	As with P(), this operation must be atomic, so we need to disable
//	interrupts.  Scheduler::ReadyToRun() assumes that threads
//	are disabled when it is called.
//----------------------------------------------------------------------

void
Semaphore::V()
{
    Thread *thread;
    IntStatus oldLevel = interrupt->SetLevel(IntOff);

    thread = (Thread *)queue->Remove();
    if (thread != NULL)	   // make thread ready, consuming the V immediately
	scheduler->ReadyToRun(thread);
    value++;
    (void) interrupt->SetLevel(oldLevel);
}

// Dummy functions -- so we can compile our later assignments 
// Note -- without a correct implementation of Condition::Wait(), 
// the test case in the network assignment won't work!
Lock::Lock(char* debugName)
{
	#ifdef CHANGED
		name = debugName;
		state = FREE;
		waitingThreads = new List;
		owner = NULL;
	#endif
}

Lock::~Lock()
{
	#ifdef CHANGED
		delete waitingThreads;
	#endif
}

void Lock::Acquire()
{
	#ifdef CHANGED
		// Disable interrupts.
		IntStatus old = interrupt->SetLevel(IntOff);

		// If we already own the lock.
		if (isHeldByCurrentThread())
		{
			// Restore interrupts and return.
			interrupt->SetLevel(old);
			return;
		}

		// If the lock is not already owned by somebody else.
		if (state == FREE)
		{
			// Take the lock and make ourselves the owner.
			state = BUSY;
			owner = currentThread;
		}
		else // if (state == BUSY)
		{
			waitingThreads->Append((void*)currentThread);
			currentThread->Sleep();
		}

		// Restore interrupts.
		interrupt->SetLevel(old);
	#endif
}

void Lock::Release()
{
	#ifdef CHANGED
		// Disable interrupts.
		IntStatus old = interrupt->SetLevel(IntOff);

		if (!isHeldByCurrentThread())
		{
			// printf("Lock::Error - Release called by non-owner");
			// Restore interrupts and return.
			interrupt->SetLevel(old);
			return;
		}

		if (!waitingThreads->IsEmpty())
		{
			// Wake up 1 Thread from waitingThreads.
			Thread* thread = (Thread*)waitingThreads->Remove();
			scheduler->ReadyToRun(thread);

			// Make it the new owner.
			owner = thread;
		}
		else // There are no threads in waitingThreads.
		{
            // Make the Lock available and clear lock ownership.
            state = FREE;
            owner = NULL;
		}

		// Restore interrupts.
		interrupt->SetLevel(old);
	#endif
}

bool Lock::isHeldByCurrentThread()
{
	#ifdef CHANGED
		return currentThread == owner;
	#endif
}

Condition::Condition(char* debugName)
{
	#ifdef CHANGED
		name = debugName;
		waitingLock = NULL;
		waitingThreads = new List;
	#endif
}

Condition::~Condition()
{
	#ifdef CHANGED
		delete waitingThreads;
	#endif
}

void Condition::Wait(Lock* conditionLock)
{
	#ifdef CHANGED
		// Disable interrupts.
		IntStatus old = interrupt->SetLevel(IntOff);

		if (conditionLock == NULL)
		{
			printf("Condition::Error - conditionLock cannot be NULL\n");
			// Restore interrupts and return.
			interrupt->SetLevel(old);
			return;
		}

		if (waitingLock == NULL)
		{
			// First thread calling wait.
			waitingLock = conditionLock;	// Save the lock.
		}

		// Make sure conditionLock matches waitingLock.
		if (conditionLock != waitingLock)
		{
			// Lock must match.
			printf("Condition::Error - Invalid conditionLock\n");
			// Restore interrupts and return.
			interrupt->SetLevel(old);
			return;
		}

		// Everything OK to be a "waitingThread" at this point.
		
		// Add thread to waitingThreads and sleep.
		waitingThreads->Append((void*)currentThread);
		conditionLock->Release();
		currentThread->Sleep();
		conditionLock->Acquire();

		// Restore interrupts.
		interrupt->SetLevel(old);
	#endif
}

void Condition::Signal(Lock* conditionLock)
{
	#ifdef CHANGED
		// Disable interrupts.
		IntStatus old = interrupt->SetLevel(IntOff);

		// If there are no waitingThreads to signal.
		if (waitingThreads->IsEmpty())
		{
			// Restore interrupts and return.
			interrupt->SetLevel(old);
			return;
		}

		if (waitingLock != conditionLock)
		{
			// Lock must match.
			printf("Condition::Error - Invalid conditionLock\n");
			// Restore interrupts and return.
			interrupt->SetLevel(old);
			return;
		}

		// Wake up 1 Thread from waitingThreads.
		Thread* thread = (Thread*)waitingThreads->Remove();
		scheduler->ReadyToRun(thread);

		if (waitingThreads->IsEmpty())
			waitingLock = NULL;

		// Restore interrupts.
		interrupt->SetLevel(old);
	#endif
}

void Condition::Broadcast(Lock* conditionLock)
{
	#ifdef CHANGED
		while (!waitingThreads->IsEmpty())
			Signal(conditionLock);
	#endif
}

/* =============================================================	
 * UTILITY METHODS
 * =============================================================*/	

const int MaxNumLocks = 500;

Lock* locks[MaxNumLocks] = {};
int numLocks = 0;

Condition* CVs[MaxNumLocks]= {};
int numCVs = 0;

int numThreads = 0;
void Fork(int func)
{
    Thread * thread;
    char *name;
	numThreads ++;
    name = new char [20];
    sprintf(name,"Thread #%d",numThreads);
	thread = new Thread(name);

	thread->Fork((VoidFunctionPtr)func,0);
}

int GetLock()
{
	locks[numLocks] = new Lock("Lock #"+numLocks);
	numLocks = numLocks + 1;
	return numLocks-1;
}

int GetCV()
{
	CVs[numCVs] = new Condition("Condition #"+numCVs );
	numCVs = numCVs +1;
	return numCVs -1;
}

void Acquire(int lock)
{
	locks[lock]->Acquire();
}

void Release(int lock)
{
	locks[lock]->Release();
}

void Wait(int CV, int lock)
{
	CVs[CV]->Wait(locks[lock]);
}

void Signal(int CV, int lock)
{
	CVs[CV]->Signal(locks[lock]);
}

void Broadcast(int CV, int lock)
{
	CVs[CV]->Broadcast(locks[lock]);
}

void Yield(int time)
{
	while(time > 0)
	{
		currentThread->Yield();
		time -= 1;
	}
}

int randomNumber(int count)
{
	return 4;
}

void PrintOut(const char* vaddr, int len)
{
	//for(int i = 0 ; i < len; i ++)
	printf(vaddr);
}

void PrintNumber(int number)
{
	printf("%d", number);
}
