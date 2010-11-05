/* testexec.c
 *	Simple program to test that two matmult programs 
 *  can run in memory at the same time
 *  without corrupting each other's memory
 */
 
#include "syscall.h"


int main()
{
  PrintOut("Exec'ing first matmult\n", 23);
  Exec("../test/matmult");
  
  PrintOut("Exec'ing second matmult\n", 23);
  Exec("../test/matmult");
  
  Exit(0);
    
  return 0;
}
