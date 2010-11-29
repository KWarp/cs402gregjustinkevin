/* testp4syscalls.c
 *  Tests network syscalls.
 */
 
#ifndef NETWORK
  /* #define NETWORK */
#endif
 
#include "syscall.h"

int main(int argc, char **argv)
{
  int i;
  int configParam;

  PrintOut("===testp4syscalls===\n", 21);
  
  configParam = GetConfigArg();
  PrintOut("Will Exec testp4syscallssub ", 28);
  PrintNumber(configParam);
  PrintOut(" times\n", 7);
  
  for(i=0; i < configParam; i++)
  {
    Exec("../test/testp4syscallssub");  
  }
  
  Exit(0);
}

