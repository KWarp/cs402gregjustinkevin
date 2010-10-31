/*

Swapfile class for 402 Project 3
Encapsutes everything needed for a swap file for the IPT

This is just a plain old file that Nachos reads and writes to.
It has a slot for every page

Requirements:
Create and open the swap file when nachos starts
Close it when Nachos finishes
Exactly one instance
Global kernel data 

On a page fault

*/

#include "system.h"
#include "addrspace.h"
#include "noff.h"
#include "table.h"
#include "synch.h"
#include "filesys.h"
#include "openfile.h"

#ifndef SWAP_FILE_H
#define SWAP_FILE_H

#define MAX_SWAP_PAGES 16000
#define MAX_SWAP_SIZE (PageSize*MAX_SWAP_PAGES)

class SwapFile
{
  public:
    SwapFile(int pMaxPagesInMemory);
    ~SwapFile();
    
    int Load(int vpn, int ppn); 
    int Store(int vpn, int ppn);
    int Evict(int ppn);
    void Free();
  
  private:
    Lock* swapAccessLock;
    OpenFile* swap;
    BitMap* pageMap;
    int maxPagesInMemory;
    int* indexFromVPN; // dynamic array
};

#endif // SWAP_FILE_H

