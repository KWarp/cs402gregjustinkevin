/* testnetsyscalls.c
 *  Tests network syscalls.
 */
 
#ifndef NETWORK
  /* #define NETWORK */
#endif
 
#include "syscall.h"

void LockTest()
{
  int lockIndex;

  PrintOut("------------------------------------------\n", 43);
  PrintOut("Testing Lock\n", 13); 
  PrintOut("------------------------------------------\n", 43);

  PrintOut("Creating Lock...\n", 18); 
  lockIndex = CreateLock("LK1");
  if (lockIndex >= 0)
    PrintOut("Succeeded\n", 10);

  PrintOut("Acquiring Lock...\n", 18);
  Acquire(lockIndex);
  PrintOut("Succeeded\n", 10);
  
  PrintOut("Releasing Lock...\n", 18);
  Release(lockIndex);
  PrintOut("Succeeded\n", 10);
  
  PrintOut("Destroying Lock...\n", 19);
  DestroyLock(lockIndex);
  PrintOut("Succeeded\n", 10);
}

void MVTest()
{
  int mvIndex, mvIndex2, value;

  PrintOut("------------------------------------------\n", 43);
  PrintOut("Testing MV\n", 11);
  PrintOut("------------------------------------------\n", 43);
  
  PrintOut("Creating MVs...\n", 16);
  mvIndex = CreateMV("MV1");
  mvIndex2 = CreateMV("MV2");
  if (mvIndex2 > mvIndex)
    PrintOut("Succeeded\n", 10);
  else
    PrintOut("Failed\n", 7);

  PrintOut("Setting/Getting MVs...\n", 23);
  SetMV(mvIndex2, 9);
  value = GetMV(mvIndex2);
  if (value == 9)
    PrintOut("Succeeded\n", 10);
  else
    PrintOut("Failed\n", 7);

  PrintOut("Destroying MVs...\n", 18);
  if (DestroyMV(mvIndex2) >= 0)
    PrintOut("Succeeded\n", 10);
  else
    PrintOut("Failed\n", 7);
}

/* NOTE: Assumes CreateLock() syscall works. */
void CVTest()
{
  int lockIndex, cvIndex;

  PrintOut("------------------------------------------\n", 43);
  PrintOut("Testing CV\n", 11);
  PrintOut("------------------------------------------\n", 43);
  
  PrintOut("Creating CV...\n", 15);
  cvIndex = CreateCondition("CV1");
  lockIndex = CreateLock("LK1");
  PrintOut("Succeeded\n", 10);
  
  /* PrintOut("Testing Signal...\n", 18); */
  /*
  if (Signal(cvIndex, lockIndex) == 0)
    PrintOut("Succeeded\n", 10);
  else
    PrintOut("Failed\n", 7);
  */
  /* Signal(cvIndex, lockIndex); */
  /* PrintOut("Succeeded\n", 10); */
  
  /* PrintOut("Testing Wait...\n", 16); */
  /*
  if (Wait(cvIndex, lockIndex) == 0)
    PrintOut("Succeeded\n", 10);
  else
    PrintOut("Failed\n", 7);
  */
  /* Wait(cvIndex, lockIndex); */
  /* PrintOut("Succeeded\n", 10); */
  
  PrintOut("Destroying CV...\n", 17);
  /*
  if (DestroyCondition(cvIndex) == 0)
    PrintOut("Succeeded\n", 10);
  else
    PrintOut("Failed\n", 7);
  */
  DestroyCondition(cvIndex);
  PrintOut("Succeeded\n", 10);
}

int main(int argc, char **argv)
{
  LockTest();
  MVTest();
  CVTest();

  Halt();
}

