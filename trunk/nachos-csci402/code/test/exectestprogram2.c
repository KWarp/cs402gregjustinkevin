/* exectestprogram2.c
 *	exectest "Exec's" this executable into a new process to test the Exec system call.
 */

#include "syscall.h"

void main()
{
  int i;
  
  PrintOut("Running test2\n", 14);
  for (i = 0; i < 4; ++i)
    PrintOut("This will be printed 4 times\n", 29);
  PrintOut("test2 complete\n", 15);
  
  Exit(0);
}
