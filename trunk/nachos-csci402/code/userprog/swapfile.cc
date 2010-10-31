

#include "swapfile.h"




SwapFile::SwapFile(int pMaxPagesInMemory)
{
  maxPagesInMemory = pMaxPagesInMemory;
  
  swapAccessLock = new Lock("SwapAccessLock");
  
  // create a file in our directory named "swap"
  if(fileSystem->Create("swap", MAX_SWAP_SIZE)) 
		swap = fileSystem->Open("swap");
	else
		swap = NULL; // very bad: not enough space or error in creating the file
  
  pageMap = new BitMap(MAX_SWAP_PAGES);
  
  indexFromVPN = new int[maxPagesInMemory];
  for(int i=0; i < maxPagesInMemory; i++)
  {
    indexFromVPN[i] = -1;
  }
  
}

SwapFile::~SwapFile()
{
  delete[] indexFromVPN;
  delete pageMap;
  delete swap;
  delete swapAccessLock;
}


int SwapFile::Load(int vpn, int ppn)
{
  if( !isValidVPN(vpn) )
    printf("ERROR: Invalid SwapFile VPN %d", vpn);
    
  // int ReadAt(char *into, int numBytes, int position);
  int success = swap->ReadAt(&(machine->mainMemory[ppn * PageSize]), PageSize, indexFromVPN[vpn] * PageSize);
  return success;
}


int SwapFile::Store(int vpn, int ppn)
{
  int swapFilePageIndex, success;
  
  swapAccessLock->Acquire();
    // find a free page in swap file
    swapFilePageIndex = pageMap->Find(); 
    if (swapFilePageIndex == -1)
    {
      printf("Swap file ran out of space! Aborting Nachos...\n\n");
      interrupt->Halt();
    }
    // write entry into swap
    // int WriteAt(char *from, int numBytes, int position);
    success = swap->WriteAt(&(machine->mainMemory[ppn*PageSize]), PageSize, swapFilePageIndex*PageSize); 
  swapAccessLock->Release();
  
  if(success)
  {
    indexFromVPN[vpn] = swapFilePageIndex;
    return success;
  }
  else
  {
    // failed
    return 0;
  }
}

int SwapFile::Evict(int vpn)
{
  if( !isValidVPN(vpn) )
    return 0; // can't evict
  
  pageMap->Clear(indexFromVPN[vpn]);
  indexFromVPN[vpn] = -1;
  
  return 1;
}


void SwapFile::EvictAll()
{
  for (int i = 0; i < MAX_SWAP_PAGES; i++)
	{
    pageMap->Clear(i);
  }

  for (int i = 0; i < maxPagesInMemory; i++)
	{
    indexFromVPN[i] = -1;
	}
  
}

// ========================
// PRIVATE
// ========================

int SwapFile::isValidVPN(int vpn)
{
  // out of range
  if(vpn < 0 || vpn >= maxPagesInMemory)
    return 0;
  // empty
  if(indexFromVPN[vpn] == -1)
    return 0;
  
  // valid
  return 1;
}





