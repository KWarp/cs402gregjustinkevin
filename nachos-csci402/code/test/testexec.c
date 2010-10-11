/* testexec.c
 *	Simple program to test the fork system call.
 */
 
#include "syscall.h"


int main()
{
  PrintOut("Press Ctrl-C once the threads are finished printing\n", 52);
  PrintOut("Exec'ing exectestprogram1\n", 26);
  Exec("../test/exectestprogram1");
  
  PrintOut("hereintestexec\n", 15);
  PrintOut("Exec'ing exectestprogram2\n", 26);
  Exec("../test/exectestprogram2");
  
  while (1)
    Yield();
    
  return 0;
}
