/*    
 *  launchcustomer.c
 *	Launches the customer for the restaurant simulation
 *	
 */
 
#include "syscall.h"

int main()
{
  int i;
  int configParam;
  
  configParam = GetConfigArg();
  
  PrintOut("Will Exec customer ", 19);
  PrintNumber(configParam);
  PrintOut(" times\n", 7);
  
  for(i=0; i < configParam; i++)
  {
    Exec("../test/customer");  
  }

  Exit(0);
}
