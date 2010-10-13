#include "processtable.h"
#include "syscall.h"

ProcessTableEntry::ProcessTableEntry()
{
  // There will always be initially be one thread for the process.
  numThreads = 1;
}

ProcessTableEntry::~ProcessTableEntry()
{}

void ProcessTableEntry::addThread()
{
  numThreads++;
}

int ProcessTableEntry::getNumThreads()
{
  return numThreads;
}

void ProcessTableEntry::killThread()
{
  ASSERT(numThreads > 0);
  numThreads--;
}

ProcessTable::ProcessTable()
{
  // Initialize all entries to NULL.
  for (int i = 0; i < MAX_NUM_PROCESSES; ++i)
    processes[i] = NULL;
}

ProcessTable::~ProcessTable()
{
  for (int i = 0; i < MAX_NUM_PROCESSES; ++i)
  {
    if (processes[i] != NULL)
    {
      delete processes[i];
      processes[i] = NULL;
    }
  }
}

void ProcessTable::addThread(AddrSpace* space)
{
  for (int i = 0; i < MAX_NUM_PROCESSES; ++i)
  {
    if (processes[i] == NULL)
      continue;
    if (processes[i]->space == space)
    {
      processes[i]->addThread();
      return;
    }
  }
  // printf("Process not found in addTread\n");
  addProcess(space);  // Should we do this or should we just assume that a process has already been created?
}

void ProcessTable::killThread(AddrSpace* space)
{
  for (int i = 0; i < MAX_NUM_PROCESSES; ++i)
  {
    if (processes[i] == NULL)
      continue;
    if (processes[i]->space == space)
    {
      processes[i]->killThread();
      if (processes[i]->getNumThreads() <= 0)
      {
        delete processes[i];
        processes[i] = NULL;
      }
      break;
    }
  }
  // printf("Process not found in killThread\n");
}

void ProcessTable::addProcess(AddrSpace* space)
{
  for (int i = 0; i < MAX_NUM_PROCESSES; ++i)
  {
    if (processes[i] == NULL)
    {
      processes[i] = new ProcessTableEntry();
      return;
    }
  }
  
  // If we get here, we have exceeded MAX_NUM_PROCESSES.
  printf("Exceeded max num processes!!! Increase MAX_NUM_PROCESSES in processtable.h\n");
}

void ProcessTable::killProcess(AddrSpace* space)
{
  for (int i = 0; i < MAX_NUM_PROCESSES; ++i)
  {
    if (processes[i] == NULL)
      continue;
    if (processes[i]->space == space)
    {
      delete processes[i];
      processes[i] = NULL;
      return;
    }
  }
  // printf("Process not found in killProcess\n");
}

int ProcessTable::getNumProcesses()
{
  int count = 0;
  for (int i = 0; i < MAX_NUM_PROCESSES; ++i)
  {
    if (processes[i] != NULL)
      count++;
  }
  return count;
}

int ProcessTable::getNumThreadsForProcess(AddrSpace* space)
{
  for (int i = 0; i < MAX_NUM_PROCESSES; ++i)
  {
    if (processes[i] == NULL)
      continue;
    if (processes[i]->space == space)
      return processes[i]->getNumThreads();
  }
  // printf("Process not found in getNumThreadsForProcess\n");
  return -1;  // Error: process not found.
}

int ProcessTable::getNumThreads()
{
  int num = 0;
  for (int i = 0; i < MAX_NUM_PROCESSES; ++i)
  {
    if (processes[i] == NULL)
      continue;
    num += processes[i]->getNumThreads();
  }
  return num;
}
