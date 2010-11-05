/* testexec.c
 *	Simple program to test that two sort programs 
 *  can run in memory at the same time
 *  without corrupting each other's memory
 */
 
#include "syscall.h"


int main()
{
  PrintOut("Exec'ing first sort\n", 20);
  Exec("../test/sort");
  
  PrintOut("Exec'ing second sort\n", 20);
  Exec("../test/sort");
  
  Exit(0);
    
  return 0;
}
