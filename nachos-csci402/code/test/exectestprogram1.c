/* exectestprogram1.c
 *	exectest "Exec's" this executable into a new process to test the Exec system call.
 */
 
#include "syscall.h"

void main()
{
  int i;
  
  /* Acquire(lock); */
  PrintOut("Running test1\n", 14);
  for (i = 0; i < 8; ++i)
  {
    PrintOut("This will be printed 8 times (", 30);
    PrintNumber(i + 1);
    PrintOut(")\n", 2);
  }
  PrintOut("test1 complete\n", 15);
  
  /* Release(lock); */
  /* Yield(); */
  Exit(0);
}
