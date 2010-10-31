

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
    indexFromVPN[i] = -1;
    
}

SwapFile::~SwapFile()
{
  
  
}


int SwapFile::Load(int vpn, int ppn)
{
  if(indexFromVPN[vpn] == -1)
    printf("ERROR: Invalid SwapFile VPN %d", vpn);
  // int ReadAt(char *into, int numBytes, int position);
  swap->ReadAt(&(machine->mainMemory[ppn * PageSize]), PageSize, indexFromVPN[vpn] * PageSize);
  return 0;
}


int SwapFile::Store(int vpn, int ppn)
{
  int swapFilePageIndex;
  
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
    swap->WriteAt(&(machine->mainMemory[ppn*PageSize]), PageSize, swapFilePageIndex*PageSize); 
  swapAccessLock->Release();
  
  indexFromVPN[vpn] = swapFilePageIndex;
  
  return 0;
}

int SwapFile::Evict(int ppn)
{
  pageMap->Clear(ppn);
  return 1;
}


void SwapFile::Free()
{
  for (int i = 0; i < maxPagesInMemory; i++)
	{
		Evict(i);
	}
  
}





