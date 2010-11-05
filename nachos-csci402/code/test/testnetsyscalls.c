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

  PrintOut("Creating Lock...", 17); 
  lockIndex = CreateLock("LK1");
  if (lockIndex >= 0)
    PrintOut("Succeeded\n", 10);

  PrintOut("Acquiring Lock...", 17);
  Acquire(lockIndex);
  PrintOut("Succeeded\n", 10);
  
  PrintOut("Releasing Lock...", 17);
  Release(lockIndex);
  PrintOut("Succeeded\n", 10);
  
  PrintOut("Destroying Lock...", 18);
  DestroyLock(lockIndex);
  PrintOut("Succeeded\n", 10);
}

void MVTest()
{
  int mvIndex, mvIndex2, value;

  PrintOut("------------------------------------------\n", 43);
  PrintOut("Testing MV\n", 11);
  PrintOut("------------------------------------------\n", 43);
  
  PrintOut("Creating MVs...", 15);
  mvIndex = CreateMV("MV1");
  mvIndex2 = CreateMV("MV2");
  if (mvIndex2 > mvIndex)
    PrintOut("Succeeded\n", 10);
  else
    PrintOut("Failed\n", 7);

  PrintOut("Setting/Getting MVs...", 22);
  SetMV(mvIndex2, 9);
  value = GetMV(mvIndex2);
  if (value == 9)
    PrintOut("Succeeded\n", 10);
  else
    PrintOut("Failed\n", 7);

  PrintOut("Destroying MVs...", 17);
  if (DestroyMV(mvIndex2) == 0)
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
  
  PrintOut("Creating CV...", 14);
  cvIndex = CreateCondition("CV1");
  lockIndex = CreateLock("LK1");

  PrintOut("Testing Signal...", 17);
  /*
  if (Signal(cvIndex, lockIndex) == 0)
    PrintOut("Succeeded\n", 10);
  else
    PrintOut("Failed\n", 7);
  */
  Signal(cvIndex, lockIndex);
  PrintOut("Succeeded\n", 10);
  
  PrintOut("Testing Wait...", 15);
  /*
  if (Wait(cvIndex, lockIndex) == 0)
    PrintOut("Succeeded\n", 10);
  else
    PrintOut("Failed\n", 7);
  */
  Wait(cvIndex, lockIndex);
  PrintOut("Succeeded\n", 10);
  
  PrintOut("Destroying CV...", 16);
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

