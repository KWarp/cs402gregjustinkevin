/*    
 *  teststartuserprogram.c
 *	Simple program to test whether running a user program works in the network directory.
 *	
 */

#include "syscall.h"

int main()
{
  PrintOut("===TestStartUserProgram===\n", 27);
  
  PrintOut("Exec'ing teststartuserprogramsub 1\n", 35);
  Exec("../test/teststartuserprogramsub");

  PrintOut("Exec'ing teststartuserprogramsub 2\n", 35);
  Exec("../test/teststartuserprogramsub");

  PrintOut("===TestStartUserProgram Completed===\n", 37);
  Exit(0);
}
