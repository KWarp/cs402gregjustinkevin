/* testfork.c
 *	Simple program to test the fork system call.
 */
 
#include "syscall.h"

/* Assumes that the lock syscalls all work. */
int lock;

void test1()
{
  int i;
  
  Acquire(lock);
  PrintOut("Running test1\n", 14);
  for (i = 0; i < 8; ++i)
  {
    Acquire(lock);
    PrintOut("This will be printed 8 times (", 30);
    PrintNumber(i + 1);
    PrintOut(")\n", 2);
    Release(lock);
  }
  PrintOut("test1 complete\n", 15);
  
  Release(lock);
  /* Yield(); */
  Exit(0);
}

void test2()
{
  int i;
  
  PrintOut("Running test2\n", 14);
  for (i = 0; i < 4; ++i)
  {
    Acquire(lock);
    PrintOut("This will be printed 4 times (", 30);
    PrintNumber(i + 1);
    PrintOut(")\n", 2);
    Release(lock);
  }
  PrintOut("test2 complete\n", 15);
  
  Exit(0);
}

int main()
{
  lock = CreateLock();

  PrintOut("Forking test1\n", 14);
  Fork(test1);
  
  PrintOut("Forking test2\n", 14);
  Fork(test2);
  
  Exit(0);
  return 0;
}
