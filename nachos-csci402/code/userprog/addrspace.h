// addrspace.h 
//	Data structures to keep track of executing user programs 
//	(address spaces).
//
//	For now, we don't keep any information about address spaces.
//	The user level CPU state is saved and restored in the thread
//	executing the user program (see thread.h).
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#ifndef ADDRSPACE_H
#define ADDRSPACE_H

#include "copyright.h"
#include "filesys.h"
#include "table.h"

#ifdef CHANGED
  #include "bitmap.h"
  #include "translate.h"
  #include "pagetableentry.h"
#endif

#define UserStackSize		1024 	// increase this as necessary!

#define MaxOpenFiles 256
#define MaxChildSpaces 256

class AddrSpace {
  public:
    // Create an address space, initializing it with the program stored
    //  in the file "executable".
    AddrSpace(OpenFile *executable);
    
    // De-allocate an address space.
    ~AddrSpace();

    // Initialize user-level CPU registers, before jumping to user code.
    void InitRegisters();

    // Save/restore address space-specific info on a context switch.
    void SaveState();
    void RestoreState();
    
    // Table of openfiles.
    Table fileTable;

    #ifdef CHANGED
      // Allocates room for a new stack and returns a pointer to the stack beginning.
      int AllocateStack();
      int maxPagesInMemory; 
    #endif
    
    #ifdef USE_TLB
      PageTableEntry *pageTable;
    #else
      TranslationEntry *pageTable;
    #endif
    
    unsigned int numPages;		    // Number of pages in the virtual address space.
};

#endif // ADDRSPACE_H
