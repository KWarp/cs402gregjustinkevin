// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#ifdef NETWORK
#include "../network/netcall.h"
#endif

#include "copyright.h"
#include "system.h"
#include "syscall.h"
#include "SynchManager.h"
#include <stdio.h>
#include <iostream>

extern "C" { int bzero(char *, int); };

using namespace std;

#ifdef CHANGED
  #include "../threads/synch.h"
  static SynchManager* synchManager = new SynchManager();
  static Lock* forkLock = new Lock("Fork Lock");
  static Lock* execLock = new Lock("Exec Lock");
  static Lock* printNumberLock = new Lock("PrintNumber Lock");
  static Lock* printOutLock = new Lock("PrintOut Lock");
  
  #ifdef NETWORK
	AddrSpace* processIDs[10];
	int procIDIndex = 0;

	void assignMailID(AddrSpace* spaceID);
	int getMailID();
  #endif
#endif

int copyin(unsigned int vaddr, int len, char *buf)
{
  // Copy len bytes from the current thread's virtual address vaddr.
  // Return the number of bytes so read, or -1 if an error occors.
  // Errors can generally mean a bad virtual address was passed in.
  bool result;
  int n = 0;			// The number of bytes copied in
  int *paddr = new int;

  while (n >= 0 && n < len)
  {
    result = machine->ReadMem(vaddr, 1, paddr);
    while (!result) // FALL 09 CHANGES
      result = machine->ReadMem(vaddr, 1, paddr); // FALL 09 CHANGES: TO HANDLE PAGE FAULT IN THE ReadMem SYS CALL
    
    buf[n++] = *paddr;
    if (!result )
    {
      //translation failed
      return -1;
    }
    vaddr++;
  }

  delete paddr;
  return len;
}

int copyout(unsigned int vaddr, int len, char *buf)
{
  // Copy len bytes to the current thread's virtual address vaddr.
  // Return the number of bytes so written, or -1 if an error
  // occors.  Errors can generally mean a bad virtual address was
  // passed in.
  bool result;
  int n = 0;			// The number of bytes copied in

  while (n >= 0 && n < len)
  {
    // Note that we check every byte's address
    result = machine->WriteMem(vaddr, 1, (int)(buf[n++]));
    if (!result)
    {
      //translation failed
      return -1;
    }
    vaddr++;
  }
  return n;
}

void Create_Syscall(unsigned int vaddr, int len)
{
  // Create the file with the name in the user buffer pointed to by
  // vaddr.  The file name is at most MAXFILENAME chars long.  No
  // way to return errors, though...
  char *buf = new char[len+1];	// Kernel buffer to put the name in

  if (!buf) return;

  if( copyin(vaddr,len,buf) == -1 )
  {
    printf("%s","Bad pointer passed to Create\n");
    delete buf;
    return;
  }

  buf[len]='\0';

  fileSystem->Create(buf,0);
  delete[] buf;
  return;
}

int Open_Syscall(unsigned int vaddr, int len)
{
  // Open the file with the name in the user buffer pointed to by
  // vaddr.  The file name is at most MAXFILENAME chars long.  If
  // the file is opened successfully, it is put in the address
  // space's file table and an id returned that can find the file
  // later.  If there are any errors, -1 is returned.
  char *buf = new char[len+1];	// Kernel buffer to put the name in
  OpenFile *f;			// The new open file
  int id;				// The openfile id

  if (!buf)
  {
    printf("%s","Can't allocate kernel buffer in Open\n");
    return -1;
  }

  if (copyin(vaddr,len,buf) == -1 )
  {
    printf("%s","Bad pointer passed to Open\n");
    delete[] buf;
    return -1;
  }

  buf[len]='\0';
  f = fileSystem->Open(buf);
  delete[] buf;

  if (f)
  {
    if ((id = currentThread->space->fileTable.Put(f)) == -1)
      delete f;
    return id;
  }
  else
    return -1;
}

void Write_Syscall(unsigned int vaddr, int len, int id)
{
  // Write the buffer to the given disk file.  If ConsoleOutput is
  // the fileID, data goes to the synchronized console instead.  If
  // a Write arrives for the synchronized Console, and no such
  // console exists, create one. For disk files, the file is looked
  // up in the current address space's open file table and used as
  // the target of the write.

  char *buf;		// Kernel buffer for output
  OpenFile *f;	// Open file for output

  if (id == ConsoleInput) 
    return;

  if (!(buf = new char[len]) )
  {
    printf("%s","Error allocating kernel buffer for write!\n");
    return;
  }
  else
  {
    if (copyin(vaddr,len,buf) == -1 )
    {
      printf("%s","Bad pointer passed to to write: data not written\n");
      delete[] buf;
      return;
    }
  }

  if ( id == ConsoleOutput)
  {
    for (int ii=0; ii<len; ii++)
      printf("%c", buf[ii]);
  }
  else
  {
    if ((f = (OpenFile *) currentThread->space->fileTable.Get(id)))
      f->Write(buf, len);
    else
    {
      printf("%s","Bad OpenFileId passed to Write\n");
      len = -1;
    }
  }

  delete[] buf;
 }

int Read_Syscall(unsigned int vaddr, int len, int id) 
{
  // Write the buffer to the given disk file.  If ConsoleOutput is
  // the fileID, data goes to the synchronized console instead.  If
  // a Write arrives for the synchronized Console, and no such
  // console exists, create one.    We reuse len as the number of bytes
  // read, which is an unnessecary savings of space.
  char *buf;		// Kernel buffer for input
  OpenFile *f;	// Open file for output

  if (id == ConsoleOutput) 
    return -1;

  if (!(buf = new char[len]) )
  {
    printf("%s", "Error allocating kernel buffer in Read\n");
    return -1;
  }

  if (id == ConsoleInput)
  {
    //Reading from the keyboard
    scanf("%s", buf);

    if (copyout(vaddr, len, buf) == -1)
      printf("%s", "Bad pointer passed to Read: data not copied\n");
  }
  else
  {
    if ((f = (OpenFile *) currentThread->space->fileTable.Get(id)))
    {
      len = f->Read(buf, len);
      if (len > 0)
      {
        //Read something from the file. Put into user's address space
        if (copyout(vaddr, len, buf) == -1)
        printf("%s", "Bad pointer passed to Read: data not copied\n");
      }
    }
    else
    {
      printf("%s", "Bad OpenFileId passed to Read\n");
      len = -1;
    }
  }

  delete[] buf;
  return len;
}

void Close_Syscall(int fd)
{
  // Close the file associated with id fd.  No error reporting.
  OpenFile *f = (OpenFile *)currentThread->space->fileTable.Remove(fd);

  if (f)
    delete f;
  else
    printf("%s","Tried to close an unopen file\n");
}

#ifdef CHANGED
/*
 * Implementations of Syscalls for Project 2
 */

void Yield_Syscall()
{
	currentThread->Yield();
}

int CreateLock_Syscall(int vaddr)
{
	int len = 3;//omfg this needs to be a fucking parameter.
	char* name = new char[len];
	
	if ( copyin(vaddr,len,name) == -1 ) 
	{
		printf("%s","Bad pointer passed to CreateLock: data not written\n");
		delete[] name;
		return -1;
    }

#ifndef NETWORK
	return synchManager->CreateLock();
#else
	return Request(CREATELOCK, name, getMailID());
#endif
}

void DestroyLock_Syscall(int index)
{
#ifndef NETWORK
	synchManager->DestroyLock(index);
#else
	char* indexBuf = new char[16];
	sprintf(indexBuf,"%d",index);
        
	Request(DESTROYLOCK, indexBuf, getMailID());
#endif
}

void Acquire_Syscall(int index)
{
#ifndef NETWORK
	synchManager->Acquire(index);
#else
	char* indexBuf = new char[16];
	sprintf(indexBuf,"%d",index);
        
	Request(ACQUIRE, indexBuf, getMailID());
#endif
}

void Release_Syscall(int index)
{
#ifndef NETWORK
	synchManager->Release(index);
#else
	char* indexBuf = new char[16];
	sprintf(indexBuf,"%d",index);
        
	Request(RELEASE, indexBuf, getMailID());
#endif
}

void Exec_Kernel_Thread()
{
  execLock->Acquire();
    currentThread->space->InitRegisters();
    currentThread->space->RestoreState();
  execLock->Release();
  machine->Run();
}

void Exec_Syscall(unsigned int executableFileName)
{
  int len = 32; // Max length of the file name. Should we make len a parameter instead?
  execLock->Acquire();
    // Copy the executable file name into buf.
    char *buf = new char[len]; // Kernel buffer to put the name in
    if (!buf)
    {
      printf("%s", "Can't allocate kernel buffer in Exec\n");
      execLock->Release();
      return;
    }
    if (copyin(executableFileName,len,buf) == -1)
    {
      printf("%s", "Bad pointer passed to Exec\n");
      delete buf;
      execLock->Release();
      return;
    }
    
    // Open the executable file.
    buf[len] = '\0';
    printf("%s\n", buf);
    OpenFile* executable = fileSystem->Open(buf);
    if (executable == NULL)
    {
      printf("Failed to open file: %s\n", buf);
      delete buf;
      execLock->Release();
      return;
    }   
    
    Thread* thread = new Thread("Exec'd Process Thread");
    thread->space = new AddrSpace(executable);
    processTable->addProcess(thread->space);
    
    #ifdef USE_TLB
      // Invalidate all entries in the TLB.
      IntStatus old = interrupt->SetLevel(IntOff);
        for (int i = 0; i < TLBSize; ++i)
          machine->tlb[i].valid = false;
      interrupt->SetLevel(old);
    #endif
    
    // delete executable;
    delete buf;
  execLock->Release();

  #ifdef NETWORK
	assignMailID(thread->space);
  #endif
  thread->Fork((VoidFunctionPtr)Exec_Kernel_Thread, 0);  
}

// Pass a pointer to this function when forking a  new process in kernel space.
void Fork_Kernel_Thread(unsigned int functionPtr)
{
  forkLock->Acquire();
    // Setup registers.
    // currentThread->space->InitRegisters();
    
    currentThread->space->RestoreState();
    machine->WriteRegister(PCReg, functionPtr);
    machine->WriteRegister(NextPCReg, functionPtr + 4);
    machine->WriteRegister(StackReg, currentThread->stackStartIndex * PageSize - 16);
  forkLock->Release();
  
  machine->Run();
}

void Fork_Syscall(unsigned int functionPtr)
{
  forkLock->Acquire();
    // Make sure the functionPtr refers to a valid address.
    if (functionPtr <= 0 || functionPtr >= PageSize * NumPhysPages)
    {
      forkLock->Release();
      printf("Bad function pointer passed to Fork: %i\n", functionPtr);
      return;
    }

    // Create the new thread.
    Thread* thread = new Thread("Forked Thread");

    // Set the address space of the new thread.
    thread->space = currentThread->space;

    // Allocate stack space for new thread.
    thread->stackStartIndex = currentThread->space->AllocateStack();

    // Add new thread to current process in processTable.
    processTable->addThread(thread->space);

    #ifdef USE_TLB
      // Invalidate all entries in the TLB.
      IntStatus old = interrupt->SetLevel(IntOff);
        for (int i = 0; i < TLBSize; ++i)
          machine->tlb[i].valid = false;
      interrupt->SetLevel(old);
    #endif

    // thread->space->RestoreState();
  forkLock->Release();

  // Actually Fork the new thread.
  thread->Fork((VoidFunctionPtr)Fork_Kernel_Thread, (unsigned int)functionPtr);
}

void Exit_Syscall(int status)
{
  printf("numThreads: %d\n", processTable->getNumThreads());
  printf("Exiting with status: %d\n", status);
  
  #if 0
  if (processTable->getNumThreads() > 1)
  {
    processTable->killThread(currentThread->space);
    if (processTable->getNumThreadsForProcess(currentThread->space) == 0)
      delete currentThread->space;
    currentThread->Finish();
    return;
  }
  else
  {
    currentThread->Finish();
  }
  #else
    currentThread->Finish();
  #endif
}

int CreateCondition_Syscall(int vaddr)
{
	int len = 3;//omfg this needs to be a fucking parameter.
	char* name = new char[len];
	
	if ( copyin(vaddr,len,name) == -1 ) 
	{
		printf("%s","Bad pointer passed to CreateLock: data not written\n");
		delete[] name;
		return -1;
    }

#ifndef NETWORK
	return synchManager->CreateCondition();
#else
	return Request(CREATECV, name, getMailID());
#endif
}

void DestroyCondition_Syscall(int index)
{
#ifndef NETWORK
	synchManager->DestroyCondition(index);
#else
	char* indexBuf = new char[16];
	sprintf(indexBuf,"%d",index);
        
	Request(DESTROYCV, indexBuf, getMailID());
#endif
}

void Signal_Syscall(int cv, int lock)
{
#ifndef NETWORK
	return synchManager->Signal(cv, lock);
#else
	char* indexBuf = new char[16];
    sprintf(indexBuf,"%d_%d", cv, lock);
	Request(SIGNAL, indexBuf, getMailID());
#endif
}

void Wait_Syscall(int cv, int lock)
{
#ifndef NETWORK
	return synchManager->Wait(cv, lock);
#else
	char* indexBuf = new char[16];
    sprintf(indexBuf,"%d_%d", cv, lock);
	Request(WAIT, indexBuf, getMailID());
#endif
}

void Broadcast_Syscall(int cv, int lock)
{
#ifndef NETWORK
	return synchManager->Broadcast(cv, lock);
#else
	char* indexBuf = new char[16];
    sprintf(indexBuf,"%d_%d", cv, lock);
	Request(BROADCAST, indexBuf, getMailID());
#endif
}

#ifdef NETWORK
int CreateMV_Syscall(int vaddr)
{
	int len = 3;//omfg this needs to be a fucking parameter.
	char* name = new char[len];
	
	if ( copyin(vaddr,len,name) == -1 ) 
	{
		printf("%s","Bad pointer passed to CreateLock: data not written\n");
		delete[] name;
		return -1;
  }

	return Request(CREATEMV, name, getMailID());
}

int SetMV_Syscall(int mv, int value)
{
	char* indexBuf = new char[16];
	sprintf(indexBuf,"%d_%d", mv, value);
  
  return Request(SETMV, indexBuf, getMailID());
}

int GetMV_Syscall(int mv)
{
	char* indexBuf = new char[16];
	sprintf(indexBuf,"%d", mv);
  
  return Request(GETMV, indexBuf, getMailID());
}

int DestroyMV_Syscall(int mv)
{
	char* indexBuf = new char[16];
	sprintf(indexBuf,"%d", mv);
  
  return Request(DESTROYMV, indexBuf, getMailID());
}
#endif

int RandomNumber_Syscall(int count)
{
	int i = Random() % (count);
	return i;
}

void PrintOut_Syscall(unsigned int vaddr, int len)
{
  printOutLock->Acquire();
    Write_Syscall(vaddr, len, ConsoleOutput);
  printOutLock->Release();
}

void PrintNumber_Syscall(int i)
{
  printNumberLock->Acquire();
    printf("%d", i);
  printNumberLock->Release();
}

#ifdef USE_TLB

// Returns -1 if not found.
int findIPTIndex(int vpn)
{
  int ppn = -1;
  for (int i = 0; i < NumPhysPages; ++i)
  {
    if (ipt[i].valid &&
        ipt[i].virtualPage == vpn &&
        ipt[i].processID == currentThread->space)
    {
      ppn = i;
      break;
    }
  }
  return ppn;
}


int evictAPage() 
{
  int ppn = -1;
  if (useRandomPageEviction)
  {
    // do RANDOM policy
    //printf("USING RANDOM POLICY \n");
    ppn = Random() % NumPhysPages;
  }
  else
  {
    // do FIFO policy
    
    // evict first entry from the queue
    int* element = (int*)ppnQueue->Remove();
    ppn = element[0];
    delete[] element;
    //printf("FIFO ppn after: %d\n", ppn);
  } 
  
  //printf("Evicting page ppn = %d\n", ppn);
  // needed???
  // ppnInUseBitMap->Clear(ppn);
  
  // CHECK TO SEE IF PAGE BELONGS IN CURRENT PROCESS
  // IF SO, INVALIDATE TLB ENTRY
  if (ipt[ppn].processID == currentThread->space)
  {
    IntStatus old = interrupt->SetLevel(IntOff);
      //printf("ipt[ppn].processID EQUALS currentThread->space \n");
      for (int i = 0; i < TLBSize; ++i)
      {
        if (machine->tlb[i].valid &&
            machine->tlb[i].physicalPage == ppn)
        {
          machine->tlb[i].valid = false;
          break;
        }
      }
    interrupt->SetLevel(old);
  }
  
  if (ipt[ppn].dirty)
  {
    // write to swap file
    // get a swap page if this entry doesn't have one
    if (ipt[ppn].processID->pageTable[ipt[ppn].virtualPage].swapPageIndex < 0)
      ipt[ppn].processID->pageTable[ipt[ppn].virtualPage].swapPageIndex = swapFile->GetSwapPageIndex();
    
    // printf("- Writing (vpn %d, ppn %d, swapPageIndex %d) to swap file for process %d\n", 
    //       ipt[ppn].virtualPage, ppn, ipt[ppn].processID->pageTable[ipt[ppn].virtualPage].swapPageIndex, 
    //       (int)ipt[ppn].processID);
    swapFile->Store(ipt[ppn].processID->pageTable[ipt[ppn].virtualPage].swapPageIndex, ppn);

    // update page location
    ipt[ppn].processID->pageTable[ipt[ppn].virtualPage].location = IN_SWAP_FILE;
  }
  
  return ppn;
}

int loadPageIntoIPT(int vpn)
{
  //printf("Entering loadPageIntoIPT()\n");

  // Load the page into memory from the correct location.
  int ppn = ppnInUseBitMap->Find();
  
  //printf("ppn = %d\n", ppn);
  
  // failed to find a page in memory
  if(ppn < 0)
     ppn = evictAPage();
  
  if(!useRandomPageEviction) 
  {
    // using FIFO Policy
    // add ppn to back of queue
    //printf("FIFO ppn before: %d\n", ppn);
    int* element = new int[1];
    element[0] = ppn;
    ppnQueue->Append((void*)element);
  }
  
  //printf("Locating vpn = %d\n", vpn);
  
  if (currentThread->space->pageTable[vpn].location == IN_EXECUTABLE)
  {
    //printf("Loading from executable (vpn %d, ppn %d, byteOffset %d) for process %d\n", vpn, ppn, currentThread->space->pageTable[vpn].byteOffset, (int)currentThread->space);
    currentThread->space->pageTable[vpn].executable->ReadAt(&(machine->mainMemory[ppn * PageSize]), PageSize,
                                                            currentThread->space->pageTable[vpn].byteOffset);
  }
  else if (currentThread->space->pageTable[vpn].location == IN_SWAP_FILE)
  {
    ASSERT(currentThread->space->pageTable[vpn].swapPageIndex >= 0);
    
    //printf("- Loading from Swap File (vpn %d, ppn %d, swapPageIndex %d) for process %d\n", 
    //    vpn, ppn, currentThread->space->pageTable[vpn].swapPageIndex, (int)currentThread->space);
    swapFile->Load(currentThread->space->pageTable[vpn].swapPageIndex, ppn);
    
  }
  else
  {
    //printf("Loading from NEITHER (vpn %d, ppn %d)\n", vpn, ppn);
    bzero(&(machine->mainMemory[ppn * PageSize]), PageSize);
  }
  
  // Populate ipt.
  ipt[ppn].virtualPage  = vpn;
  ipt[ppn].physicalPage = ppn;
  ipt[ppn].valid        = TRUE;
  ipt[ppn].use          = FALSE;
  ipt[ppn].dirty        = FALSE;
  ipt[ppn].readOnly     = FALSE;  // If the code segment was entirely on a separate page, we could set its pages to be read-only.
  ipt[ppn].processID    = currentThread->space;
  
  //printf("Leaving loadPageIntoIPT()\n");
  
  return ppn;
}


void updateTLBFromIPT(int ppn)
{
  // Disable interrupts while messing with tlb.
  IntStatus old = interrupt->SetLevel(IntOff);

  // Propagate the tlb's dirty bit to the ipt.
  if (machine->tlb[currentTLBIndex].valid)
    ipt[machine->tlb[currentTLBIndex].physicalPage].dirty = machine->tlb[currentTLBIndex].dirty;
  
  // Load the proper ipt values into the tlb.
  machine->tlb[currentTLBIndex].virtualPage  = ipt[ppn].virtualPage;
  machine->tlb[currentTLBIndex].physicalPage = ipt[ppn].physicalPage;
  machine->tlb[currentTLBIndex].valid        = ipt[ppn].valid;
  machine->tlb[currentTLBIndex].readOnly     = ipt[ppn].readOnly;
  machine->tlb[currentTLBIndex].use          = ipt[ppn].use;
  machine->tlb[currentTLBIndex].dirty        = ipt[ppn].dirty;
  
  // Update currentTLBIndex for next time.
  currentTLBIndex = ++currentTLBIndex % TLBSize;

  // Re-enable interrupts.
  interrupt->SetLevel(old);
}


void HandlePageFault()
{
    // For now, read the value from the PageTable straight into the TLB.
    int vpn = machine->ReadRegister(BadVAddrReg) / PageSize;

    ppnInUseLock->Acquire();
    
    // Look in the ipt if the vpn for the currentProcess is already in memory.
    int ppn = findIPTIndex(vpn);
    
    // If the vpn was not found in the ipt, load it into the ipt, overwriting an old value.
    if (ppn < 0)
      ppn = loadPageIntoIPT(vpn);
    
    // Update the tlb from the ipt.
    updateTLBFromIPT(ppn);
    
    ppnInUseLock->Release();
}

#endif // USE_TLB

#ifdef NETWORK
void assignMailID(AddrSpace* spaceIdentifier)
{
	if(procIDIndex > 9)
	{
		printf("Too many processes\n");
		return;
	}

	for(int i = 0; i < 10; i++)
	{
		if(processIDs[i] == spaceIdentifier)
		{
			printf("Process already in instance process array!\n");
			return;
		}
	}

	processIDs[procIDIndex++] = spaceIdentifier;
	return;
}

int getMailID()
{
	int i = 0;
	for(i = 0; i < 10; i++)
	{
		if(processIDs[i] == currentThread->space)
		{
			return i;
		}
	}
	// printf("Failed to find mailID %d\n",i);
	return -1;
}
#endif /* NETWORK */

#endif /* CHANGED */

void ExceptionHandler(ExceptionType which)
{
  int type = machine->ReadRegister(2); // Which syscall?
  int rv=0; 	// the return value from a syscall

  if (which == SyscallException)
  {
    switch (type)
    {
      default:
        DEBUG('a', "Unknown syscall - shutting down.\n");
        /* Fall through */
      case SC_Halt:
        DEBUG('a', "Shutdown, initiated by user program.\n");
        interrupt->Halt();
        break;
      case SC_Create:
        DEBUG('a', "Create syscall.\n");
        Create_Syscall(machine->ReadRegister(4), machine->ReadRegister(5));
        break;
      case SC_Open:
        DEBUG('a', "Open syscall.\n");
        rv = Open_Syscall(machine->ReadRegister(4), machine->ReadRegister(5));
        break;
      case SC_Write:
        DEBUG('a', "Write syscall.\n");
        Write_Syscall(machine->ReadRegister(4),
                      machine->ReadRegister(5),
                      machine->ReadRegister(6));
        break;
      case SC_Read:
        DEBUG('a', "Read syscall.\n");
        rv = Read_Syscall(machine->ReadRegister(4),
                          machine->ReadRegister(5),
                          machine->ReadRegister(6));
        break;
      case SC_Close:
        DEBUG('a', "Close syscall.\n");
        Close_Syscall(machine->ReadRegister(4));
        break;

      #ifdef CHANGED
        case SC_Yield:
          DEBUG('a', "Yield syscall. \n");
          Yield_Syscall();
          break;
        case SC_CreateLock:
          DEBUG('a', "CreateLock syscall. \n");
          rv = CreateLock_Syscall(machine->ReadRegister(4));
          break;
        case SC_DestroyLock:
          DEBUG('a', "DestroyLock syscall. \n");
          DestroyLock_Syscall(machine->ReadRegister(4));
          break;
        case SC_Acquire:
          DEBUG('a', "Acquire syscall. \n");
          Acquire_Syscall(machine->ReadRegister(4));
          break;
        case SC_Release:
          DEBUG('a', "Release syscall. \n");
          Release_Syscall(machine->ReadRegister(4));
          break;
        case SC_CreateCondition:
          DEBUG('a', "CreateCondition syscall.\n");
          rv = CreateCondition_Syscall(machine->ReadRegister(4));
          break;
        case SC_DestroyCondition:
          DEBUG('a', "DestroyCondition syscall. \n");
          DestroyCondition_Syscall(machine->ReadRegister(4));
          break;
        case SC_Signal:
          DEBUG('a', "Signal syscall. \n");
          Signal_Syscall(machine->ReadRegister(4), machine->ReadRegister(5));
          break;
        case SC_Wait:
          DEBUG('a', "Wait syscall. \n");
          Wait_Syscall(machine->ReadRegister(4), machine->ReadRegister(5));
          break;
        case SC_Broadcast:
          DEBUG('a', "Broadcast syscall. \n");
          Broadcast_Syscall(machine->ReadRegister(4), machine->ReadRegister(5));
          break;
        case SC_Fork:
          DEBUG('a', "Fork syscall. \n");
          Fork_Syscall((unsigned int)machine->ReadRegister(4));
          break;
        case SC_Exec:
          DEBUG('a', "Exec syscall. \n");
          Exec_Syscall((unsigned int)machine->ReadRegister(4));
          break;
        case SC_Exit:
          DEBUG('a', "Exit syscall. \n");
          Exit_Syscall((unsigned int)machine->ReadRegister(4));
          break;
        case SC_PrintOut:
          DEBUG('a', "PrintOut syscall. \n");
          PrintOut_Syscall(machine->ReadRegister(4), machine->ReadRegister(5));
          break;
        case SC_PrintNumber:
          DEBUG('a', "PrintNumber syscall. \n");
          PrintNumber_Syscall(machine->ReadRegister(4));
          break;
        case SC_RandomNumber:
          DEBUG('a', "RandomNumber syscall. \n");
          rv = RandomNumber_Syscall(machine->ReadRegister(4));
          break;
        #ifdef NETWORK
          case SC_CreateMV:
            DEBUG('a', "CreateMV syscall. \n");
            rv = CreateMV_Syscall(machine->ReadRegister(4));
            break;
          case SC_SetMV:
            DEBUG('a', "SetMV syscall. \n");
            rv = SetMV_Syscall(machine->ReadRegister(4), machine->ReadRegister(5));
            break;
          case SC_GetMV:
            DEBUG('a', "GetMV syscall. \n");
            rv = GetMV_Syscall(machine->ReadRegister(4));
            break;
          case SC_DestroyMV:
            DEBUG('a', "DestroyMV syscall. \n");
            rv = DestroyMV_Syscall(machine->ReadRegister(4));
            break;
        #endif
      #endif
    }

    // Put in the return value and increment the PC
    machine->WriteRegister(2,rv);
    machine->WriteRegister(PrevPCReg,machine->ReadRegister(PCReg));
    machine->WriteRegister(PCReg,machine->ReadRegister(NextPCReg));
    machine->WriteRegister(NextPCReg,machine->ReadRegister(PCReg)+4);
    
    return;
  }
  #ifdef CHANGED
    #ifdef USE_TLB
      else if (which == PageFaultException)
      {
        HandlePageFault();
      }
    #endif
    else if (which == BusErrorException)
    {
      printf("BusErrorException of type %d \n", type);
      interrupt->Halt();
    }
    else if (which == AddressErrorException)
    {
      printf("AddressErrorException of type %d \n", type);
      interrupt->Halt();
    }
    else if( which == IllegalInstrException)
    {
      printf("IllegalInstrException of type %d \n", type);
      interrupt->Halt();
      //printf("Starting up NachOS Debugger...\n");
      //machine->Debugger();
      //printf("Dividing by zero to stop program for debugging...\n");
      //int fail = 1 / 0;
    }
  #endif
  else
  {
    cout<<"Unexpected user mode exception - which:"<<which<<"  type:"<< type<<endl;
    interrupt->Halt();
  }
}
