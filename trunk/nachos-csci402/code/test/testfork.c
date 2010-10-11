/* testfork.cc
 *	Simple program to test the fork system call.
 */

#include "syscall.h"

void test1()
{
  int i;
  
  Write("Running test1\n", 14, ConsoleOutput);
  for (i = 0; i < 20; ++i)
    Write("This will be printed 20 times\n", 30, ConsoleOutput);
  Write("test1 complete\n", 15, ConsoleOutput);
  
  Yield();
}

void test2()
{
  int i;
  
  Write("Running test2\n", 14, ConsoleOutput);
  for (i = 0; i < 10; ++i)
    Write("This will be printed 10 times\n", 30, ConsoleOutput);
  Write("test2 complete\n", 15, ConsoleOutput);
  
  Yield();
}

int main()
{
  Write("Forking test1\n", 14, ConsoleOutput);
  Fork(test1);
  
  Write("Forking test2\n", 14, ConsoleOutput);
  Fork(test2);
  
  Yield();
}
