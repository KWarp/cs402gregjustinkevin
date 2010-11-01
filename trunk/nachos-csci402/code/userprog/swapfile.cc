

#include "swapfile.h"


SwapFile::SwapFile()
{
  swapAccessLock = new Lock("SwapAccessLock");
  
  // create a file in our directory named "swap"
  if(fileSystem->Create("swap", MAX_SWAP_SIZE)) 
		swap = fileSystem->Open("swap");
	else
		swap = NULL; // very bad: not enough space or error in creating the file
  
  pageMap = new BitMap(MAX_SWAP_PAGES);
  
}

SwapFile::~SwapFile()
{
  delete pageMap;
  delete swap;
  delete swapAccessLock;
}


int SwapFile::GetSwapPageIndex()
{
  int swapPageIndex;
  
  swapAccessLock->Acquire();
    // find a free page in swap file
    swapPageIndex = pageMap->Find(); 
    if (swapPageIndex == -1)
    {
      printf("Swap file ran out of space! Aborting Nachos...\n\n");
      interrupt->Halt();
    }
  swapAccessLock->Release();
  
  return swapPageIndex;
}


int SwapFile::Load(int index, int ppn)
{
  if( !isValidIndex(index) )
    printf("ERROR: Invalid SwapFile index %d", index);
    
  // int ReadAt(char *into, int numBytes, int position);
  int success = swap->ReadAt(&(machine->mainMemory[ppn * PageSize]), PageSize, index * PageSize);
  return success;
}


int SwapFile::Store(int index, int ppn)
{
  int success;
  
  swapAccessLock->Acquire();
  
    if( !isValidIndex(index) )
      printf("ERROR: Invalid SwapFile index %d", index);
    
    // write entry into swap
    // int WriteAt(char *from, int numBytes, int position);
    success = swap->WriteAt(&(machine->mainMemory[ppn*PageSize]), PageSize, index*PageSize); 
  swapAccessLock->Release();
  
  return success;
}


int SwapFile::Evict(int index)
{
  if( !isValidIndex(index) )
    return 0; // can't evict
  
  pageMap->Clear(index);
  
  return 1;
}


void SwapFile::EvictAll()
{
  for (int i = 0; i < MAX_SWAP_PAGES; i++)
	{
    pageMap->Clear(i);
  }
  
}


int SwapFile::isValidIndex(int index)
{
  // out of range
  if(index < 0 || index >= MAX_SWAP_PAGES)
    return 0;
  
  // unregistered = fail
  if( !pageMap->Test(index) )
  {
    return 0;
  }
  
  // valid
  return 1;
}





