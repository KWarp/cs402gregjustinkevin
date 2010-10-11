/* testfork.cc
 *	Simple program to test the fork system call.
 */

#include "syscall.h"

void test1()
{
  int i;
  
  Write("Running test1\n", 14, ConsoleOutput);
  for (i = 0; i < 8; ++i)
    Write("This will be printed 8 times\n", 29, ConsoleOutput);
  Write("test1 complete\n", 15, ConsoleOutput);
  
  /* Yield(); */
  Exit(0);
}

void test2()
{
  int i;
  
  Write("Running test2\n", 14, ConsoleOutput);
  for (i = 0; i < 4; ++i)
    Write("This will be printed 4 times\n", 29, ConsoleOutput);
  Write("test2 complete\n", 15, ConsoleOutput);
  
  /* Yield(); */
  Exit(0);
}

int main()
{
  Write("Press Ctrl-C once the threads are finished printing\n", 52, ConsoleOutput);
  Write("Forking test1\n", 14, ConsoleOutput);
  Fork(test1);
  
  Write("Forking test2\n", 14, ConsoleOutput);
  Fork(test2);
  
  while (1)
  {
    Yield();
  }
}
