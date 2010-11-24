#ifndef PROCESSTABLEENTRY_H
#define PROCESSTABLEENTRY_H

#include "addrspace.h"
#include "bitmap.h"

#define MAX_NUM_PROCESSES 100

class ProcessTableEntry
{
 public:
  ProcessTableEntry();
  ~ProcessTableEntry();
  
  void addThread();
  void killThread();
  int getNumThreads();
  
  AddrSpace* space;
  
 private:
  int numThreads;
};

class ProcessTable
{
 public:
  ProcessTable();
  ~ProcessTable();
  
  void addThread(AddrSpace* space);
  void addProcess(AddrSpace* space);
  void killThread(AddrSpace* space);
  void killProcess(AddrSpace* space);
  int getNumProcesses();
  int getNumThreadsForProcess(AddrSpace* space);
  int getNumThreads();
  
 private:
  ProcessTableEntry* processes[MAX_NUM_PROCESSES];
};

#endif
