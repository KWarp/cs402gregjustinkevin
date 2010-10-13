/* testexec.c
 *	Simple program to test the fork system call.
 */
 
#include "syscall.h"


int main()
{
  PrintOut("Exec'ing exectestprogram1\n", 26);
  Exec("../test/exectestprogram1");
  
  PrintOut("Exec'ing exectestprogram2\n", 26);
  Exec("../test/exectestprogram2");
  
  Exit(0);
    
  return 0;
}
