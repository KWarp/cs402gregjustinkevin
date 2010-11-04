
#include "../threads/system.h"
#include "swapfile.h"

void SwapFile::printMemoryPage(int ppn)
{
  printf("Main memory: \n");
  for(int i= ppn * PageSize; i < (ppn * PageSize) + PageSize; i++)
    printf("%c", &(machine->mainMemory[i]));
  printf("\n");
}

void SwapFile::printFilePage(int index)
{
  char* array = new char[PageSize];
  printf("File Page: \n");
  swap->ReadAt(array, PageSize, index * PageSize);

  for(int i= 0; i < PageSize; i++)
    printf("%c", array[i]);
  printf("\n");
  delete[] array;
}



SwapFile::SwapFile()
{
  swapAccessLock = new Lock("SwapAccessLock");
  
  // create a file in our directory named "swap"
  if(fileSystem->Create("swap", MAX_SWAP_SIZE)) 
		swap = fileSystem->Open("swap");
	else
		swap = NULL; // very bad: not enough space or error in creating the file
  
  pageMap = new BitMap(MAX_SWAP_PAGES);
  
  
  /*
  // throw in some data
  char* data = new char[PageSize];
  for(int i=0; i< PageSize; i++)
    data[i] = 'i';
  
  for(int i=0; i< 1; i++)
  {
    swap->WriteAt(data, PageSize, i * PageSize);
    printFilePage(i);
  }
  */
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
  swapAccessLock->Acquire();
    
    printf("index in Load: %d\n", index);
    
    if (!isValidIndex(index))
      printf("ERROR: Invalid SwapFile index %d \n", index);
      
    // int ReadAt(char *into, int numBytes, int position);
    int success = swap->ReadAt(&(machine->mainMemory[ppn * PageSize]), PageSize, index * PageSize);
    //printMemoryPage(ppn);
    //printFilePage(index);
  swapAccessLock->Release();
  return success;
}


int SwapFile::Store(int index, int ppn)
{
  int success;
  
  swapAccessLock->Acquire();
  
    if( !isValidIndex(index) )
      printf("ERROR: Invalid SwapFile index %d \n", index);
    
    // write entry into swap
    //printMemoryPage(ppn); 
    //printFilePage(index);    
    // int WriteAt(char *from, int numBytes, int position);
    success = swap->WriteAt(&(machine->mainMemory[ppn * PageSize]), PageSize, index * PageSize);
    //printFilePage(index);
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





