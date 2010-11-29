/*    
 *  launchordertaker.c
 *	Launches the ordertaker for the restaurant simulation
 *	
 */
 
#include "syscall.h"

int main()
{
  int i;
  int configParam;
  
  configParam = GetConfigArg();
  
  PrintOut("Will Exec ordertaker ", 21);
  PrintNumber(configParam);
  PrintOut(" times\n", 7);
  
  for(i=0; i < configParam; i++)
  {
    Exec("../test/ordertaker");  
  }

  Exit(0);
}
