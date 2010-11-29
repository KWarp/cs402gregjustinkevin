/*    
 *  teststartuserprogram.c
 *	Simple program to test whether running a user program works in the network directory.
 *	
 */
 
#include "syscall.h"

int main()
{
  int i;
  int configParam;

  PrintOut("===TestStartUserProgram===\n", 27);
  
  configParam = GetConfigArg();
  PrintOut("Will Exec teststartuserprogramsub ", 34);
  PrintNumber(configParam);
  PrintOut(" times\n", 7);
  
  for(i=0; i < configParam; i++)
  {
    Exec("../test/teststartuserprogramsub");  
  }

  PrintOut("===TestStartUserProgram Completed===\n", 37);
  Exit(0);
}
