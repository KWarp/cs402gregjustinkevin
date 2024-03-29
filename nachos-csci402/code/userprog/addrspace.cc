// addrspace.cc 
//	Routines to manage address spaces (executing user programs).
//
//	In order to run a user program, you must:
//
//	1. link with the -N -T 0 option 
//	2. run coff2noff to convert the object file to Nachos format
//		(Nachos object code format is essentially just a simpler
//		version of the UNIX executable object code format)
//	3. load the NOFF file into the Nachos file system
//		(if you haven't implemented the file system yet, you
//		don't need to do this last step)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "addrspace.h"
#include "noff.h"
#include "table.h"
#include "synch.h"

extern "C" { int bzero(char *, int); };

Table::Table(int s) : map(s), table(0), lock(0), size(s)
{
  table = new void *[size];
  lock = new Lock("TableLock");
}

Table::~Table()
{
  if (table)
  {
    delete table;
    table = 0;
  }
  if (lock)
  {
    delete lock;
    lock = 0;
  }
}

void* Table::Get(int i)
{
  // Return the element associated with the given if, or 0 if
  // there is none.

  return (i >=0 && i < size && map.Test(i)) ? table[i] : 0;
}

int Table::Put(void *f)
{
  // Put the element in the table and return the slot it used.  Use a
  // lock so 2 files don't get the same space.
  int i;	// to find the next slot

  lock->Acquire();
  i = map.Find();
  lock->Release();
  if (i != -1)
    table[i] = f;
    
  return i;
}

void* Table::Remove(int i)
{
  // Remove the element associated with identifier i from the table,
  // and return it.

  void *f =0;

  if (i >= 0 && i < size)
  {
    lock->Acquire();
    if (map.Test(i))
    {
      map.Clear(i);
      f = table[i];
      table[i] = 0;
    }
    lock->Release();
  }
  
  return f;
}

//----------------------------------------------------------------------
// SwapHeader
// 	Do little endian to big endian conversion on the bytes in the 
//	object file header, in case the file was generated on a little
//	endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------
static void SwapHeader (NoffHeader *noffH)
{
	noffH->noffMagic = WordToHost(noffH->noffMagic);
	noffH->code.size = WordToHost(noffH->code.size);
	noffH->code.virtualAddr = WordToHost(noffH->code.virtualAddr);
	noffH->code.inFileAddr = WordToHost(noffH->code.inFileAddr);
	noffH->initData.size = WordToHost(noffH->initData.size);
	noffH->initData.virtualAddr = WordToHost(noffH->initData.virtualAddr);
	noffH->initData.inFileAddr = WordToHost(noffH->initData.inFileAddr);
	noffH->uninitData.size = WordToHost(noffH->uninitData.size);
	noffH->uninitData.virtualAddr = WordToHost(noffH->uninitData.virtualAddr);
	noffH->uninitData.inFileAddr = WordToHost(noffH->uninitData.inFileAddr);
}

//----------------------------------------------------------------------
// AddrSpace::AddrSpace
// 	Create an address space to run a user program.
//	Load the program from a file "executable", and set everything
//	up so that we can start executing user instructions.
//
//	Assumes that the object code file is in NOFF format.
//
//	"executable" is the file containing the object code to load into memory
//
//      It's possible to fail to fully construct the address space for
//      several reasons, including being unable to allocate memory,
//      and being unable to read key parts of the executable.
//      Incompletely consretucted address spaces have the member
//      constructed set to false.
//----------------------------------------------------------------------
AddrSpace::AddrSpace(OpenFile *executable) : fileTable(MaxOpenFiles)
{
  #ifdef CHANGED
    NoffHeader noffH;

    ppnInUseLock->Acquire();
    
    // Don't allocate the input or output to disk files.
    fileTable.Put(0);
    fileTable.Put(0);

    // Read header from the executable file.
    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) && (WordToHost(noffH.noffMagic) == NOFFMAGIC))
      SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);

    // Calculate size and numPages.
    int size = noffH.code.size + noffH.initData.size + noffH.uninitData.size;
    numPages = divRoundUp(size, PageSize);
    int numStackPages = divRoundUp(UserStackSize, PageSize);

    size = numPages * PageSize;
    DEBUG('a', "Initializing address space, num pages %d, size %d\n", numPages, size);

    // Setup the pageTable.
    #ifdef USE_TLB
      pageTable = new PageTableEntry[numPages + numStackPages];
    #else
      pageTable = new TranslationEntry[numPages + numStackPages];
      // Check we're not trying to run anything too big -- at least until we have virtual memory.
      ASSERT(numPages + numStackPages <= NumPhysPages);    
    #endif

    // Read data from just the code and initData sections.
    unsigned int vpn = 0;
    for (; vpn < (unsigned int)(divRoundUp(noffH.code.size + noffH.initData.size, PageSize)); ++vpn)
    {
      #ifdef USE_TLB
        pageTable[vpn].virtualPage  = vpn;
        pageTable[vpn].physicalPage = -1;
        pageTable[vpn].valid        = FALSE;
        pageTable[vpn].use          = FALSE;
        pageTable[vpn].dirty        = FALSE;
        pageTable[vpn].readOnly     = FALSE;  // If the code segment was entirely on a separate page, we could set its pages to be read-only.
      
        pageTable[vpn].location     = IN_EXECUTABLE;
        pageTable[vpn].executable   = executable;
        pageTable[vpn].byteOffset   = noffH.code.inFileAddr + (vpn * PageSize);
        pageTable[vpn].swapPageIndex = -1;    // Invalid value.
      #else
        int ppn = ppnInUseBitMap->Find();
        executable->ReadAt(&(machine->mainMemory[ppn * PageSize]), PageSize, noffH.code.inFileAddr + (vpn * PageSize));
        
        pageTable[vpn].virtualPage  = vpn;
        pageTable[vpn].physicalPage = ppn;
        pageTable[vpn].valid        = TRUE;
        pageTable[vpn].use          = FALSE;
        pageTable[vpn].dirty        = FALSE;
        pageTable[vpn].readOnly     = FALSE;  // If the code segment was entirely on a separate page, we could set its pages to be read-only.
      #endif
    }
    
    // Increase the size to leave room for the stack.
    numPages += numStackPages;
    
    for (; vpn < numPages; ++vpn)
    {
      #ifdef USE_TLB
        pageTable[vpn].virtualPage  = vpn;
        pageTable[vpn].physicalPage = -1;
        pageTable[vpn].valid        = FALSE;
        pageTable[vpn].use          = FALSE;
        pageTable[vpn].dirty        = FALSE;
        pageTable[vpn].readOnly     = FALSE; // If the code segment was entirely on a separate page, we could set its pages to be read-only.
      
        pageTable[vpn].location     = NEITHER;
        pageTable[vpn].executable   = NULL;  // Invalid value.
        pageTable[vpn].byteOffset   = -1;    // Invalid value.
        pageTable[vpn].swapPageIndex = -1;   // Invalid value.
      #else
        int ppn = ppnInUseBitMap->Find();
        bzero(&(machine->mainMemory[ppn * PageSize]), PageSize);
        
        pageTable[vpn].virtualPage  = vpn;
        pageTable[vpn].physicalPage = ppn;
        pageTable[vpn].valid        = TRUE;
        pageTable[vpn].use          = FALSE;
        pageTable[vpn].dirty        = FALSE;
        pageTable[vpn].readOnly     = FALSE;  // If the code segment was entirely on a separate page, we could set its pages to be read-only.
      #endif
    }
    
    ppnInUseLock->Release();
    
    
  #else
  
  
    NoffHeader noffH;
    unsigned int i, size;

    // Don't allocate the input or output to disk files
    fileTable.Put(0);
    fileTable.Put(0);

    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) && (WordToHost(noffH.noffMagic) == NOFFMAGIC))
      SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);

    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size ;
    numPages = divRoundUp(size, PageSize) + divRoundUp(UserStackSize,PageSize);
                                                // we need to increase the size
            // to leave room for the stack
    size = numPages * PageSize;

    ASSERT(numPages <= NumPhysPages);		// check we're not trying
            // to run anything too big --
            // at least until we have
            // virtual memory

    DEBUG('a', "Initializing address space, num pages %d, size %d\n", 
          numPages, size);
    // first, set up the translation 
    pageTable = new TranslationEntry[numPages];
    for (i = 0; i < numPages; i++)
    {
      pageTable[i].virtualPage = i;	// for now, virtual page # = phys page #
      pageTable[i].physicalPage = i;
      pageTable[i].valid = TRUE;
      pageTable[i].use = FALSE;
      pageTable[i].dirty = FALSE;
      pageTable[i].readOnly = FALSE;  // if the code segment was entirely on 
          // a separate page, we could set its 
          // pages to be read-only
    }
    
    // zero out the entire address space, to zero the unitialized data segment 
    // and the stack segment
    bzero(machine->mainMemory, size);

    // then, copy in the code and data segments into memory
    if (noffH.code.size > 0)
    {
      DEBUG('a', "Initializing code segment, at 0x%x, size %d\n", 
            noffH.code.virtualAddr, noffH.code.size);
      executable->ReadAt(&(machine->mainMemory[noffH.code.virtualAddr]),
                         noffH.code.size, noffH.code.inFileAddr);
    }
    if (noffH.initData.size > 0)
    {
      DEBUG('a', "Initializing data segment, at 0x%x, size %d\n", 
            noffH.initData.virtualAddr, noffH.initData.size);
      executable->ReadAt(&(machine->mainMemory[noffH.initData.virtualAddr]),
                         noffH.initData.size, noffH.initData.inFileAddr);
    }
  #endif //ifdef CHANGED
}

//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
//
// 	Dealloate an address space.  release pages, page tables, files
// 	and file tables
//----------------------------------------------------------------------
AddrSpace::~AddrSpace()
{
    delete pageTable;
}

//----------------------------------------------------------------------
// AddrSpace::InitRegisters
// 	Set the initial values for the user-level register set.
//
// 	We write these directly into the "machine" registers, so
//	that we can immediately jump to user code.  Note that these
//	will be saved/restored into the currentThread->userRegisters
//	when this thread is context switched out.
//----------------------------------------------------------------------
void AddrSpace::InitRegisters()
{
  int i;

  for (i = 0; i < NumTotalRegs; i++)
    machine->WriteRegister(i, 0);

  // Initial program counter -- must be location of "Start"
  machine->WriteRegister(PCReg, 0);	

  // Need to also tell MIPS where next instruction is, because
  // of branch delay possibility
  machine->WriteRegister(NextPCReg, 4);

 // Set the stack register to the end of the address space, where we
 // allocated the stack; but subtract off a bit, to make sure we don't
 // accidentally reference off the end!
  machine->WriteRegister(StackReg, numPages * PageSize - 16);
  DEBUG('a', "Initializing stack register to %x\n", numPages * PageSize - 16);
}

//----------------------------------------------------------------------
// AddrSpace::SaveState
// 	On a context switch, save any machine state, specific
//	to this address space, that needs saving.
//
//	For now, nothing!
//----------------------------------------------------------------------
void AddrSpace::SaveState()
{}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
// 	On a context switch, restore the machine state so that
//	this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------
void AddrSpace::RestoreState()
{
  #ifdef USE_TLB
  
  #else
    machine->pageTable = pageTable;
    machine->pageTableSize = numPages;
  #endif
}

#ifdef CHANGED
  //----------------------------------------------------------------------
  // AddrSpace::AllocateStack
  // 	When creating a new thread, call this function to allocate
  //  memory for another stack.
  //
  //      Returns a pointer to the beginning of the new stack.
  //      NOTE: Assumes "stackPtr--; memory[stackPtr] = var;" usage.
  //----------------------------------------------------------------------
  int AddrSpace::AllocateStack()
  {
    ppnInUseLock->Acquire();
      int numStackPages = divRoundUp(UserStackSize, PageSize);
      
      #ifdef USE_TLB
        PageTableEntry* newPageTable = new PageTableEntry[numPages + numStackPages];
      #else
        TranslationEntry* newPageTable = new TranslationEntry[numPages + numStackPages];
      #endif
      
      // Deep copy old pageTable's data to newPageTable.
      for (unsigned int vpn = 0; vpn < numPages; ++vpn)
      {
        newPageTable[vpn].virtualPage  = pageTable[vpn].virtualPage;
        newPageTable[vpn].physicalPage = pageTable[vpn].physicalPage;
        newPageTable[vpn].valid        = pageTable[vpn].valid;
        newPageTable[vpn].use          = pageTable[vpn].use;
        newPageTable[vpn].dirty        = pageTable[vpn].dirty;
        newPageTable[vpn].readOnly     = pageTable[vpn].readOnly;
        
        #ifdef USE_TLB
          newPageTable[vpn].location     = pageTable[vpn].location;
          newPageTable[vpn].executable   = pageTable[vpn].executable;
          newPageTable[vpn].byteOffset   = pageTable[vpn].byteOffset;
          newPageTable[vpn].swapPageIndex = pageTable[vpn].swapPageIndex;
        #endif
      }
      
      // Initialize page table for the new stack's memory.
      for (unsigned int vpn = numPages; vpn < numPages + numStackPages; ++vpn)
      {
        #ifdef USE_TLB
          newPageTable[vpn].virtualPage  = vpn;
          newPageTable[vpn].physicalPage = -1;
          newPageTable[vpn].valid        = FALSE;
          newPageTable[vpn].use          = FALSE;
          newPageTable[vpn].dirty        = FALSE;
          newPageTable[vpn].readOnly     = FALSE; // If the code segment was entirely on a separate page, we could set its pages to be read-only.
        
          newPageTable[vpn].location     = NEITHER;
          newPageTable[vpn].executable   = NULL;  // Invalid value.
          newPageTable[vpn].byteOffset   = -1;    // Invalid value.
          newPageTable[vpn].swapPageIndex = -1;   // Invalid value.
        #else
          int ppn = ppnInUseBitMap->Find();
          bzero(&(machine->mainMemory[ppn * PageSize]), PageSize);
          
          pageTable[vpn].virtualPage  = vpn;
          pageTable[vpn].physicalPage = ppn;
          pageTable[vpn].valid        = TRUE;
          pageTable[vpn].use          = FALSE;
          pageTable[vpn].dirty        = FALSE;
          pageTable[vpn].readOnly     = FALSE;  // If the code segment was entirely on a separate page, we could set its pages to be read-only.        
        #endif
      }

      delete pageTable;
      pageTable = newPageTable;
      numPages += numStackPages;
      RestoreState();
    ppnInUseLock->Release();
    
    // Return an index to the last Page (the start of the new stack).
    return numPages;
  }
#endif
