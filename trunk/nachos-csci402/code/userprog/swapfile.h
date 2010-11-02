/*

Swapfile class for 402 Project 3
Encapsutes everything needed for a swap file for the IPT

This is just a plain old file that Nachos reads and writes to.

Access to contents of the swap file is indexed by PageTableEntryindex.swapIndexOffset

Requirements:
Create and open the swap file when nachos starts
Close it when Nachos finishes
Exactly one instance
Global kernel data 



*/

#ifndef SWAP_FILE_H
#define SWAP_FILE_H

///
// GREG: Don't include things you don't need!!!
//       Sometimes it helps to comment on specifically why you decided you need to include something.
///

// #include "../threads/system.h" // Don't need this.
// #include "addrspace.h"         // Don't need this.
// #include "noff.h"              // Don't need this.
// #include "table.h"             // Don't need this.
#include "../threads/synch.h"     // For Lock.
// #include "filesys.h"           // Don't need this.
#include "../filesys/openfile.h"  // For OpenFile.



#define MAX_SWAP_PAGES 16000
#define MAX_SWAP_SIZE (PageSize*MAX_SWAP_PAGES)

class SwapFile
{
  public:
    SwapFile();
    ~SwapFile();
    
    int GetSwapPageIndex();
    
    // index to load from, ppn in main memory to load into
    int Load(int index, int ppn); 
    int Store(int index, int ppn); 
    
    int Evict(int index);
    void EvictAll();
  
  private:
    Lock* swapAccessLock;
    OpenFile* swap;
    BitMap* pageMap;
    
    int isValidIndex(int index);
};

#endif // SWAP_FILE_H

